"""Stored captures read the same raw or zstd-compressed (tools/unirally_lab/capture_files.py, EVIDENCE-LARGE-FILES).

The project counts a skipped test as a failure, so the `.zst` tests are defined only where a zstd
decoder exists (Python 3.14's `compression.zstd` or the `zstd` command).
"""
import importlib.util
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))  # the suite may load this module first
from tools.unirally_lab.capture_files import open_capture, read_capture, stored_path  # noqa: E402

DATA = bytes(range(256)) * 4096 + b"\x00" * 65536
HAVE_ZSTD = importlib.util.find_spec("compression.zstd") is not None if sys.version_info >= (3, 14) else False
HAVE_ZSTD_CLI = shutil.which("zstd") is not None


def write_zst(path: Path, data: bytes) -> None:
    if HAVE_ZSTD:
        from compression import zstd
        path.write_bytes(zstd.compress(data))
    else:
        path.write_bytes(subprocess.run(["zstd", "-q", "-c"], input=data, check=True, capture_output=True).stdout)


class RawCaptureTests(unittest.TestCase):
    def test_raw_file_reads_and_streams(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "memory.wram"
            path.write_bytes(DATA)
            self.assertEqual(read_capture(path), DATA)
            with open_capture(path) as stream:
                self.assertEqual(stream.read(131072), DATA[:131072])
            self.assertEqual(stored_path(path), path)

    def test_missing_capture_names_both_forms(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            with self.assertRaises(FileNotFoundError) as caught:
                read_capture(Path(tmp) / "series.bin")
            self.assertIn("series.bin.zst", str(caught.exception))


if HAVE_ZSTD or HAVE_ZSTD_CLI:
    class CompressedCaptureTests(unittest.TestCase):
        def test_zst_reads_the_same_bytes_in_chunks(self) -> None:
            with tempfile.TemporaryDirectory() as tmp:
                path = Path(tmp) / "memory.wram"
                write_zst(path.with_name("memory.wram.zst"), DATA)
                self.assertEqual(stored_path(path).name, "memory.wram.zst")
                self.assertEqual(read_capture(path), DATA)
                with open_capture(path) as stream:
                    chunks = []
                    while chunk := stream.read(131072):
                        chunks.append(chunk)
                self.assertEqual(b"".join(chunks), DATA)

        def test_raw_wins_over_zst(self) -> None:
            with tempfile.TemporaryDirectory() as tmp:
                path = Path(tmp) / "series.bin"
                path.write_bytes(DATA)
                write_zst(path.with_name("series.bin.zst"), b"other")
                self.assertEqual(read_capture(path), DATA)


if __name__ == "__main__":
    unittest.main()
