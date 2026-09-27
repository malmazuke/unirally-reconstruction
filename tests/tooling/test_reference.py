"""Checks for the reference adapter tooling that need neither the ROM nor a built core.

Runs that need the pinned core and the ROM are performed through
``project.py reference ...`` and recorded as task evidence; here we prove the
error paths and the script contract, and that a missing prerequisite is
reported as missing rather than as a pass.
"""

from __future__ import annotations

import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

from unirally_lab import EXIT_FAILURE, EXIT_INVALID_INPUT, EXIT_MISSING_PREREQUISITE, EXIT_OK  # noqa: E402
from unirally_lab.reference import commands, worker  # noqa: E402

PROJECT = ROOT / "tools" / "project.py"
SCRIPTS = ROOT / "tests" / "manifests" / "reference"


def run_cli(*args: str) -> subprocess.CompletedProcess:
    return subprocess.run([sys.executable, str(PROJECT), *args], capture_output=True, text=True, timeout=300)


def write_lock(tmp: Path, **overrides) -> Path:
    lock = json.loads((ROOT / "tools" / "locks" / "emulators.json").read_text())
    lock["install_dir"] = str(tmp / "emulators")
    lock["cores"]["bsnes"].update(overrides)
    path = tmp / "emulators.json"
    path.write_text(json.dumps(lock))
    return path


class ScriptContractTests(unittest.TestCase):
    def test_tracked_scripts_are_valid(self) -> None:
        for path in sorted(SCRIPTS.glob("*.json")):
            with self.subTest(script=path.name):
                worker.load_script(path)

    def test_invalid_scripts_are_rejected(self) -> None:
        bad = [
            {"schema_version": 2, "frames": 1},
            {"schema_version": 1, "frames": 0},
            {"schema_version": 1, "frames": True},
            {"schema_version": 1, "frames": 10, "sample_every": 0},
            {"schema_version": 1, "frames": 10, "core": "mesen"},
            {"schema_version": 1, "frames": 10, "inputs": [{"from": 5, "to": 4, "buttons": ["start"]}]},
            {"schema_version": 1, "frames": 10, "inputs": [{"from": 0, "to": 1, "port": 2, "buttons": ["start"]}]},
            {"schema_version": 1, "frames": 10, "inputs": [{"from": 0, "to": 1, "buttons": ["turbo"]}]},
            {"schema_version": 1, "frames": 10, "inputs": [{"from": 0, "to": 1, "buttons": []}]},
            {"schema_version": 1, "frames": 10, "core_options": {"bsnes_entropy": 1}},
            {"schema_version": 1, "frames": 10, "core_options": {"not_an_option": "x"}},
            {"schema_version": 1, "frames": 10, "trace_entries": -1},
            {"schema_version": 1, "frames": 10, "cartridge_ram_writes": {}},
            {"schema_version": 1, "frames": 10, "cartridge_ram_writes": [{"after_frame": 10, "offset": 0, "byte": 1}]},
            {"schema_version": 1, "frames": 10, "cartridge_ram_writes": [{"after_frame": 1, "offset": -1, "byte": 1}]},
            {"schema_version": 1, "frames": 10, "cartridge_ram_writes": [{"after_frame": 1, "offset": 0, "byte": 0x100}]},
            {"schema_version": 1, "frames": 10, "cartridge_ram_writes": [{"after_frame": 1, "offset": 0}]},
            {"schema_version": 1, "frames": 10, "cartridge_ram_writes": [{"after_frame": 1, "offset": 0, "byte": 1},
                                                                         {"after_frame": 1, "offset": 0, "byte": 2}]},
        ]
        for data in bad:
            with self.subTest(script=data):
                with self.assertRaises(worker.ScriptError):
                    worker.validate_script(data)

    def test_inputs_apply_only_inside_their_window(self) -> None:
        script = worker.validate_script({"schema_version": 1, "frames": 10,
                                         "inputs": [{"from": 3, "to": 4, "buttons": ["start", "a"]}, {"from": 4, "to": 9, "port": 1, "buttons": ["b"]}]})
        self.assertEqual(worker.inputs_for_frame(script, 2), {0: set(), 1: set()})
        self.assertEqual(worker.inputs_for_frame(script, 3), {0: {"start", "a"}, 1: set()})
        self.assertEqual(worker.inputs_for_frame(script, 4), {0: {"start", "a"}, 1: {"b"}})
        self.assertEqual(worker.inputs_for_frame(script, 5), {0: set(), 1: {"b"}})

    def test_cartridge_ram_writes_follow_their_frame(self) -> None:
        script = worker.validate_script({"schema_version": 1, "frames": 10, "cartridge_ram_writes": [
            {"after_frame": 3, "offset": 0x1076, "byte": 1}, {"after_frame": 3, "offset": 0x1077, "byte": 2}]})
        self.assertEqual(worker.cartridge_ram_writes_after(script, 2), [])
        self.assertEqual(worker.cartridge_ram_writes_after(script, 3), [(0x1076, 1), (0x1077, 2)])


