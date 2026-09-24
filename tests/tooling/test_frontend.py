from __future__ import annotations

import contextlib
import hashlib
import json
import os
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
                      report=None, task="M3-03", track="dragster", replace_pack=False)
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

    def two_track_rules(self):
        two_track = dict(self.rules, profile_id=pack.TWO_TRACK_PROFILE,
                         start_state_id=pack.TWO_TRACK_START)
        path = self.root / "two-track-rules.json"
        path.write_text(json.dumps(two_track))
        rules, rules_sha = pack.load_rules(path)
        return path, rules, rules_sha

    def check(self, report, name):
        return next(c for c in json.loads(Path(report).read_text())["checks"] if c["name"] == name)

    def test_named_pack_must_carry_the_rules_profile(self):
        # A pack is chosen by the profile recorded inside it. A typed --pack of
        # another profile is refused, naming both profiles and the remedy; the
        # launcher never substitutes a different file for the one it was given.
        rules_path, rules, rules_sha = self.two_track_rules()
        payload, _ = pack.build_pack(self.rom, rules, rules_sha)
        two_track_pack = self.root / "two-track.pack"
        two_track_pack.write_bytes(payload)
        for track in ("dragster", "zoom-zoo"):
            self.assertEqual(commands.cmd_run(self.args(track=track, pack=str(two_track_pack),
                                                        rules=str(rules_path))), EXIT_OK)
        report = self.root / "refused.json"
        self.assertEqual(commands.cmd_run(self.args(track="dragster", pack=str(two_track_pack),
                                                    report=str(report))), EXIT_INVALID_INPUT)
        detail = self.check(report, "classic_pack")["detail"]
        for expected in ("was not replaced", pack.TWO_TRACK_PROFILE, pack.PROFILE_ID, "--replace-pack"):
            self.assertIn(expected, detail)
        self.assertEqual(two_track_pack.read_bytes(), payload)
        v1_payload, _ = pack.build_pack(self.rom, *pack.load_rules(self.rules_path))
        v1_pack = self.root / "v1.pack"
        v1_pack.write_bytes(v1_payload)
        for track in ("dragster", "zoom-zoo"):
            report = self.root / f"v1-{track}.json"
            self.assertEqual(commands.cmd_run(self.args(track=track, pack=str(v1_pack), rules=str(rules_path),
                                                        report=str(report))), EXIT_INVALID_INPUT)
            self.assertIn(pack.PROFILE_ID, self.check(report, "classic_pack")["detail"])
        corrupt = bytearray(payload)
        corrupt[-1] ^= 0xFF
        corrupt_pack = self.root / "corrupt-two-track.pack"
        corrupt_pack.write_bytes(bytes(corrupt))
        report = self.root / "corrupt-two-track.json"
        self.assertEqual(commands.cmd_run(self.args(track="dragster", pack=str(corrupt_pack), rules=str(rules_path),
                                                    report=str(report))), EXIT_INVALID_INPUT)
        detail = self.check(report, "classic_pack")["detail"]
        self.assertIn("was not replaced", detail)
        self.assertNotIn("records profile", detail)  # same profile, so no identity clause

    def test_default_pack_is_selected_by_profile_under_local(self):
        # Without --pack the newest pack under local/ that validates against
        # the rules is used, whatever its name; a first launch extracts to the
        # profile's own path, so a profile bump never finds an old file in its way.
        rules_path, rules, rules_sha = self.two_track_rules()
        payload, _ = pack.build_pack(self.rom, rules, rules_sha)
        v1_payload, _ = pack.build_pack(self.rom, *pack.load_rules(self.rules_path))
        local = self.root / "local"
        local.mkdir()
        with mock.patch.object(commands, "ROOT", self.root):
            default = commands.default_pack_path(rules)
            self.assertEqual(default.parent, local.resolve())
            self.assertIn("tracks-v12", default.name)
            report = self.root / "none.json"
            self.assertEqual(commands.cmd_run(self.args(pack=None, rules=str(rules_path), report=str(report))),
                             EXIT_MISSING_PREREQUISITE)
            detail = self.check(report, "supported_rom")["detail"]
            self.assertIn(pack.TWO_TRACK_PROFILE, detail)
            self.assertIn(str(default), detail)
            self.assertEqual(commands.cmd_run(self.args(pack=None, rules=str(rules_path), rom=str(self.rom_path))),
                             EXIT_OK)
            self.assertTrue(default.is_file())
            self.rom_path.unlink()
            now = default.stat().st_mtime
            decoy = local / "zzz-newest.pack"
            decoy.write_bytes(b"corrupt")
            os.utime(decoy, (now + 100, now + 100))
            other_profile = local / "aaa-other-profile.pack"
            other_profile.write_bytes(v1_payload)
            os.utime(other_profile, (now + 50, now + 50))
            report = self.root / "selected.json"
            self.assertEqual(commands.cmd_run(self.args(pack=None, rules=str(rules_path), report=str(report))),
                             EXIT_OK)
            detail = self.check(report, "classic_pack")["detail"]
            for expected in (str(default), "ROM was not opened", "zzz-newest.pack", "aaa-other-profile.pack",
                             pack.PROFILE_ID):
                self.assertIn(expected, detail)
            newer = local / "any-name-at-all.pack"
            newer.write_bytes(payload)
            os.utime(newer, (now + 200, now + 200))
            report = self.root / "newer.json"
            self.assertEqual(commands.cmd_run(self.args(pack=None, rules=str(rules_path), report=str(report))),
                             EXIT_OK)
            self.assertIn(str(newer.resolve()), self.check(report, "classic_pack")["detail"])

    def test_replace_pack_moves_an_incompatible_pack_aside(self):
        # --rom over an existing incompatible pack is refused with the remedy
        # named; --replace-pack with --rom moves the old file aside, keeping
        # its bytes, and extracts a fresh pack in its place.
        rules_path, rules, rules_sha = self.two_track_rules()
        v1_payload, _ = pack.build_pack(self.rom, *pack.load_rules(self.rules_path))
        stale = self.root / "stale.pack"
        stale.write_bytes(v1_payload)
        report = self.root / "refused.json"
        self.assertEqual(commands.cmd_run(self.args(pack=str(stale), rules=str(rules_path), rom=str(self.rom_path),
                                                    report=str(report))), EXIT_INVALID_INPUT)
        self.assertIn("--replace-pack", self.check(report, "classic_pack")["detail"])
        self.assertEqual(stale.read_bytes(), v1_payload)
        self.assertEqual(commands.cmd_run(self.args(pack=str(stale), rules=str(rules_path), rom=None,
                                                    replace_pack=True)), EXIT_INVALID_INPUT)
        self.assertEqual(stale.read_bytes(), v1_payload)
        report = self.root / "replaced.json"
        self.assertEqual(commands.cmd_run(self.args(pack=str(stale), rules=str(rules_path), rom=str(self.rom_path),
                                                    replace_pack=True, report=str(report))), EXIT_OK)
        moved = list(self.root.glob("stale.pack.stale-*"))
        self.assertEqual(len(moved), 1)
        self.assertEqual(moved[0].read_bytes(), v1_payload)
        self.assertEqual(pack.validate_pack(stale.read_bytes(), rules, rules_sha)["profile_id"],
                         pack.TWO_TRACK_PROFILE)
        self.assertIn(str(moved[0]), self.check(report, "classic_pack")["detail"])
        # A failed extraction leaves the incompatible pack where it was.
        stale.write_bytes(v1_payload)
        report = self.root / "failed-replace.json"
        with mock.patch.object(pack, "build_pack", side_effect=subprocess.CalledProcessError(1, ["decompress"])):
            self.assertEqual(commands.cmd_run(self.args(pack=str(stale), rules=str(rules_path), rom=str(self.rom_path),
                                                        replace_pack=True, report=str(report))), EXIT_INVALID_INPUT)
        self.assertEqual(stale.read_bytes(), v1_payload)
        self.assertEqual(len(list(self.root.glob("stale.pack.stale-*"))), 1)
        self.assertIn("nothing was replaced", self.check(report, "supported_rom")["detail"])
        self.assertEqual(self.check(report, "classic_pack")["outcome"], "failed")

    def test_stale_build_is_reported_before_launch(self):
        # The supported profile is compiled into the app. A build from before a
        # profile bump is diagnosed as a stale build with the rebuild command,
        # not as a bad pack; an executable that reports nothing is recorded.
        profiles = self.root / "profiles.txt"
        child = self.root / "child"
        child.write_text(
            "#!/usr/bin/env python3\n"
            "import sys\n"
            "from pathlib import Path\n"
            "if '--supported-profiles' in sys.argv:\n"
            f"    sys.stdout.write(Path({str(profiles)!r}).read_text())\n"
        )
        child.chmod(0o755)
        profiles.write_text("some.other.profile\n")
        report = self.root / "stale.json"
        self.assertEqual(commands.cmd_run(self.args(rom=str(self.rom_path), executable=str(child),
                                                    report=str(report))), EXIT_MISSING_PREREQUISITE)
        check = self.check(report, "supported_profile")
        self.assertEqual(check["outcome"], "missing")
        for expected in ("some.other.profile", pack.PROFILE_ID, "build --preset app-debug"):
            self.assertIn(expected, check["detail"])
        profiles.write_text(f"other.profile\n{pack.PROFILE_ID}\n")
        report = self.root / "current.json"
        self.assertEqual(commands.cmd_run(self.args(executable=str(child), report=str(report))), EXIT_OK)
        self.assertEqual(self.check(report, "supported_profile")["outcome"], "passed")
        profiles.write_text("")
        report = self.root / "silent.json"
        self.assertEqual(commands.cmd_run(self.args(executable=str(child), report=str(report))), EXIT_OK)
        self.assertEqual(self.check(report, "supported_profile")["outcome"], "skipped")

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
