"""ROM-free checks for the trace-derived access record (M1-02).

Nothing here loads the core or the ROM. The decoder is checked for every
opcode under every M/X/E combination and, per addressing mode, against
hand-derived effective addresses (direct-page wraps in emulation mode, the
16-bit D offset with its bank-0 wrap, bank crossing of absolute and long
indexed forms, stack pushes decrementing S, block-move directions, widths
under M/X, stored values). The streaming derivation runs on a synthetic ROM
image and a stub ring with full registers, including the resolution of
pointers and work RAM code from recorded stores and the frame's work RAM
images, the residual counting, the DMA parameter log, the watch logs and the
work RAM series writer. The CLI error paths are exercised through
``project.py``.
"""

from __future__ import annotations

import json
import struct
import subprocess
import sys
import tempfile
import unittest
import unittest.mock
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

from unirally_lab import EXIT_INVALID_INPUT, EXIT_MISSING_PREREQUISITE  # noqa: E402
from unirally_lab.access import commands as acmd  # noqa: E402
from unirally_lab.access import derive, modes  # noqa: E402
from unirally_lab.access.modes import BLOCK_READ, BLOCK_WRITE, PULL, PUSH, READ, RMW, WRAP_BANK, WRAP_LINEAR, WRAP_PAGE, WRITE  # noqa: E402
from unirally_lab.coverage import opcodes  # noqa: E402
from unirally_lab.reference import worker  # noqa: E402

PROJECT = ROOT / "tools" / "project.py"


def run_cli(*args: str) -> subprocess.CompletedProcess:
    return subprocess.run([sys.executable, str(PROJECT), *args], capture_output=True, text=True, timeout=300)


# Registers used by the hand-derived cases: native mode, 16-bit A and index.
PC, A, X, Y, S, D, B = 0x808000, 0x1234, 0x0102, 0x0203, 0x01F0, 0x0100, 0x7E
NEXT = (0x809000, 0x5678, 0x0A0B, 0x0C0D, 0x01F2, 0x0100, 0x7E, 0x00, 0)


def dec(opcode: int, operand: bytes = b"", *, a=A, x=X, y=Y, s=S, d=D, b=B, p=0x00, e=0, pc=PC, nxt=NEXT):
    return modes.decode(opcode, operand, pc, a, x, y, s, d, b, p, e, nxt)


class DecoderCoverageTests(unittest.TestCase):
    def test_every_opcode_decodes_in_every_mode(self) -> None:
        kinds_seen = set()
        for opcode in range(256):
            for p in (0x00, 0x10, 0x20, 0x30):
                for e in (0, 1):
                    with self.subTest(opcode=hex(opcode), p=hex(p), e=e):
                        operand = bytes([0x10, 0x20, 0x30])
                        accesses, deferred = dec(opcode, operand, p=p, e=e)
                        for kind, address, width, value, wrap in accesses:
                            self.assertIn(kind, range(7))
                            self.assertTrue(0 <= address <= 0xFFFFFF)
                            self.assertIn(width, (1, 2, 3))
                            self.assertIn(wrap, (0, 1, 2))
                            self.assertTrue(value is None or 0 <= value < (1 << (8 * width)))
                            kinds_seen.add(kind)
                        for item in deferred:
                            self.assertEqual(len(item), 8)
                            self.assertIn(item[5], (READ, WRITE, RMW))
        self.assertEqual(kinds_seen, set(range(7)))

    def test_access_classes_by_mnemonic(self) -> None:
        # Data-operand instructions: which kind each mnemonic produces (absolute form where it exists).
        expect = {"LDA": READ, "LDX": READ, "LDY": READ, "ADC": READ, "AND": READ, "BIT": READ, "CMP": READ, "CPX": READ, "CPY": READ,
                  "EOR": READ, "ORA": READ, "SBC": READ, "STA": WRITE, "STX": WRITE, "STY": WRITE, "STZ": WRITE,
                  "ASL": RMW, "DEC": RMW, "INC": RMW, "LSR": RMW, "ROL": RMW, "ROR": RMW, "TSB": RMW, "TRB": RMW}
        seen = {}
        for opcode, (mnemonic, addressing) in enumerate(opcodes.TABLE):
            if addressing == "abs" and mnemonic in expect and mnemonic not in seen:
                accesses, _ = dec(opcode, b"\x00\x10")
                seen[mnemonic] = accesses[0][0]
        self.assertEqual(seen, expect)
        # No memory access at all: immediates, implied register ops, branches, JMP abs, JML long.
        for opcode in (0xA9, 0xA2, 0xC9, 0x89, 0xEA, 0xAA, 0x18, 0xFB, 0xC2, 0xE2, 0x80, 0x82, 0xD0, 0x4C, 0x5C, 0x42, 0xCB, 0xDB):
            with self.subTest(opcode=hex(opcode)):
                self.assertEqual(dec(opcode, b"\x00\x10\x00"), ([], []))


