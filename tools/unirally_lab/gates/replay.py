"""Replay the front-end corpus through main's runner and the candidate's, comparing every output.

Each case runs on both binaries with every output the runner has, on every frame: the state rows
(stdout), stderr, the race, one-player and look timelines, the sound cues, and digests of the
picture, cartridge RAM and VRAM (`--frame-digests`). Outputs are read from pipes and named pipes as
they are written and reduced to one 8-byte digest per line, so nothing is stored: a 102,000-frame
run's 500 MB of rows costs 800 KB of memory and no disk. A difference is reported at its first
line, with the frame that line names.

A case recorded on the gates' current pack (`$PACK` when the corpus was recorded) runs the
candidate on the candidate's pack and main on main's, as the gates always have; a case a check
pinned to an older pack runs both on that pack.
"""
from __future__ import annotations

import hashlib
import json
import os
import subprocess
import tempfile
import threading
import time
from concurrent.futures import ThreadPoolExecutor
from dataclasses import dataclass
from pathlib import Path

STREAMS = ('race-timeline', 'one-player-timeline', 'look-timeline', 'sound-cues', 'frame-digests')


def line_digests(stream) -> list[bytes]:
    out = []
    for line in stream:
        out.append(hashlib.blake2b(line, digest_size=8).digest())
    return out


@dataclass
class Side:
    binary: Path
    pack: Path


def run_side(side: Side, case: dict, store: Path, streams: tuple[str, ...], tmp: Path, timeout: int) -> dict:
    """Run one binary on a case; every stream reduced to line digests."""
    fifos = {name: tmp / name for name in streams}
    for path in fifos.values():
        os.mkfifo(path)
    args = [str(side.binary), '--content-pack', str(side.pack)]
    inputs = case['inputs']
    i = 0
    while i < len(inputs):
        value = inputs[i]
        args.append(value if not value.startswith('@') else str(store / value[1:]))
        i += 1
    for name, path in fifos.items():
        args += [f'--{name}', str(path)]
    results: dict[str, list[bytes]] = {}
    opened: dict[str, bool] = {name: False for name in fifos}

    def read(name: str, path: Path) -> None:
        with open(path, 'rb') as f:
            opened[name] = True
            results[name] = line_digests(f)

    threads = [threading.Thread(target=read, args=(n, p), daemon=True) for n, p in fifos.items()]
    for t in threads:
        t.start()
    started = time.time()
    proc = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out_thread_result: dict[str, list[bytes]] = {}
    err: list[bytes] = []
    t_out = threading.Thread(target=lambda: out_thread_result.update(stdout=line_digests(proc.stdout)), daemon=True)
    t_err = threading.Thread(target=lambda: err.append(proc.stderr.read()), daemon=True)
    t_out.start(); t_err.start()
    try:
        rc = proc.wait(timeout=timeout)
    except subprocess.TimeoutExpired:
        proc.kill(); rc = proc.wait()
        err.append(b'\n[gates] timed out')
    t_out.join(); t_err.join()
    # A runner that failed before opening a stream leaves its reader waiting: open the writing end
    # once so the reader sees an empty stream.
    for name, path in fifos.items():
        if not opened[name]:
            try:
                fd = os.open(path, os.O_WRONLY | os.O_NONBLOCK)
                os.close(fd)
            except OSError:
                pass
    for t in threads:
        t.join(timeout=30)
    for path in fifos.values():
        path.unlink(missing_ok=True)
    results.update(out_thread_result)
    stderr = b''.join(err)
    return dict(rc=rc, stderr=stderr, streams=results, seconds=round(time.time() - started, 2))


def first_difference(a: list[bytes], b: list[bytes]) -> int | None:
    for k, (x, y) in enumerate(zip(a, b)):
        if x != y:
            return k
    return None if len(a) == len(b) else min(len(a), len(b))


