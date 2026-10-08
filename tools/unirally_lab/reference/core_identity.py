"""Which reference core libraries the laboratory accepts (D-0001, PORTABLE-CORE-IDENTITY).

The reference core is identified by its source: the lock's commit and tracked patch. A built
library's SHA-256 also depends on the compiler, SDK and build path (a rebuild of the same source on
the same Mac gives another hash), so the hash is provenance, not identity. A library is accepted
when either:

- `reference build` recorded building it from the lock's commit and patch (`lab-core.json` in the
  checkout), and its hash is still the one recorded then; or
- it is listed in `tools/locks/verified-cores.json`: a library whose behaviour was checked against
  the pinned results (the one every frozen reference was captured with, and the libraries that
  `core_check` and the landing-matrix pin verified on other hosts).

A stored capture records the library it was made with (`core_sha256`). It is accepted when that
library is verified or is this host's own source-built library. Behaviour itself is checked by
output pins: the pack rules' SHA-256 of each extracted entry, frozen digests, `core_check`.
"""
from __future__ import annotations

import json
from pathlib import Path
from typing import Any

from .commands import LockError, core_entry, core_manifest_path, load_core_manifest, load_lock, sha256_file

ROOT = Path(__file__).resolve().parents[3]
LOCK_PATH = ROOT / 'tools/locks/emulators.json'
VERIFIED_PATH = ROOT / 'tools/locks/verified-cores.json'
CORE = 'bsnes'


class CoreIdentityError(ValueError):
    pass


def lock_source(lock_path: Path = LOCK_PATH) -> dict[str, Any]:
    """The lock's source identity of the reference core: commit and patch SHA-256."""
    entry = core_entry(load_lock(lock_path), CORE)
    return {'commit': entry['commit'], 'patch_sha256': entry.get('patch_sha256')}


def verified_cores(path: Path = VERIFIED_PATH, lock_path: Path = LOCK_PATH) -> dict[str, dict[str, Any]]:
    """The verified libraries by SHA-256. The file must name the lock's own source."""
    document = json.loads(path.read_text(encoding='utf-8'))
    source = lock_source(lock_path)
    if (document.get('schema_version') != 1 or document.get('core') != CORE
            or document.get('commit') != source['commit'] or document.get('patch_sha256') != source['patch_sha256']):
        raise CoreIdentityError(f'{path.name} does not name the lock\'s {CORE} commit and patch')
    libraries = {item['sha256']: item for item in document['libraries']}
    if len(libraries) != len(document['libraries']):
        raise CoreIdentityError(f'{path.name} lists a library twice')
    return libraries


def source_built_library(root: Path = ROOT, lock_path: Path = LOCK_PATH) -> str | None:
    """The SHA-256 of this checkout's library if `lab-core.json` shows it was built from the lock's
    commit and patch and the library is unchanged since; otherwise None."""
    try:
        lock = load_lock(lock_path)
        entry = core_entry(lock, CORE)
    except LockError:
        return None
    manifest = load_core_manifest(core_manifest_path(root, lock, entry))
    if manifest is None or not Path(manifest['library']).is_file():
        return None
    if manifest['commit'] != entry['commit'] or manifest['patch_sha256'] != entry.get('patch_sha256'):
        return None
    library_sha = sha256_file(Path(manifest['library']))
    return library_sha if library_sha == manifest['library_sha256'] else None


def require_core(library: Path, *, root: Path = ROOT, lock_path: Path = LOCK_PATH,
                 verified_path: Path = VERIFIED_PATH) -> str:
    """The library's SHA-256 if it may run as the reference core; refuses any other library."""
    library_sha = sha256_file(library)
    if library_sha in verified_cores(verified_path, lock_path):
        return library_sha
    built = source_built_library(root, lock_path)
    if built is not None and built == library_sha:
        return library_sha
    source = lock_source(lock_path)
    raise CoreIdentityError(
        f'core library {library_sha[:16]} is not built from the lock\'s {CORE} commit {source["commit"][:12]} and '
        f'patch {str(source["patch_sha256"])[:12]} (no matching lab-core.json; run `reference build`) and is not '
        f'in {VERIFIED_PATH.name}')


def accepted_reference(core_sha256: str, *, root: Path = ROOT, lock_path: Path = LOCK_PATH,
                       verified_path: Path = VERIFIED_PATH) -> bool:
    """Whether a stored capture made with this library is accepted as a reference."""
    return core_sha256 in verified_cores(verified_path, lock_path) or core_sha256 == source_built_library(root, lock_path)