class EffectiveAddressTests(unittest.TestCase):
    def test_direct_page_native(self) -> None:
        self.assertEqual(dec(0xA5, b"\x10"), ([(READ, 0x0110, 2, 0x5678, WRAP_BANK)], []))          # LDA dp
        self.assertEqual(dec(0xB5, b"\x10"), ([(READ, 0x0212, 2, 0x5678, WRAP_BANK)], []))          # LDA dp,X
        self.assertEqual(dec(0xB6, b"\x10"), ([(READ, 0x0313, 2, 0x0A0B, WRAP_BANK)], []))          # LDX dp,Y
        self.assertEqual(dec(0xA5, b"\x20", d=0xFFF0), ([(READ, 0x0010, 2, 0x5678, WRAP_BANK)], []))  # 16-bit D wraps in bank 0
        self.assertEqual(modes.byte_addresses(0xFFFF, 2, WRAP_BANK), [0x00FFFF, 0x000000])

    def test_direct_page_emulation_wraps(self) -> None:
        # E=1, DL=0: dp,X wraps within the direct page; X is 8-bit.
        self.assertEqual(dec(0xB5, b"\xF0", x=0x20, e=1, d=0x0100), ([(READ, 0x0110, 1, 0x78, WRAP_PAGE)], []))
        # Native or DL != 0: no page wrap.
        self.assertEqual(dec(0xB5, b"\xF0", x=0x20, e=0, d=0x0100, p=0x30), ([(READ, 0x0210, 1, 0x78, WRAP_BANK)], []))
        self.assertEqual(dec(0xB5, b"\xF0", x=0x20, e=1, d=0x0101), ([(READ, 0x0211, 1, 0x78, WRAP_BANK)], []))
        self.assertEqual(modes.byte_addresses(0x01FF, 2, WRAP_PAGE), [0x0001FF, 0x000100])
        # The (dp,X) pointer in emulation mode with DL != 0 keeps the page of D + dp.
        accesses, deferred = dec(0xA1, b"\xF0", x=0x20, e=1, d=0x0101)
        self.assertEqual(accesses, [(READ, 0x0111, 2, None, WRAP_PAGE)])
        self.assertEqual(deferred[0][:3], (0x0111, 2, WRAP_PAGE))
        # [dp] never page-wraps, (dp) does.
        self.assertEqual(dec(0xA7, b"\xFF", e=1, d=0x0100)[0], [(READ, 0x01FF, 3, None, WRAP_BANK)])
        self.assertEqual(dec(0xB2, b"\xFF", e=1, d=0x0100)[0], [(READ, 0x01FF, 2, None, WRAP_PAGE)])

    def test_absolute_and_long(self) -> None:
        self.assertEqual(dec(0x8D, b"\x34\x12"), ([(WRITE, 0x7E1234, 2, 0x1234, WRAP_LINEAR)], []))     # STA abs
        self.assertEqual(dec(0xBD, b"\xFF\xFF", x=2), ([(READ, 0x7F0001, 2, 0x5678, WRAP_LINEAR)], []))   # abs,X crosses the bank
        self.assertEqual(dec(0xB9, b"\xFF\xFF", y=2), ([(READ, 0x7F0001, 2, 0x5678, WRAP_LINEAR)], []))   # abs,Y
        self.assertEqual(modes.byte_addresses(0x7EFFFF, 2, WRAP_LINEAR), [0x7EFFFF, 0x7F0000])
        self.assertEqual(dec(0xAF, b"\x00\x10\x83"), ([(READ, 0x831000, 2, 0x5678, WRAP_LINEAR)], []))    # LDA long
        self.assertEqual(dec(0xBF, b"\xFF\xFF\x83", x=1), ([(READ, 0x840000, 2, 0x5678, WRAP_LINEAR)], []))  # long,X crosses
        self.assertEqual(dec(0x9C, b"\x00\x10"), ([(WRITE, 0x7E1000, 2, 0, WRAP_LINEAR)], []))         # STZ
        self.assertEqual(dec(0xEE, b"\x00\x10"), ([(RMW, 0x7E1000, 2, None, WRAP_LINEAR)], []))        # INC abs
        self.assertEqual(dec(0x16, b"\x10"), ([(RMW, 0x0212, 2, None, WRAP_BANK)], []))                # ASL dp,X

    def test_widths_follow_m_and_x(self) -> None:
        self.assertEqual(dec(0x8D, b"\x00\x10", p=0x20), ([(WRITE, 0x7E1000, 1, 0x34, WRAP_LINEAR)], []))   # STA with M=1
        self.assertEqual(dec(0x8E, b"\x00\x10", p=0x20), ([(WRITE, 0x7E1000, 2, 0x0102, WRAP_LINEAR)], []))  # STX with X=0
        self.assertEqual(dec(0x8E, b"\x00\x10", p=0x10), ([(WRITE, 0x7E1000, 1, 0x02, WRAP_LINEAR)], []))    # STX with X=1
        self.assertEqual(dec(0x8C, b"\x00\x10", p=0x10), ([(WRITE, 0x7E1000, 1, 0x03, WRAP_LINEAR)], []))    # STY with X=1
        self.assertEqual(dec(0xEC, b"\x00\x10", p=0x20), ([(READ, 0x7E1000, 2, None, WRAP_LINEAR)], []))    # CPX follows X
        self.assertEqual(dec(0x2C, b"\x00\x10", p=0x10), ([(READ, 0x7E1000, 2, None, WRAP_LINEAR)], []))    # BIT follows M
        self.assertEqual(dec(0x8D, b"\x00\x10", e=1), ([(WRITE, 0x7E1000, 1, 0x34, WRAP_LINEAR)], []))      # emulation forces 8-bit
        # Index registers are masked to 8 bits when X=1 even if the entry carries a high byte.
        self.assertEqual(dec(0xBD, b"\x00\x10", x=0x0102, p=0x10), ([(READ, 0x7E1002, 2, 0x5678, WRAP_LINEAR)], []))

    def test_stack_relative(self) -> None:
        self.assertEqual(dec(0xA3, b"\x03"), ([(READ, 0x01F3, 2, 0x5678, WRAP_BANK)], []))
        self.assertEqual(dec(0x83, b"\x03", s=0xFFFE), ([(WRITE, 0x0001, 2, 0x1234, WRAP_BANK)], []))
        accesses, deferred = dec(0xB3, b"\x03")   # LDA (sr),Y
        self.assertEqual(accesses, [(READ, 0x01F3, 2, None, WRAP_BANK)])
        self.assertEqual(modes.resolve_with_bank(deferred[0], 0x1000, B), (READ, 0x7E1203, 2, 0x5678, WRAP_LINEAR))

    def test_indirect_forms_defer_and_resolve(self) -> None:
        accesses, deferred = dec(0xB2, b"\x10")   # LDA (dp)
        self.assertEqual(accesses, [(READ, 0x0110, 2, None, WRAP_BANK)])
        self.assertEqual(deferred, [(0x0110, 2, WRAP_BANK, 0, True, READ, 2, 0x5678)])
        self.assertEqual(modes.resolve_with_bank(deferred[0], 0x2000, B), (READ, 0x7E2000, 2, 0x5678, WRAP_LINEAR))
        accesses, deferred = dec(0x91, b"\x10")   # STA (dp),Y: the bank crossing applies after the index
        self.assertEqual(deferred[0][3:], (Y, True, WRITE, 2, 0x1234))
        self.assertEqual(modes.resolve_with_bank(deferred[0], 0xFFF0, B), (WRITE, 0x7F01F3, 2, 0x1234, WRAP_LINEAR))
        accesses, deferred = dec(0xA1, b"\x10")   # LDA (dp,X): pointer at D + dp + X
        self.assertEqual(accesses, [(READ, 0x0212, 2, None, WRAP_BANK)])
        accesses, deferred = dec(0xA7, b"\x10")   # LDA [dp]: 24-bit pointer, no DBR
        self.assertEqual(accesses, [(READ, 0x0110, 3, None, WRAP_BANK)])
        self.assertEqual(modes.resolve_with_bank(deferred[0], 0x7F1000, B), (READ, 0x7F1000, 2, 0x5678, WRAP_LINEAR))
        accesses, deferred = dec(0xB7, b"\x10")   # LDA [dp],Y wraps in the 24-bit space
        self.assertEqual(modes.resolve_with_bank(deferred[0], 0xFFFFF0, B), (READ, 0x0001F3, 2, 0x5678, WRAP_LINEAR))
        self.assertEqual(dec(0xD4, b"\x10"), ([(READ, 0x0110, 2, None, WRAP_BANK), (PUSH, 0x01EF, 2, None, WRAP_BANK)], []))  # PEI

    def test_indirect_jumps_take_the_pointer_value_from_the_next_pc(self) -> None:
        self.assertEqual(dec(0x6C, b"\x00\x10"), ([(READ, 0x001000, 2, 0x9000, WRAP_BANK)], []))     # JMP (abs) in bank 0
        self.assertEqual(dec(0xDC, b"\x00\x10"), ([(READ, 0x001000, 3, 0x809000, WRAP_BANK)], []))   # JML [abs]
        self.assertEqual(dec(0x7C, b"\x00\x10"), ([(READ, 0x801102, 2, 0x9000, WRAP_BANK)], []))     # JMP (abs,X) in the program bank
        self.assertEqual(dec(0x6C, b"\x00\x10", nxt=None), ([(READ, 0x001000, 2, None, WRAP_BANK)], []))

    def test_pushes_decrement_s_and_carry_the_register(self) -> None:
        self.assertEqual(dec(0x48), ([(PUSH, 0x01EF, 2, 0x1234, WRAP_BANK)], []))            # PHA 16-bit: bytes at S-1, S
        self.assertEqual(dec(0x48, p=0x20), ([(PUSH, 0x01F0, 1, 0x34, WRAP_BANK)], []))      # PHA 8-bit at S
        self.assertEqual(dec(0xDA, p=0x10), ([(PUSH, 0x01F0, 1, 0x02, WRAP_BANK)], []))      # PHX 8-bit
        self.assertEqual(dec(0x5A), ([(PUSH, 0x01EF, 2, 0x0203, WRAP_BANK)], []))            # PHY 16-bit
        self.assertEqual(dec(0x08, p=0x35), ([(PUSH, 0x01F0, 1, 0x35, WRAP_BANK)], []))      # PHP
        self.assertEqual(dec(0x8B), ([(PUSH, 0x01F0, 1, 0x7E, WRAP_BANK)], []))              # PHB
        self.assertEqual(dec(0x4B), ([(PUSH, 0x01F0, 1, 0x80, WRAP_BANK)], []))              # PHK
        self.assertEqual(dec(0x0B), ([(PUSH, 0x01EF, 2, 0x0100, WRAP_BANK)], []))            # PHD
        self.assertEqual(dec(0xF4, b"\x34\x12"), ([(PUSH, 0x01EF, 2, 0x1234, WRAP_BANK)], []))    # PEA
        self.assertEqual(dec(0x62, b"\x10\x00"), ([(PUSH, 0x01EF, 2, 0x8013, WRAP_BANK)], []))    # PER: pc + 3 + rel
        self.assertEqual(dec(0x20, b"\x00\x90"), ([(PUSH, 0x01EF, 2, 0x8002, WRAP_BANK)], []))    # JSR abs pushes pc + 2
        self.assertEqual(dec(0x22, b"\x00\x90\x01"), ([(PUSH, 0x01EE, 3, 0x808003, WRAP_BANK)], []))   # JSL pushes PB and pc + 3
        self.assertEqual(dec(0xFC, b"\x00\x90"), ([(PUSH, 0x01EF, 2, 0x8002, WRAP_BANK), (READ, 0x809102, 2, 0x9000, WRAP_BANK)], []))
        self.assertEqual(dec(0x00, b"\x00"), ([(PUSH, 0x01EE, 3, 0x808002, WRAP_BANK), (PUSH, 0x01ED, 1, 0x00, WRAP_BANK)], []))  # BRK native
        # Emulation mode: the 6502-era pushes stay within page 1, the 65816-only ones do not.
        self.assertEqual(dec(0x48, s=0x0100, e=1), ([(PUSH, 0x0100, 1, 0x34, WRAP_PAGE)], []))
        self.assertEqual(dec(0x20, b"\x00\x90", s=0x0100, e=1), ([(PUSH, 0x01FF, 2, 0x8002, WRAP_PAGE)], []))
        self.assertEqual(modes.byte_addresses(0x01FF, 2, WRAP_PAGE), [0x01FF, 0x0100])
        self.assertEqual(dec(0x0B, s=0x0100, e=1), ([(PUSH, 0x00FF, 2, 0x0100, WRAP_BANK)], []))
        self.assertEqual(dec(0x00, b"\x00", s=0x01F0, e=1, p=0x30), ([(PUSH, 0x01EF, 2, 0x8002, WRAP_PAGE), (PUSH, 0x01EE, 1, 0x30, WRAP_PAGE)], []))

    def test_pulls_read_above_s_and_take_the_next_register(self) -> None:
        self.assertEqual(dec(0x68), ([(PULL, 0x01F1, 2, 0x5678, WRAP_BANK)], []))            # PLA
        self.assertEqual(dec(0x68, p=0x20), ([(PULL, 0x01F1, 1, 0x78, WRAP_BANK)], []))
        self.assertEqual(dec(0xFA), ([(PULL, 0x01F1, 2, 0x0A0B, WRAP_BANK)], []))            # PLX
        self.assertEqual(dec(0x7A, p=0x10), ([(PULL, 0x01F1, 1, 0x0D, WRAP_BANK)], []))      # PLY
        self.assertEqual(dec(0x28), ([(PULL, 0x01F1, 1, 0x00, WRAP_BANK)], []))              # PLP
        self.assertEqual(dec(0xAB), ([(PULL, 0x01F1, 1, 0x7E, WRAP_BANK)], []))              # PLB
        self.assertEqual(dec(0x2B), ([(PULL, 0x01F1, 2, 0x0100, WRAP_BANK)], []))            # PLD
        self.assertEqual(dec(0x60), ([(PULL, 0x01F1, 2, 0x8FFF, WRAP_BANK)], []))            # RTS pulls next pc - 1
        self.assertEqual(dec(0x6B), ([(PULL, 0x01F1, 3, 0x808FFF, WRAP_BANK)], []))          # RTL
        self.assertEqual(dec(0x40), ([(PULL, 0x01F1, 1, 0x00, WRAP_BANK), (PULL, 0x01F2, 3, 0x809000, WRAP_BANK)], []))   # RTI native
        self.assertEqual(dec(0x40, e=1, s=0x01FE), ([(PULL, 0x01FF, 1, 0x00, WRAP_PAGE), (PULL, 0x0100, 2, 0x9000, WRAP_PAGE)], []))
        self.assertEqual(dec(0x68, nxt=None), ([(PULL, 0x01F1, 2, None, WRAP_BANK)], []))

    def test_block_moves(self) -> None:
        # Operand: destination bank first, then source bank; one byte per traced re-execution.
        self.assertEqual(dec(0x54, b"\x7E\x81"), ([(BLOCK_READ, 0x810102, 1, None, WRAP_BANK), (BLOCK_WRITE, 0x7E0203, 1, None, WRAP_BANK)], []))
        self.assertEqual(dec(0x44, b"\x7F\x00", x=0xFFFF, y=0x0000), ([(BLOCK_READ, 0x00FFFF, 1, None, WRAP_BANK), (BLOCK_WRITE, 0x7F0000, 1, None, WRAP_BANK)], []))
        self.assertEqual(dec(0x54, b"\x7E\x81", x=0x0102, p=0x10), ([(BLOCK_READ, 0x810002, 1, None, WRAP_BANK), (BLOCK_WRITE, 0x7E0003, 1, None, WRAP_BANK)], []))

    def test_key_round_trip(self) -> None:
        for fields in ((0x808000, 7, 24, 6, 3, 2, 0xFFFFFF), (0, 0, 0, 0, 1, 0, 0), (0x123456, 3, 9, 1, 2, 1, 0x7E0313)):
            self.assertEqual(derive.decode_key(derive.encode_key(*fields)), fields)


