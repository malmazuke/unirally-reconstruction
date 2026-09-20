"""The gate-identity shortcut must fail closed.

It decides whether a candidate may cite an earlier candidate's differential
gate reports instead of re-running 35 minutes of replays, so every way it can
be wrong has to be a refusal. An earlier version derived the gate binary's
inputs from three hardcoded objects, missed six of the nine translation units
and reported "identical" across an edit that moved the gate's own rows hash
(review E1); these check the refusals that replaced it, without needing a build.
"""
from __future__ import annotations

import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
TOOL = ['python3', '-m', 'tools.unirally_lab.native.gate_identity']
EMPTY_DIFF = 'e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855'


def run(arguments: list[str], cwd: Path) -> subprocess.CompletedProcess[str]:
    return subprocess.run(TOOL + arguments, cwd=cwd, capture_output=True, text=True)


def report(**overrides) -> dict:
    base = dict(status='passed', source_commit='0' * 40, source_diff_sha256=EMPTY_DIFF,
                pack_sha256='1' * 64, contract_sha256='2' * 64, restore_frames=[1, 2, 3])
    base.update(overrides)
    return base


class GateIdentityRefusals(unittest.TestCase):
    def citation(self, directory: Path, **overrides) -> subprocess.CompletedProcess[str]:
        (directory/'a-gate.json').write_text(json.dumps(report(**overrides)))
        pack = directory/'pack.bin'
        pack.write_bytes(b'')
        return run(['--since', 'HEAD', '--reports', str(directory), '--expect', '1',
                    '--pack', str(pack), '--build', str(directory/'absent-build')], ROOT)

    def test_a_missing_build_directory_is_refused(self):
        with tempfile.TemporaryDirectory() as directory:
            result = self.citation(Path(directory))
            self.assertEqual(result.returncode, 1)
            self.assertIn('refused', result.stdout)

    def test_the_tool_reports_its_own_arguments(self):
        result = run(['--help'], ROOT)
        self.assertEqual(result.returncode, 0)
        for flag in ('--since', '--reports', '--expect', '--pack', '--ninja'):
            self.assertIn(flag, result.stdout)

    def test_expect_is_required(self):
        with tempfile.TemporaryDirectory() as directory:
            pack = Path(directory)/'pack.bin'
            pack.write_bytes(b'')
            result = run(['--since', 'HEAD', '--reports', directory, '--pack', str(pack)], ROOT)
            self.assertEqual(result.returncode, 2)
            self.assertIn('--expect', result.stderr)


if __name__ == '__main__':
    unittest.main()
