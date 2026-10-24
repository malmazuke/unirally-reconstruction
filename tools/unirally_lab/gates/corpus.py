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
            # The runner hands the checks the current pack through an alias, so a check that pins
            # a pack (even the same version) is told apart from one that takes `$PACK`.
            given = os.path.abspath(cwd / argv[i + 1])
            path = Path(given).resolve()
            current = given == os.environ.get('GATES_CURRENT_PACK')
            pack = dict(path=str(path), sha256=sha_path(path), current=current)
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
    key = sha_bytes(json.dumps([pack['sha256'], pack['current'], inputs]).encode())
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
    """Distinct cases by key, and for each step the keys it ran. A step whose recording logged
    an error is left out: its corpus may be incomplete."""
    cases: dict[str, dict] = {}
    by_step: dict[str, set[str]] = {}
    for log in sorted(corpus.glob('*.jsonl')):
        step = log.stem
        errors = log.with_name(log.name + '.errors')
        if errors.exists() and errors.stat().st_size:
            continue
        keys = by_step.setdefault(step, set())
        for line in log.read_text().splitlines():
            entry = json.loads(line)
            case = cases.setdefault(entry['key'], dict(entry, steps=set(), outputs=set(), picture_frames=set(),
                                                       step_outputs={}))
            case['steps'].add(step)
            case['outputs'].update(entry['outputs'])
            case['step_outputs'].setdefault(step, set()).update(entry['outputs'])
            case['picture_frames'].update(from_ranges(entry.get('pictures', [])))
            keys.add(entry['key'])
    return cases, by_step


# The replay's streams each output option feeds. Stdout and stderr are read by every check.
STREAMS_OF = {'--race-timeline': 'race-timeline', '--one-player-timeline': 'one-player-timeline',
              '--look-timeline': 'look-timeline', '--sound-cues': 'sound-cues', '--picture': 'pictures',
              '--vram': 'vram', '--records': 'records', '--frame-digests': 'pictures', '--rider-word': 'rider-word'}


def reads(outputs: set[str]) -> set[str]:
    """The replay streams a check that asked for these outputs can have read."""
    return {'stdout', 'stderr'} | {STREAMS_OF[o] for o in outputs if o in STREAMS_OF}


SMALL = 1 << 20
SCRIPTS = ('.py', '.sh', '.list')
# The input recipes a check reads in the folders it names. Capture manifests and reports are left
# out: gate steps rewrite some (cond-*.json, anchors.json) on every run.
RECIPES = ('.frontend-inputs.txt', '.record-writes.txt', '.runner-args.txt', '.script.json', '.inputs',
           '.reset-delays.txt')


def _folder_digest(folder: Path, recipes: bool, cache: dict) -> tuple[str, str]:
    """The digest of a folder's scripts (top level and one below), with its input recipes when
    `recipes`, and the scripts' text, scanned for further folders."""
    key = (folder, recipes)
    if key in cache:
        return cache[key]
    h, text = hashlib.sha256(), []
    for path in sorted([*folder.glob('*'), *folder.glob('*/*')]):
        script = path.suffix in SCRIPTS
        if not (script or (recipes and path.name.endswith(RECIPES))):
            continue
        try:
            if path.is_file() and path.stat().st_size < SMALL:
                data = path.read_bytes()
                h.update(str(path.relative_to(folder)).encode()); h.update(data)
                if script:
                    text.append(data.decode(errors='replace'))
        except OSError:
            continue
    cache[key] = (h.hexdigest(), '\n'.join(text))
    return cache[key]


def fingerprint(command: str, evidence: Path, cache: dict | None = None) -> str:
    """A check's command; the scripts and input recipes of the evidence folders it names, by any
    spelling (`$L/x`, or a bare `x` an argument passes to a script); and the scripts of the folders
    those scripts name. A changed fingerprint means its recorded runs may no longer be what it
    runs. Capture manifests, and scripts two folders away, are outside it."""
    import re
    cache = {} if cache is None else cache
    names = cache.get('names')
    if names is None:
        try:
            names = cache['names'] = {n for n in os.listdir(evidence) if (evidence / n).is_dir()}
        except OSError:
            names = cache['names'] = set()
    mentioned = lambda text: {t for t in re.findall(r'[A-Za-z0-9._-]+', text) if t in names}
    h = hashlib.sha256(command.encode())
    direct = sorted(mentioned(command))
    reached: set[str] = set()
    for name in direct:
        digest, text = _folder_digest(evidence / name, True, cache)
        h.update(name.encode()); h.update(digest.encode())
        reached |= mentioned(text)
    for name in sorted(reached - set(direct)):
        digest, _ = _folder_digest(evidence / name, False, cache)
        h.update(name.encode()); h.update(digest.encode())
    return h.hexdigest()


def write_meta(corpus: Path, step: str, **fields) -> None:
    (corpus / f'{step}.meta.json').write_text(json.dumps(fields, indent=1) + '\n')


def read_meta(corpus: Path, step: str) -> dict:
    try:
        return json.loads((corpus / f'{step}.meta.json').read_text())
    except (OSError, ValueError):
        return {}


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
