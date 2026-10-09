"""The reference core is accepted by its source and verified outputs, not one binary hash
(PORTABLE-CORE-IDENTITY). None of these need the ROM or a built core."""

from __future__ import annotations

import hashlib
import json
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))

from tools.unirally_lab.content import pack  # noqa: E402
from tools.unirally_lab.native import core_check  # noqa: E402
from tools.unirally_lab.reference import core_identity  # noqa: E402

LOCK = json.loads((ROOT / "tools/locks/emulators.json").read_text())
ENTRY = LOCK["cores"]["bsnes"]


class CoreIdentityTest(unittest.TestCase):
    def setUp(self) -> None:
        self.tmp = Path(tempfile.mkdtemp())
        self.addCleanup(lambda: __import__("shutil").rmtree(self.tmp))
        lock = dict(LOCK, install_dir="emulators")
        self.lock = self.tmp / "emulators.json"
        self.lock.write_text(json.dumps(lock))
        self.library = self.tmp / "emulators/bsnes/bsnes/out/bsnes_libretro.so"
        self.library.parent.mkdir(parents=True)
        self.library.write_bytes(b"a library built from the lock's source")
        self.verified = self.tmp / "verified-cores.json"
        self.write_verified([])

    def write_verified(self, libraries, **overrides) -> None:
        document = dict(schema_version=1, core="bsnes", commit=ENTRY["commit"], patch_sha256=ENTRY["patch_sha256"],
                        libraries=libraries)
        document.update(overrides)
        self.verified.write_text(json.dumps(document))

    def write_build_record(self, **overrides) -> None:
        record = dict(schema_version=1, core="bsnes", commit=ENTRY["commit"], patch_sha256=ENTRY["patch_sha256"],
                      library=str(self.library), library_sha256=hashlib.sha256(self.library.read_bytes()).hexdigest())
        record.update(overrides)
        (self.tmp / "emulators/bsnes/lab-core.json").write_text(json.dumps(record))

    def require(self) -> str:
        return core_identity.require_core(self.library, root=self.tmp, lock_path=self.lock, verified_path=self.verified)

    def accepted(self, sha: str) -> bool:
        return core_identity.accepted_reference(sha, root=self.tmp, lock_path=self.lock, verified_path=self.verified)

    def test_a_library_built_from_the_lock_is_accepted_whatever_its_hash(self) -> None:
        self.write_build_record()
        sha = self.require()
        self.assertEqual(sha, hashlib.sha256(self.library.read_bytes()).hexdigest())
        self.assertTrue(self.accepted(sha))

    def test_another_commit_or_patch_is_refused_naming_the_lock(self) -> None:
        for field, value in (("commit", "0" * 40), ("patch_sha256", "f" * 64)):
            with self.subTest(field=field):
                self.write_build_record(**{field: value})
                with self.assertRaisesRegex(core_identity.CoreIdentityError, "not built from the lock's bsnes commit"):
                    self.require()
                self.assertFalse(self.accepted(hashlib.sha256(self.library.read_bytes()).hexdigest()))

    def test_a_library_changed_after_its_build_is_refused(self) -> None:
        self.write_build_record()
        self.library.write_bytes(b"replaced after the build")
        with self.assertRaises(core_identity.CoreIdentityError):
            self.require()

    def test_no_build_record_refuses_an_unverified_library(self) -> None:
        with self.assertRaises(core_identity.CoreIdentityError):
            self.require()

    def test_a_verified_library_is_accepted_without_a_build_record(self) -> None:
        sha = hashlib.sha256(self.library.read_bytes()).hexdigest()
        self.write_verified([dict(sha256=sha, platform="test")])
        self.assertEqual(self.require(), sha)
        self.assertTrue(self.accepted(sha))
        self.assertFalse(self.accepted("0" * 64))

    def test_a_verified_list_for_another_source_is_refused(self) -> None:
        self.write_verified([], commit="0" * 40)
        with self.assertRaisesRegex(core_identity.CoreIdentityError, "does not name the lock"):
            self.require()

    def test_the_tracked_verified_list_names_the_tracked_lock(self) -> None:
        libraries = core_identity.verified_cores()
        self.assertIn("e59bf88d4fc922c9fe3b5438e65ff3a6909d24e1628f0f87141c8de17699a91b", libraries)
        for sha, item in libraries.items():
            self.assertRegex(sha, "^[0-9a-f]{64}$")
            self.assertTrue(item.get("platform") and item.get("checked_by"), sha)


class CoreCheckTest(unittest.TestCase):
    """core_check compares only against a capture another, verified library made."""

    def capture(self, core_sha256: str) -> Path:
        directory = Path(tempfile.mkdtemp())
        self.addCleanup(lambda: __import__("shutil").rmtree(directory))
        (directory / "reference.json").write_text(json.dumps(dict(
            kind="track_breadth_original", rom_sha256=core_check.ROM_SHA, core_sha256=core_sha256)))
        return directory

    def test_a_capture_from_an_unverified_library_is_refused(self) -> None:
        library = self.capture("0" * 64) / "reference.json"
        with self.assertRaisesRegex(ValueError, "not verified"):
            core_check.check(library, self.capture("1" * 64))

    def test_a_library_cannot_verify_itself(self) -> None:
        verified = next(iter(core_identity.verified_cores()))
        library = Path(tempfile.mkdtemp()) / "lib"
        self.addCleanup(lambda: __import__("shutil").rmtree(library.parent))
        library.write_bytes(b"x")
        with mock.patch.object(core_check, "sha256_file", return_value=verified):
            with self.assertRaisesRegex(ValueError, "library under test"):
                core_check.check(library, self.capture(verified))


class OutputPinTest(unittest.TestCase):
    def test_an_extraction_of_other_bytes_is_refused_by_the_rules_pin(self) -> None:
        rules, rules_sha = pack.load_rules(ROOT / "tests/manifests/content/classic-crawler-tracks-pack.json")
        entry = next(e for e in rules["entries"] if e["source"]["kind"] == "pre_race_matrix")
        single = dict(rules, entries=[entry])
        rom = b"\0" * rules["source_rom"]["size"]
        with mock.patch.object(pack, "sha256", side_effect=lambda data: rules["source_rom"]["sha256"]
                               if data is rom else hashlib.sha256(data).hexdigest()), \
                mock.patch.object(pack, "decode_entry", return_value=b"\1" * entry["size"]):
            with self.assertRaisesRegex(ValueError, "extracted entry identity differs: zoom.landing-response-matrices"):
                pack.build_pack(rom, single, rules_sha)


if __name__ == "__main__":
    unittest.main()
