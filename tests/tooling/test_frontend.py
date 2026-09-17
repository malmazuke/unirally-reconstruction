from __future__ import annotations

import contextlib
import hashlib
import json
import subprocess
import sys
import tempfile
import unittest
from argparse import Namespace
from pathlib import Path
from unittest import mock

from unirally_lab import EXIT_FAILURE, EXIT_INVALID_INPUT, EXIT_MISSING_PREREQUISITE, EXIT_OK
from unirally_lab.content import pack
from unirally_lab.frontend import commands

ROOT = Path(__file__).resolve().parents[2]
PROJECT = ROOT / "tools" / "project.py"


class FrontendLaunchTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        self.rom = b"authored-rom"
        entry = b"entry"
        self.rules = {
            "schema_version": 1,
            "kind": "classic_pack_rules",
            "profile_id": pack.PROFILE_ID,
            "start_state_id": pack.START_STATE_ID,
            "source_rom": {"size": len(self.rom), "sha256": hashlib.sha256(self.rom).hexdigest()},
            "entries": [{"id": "test.entry",
                         "source": {"kind": "raw", "pieces": [{"file_offset": 0, "length": len(entry)}]},
                         "size": len(entry), "sha256": hashlib.sha256(self.rom[:len(entry)]).hexdigest()}],
        }
        self.rules_path = self.root / "rules.json"
        self.rules_path.write_text(json.dumps(self.rules))
        self.rom_path = self.root / "rom.sfc"
        self.rom_path.write_bytes(self.rom)

    def tearDown(self):
        self.temp.cleanup()

    def args(self, **changed):
        values = dict(pack=str(self.root / "classic.pack"), rom=None, preset="app-debug",
                      executable="/usr/bin/true", rules=str(self.rules_path), updates=1,
                      fixed_controller_mask=None, hidden=True, timeout=10,
                      report=None, task="M3-03")
        values.update(changed)
        return Namespace(**values)

    def test_default_executable_is_platform_specific(self):
        with mock.patch.object(commands.sys, "platform", "darwin"):
            mac = commands._frontend_paths(self.args(executable=None)).executable
        with mock.patch.object(commands.sys, "platform", "linux"):
            linux = commands._frontend_paths(self.args(executable=None)).executable
        self.assertEqual(
            mac,
            ROOT / "build" / "app-debug" / "src" / "app" /
            "unirally.app" / "Contents" / "MacOS" / "unirally",
        )
        self.assertEqual(
            linux,
            ROOT / "build" / "app-debug" / "src" / "app" / "unirally",
        )

    def test_first_launch_then_pack_only_relaunch(self):
        self.assertEqual(commands.cmd_run(self.args(rom=str(self.rom_path))), EXIT_OK)
        self.rom_path.unlink()
        self.assertEqual(commands.cmd_run(self.args(rom=None)), EXIT_OK)

    def test_cancel_missing_wrong_and_corrupt_inputs_fail(self):
        self.assertEqual(commands.cmd_run(self.args()), EXIT_MISSING_PREREQUISITE)
        self.assertEqual(commands.cmd_run(self.args(rom="")), EXIT_MISSING_PREREQUISITE)
        self.assertEqual(commands.cmd_run(self.args(rom=str(self.root / "missing.sfc"))), EXIT_MISSING_PREREQUISITE)
        wrong = self.root / "wrong.sfc"
        wrong.write_bytes(b"wrong")
        self.assertEqual(commands.cmd_run(self.args(rom=str(wrong))), EXIT_INVALID_INPUT)
        Path(self.args().pack).write_bytes(b"corrupt")
        self.assertEqual(commands.cmd_run(self.args(rom=str(self.rom_path))), EXIT_INVALID_INPUT)

    def test_dragster_launch_accepts_a_valid_two_track_pack(self):
        # DRAGSTER also runs from the two-track pack (R-0037). With the default
        # DRAGSTER rules, an existing pack that fails them is validated against
        # the two-track rules; an explicit other rules path gets no fallback.
        two_track = dict(self.rules, profile_id=pack.TWO_TRACK_PROFILE,
                         start_state_id=pack.TWO_TRACK_START)
        (self.root / "two-track-rules.json").write_text(json.dumps(two_track))
        two_track_rules, two_track_sha = pack.load_rules(self.root / "two-track-rules.json")
        payload, _ = pack.build_pack(self.rom, two_track_rules, two_track_sha)
        two_track_pack = self.root / "two-track.pack"
        two_track_pack.write_bytes(payload)
        with mock.patch.object(commands, "ROOT", self.root), \
             mock.patch.object(pack, "RULES_PATH", "rules.json"), \
             mock.patch.object(pack, "TWO_TRACK_RULES_PATH", "two-track-rules.json"):
            self.assertEqual(commands.cmd_run(self.args(track="dragster", pack=str(two_track_pack))), EXIT_OK)
            self.assertEqual(commands.cmd_run(self.args(track="dragster")), EXIT_MISSING_PREREQUISITE)
            other_rules = self.root / "copy-of-rules.json"
            other_rules.write_text(self.rules_path.read_text())
            self.assertEqual(commands.cmd_run(self.args(track="dragster", pack=str(two_track_pack),
                                                        rules=str(other_rules))), EXIT_INVALID_INPUT)
            corrupt = bytearray(payload)
            corrupt[-1] ^= 0xFF
            corrupt_pack = self.root / "corrupt-two-track.pack"
            corrupt_pack.write_bytes(bytes(corrupt))
            report = self.root / "corrupt-two-track.json"
            self.assertEqual(commands.cmd_run(self.args(track="dragster", pack=str(corrupt_pack),
                                                        report=str(report))), EXIT_INVALID_INPUT)
            detail = next(c for c in json.loads(report.read_text())["checks"]
                          if c["name"] == "classic_pack")["detail"]
            self.assertNotIn("extraction-rules identity", detail)

    def test_dragster_only_pack_launches_with_the_two_track_pack(self):
        # DRAGSTER's ordinary controls need the shared race tables (R-0038),
        # which only the two-track pack carries. A valid DRAGSTER-only pack
        # launches with a valid two-track pack beside it, or one extracted
        # from --rom; without either it is a missing prerequisite.
        two_track = dict(self.rules, profile_id=pack.TWO_TRACK_PROFILE,
                         start_state_id=pack.TWO_TRACK_START)
        (self.root / "two-track-rules.json").write_text(json.dumps(two_track))
        dragster_pack = self.root / "classic.pack"
        with mock.patch.object(commands, "ROOT", self.root), \
             mock.patch.object(pack, "RULES_PATH", "rules.json"), \
             mock.patch.object(pack, "TWO_TRACK_RULES_PATH", "two-track-rules.json"):
            (self.root / "rules.json").write_text(self.rules_path.read_text())
            rules = str(self.root / "rules.json")
            report = self.root / "missing.json"
            self.assertEqual(commands.cmd_run(self.args(rules=rules, rom=None, report=str(report))),
                             EXIT_MISSING_PREREQUISITE)
            self.assertFalse(dragster_pack.exists())
            # First launch extracts the DRAGSTER pack and the two-track pack.
            self.assertEqual(commands.cmd_run(self.args(rules=rules, rom=str(self.rom_path))), EXIT_OK)
            upgraded = self.root / "local" / commands.TWO_TRACK_PACK_NAMES[0]
            self.assertTrue(dragster_pack.is_file() and upgraded.is_file())
            # Pack-only relaunch finds the two-track pack without the ROM.
            self.rom_path.unlink()
            report = self.root / "relaunch.json"
            self.assertEqual(commands.cmd_run(self.args(rules=rules, rom=None, report=str(report))), EXIT_OK)
            check = next(c for c in json.loads(report.read_text())["checks"] if c["name"] == "dragster_two_track_pack")
            self.assertIn("ROM was not opened", check["detail"])
            # Without a valid two-track pack and without --rom it cannot play.
            upgraded.write_bytes(b"corrupt")
            self.assertEqual(commands.cmd_run(self.args(rules=rules, rom=None)), EXIT_MISSING_PREREQUISITE)

    def test_frontend_failure_is_not_a_successful_launch(self):
        self.assertEqual(commands.cmd_run(self.args(rom=str(self.rom_path), executable="/usr/bin/false")), EXIT_FAILURE)

    def test_nonfinite_timeout_reports_without_starting_child(self):
        for index, timeout in enumerate((float("nan"), float("inf"), float("-inf"))):
            with self.subTest(timeout=timeout):
                report = self.root / f"nonfinite-{index}.json"
                with mock.patch.object(commands, "run_bounded") as child:
                    status = commands.cmd_run(self.args(timeout=timeout, report=str(report)))
                self.assertEqual(status, EXIT_INVALID_INPUT)
                child.assert_not_called()
                self.assertTrue(report.is_file())
                document = json.loads(report.read_text())
                self.assertEqual(document["status"], "failed")
                check = next(c for c in document["checks"] if c["name"] == "arguments")
                self.assertEqual(check["outcome"], "failed")
                self.assertIn("finite and positive", check["detail"])

    def test_cli_nonfinite_timeout_spellings_report_without_starting_child(self):
        marker = self.root / "child-started"
        child = self.root / "child"
        child.write_text(
            "#!/usr/bin/env python3\n"
            "from pathlib import Path\n"
            f"Path({str(marker)!r}).write_text('started')\n"
        )
        child.chmod(0o755)
        spellings = (("nan",), ("inf",), ("-inf",), ("--equals", "-inf"))
        for index, spelling in enumerate(spellings):
            with self.subTest(spelling=spelling):
                report = self.root / f"cli-nonfinite-{index}.json"
                timeout_arguments = (
                    [f"--timeout={spelling[1]}"]
                    if spelling[0] == "--equals"
                    else ["--timeout", spelling[0]]
                )
                result = subprocess.run(
                    [sys.executable, str(PROJECT), "frontend", "run",
                     "--pack", str(self.root / "absent.pack"),
                     "--executable", str(child), "--report", str(report),
                     *timeout_arguments],
                    capture_output=True, text=True, timeout=30,
                )
                self.assertEqual(result.returncode, EXIT_INVALID_INPUT,
                                 result.stderr)
                self.assertTrue(report.is_file())
                self.assertEqual(json.loads(report.read_text())["status"],
                                 "failed")
                self.assertFalse(marker.exists())

    def test_report_aliases_never_modify_inputs_or_start_child(self):
        original_rom = self.rom_path.read_bytes()
        alias_dir = self.root / "aliases"
        alias_dir.mkdir()
        symlink = alias_dir / "rom-link"
        symlink.symlink_to(self.rom_path)
        hardlink = alias_dir / "rom-hardlink"
        hardlink.hardlink_to(self.rom_path)
        resolved_absent = alias_dir / ".." / "absent.pack"
        cases = (
            ("rom", self.args(rom=str(self.rom_path), report=str(self.rom_path))),
            ("rom-symlink", self.args(rom=str(self.rom_path), report=str(symlink))),
            ("rom-hardlink", self.args(rom=str(self.rom_path), report=str(hardlink))),
            ("rules", self.args(rom=str(self.rom_path), report=str(self.rules_path))),
            ("executable", self.args(rom=str(self.rom_path),
                                     report="/usr/bin/true")),
            ("absent-pack", self.args(pack=str(self.root / "absent.pack"),
                                      rom=str(self.rom_path),
                                      report=str(resolved_absent))),
        )
        for label, args in cases:
            with self.subTest(label=label), \
                    mock.patch.object(commands, "run_bounded") as child, \
                    mock.patch.object(commands.reportmod, "Report") as report:
                self.assertEqual(commands.cmd_run(args), EXIT_INVALID_INPUT)
                child.assert_not_called()
                report.assert_not_called()
                self.assertEqual(self.rom_path.read_bytes(), original_rom)
                self.assertFalse((self.root / "absent.pack").exists())

        self.assertEqual(commands.cmd_run(self.args(rom=str(self.rom_path))),
                         EXIT_OK)
        pack_path = Path(self.args().pack)
        original_pack = pack_path.read_bytes()
        with mock.patch.object(commands, "run_bounded") as child, \
                mock.patch.object(commands.reportmod, "Report") as report:
            self.assertEqual(
                commands.cmd_run(self.args(report=str(pack_path))),
                EXIT_INVALID_INPUT,
            )
        child.assert_not_called()
        report.assert_not_called()
        self.assertEqual(pack_path.read_bytes(), original_pack)

    def test_cli_report_rom_and_pack_aliases_preserve_bytes_and_skip_child(self):
        marker = self.root / "collision-child-started"
        child = self.root / "collision-child"
        child.write_text(
            "#!/usr/bin/env python3\n"
            "from pathlib import Path\n"
            f"Path({str(marker)!r}).write_text('started')\n"
        )
        child.chmod(0o755)
        pack_path = self.root / "cli.pack"
        original_rom = self.rom_path.read_bytes()
        common = [
            sys.executable, str(PROJECT), "frontend", "run",
            "--pack", str(pack_path), "--rom", str(self.rom_path),
            "--rules", str(self.rules_path), "--executable", str(child),
            "--updates", "1", "--timeout", "30",
        ]
        rom_collision = subprocess.run(
            [*common, "--report", str(self.rom_path)],
            capture_output=True, text=True, timeout=30,
        )
        self.assertEqual(rom_collision.returncode, EXIT_INVALID_INPUT)
        self.assertIn("refusing to write --report onto --rom", rom_collision.stderr)
        self.assertEqual(self.rom_path.read_bytes(), original_rom)
        self.assertFalse(pack_path.exists())
        self.assertFalse(marker.exists())

        self.assertEqual(commands.cmd_run(self.args(
            pack=str(pack_path), rom=str(self.rom_path))), EXIT_OK)
        original_pack = pack_path.read_bytes()
        pack_collision = subprocess.run(
            [*common, "--report", str(pack_path)],
            capture_output=True, text=True, timeout=30,
        )
        self.assertEqual(pack_collision.returncode, EXIT_INVALID_INPUT)
        self.assertIn("refusing to write --report onto --pack", pack_collision.stderr)
        self.assertEqual(pack_path.read_bytes(), original_pack)
        self.assertFalse(marker.exists())

    def test_cli_quoted_tilde_alias_is_refused(self):
        literal_home = self.root / "~"
        literal_home.mkdir()
        pack_path = literal_home / "victim.pack"
        self.assertEqual(commands.cmd_run(self.args(
            pack=str(pack_path), rom=str(self.rom_path))), EXIT_OK)
        original_pack = pack_path.read_bytes()
        with contextlib.chdir(self.root), \
                mock.patch.object(commands, "run_bounded") as launched, \
                mock.patch.object(commands.reportmod, "Report") as report:
            self.assertEqual(commands.cmd_run(self.args(
                pack=str(pack_path), report="~/victim.pack")),
                EXIT_INVALID_INPUT)
        launched.assert_not_called()
        report.assert_not_called()
        self.assertEqual(pack_path.read_bytes(), original_pack)
        marker = self.root / "tilde-child-started"
        child = self.root / "tilde-child"
        child.write_text(
            "#!/usr/bin/env python3\n"
            "from pathlib import Path\n"
            f"Path({str(marker)!r}).write_text('started')\n"
        )
        child.chmod(0o755)
        result = subprocess.run(
            [sys.executable, str(PROJECT), "frontend", "run",
             "--pack", str(pack_path), "--rules", str(self.rules_path),
             "--executable", str(child), "--updates", "1",
             "--timeout", "30", "--report", "~/victim.pack"],
            cwd=self.root, capture_output=True, text=True, timeout=30,
        )
        self.assertEqual(result.returncode, EXIT_INVALID_INPUT)
        self.assertIn("refusing to write --report onto --pack", result.stderr)
        self.assertEqual(pack_path.read_bytes(), original_pack)
        self.assertFalse(marker.exists())

    def test_cli_symlink_parent_traversal_keeps_distinct_report(self):
        directory_a = self.root / "a"
        directory_b = self.root / "b"
        (directory_b / "sub").mkdir(parents=True)
        directory_a.mkdir()
        (directory_a / "link").symlink_to(directory_b / "sub")
        pack_path = directory_a / "report.json"
        self.assertEqual(commands.cmd_run(self.args(
            pack=str(pack_path), rom=str(self.rom_path))), EXIT_OK)
        original_pack = pack_path.read_bytes()
        marker = self.root / "distinct-child-started"
        child = self.root / "distinct-child"
        child.write_text(
            "#!/usr/bin/env python3\n"
            "from pathlib import Path\n"
            f"Path({str(marker)!r}).write_text('started')\n"
        )
        child.chmod(0o755)
        report_spelling = directory_a / "link" / ".." / "report.json"
        actual_report = directory_b / "report.json"
        result = subprocess.run(
            [sys.executable, str(PROJECT), "frontend", "run",
             "--pack", str(pack_path), "--rules", str(self.rules_path),
             "--executable", str(child), "--updates", "1",
             "--timeout", "30", "--report", str(report_spelling)],
            cwd=self.root, capture_output=True, text=True, timeout=30,
        )
        self.assertEqual(result.returncode, EXIT_OK, result.stderr)
        self.assertEqual(pack_path.read_bytes(), original_pack)
        self.assertTrue(marker.is_file())
        self.assertEqual(json.loads(actual_report.read_text())["status"],
                         "passed")


if __name__ == "__main__":
    unittest.main()
