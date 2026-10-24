"""Which native binaries a change can alter.

A binary's behaviour is a function of the files it is compiled from, the build configuration and
the content pack it reads. Ninja records the first exactly (`gate_identity` uses the same
records), so a binary none of whose recorded inputs changed between main and the candidate, built
the same way and reading the same pack, replays every input exactly as main's did. Every query
that fails is reported as "changed": the selection that skips a check must fail closed.
"""
from __future__ import annotations

import hashlib
import subprocess
from dataclasses import dataclass, field
from pathlib import Path

from ..native.gate_identity import engine_files

# The binaries the gates drive, by ninja target, and the preset each is built in.
TARGETS = {
    'front_end_runner': ('lab-release', 'src/core/front_end_runner'),
    'zoom_zoo_runner': ('lab-release', 'src/core/zoom_zoo_runner'),
    'classic_race_presentation_runner': ('lab-release', 'src/core/classic_race_presentation_runner'),
    'movement_runner': ('lab-release', 'src/core/movement_runner'),
    'presentation_runner': ('lab-debug', 'src/core/presentation_runner'),
    'split_demo_runner': ('lab-release', 'src/core/split_demo_runner'),
    'audio_driver_runner': ('lab-release', 'tests/native/audio_driver_runner'),
    'race_audio_runner': ('lab-release', 'tests/native/race_audio_runner'),
    'dragster_fuzz_runner': ('lab-release', 'src/app/dragster_fuzz_runner'),
    'app': ('app-debug', 'src/app/unirally.app/Contents/MacOS/unirally'),
}

# A change to any of these can change how every binary is built: fail closed.
BUILD_WIDE = ('CMakeLists.txt', 'CMakePresets.json', 'cmake/', 'third_party/', 'tools/locks/')


@dataclass
class Identity:
    base: str
    changed_files: list[str]
    pack_changed: bool
    binaries: dict[str, dict] = field(default_factory=dict)  # name -> {changed, files, reason}

    def changed(self, name: str) -> bool:
        return self.binaries.get(name, {'changed': True})['changed']

    def any_code(self) -> bool:
        return any(not f.startswith(('docs/', 'tasks/')) and not f.endswith('.md') for f in self.changed_files)


def git(root: Path, *args: str) -> str:
    return subprocess.check_output(['git', *args], cwd=root, text=True)


def changed_files(root: Path, base: str) -> list[str]:
    """Files whose content differs from `base`: committed, staged and unstaged, plus untracked
    files git would track (an untracked source a glob picks up still changes the build)."""
    names = set(git(root, 'diff', '--name-only', base).split())
    names.update(git(root, 'ls-files', '--others', '--exclude-standard').split())
    return sorted(names)


def sha_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open('rb') as f:
        for block in iter(lambda: f.read(1 << 20), b''):
            h.update(block)
    return h.hexdigest()


def identify(root: Path, base: str, ninja_tool: str, candidate_pack: Path, base_pack: Path) -> Identity:
    files = changed_files(root, base)
    pack_changed = sha_file(candidate_pack) != sha_file(base_pack)
    ident = Identity(base=base, changed_files=files, pack_changed=pack_changed)
    wide = [f for f in files if f.startswith(BUILD_WIDE)]
    changed = set(files)
    for name, (preset, target) in TARGETS.items():
        build = root / 'build' / preset
        if wide:
            ident.binaries[name] = dict(changed=True, reason=f'build-wide file changed: {wide[0]}')
            continue
        try:
            inputs, _ = engine_files(build, root, ninja_tool, target)
        except RuntimeError as error:
            ident.binaries[name] = dict(changed=True, reason=f'inputs unknown: {error}')
            continue
        hit = sorted(changed.intersection(inputs))
        reason = f'{len(hit)} of its {len(inputs)} input files changed' if hit else f'none of its {len(inputs)} input files changed'
        if pack_changed:
            reason += '; the content pack changed'
        ident.binaries[name] = dict(changed=bool(hit) or pack_changed, reason=reason, files=hit[:20])
    return ident
