"""Conditional semantic transport timing, R-0075. No cold audio claim."""
import sys
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'tools/unirally_lab/reference/audio_lab'))
from transport_clock import (PalBusClock, begin_receiver_poll, send_ready_command,
                             MenuPaletteClockState, run_menu_nmi, MenuPollInterrupt)


class TransportClockTests(unittest.TestCase):
    def test_refresh_occurs_once_when_a_bus_step_reaches_the_deadline(self):
        clock = PalBusClock(530)
        clock.step(8)
        self.assertEqual(clock.ticks, 578)
        clock.step(4)
        self.assertEqual(clock.ticks, 582)

    def test_refresh_deadline_changes_with_scanline_cpu_divider(self):
        clock = PalBusClock(1364 + 526)
        clock.step(8)
        self.assertEqual(clock.ticks, 1364 + 574)

    def test_drained_phase_does_not_insert_refresh_twice(self):
        clock = PalBusClock(580)
        clock.step(8)
        self.assertEqual(clock.ticks, 588)

    def test_observed_navigation_transfers_match_with_and_without_refresh(self):
        # Entry-clock inputs intentionally remain observations. These retained
        # original transfers test bus timing, not a cold/native producer gate.
        for start, expected in (
            (191399214, (191399938, 191399968, 191399974)),
            (190975764, (190976448, 190976478, 190976484)),
            (191399852, (191400536, 191400566, 191400572)),
        ):
            with self.subTest(start=start):
                clock = PalBusClock(start)
                begin_receiver_poll(clock)
                self.assertEqual(send_ready_command(clock), expected)

    def test_odd_or_arbitrary_steps_are_rejected(self):
        for clocks in (0, 1, 3, 13, 40):
            with self.assertRaises(ValueError): PalBusClock(0).step(clocks)


    def test_menu_nmi_matches_palette_no_wrap_boundaries(self):
        clock = PalBusClock(180322242)
        result = run_menu_nmi(clock, MenuPaletteClockState(1, 0))
        self.assertEqual(result, {
            'logo': 180322618, 'palette': 180322858,
            'palette_leave': 180322940, 'restore': 180322972,
            'rti': 180323188, 'resumed': 180323240,
        })

    def test_nmi_arriving_in_the_callers_last_stack_write_is_not_missed(self):
        start = 119465950
        clock = PalBusClock(start)
        self.assertEqual(begin_receiver_poll(clock, MenuPollInterrupt(start)), 119467162)

    def test_palette_wrap_changes_poll_latency_without_timestamp_lookup(self):
        start = 181598778
        clock = PalBusClock(start)
        self.assertEqual(begin_receiver_poll(clock, MenuPollInterrupt(start)), 181601116)
