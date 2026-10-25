"""tools/compact_evidence.py keeps every byte a reader sees and only changes how files are stored (D-0013)."""
import hashlib
import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
TOOL = ROOT / "tools" / "compact_evidence.py"
COMPRESSED_FLAG = 0x20


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


@unittest.skipUnless(sys.platform == "darwin", "APFS clones and decmpfs compression are macOS only")
class CompactEvidenceTests(unittest.TestCase):
    def run_tool(self, *args: str) -> subprocess.CompletedProcess:
        return subprocess.run([sys.executable, str(TOOL), *args, "--skip-recent", "0", "--min-size", "4096"],
                              capture_output=True, text=True, timeout=300)

    def test_compresses_and_clones_without_changing_bytes(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "a").mkdir()
            (root / "b").mkdir()
            series = bytes(range(256)) * 8192 + b"\x00" * (1 << 21)  # repetitive, like a WRAM series
            for name in ("a/series.bin", "b/series.bin"):
                (root / name).write_bytes(series)
            (root / "a/frame.png").write_bytes(os.urandom(8192))
            before = {p: digest(p) for p in root.rglob("*") if p.is_file()}
            r = self.run_tool(str(root))
            self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
            self.assertIn("status=passed", r.stdout)
            self.assertEqual({p: digest(p) for p in root.rglob("*") if p.is_file()}, before)
            for name in ("a/series.bin", "b/series.bin"):
                self.assertTrue(os.lstat(root / name).st_flags & COMPRESSED_FLAG, name)
            self.assertFalse(os.lstat(root / "a/frame.png").st_flags & COMPRESSED_FLAG, "PNGs are left alone")
            self.assertIn("identical copies: 1", r.stdout)

    def test_dry_run_changes_nothing(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "series.bin"
            path.write_bytes(b"\x00" * (1 << 20))
            r = self.run_tool(tmp, "--dry-run")
            self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
            self.assertFalse(os.lstat(path).st_flags & COMPRESSED_FLAG)

    def test_recent_files_are_skipped(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "series.bin"
            path.write_bytes(b"\x00" * (1 << 20))
            r = subprocess.run([sys.executable, str(TOOL), tmp, "--min-size", "4096"],
                               capture_output=True, text=True, timeout=300)
            self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
            self.assertIn("0 files", r.stdout)
            self.assertFalse(os.lstat(path).st_flags & COMPRESSED_FLAG)


if __name__ == "__main__":
    unittest.main()