class SamplingAndPairingTests(unittest.TestCase):
    def test_forced_frames_cover_save_resume_and_last_frame(self) -> None:
        self.assertEqual(worker.forced_sample_frames(300, None, 0), {299})
        self.assertEqual(worker.forced_sample_frames(300, 150, 0), {150, 151, 299})
        self.assertEqual(worker.forced_sample_frames(300, None, 151), {151, 299})
        self.assertEqual(worker.forced_sample_frames(300, 298, 0), {298, 299})  # 299 + 1 is out of range
        self.assertTrue(worker.should_sample(150, 100, {150}))
        self.assertTrue(worker.should_sample(200, 100, set()))
        self.assertFalse(worker.should_sample(151, 100, {150}))

    @staticmethod
    def make_run(frames, final="F", tweak=None):
        out = {"frames": [{"frame": n, "wram_sha256": f"w{n}", "registers": {"pc": n}, "video": (1, 1, f"v{n}"), "audio_sha256": f"a{n}"} for n in frames],
               "final": {"state_sha256": final}}
        if tweak:
            tweak(out)
        return out

    def test_sparse_saving_run_is_not_a_perturbation(self) -> None:
        """The saving run samples extra frames around the save; that alone must not differ."""
        a = self.make_run([0, 100, 200, 299])
        b = self.make_run([0, 100, 150, 151, 200, 299])
        r = commands.compare_runs(a, b)
        self.assertTrue(r["identical"])
        self.assertEqual(r["compared"], 4)
        self.assertEqual(r["only_in_y"], [150, 151])
        self.assertEqual(r["only_in_x"], [])

    def test_real_difference_and_final_state_are_detected(self) -> None:
        a = self.make_run([0, 100, 200, 299])

        def poke(out):
            out["frames"][2]["wram_sha256"] = "other"
        r = commands.compare_runs(a, self.make_run([0, 100, 200, 299], tweak=poke))
        self.assertFalse(r["identical"])
        self.assertEqual(r["first_differing_frame"], 200)
        r = commands.compare_runs(a, self.make_run([0, 100, 200, 299], final="G"))
        self.assertFalse(r["identical"])
        self.assertFalse(r["final_state_identical"])
        self.assertIsNone(r["first_differing_frame"])

    def test_restore_pairing_starts_at_the_resume_frame(self) -> None:
        b = self.make_run([0, 100, 150, 151, 200, 299])
        c = self.make_run([151, 200, 299])
        r = commands.compare_runs(b, c, from_frame=151)
        self.assertTrue(r["identical"])
        self.assertEqual(r["compared"], 3)
        self.assertEqual(r["only_in_x"], [])
        missing = commands.compare_runs(b, self.make_run([200, 299]), from_frame=151)
        self.assertEqual(missing["only_in_x"], [151])  # the resume frame was not sampled: must be reported

    def test_no_common_frames_is_not_identical(self) -> None:
        self.assertFalse(commands.compare_runs(self.make_run([0]), self.make_run([5]))["identical"])

    def test_dense_sampling_from_the_save_point(self) -> None:
        self.assertTrue(worker.should_sample(151, 100, set(), dense_from=150))
        self.assertTrue(worker.should_sample(150, 100, set(), dense_from=150))
        self.assertFalse(worker.should_sample(149, 100, set(), dense_from=150))
        self.assertTrue(worker.should_sample(100, 100, set(), dense_from=150))

    def test_perturbation_verdict_needs_equal_dense_frame_sets(self) -> None:
        """With every frame from the save point sampled in both runs, an extra or
        missing frame on either side is a failure, not a schedule artefact."""
        dense = [0, 100, *range(150, 300)]
        a, b = self.make_run(dense), self.make_run(dense)
        self.assertTrue(commands.perturbation_verdict(commands.compare_runs(a, b)))
        self.assertFalse(commands.perturbation_verdict(commands.compare_runs(a, self.make_run([0, 100, *range(151, 300)]))))
        self.assertFalse(commands.perturbation_verdict(commands.compare_runs(self.make_run([0, 100, *range(151, 300)]), b)))

        def transient(out):  # differs on one frame only, reconverging before the end
            out["frames"][60]["registers"] = {"pc": -1}
        self.assertFalse(commands.perturbation_verdict(commands.compare_runs(a, self.make_run(dense, tweak=transient))))
        self.assertEqual(commands.compare_runs(a, self.make_run(dense, tweak=transient))["first_differing_frame"], dense[60])

    def test_continuation_verdict_requires_resume_frame_and_complete_tail(self) -> None:
        b = self.make_run([0, 100, *range(150, 300)])
        c = self.make_run(range(151, 300)); c["start_frame"] = 151
        self.assertTrue(commands.continuation_verdict(commands.compare_runs(b, c, 151), c))
        short = self.make_run(range(152, 300)); short["start_frame"] = 151
        self.assertFalse(commands.continuation_verdict(commands.compare_runs(b, short, 151), short))
        empty = self.make_run([]); empty["start_frame"] = 151
        self.assertFalse(commands.continuation_verdict(commands.compare_runs(b, empty, 151), empty))


