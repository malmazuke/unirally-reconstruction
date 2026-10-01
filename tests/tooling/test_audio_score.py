"""Data-only score boundary tests; no cartridge bytes or cold audio claim."""
import copy
from pathlib import Path
import sys
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'tools/unirally_lab/reference/audio_lab'))
from title_menu_score import TitleMenuScore


def score(events):
    tables, title = bytearray(621), bytearray(2200)
    title[:len(events)] = bytes(events)
    result = TitleMenuScore(bytes(tables), bytes(title))
    result.reset_voice(0, 0x1d00)
    return result


class ScoreTests(unittest.TestCase):
    def test_call_and_counted_loop_restore_the_continuation(self):
        # Call a two-note loop, return, and then terminate the caller.
        native = score([0x82, 0x06, 0x1d, 0x80, 0, 0,
                        0x84, 2, 12, 1, 0x85, 0x83])
        native.update_voice(0, False)
        self.assertEqual(native.dispatches, [(0, 0x1d00, 0x82), (0, 0x1d06, 0x84), (0, 0x1d08, 12)])
        native.dispatches.clear()
        native.update_voice(0, False)
        self.assertEqual(native.dispatches, [(0, 0x1d0a, 0x85), (0, 0x1d08, 12)])
        native.update_voice(0, False)
        self.assertFalse(native.voices[0].enabled)
        self.assertEqual(native.voices[0].stack_position, 0)
        self.assertEqual(native.voices[0].pointer, 0x1d04)

    def test_zero_duration_wraps_through_255_updates(self):
        native = score([12, 0, 0x80])
        native.update_voice(0, False)
        for _ in range(255): native.update_voice(0, False)
        self.assertTrue(native.voices[0].enabled)
        native.update_voice(0, False)
        self.assertFalse(native.voices[0].enabled)

    def test_music_and_effect_updates_use_separate_modes(self):
        native = score([12, 1, 0x80])
        before = copy.deepcopy(native)
        native.update_voice(0, True)
        self.assertEqual(native, before)
        native.update_voice(0, False)
        self.assertEqual(native.voices[0].pointer, 0x1d02)

    def test_inline_volume_toggle_changes_parameter_boundaries(self):
        native = score([0xbf, 12, 70, 2, 0xc0, 14, 3, 0x80])
        native.update_voice(0, False)
        self.assertEqual(native.voices[0].volume, 70)
        self.assertEqual(native.voices[0].remaining, 2)
        native.update_voice(0, False); native.update_voice(0, False)
        self.assertEqual(native.voices[0].remaining, 3)
        self.assertEqual(native.voices[0].pointer, 0x1d07)

    def test_unknown_control_and_executable_pointer_are_rejected(self):
        native = score([0xc3])
        with self.assertRaisesRegex(ValueError, 'unrecovered'): native.update_voice(0, False)
        native = score([0x81, 0x00, 0x04])
        with self.assertRaisesRegex(ValueError, 'outside identified'): native.update_voice(0, False)

    def test_restore_preserves_nested_call_and_loop_state(self):
        native = score([0x84, 3, 12, 2, 0x85, 0x80])
        native.update_voice(0, False)
        restored = copy.deepcopy(native)
        for _ in range(10):
            native.update_voice(0, False); restored.update_voice(0, False)
        self.assertEqual(native, restored)

    def test_return_underflow_does_not_read_an_unrelated_range(self):
        native = score([0x83])
        with self.assertRaisesRegex(ValueError, 'underflow'): native.update_voice(0, False)