# ------------------------------------------------------------------ derivation


class StubRing:
    def __init__(self) -> None:
        self.entries: list[bytes] = []

    def execute(self, pc: int, a=A, x=X, y=Y, s=S, d=D, b=B, p=0x00, e=0) -> None:
        self.entries.append(struct.pack("<IHHHHHBBBBHH", pc, a, x, y, s, d, b, p, e, 0, 0, 0))

    def raw(self) -> bytes:
        out = b"".join(self.entries)
        self.entries = []
        return out


def synthetic_rom() -> bytes:
    rom = bytearray(0x10000)
    prog = bytes([
        0x8D, 0x00, 0x10,        # 8000 STA $1000        write $7E:1000 (16-bit)
        0x86, 0x20,              # 8003 STX $20          write $00:0120 (pointer for (dp) below)
        0xB2, 0x20,              # 8005 LDA ($20)        pointer recorded this frame -> recorded_store
        0x92, 0x22,              # 8007 STA ($22)        pointer never written this frame -> end_of_frame
        0xB7, 0x24,              # 8009 LDA [$24],Y      pointer written later in the frame -> start_of_frame
        0x9C, 0x24, 0x01,        # 800B STZ $0124        the later write (bank $7E mirror of $00:0124)
        0xEE, 0x26, 0x01,        # 800E INC $0126        RMW: unknown value ...
        0xB2, 0x26,              # 8011 LDA ($26)        ... so this pointer is unresolved
        0x20, 0x30, 0x80,        # 8013 JSR $8030        push $8015
        0xAF, 0x00, 0x80, 0x01,  # 8016 LDA $018000      ROM read
        0x54, 0x7E, 0x01,        # 801A MVN $01,$7E      block move (dst $7E, src $01)
        0xA9, 0x00, 0x43,        # 801D LDA #$4300       (no access)
        0x8D, 0x0B, 0x42,        # 8020 STA $420B        DMA trigger (value from A)
        0xEE, 0x04, 0x30,        # 8023 INC $3004        RMW on a byte that later executes as code in work RAM
        0xEA,                    # 8026 NOP
    ])
    rom[0:len(prog)] = prog
    rom[0x30:0x32] = bytes([0xEA, 0x60])   # 8030 NOP; RTS
    rom[0x8000:0x8002] = bytes([0x11, 0x22])  # $01:8000 data
    return bytes(rom)


