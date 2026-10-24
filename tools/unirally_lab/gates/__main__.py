"""The gate runner (D-0012).

  local/locks/heavy-run.sh TASK python3 -m tools.unirally_lab.gates run [options]

From the candidate's checkout (with `local` linked to the main checkout's): builds main's binaries
at the merge base and the candidate's presets, decides from ninja's recorded inputs which binaries
the change can alter, and runs the steps of `catalogue.py` that those binaries' changes call for,
in parallel where they do not contend. Prints one line per step and writes `report.json`.

  --full      every step and every check against the original (the periodic and pre-milestone
              run)
  --record    with --full: record the front-end corpus from the checks' own invocations
  --reproject the frozen race gates re-project both captures of the original instead of checking
              the contract's frozen digest (always when the projection or its guards changed)
  --task-checks FILE   the task's own checks (run as `SHELL FILE OUTDIR`, the shell from its first
              line); repeatable. A file of commands, one per line, named *.list, runs each line
              as its own step, in parallel; a line ending `# cap=GB` gets that temp cap (1 GB
              otherwise).
"""
from __future__ import annotations

import argparse
import json
import os
import shutil
import signal
import subprocess
import sys
import tempfile
import time
from pathlib import Path

from . import catalogue, corpus
from .base import main_checkout
from .disk import GB, fmt, free_bytes
from .identity import identify
from .schedule import Run, write_report


def git(root: Path, *args: str) -> str:
    return subprocess.check_output(['git', *args], cwd=root, text=True).strip()


def task_steps(paths: list[str]) -> list[tuple[str, str, str]]:
    out = []
    for raw in paths:
        path = Path(raw).resolve()
        name = path.stem if path.suffix == '.list' else path.parent.name
        if path.suffix == '.list':
            for n, line in enumerate(l for l in path.read_text().splitlines() if l.strip() and not l.startswith('#')):
                # A line may end with `# cap=GB`: its temp cap, when it keeps more than 1 GB.
                command, _, note = line.partition('# cap=')
                cap = int(float(note) * GB) if note else GB
                out.append((f'checks-{name}-{n + 1:02d}', command.strip(), 'zsh' if command.startswith('zsh ') else 'bash', cap))
        else:
            first = path.read_text().split('\n', 1)[0]
            shell = 'zsh' if 'zsh' in first else 'bash'
            out.append((f'checks-{name}', f'{shell} "{path}" "$G/out" > "$G/checks.out" 2>&1; echo "  rc=$?"; grep -E "^  " "$G/checks.out" | cut -c1-240', 'bash', 3 * GB))
    return out


