"""ROM-free tests of the TRACK-BREADTH reference menu path: the inputs for ZOOM ZOO equal the
accepted M4-16 menu prefix, and every other path only adds Downs and shifts the later presses."""

from __future__ import annotations

import json
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

from unirally_lab.native import track_reference  # noqa: E402

ZOOM_ZOO_MANIFEST = ROOT / "tests" / "manifests" / "replay" / "race-crawler-zoom-zoo-3300.json"


class MenuPathTests(unittest.TestCase):
    def test_zoom_zoo_path_is_the_accepted_prefix(self) -> None:
        manifest = json.loads(ZOOM_ZOO_MANIFEST.read_text(encoding="utf-8"))
        accepted = [(e["from"], e["to"], e["buttons"][0]) for e in manifest["inputs"]["controllers"][0]["events"]
                    if e["to"] <= 1205]
        self.assertEqual(track_reference.menu_events(1), accepted)

    def test_positions_add_downs_and_keep_order(self) -> None:
        for tour_row in range(4):
            for position in range(5):
                events = track_reference.menu_events(position, tour_row)
                downs = [e for e in events if e[2] == "down"]
                self.assertEqual(len(downs), position + tour_row)
                self.assertEqual([e[2] for e in events].count("start"), 6)
                # Presses never overlap and are released for at least one frame between them.
                ordered = sorted(events)
                for (a0, a1, _), (b0, _, _) in zip(ordered, ordered[1:]):
                    self.assertLess(a1 + 1, b0)

    def test_timeline_releases_the_controller_after_the_menu(self) -> None:
        rows = track_reference.timeline(3, 2000, 2)
        last = max(e[1] for e in track_reference.menu_events(3, 2))
        self.assertTrue(all(row == [[], []] for row in rows[last + 1:]))
        self.assertEqual(rows[track_reference.FIRST_TOUR_DOWN][0], ["down"])

    def test_rejects_positions_outside_the_screens(self) -> None:
        for position, tour_row in ((5, 0), (-1, 0), (0, 4)):
            with self.assertRaises(ValueError):
                track_reference.menu_events(position, tour_row)


if __name__ == "__main__":
    unittest.main()