def run_synthetic(series_path: Path | None = None, watch: list[int] | None = None):
    rom = synthetic_rom()
    ring = StubRing()
    b0 = 0x00
    # Frame 0 executes the program once (b = $00 for the DMA and register stores, $7E for the data stores).
    ring.execute(0x8000, a=0x1234, b=0x7E)
    ring.execute(0x8003, x=0x1000, b=0x7E)                 # STX $20 -> $0120 = 0x1000
    ring.execute(0x8005, b=0x7E)                           # LDA ($20) -> read $7E:1000
    ring.execute(0x8007, a=0xBEEF, b=0x7E)                 # STA ($22) -> pointer from end-of-frame WRAM
    ring.execute(0x8009, y=0x0004, b=0x7E)                 # LDA [$24],Y -> pointer from start-of-frame WRAM
    ring.execute(0x800B, b=0x7E)                           # STZ $0124
    ring.execute(0x800E, b=0x7E)                           # INC $0126
    ring.execute(0x8011, b=0x7E)                           # LDA ($26) unresolved
    ring.execute(0x8013, s=0x01F0, b=0x7E)                 # JSR
    ring.execute(0x8030, s=0x01EE, b=0x7E)
    ring.execute(0x8031, s=0x01EE, b=0x7E)                 # RTS
    ring.execute(0x8016, b=0x7E)                           # LDA long from ROM
    ring.execute(0x801A, x=0x8000, y=0x2000, a=1, b=0x7E)  # MVN twice (two bytes)
    ring.execute(0x801A, x=0x8001, y=0x2001, a=0, b=0x7E)
    ring.execute(0x801D, b=b0)
    ring.execute(0x8020, a=0x0001, b=b0, p=0x20)           # STA $420B (8-bit) value 1
    ring.execute(0x8023, b=0x7E)                           # INC $3004: unknown value written to the future opcode byte
    ring.execute(0x8026, b=b0)
    # A work RAM code site whose bytes are only known from the end-of-frame image.
    ring.execute(0x7E3000, a=0x0055, b=0x7E, p=0x20)       # bytes at $7E:3000: 8D 00 31 = STA $3100 (8-bit)
    ring.execute(0x7E3003, b=0x7E, p=0x20)                 # bytes: EA
    ring.execute(0x7E3004, b=0x7E, p=0x20)                 # opcode byte last written by the INC (unknown value) -> unresolved
    sof = bytearray(derive.WRAM_SIZE)
    eof = bytearray(derive.WRAM_SIZE)
    sof[0x124:0x127] = bytes([0x00, 0x50, 0x7F])      # [$24] at frame start = $7F5000
    eof[0x122:0x124] = bytes([0x00, 0x20])            # ($22) = $2000 (unchanged in the frame)
    eof[0x120:0x122] = bytes([0x00, 0x10])            # ($20) as the store left it
    eof[0x3000:0x3005] = bytes([0x8D, 0x00, 0x31, 0xEA, 0x00])
    eof[0x1000:0x1002] = bytes([0x34, 0x12])
    series = None if series_path is None else (0x1000, 4, 1, series_path)
    d = derive.AccessDrain(rom, 64, watch_addresses=watch, watch_pcs=[0x8005], series=series)
    d.set_previous_wram(bytes(sof))
    raw = ring.raw()
    d.series_frame(0, bytes(eof))
    d.drain_frame(0, raw, bytes(eof))
    # Frame 1: only the INC and the unresolved pointer read, to check first/last frames and counts.
    ring.execute(0x800E, b=0x7E)
    ring.execute(0x8011, b=0x7E)
    d.series_frame(1, bytes(eof))
    d.drain_frame(1, ring.raw(), bytes(eof))
    doc = d.document({"rom": {"sha256": "a" * 64, "size": len(rom)}, "core": {"name": "stub"}, "script": {"frames": 2}, "status": "complete"}, (0, 1))
    return derive.validate_document(doc), d


