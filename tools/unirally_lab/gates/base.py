"""Main's binaries for the native-against-main comparisons, built from the base commit.

One persistent worktree, `.worktrees/gates-base` in the main checkout, is moved to the base commit
and built incrementally (lab-release), so a run after the previous task's pays only for what
changed. The binaries are linked into the run's `B` directory; they load their DSP library from
that build tree, which stays in place for the run.
"""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

BINARIES = {'front_end_runner': 'src/core/front_end_runner', 'zoom_zoo_runner': 'src/core/zoom_zoo_runner',
            'classic_race_presentation_runner': 'src/core/classic_race_presentation_runner',
            'movement_runner': 'src/core/movement_runner',
            'audio_driver_runner': 'tests/native/audio_driver_runner'}


def main_checkout(root: Path) -> Path:
    common = subprocess.check_output(['git', 'rev-parse', '--path-format=absolute', '--git-common-dir'], cwd=root, text=True).strip()
    return Path(common).parent


def build(sha: str, out: Path, root: Path) -> int:
    main = main_checkout(root)
    tree = main / '.worktrees' / 'gates-base'
    if not tree.exists():
        subprocess.run(['git', 'worktree', 'add', '--detach', str(tree), sha], cwd=main, check=True)
    else:
        subprocess.run(['git', 'checkout', '--quiet', '--detach', '--force', sha], cwd=tree, check=True)
    head = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=tree, text=True).strip()
    full = subprocess.check_output(['git', 'rev-parse', sha], cwd=root, text=True).strip()
    if head != full:
        print(f'base worktree is at {head}, not {full}'); return 1
    link = tree / 'local'
    if not link.exists():
        link.symlink_to('../../local')
    log = out / 'build-lab-release.log'
    out.mkdir(parents=True, exist_ok=True)
    with log.open('w') as f:
        subprocess.run([sys.executable, 'tools/project.py', 'build', '--preset', 'lab-release'], cwd=tree, stdout=f, stderr=subprocess.STDOUT)
    status = [l for l in log.read_text().splitlines() if 'status=' in l]
    print(f'  base {full[:10]} lab-release build {status[-1] if status else "status=?"}')
    if not status or 'passed' not in status[-1]:
        return 1
    for name, rel in BINARIES.items():
        target = out / name
        target.unlink(missing_ok=True)
        target.symlink_to(tree / 'build' / 'lab-release' / rel)
    (out / 'SOURCE-COMMIT.txt').write_text(full + '\n')
    return 0


if __name__ == '__main__':
    if len(sys.argv) != 4 or sys.argv[1] != 'build':
        print('usage: python3 -m tools.unirally_lab.gates.base build SHA OUTDIR'); raise SystemExit(2)
    raise SystemExit(build(sys.argv[2], Path(sys.argv[3]), Path.cwd()))