def main() -> int:
    sys.stdout.reconfigure(line_buffering=True)
    # A kill from outside (a watchdog, a session) still removes the shim and the temp root.
    signal.signal(signal.SIGTERM, lambda *_: (_ for _ in ()).throw(KeyboardInterrupt()))
    p = argparse.ArgumentParser(prog='python3 -m tools.unirally_lab.gates', description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('command', choices=['run', 'plan'])
    p.add_argument('--base', help='main\'s commit (default: the merge base with origin/main)')
    p.add_argument('--pack', help="the candidate's pack (default: the newest local/classic-pal-crawler-tracks-v4N.pack)")
    p.add_argument('--base-pack', help="main's pack (default: the candidate's)")
    p.add_argument('--full', action='store_true')
    p.add_argument('--record', action='store_true')
    p.add_argument('--reproject', action='store_true')
    p.add_argument('--task-checks', action='append', default=[])
    p.add_argument('--jobs', type=int, default=max(1, (os.cpu_count() or 8) - 4))
    p.add_argument('--out', help='report directory (default artifacts/gates/<head>)')
    p.add_argument('--disk-floor', type=float, default=4.0, help='GB of free disk kept beyond running steps\' caps')
    p.add_argument('--temp-cap', type=float, default=6.0, help='GB: running steps\' caps may not sum past this')
    p.add_argument('--only', help='comma-separated step ids (with build-base and build)')
    p.add_argument('--skip', help="comma-separated step ids left out")
    p.add_argument('--expect-changed', default='', help="comma-separated checks whose results the task means to change, "
                   "read and recorded: `changed (expected)` does not fail the run")
    p.add_argument('--force', help="comma-separated step ids that always run: a task's own checks already in the catalogue")
    a = p.parse_args()

    root = Path(git(Path.cwd(), 'rev-parse', '--show-toplevel'))
    main_root = main_checkout(root)
    evidence = main_root / 'local' / 'evidence'
    if not (root / 'local').exists():
        print(f'refused: {root}/local is missing; link it to {main_root}/local'); return 2
    head = git(root, 'rev-parse', 'HEAD')
    base = git(root, 'rev-parse', a.base) if a.base else git(root, 'merge-base', 'HEAD', 'origin/main')
    packs = sorted(root.glob('local/classic-pal-crawler-tracks-v4[0-9].pack'), key=lambda x: int(x.stem.rsplit('-v', 1)[1]))
    pack = Path(a.pack).resolve() if a.pack else packs[-1].resolve()
    base_pack = Path(a.base_pack).resolve() if a.base_pack else pack
    out = Path(a.out).resolve() if a.out else root / 'artifacts' / 'gates' / head[:10]
    corpus_dir = main_root / 'local' / 'gates' / 'corpus'
    fer = root / 'build' / 'lab-release' / 'src' / 'core' / 'front_end_runner'
    _, by_step = corpus.load(corpus_dir) if corpus_dir.exists() else ({}, {})

    changed = subprocess.check_output(['git', 'diff', '--name-only', base], cwd=root, text=True).split()
    if a.command == 'run' and not a.full and changed and all(f.startswith(('docs/', 'tasks/')) or f.endswith('.md') for f in changed):
        print('only records changed: no local gates (CI\'s docs-only fast path covers them)'); return 0

    task = task_steps(a.task_checks)
    if a.command == 'plan':
        print('the steps, in order (which run is decided after the builds):')
        for s in catalogue.steps(a.jobs, task):
            print(f'  {s.id:28s} {s.lane:9s} {s.system:9s} {s.title}')
        return 0
    if out.exists():
        shutil.rmtree(out)
    out.mkdir(parents=True)
    temp_root = Path(tempfile.mkdtemp(prefix='gates-'))
    steps = catalogue.steps(a.jobs, task)
    if a.only:
        keep = set(a.only.split(',')) | {'build-base', 'build'}
        keep |= {k + '@main' for k in keep}
        steps = [s for s in steps if s.id in keep]
    if a.skip:
        steps = [s for s in steps if s.id not in set(a.skip.split(','))]
    kept = {s.id for s in steps}
    steps = [s for s in steps if not s.twin or s.twin in kept]      # a twin goes with its step
    for s in steps:
        if a.force and s.id.split('@')[0] in a.force.split(','):
            s.select = lambda run: (True, "the task's own checks (--force)")
    # The checks get the current pack through an alias, so the corpus tells `$PACK` from a pinned
    # pack of the same version.
    alias = out / 'pack' / pack.name
    alias.parent.mkdir(parents=True, exist_ok=True)
    alias.symlink_to(pack)
    env = dict(os.environ, W=str(root), L=str(evidence), PACK=str(alias), GATES_CURRENT_PACK=str(alias),
               BASE_PACK=str(base_pack), B=str(out / 'base-bin'),
               BASE_SHA=base, CORPUS=str(corpus_dir), JOBS=str(a.jobs), RUN=str(out), FER=str(fer),
               BASE_TREE=str(main_root / '.worktrees' / 'gates-base'))
    context = dict(full=a.full, record=a.full and a.record, corpus_steps=by_step, identity=None, replay=None,
                   expect_changed=set(filter(None, a.expect_changed.split(','))))
    def corpus_log(step):
        corpus.write_meta(corpus_dir, step.id, fingerprint=corpus.fingerprint(step.command, evidence), base=base,
                          recorded_at=head)
        return corpus.clear_step(corpus_dir, step.id)

    def corpus_lines(step, lines):
        (corpus_dir / f'{step.id}.lines').write_text('\n'.join(lines) + '\n')

    def recorded_lines(step_id):
        meta = corpus.read_meta(corpus_dir, step_id)
        path = corpus_dir / f'{step_id}.lines'
        # Only a recording made on this same main says what the check should print here.
        if meta.get('recorded_at') != base or not path.exists():
            return None
        return path.read_text().splitlines()

    context.update(corpus_log=corpus_log, corpus_lines=corpus_lines, recorded_lines=recorded_lines)
    # A check whose command or scripts changed since its runs were recorded, or whose recording
    # logged an error, has no corpus to stand on.
    stale = {}
    for s in steps:
        if not s.record or s.system == 'task' or '@' in s.id:
            continue
        meta = corpus.read_meta(corpus_dir, s.id)
        if (corpus_dir / f'{s.id}.jsonl.errors').exists() and (corpus_dir / f'{s.id}.jsonl.errors').stat().st_size:
            stale[s.id] = 'its recording logged errors'
        elif s.id in by_step and meta.get('fingerprint') != corpus.fingerprint(s.command, evidence):
            stale[s.id] = 'its command or scripts changed since its runs were recorded' if meta else 'its runs carry no fingerprint'
    context['stale'] = stale
    if context['record']:
        corpus_dir.mkdir(parents=True, exist_ok=True)
        (corpus_dir / 'meta.json').write_text(json.dumps(dict(current_pack=str(pack), current_pack_sha256=corpus.sha_path(pack),
                                                              recorded_at=head, base=base)) + '\n')
    print(f'gates: candidate {head[:10]} base {base[:10]} pack {pack.name} (main {base_pack.name}); {len(changed)} files changed; '
          f'{"full run" if a.full else "selective"}{", recording the corpus" if context["record"] else ""}; jobs {a.jobs}')
    print(f'  free disk {fmt(free_bytes(out))}; temp root {temp_root}; report {out}')

    run = Run(steps, env, root, out, temp_root, a.jobs, int(a.disk_floor * GB), int(a.temp_cap * GB), context)

    def after_build(run: Run, outcome) -> None:
        ident = identify(root, base, str(root / 'local/toolchain/ninja-1.13.2-darwin-arm64/ninja'), pack, base_pack)
        run.context['identity'] = ident
        print(f'  identity: {len(ident.changed_files)} files changed, pack {"changed" if ident.pack_changed else "unchanged"}')
        for name, b in ident.binaries.items():
            print(f'    {name:34s} {"CHANGED" if b["changed"] else "same   "} {b["reason"]}')
        if a.reproject or any(f.startswith(catalogue.FROZEN_TRUSTS) for f in ident.changed_files):
            run.env['FROZEN_FULL'] = '1'
        if fer.exists():
            corpus.install_shim(fer)
            run.env['FER'] = str(corpus.real_binary(fer))

    def after_replay(run: Run, outcome) -> None:
        path = out / 'fe-replay' / 'replay.json'
        if path.exists():
            run.context['replay'] = json.loads(path.read_text())
            print(f'  replay: {json.dumps(run.context["replay"]["summary"])[:400]}')

    run.callbacks['build'] = after_build
    run.callbacks['fe-replay'] = after_replay
    try:
        report = run.go()
    finally:
        corpus.remove_shim(fer)
        shutil.rmtree(temp_root, ignore_errors=True)
    ident = run.context['identity']
    report.update(candidate=head, base=base, pack=str(pack), base_pack=str(base_pack), full=a.full, record=context['record'],
                  jobs=a.jobs, identity=None if ident is None else dict(changed_files=ident.changed_files,
                  pack_changed=ident.pack_changed, binaries=ident.binaries),
                  replay_summary=(run.context.get('replay') or {}).get('summary'))
    write_report(out / 'report.json', report)
    bad = [s for s in report['steps'] if s['status'] in ('failed', 'timeout', 'over-cap', 'stalled', 'changed', 'broken')]
    print(f'gates done in {report["seconds"] / 60:.1f} min; peak step temp {fmt(report["peak_temp"])}; '
          f'peak disk used {fmt(max(0, report["disk_used_peak"]))}; lowest free {fmt(report["min_free"])}')
    for s in report['steps']:
        print(f'  {s["id"]:30s} {s["status"]:8s} {s["seconds"]:7.0f} s  {fmt(s["peak_temp"]):>8s}  {s["why"]}')
    print('status=' + ('failed' if bad else 'passed') + (f' ({", ".join(s["id"] for s in bad)})' if bad else ''))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