class DerivationTests(unittest.TestCase):
    def setUp(self) -> None:
        self.doc, self.drain = run_synthetic(watch=[0x7E1000, 0x000124])
        self.rows = {(r["pc"], r["kind"], r["address"]): r for r in acmd.access_rows(self.doc)}

    def test_direct_accesses_and_values(self) -> None:
        r = self.rows[(0x8000, "write", 0x7E1000)]
        self.assertEqual((r["width"], r["values"], r["count"], r["addressing"], r["mode"], r["label"]), (2, [0x1234], 1, "abs", "NMX", None))
        r = self.rows[(0x8003, "write", 0x0120)]
        self.assertEqual((r["width"], r["values"]), (2, [0x1000]))
        self.assertEqual(self.rows[(0x8013, "push", 0x01EF)]["values"], [0x8015])
        self.assertEqual(self.rows[(0x8031, "pull", 0x01EF)]["count"], 1)
        self.assertEqual(self.rows[(0x800E, "rmw", 0x7E0126)]["count"], 2)
        self.assertEqual((self.rows[(0x800E, "rmw", 0x7E0126)]["first_frame"], self.rows[(0x800E, "rmw", 0x7E0126)]["last_frame"]), (0, 1))
        self.assertEqual(self.rows[(0x801A, "block_write", 0x7E2000)]["count"], 1)
        self.assertEqual(self.rows[(0x8020, "write", 0x00420B)]["values"], [1])
        self.assertEqual(self.doc["accesses_total"], sum(r["count"] for r in self.rows.values()))
        self.assertEqual(self.rows[(0x8023, "rmw", 0x7E3004)]["count"], 1)
        self.assertEqual(self.doc["instructions"], {"total": 23, "max_frame_delta": 21, "per_frame": [21, 2]})

    def test_rom_reads_are_ranges_with_a_bitmap(self) -> None:
        names = self.doc["addressing_names"]
        kinds = self.doc["kind_names"]
        reads = {(r[0], kinds[r[3]]): r for r in self.doc["rom_reads"]}
        self.assertEqual(reads[(0x8016, "read")][4:7], [1, 0x018000, 0x018000])
        self.assertEqual(names[reads[(0x8016, "read")][2]], "long")
        self.assertEqual(reads[(0x801A, "block_read")][4:7], [2, 0x018000, 0x018001])
        self.assertEqual(self.doc["rom_bytes_read"], 2)
        self.assertEqual(self.doc["rom_read_ranges"], [[0x8000, 2]])

    def test_resolution_labels(self) -> None:
        res = {(pc, self.doc["addressing_names"][ad] if ad >= 0 else "wram_code", label): n for pc, ad, label, n in self.doc["resolutions"]}
        self.assertEqual(res[(0x8005, "idp", "recorded_store")], 1)
        self.assertEqual(res[(0x8007, "idp", "end_of_frame")], 1)
        self.assertEqual(res[(0x8009, "ildpy", "start_of_frame")], 1)
        self.assertEqual(res[(0x8011, "idp", "unresolved")], 2)
        self.assertEqual(res[(0x7E3000, "abs", "end_of_frame")], 1)
        self.assertEqual(res[(0x7E3004, "wram_code", "unresolved")], 1)
        self.assertEqual(res[(0x7E3003, "imp", "end_of_frame")], 1)
        # Resolved accesses carry their label and the resolved address.
        self.assertEqual(self.rows[(0x8005, "read", 0x7E1000)]["label"], "recorded_store")
        self.assertEqual(self.rows[(0x8007, "write", 0x7E2000)]["values"], [0xBEEF])
        self.assertEqual(self.rows[(0x8007, "write", 0x7E2000)]["label"], "end_of_frame")
        self.assertEqual(self.rows[(0x8009, "read", 0x7F5004)]["label"], "start_of_frame")
        self.assertEqual(self.rows[(0x7E3000, "write", 0x7E3100)]["values"], [0x55])
        self.assertEqual(self.rows[(0x7E3000, "write", 0x7E3100)]["label"], "end_of_frame")
        self.assertEqual(self.doc["per_frame_unresolved"], [2, 1])
        self.assertEqual(self.doc["residual"]["unresolved_total"], 3)
        self.assertEqual(self.doc["residual"]["unresolved_stores"], 0)
        self.assertEqual(self.doc["residual"]["non_rom_pcs"], [[0x7E3000, 1], [0x7E3003, 1], [0x7E3004, 1]])

    def test_dma_log_and_watches(self) -> None:
        self.assertEqual(len(self.doc["dma_log"]), 1)
        frame, seq, pc, reg, value, channels = self.doc["dma_log"][0]
        self.assertEqual((frame, pc, reg, value, list(channels)), (0, 0x8020, "MDMAEN", 1, ["0"]))
        self.assertEqual(self.doc["residual"]["dma_triggers"], 1)
        watch = self.doc["watch_addresses"]
        self.assertEqual(sorted(watch), ["292", "4096"])   # keys are work RAM offsets in decimal (mirrors fold)
        f0 = watch["4096"]["0"]
        self.assertEqual([w[1:] for w in f0["w"]], [[0x8000, "write", 0x7E1000, 2, 0x1234]])
        self.assertEqual([r[1] for r in f0["r"]], [0x8005])
        self.assertEqual([w[1] for w in watch["292"]["0"]["w"]], [0x800B])
        self.assertEqual(self.doc["watch_pcs"]["32773"], [[0, A, X, Y, S, D, B, 0x00, 0]])

    def test_documents_are_byte_identical_and_validated(self) -> None:
        doc2, _ = run_synthetic(watch=[0x7E1000, 0x000124])
        self.assertEqual(json.dumps(self.doc, sort_keys=True), json.dumps(doc2, sort_keys=True))
        for label, mutate in {"schema": lambda x: x.update(schema_version=2), "kind": lambda x: x.update(kind="samples"),
                              "missing": lambda x: x.pop("residual"), "shape": lambda x: x.update(accesses=[[1, 2]]),
                              "total": lambda x: x["instructions"].update(total=1)}.items():
            with self.subTest(case=label):
                bad = json.loads(json.dumps(self.doc))
                mutate(bad)
                with self.assertRaises(ValueError):
                    derive.validate_document(bad)

    def test_query_matches_byte_spans_and_mirrors(self) -> None:
        q = acmd.query(self.doc, address=0x7E1001)     # second byte of the 16-bit store
        self.assertEqual([w["pc"] for w in q["writers"]], [0x8000])
        self.assertEqual([r["pc"] for r in q["readers"]], [0x8005])
        q = acmd.query(self.doc, address=0x7E0120)     # $00:0120 written by STX dp, queried through the mirror
        self.assertEqual([w["pc"] for w in q["writers"]], [0x8003])
        q = acmd.query(self.doc, pc=0x8016)
        self.assertEqual(len(q["rom_reads"]), 1)
        self.assertEqual(acmd.query(self.doc, address=0x7E1000, kind="read")["writers"], [])
        self.assertIn("$00:8000", acmd.format_rows(acmd.query(self.doc, address=0x7E1000)))

    def test_wram_series(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "series.bin"
            doc, _ = run_synthetic(series_path=path)
            self.assertEqual(path.read_bytes(), bytes([0x34, 0x12, 0, 0]) * 2)
            self.assertEqual(doc["wram_series"]["frames"], [0, 1])
            self.assertEqual(doc["wram_series"]["bytes"], 8)

    def test_resolver_rules(self) -> None:
        start = bytes([0xAA]) * derive.WRAM_SIZE
        end = bytes([0xBB]) * derive.WRAM_SIZE
        r = derive.Resolver({0x10: [(5, 0x11), (9, None), (20, 0x22)]}, start, end)
        self.assertEqual(r.byte(0x11, 3), (0xBB, 2))        # never written: end of frame
        self.assertEqual(r.byte(0x10, 3), (0xAA, 1))        # written only later: start of frame
        self.assertEqual(r.byte(0x10, 5), (0xAA, 1))        # a write at the same seq comes after the read
        self.assertEqual(r.byte(0x10, 7), (0x11, 0))        # last recorded store
        self.assertEqual(r.byte(0x10, 12), (None, 3))       # last write has an unknown value
        self.assertEqual(r.byte(0x10, 25), (0x22, 0))
        self.assertEqual(r.value([0x10, 0x11], 7), (0xBB11, 2))


def run_unresolved_store():
    """Two frames of a program whose indirect stores go through a pointer: in frame 0 an INC changes
    the pointer bytes earlier in the same frame (the D-0002 residual: the stored value is exact, the
    address is not), in frame 1 the pointer is untouched and the stores resolve from the end-of-frame image."""
    rom = bytearray(0x10000)
    rom[0:8] = bytes([
        0xEE, 0x26, 0x01,   # 8000 INC $0126       RMW on the pointer bytes: their value is unknown from here on
        0x92, 0x26,         # 8003 STA ($26)       store through the changed pointer -> unresolved store
        0x87, 0x26,         # 8005 STA [$26]       long indirect store through the same pointer -> unresolved store
        0xEA,               # 8007 NOP
    ])
    ring = StubRing()
    ring.execute(0x8000, b=0x7E)
    ring.execute(0x8003, a=0xBEEF, b=0x7E)
    ring.execute(0x8005, a=0xCAFE, b=0x7E)
    ring.execute(0x8007, b=0x7E)
    eof = bytearray(derive.WRAM_SIZE)
    eof[0x126:0x129] = bytes([0x00, 0x20, 0x7E])   # ($26) = $2000 in the data bank, [$26] = $7E:2000
    d = derive.AccessDrain(bytes(rom), 64)
    d.set_previous_wram(bytes(derive.WRAM_SIZE))
    d.drain_frame(0, ring.raw(), bytes(eof))
    ring.execute(0x8003, a=0xBEEF, b=0x7E)
    ring.execute(0x8005, a=0xCAFE, b=0x7E)
    d.drain_frame(1, ring.raw(), bytes(eof))
    doc = d.document({"rom": {"sha256": "a" * 64, "size": len(rom)}, "core": {"name": "stub"}, "script": {"frames": 2}, "status": "complete"}, (0, 1))
    return derive.validate_document(doc)


class UnresolvedStoreTests(unittest.TestCase):
    """The residual the D-0002 fallback condition rests on: a store through a pointer changed by a
    read-modify-write instruction in the same frame is counted as an unresolved store, never guessed."""

    def setUp(self) -> None:
        self.doc = run_unresolved_store()
        self.rows = {(r["pc"], r["kind"], r["address"]): r for r in acmd.access_rows(self.doc)}

    def test_unresolved_stores_are_counted_per_pc(self) -> None:
        residual = self.doc["residual"]
        self.assertEqual(residual["unresolved_stores"], 2)
        self.assertEqual(residual["unresolved_store_pcs"], [[0x8003, 1], [0x8005, 1]])
        self.assertEqual(residual["unresolved_total"], 2)
        self.assertEqual(self.doc["per_frame_unresolved"], [2, 0])
        names = self.doc["addressing_names"]
        res = {(pc, names[ad], label): n for pc, ad, label, n in self.doc["resolutions"] if ad >= 0}
        self.assertEqual(res[(0x8003, "idp", "unresolved")], 1)
        self.assertEqual(res[(0x8005, "ildp", "unresolved")], 1)
        self.assertEqual(res[(0x8003, "idp", "end_of_frame")], 1)
        self.assertEqual(res[(0x8005, "ildp", "end_of_frame")], 1)

    def test_unresolved_stores_produce_no_access_row(self) -> None:
        # Frame 1's resolved stores are the only rows at those pcs: one store each, first and last frame 1.
        self.assertEqual(self.rows[(0x8000, "rmw", 0x7E0126)]["count"], 1)
        for pc, value in ((0x8003, 0xBEEF), (0x8005, 0xCAFE)):
            row = self.rows[(pc, "write", 0x7E2000)]
            self.assertEqual((row["count"], row["values"], row["label"], row["first_frame"], row["last_frame"]), (1, [value], "end_of_frame", 1, 1))
        # The pointer fetch itself is a direct-page read at a known address and is recorded in both frames.
        self.assertEqual(sorted(k for k in self.rows if k[0] in (0x8003, 0x8005)),
                         [(0x8003, "read", 0x0126), (0x8003, "write", 0x7E2000), (0x8005, "read", 0x0126), (0x8005, "write", 0x7E2000)])
        self.assertEqual((self.rows[(0x8003, "read", 0x0126)]["count"], self.rows[(0x8003, "read", 0x0126)]["width"]), (2, 2))
        self.assertEqual((self.rows[(0x8005, "read", 0x0126)]["count"], self.rows[(0x8005, "read", 0x0126)]["width"]), (2, 3))
        self.assertEqual(self.doc["accesses_total"], 7)


def rmw_rom() -> bytes:
    rom = bytearray(0x10000)
    prog = bytes([
        0xA7, 0x63,              # 8000 LDA [$63]        pointer $01:8000 from the frame's start
        0xE6, 0x63,              # 8002 INC $63          16-bit: the pointer's low word plus one
        0xA7, 0x63,              # 8004 LDA [$63]        $01:8001 only when the INC's result is computed
        0x54, 0x00, 0x01,        # 8006 MVN $01,$00      block move: writes $00:0063 with an unknown value
        0xE6, 0x63,              # 8009 INC $63          old value unknown -> stays unknown
        0xA7, 0x63,              # 800B LDA [$63]        unresolved either way
    ])
    rom[0:len(prog)] = prog
    return bytes(rom)


def run_rmw(resolve_rmw: bool, incs: int = 1):
    """Frame 0: read through [$63], advance it by ``incs`` INCs, read again. Frame 1: a block move
    clobbers the pointer's low byte before the INC, so the next read is unresolved."""
    rom = rmw_rom()
    ring = StubRing()
    ring.execute(0x8000, d=0, b=0x7E)
    for _ in range(incs):
        ring.execute(0x8002, d=0, b=0x7E)
    ring.execute(0x8004, d=0, b=0x7E)
    sof = bytearray(derive.WRAM_SIZE)
    sof[0x63:0x66] = bytes([0x00, 0x80, 0x01])
    eof = bytearray(sof)
    end = 0x8000 + incs
    eof[0x63:0x65] = end.to_bytes(2, "little")
    d = derive.AccessDrain(rom, incs + 8, resolve_rmw=resolve_rmw)
    d.set_previous_wram(bytes(sof))
    d.drain_frame(0, ring.raw(), bytes(eof))
    ring.execute(0x8006, x=0x8000, y=0x0063, a=0, d=0, b=0x7E)
    ring.execute(0x8009, d=0, b=0x7E)
    ring.execute(0x800B, d=0, b=0x7E)
    d.drain_frame(1, ring.raw(), bytes(eof))
    doc = d.document({"rom": {"sha256": "a" * 64, "size": len(rom)}, "core": {"name": "stub"}, "script": {"frames": 2}, "status": "complete"}, (0, 1))
    return derive.validate_document(doc)


class ResolveRmwTests(unittest.TestCase):
    def test_off_by_default_an_inc_leaves_the_pointer_unknown(self) -> None:
        doc = run_rmw(False)
        self.assertFalse(doc["resolve_rmw"])
        self.assertEqual(doc["rom_read_ranges"], [[0x8000, 2]])            # [$63] read at $01:8000 only (16-bit A)
        self.assertEqual(doc["per_frame_unresolved"], [1, 1])

    def test_an_inc_advances_the_resolved_pointer(self) -> None:
        doc = run_rmw(True)
        self.assertTrue(doc["resolve_rmw"])
        self.assertEqual(doc["rom_read_ranges"], [[0x8000, 3]])            # $01:8000-8001 and $01:8001-8002
        self.assertEqual(doc["per_frame_unresolved"], [0, 1])              # the clobbered pointer stays unknown
        rows = {(r[0], r[3]): r for r in doc["rom_reads"]}
        self.assertEqual((rows[(0x8004, READ)][5], rows[(0x8004, READ)][6]), (0x018001, 0x018001))

    def test_a_long_chain_is_computed_without_recursion(self) -> None:
        doc = run_rmw(True, incs=5000)
        self.assertIn([0x8000 + 5000, 2], doc["rom_read_ranges"])
        self.assertEqual(doc["per_frame_unresolved"], [0, 1])

    def test_every_rmw_result(self) -> None:
        def op(m, width=1, a=0, carry=0):
            return derive.RmwOp(0, m, width, a, carry, (0,) * width)
        self.assertEqual(op("INC").apply(0xFF), 0x00)
        self.assertEqual(op("INC", 2).apply(0x00FF), 0x0100)
        self.assertEqual(op("DEC", 2).apply(0x0000), 0xFFFF)
        self.assertEqual(op("ASL").apply(0x81), 0x02)
        self.assertEqual(op("LSR", 2).apply(0x8001), 0x4000)
        self.assertEqual(op("ROL", carry=1).apply(0x80), 0x01)
        self.assertEqual(op("ROR", 2, carry=1).apply(0x0002), 0x8001)
        self.assertEqual(op("TSB", a=0x1234).apply(0x01), 0x35)
        self.assertEqual(op("TRB", 2, a=0x00F0).apply(0x12FF), 0x120F)
        with self.assertRaises(ValueError):
            op("LDA").apply(0)


def round_rom() -> bytes:
    rom = bytearray(0x10000)
    prog = bytes([
        0xA7, 0x63,              # 8000 LDA [$63]        pointer bytes written later in the frame
        0x87, 0x80,              # 8002 STA [$80]        resolved store to $7E:0063: the later write
        0xE6, 0x63,              # 8004 INC $63          16-bit RMW of the pointer (resolve_rmw)
        0x87, 0x63,              # 8006 STA [$63]        store through the pointer
    ])
    rom[0:len(prog)] = prog
    return bytes(rom)


def run_rounds(program: list[tuple[int, dict]], sof_bytes: dict[int, bytes], eof_bytes: dict[int, bytes], resolve_rmw: bool = False):
    """One frame of ``round_rom`` with the given (pc, registers) entries and work RAM images."""
    rom = round_rom()
    ring = StubRing()
    for pc, regs in program:
        ring.execute(pc, **{"d": 0, "b": 0x7E, **regs})
    sof = bytearray(derive.WRAM_SIZE)
    eof = bytearray(derive.WRAM_SIZE)
    for off, data in sof_bytes.items():
        sof[off:off + len(data)] = data
    for off, data in eof_bytes.items():
        eof[off:off + len(data)] = data
    d = derive.AccessDrain(rom, 16, resolve_rmw=resolve_rmw)
    d.set_previous_wram(bytes(sof))
    d.drain_frame(0, ring.raw(), bytes(eof))
    doc = d.document({"rom": {"sha256": "a" * 64, "size": len(rom)}, "core": {"name": "stub"}, "script": {"frames": 1}, "status": "complete"}, (0, 0))
    return derive.validate_document(doc)


# LDA [$63] reads the pointer before STA [$80] writes it. The first round has no record of that
# write, so it takes the pointer from the end-of-frame image ($01:9000); once the round's resolved
# store is merged, the pointer comes from the start-of-frame image ($01:8000), its value at the load.
LATER_WRITE = ([(0x8000, {}), (0x8002, {"a": 0x9000})],
               {0x63: bytes([0x00, 0x80, 0x01]), 0x80: bytes([0x63, 0x00, 0x7E])},
               {0x63: bytes([0x00, 0x90, 0x01]), 0x80: bytes([0x63, 0x00, 0x7E])})


class ResolutionRoundTests(unittest.TestCase):
    """Only the kept round's kept resolutions reach the record (ACCESS-ROUND-READS)."""

    def test_a_discarded_round_leaves_no_rom_read(self) -> None:
        doc = run_rounds(*LATER_WRITE)
        self.assertNotIn([0x9000, 2], doc["rom_read_ranges"])       # the first round's wrong pointer
        self.assertEqual(doc["rom_bytes_read"], sum(n for _off, n in doc["rom_read_ranges"]))
        for row in doc["rom_reads"]:
            self.assertEqual((row[5], row[6]), (0x018000, 0x018000))

    def test_the_frame_settles_on_the_value_at_the_load(self) -> None:
        doc = run_rounds(*LATER_WRITE)
        self.assertEqual(doc["rom_read_ranges"], [[0x8000, 2]])
        self.assertEqual([r[4] for r in doc["rom_reads"]], [1])
        self.assertEqual(doc["per_frame_unresolved"], [0])
        self.assertEqual((doc["resolution_conflict_bytes"], doc["resolutions_dropped"]), (0, 0))
        self.assertEqual(doc["resolution_rounds"], [[2, 1]])
        names = doc["addressing_names"]
        res = {(pc, names[ad], label): n for pc, ad, label, n in doc["resolutions"]}
        self.assertEqual(res, {(0x8000, "ildp", "end_of_frame"): 1, (0x8002, "ildp", "end_of_frame"): 1})

    def test_a_dropped_resolution_leaves_no_rom_read(self) -> None:
        with unittest.mock.patch.object(derive, "MAX_RESOLVE_ROUNDS", 1):
            doc = run_rounds(*LATER_WRITE)
        self.assertEqual(doc["rom_read_ranges"], [])
        self.assertEqual(doc["rom_reads"], [])
        self.assertEqual(doc["per_frame_unresolved"], [1])
        self.assertEqual(doc["resolutions_dropped"], 1)
        self.assertEqual(doc["resolution_rounds"], [[1, 1]])

    def test_a_discarded_round_leaves_no_register_store(self) -> None:
        # STA [$63] goes to $00:420B in the first round (end-of-frame pointer) and to $00:1000 at the store.
        program = [(0x8006, {"a": 0x01, "p": 0x20}), (0x8002, {"a": 0x420B})]
        doc = run_rounds(program, {0x63: bytes([0x00, 0x10, 0x00]), 0x80: bytes([0x63, 0x00, 0x7E])},
                         {0x63: bytes([0x0B, 0x42, 0x00]), 0x80: bytes([0x63, 0x00, 0x7E])})
        self.assertEqual(doc["dma_log"], [])
        rows = {(r["pc"], r["kind"], r["address"]): r for r in acmd.access_rows(doc)}
        self.assertEqual(rows[(0x8006, "write", 0x001000)]["values"], [0x01])
        self.assertNotIn((0x8006, "write", 0x00420B), rows)

    def test_an_rmw_of_a_resolved_store_settles(self) -> None:
        # PR #71 review finding 4: STA [$80] stores $9000 to the pointer, INC $63 makes it $9001, LDA [$63] reads $01:9001.
        program = [(0x8002, {"a": 0x9000}), (0x8004, {}), (0x8000, {})]
        sof = {0x63: bytes([0x00, 0x80, 0x01]), 0x80: bytes([0x63, 0x00, 0x7E])}
        eof = {0x63: bytes([0x01, 0x90, 0x01]), 0x80: bytes([0x63, 0x00, 0x7E])}
        doc = run_rounds(program, sof, eof, resolve_rmw=True)
        self.assertEqual(doc["rom_read_ranges"], [[0x9001, 2]])
        self.assertEqual(doc["per_frame_unresolved"], [0])
        self.assertEqual(doc["resolutions_dropped"], 0)
        with unittest.mock.patch.object(derive, "MAX_RESOLVE_ROUNDS", 1):
            doc = run_rounds(program, sof, eof, resolve_rmw=True)
        self.assertEqual(doc["rom_read_ranges"], [])                 # dropped: neither $01:8001 nor $01:9001
        self.assertEqual(doc["per_frame_unresolved"], [1])

    def test_frames_without_a_conflict_take_one_round(self) -> None:
        doc, _ = run_synthetic()
        self.assertEqual(doc["resolution_rounds"], [[1, 2]])


class WorkerAndCliTests(unittest.TestCase):
    def test_worker_rejects_bad_access_arguments(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            script = Path(tmp) / "s.json"
            script.write_text(json.dumps({"schema_version": 1, "frames": 10}))
            common = ["--core", str(Path(tmp) / "missing.dylib"), "--rom", str(Path(tmp) / "missing.sfc"), "--script", str(script),
                      "--samples-out", str(Path(tmp) / "out.json")]
            access = ["--access-out", str(Path(tmp) / "a.json")]
            self.assertEqual(worker.main(common + access + ["--access-ring", "0"]), EXIT_INVALID_INPUT)
            self.assertEqual(worker.main(common + access + ["--access-from-frame", "10"]), EXIT_INVALID_INPUT)
            self.assertEqual(worker.main(common + access + ["--access-from-frame", "5", "--access-to-frame", "4"]), EXIT_INVALID_INPUT)
            self.assertEqual(worker.main(common + access + ["--access-watch-address", "0x1000000"]), EXIT_INVALID_INPUT)
            self.assertEqual(worker.main(common + access + ["--wram-series-out", str(Path(tmp) / "w.bin")]), EXIT_INVALID_INPUT)
            self.assertEqual(worker.main(common + access + ["--wram-series-out", str(Path(tmp) / "w.bin"), "--wram-series-range", "0", "0x20001"]), EXIT_INVALID_INPUT)
            self.assertEqual(worker.main(common + ["--wram-series-out", str(Path(tmp) / "w.bin"), "--wram-series-range", "0", "16"]), EXIT_INVALID_INPUT)
            self.assertEqual(worker.main(common + access + ["--access-from-frame", "2", "--access-to-frame", "5"]), EXIT_MISSING_PREREQUISITE)

    def test_query_without_access_file_is_missing(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            r = run_cli("access", "query", "--access", str(Path(tmp) / "none.json"), "--address", "$7E:0313", "--report", str(Path(tmp) / "r.json"))
            self.assertEqual(r.returncode, EXIT_MISSING_PREREQUISITE, r.stderr)
            self.assertEqual([c["outcome"] for c in json.loads((Path(tmp) / "r.json").read_text())["checks"]], ["missing"])

    def test_query_invalid_arguments(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            doc, _ = run_synthetic()
            path = Path(tmp) / "access.json"
            path.write_text(json.dumps(doc))
            self.assertEqual(run_cli("access", "query", "--access", str(path)).returncode, EXIT_INVALID_INPUT)
            self.assertEqual(run_cli("access", "query", "--access", str(path), "--address", "$7E:0313", "--kind", "store").returncode, EXIT_INVALID_INPUT)
            self.assertEqual(run_cli("access", "query", "--access", str(path), "--address", "zz").returncode, EXIT_INVALID_INPUT)
            bad = Path(tmp) / "bad.json"
            bad.write_text(json.dumps({"schema_version": 1, "kind": "access_record"}))
            self.assertEqual(run_cli("access", "query", "--access", str(bad), "--address", "$7E:0313").returncode, EXIT_INVALID_INPUT)
            r = run_cli("access", "query", "--access", str(path), "--address", "$7E:1000", "--json")
            self.assertEqual(r.returncode, 0, r.stderr)
            self.assertEqual([w["pc"] for w in json.loads(r.stdout)["writers"]], [0x8000])
            self.assertEqual(run_cli("access", "query", "--access", str(path), "--address", "$7E:7777").returncode, 1)

    def test_capture_with_unreachable_manifest(self) -> None:
        out = ROOT / "artifacts" / "test-access-unreachable"
        r = run_cli("access", "capture", "--manifest", str(ROOT / "tests" / "manifests" / "replay" / "unreachable.json"), "--out", str(out), "--ring", "0")
        self.assertEqual(r.returncode, EXIT_INVALID_INPUT, r.stderr)
        r = run_cli("access", "capture", "--manifest", str(ROOT / "tests" / "manifests" / "replay" / "does-not-exist.json"), "--out", str(out))
        self.assertEqual(r.returncode, EXIT_INVALID_INPUT, r.stderr)
        r = run_cli("access", "capture", "--manifest", str(ROOT / "tests" / "manifests" / "replay" / "boot-start-600.json"), "--out", str(out),
                    "--from-frame", "600")
        self.assertEqual(r.returncode, EXIT_INVALID_INPUT, r.stderr)
        r = run_cli("access", "capture", "--manifest", str(ROOT / "tests" / "manifests" / "replay" / "boot-start-600.json"), "--out", str(out),
                    "--watch-address", "nope")
        self.assertEqual(r.returncode, EXIT_INVALID_INPUT, r.stderr)


if __name__ == "__main__":
    unittest.main()
