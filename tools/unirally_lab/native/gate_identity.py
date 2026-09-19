"""Decide whether the differential gates can be cited instead of re-run.

Every differential compare drives one binary, `zoom_zoo_runner`. It links
`unirally_movement` only and contains no presentation symbol, so a change that
touches presentation alone cannot alter a single row it emits and the compares
would replay exactly as they did.

This checks that rather than asserting it, and it checks the *inputs*: ninja
records every source and header each object was compiled from, so the engine's
own file set is known exactly. If each of those repository files is byte
identical between the commit whose gate reports are being cited and this tree,
the reports still describe this tree. Comparing the built binaries instead would
be wrong across checkouts: a debug build embeds its absolute path, so the same
sources in two worktrees produce two hashes.

Toolchain and system headers are deliberately out of scope here; the gate
reports pin the pack and contract hashes, and the build preset pins the
compiler.
"""
from __future__ import annotations
import argparse, hashlib, json, subprocess, sys
from pathlib import Path

OBJECTS = ('src/core/CMakeFiles/unirally_movement.dir/movement.cpp.o',
           'src/core/CMakeFiles/unirally_movement.dir/content_pack.cpp.o',
           'src/core/CMakeFiles/zoom_zoo_runner.dir/zoom_zoo_runner.cpp.o')


def engine_files(build: Path, root: Path, ninja: str) -> list[str]:
    """The repository files ninja recorded as inputs of the gate binary."""
    seen: set[str] = set()
    for object_path in OBJECTS:
        out = subprocess.run([ninja, '-C', str(build), '-t', 'deps', object_path],
                             capture_output=True, text=True)
        for line in out.stdout.splitlines():
            name = line.strip()
            if not name.startswith(str(root)):
                continue
            relative = str(Path(name).relative_to(root))
            if relative.startswith('build/'):
                continue
            seen.add(relative)
    return sorted(seen)


def at_commit(root: Path, commit: str, path: str) -> bytes | None:
    out = subprocess.run(['git', 'show', f'{commit}:{path}'], cwd=root, capture_output=True)
    return out.stdout if out.returncode == 0 else None


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--since', required=True, help='the commit whose gate reports would be cited')
    p.add_argument('--reports', type=Path, help='that run\'s report directory, to name what is cited')
    p.add_argument('--build', type=Path, default=Path('build/app-debug'))
    p.add_argument('--root', type=Path, default=Path('.'))
    p.add_argument('--ninja', default='ninja')
    a = p.parse_args()
    root = a.root.resolve()

    files = engine_files(a.build, root, a.ninja)
    if not files:
        print('no recorded dependencies; build the preset first')
        return 1
    changed = [f for f in files if at_commit(root, a.since, f) != (root/f).read_bytes()]
    head = subprocess.check_output(['git', 'rev-parse', '--short', 'HEAD'], cwd=root, text=True).strip()
    print(f'{len(files)} repository files build zoom_zoo_runner; comparing {a.since} with {head}')
    if changed:
        print('the differential gates must run; these inputs differ:')
        for f in changed:
            print('  ' + f)
        return 1
    print('every input is byte identical, so the differential gates at that commit still hold')
    if a.reports:
        for report in sorted(a.reports.glob('*.json')):
            text = report.read_text()
            if 'restore_frames' not in text:
                continue
            d = json.loads(text)
            print(f'  cite {report.stem:42s} {d["status"]}, {len(d["restore_frames"])} restores,'
                  f' binary {d["binary_sha256"][:12]}')
    return 0


if __name__ == '__main__':
    sys.exit(main())