class FieldCaptureTests(unittest.TestCase):
    """Samples schema 2: declared work RAM ranges are captured per sampled frame."""

    def test_samples_schema_is_2(self) -> None:
        self.assertEqual(worker.SAMPLES_SCHEMA_VERSION, 2)
        self.assertEqual(worker.WRAM_SIZE, 131072)

    def test_valid_fields(self) -> None:
        fields = worker.validate_fields([{"name": "a", "start": 0, "length": 256, "extra": "ignored"},
                                         {"name": "b", "start": 0x73, "length": 1}, {"name": "c", "start": 0x1FF00, "length": 256}])
        self.assertEqual(fields, [{"name": "a", "start": 0, "length": 256}, {"name": "b", "start": 0x73, "length": 1}, {"name": "c", "start": 0x1FF00, "length": 256}])
        self.assertEqual(worker.validate_fields([]), [])

    def test_invalid_fields_are_rejected(self) -> None:
        bad = [
            {"name": "a", "start": 0, "length": 1},  # not a list
            [{"name": "", "start": 0, "length": 1}],
            [{"name": "a", "start": 0, "length": 1}, {"name": "a", "start": 1, "length": 1}],
            [{"name": "a", "start": -1, "length": 1}],
            [{"name": "a", "start": 0, "length": 0}],
            [{"name": "a", "start": 0x1FFFF, "length": 2}],
            [{"name": "a", "start": True, "length": 1}],
            [{"name": "a", "start": "0", "length": 1}],
            ["a"],
            [{"name": f"f{i}", "start": i, "length": 1} for i in range(worker.MAX_FIELDS + 1)],
        ]
        for data in bad:
            with self.subTest(fields=data if not isinstance(data, list) or len(data) < 5 else "too many"):
                with self.assertRaises(worker.ScriptError):
                    worker.validate_fields(data)

    def test_capture_slices_the_declared_ranges_as_hex(self) -> None:
        wram = bytes(range(256)) * 512
        fields = worker.validate_fields([{"name": "head", "start": 0, "length": 4}, {"name": "x73", "start": 0x73, "length": 2}])
        self.assertEqual(worker.capture_fields(wram, fields), {"head": "00010203", "x73": "7374"})
        self.assertEqual(worker.capture_fields(wram, []), {})

    def test_forced_frames_follow_the_stop_frame(self) -> None:
        # --stop-after-frame 149 runs 150 frames: the last executed frame is forced.
        self.assertEqual(worker.forced_sample_frames(150, None, 0), {149})


class StateSidecarTests(unittest.TestCase):
    ACTUAL = {"sha256": "s" * 64, "core_sha256": "c" * 64, "rom_sha256": "r" * 64, "script_sha256": "p" * 64, "serialization_method": "Strict"}

    def meta(self, **overrides):
        m = {**self.ACTUAL, "after_frame": 149, "post_serialize": {"wram_sha256": "w" * 64, "registers": {"pc": 1}}}
        m.update(overrides)
        return m

    def test_valid_sidecar_resumes_after_the_saved_frame(self) -> None:
        start, post = worker.validate_state_sidecar(self.meta(), self.ACTUAL, 300)
        self.assertEqual(start, 150)
        self.assertEqual(post["wram_sha256"], "w" * 64)

    def test_state_from_another_core_rom_script_or_method_is_rejected(self) -> None:
        for key in worker.STATE_IDENTITY_KEYS:
            with self.subTest(key=key):
                with self.assertRaisesRegex(worker.ScriptError, key):
                    worker.validate_state_sidecar(self.meta(**{key: "other"}), self.ACTUAL, 300)

    def test_state_must_leave_frames_to_run(self) -> None:
        for after in (299, 400, -1, "149", True):
            with self.subTest(after_frame=after):
                with self.assertRaises(worker.ScriptError):
                    worker.validate_state_sidecar(self.meta(after_frame=after), self.ACTUAL, 300)
        worker.validate_state_sidecar(self.meta(after_frame=298), self.ACTUAL, 300)

    def test_incomplete_sidecar_is_rejected(self) -> None:
        for broken in ({}, "not an object", self.meta(post_serialize=None), {k: v for k, v in self.meta().items() if k != "rom_sha256"}):
            with self.subTest(sidecar=broken):
                with self.assertRaises(worker.ScriptError):
                    worker.validate_state_sidecar(broken, self.ACTUAL, 300)


