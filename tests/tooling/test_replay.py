"""ROM-free checks for replay manifests and the comparator (M0-04, M0-05).

Runs that need the pinned core and the ROM go through ``project.py replay
run|compare`` and are recorded as task evidence. Here we prove the manifest
contract (invalid manifests are rejected with exit 3), the derivation of the
reference script, the comparison and divergence logic on synthetic samples,
the origin resolution (absent state or restore-check evidence is missing,
not a pass) and the command error paths.

``StubbedCompareTests`` (M0-05) adds ``replay compare`` end to end with the
worker process stubbed out, which covers the post-run wiring the ROM-bound
evidence was the only check of: the fresh-process check, the required or
optional ``av_identical`` rule, the optional localization re-runs, the
divergence report and the mapping from worker outcomes to exit codes. The
fixture names a core library and a ROM that do not exist, so nothing there
can reach an emulator.
"""

from __future__ import annotations

import argparse
import contextlib
import copy
import hashlib
import io
import json
import os
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

from unirally_lab import EXIT_FAILURE, EXIT_INVALID_INPUT, EXIT_MISSING_PREREQUISITE, EXIT_OK, EXIT_TIMEOUT  # noqa: E402
from unirally_lab import procs  # noqa: E402
from unirally_lab import report as reportmod  # noqa: E402
from unirally_lab.compare import samples as cmp  # noqa: E402
from unirally_lab.reference import worker  # noqa: E402
from unirally_lab.replay import commands as replay  # noqa: E402
from unirally_lab.replay import manifest as mf  # noqa: E402

PROJECT = ROOT / "tools" / "project.py"
MANIFESTS = ROOT / "tests" / "manifests" / "replay"
PRIMARY = MANIFESTS / "boot-start-600.json"


def run_cli(*args: str) -> subprocess.CompletedProcess:
    return subprocess.run([sys.executable, str(PROJECT), *args], capture_output=True, text=True, timeout=300)


def primary() -> dict:
    return json.loads(PRIMARY.read_text())


def outcomes(report: Path) -> dict[str, str]:
    return {c["name"]: c["outcome"] for c in json.loads(report.read_text())["checks"]}


