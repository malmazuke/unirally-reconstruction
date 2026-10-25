"""tools/compact_evidence.py keeps every byte a reader sees and only changes how files are stored (D-0013).

The project counts a skipped test as a failure, so the macOS tests are defined only on macOS and the
refusal test only elsewhere.
"""
import hashlib
import importlib.util
import os
import stat
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
TOOL = ROOT / "tools" / "compact_evidence.py"
COMPRESSED_FLAG = 0x20
OLD = 1_700_000_000  # an mtime well outside the --skip-recent window
SERIES = bytes(range(256)) * 8192 + b"\x00" * (1 << 21)  # repetitive, like a WRAM series


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run_tool(*args: str, cwd: Path = ROOT) -> subprocess.CompletedProcess:
    return subprocess.run([sys.executable, str(TOOL), *args, "--min-size", "4096", "--min-free", "0"],
                          capture_output=True, text=True, timeout=300, cwd=cwd)


def write_old(path: Path, data: bytes) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)
    os.utime(path, (OLD, OLD))


def compressed(path: Path) -> bool:
    return bool(os.lstat(path).st_flags & COMPRESSED_FLAG)


if sys.platform == "darwin":
    class CompactEvidenceTests(unittest.TestCase):
        def setUp(self) -> None:
            self.tmp = tempfile.TemporaryDirectory()
            self.root = Path(self.tmp.name)

        def tearDown(self) -> None:
            self.tmp.cleanup()

        def snapshot(self) -> dict:
            return {p: digest(p) for p in self.root.rglob("*") if p.is_file()}

        def test_compresses_and_clones_without_changing_bytes(self) -> None:
            for name in ("a/series.bin", "b/series.bin"):
                write_old(self.root / name, SERIES)
            write_old(self.root / "a/frame.png", os.urandom(8192))
            before = self.snapshot()
            r = run_tool(str(self.root))
            self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
            self.assertIn("status=passed", r.stdout)
            self.assertEqual(self.snapshot(), before)
            self.assertTrue(compressed(self.root / "a/series.bin") and compressed(self.root / "b/series.bin"))
            self.assertFalse(compressed(self.root / "a/frame.png"), "PNGs are left alone")
            self.assertIn("identical copies: 1", r.stdout)

        def test_pairs_follow_path_order_not_directory_order(self) -> None:
            # Review round 1: `z` listed before `m` made `z` the source of a clone onto itself.
            for name in ("z/series.bin", "m/series.bin"):
                write_old(self.root / name, SERIES)
            r = run_tool(str(self.root))
            self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
            for name in ("z/series.bin", "m/series.bin"):
                self.assertTrue(compressed(self.root / name), name)
                self.assertLess(os.lstat(self.root / name).st_blocks * 512, len(SERIES) // 4, name)

        def test_interrupted_scratch_is_removed_and_never_counted(self) -> None:
            write_old(self.root / "a/series.bin", SERIES)
            write_old(self.root / ".compact-scratch/123-456/series.bin", SERIES)
            r = run_tool(str(self.root))
            self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
            self.assertIn("identical copies: 0", r.stdout)
            self.assertFalse((self.root / ".compact-scratch").exists())

        def test_clone_keeps_its_own_metadata_and_skips_a_different_mode(self) -> None:
            for name in ("a/series.bin", "b/series.bin", "c/series.bin"):
                write_old(self.root / name, SERIES)
            os.utime(self.root / "b/series.bin", (OLD + 99, OLD + 99))
            os.chmod(self.root / "c/series.bin", 0o600)
            r = run_tool(str(self.root))
            self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
            self.assertEqual(int(self.root.joinpath("b/series.bin").stat().st_mtime), OLD + 99)
            self.assertEqual(stat.S_IMODE(self.root.joinpath("c/series.bin").stat().st_mode), 0o600)
            self.assertIn("different mode or attributes: 1", r.stdout)

        def test_a_changed_original_is_not_replaced(self) -> None:
            spec = importlib.util.spec_from_file_location("compact_evidence", TOOL)
            tool = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(tool)
            path = self.root / "series.bin"
            write_old(path, SERIES)
            seen = tool.identity(path.lstat())
            replacement = self.root / "replacement.bin"
            replacement.write_bytes(SERIES)
            with path.open("ab") as stream:  # a writer appends after the file was read
                stream.write(b"more")
            with self.assertRaises(RuntimeError):
                tool.Compactor(self.root, 0).swap(path, replacement, digest(replacement), seen)
            self.assertTrue(path.read_bytes().endswith(b"more"))

        def test_a_second_run_does_no_work(self) -> None:
            write_old(self.root / "a/series.bin", SERIES)
            self.assertEqual(run_tool(str(self.root)).returncode, 0)
            r = run_tool(str(self.root))
            self.assertIn("to compress: 0", r.stdout)

        def test_runs_from_any_directory_and_recent_files_wait(self) -> None:
            path = self.root / "series.bin"
            path.write_bytes(SERIES)  # just written: inside the default 60-minute window
            r = subprocess.run([sys.executable, str(TOOL), str(self.root), "--min-size", "4096"],
                               capture_output=True, text=True, timeout=300, cwd=tempfile.gettempdir())
            self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
            self.assertIn("0 files", r.stdout)
            self.assertFalse(compressed(path))

        def test_large_captures_are_stored_as_zst(self) -> None:
            from tools.unirally_lab.capture_files import read_capture
            for name in ("a/series.bin", "b/series.bin", "c/memory.wram"):
                write_old(self.root / name, SERIES)
            write_old(self.root / "c/other.bin", SERIES)  # not a capture kind: compressed in place
            r = run_tool(str(self.root), "--max-compress", "100000")
            self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
            for name in ("a/series.bin", "b/series.bin", "c/memory.wram"):
                self.assertFalse((self.root / name).exists(), name)
                self.assertTrue((self.root / (name + ".zst")).exists(), name)
                self.assertEqual(read_capture(self.root / name), SERIES)
            self.assertTrue((self.root / "c/other.bin").exists())
            self.assertIn("stored as .zst: 3", r.stdout)
            self.assertIn("cloned:", r.stdout)

        def test_dry_run_changes_nothing(self) -> None:
            write_old(self.root / "series.bin", SERIES)
            r = run_tool(str(self.root), "--dry-run")
            self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
            self.assertFalse(compressed(self.root / "series.bin"))
else:
    class CompactEvidenceRefusalTests(unittest.TestCase):
        def test_refuses_outside_macos(self) -> None:
            with tempfile.TemporaryDirectory() as tmp:
                r = run_tool(tmp)
                self.assertNotEqual(r.returncode, 0)
                self.assertIn("macOS only", r.stderr)


if __name__ == "__main__":
    unittest.main()