class WorkerErrorPathTests(unittest.TestCase):
    def test_missing_core_is_missing_prerequisite(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            r = subprocess.run([sys.executable, str(commands.WORKER), "--core", f"{tmp}/absent.dylib", "--probe"],
                               capture_output=True, text=True, timeout=60)
            self.assertEqual(r.returncode, EXIT_MISSING_PREREQUISITE, r.stdout + r.stderr)
            self.assertIn("error", json.loads(r.stdout))

    def test_invalid_script_is_invalid_input_before_loading_anything(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            script = Path(tmp) / "bad.json"
            script.write_text(json.dumps({"schema_version": 1, "frames": -5}))
            r = subprocess.run([sys.executable, str(commands.WORKER), "--core", f"{tmp}/absent.dylib", "--rom", f"{tmp}/absent.sfc",
                                "--script", str(script), "--samples-out", f"{tmp}/s.json"], capture_output=True, text=True, timeout=60)
            self.assertEqual(r.returncode, EXIT_INVALID_INPUT, r.stderr)

    def test_missing_rom_is_missing_prerequisite(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            r = subprocess.run([sys.executable, str(commands.WORKER), "--core", f"{tmp}/absent.dylib", "--rom", f"{tmp}/absent.sfc",
                                "--script", str(SCRIPTS / "boot-300.json"), "--samples-out", f"{tmp}/s.json"], capture_output=True, text=True, timeout=60)
            self.assertEqual(r.returncode, EXIT_MISSING_PREREQUISITE, r.stderr)

    def test_unknown_serialization_method_is_invalid_input(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            r = subprocess.run([sys.executable, str(commands.WORKER), "--core", f"{tmp}/absent.dylib", "--rom", f"{tmp}/absent.sfc",
                                "--script", str(SCRIPTS / "boot-300.json"), "--samples-out", f"{tmp}/s.json", "--serialization-method", "Loose"],
                               capture_output=True, text=True, timeout=60)
            self.assertEqual(r.returncode, EXIT_INVALID_INPUT, r.stderr)

    def test_worker_command_forwards_the_serialization_method(self) -> None:
        p = commands.Prepared()
        p.core = {"library": "/lib"}
        p.rom = Path("/rom")
        p.serialization_method = "Fast"
        cmd = commands._worker_command(p, Path("/script.json"), Path("/out/samples.json"), save_after=3, state_out=Path("/out/s.bst"), sample_from=3)
        self.assertIn("Fast", cmd[cmd.index("--serialization-method") + 1])
        self.assertEqual(cmd[cmd.index("--save-after") + 1], "3")
        self.assertEqual(cmd[cmd.index("--sample-from-frame") + 1], "3")
        self.assertNotIn("--sample-from-frame", commands._worker_command(p, Path("/script.json"), Path("/out/samples.json")))

    def test_stop_frame_and_fields_arguments_are_validated_before_loading_anything(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            common = [sys.executable, str(commands.WORKER), "--core", f"{tmp}/absent.dylib", "--rom", f"{tmp}/absent.sfc",
                      "--script", str(SCRIPTS / "boot-300.json"), "--samples-out", f"{tmp}/s.json"]
            for extra in (["--stop-after-frame", "300"], ["--stop-after-frame", "-1"], ["--fields", f"{tmp}/absent-fields.json"]):
                with self.subTest(extra=extra):
                    r = subprocess.run(common + extra, capture_output=True, text=True, timeout=60)
                    self.assertEqual(r.returncode, EXIT_INVALID_INPUT, r.stderr)
            bad_fields = Path(tmp) / "fields.json"
            bad_fields.write_text(json.dumps([{"name": "a", "start": 0, "length": 0}]))
            r = subprocess.run(common + ["--fields", str(bad_fields)], capture_output=True, text=True, timeout=60)
            self.assertEqual(r.returncode, EXIT_INVALID_INPUT, r.stderr)
            good_fields = Path(tmp) / "good.json"
            good_fields.write_text(json.dumps([{"name": "a", "start": 0, "length": 1}]))
            r = subprocess.run(common + ["--fields", str(good_fields), "--stop-after-frame", "299"], capture_output=True, text=True, timeout=60)
            self.assertEqual(r.returncode, EXIT_MISSING_PREREQUISITE, r.stderr)  # arguments fine; the ROM is absent

    def test_save_arguments_must_pair(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            r = subprocess.run([sys.executable, str(commands.WORKER), "--core", f"{tmp}/absent.dylib", "--rom", f"{tmp}/absent.sfc",
                                "--script", str(SCRIPTS / "boot-300.json"), "--samples-out", f"{tmp}/s.json", "--save-after", "3"],
                               capture_output=True, text=True, timeout=60)
            self.assertEqual(r.returncode, EXIT_INVALID_INPUT, r.stderr)


class CommandPrerequisiteTests(unittest.TestCase):
    def test_unbuilt_core_is_missing_not_pass(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            lock = write_lock(Path(tmp))
            report = Path(tmp) / "report.json"
            r = run_cli("reference", "run", "--lock", str(lock), "--script", str(SCRIPTS / "boot-300.json"),
                        "--rom", str(Path(tmp) / "absent.sfc"), "--report", str(report))
            self.assertEqual(r.returncode, EXIT_MISSING_PREREQUISITE, r.stderr)
            rep = json.loads(report.read_text())
            outcomes = {c["name"]: c["outcome"] for c in rep["checks"]}
            self.assertEqual(outcomes["core_available"], "missing")
            self.assertEqual(rep["status"], "failed")
            self.assertNotIn("reference_run", outcomes)

    def test_unknown_core_and_bad_lock_are_invalid_input(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            lock = write_lock(Path(tmp))
            r = run_cli("reference", "verify", "--lock", str(lock), "--core", "nonesuch", "--script", str(SCRIPTS / "boot-300.json"))
            self.assertEqual(r.returncode, EXIT_INVALID_INPUT, r.stderr)
            bad = Path(tmp) / "bad.json"
            bad.write_text("{}")
            r = run_cli("reference", "build", "--lock", str(bad))
            self.assertEqual(r.returncode, EXIT_INVALID_INPUT, r.stderr)
            r = run_cli("reference", "run", "--lock", str(lock), "--script", str(SCRIPTS / "boot-300.json"), "--timeout", "0")
            self.assertEqual(r.returncode, EXIT_INVALID_INPUT, r.stderr)

    def test_build_without_checkout_and_network_is_missing(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            lock = write_lock(Path(tmp))
            report = Path(tmp) / "report.json"
            r = run_cli("reference", "build", "--lock", str(lock), "--no-network", "--report", str(report))
            self.assertEqual(r.returncode, EXIT_MISSING_PREREQUISITE, r.stderr)
            outcomes = {c["name"]: c["outcome"] for c in json.loads(report.read_text())["checks"]}
            self.assertEqual(outcomes["checkout_pinned"], "missing")

    def test_build_refuses_a_core_without_adapter(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            lock = write_lock(Path(tmp))
            r = run_cli("reference", "build", "--lock", str(lock), "--core", "mesen", "--no-network")
            self.assertEqual(r.returncode, EXIT_INVALID_INPUT, r.stderr)

    def test_stale_core_manifest_is_rejected(self) -> None:
        """A built core recorded against another commit or patch must not be used."""
        with tempfile.TemporaryDirectory() as tmp:
            lock = write_lock(Path(tmp))
            lockdata = json.loads(lock.read_text())
            checkout = Path(lockdata["install_dir"]) / "bsnes"
            checkout.mkdir(parents=True)
            fake_lib = checkout / "core.dylib"
            fake_lib.write_bytes(b"not a core")
            manifest = {"schema_version": 1, "core": "bsnes", "commit": "0" * 40, "patch_sha256": lockdata["cores"]["bsnes"]["patch_sha256"],
                        "library": str(fake_lib), "library_sha256": commands.sha256_file(fake_lib), "api_version": 1}
            (checkout / "lab-core.json").write_text(json.dumps(manifest))
            report = Path(tmp) / "report.json"
            r = run_cli("reference", "run", "--lock", str(lock), "--script", str(SCRIPTS / "boot-300.json"), "--report", str(report))
            self.assertEqual(r.returncode, EXIT_FAILURE, r.stderr)
            outcomes = {c["name"]: c["outcome"] for c in json.loads(report.read_text())["checks"]}
            self.assertEqual(outcomes["core_available"], "failed")


if __name__ == "__main__":
    unittest.main()