class ManifestSchemaTests(unittest.TestCase):
    def test_tracked_manifests_are_valid(self) -> None:
        paths = sorted(MANIFESTS.glob("*.json"))
        self.assertTrue(paths)
        for path in paths:
            with self.subTest(manifest=path.name):
                m = mf.load_manifest(path)
                self.assertEqual(m["scenario_id"], path.stem)
                r = run_cli("replay", "validate", "--manifest", str(path))
                self.assertEqual(r.returncode, EXIT_OK, r.stderr)

    def test_invalid_manifests_are_rejected(self) -> None:
        def mutate(**changes):
            m = primary()
            for dotted, value in changes.items():
                target = m
                keys = dotted.split(".")
                for k in keys[:-1]:
                    target = target[int(k)] if isinstance(target, list) else target[k]
                last = keys[-1]
                if value is KeyError:
                    del target[last]
                elif isinstance(target, list):
                    target[int(last)] = value
                else:
                    target[last] = value
            return m
        state_origin = {"kind": "state", "path": "local/states/x.bst", "sha256": "a" * 64, "script": "tests/manifests/reference/boot-start-600.json",
                        "script_sha256": "b" * 64, "after_frame": 150, "restore_check": {"report": "local/states/rc.json", "save_after": 150}}
        bad = {
            "schema": mutate(schema_version=2),
            "scenario id": mutate(scenario_id="bad id!"),
            "no description": mutate(description=KeyError),
            "empty regeneration": mutate(regeneration_command=" "),
            "rom digest": mutate(**{"rom.sha256": "abc"}),
            "absolute rom manifest": mutate(**{"rom.manifest": "/etc/passwd"}),
            "parent path": mutate(**{"rom.manifest": "../outside.json"}),
            "other core": mutate(**{"core.name": "mesen"}),
            "short commit": mutate(**{"core.commit": "7d5aa1e"}),
            "method": mutate(**{"core.serialization_method": "Loose"}),
            "unknown option": mutate(**{"core.options": {"bsnes_turbo": "ON"}}),
            "option type": mutate(**{"core.options": {"bsnes_entropy": 1}}),
            "zero frames": mutate(**{"run.frames": 0}),
            "bool frames": mutate(**{"run.frames": True}),
            "sample_every": mutate(**{"run.sample_every": 0}),
            "trace": mutate(**{"run.trace_entries": -1}),
            "timing unit": mutate(**{"inputs.timing_unit": "ms"}),
            "injection": mutate(**{"inputs.injection_point": ""}),
            "one controller": mutate(**{"inputs.controllers": [{"port": 0, "events": []}]}),
            "duplicate port": mutate(**{"inputs.controllers": [{"port": 0, "events": []}, {"port": 0, "events": []}]}),
            "event order": mutate(**{"inputs.controllers.0.events.0": {"from": 305, "to": 300, "buttons": ["start"]}}),
            "event past end": mutate(**{"inputs.controllers.0.events.0": {"from": 300, "to": 600, "buttons": ["start"]}}),
            "event button": mutate(**{"inputs.controllers.0.events.0": {"from": 300, "to": 305, "buttons": ["turbo"]}}),
            "event empty buttons": mutate(**{"inputs.controllers.0.events.0": {"from": 300, "to": 305, "buttons": []}}),
            "event port": mutate(**{"inputs.controllers.0.events.0": {"from": 300, "to": 305, "port": 1, "buttons": ["start"]}}),
            "no fields": mutate(fields=[]),
            "duplicate field": mutate(fields=[{"name": "a", "kind": "wram_range", "start": 0, "length": 1}, {"name": "a", "kind": "wram_range", "start": 1, "length": 1}]),
            "field kind": mutate(fields=[{"name": "a", "kind": "vram_range", "start": 0, "length": 1}]),
            "negative start": mutate(fields=[{"name": "a", "kind": "wram_range", "start": -1, "length": 1}]),
            "beyond wram": mutate(fields=[{"name": "a", "kind": "wram_range", "start": 0x1FFFF, "length": 2}]),
            "zero length": mutate(fields=[{"name": "a", "kind": "wram_range", "start": 0, "length": 0}]),
            "reserved name": mutate(fields=[{"name": "wram_sha256", "kind": "wram_range", "start": 0, "length": 1}]),
            "misnamed builtin": mutate(fields=[{"name": "regs", "kind": "registers"}]),
            "origin kind": mutate(origin={"kind": "snapshot"}),
            "state without path": mutate(origin={k: v for k, v in state_origin.items() if k != "path"}),
            "state after last frame": mutate(origin={**state_origin, "after_frame": 599, "restore_check": {"report": "r.json", "save_after": 599}}),
            "restore check mismatch": mutate(origin={**state_origin, "restore_check": {"report": "r.json", "save_after": 149}}),
            "restore check missing": mutate(origin={k: v for k, v in state_origin.items() if k != "restore_check"}),
            "writes not a list": mutate(cartridge_ram_writes={"after_frame": 10, "offset": 0, "byte": 1}),
            "write key": mutate(cartridge_ram_writes=[{"after_frame": 10, "offset": 0, "value": 1}]),
            "write after last frame": mutate(cartridge_ram_writes=[{"after_frame": 599, "offset": 0, "byte": 1}]),
            "write offset": mutate(cartridge_ram_writes=[{"after_frame": 10, "offset": 0x2000, "byte": 1}]),
            "write byte": mutate(cartridge_ram_writes=[{"after_frame": 10, "offset": 0, "byte": 256}]),
            "write bool": mutate(cartridge_ram_writes=[{"after_frame": 10, "offset": 0, "byte": True}]),
            "write before state": mutate(origin=state_origin, cartridge_ram_writes=[{"after_frame": 150, "offset": 0, "byte": 1}]),
            "expected key": mutate(expected={"av_digest": "a" * 64}),
            "expected digest": mutate(expected={"sample_digest": "xyz"}),
        }
        for label, data in bad.items():
            with self.subTest(case=label):
                with self.assertRaises(mf.ManifestError):
                    mf.validate_manifest(data)
        mf.validate_manifest(mutate(origin=state_origin))  # the state origin itself is well-formed

    def test_cli_rejects_invalid_or_absent_manifest_with_exit_3(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            bad = Path(tmp) / "bad.json"
            bad.write_text(json.dumps({"schema_version": 1}))
            bad_range = Path(tmp) / "bad-range.json"  # review 1, M1: escaped as a traceback (exit 1) before
            m = primary()
            m["fields"].append({"name": "neg", "kind": "wram_range", "start": -1, "length": 1})
            bad_range.write_text(json.dumps(m))
            nested = Path(tmp) / "nested.json"
            nested.write_text(json.dumps({**primary(), "inputs": {"timing_unit": "frame", "injection_point": "x", "controllers": [0, 1]}}))
            report = Path(tmp) / "report.json"
            for sub in ("validate", "run", "compare"):
                for path in (bad, bad_range, nested):
                    with self.subTest(command=sub, manifest=path.name):
                        report.unlink(missing_ok=True)
                        r = run_cli("replay", sub, "--manifest", str(path), "--report", str(report))
                        self.assertEqual(r.returncode, EXIT_INVALID_INPUT, r.stderr)
                        self.assertNotIn("Traceback", r.stderr)
                        self.assertEqual(json.loads(report.read_text())["status"], "failed")
            r = run_cli("replay", "validate", "--manifest", str(Path(tmp) / "absent.json"))
            self.assertEqual(r.returncode, EXIT_INVALID_INPUT, r.stderr)

    def test_validate_reports_lock_and_rom_identity_mismatch(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            m = primary()
            m["core"]["commit"] = "0" * 40
            path = Path(tmp) / "m.json"
            path.write_text(json.dumps(m))
            report = Path(tmp) / "report.json"
            r = run_cli("replay", "validate", "--manifest", str(path), "--report", str(report))
            self.assertEqual(r.returncode, EXIT_FAILURE, r.stderr)
            self.assertEqual(outcomes(report)["replay_core_identity_matches_lock"], "failed")
            m = primary()
            m["rom"]["sha256"] = "f" * 64
            path.write_text(json.dumps(m))
            r = run_cli("replay", "validate", "--manifest", str(path), "--report", str(report))
            self.assertEqual(r.returncode, EXIT_FAILURE, r.stderr)
            self.assertEqual(outcomes(report)["replay_rom_identity_matches_manifest"], "failed")


class DerivationTests(unittest.TestCase):
    def test_derived_script_is_a_valid_schema_1_script(self) -> None:
        script = mf.derive_script(mf.load_manifest(PRIMARY))
        self.assertEqual(script["schema_version"], 1)
        self.assertEqual(script["frames"], 600)
        self.assertEqual(script["inputs"], [{"from": 300, "to": 305, "port": 0, "buttons": ["start"]}])
        self.assertEqual(script["core_options"], {})
        worker.validate_script(script)

    def test_derived_script_matches_the_tracked_reference_script(self) -> None:
        tracked = worker.load_script(ROOT / "tests" / "manifests" / "reference" / "boot-start-600.json")
        self.assertEqual(mf.script_equivalent(tracked, mf.derive_script(mf.load_manifest(PRIMARY))), [])

    def test_script_equivalence_reports_the_differing_keys(self) -> None:
        derived = mf.derive_script(mf.load_manifest(PRIMARY))
        other = copy.deepcopy(derived)
        other["frames"] = 601
        other["inputs"] = []
        self.assertEqual(mf.script_equivalent(other, derived), ["frames", "inputs"])
        reordered = copy.deepcopy(derived)
        reordered["inputs"][0]["buttons"] = ["start"]
        self.assertEqual(mf.script_equivalent(reordered, derived), [])

    def test_cartridge_ram_writes_reach_the_script_and_its_equivalence(self) -> None:
        plain = mf.derive_script(mf.load_manifest(PRIMARY))
        self.assertNotIn("cartridge_ram_writes", plain)  # the scripts of manifests without writes are unchanged
        m = primary()
        m["cartridge_ram_writes"] = [{"after_frame": 400, "offset": 0x1076, "byte": 1}]
        derived = mf.derive_script(mf.validate_manifest(m))
        self.assertEqual(derived["cartridge_ram_writes"], [{"after_frame": 400, "offset": 0x1076, "byte": 1}])
        self.assertEqual(mf.script_equivalent(plain, derived), ["cartridge_ram_writes"])
        self.assertEqual(mf.script_equivalent(derived, derived), [])

    def test_inputs_at_frame_covers_both_ports(self) -> None:
        m = mf.load_manifest(PRIMARY)
        self.assertEqual(mf.inputs_at(m, 299), {"0": [], "1": []})
        self.assertEqual(mf.inputs_at(m, 300), {"0": ["start"], "1": []})
        self.assertEqual(mf.inputs_at(m, 305), {"0": ["start"], "1": []})
        self.assertEqual(mf.inputs_at(m, 306), {"0": [], "1": []})
        self.assertEqual(mf.range_fields(m), [{"name": "wram_0000_0200", "start": 0, "length": 512}])
        self.assertEqual(mf.field_names(m), ["wram_sha256", "registers", "wram_0000_0200"])


def make_samples(frames, final="F", range_len=4, tweak=None, schema=2):
    out = {
        "schema_version": schema,
        "fields": [{"name": "r", "start": 0x100, "length": range_len}],
        "frames": [{"frame": n, "wram_sha256": f"w{n}", "registers": {"pc": n, "a": 1},
                    "fields": {"r": "00" * range_len}, "video": [1, 1, f"v{n}"], "audio_sha256": f"a{n}"} for n in frames],
        "final": {"state_sha256": final},
        "sample_digest": "d",
        "end_frame": max(frames) if frames else None,
        "trace": {"instructions_executed": 10, "window": [{"pc": 1}]},
    }
    if tweak:
        tweak(out)
    return out


FIELDS = ["wram_sha256", "registers", "r"]
STARTS = {"r": 0x100}


class ComparatorTests(unittest.TestCase):
    def test_identical_runs(self) -> None:
        r = cmp.compare_samples(make_samples(range(5)), make_samples(range(5)), FIELDS, STARTS)
        self.assertTrue(r["identical"])
        self.assertIsNone(r["first_divergence"])
        self.assertEqual(r["compared"], 5)
        self.assertTrue(r["av_identical"] and r["final_state_identical"])

    def test_digest_divergence_reports_frame_prior_sample_and_values(self) -> None:
        def poke(out):
            out["frames"][3]["wram_sha256"] = "other"
        r = cmp.compare_samples(make_samples(range(5)), make_samples(range(5), tweak=poke), FIELDS, STARTS)
        self.assertFalse(r["identical"])
        fd = r["first_divergence"]
        self.assertEqual(fd["frame"], 3)
        self.assertEqual(fd["differing_fields"], ["wram_sha256"])
        self.assertEqual(fd["differences"][0], {"field": "wram_sha256", "left": "w3", "right": "other"})
        self.assertEqual(fd["prior_sample"]["frame"], 2)
        self.assertEqual(fd["prior_sample"]["left"]["wram_sha256"], "w2")
        self.assertEqual(fd["prior_sample"]["right"]["registers"], {"pc": 2, "a": 1})

    def test_divergence_on_the_first_frame_has_no_prior_sample(self) -> None:
        def poke(out):
            out["frames"][0]["registers"] = {"pc": 9, "a": 1}
        fd = cmp.compare_samples(make_samples(range(3)), make_samples(range(3), tweak=poke), FIELDS, STARTS)["first_divergence"]
        self.assertEqual(fd["frame"], 0)
        self.assertIsNone(fd["prior_sample"])
        self.assertEqual(fd["differences"][0]["differing_registers"], [{"register": "pc", "left": 0, "right": 9}])

    def test_range_divergence_lists_absolute_offsets(self) -> None:
        def poke(out):
            out["frames"][2]["fields"]["r"] = "00100000"
        fd = cmp.compare_samples(make_samples(range(4)), make_samples(range(4), tweak=poke), FIELDS, STARTS)["first_divergence"]
        self.assertEqual(fd["differing_fields"], ["r"])
        self.assertEqual(fd["differences"][0]["differing_bytes"], 1)
        self.assertEqual(fd["differences"][0]["first"], [{"offset": "0x00101", "left": "0x00", "right": "0x10"}])

    def test_only_declared_fields_are_compared(self) -> None:
        def poke(out):
            out["frames"][2]["fields"]["r"] = "ff" * 4
        r = cmp.compare_samples(make_samples(range(4)), make_samples(range(4), tweak=poke), ["wram_sha256", "registers"], {})
        self.assertIsNone(r["first_divergence"])

    def test_frame_sets_and_final_state(self) -> None:
        r = cmp.compare_samples(make_samples([0, 1, 2, 3]), make_samples([0, 2, 3, 4]), FIELDS, STARTS)
        self.assertEqual(r["only_in_left"], [1])
        self.assertEqual(r["only_in_right"], [4])
        self.assertEqual(r["compared"], 3)
        self.assertFalse(r["identical"])
        self.assertIsNone(r["first_divergence"])
        r = cmp.compare_samples(make_samples(range(3)), make_samples(range(3), final="G"), FIELDS, STARTS)
        self.assertFalse(r["identical"])
        self.assertFalse(r["final_state_identical"])
        self.assertFalse(cmp.compare_samples(make_samples([0]), make_samples([5]), FIELDS, STARTS)["identical"])

    def test_samples_without_the_declared_field_or_schema_are_refused(self) -> None:
        with self.assertRaises(cmp.SamplesError):
            cmp.compare_samples(make_samples(range(2)), make_samples(range(2)), ["wram_sha256", "missing"], {})
        with self.assertRaises(cmp.SamplesError):
            cmp.compare_samples(make_samples(range(2), schema=1), make_samples(range(2)), FIELDS, STARTS)

    def test_byte_diff_counts_all_and_lists_a_bounded_prefix(self) -> None:
        left, right = bytes(200), bytes([1]) * 200
        d = cmp.diff_bytes(left, right, base=0x73)
        self.assertEqual(d["differing_bytes"], 200)
        self.assertEqual(len(d["first"]), cmp.MAX_LISTED_DIFFERENCES)
        self.assertEqual(d["first"][0], {"offset": "0x00073", "left": "0x00", "right": "0x01"})
        self.assertIsNone(cmp.diff_bytes(bytes(2), bytes(3))["differing_bytes"])
        self.assertEqual(cmp.diff_registers({"pc": 1, "a": 2}, {"pc": 1, "a": 3}), [{"register": "a", "left": 2, "right": 3}])

    def test_trace_windows_come_from_both_samples(self) -> None:
        t = cmp.trace_windows(make_samples(range(3)), make_samples(range(3)))
        self.assertEqual(t["left"]["end_frame"], 2)
        self.assertEqual(t["right"]["window"], [{"pc": 1}])


class OriginResolutionTests(unittest.TestCase):
    """The origin of a state manifest must exist, match its digests and carry
    passing restore-check evidence; anything else is missing or failed."""

    SCRIPT_REL = "tests/manifests/reference/boot-start-600.json"

    def setUp(self) -> None:
        self.tmp = Path(tempfile.mkdtemp())
        self.root = self.tmp / "root"
        (self.root / "tests" / "manifests" / "reference").mkdir(parents=True)
        shutil.copy(ROOT / self.SCRIPT_REL, self.root / self.SCRIPT_REL)
        self.script_sha = reportmod.file_sha256(self.root / self.SCRIPT_REL)
        self.state = self.root / "local" / "states" / "s.bst"
        self.state.parent.mkdir(parents=True)
        self.state.write_bytes(b"state bytes")
        self.state_sha = reportmod.file_sha256(self.state)
        self.write_sidecar()
        self.report = self.root / "local" / "states" / "rc.json"
        self.write_report()
        self.art = self.tmp / "art"
        self.art.mkdir()

    def tearDown(self) -> None:
        shutil.rmtree(self.tmp, ignore_errors=True)

    def write_sidecar(self, **overrides) -> None:
        meta = {"after_frame": 150, "sha256": self.state_sha, "script_sha256": self.script_sha, "serialization_method": "Strict"}
        meta.update(overrides)
        self.state.with_suffix(".bst.json").write_text(json.dumps(meta))

    def write_report(self, failing: str | None = None, state_sha: str | None = None, save_after: int = 150) -> None:
        checks = [{"name": c, "outcome": "failed" if c == failing else "passed", "required": True} for c in mf.RESTORE_CHECKS]
        rep = {"checks": checks, "inputs": {"script": {"sha256": self.script_sha}},
               "artifacts": [{"kind": "state", "sha256": state_sha or self.state_sha}],
               "samples": {"restore_and_continue": {"start_frame": save_after + 1}}}
        self.report.write_text(json.dumps(rep))

    def manifest(self, **origin_overrides) -> dict:
        m = primary()
        m["origin"] = {"kind": "state", "path": "local/states/s.bst", "sha256": self.state_sha, "script": self.SCRIPT_REL,
                       "script_sha256": self.script_sha, "after_frame": 150,
                       "restore_check": {"report": "local/states/rc.json", "save_after": 150}}
        m["origin"].update(origin_overrides)
        return mf.validate_manifest(m)

    def resolve(self, manifest: dict):
        rep = reportmod.Report(["test"])
        side = replay._resolve_side(rep, self.root, self.art, replay.Side("x", manifest))
        return side, {c["name"]: c for c in rep.data["checks"]}

    def test_cold_start_writes_the_derived_script_and_fields(self) -> None:
        side, checks = self.resolve(mf.load_manifest(PRIMARY))
        self.assertEqual(side.status, EXIT_OK)
        self.assertEqual(checks["x_origin_available"]["outcome"], "passed")
        self.assertEqual(worker.load_script(side.script)["frames"], 600)
        self.assertEqual(json.loads(side.fields.read_text()), [{"name": "wram_0000_0200", "start": 0, "length": 512}])
        self.assertIsNone(side.state_in)

    def test_valid_state_origin_is_accepted(self) -> None:
        side, checks = self.resolve(self.manifest())
        self.assertEqual(side.status, EXIT_OK, checks)
        self.assertEqual(checks["x_origin_available"]["outcome"], "passed")
        self.assertEqual(checks["x_origin_restore_check"]["outcome"], "passed")
        self.assertEqual(side.state_in, self.state)
        self.assertEqual(side.script, self.root / self.SCRIPT_REL)

    def test_absent_state_is_missing(self) -> None:
        self.state.unlink()
        side, checks = self.resolve(self.manifest())
        self.assertEqual(side.status, EXIT_MISSING_PREREQUISITE)
        self.assertEqual(checks["x_origin_available"]["outcome"], "missing")
        self.assertIn("regenerate", checks["x_origin_available"]["detail"])

    def test_absent_sidecar_or_script_is_missing(self) -> None:
        self.state.with_suffix(".bst.json").unlink()
        side, checks = self.resolve(self.manifest())
        self.assertEqual(side.status, EXIT_MISSING_PREREQUISITE)
        self.write_sidecar()
        (self.root / self.SCRIPT_REL).unlink()
        side, checks = self.resolve(self.manifest())
        self.assertEqual(side.status, EXIT_MISSING_PREREQUISITE)

    def test_state_or_sidecar_mismatch_fails(self) -> None:
        for label, manifest_override, sidecar_override in (
            ("state digest", {"sha256": "0" * 64}, {}),
            ("script digest", {"script_sha256": "0" * 64}, {}),
            ("sidecar frame", {}, {"after_frame": 149}),
            ("sidecar method", {}, {"serialization_method": "Fast"}),
            ("sidecar state", {}, {"sha256": "0" * 64}),
        ):
            with self.subTest(case=label):
                self.write_sidecar(**sidecar_override)
                side, checks = self.resolve(self.manifest(**manifest_override))
                self.assertEqual(side.status, EXIT_FAILURE)
                self.assertEqual(checks["x_origin_available"]["outcome"], "failed")
        self.write_sidecar()

    def test_absent_restore_check_report_is_missing(self) -> None:
        self.report.unlink()
        side, checks = self.resolve(self.manifest())
        self.assertEqual(side.status, EXIT_MISSING_PREREQUISITE)
        self.assertEqual(checks["x_origin_restore_check"]["outcome"], "missing")

    def test_perturbing_or_foreign_restore_check_fails(self) -> None:
        for c in mf.RESTORE_CHECKS:
            with self.subTest(failing=c):
                self.write_report(failing=c)
                side, checks = self.resolve(self.manifest())
                self.assertEqual(side.status, EXIT_FAILURE)
                self.assertIn(c, checks["x_origin_restore_check"]["detail"])
        self.write_report(state_sha="0" * 64)
        side, checks = self.resolve(self.manifest())
        self.assertEqual(side.status, EXIT_FAILURE)
        self.assertIn("another", checks["x_origin_restore_check"]["detail"])

    def test_restore_check_must_be_for_the_manifests_save_point(self) -> None:
        """Editing after_frame in both sidecar and manifest must not be enough (review 1, m1):
        the report's measured resume frame ties the evidence to the save point."""
        self.write_sidecar(after_frame=151)
        self.write_report(save_after=150)
        side, checks = self.resolve(self.manifest(after_frame=151, restore_check={"report": "local/states/rc.json", "save_after": 151}))
        self.assertEqual(side.status, EXIT_FAILURE)
        self.assertIn("save point 150", checks["x_origin_restore_check"]["detail"])
        self.write_report(save_after=151)
        side, checks = self.resolve(self.manifest(after_frame=151, restore_check={"report": "local/states/rc.json", "save_after": 151}))
        self.assertEqual(side.status, EXIT_OK)
        report = self.report.read_text()
        self.report.write_text(report.replace('"samples"', '"samples_gone"'))
        side, checks = self.resolve(self.manifest(after_frame=151, restore_check={"report": "local/states/rc.json", "save_after": 151}))
        self.assertEqual(side.status, EXIT_FAILURE)
        self.assertIn("unreadable", checks["x_origin_restore_check"]["detail"])


class CommandPrerequisiteTests(unittest.TestCase):
    def make_root(self, tmp: Path) -> Path:
        root = tmp / "root"
        for rel in ("tools/locks/emulators.json", "tests/manifests/rom/unirally-pal.json"):
            (root / rel).parent.mkdir(parents=True, exist_ok=True)
            shutil.copy(ROOT / rel, root / rel)
        return root

    def test_unbuilt_core_is_missing_not_pass(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = self.make_root(Path(tmp))
            report = Path(tmp) / "report.json"
            for sub in ("run", "compare"):
                with self.subTest(command=sub):
                    r = run_cli("replay", sub, "--manifest", str(PRIMARY), "--root", str(root), "--rom", str(Path(tmp) / "absent.sfc"), "--report", str(report))
                    self.assertEqual(r.returncode, EXIT_MISSING_PREREQUISITE, r.stderr)
                    out = outcomes(report)
                    self.assertEqual(out["core_available"], "missing")
                    self.assertNotIn("fields_identical", out)
                    self.assertNotIn("reference_run", out)

    def test_incomparable_manifests_and_bad_arguments_are_invalid_input(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            other = primary()
            other["scenario_id"] = "other-fields"
            other["fields"] = other["fields"][:2]
            path = Path(tmp) / "other.json"
            path.write_text(json.dumps(other))
            report = Path(tmp) / "report.json"
            r = run_cli("replay", "compare", "--manifest", str(PRIMARY), "--against", str(path), "--report", str(report))
            self.assertEqual(r.returncode, EXIT_INVALID_INPUT, r.stderr)
            self.assertEqual(outcomes(report)["manifests_comparable"], "failed")
            for args in (("--runs", "1"), ("--against", str(PRIMARY), "--runs", "3"), ("--timeout", "0")):
                with self.subTest(args=args):
                    r = run_cli("replay", "compare", "--manifest", str(PRIMARY), *args)
                    self.assertEqual(r.returncode, EXIT_INVALID_INPUT, r.stderr)


# --------------------------------------------------- stubbed-worker compare
#
# ``replay compare``'s post-run wiring (fresh-process check, the required or
# optional ``av_identical`` rule, the optional localization re-runs, the
# divergence report and the exit codes) was only ever exercised with the ROM
# and the built core (M0-04 review 2, note (a)). The stub below stands in for
# the worker process so those paths are regression-safe without either.

# ``print_summary``'s stream default is bound at definition time, so
# ``redirect_stderr`` does not reach it; the real renderer is called into a buffer.
PRINT_SUMMARY = reportmod.print_summary

STUB_WRAM_SIZE = 0x200
STUB_DIVERGENT_OFFSET = 0x100


def parse_worker_command(command: list) -> dict:
    """The worker's argv as a flag map; every flag ``_worker_command`` emits takes a value."""
    argv = [str(c) for c in command]
    out: dict = {"argv": argv}
    it = iter(argv[2:])
    for token in it:
        if token.startswith("--"):
            out[token] = next(it)
    return out


def held_buttons(script: dict, frame: int, port: int) -> set:
    return {b for e in script["inputs"] if e.get("port", 0) == port and e["from"] <= frame <= e["to"] for b in e["buttons"]}


class StubWorker:
    """Stands in for a fresh ``reference/worker.py`` process: no core, no ROM.

    Work RAM byte ``0x73`` holds ``0x10`` exactly while ``start`` is pressed on
    port 0 — the shape of the real ROM's behaviour recorded in R-0003, without
    executing anything. ``outcomes`` gives the per-call result (``ok``,
    ``timeout``, ``missing``, ``crash``, ``garbage``), ``pids`` the process
    identity each call reports, ``av`` a per-call video/audio tag that leaves
    work RAM, registers and the final state untouched, and ``divergent`` the
    call indices whose work RAM differs from frame 2 on, and ``localize_defects``
    the call indices whose result is inconsistent with what was asked of them:
    ``short`` stops one frame before the requested ``--stop-after-frame``, and
    ``dump_mismatch`` leaves a work RAM dump on disk that is not the one the
    samples file records a digest of (the shape of a stale or orphaned file in
    the artifacts directory, R-0004 G2/G3).
    """

    def __init__(self, rom_sha256: str, outcomes=(), pids=None, av=None, divergent=(), localize_defects=None) -> None:
        self.rom_sha256 = rom_sha256
        self.outcomes = list(outcomes)
        self.pids = list(pids) if pids is not None else None
        self.av = dict(av or {})
        self.divergent = set(divergent)
        self.localize_defects = dict(localize_defects or {})
        self.calls: list[dict] = []

    def __call__(self, command, timeout, **kwargs):
        index = len(self.calls)
        a = parse_worker_command(command)
        self.calls.append(a)
        outcome = self.outcomes[index] if index < len(self.outcomes) else "ok"
        if outcome == "timeout":
            return procs.RunResult(command, None, "", "", 0.01, timed_out=True)
        if outcome == "missing":
            return procs.RunResult(command, None, "", "no such executable", 0.01, missing=True)
        if outcome == "crash":
            return procs.RunResult(command, EXIT_FAILURE, "", "the core died", 0.01)
        out = Path(a["--samples-out"])
        out.parent.mkdir(parents=True, exist_ok=True)
        if outcome == "garbage":  # a worker killed while writing leaves an unreadable file
            out.write_text("{ this is not json")
            return procs.RunResult(command, 0, "", "", 0.01)
        pid = self.pids[index] if self.pids is not None else 9000 + index
        out.write_text(json.dumps(self.samples(a, pid, self.av.get(index, ""), index in self.divergent,
                                               self.localize_defects.get(index))))
        return procs.RunResult(command, 0, "", "", 0.01)

    def samples(self, a: dict, pid: int, av_tag: str, divergent: bool, defect: str | None = None) -> dict:
        script = json.loads(Path(a["--script"]).read_text())
        ranges = json.loads(Path(a["--fields"]).read_text())
        start = 0
        if "--state-in" in a:
            start = json.loads(Path(a["--state-in"] + ".json").read_text())["after_frame"] + 1
        end = script["frames"] if "--stop-after-frame" not in a else int(a["--stop-after-frame"]) + 1
        if defect == "short":  # the re-run stops before the frame it was asked to stop after
            end -= 1
        every = script.get("sample_every", 1)
        wram = bytearray(STUB_WRAM_SIZE)
        frames = []
        digest = hashlib.sha256()
        av_digest = hashlib.sha256()
        for n in range(start, end):
            wram[0x73] = 0x10 if "start" in held_buttons(script, n, 0) else 0x00
            if divergent and n >= 2:
                wram[STUB_DIVERGENT_OFFSET] = 0xAA
            if (n - start) % every and n != end - 1:
                continue
            wram_sha = hashlib.sha256(bytes(wram)).hexdigest()
            frames.append({"frame": n, "wram_sha256": wram_sha, "registers": {"pc": n, "a": wram[0x73]},
                           "fields": {f["name"]: bytes(wram[f["start"]:f["start"] + f["length"]]).hex() for f in ranges},
                           "video": [1, 1, f"v{n}{av_tag}"], "audio_sha256": f"a{n}{av_tag}"})
            digest.update(f"{n}:{wram_sha}".encode())
            av_digest.update(f"{n}:{av_tag}".encode())
        samples = {
            "schema_version": 2,
            "rom": {"sha256": self.rom_sha256, "region": "PAL"},
            "fields": ranges,
            "frames": frames,
            "process": {"pid": pid, "parent_pid": os.getpid(), "argv": a["argv"]},
            "start_frame": start,
            "end_frame": end - 1,
            "sample_digest": digest.hexdigest(),
            "av_digest": av_digest.hexdigest(),
            "elapsed_seconds": 0.01,
            "trace": {"instructions_executed": 1000 * end, "window": [{"pc": end}]},
            "final": {"state_sha256": hashlib.sha256(b"final:" + bytes(wram)).hexdigest()},
        }
        if "--wram-dump-out" in a:
            dump = Path(a["--wram-dump-out"])
            dump.parent.mkdir(parents=True, exist_ok=True)
            dump.write_bytes(bytes(wram))
            hashed = bytes(wram) if defect != "dump_mismatch" else bytes(wram) + b"\x00"
            samples["wram_dump"] = {"after_frame": end - 1, "path": str(dump),
                                    "sha256": hashlib.sha256(hashed).hexdigest(), "size": len(wram)}
        return samples


class StubbedCompareTests(unittest.TestCase):
    """``replay compare`` end to end with the worker stubbed out.

    ``_prepare`` is replaced by prerequisites that name a core library and a
    ROM which do not exist, so a run that reached the real worker could only
    report ``missing`` — see ``test_without_the_stub_the_worker_cannot_load_a_core``.
    """

    FRAMES = 10
    PRESS = (4, 6)

    def setUp(self) -> None:
        self.tmp = Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, self.tmp, ignore_errors=True)
        self.root = self.tmp / "root"
        for rel in ("tools/locks/emulators.json", "tests/manifests/rom/unirally-pal.json"):
            (self.root / rel).parent.mkdir(parents=True, exist_ok=True)
            shutil.copy(ROOT / rel, self.root / rel)
        self.core = primary()["core"]
        self.rom_sha = primary()["rom"]["sha256"]
        self.cold = self.write_manifest("stub-cold", press=self.PRESS)
        self.removed = self.write_manifest("stub-start-removed", press=None)
        self.artifacts = self.tmp / "artifacts"

    # ------------------------------------------------------------ fixtures

    def manifest_data(self, scenario: str, press: tuple | None, frames: int | None = None) -> dict:
        m = primary()
        m["scenario_id"] = scenario
        m.pop("expected", None)
        m["run"] = {"frames": frames or self.FRAMES, "sample_every": 1, "trace_entries": 4}
        events = [] if press is None else [{"from": press[0], "to": press[1], "buttons": ["start"]}]
        m["inputs"]["controllers"] = [{"port": 0, "events": events}, {"port": 1, "events": []}]
        m["origin"] = {"kind": "cold_start"}
        return mf.validate_manifest(m)

    def write_manifest(self, scenario: str, press: tuple | None, **extra) -> Path:
        data = {**self.manifest_data(scenario, press), **extra}
        mf.validate_manifest(data)
        path = self.tmp / f"{scenario}.json"
        path.write_text(json.dumps(data))
        return path

    def state_origin_manifest(self, after_frame: int = 4) -> Path:
        """A manifest whose origin is a state with passing restore-check evidence."""
        data = self.manifest_data("stub-from-state", press=self.PRESS)
        script_rel = "tests/manifests/reference/stub-from-state.json"
        (self.root / script_rel).parent.mkdir(parents=True, exist_ok=True)
        mf.write_json(self.root / script_rel, mf.derive_script(data))
        script_sha = reportmod.file_sha256(self.root / script_rel)
        state = self.root / "local" / "states" / "stub.bst"
        state.parent.mkdir(parents=True, exist_ok=True)
        state.write_bytes(b"stub state")
        state_sha = reportmod.file_sha256(state)
        state.with_suffix(".bst.json").write_text(json.dumps(
            {"after_frame": after_frame, "sha256": state_sha, "script_sha256": script_sha, "serialization_method": "Strict"}))
        report = self.root / "local" / "states" / "stub-restore-check.json"
        report.write_text(json.dumps({
            "checks": [{"name": c, "outcome": "passed", "required": True} for c in mf.RESTORE_CHECKS],
            "inputs": {"script": {"sha256": script_sha}},
            "artifacts": [{"kind": "state", "sha256": state_sha}],
            "samples": {"restore_and_continue": {"start_frame": after_frame + 1}}}))
        data["origin"] = {"kind": "state", "path": "local/states/stub.bst", "sha256": state_sha, "script": script_rel,
                          "script_sha256": script_sha, "after_frame": after_frame,
                          "restore_check": {"report": "local/states/stub-restore-check.json", "save_after": after_frame}}
        mf.validate_manifest(data)
        path = self.tmp / "stub-from-state.json"
        path.write_text(json.dumps(data))
        return path

    # ------------------------------------------------------------- harness

    def prepared(self, rep, ns) -> object:
        p = replay.refcmd.Prepared()
        p.core = {"library": str(self.root / "local" / "absent-core.dylib"), "commit": self.core["commit"],
                  "patch_sha256": self.core["patch_sha256"]}
        p.rom = self.root / "local" / "absent-rom.sfc"
        p.rom_sha256 = p.expected_sha256 = self.rom_sha
        p.serialization_method = ns.serialization_method
        rep.add_check("core_available", "passed", detail="stubbed prerequisites; no core is loaded in this test")
        rep.add_check("rom_available", "passed", detail="stubbed prerequisites; no ROM is read in this test")
        return p

    def compare(self, *args: str, stub: StubWorker | None = None, artifacts: str | None = None):
        """Run ``replay compare`` in process; returns (exit code, report)."""
        parser = argparse.ArgumentParser()
        replay.register(parser.add_subparsers(dest="command", required=True))
        art = artifacts or str(self.artifacts / "run")
        report = self.tmp / "report.json"
        ns = parser.parse_args(["replay", "compare", "--root", str(self.root), "--artifacts", art,
                                "--report", str(report), "--task", "M0-05", *args])
        patches = [mock.patch.object(replay.refcmd, "_prepare", self.prepared)]
        if stub is not None:
            patches.append(mock.patch.object(replay.refcmd, "run_bounded", stub))
        with contextlib.ExitStack() as stack:
            for p in patches:
                stack.enter_context(p)
            summary = io.StringIO()
            stack.enter_context(contextlib.redirect_stdout(io.StringIO()))
            stack.enter_context(mock.patch.object(reportmod, "print_summary", lambda rep: PRINT_SUMMARY(rep, summary)))
            code = replay.cmd_compare(ns)
        self.assertIn(f"status={'passed' if code == EXIT_OK else 'failed'}", summary.getvalue())
        return code, json.loads(report.read_text())

    def checks(self, report: dict) -> dict:
        return {c["name"]: c for c in report["checks"]}

    # --------------------------------------------------------------- tests

    def test_identical_stub_runs_are_reported_identical(self) -> None:
        stub = StubWorker(self.rom_sha)
        code, report = self.compare("--manifest", str(self.cold), stub=stub)
        checks = self.checks(report)
        self.assertEqual(code, EXIT_OK, [(c["name"], c["outcome"]) for c in report["checks"]])
        for name in ("fresh_processes", "frame_sets_identical", "fields_identical", "final_state_identical", "av_identical"):
            self.assertEqual(checks[name]["outcome"], "passed", name)
        self.assertTrue(checks["av_identical"]["required"])
        self.assertIsNone(report["divergence"])
        self.assertEqual(report["comparisons"][0]["compared"], self.FRAMES)
        self.assertEqual(len(stub.calls), 2)
        self.assertEqual([s["process"] for s in report["samples"].values()], [9000, 9001])
        self.assertIn("worker pids [9000, 9001]", checks["fresh_processes"]["detail"])
        for call in stub.calls:  # the stubbed worker was given a core and a ROM that do not exist
            self.assertFalse(Path(call["--core"]).exists())
            self.assertFalse(Path(call["--rom"]).exists())

    def test_repeated_or_comparator_pid_fails_the_fresh_process_check(self) -> None:
        for label, pids in (("same worker pid", [9001, 9001]), ("comparator pid", [os.getpid(), 9002]),
                            ("no pid", [None, 9003])):
            with self.subTest(case=label):
                code, report = self.compare("--manifest", str(self.cold), stub=StubWorker(self.rom_sha, pids=pids))
                checks = self.checks(report)
                self.assertEqual(code, EXIT_FAILURE)
                self.assertEqual(checks["fresh_processes"]["outcome"], "failed")
                self.assertEqual(checks["fields_identical"]["outcome"], "passed")

    def test_av_difference_is_required_for_a_cold_start_and_optional_after_a_restore(self) -> None:
        code, report = self.compare("--manifest", str(self.cold), stub=StubWorker(self.rom_sha, av={1: "x"}))
        checks = self.checks(report)
        self.assertEqual(code, EXIT_FAILURE)
        self.assertEqual(checks["av_identical"]["outcome"], "failed")
        self.assertTrue(checks["av_identical"]["required"])
        self.assertEqual(checks["fields_identical"]["outcome"], "passed")
        self.assertEqual(checks["final_state_identical"]["outcome"], "passed")

        state = self.state_origin_manifest()
        code, report = self.compare("--manifest", str(state), stub=StubWorker(self.rom_sha, av={1: "x"}))
        checks = self.checks(report)
        self.assertEqual(code, EXIT_OK, [(c["name"], c["outcome"]) for c in report["checks"]])
        self.assertEqual(checks["av_identical"]["outcome"], "failed")
        self.assertFalse(checks["av_identical"]["required"])
        self.assertIn("informational after a restore", checks["av_identical"]["detail"])
        self.assertEqual(report["samples"]["run1"]["start_frame"], 5)

    def test_divergence_report_records_the_frame_inputs_and_localization(self) -> None:
        stub = StubWorker(self.rom_sha)
        code, report = self.compare("--manifest", str(self.cold), "--against", str(self.removed), stub=stub)
        checks = self.checks(report)
        self.assertEqual(code, EXIT_FAILURE)
        self.assertEqual(checks["fields_identical"]["outcome"], "failed")
        d = report["divergence"]
        self.assertEqual(d["pair"], "left_vs_right")
        self.assertEqual(d["scenario"], {"left": "stub-cold", "right": "stub-start-removed"})
        self.assertEqual(d["frame"], self.PRESS[0])
        self.assertEqual(d["differing_fields"], ["wram_sha256", "registers", "wram_0000_0200"])
        self.assertEqual(d["prior_sample"]["frame"], self.PRESS[0] - 1)
        self.assertEqual(d["inputs"]["left"], {"prior_frame": {"0": [], "1": []}, "at_frame": {"0": ["start"], "1": []}})
        self.assertEqual(d["inputs"]["right"]["at_frame"], {"0": [], "1": []})
        self.assertEqual(d["trace_end_of_run"]["left"]["end_frame"], self.FRAMES - 1)
        loc = d["localization"]
        self.assertEqual(loc["after_frame"], self.PRESS[0])
        self.assertTrue(loc["consistent_with_first_runs"])
        self.assertEqual(loc["wram"]["differing_bytes"], 1)
        self.assertEqual(loc["wram"]["first"], [{"offset": "0x00073", "left": "0x10", "right": "0x00"}])
        self.assertEqual(loc["differing_registers"], [{"register": "a", "left": 0x10, "right": 0}])
        self.assertEqual(checks["divergence_localized"]["outcome"], "passed")
        self.assertFalse(checks["divergence_localized"]["required"])
        written = json.loads((self.artifacts / "run" / "divergence.json").read_text())
        self.assertEqual(written["frame"], self.PRESS[0])
        self.assertIn("divergence_report", [a["kind"] for a in report["artifacts"]])
        # two comparison runs plus one localization re-run per side, each bounded to the frame
        self.assertEqual(len(stub.calls), 4)
        for call in stub.calls[2:]:
            self.assertEqual(call["--stop-after-frame"], str(self.PRESS[0]))
            self.assertIn("--wram-dump-out", call)

    def test_no_localize_skips_the_re_runs(self) -> None:
        stub = StubWorker(self.rom_sha)
        code, report = self.compare("--manifest", str(self.cold), "--against", str(self.removed), "--no-localize", stub=stub)
        self.assertEqual(code, EXIT_FAILURE)
        self.assertEqual(len(stub.calls), 2)
        self.assertEqual(report["divergence"]["localization"], {"after_frame": self.PRESS[0], "skipped": "--no-localize"})
        self.assertNotIn("divergence_localized", self.checks(report))

    def test_a_failed_localization_cannot_change_the_verdict(self) -> None:
        for label, outcomes, expected in (("timeout", ["ok", "ok", "timeout"], "timeout"),
                                          ("crash", ["ok", "ok", "ok", "crash"], "failed")):
            with self.subTest(case=label):
                stub = StubWorker(self.rom_sha, outcomes=outcomes)
                code, report = self.compare("--manifest", str(self.cold), "--against", str(self.removed), stub=stub)
                checks = self.checks(report)
                self.assertEqual(code, EXIT_FAILURE)  # the divergence, not the failed re-run
                self.assertEqual(checks["divergence_localized"]["outcome"], "skipped")
                self.assertFalse(checks["divergence_localized"]["required"])
                self.assertIn("re-run did not complete", checks["divergence_localized"]["detail"])
                failed = checks["localize_left"] if label == "timeout" else checks["localize_right"]
                self.assertEqual(failed["outcome"], expected)
                self.assertFalse(failed["required"])
                self.assertIn("skipped", report["divergence"]["localization"])

    def test_an_inconsistent_localization_is_failed_not_passed(self) -> None:
        """``divergence_localized`` fails when a re-run did complete but did not
        reproduce what it was asked for: its last sample is not the divergence
        frame, or the work RAM dump on disk is not the one it hashed. The verdict
        still comes from the required field comparison (M0-05 review 1, minor 1)."""
        for label, defects, differing in (("a re-run stopped one frame early", {2: "short"}, 0),
                                          ("the dump on disk is not the hashed one", {3: "dump_mismatch"}, 1)):
            with self.subTest(case=label):
                stub = StubWorker(self.rom_sha, localize_defects=defects)
                code, report = self.compare("--manifest", str(self.cold), "--against", str(self.removed), stub=stub,
                                            artifacts=str(self.artifacts / f"inconsistent-{len(defects)}-{differing}"))
                checks = self.checks(report)
                self.assertEqual(code, EXIT_FAILURE)  # the divergence, not the inconsistent re-run
                self.assertEqual(checks["fields_identical"]["outcome"], "failed")
                self.assertEqual(len(stub.calls), 4)  # both re-runs completed
                for name in ("localize_left", "localize_right"):
                    self.assertEqual(checks[name]["outcome"], "passed", name)
                loc = report["divergence"]["localization"]
                self.assertNotIn("skipped", loc)
                self.assertFalse(loc["consistent_with_first_runs"])
                # an empty or unrepresentative work RAM diff must not read as a pass
                self.assertEqual(loc["wram"]["differing_bytes"], differing)
                self.assertEqual(checks["divergence_localized"]["outcome"], "failed")
                self.assertFalse(checks["divergence_localized"]["required"])
                self.assertIn(f"{differing} differing work RAM bytes", checks["divergence_localized"]["detail"])

    def test_worker_outcomes_map_to_the_command_exit_codes(self) -> None:
        for label, outcomes, code in (("timeout", ["timeout"], EXIT_TIMEOUT),
                                      ("missing executable", ["missing"], EXIT_MISSING_PREREQUISITE),
                                      ("crash on the second run", ["ok", "crash"], EXIT_FAILURE),
                                      ("unreadable samples", ["ok", "garbage"], EXIT_FAILURE)):
            with self.subTest(case=label):
                got, report = self.compare("--manifest", str(self.cold),
                                           stub=StubWorker(self.rom_sha, outcomes=outcomes),
                                           artifacts=str(self.artifacts / label.replace(" ", "-")))
                checks = self.checks(report)
                self.assertEqual(got, code)
                self.assertEqual(checks["fields_identical"]["outcome"], "skipped")
                self.assertNotIn("comparisons", report)
                self.assertNotIn("divergence", report)
                self.assertNotIn("fresh_processes", checks)

    def test_a_stale_samples_file_is_not_consumed_after_an_interrupted_run(self) -> None:
        """A killed worker leaves its samples file behind; the re-run must not read it."""
        art = str(self.artifacts / "interrupted")
        code, report = self.compare("--manifest", str(self.cold), stub=StubWorker(self.rom_sha), artifacts=art)
        self.assertEqual(code, EXIT_OK)
        left_behind = Path(art) / "run1-samples.json"
        digest = json.loads(left_behind.read_text())["sample_digest"]
        code, report = self.compare("--manifest", str(self.cold), stub=StubWorker(self.rom_sha, outcomes=["timeout"]), artifacts=art)
        checks = self.checks(report)
        self.assertEqual(code, EXIT_TIMEOUT)
        self.assertEqual(checks["run1"]["outcome"], "timeout")
        self.assertIn("killed after --timeout", checks["run1"]["detail"])
        self.assertEqual(checks["fields_identical"]["outcome"], "skipped")
        self.assertNotIn("comparisons", report)
        self.assertNotIn("run1_samples", [a["kind"] for a in report["artifacts"]])
        self.assertEqual(json.loads(left_behind.read_text())["sample_digest"], digest)  # untouched, and unused

    def test_three_runs_name_every_pair_and_report_the_first_divergence(self) -> None:
        stub = StubWorker(self.rom_sha, divergent=[2])
        code, report = self.compare("--manifest", str(self.cold), "--runs", "3", stub=stub)
        checks = self.checks(report)
        self.assertEqual(code, EXIT_FAILURE)
        self.assertEqual(len(stub.calls), 3 + 2)  # three runs plus the localization pair
        self.assertEqual(checks["fields_identical_run1_vs_run2"]["outcome"], "passed")
        self.assertEqual(checks["fields_identical_run1_vs_run3"]["outcome"], "failed")
        self.assertEqual(checks["fresh_processes"]["outcome"], "passed")
        self.assertEqual([c["pair"] for c in report["comparisons"]], ["run1_vs_run2", "run1_vs_run3"])
        self.assertEqual(report["divergence"]["pair"], "run1_vs_run3")
        self.assertEqual(report["divergence"]["frame"], 2)
        # Observed limit (M0-05): without --against both localization re-runs execute the
        # same manifest, so a divergence between repetitions of one manifest is not
        # reproduced by them and the work RAM diff is empty. The divergence itself still
        # stands: the verdict comes from fields_identical_run1_vs_run3, and
        # divergence_localized is an optional check.
        self.assertEqual(report["divergence"]["localization"]["wram"]["differing_bytes"], 0)
        self.assertFalse(checks["divergence_localized"]["required"])

    def test_expected_digests_are_checked_for_every_run(self) -> None:
        wrong = self.write_manifest("stub-wrong-expected", press=self.PRESS,
                                    expected={"sample_digest": "b" * 64, "final_state_sha256": "c" * 64})
        code, report = self.compare("--manifest", str(wrong), stub=StubWorker(self.rom_sha))
        checks = self.checks(report)
        self.assertEqual(code, EXIT_FAILURE)
        for run in ("run1", "run2"):
            self.assertEqual(checks[f"{run}_sample_digest_matches_expected"]["outcome"], "failed")
            self.assertEqual(checks[f"{run}_final_state_sha256_matches_expected"]["outcome"], "failed")
        self.assertEqual(checks["fields_identical"]["outcome"], "passed")

    def test_without_the_stub_the_worker_cannot_load_a_core(self) -> None:
        """The fixture names a core and a ROM that do not exist, so the real worker
        can only report a missing prerequisite: no stubbed result above came from an
        emulator, and no test in this file needs the ROM."""
        code, report = self.compare("--manifest", str(self.cold), artifacts=str(self.artifacts / "real-worker"))
        checks = self.checks(report)
        self.assertEqual(code, EXIT_MISSING_PREREQUISITE)
        self.assertEqual(checks["run1"]["outcome"], "missing")
        self.assertEqual(checks["fields_identical"]["outcome"], "skipped")
        log = (self.artifacts / "real-worker" / "run1.log").read_text()
        self.assertIn("absent-core.dylib", log)


if __name__ == "__main__":
    unittest.main()
