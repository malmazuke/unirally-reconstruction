"""Loss/order/clock failure paths of laboratory raw audio capture, no ROM required."""
import json
import struct
import tempfile
import sys
import unittest
from pathlib import Path
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'tools'))
from unirally_lab.reference import audio, bsnes


class FakeLib:
    def __init__(self):
        self.total, self.dropped, self.smp, self.dsp, self.balance = 0, 0, 64, 32, 0
    def unirally_audio_apu_frequency(self): return 24606720.0
    def unirally_audio_cpu_frequency(self): return 21281370
    def unirally_audio_smp_frequency(self): return 2050560
    def unirally_audio_total(self): return self.total
    def unirally_audio_dropped(self): return self.dropped
    def unirally_audio_smp_ticks(self): return self.smp
    def unirally_audio_dsp_clocks(self): return self.dsp
    def unirally_audio_dsp_balance(self): return self.balance
    def unirally_audio_enable(self, capacity, instructions): return True


class FakeCore:
    options = {'bsnes_run_ahead_frames': 'OFF'}
    def __init__(self): self._lib = FakeLib()
    def _memory(self, id_): return bytes(65536)


class AudioCaptureTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.directory = Path(self.temp.name) / 'capture'
        self.core = FakeCore()
        self.bind = patch.object(bsnes, 'bind_audio').start()
        self.addCleanup(patch.stopall)
        self.addCleanup(self.temp.cleanup)

    def create(self):
        return audio.AudioCapture(self.core, self.directory, 10, False, set(), {})

    def event(self, sequence, kind=2, address=0x8000, value=0x7fff, reserved=0):
        return bsnes._AUDIO_EVENT.pack(sequence, 64, 32, 100, 0x456, address, value, kind, reserved)

    def test_signed_stereo_bits_and_counts_are_retained(self):
        capture = self.create()
        self.core._lib.total = 2
        raw = self.event(0, 3, 0x4c, 1) + self.event(1)
        with patch.object(bsnes, 'read_audio_raw', return_value=raw): capture.drain(0)
        capture.finish()
        self.assertEqual((self.directory / 'pcm.s16le').read_bytes(), struct.pack('<hh', -32768, 32767))
        self.assertEqual((self.directory / 'events.bin').read_bytes(), raw)
        record = json.loads((self.directory / 'audio.json').read_text())
        self.assertEqual((record['events'], record['pairs'], record['status']), (2, 1, 'complete'))

    def test_missing_event_invalidates_capture(self):
        capture = self.create()
        self.core._lib.total = 2
        with patch.object(bsnes, 'read_audio_raw', return_value=self.event(0)):
            with self.assertRaisesRegex(bsnes.CoreError, 'lost'): capture.drain(0)
        capture.finish('failed', 'loss')
        self.assertEqual(json.loads((self.directory / 'audio.json').read_text())['status'], 'failed')

    def test_overflow_invalidates_capture_even_with_full_buffer(self):
        capture = self.create()
        self.core._lib.total = self.core._lib.dropped = 1
        with patch.object(bsnes, 'read_audio_raw', return_value=self.event(0)):
            with self.assertRaisesRegex(bsnes.CoreError, 'lost'): capture.drain(0)
        capture.finish('failed', 'overflow')

    def test_reordered_or_unknown_events_are_rejected(self):
        for raw in (self.event(1), self.event(0, kind=99), self.event(0, reserved=1)):
            with self.subTest(raw=raw):
                self.directory = Path(self.temp.name) / str(raw.hex())
                capture = self.create(); self.core._lib.total = 1
                with patch.object(bsnes, 'read_audio_raw', return_value=raw):
                    with self.assertRaisesRegex(bsnes.CoreError, 'sequence/layout'): capture.drain(0)
                capture.finish('failed', 'invalid event')

    def test_clock_frontier_must_agree_with_dsp_balance(self):
        capture = self.create(); self.core._lib.balance = 1
        with patch.object(bsnes, 'read_audio_raw', return_value=b''):
            with self.assertRaisesRegex(bsnes.CoreError, 'clock accounting'): capture.drain(0)
        capture.finish('failed', 'clock')

    def test_existing_output_is_not_overwritten(self):
        self.directory.mkdir(); marker = self.directory / 'events.bin'; marker.write_bytes(b'old')
        with self.assertRaises(FileExistsError): self.create()
        self.assertEqual(marker.read_bytes(), b'old')

    def test_unbounded_allocation_is_rejected(self):
        for capacity in (0, -1, 1000001):
            with self.assertRaises(bsnes.CoreError):
                audio.AudioCapture(self.core, self.directory, capacity, False, set(), {})