def compare_case(key: str, case: dict, base: Side, candidate: Side, current_pack_sha: str,
                 store: Path, streams: tuple[str, ...], timeout: int) -> dict:
    if case['pack']['sha256'] == current_pack_sha:
        base_side, cand_side = base, candidate
    else:
        pinned = Path(case['pack']['path'])
        base_side, cand_side = Side(base.binary, pinned), Side(candidate.binary, pinned)
    with tempfile.TemporaryDirectory(prefix='gates-replay-') as tmp:
        tb, tc = Path(tmp) / 'b', Path(tmp) / 'c'
        tb.mkdir(); tc.mkdir()
        with ThreadPoolExecutor(2) as pool:
            fb = pool.submit(run_side, base_side, case, store, streams, tb, timeout)
            fc = pool.submit(run_side, cand_side, case, store, streams, tc, timeout)
            b, c = fb.result(), fc.result()
    compared = {}
    for name in ('stdout',) + streams:
        x, y = b['streams'].get(name, []), c['streams'].get(name, [])
        compared[name] = dict(lines=len(y), base_lines=len(x), first_difference=first_difference(x, y))
    differing = [n for n, s in compared.items() if s['first_difference'] is not None]
    same = not differing and b['rc'] == c['rc'] and b['stderr'] == c['stderr']
    row = dict(key=key, steps=sorted(case['steps']), equal=same, rc=[b['rc'], c['rc']],
               frames=_frames(case), seconds=[b['seconds'], c['seconds']], differing_streams=differing)
    if differing:
        row['first_differences'] = {n: compared[n] for n in differing}
    if b['stderr'] != c['stderr']:
        row['stderr'] = [b['stderr'].decode(errors='replace')[-300:], c['stderr'].decode(errors='replace')[-300:]]
    return row


def _frames(case: dict) -> int:
    inputs = case['inputs']
    return int(inputs[inputs.index('--frames') + 1]) if '--frames' in inputs else 0


def supports(binary: Path, pack: Path, option: list[str]) -> bool:
    """Whether a runner accepts an output option (main may predate it)."""
    with tempfile.TemporaryDirectory() as tmp:
        args = [str(binary), '--content-pack', str(pack), '--frames', '1'] + [a.replace('{tmp}', tmp) for a in option]
        return subprocess.run(args, capture_output=True).returncode == 0


def replay(cases: dict[str, dict], store: Path, base: Side, candidate: Side, current_pack_sha: str,
           jobs: int, timeout: int = 1800, progress=None) -> dict:
    """`current_pack_sha`: the pack the corpus was recorded on as the gates' `$PACK`."""
    streams = STREAMS
    # Outputs only newer runners have are compared once both sides have them.
    for name in ('rider-word',):
        option = [f'--{name}', '{tmp}/x']
        if supports(base.binary, base.pack, option) and supports(candidate.binary, candidate.pack, option):
            streams += (name,)
    if not supports(base.binary, base.pack, ['--frame-digests', '{tmp}/d']):
        raise RuntimeError(f"main's runner {base.binary} has no --frame-digests: build main with D-0012's runner")
    started = time.time()
    rows = []
    # Longest cases first, so the pool does not end on one long run.
    order = sorted(cases.items(), key=lambda kv: -_frames(kv[1]))
    with ThreadPoolExecutor(max(1, jobs // 2)) as pool:
        futures = [pool.submit(compare_case, k, c, base, candidate, current_pack_sha, store, streams, timeout) for k, c in order]
        for n, f in enumerate(futures, 1):
            rows.append(f.result())
            if progress:
                progress(n, len(futures))
    differing = [r for r in rows if not r['equal']]
    steps_differing = sorted({s for r in differing for s in r['steps']})
    summary = dict(cases=len(rows), frames=sum(r['frames'] for r in rows), equal=len(rows) - len(differing),
                   differing=len(differing), steps_with_differences=steps_differing,
                   streams=['stdout', 'stderr', *streams], seconds=round(time.time() - started, 1))
    return dict(summary=summary, rows=rows)


def main() -> int:
    import argparse
    from .corpus import load
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--corpus', type=Path, required=True)
    p.add_argument('--base', type=Path, required=True, help="main's front_end_runner")
    p.add_argument('--base-pack', type=Path, required=True)
    p.add_argument('--candidate', type=Path, required=True)
    p.add_argument('--candidate-pack', type=Path, required=True)
    p.add_argument('--jobs', type=int, default=8)
    p.add_argument('--steps', help='only cases these comma-separated steps ran')
    p.add_argument('--out', type=Path, required=True)
    a = p.parse_args()
    cases, by_step = load(a.corpus)
    current = json.loads((a.corpus / 'meta.json').read_text())['current_pack_sha256']
    if a.steps:
        wanted = set().union(*(by_step.get(s, set()) for s in a.steps.split(',')))
        cases = {k: v for k, v in cases.items() if k in wanted}
    result = replay(cases, a.corpus / 'store', Side(a.base.resolve(), a.base_pack.resolve()),
                    Side(a.candidate.resolve(), a.candidate_pack.resolve()), current, a.jobs,
                    progress=lambda n, t: print(f'{n}/{t}', flush=True) if n % 25 == 0 or n == t else None)
    a.out.write_text(json.dumps(result, indent=1) + '\n')
    print(json.dumps(result['summary']))
    print('status=' + ('passed' if result['summary']['differing'] == 0 else 'differs'))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
