"""The front-end corpus: every `front_end_runner` invocation the checks against the original make.

D-0012 replaces a check against the original's capture by a comparison with main's native output
wherever the candidate's output equals main's on the check's own inputs. That needs the check's
inputs exactly: its pads, its pack (several checks pin an older pack), the race initialization
frames and record writes it derives from the original, its warm-boot image. Rather than
re-deriving them from forty scripts, a full gate run installs a shim in place of the candidate's
`front_end_runner` that logs each invocation and then runs the real binary, so the checks run
unchanged and the corpus is exactly what they ran.

An invocation is stored without its outputs: the replay asks both binaries for every output on
every frame, a superset of what any check reads, except pictures. Drawing a picture costs about
2 ms, so the replay draws the pictures on the frames the checks asked for (`--picture`, kept as
ranges) and compares those; everything else it compares on every frame. Input files (pads,
cartridge images) are stored by content in `store/`; packs by path and digest.

Layout under the corpus directory:
  store/<sha256>       input file contents
  <step>.jsonl         one line per invocation the step made (written while recording)
"""
from __future__ import annotations

import hashlib
import json
import os
import shutil
import sys
from pathlib import Path

# Options that name an input file whose contents are stored.
INPUT_FILES = {'--inputs', '--cartridge-in'}
# Input options with their value count.
INPUT_VALUES = {'--frames': 1, '--race-initialization': 1, '--short-demo-title': 1,
                '--reset-upload-delay': 1, '--record-write': 3, '--human-after': 1,
                '--restore-check': 1}
# Output options with their value count; replay replaces them by its own streams.
OUTPUTS = {'--race-timeline': 1, '--one-player-timeline': 1, '--look-timeline': 1,
           '--sound-cues': 1, '--picture': 2, '--records': 2, '--vram': 2,
           '--frame-digests': 1, '--rider-word': 1}

SHIM = """#!/bin/bash
# Gate corpus recorder (tools/unirally_lab/gates/corpus.py): log this invocation, then run the
# real runner. Installed only while a full gate run records the corpus.
if [ -n "${GATES_RECORD:-}" ]; then
  python3 -I "%(recorder)s" "$GATES_RECORD" "$PWD" "$@" 2>> "$GATES_RECORD.errors" || true
fi
exec "$0.real" "$@"
"""


def sha_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha_path(path: Path) -> str:
    h = hashlib.sha256()
    with path.open('rb') as f:
        for block in iter(lambda: f.read(1 << 20), b''):
            h.update(block)
    return h.hexdigest()


def normalize(argv: list[str], cwd: Path, store: Path | None) -> dict:
    """An invocation's inputs, without its outputs. With `store`, input files are copied there."""
    inputs: list = []
    outputs: list[str] = []
    pictures: set[int] = set()
    pack = None
    i = 0
    while i < len(argv):
        option = argv[i]
        if option == '--content-pack':
            path = (cwd / argv[i + 1]).resolve()
            pack = dict(path=str(path), sha256=sha_path(path))
            i += 2
        elif option in INPUT_FILES:
            data = (cwd / argv[i + 1]).read_bytes()
            digest = sha_bytes(data)
            if store is not None and not (store / digest).exists():
                tmp = store / f'.{digest}.{os.getpid()}'
                tmp.write_bytes(data)
                tmp.replace(store / digest)
            inputs += [option, '@' + digest]
            i += 2
        elif option in INPUT_VALUES:
            n = INPUT_VALUES[option]
            inputs += argv[i:i + 1 + n]
            i += 1 + n
        elif option in OUTPUTS:
            outputs.append(option)
            if option in ('--picture', '--vram'):
                pictures.add(int(argv[i + 1]))
            i += 1 + OUTPUTS[option]
        else:
            raise ValueError(f'unknown front_end_runner option {option!r}: teach corpus.py its kind')
    if pack is None:
        raise ValueError('invocation names no content pack')
    key = sha_bytes(json.dumps([pack['sha256'], inputs]).encode())
    return dict(key=key, pack=pack, inputs=inputs, outputs=sorted(set(outputs)), pictures=to_ranges(pictures))


def to_ranges(frames: set[int]) -> list[list[int]]:
    out: list[list[int]] = []
    for f in sorted(frames):
        if out and out[-1][1] == f - 1:
            out[-1][1] = f
        else:
            out.append([f, f])
    return out


def from_ranges(ranges: list[list[int]]) -> set[int]:
    return {f for a, b in ranges for f in range(a, b + 1)}


def record(log: Path, cwd: Path, argv: list[str]) -> None:
    store = log.parent / 'store'
    store.mkdir(parents=True, exist_ok=True)
    entry = normalize(argv, cwd, store)
    with log.open('a') as f:
        f.write(json.dumps(entry) + '\n')


SHIM_MARK = b'# Gate corpus recorder'


def is_shim(binary: Path) -> bool:
    return binary.exists() and SHIM_MARK in binary.read_bytes()[:200]


def install_shim(binary: Path) -> None:
    """Put the recorder in place of `binary`, the binary moving to `binary.real`. A binary the
    build rewrote since an earlier shim (a run that died) replaces the stale `.real`."""
    real = binary.with_name(binary.name + '.real')
    if not is_shim(binary):
        binary.replace(real)
    elif not real.exists():
        raise RuntimeError(f'{binary} is a recorder shim with no real binary beside it: rebuild')
    binary.write_text(SHIM % dict(recorder=Path(__file__).resolve()))
    binary.chmod(0o755)


def remove_shim(binary: Path) -> None:
    real = binary.with_name(binary.name + '.real')
    if is_shim(binary) and real.exists():
        real.replace(binary)


def real_binary(binary: Path) -> Path:
    """The runner itself, whether or not the recorder is installed."""
    return binary.with_name(binary.name + '.real') if is_shim(binary) else binary


def load(corpus: Path) -> tuple[dict[str, dict], dict[str, set[str]]]:
    """Distinct cases by key, and for each step the keys it ran."""
    cases: dict[str, dict] = {}
    by_step: dict[str, set[str]] = {}
    for log in sorted(corpus.glob('*.jsonl')):
        step = log.stem
        keys = by_step.setdefault(step, set())
        for line in log.read_text().splitlines():
            entry = json.loads(line)
            case = cases.setdefault(entry['key'], dict(entry, steps=set(), outputs=set(), picture_frames=set()))
            case['steps'].add(step)
            case['outputs'].update(entry['outputs'])
            case['picture_frames'].update(from_ranges(entry.get('pictures', [])))
            keys.add(entry['key'])
    return cases, by_step


def clear_step(corpus: Path, step: str) -> Path:
    """Start a step's log afresh; returns its path."""
    corpus.mkdir(parents=True, exist_ok=True)
    log = corpus / f'{step}.jsonl'
    log.unlink(missing_ok=True)
    (corpus / f'{step}.jsonl.errors').unlink(missing_ok=True)
    return log


def copy_corpus(source: Path, target: Path) -> None:
    shutil.copytree(source, target, dirs_exist_ok=True)


if __name__ == '__main__':
    # The shim's entry point: corpus.py LOG CWD ARGV...
    record(Path(sys.argv[1]), Path(sys.argv[2]), sys.argv[3:])
