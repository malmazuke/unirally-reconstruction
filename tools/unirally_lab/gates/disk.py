"""Temporary disk: a private directory per step, a cap on its size, and a free-space floor.

The gates' comparison scripts render pictures into `$TMPDIR`; one step held 2.2 GB, and on 26
September 2026 leaked temp directories filled the disk. Each step here gets its own `TMPDIR`
under the run's temp root, which is measured while the step runs and deleted when it ends. A step
whose directories pass its cap is stopped and fails; a step does not start while free space is
under the floor.
"""
from __future__ import annotations

import os
import shutil
from pathlib import Path

GB = 1 << 30


def free_bytes(path: Path) -> int:
    return shutil.disk_usage(path).free


def tree_bytes(path: Path) -> int:
    """Allocated bytes under `path` (blocks, as `du` counts), not following links."""
    total = 0
    stack = [path]
    while stack:
        here = stack.pop()
        try:
            with os.scandir(here) as entries:
                for entry in entries:
                    try:
                        st = entry.stat(follow_symlinks=False)
                    except FileNotFoundError:
                        continue
                    if entry.is_dir(follow_symlinks=False):
                        stack.append(Path(entry.path))
                    else:
                        total += st.st_blocks * 512
        except (FileNotFoundError, NotADirectoryError, PermissionError):
            continue
    return total


def fmt(n: int) -> str:
    return f'{n / GB:.2f} GB' if n >= GB // 10 else f'{n / (1 << 20):.0f} MB'
