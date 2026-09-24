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

    def test_hold_segments_follow_one_another(self) -> None:
        # One (frame, buttons) pair holds to the horizon; several are held in turn, and a
        # segment without buttons releases the controller (RACE-FINISH-BREADTH).
        single = track_reference.timeline(3, 2000, 0, (1500, ["right"]))
        self.assertEqual(single[1499][0], [])
        self.assertTrue(all(row[0] == ["right"] for row in single[1500:]))
        rows = track_reference.timeline(3, 2000, 0, [(1700, ["left"]), (1500, ["right", "b"]), (1900, [])])
        self.assertEqual(rows[1500][0], ["b", "right"])
        self.assertEqual(rows[1699][0], ["b", "right"])
        self.assertEqual(rows[1700][0], ["left"])
        self.assertTrue(all(row[0] == [] for row in rows[1900:]))
        with self.assertRaises(ValueError):
            track_reference.timeline(3, 2000, 0, (100, ["right"]))  # inside the menu

    def test_rejects_positions_outside_the_screens(self) -> None:
        for position, tour_row in ((5, 0), (-1, 0), (0, 5)):
            with self.assertRaises(ValueError):
                track_reference.menu_events(position, tour_row)
        for tour_row, tour_column in ((4, 1), (0, 2)):
            with self.assertRaises(ValueError):
                track_reference.menu_events(0, tour_row, tour_column)

    def test_locked_tours_take_right_for_the_second_column(self) -> None:
        # LOCKED-TOURS: JUMPER, BOUNDER, RUNNER and SPRINTER sit right of CRAWLER, SHUFFLER,
        # WALKER and HOPPER; HUNTER is a fifth row below HOPPER.
        events = track_reference.menu_events(2, 3, 1)
        tour = [e[2] for e in sorted(events) if e[2] in ("down", "right") and e[0] < track_reference.FIRST_DOWN]
        self.assertEqual(tour, ["down", "down", "down", "right"])
        self.assertEqual([e[2] for e in events].count("start"), 6)


if __name__ == "__main__":
    unittest.main()
