"""Conditional score-reading recovery experiment, R-0075.

This parser reads identified data uploads only. It never fetches SPC instructions.
The diagnostic caller supplies voice-update modes and consumed commands; cold
transport, timers, voice arithmetic and DSP writes are still separate work.
Addresses identify the original data format, not executable entry points.
"""
from dataclasses import dataclass, field


@dataclass
class ScoreVoice:
    pointer: int = 0
    enabled: bool = False
    effect: int = 255
    priority: int = 255
    remaining: int = 1  # unsigned 8-bit update count, including zero -> 255
    per_note_pan: bool = False
    fixed_duration: int = 0
    next_duration_inline: bool = False
    sample: int = 0
    transpose: int = 0
    detune: int = 0
    release_relative: int = 0
    release_absolute: int = 0
    volume: int = 0
    pan: int = 0
    pan_step: int = 0
    pan_step_period: int = 0
    instrument: bytes = bytes(7)
    stack_position: int = 0
    # Names for these controls remain provisional until voice arithmetic closes.
    pitch_step: int = 0
    pitch_delay: int = 0
    pitch_rate: int = 0
    pitch_period: int = 0
    pitch_glide: int = 0
    pitch_alternate: tuple[int, int, int] = (0, 0, 0)
    suppress_key_on: bool = False
    envelope_mode_c9c: bool = False


