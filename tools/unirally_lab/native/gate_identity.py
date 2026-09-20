"""Decide whether the differential gates can be cited instead of re-run.

Every differential compare drives one binary, `zoom_zoo_runner`, which carries
no presentation symbol, so a change that touches presentation alone cannot
alter a row it emits and the compares would replay exactly as they did.

This checks that rather than asserting it, and it checks the *inputs*: ninja
names every object linked into the binary and records every source and header
each object was compiled from, so the engine's own file set is known exactly
rather than guessed. If each of those repository files is byte identical
between the commit whose gate reports are cited and this tree, those reports
still describe this tree.

Two deliberate choices. It compares inputs, not the built binary: a debug build
embeds its absolute path, so identical sources in two checkouts produce two
hashes. And every query that fails, every report that does not match, and every
uncommitted tracked change is a refusal, because a check that skips 35 minutes
of replays has to fail closed - an earlier version of this tool derived the
input set from three hardcoded objects, missed the contact, speed-limit and
sampling engine, and reported "identical" across an edit to `track_sampling.cpp`
that moved the gate's own rows hash (review E1).

Out of scope, and not claimed: the toolchain, system headers, the CMake files
and presets that decide how the binary is built, and anything about the
binary's *behaviour* beyond the bytes of its inputs. The build preset pins the
compiler and each cited report pins its pack and contract hashes.
"""
from __future__ import annotations
import argparse, hashlib, json, subprocess, sys
from pathlib import Path

BINARY_TARGET = 'src/core/zoom_zoo_runner'
# sha256 of an empty `git diff HEAD`, which every compare records when its tree
# was a commit.
EMPTY_DIFF = 'e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855'


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def ninja(args: list[str], build: Path, tool: str) -> list[str]:
    out = subprocess.run([tool, '-C', str(build), '-t', *args], capture_output=True, text=True)
    if out.returncode:
        raise RuntimeError(f'ninja -t {" ".join(args)} failed: {out.stderr.strip()[:200]}')
    return out.stdout.splitlines()


def object_is_current(build: Path, root: Path, object_path: str, inputs: list[str]) -> None:
    """Refuse a dependency record that predates the files it claims to describe.

    `ninja -n` cannot answer this here: the project's CMake globs re-check on
    every invocation, so it always reports work to do. Comparing modification
    times answers the question directly - an input newer than the object that
    recorded it means the record describes an older tree (review F1).
    """
    built = (build/object_path)
    if not built.exists():
        raise RuntimeError(f'{object_path} has not been built; build the preset')
    stamp = built.stat().st_mtime
    stale = [name for name in inputs if (root/name).exists() and (root/name).stat().st_mtime > stamp]
    if stale:
        raise RuntimeError(f'{object_path} is older than {len(stale)} of its inputs '
                           f'({stale[0]}); build the preset')


def engine_files(build: Path, root: Path, tool: str) -> list[str]:
    objects = [line.strip() for line in ninja(['inputs', BINARY_TARGET], build, tool)
               if line.strip().endswith('.o')]
    if not objects:
        raise RuntimeError(f'ninja named no objects for {BINARY_TARGET}')
    seen: set[str] = set()
    for object_path in objects:
        lines = ninja(['deps', object_path], build, tool)
        if not any('#deps' in line and 'VALID' in line for line in lines):
            raise RuntimeError(f'{object_path} has no valid recorded dependencies; rebuild the preset')
        own: list[str] = []
        for line in lines:
            name = line.strip()
            if not name.startswith(str(root)):
                continue
            relative = str(Path(name).relative_to(root))
            if not relative.startswith('build/'):
                own.append(relative)
        object_is_current(build, root, object_path, own)
        seen.update(own)
    return sorted(seen), objects


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--since', required=True, help='the commit whose gate reports would be cited')
    p.add_argument('--reports', type=Path, required=True, help='that run\'s report directory')
    p.add_argument('--expect', type=int, required=True,
                   help='how many differential gates must be cited; a thinned directory is a refusal')
    p.add_argument('--pack', type=Path, required=True)
    p.add_argument('--build', type=Path, default=Path('build/app-debug'))
    p.add_argument('--root', type=Path, default=Path('.'))
    p.add_argument('--ninja', default='ninja')
    a = p.parse_args()
    root = a.root.resolve()

    if subprocess.run(['git', 'diff', '--quiet', 'HEAD'], cwd=root).returncode:
        print('refused: the working tree has uncommitted tracked changes, so it is not a commit')
        return 1
    head = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=root, text=True).strip()
    since = subprocess.check_output(['git', 'rev-parse', a.since], cwd=root, text=True).strip()

    try:
        files, objects = engine_files(a.build, root, a.ninja)
    except RuntimeError as error:
        print(f'refused: {error}')
        return 1

    changed, missing = [], []
    for name in files:
        recorded = subprocess.run(['git', 'show', f'{since}:{name}'], cwd=root, capture_output=True)
        if recorded.returncode:
            missing.append(name)
        elif not (root/name).exists():
            missing.append(f'{name} (present at the cited commit, gone from this tree)')
        elif recorded.stdout != (root/name).read_bytes():
            changed.append(name)
    print(f'{len(objects)} objects, {len(files)} repository files build {BINARY_TARGET}')
    print(f'comparing {since[:7]} with {head[:7]}')
    if changed or missing:
        print('refused: the differential gates must run')
        for name in changed:
            print(f'  changed since {since[:7]}: {name}')
        for name in missing:
            print(f'  absent at {since[:7]}, so new to this tree: {name}')
        return 1

    pack = sha(a.pack.read_bytes())
    cited, problems = [], []
    for report in sorted(a.reports.glob('*.json')):
        text = report.read_text()
        if 'restore_frames' not in text:
            continue
        d = json.loads(text)
        name = report.stem
        if d.get('status') != 'passed':
            problems.append(f'{name}: status {d.get("status")}')
        if d.get('source_commit') != since:
            problems.append(f'{name}: ran at {d.get("source_commit", "?")[:7]}, not {since[:7]}')
        if d.get('pack_sha256') != pack:
            problems.append(f'{name}: ran against a different pack')
        if d.get('source_diff_sha256') != EMPTY_DIFF:
            problems.append(f'{name}: ran on a tree with uncommitted changes')
        contracts = (root/'tests/manifests/native')
        if not any(sha(c.read_bytes()) == d.get('contract_sha256') for c in contracts.glob('*.json')):
            problems.append(f'{name}: its frozen contract is not in the tree unchanged')
        cited.append((name, len(d.get('restore_frames', []))))
    if len(cited) != a.expect:
        problems.append(f'{len(cited)} differential gate reports in {a.reports}, expected {a.expect}')
    if problems:
        print('refused: the reports do not describe this tree')
        for problem in problems:
            print('  ' + problem)
        return 1
    print(f'every input is byte identical and every report matches; {len(cited)} gates may be cited:')
    for name, restores in cited:
        print(f'  {name:42s} passed, {restores} restores')
    return 0


if __name__ == '__main__':
    sys.exit(main())
