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


class StuntEventTests(unittest.TestCase):
    """STUNT-EVENT-RACE (R-0066): race mode 2 compares on each stunt track's own scenario, its
    rows end once the result is stable (105 loads), and its state appends the stunt words."""

    def test_mode_two_is_compared_on_the_track_itself(self) -> None:
        for per_track in (False, True):
            self.assertEqual(track_reference.native_scenario(2, 2, per_track), "classic.track.02")
            self.assertEqual(track_reference.native_scenario(42, 2, per_track), "classic.track.42")
        self.assertEqual(track_reference.native_scenario(13, 0, False), "classic.crawler.dragster")
        self.assertEqual(track_reference.native_scenario(13, 0, True), "classic.track.13")
        self.assertEqual(track_reference.native_scenario(1, 1, True), "classic.crawler.zoom-zoo")
        self.assertIsNone(track_reference.native_scenario(2, 3, True))
        self.assertEqual(track_reference.STABLE_RESULT[2], dict(player_won=105, player_lost=105))
        # The stunt event's own constants replace the ZOOM ZOO guard values of these words.
        self.assertEqual(track_reference.STUNT_GUARDS, {0xc6d: 0, 0x1275: 0, 0xd15: 1})

    def test_stunt_words_follow_the_native_order(self) -> None:
        from unirally_lab.native.classic_race_layout import LAYOUT, describe, stunt_event_bytes
        wram, sram = bytearray(0x20000), bytearray(0x2000)
        sram[0x753:0x755] = (68).to_bytes(2, "little")      # the qualifying score
        wram[0xbf5] = 1                                      # the clock's stop
        wram[0xfe9] = 1                                      # the finish display
        wram[0x12df], wram[0x12e1] = 1, 1                    # both riders settled
        sram[0x79b], sram[0x79d] = 2, 6                      # z flip x1: 2 shown, 6 points
        sram[0x7ab + 4 + 2] = 12                             # mega x2 (tabletop): 12 points
        words = stunt_event_bytes(bytes(wram), bytes(sram))
        self.assertEqual(len(words), 90)
        row = bytes(916) + words
        named = {name: (offset, width) for offset, width, name in LAYOUT}
        def value(name):
            offset, width = named[name]
            return int.from_bytes(row[offset:offset+width], "little")
        self.assertEqual(value("stunt.qualifying_score"), 68)
        self.assertEqual(value("stunt.clock_stopped"), 1)
        self.assertEqual(value("stunt.finish_display"), 1)
        self.assertEqual((value("player.stunt.settled"), value("opponent.stunt.settled")), (1, 1))
        self.assertEqual((value("stunt.z_flip.x1.shown"), value("stunt.z_flip.x1.points")), (2, 6))
        self.assertEqual(value("stunt.mega.x2.points"), 12)
        # The layout covers the stunt words once each, up to the state's 1,006 bytes.
        stunt = sorted((o, w) for o, w, n in LAYOUT if o >= 916)
        self.assertEqual(stunt[0][0], 916)
        self.assertEqual(sum(w for _, w in stunt), 90)
        self.assertTrue(all(a[0] + a[1] == b[0] for a, b in zip(stunt, stunt[1:])))
        other = bytearray(row)
        other[named["stunt.roll.x4.points"][0]] = 32
        self.assertEqual(describe(bytes(other), row), [("stunt.roll.x4.points", 32, 0)])


if __name__ == "__main__":
    unittest.main()
