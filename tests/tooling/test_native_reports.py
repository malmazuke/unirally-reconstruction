"""Prove the test command reports native failures and timeouts as such.

Needs the isolated toolchain (bootstrap). Without it the checks are skipped,
and a skip is reported as a skip, never as a pass.
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

from unirally_lab import EXIT_FAILURE, EXIT_OK, EXIT_TIMEOUT  # noqa: E402
from unirally_lab import toolchain  # noqa: E402

PROJECT = ROOT / "tools" / "project.py"
MANIFEST = ROOT / "local" / "toolchain" / "manifest.json"


def run_cli(*args: str) -> subprocess.CompletedProcess:
    return subprocess.run([sys.executable, str(PROJECT), *args], capture_output=True, text=True, timeout=900)


class PythonRunnerTests(unittest.TestCase):
    """The Python runner must surface every failing test, including subtests,
    and the test command must not report a run the runner counted as failed as
    a pass (M0-04 review 1, finding M2: subtest errors vanished and the
    runner's exit status was ignored, so a failing test read as 101/101)."""

    PROBE = (
        "import unittest\n"
        "class T(unittest.TestCase):\n"
        "    def test_sub(self):\n"
        "        for i in (1, 2, 3):\n"
        "            with self.subTest(i=i):\n"
        "                self.assertEqual(i, 1)\n"
        "    def test_sub_error(self):\n"
        "        with self.subTest(k=1):\n"
        "            raise RuntimeError('boom')\n"
        "    def test_ok(self):\n"
        "        pass\n"
        "    def test_plain_failure(self):\n"
        "        self.fail('plain')\n"
        "    @unittest.skip('why')\n"
        "    def test_skipped(self):\n"
        "        pass\n"
    )

    def test_subtest_failures_are_recorded(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            (Path(tmp) / "test_probe.py").write_text(self.PROBE)
            r = subprocess.run([sys.executable, str(ROOT / "tools" / "unirally_lab" / "pytests.py"), tmp], capture_output=True, text=True, timeout=120)
            self.assertEqual(r.returncode, 1, r.stderr)
            summary = json.loads(r.stdout.strip().splitlines()[-1])
            self.assertEqual(summary["tests_run"], 5)
            self.assertFalse(summary["successful"])
            outcomes = {rec["name"].split(".")[-1]: rec["outcome"] for rec in summary["records"]}
            self.assertEqual(outcomes["test_ok"], "passed")
            self.assertEqual(outcomes["test_plain_failure"], "failed")
            self.assertEqual(outcomes["test_skipped"], "skipped")
            self.assertEqual(outcomes["test_sub (i=2)"], "failed")
            self.assertEqual(outcomes["test_sub (i=3)"], "failed")
            self.assertEqual(outcomes["test_sub_error (k=1)"], "failed")
            self.assertNotIn("test_sub", outcomes)  # no success record for a test whose subtests failed
            self.assertIn("2 != 1", next(rec["detail"] for rec in summary["records"] if rec["name"].endswith("(i=2)")))

    def test_all_passing_subtests_record_one_success(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            (Path(tmp) / "test_probe.py").write_text(
                "import unittest\nclass T(unittest.TestCase):\n    def test_sub(self):\n        for i in (1, 2):\n            with self.subTest(i=i):\n                self.assertTrue(i)\n")
            r = subprocess.run([sys.executable, str(ROOT / "tools" / "unirally_lab" / "pytests.py"), tmp], capture_output=True, text=True, timeout=120)
            self.assertEqual(r.returncode, 0, r.stderr)
            summary = json.loads(r.stdout.strip().splitlines()[-1])
            self.assertEqual([(rec["name"].split(".")[-1], rec["outcome"]) for rec in summary["records"]], [("test_sub", "passed")])


@unittest.skipUnless(toolchain.load_manifest(MANIFEST), "isolated toolchain not bootstrapped")
class NativeReportTests(unittest.TestCase):
    def test_failure_and_timeout_are_distinguished(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            report = Path(tmp) / "build.json"
            r = run_cli("build", "--preset", "lab-failure-probe", "--report", str(report))
            self.assertEqual(r.returncode, EXIT_OK, r.stderr[-2000:])
            report = Path(tmp) / "test.json"
            r = run_cli("test", "--suite", "synthetic", "--preset", "lab-failure-probe", "--no-python-tests",
                        "--artifacts", str(Path(tmp) / "art"), "--report", str(report), "--test-timeout", "30")
            self.assertEqual(r.returncode, EXIT_TIMEOUT, r.stderr[-2000:])
            rep = json.loads(report.read_text())
            self.assertEqual(rep["status"], "failed")
            outcomes = {c["name"]: c["outcome"] for c in rep["checks"]}
            self.assertEqual(outcomes["ctest:lab_probe_expected_failure"], "failed")
            self.assertEqual(outcomes["ctest:lab_probe_expected_timeout"], "timeout")
            self.assertEqual(outcomes["ctest:lab_state_known_hash"], "passed")
            self.assertEqual(outcomes["python_tooling_tests"], "skipped")
            self.assertEqual(outcomes["native_fresh_process_repeatability"], "passed")
            self.assertEqual(rep["summary"]["failed"], 1)
            self.assertEqual(rep["summary"]["timeout"], 1)
            junit = [a for a in rep["artifacts"] if a["kind"] == "ctest_junit"]
            self.assertTrue(junit and Path(junit[0]["path"]).is_file(), "junit artifact must be recorded")

    def test_results_follow_the_current_source_not_stale_binaries(self) -> None:
        """A changed expectation must fail even when the caller runs test without build.

        The expectation is injected through a cache variable so no tracked file
        is touched; the probe build directory is restored afterwards.
        """
        with tempfile.TemporaryDirectory() as tmp:
            r = run_cli("build", "--preset", "lab-failure-probe", "--define", "LAB_KNOWN_HASH_OVERRIDE=0x0be347c529fadda8ull")
            self.assertEqual(r.returncode, EXIT_OK, r.stderr[-2000:])
            try:
                report = Path(tmp) / "test.json"
                r = run_cli("test", "--suite", "synthetic", "--preset", "lab-failure-probe", "--no-python-tests",
                            "--artifacts", str(Path(tmp) / "art"), "--report", str(report))
            finally:
                self.assertEqual(run_cli("build", "--preset", "lab-failure-probe", "--define", "LAB_KNOWN_HASH_OVERRIDE=").returncode, EXIT_OK)
            self.assertNotEqual(r.returncode, EXIT_OK)
            rep = json.loads(report.read_text())
            outcomes = {c["name"]: c["outcome"] for c in rep["checks"]}
            self.assertEqual(outcomes["ctest:lab_state_known_hash"], "failed")
            self.assertEqual(outcomes["native_fresh_process_repeatability"], "passed")
            self.assertEqual(rep["build"]["source"]["commit"], rep["source"]["commit"])
            self.assertEqual(rep["build"]["source"]["dirty_diff_sha256"], rep["source"]["dirty_diff_sha256"])

    def test_failed_build_is_not_reported_as_available(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            r = run_cli("build", "--preset", "lab-failure-probe", "--define", "LAB_PROBE_COMPILE_ERROR=ON",
                        "--report", str(Path(tmp) / "build.json"))
            self.assertEqual(r.returncode, EXIT_FAILURE, r.stderr[-2000:])
            build_rep = json.loads((Path(tmp) / "build.json").read_text())
            self.assertEqual(build_rep["build"]["outcome"], "failed")
            try:
                report = Path(tmp) / "test.json"
                r = run_cli("test", "--suite", "synthetic", "--preset", "lab-failure-probe", "--no-python-tests",
                            "--artifacts", str(Path(tmp) / "art"), "--report", str(report))
            finally:
                self.assertEqual(run_cli("build", "--preset", "lab-failure-probe", "--define", "LAB_PROBE_COMPILE_ERROR=OFF").returncode, EXIT_OK)
            self.assertNotEqual(r.returncode, EXIT_OK)
            rep = json.loads(report.read_text())
            outcomes = {c["name"]: c["outcome"] for c in rep["checks"]}
            self.assertEqual(outcomes["native_build_available"], "failed")
            self.assertNotIn("ctest:lab_state_known_hash", outcomes)
            self.assertEqual(rep["build"]["outcome"], "failed")

    def test_preset_cache_change_is_applied_without_explicit_build(self) -> None:
        """Reconfigure inside test must pick up a cache variable set at configure time."""
        with tempfile.TemporaryDirectory() as tmp:
            self.assertEqual(run_cli("build", "--preset", "lab-failure-probe").returncode, EXIT_OK)
            report = Path(tmp) / "test.json"
            r = run_cli("test", "--suite", "synthetic", "--preset", "lab-failure-probe", "--no-python-tests",
                        "--artifacts", str(Path(tmp) / "art"), "--report", str(report), "--test-timeout", "30")
            self.assertEqual(r.returncode, EXIT_TIMEOUT)
            rep = json.loads(report.read_text())
            check = next(c for c in rep["checks"] if c["name"] == "native_build_available")
            self.assertIn("reconfigure", check["detail"])
            self.assertIn("ctest:lab_probe_expected_timeout", {c["name"] for c in rep["checks"]})

    def test_nested_run_with_python_tests_is_refused(self) -> None:
        import os
        env = {**os.environ, "UNIRALLY_LAB_NESTED_TEST": "1"}
        r = subprocess.run([sys.executable, str(PROJECT), "test", "--suite", "synthetic"],
                           capture_output=True, text=True, timeout=120, env=env)
        self.assertEqual(r.returncode, 3)
        self.assertIn("recursive invocation", r.stderr)


if __name__ == "__main__":
    unittest.main()