@dataclass
class TitleMenuScore:
    """Data-only control parser with original unsigned pointer/stack semantics.

    `tables` is nominal $1600-$186C; `title` is $1D00-$2597. Executable upload
    bytes and the six next-record overrun bytes are intentionally unavailable.
    """
    tables: bytes
    title: bytes
    voices: list[ScoreVoice] = field(default_factory=lambda: [ScoreVoice() for _ in range(8)])
    stack: bytearray = field(default_factory=lambda: bytearray(128))
    random_state: int = 0x1b09133a
    timer2_target: int = 133
    reads: list[tuple[int, int, int]] = field(default_factory=list)
    dispatches: list[tuple[int, int, int]] = field(default_factory=list)
    instruments: bytearray = field(init=False)

    def __post_init__(self):
        if len(self.tables) != 621 or len(self.title) != 2200:
            raise ValueError('nominal title/menu data upload sizes differ')
        self.instruments = bytearray(self.tables[0x22:0x125])

    def data_byte(self, pointer: int) -> int:
        if 0x1600 <= pointer < 0x1600 + len(self.tables):
            return self.tables[pointer - 0x1600]
        if 0x1d00 <= pointer < 0x1d00 + len(self.title):
            return self.title[pointer - 0x1d00]
        raise ValueError(f'score pointer {pointer:04x} is outside identified data')

    def read_byte(self, index: int) -> int:
        voice = self.voices[index]
        pointer = voice.pointer
        result = self.data_byte(pointer)
        voice.pointer = (pointer + 1) & 65535
        self.reads.append((index, pointer, result))
        return result

    def read_word(self, index: int) -> int:
        low = self.read_byte(index)
        return low | self.read_byte(index) << 8

    def push_byte(self, index: int, value: int):
        voice = self.voices[index]
        if not 0 <= voice.stack_position < len(self.stack):
            raise ValueError('score stack leaves the recovered data domain')
        self.stack[voice.stack_position] = value
        voice.stack_position = (voice.stack_position + 1) & 255

    def pop_byte(self, index: int) -> int:
        voice = self.voices[index]
        voice.stack_position = (voice.stack_position - 1) & 255
        if voice.stack_position >= len(self.stack):
            raise ValueError('score stack underflow leaves the recovered domain')
        return self.stack[voice.stack_position]

    def push_pointer(self, index: int):
        pointer = self.voices[index].pointer
        self.push_byte(index, pointer & 255)
        self.push_byte(index, pointer >> 8)

    def reset_voice(self, index: int, pointer: int, effect=255, priority=255):
        self.voices[index] = ScoreVoice(pointer=pointer, enabled=True,
                                       effect=effect, priority=priority,
                                       stack_position=16 * index)

    def start_music(self, program: int):
        if program not in (0, 1):
            raise ValueError('music program outside the two-entry table')
        for index in range(8):
            base = 0x1600 + 4 * index + program
            high = self.data_byte(base + 2)
            if high:
                self.reset_voice(index, self.data_byte(base) | high << 8)
        root_index = (program - 1) & 255
        root = self.data_byte(0x1620 + root_index) | self.data_byte(0x1621 + root_index) << 8
        if root == 0x0101 and program == 0:
            # The silent program points into the driver's own freshly reset
            # voice fields. Its copy has FF effect tags and zero other fields;
            # this is native state, not another ROM or APU snapshot range.
            copied_state = bytearray(252)
            for index in range(8):
                copied_state[2 * index] = self.voices[index].effect
            self.instruments[7:] = copied_state
        elif root >> 8:
            copied = bytes(self.data_byte(root + n) for n in range(252))
            self.instruments[7:] = copied

    def start_effect(self, effect: int) -> int | None:
        if not 0 <= effect < 32:
            raise ValueError('effect outside its 32-entry table')
        flags = self.data_byte(0x1766 + effect)
        selected = None
        descending = range(7, -1, -1)
        if not flags & 128:
            selected = next((i for i in descending if self.voices[i].effect == effect), None)
        if selected is None:
            selected = next((i for i in descending if not self.voices[i].enabled), None)
        if selected is None:
            lowest = 255
            for i in descending:
                if lowest >= self.voices[i].priority:
                    lowest, selected = self.voices[i].priority, i
            if flags & 127 < lowest:
                return None
        pointer = self.data_byte(0x1726 + effect) | self.data_byte(0x1746 + effect) << 8
        self.reset_voice(selected, pointer, effect, flags & 127)
        return selected

    def random_choice(self, count: int) -> int:
        # $13DA-$1406: rotate EF, EE, ED, EC in that order, so the
        # four state bytes are a big-endian integer. Feedback/output uses EC.
        # Dispatch's doubled control byte supplies carry=1 on entry. Preserve
        # that carry between rotations; the 8-bit ADC itself never overflows.
        carry = 1
        for _ in range(8):
            feedback = (((self.random_state >> 24) & 0x48) + 0x38 + carry) >> 6 & 1
            carry = self.random_state >> 31
            self.random_state = ((self.random_state << 1) | feedback) & 0xffffffff
        return ((self.random_state >> 24) * count) >> 8

    def set_instrument(self, index: int, instrument: int):
        offset = 7 * instrument
        if offset + 7 > len(self.instruments):
            raise ValueError('instrument index outside identified table')
        self.voices[index].instrument = bytes(self.instruments[offset:offset + 7])

    def update_voice(self, index: int, effect_tick: bool):
        voice = self.voices[index]
        if not voice.enabled or effect_tick != (voice.effect != 255):
            return
        voice.remaining = (voice.remaining - 1) & 255
        if voice.remaining:
            return
        # Data may contain calls/jumps, but executable bytes are never decoded.
        # The diagnostic guard diagnoses a zero-duration control loop.
        for _ in range(4096):
            pointer = voice.pointer
            control = self.read_byte(index)
            self.dispatches.append((index, pointer, control))
            if control < 128:
                if voice.per_note_pan:
                    voice.volume = self.read_byte(index)
                if not voice.fixed_duration or voice.next_duration_inline:
                    voice.next_duration_inline = False
                    voice.remaining = self.read_byte(index)
                else:
                    voice.remaining = voice.fixed_duration
                return
            if control == 0x80:
                voice.enabled, voice.effect, voice.priority = False, 255, 0
                return
            if control == 0x81:
                voice.pointer = self.read_word(index)
            elif control == 0x82:
                target = self.read_word(index)
                self.push_pointer(index)
                voice.pointer = target
            elif control == 0x83:
                high = self.pop_byte(index)
                voice.pointer = self.pop_byte(index) | high << 8
            elif control == 0x84:
                repeats = self.read_byte(index)
                self.push_pointer(index)
                self.push_byte(index, repeats)
            elif control == 0x85:
                position = voice.stack_position
                if position < 3:
                    raise ValueError('loop has no stored continuation')
                repeats = (self.stack[position - 1] - 1) & 255
                if repeats:
                    self.stack[position - 1] = repeats
                    voice.pointer = self.stack[position - 3] | self.stack[position - 2] << 8
                else:
                    voice.stack_position = (position - 3) & 255
            elif control == 0x88:
                voice.transpose = self.read_byte(index)
            elif control == 0x89:
                voice.sample = self.read_byte(index)
            elif control == 0x8d:
                voice.detune = self.read_byte(index)
            elif control in (0x8e, 0x8f):
                voice.pitch_step = 1 if control == 0x8e else 255
                voice.pitch_delay = self.read_byte(index)
                voice.pitch_rate = self.read_byte(index)
                voice.pitch_period = self.read_byte(index)
            elif control == 0x90:
                voice.pitch_glide = self.read_byte(index)
            elif control == 0x91:
                voice.pitch_step = 0
            elif control == 0x92:
                voice.release_relative, voice.release_absolute = self.read_byte(index), 0
            elif control == 0x96:
                voice.pitch_alternate = tuple(self.read_byte(index) for _ in range(3))
            elif control == 0x97:
                self.set_instrument(index, self.read_byte(index))
            elif control in (0x9b, 0x9c):
                voice.envelope_mode_c9c = control == 0x9c
            elif control in (0x9e, 0x9f):
                voice.suppress_key_on = control == 0x9f
            elif control == 0xa2:
                voice.instrument = bytes(self.read_byte(index) for _ in range(7))
                voice.envelope_mode_c9c = False
            elif control == 0xa3:
                count = self.read_byte(index)
                skip = self.random_choice(count) * 2
                voice.pointer = (voice.pointer + skip) & 65535
                voice.pointer = self.read_word(index)
            elif control == 0xb3:
                voice.volume = self.read_byte(index)
                voice.pan = self.read_byte(index)
            elif control == 0xb4:
                voice.pan_step = self.read_byte(index)
            elif control == 0xb6:
                self.timer2_target = self.read_byte(index)
            elif control == 0xbc:
                pass  # Clear global $E9; it changes no score pointer or read.
            elif control == 0xbd:
                voice.pan = self.read_byte(index)
            elif control == 0xbe:
                voice.volume = self.read_byte(index)
            elif control in (0xbf, 0xc0):
                voice.per_note_pan = control == 0xbf
            else:
                raise ValueError(f'unrecovered score control {control:02x} at {pointer:04x}')
        raise ValueError('score control loop exceeds the diagnostic bound')
