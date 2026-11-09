# R-0112 - The bytes R-0109 left unexplained

Status: research result ([CENSUS-REMAINDER](../../tasks/CENSUS-REMAINDER.md)), 9 November 2026, on main
`2a38ec3e`. PAL ROM SHA-256 `a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`,
reference core bsnes `7d5aa1e6` (patched, `tools/locks/emulators.json`, Strict). Evidence in main
`local/evidence/census-remainder/`. It extends [R-0109](R-0109-coverage-census.md) (the census and
its headline) and [R-0093](R-0093-data-coverage.md) (the access record and the data banks' open
entries).

Tags: **[L]** read from the listing (`coverage disassemble` with the eight coverage inputs,
`coverage-census/listing/`); **[C]** an access record of the original (R-0093's, R-0109's or this
task's); **[R]** stated by an accepted record; **[S]** a scan of the ROM's own data.

## Domain

- **The corpus** is R-0109's: R-0089's 236 runs, the 651 runs COVERAGE-CENSUS captured (the static
  map's eighth input) and the 32 whose existing coverage it reused (3 more equal R-0089 runs).
  Nothing new is captured for coverage.
- **Access records for all of it.** R-0093 recorded the 236; R-0109 recorded 118 of the 651. This
  task records the other 533 with R-0093's method (`access capture --resolve-rmw`, reduced by
  `data-coverage/extract.py`, the raw record deleted after; `access.sh`): 533 runs, 2,700,228
  frames, every one `status=passed`. R-0109 also reused existing coverage for 32 runs (OPTIONS,
  WIPE-RAM, CREDITS-NAME and the race tasks' own captures) that no access record covered; this
  task records them too (`access-reused.sh`): 32 runs, 96,882 frames, all `status=passed`. With
  R-0093's 236 and R-0109's 118, all 919 runs of the corpus have an access record.
- **The universe** is R-0109's: the 42,656 bytes of banks `$80-$83` the static map classed
  unknown or inferred before the eighth input, each in one class. The data banks `$84-$BF`
  (1,966,080 bytes) are measured as R-0093 did.

## Findings

### The code banks, R-0109 and now

`census.py` is R-0109's with two changes: the reads are all 919 access records
(`codebank_reads.py`), and this task's readings (`attrib.json`, written by `make_attrib.py`) may
attribute unknown bytes as well as inferred ones. One class is new:

- **Unreferenced:** bytes that decode as code or data that no instruction operand, 16-bit
  immediate (or immediate plus one, for an `RTS` dispatch), `PEA`, little-endian word in the same
  bank or 24-bit pointer anywhere in the ROM names, and that nothing falls into (they follow an
  unconditional transfer or a routine's end). Read block by block (`refs.py`, `inrefs.py`). An
  immediate is matched in all four code banks; the matches found in other banks are those banks'
  text, menu-table and DMA source pointers (for example `$80:9CAC LDX #$9E29` before the printer
  `JSR $C3BC`), not code addresses. Words inside tables that equal an address in an unreferenced
  block (`table_words.py`, 164 matches) all lie in glyph strings, sprite or directory records
  or inside the block itself; none is a jump-table entry.

| Class | R-0109 | Now | $80 | $81 | $82 | $83 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| executed (the eighth input) | 6,294 | 6,294 | 1,240 | 261 | 221 | 4,572 |
| read as data by the original | 11,255 | 11,665 | 6,433 | 422 | 1,576 | 3,234 |
| fill (16+ equal bytes) | 12,610 | 12,606 | 856 | 3,347 | 7,431 | 972 |
| table: the rest of a read or operand-named table | 2,516 | 2,364 | 595 | 169 | 662 | 938 |
| inside a ROM range native cites | 707 | 481 | 46 | 1 | 31 | 403 |
| code shown unreachable | 1,037 | 1,555 | 57 | 1,418 | 76 | 4 |
| unreferenced (new) | 0 | 3,415 | 215 | 2,170 | 453 | 577 |
| code reachable, never captured | 6 | 6 | 0 | 6 | 0 | 0 |
| code, reachability open (named) | 2,361 | 2,361 | 28 | 197 | 1,814 | 322 |
| unexplained: code never run (small arms) | 1,907 | 1,875 | 336 | 637 | 593 | 309 |
| unexplained: decodes as code (gap-sweep candidate) | 868 | 34 | 34 | 0 | 0 | 0 |
| unexplained: unknown bytes | 3,095 | 0 | 0 | 0 | 0 | 0 |
| All | 42,656 | 42,656 | 9,840 | 8,628 | 12,857 | 11,331 |

Where the bytes went (`census.json` of both tasks, byte by byte): R-0109's unknown bytes to
unreferenced 3,012, unreachable 55 and read 28; its gap-sweep candidates to unreachable 431 and
unreferenced 403 (34 left); its small arms to unreachable 32 (1,875 left); and 382 bytes R-0109
classed table (152), native-cited (226) or fill (4) are now read, which takes precedence.

- **Read as data** rises by 410 bytes with the 565 new access records [C]: 382 of them were
  already accounted for (table, native-cited, fill); 28 were unknown, the WIPE RAM menu's pointer
  pair and text (`$80:A9FA`, `$80:A9FF-AA16`, named by `$80:A9C4 LDX #$A9FA` for `$80:EF4D`),
  read by WIPE-RAM's own runs, which R-0109 had reused without an access record. The 533 runs
  read no byte R-0109 left unexplained.
- **Code shown unreachable** rises by 518 bytes. Each with its argument (`attrib.json`):
  - `$81:C691-C693` and `$81:CB24-CCD2` (434 bytes): `$81:C691` is a `JSR $CB24` that the
    `BRA $81:C694` at `$81:C68F` always jumps over, and nothing else names `$81:C691`; the routine
    at `$81:CB24` (it begins `LDA $122B`) is named only by that `JSR` (its operand, the word at
    `$81:C692`) [L].
  - `$83:8B18-8B1B` and `$80:94EB-951B` (53 bytes): the cartridge RAM size check `$83:8AFA-8B16`
    writes `$12` to `$77:0000` and the word `$3456` to `$77:1FFF`, then expects `$34` back at
    `$77:0000`. That holds when the cartridge RAM is 8 KiB and mirrors at `$77:2000`, which is what
    the header declares (`$80:FFD8` = 3). Only on other hardware does the `JML $8094EB` run: the
    GAME PAK NOT FOUND screen (`LDX #$9505; JSR $C3BC`, text `game_pak_not_found` at `$80:9505`)
    [L]. Every capture passes the check [C].
  - `$81:B908-B90C` and `$81:B91A-B920`: the track unpacker's pointer carrying into the next bank
    while it skips the 18-byte header. The first runs only for a stream that starts at `$xx:FFEF`
    or later, the second only for one that starts at `$xx:FFEE`. The 45 streams (asset entries
    `$C2-$EE`, the only ones `$82:E152` passes) start no later than `$9B:FF01`; seven cross a bank
    end later in the body, which the read-ahead `$81:BB60` handles (`track_streams.py`) [S].
  - `$82:82D9-82DD` and `$82:82E1-82E5`: the sample upload's length word starting at `$xx:FFFF` or
    `$xx:FFFE`. `$82:812A` finds entry X of the sample chain by walking it from `$90:8000` (each
    entry begins with its length). The chain has 69 entries in ROM (indices 69 and on start below
    `$8000`), and none of them has its header at a bank end (`sample_chain.py`) [S].
  - `$82:E19D-E19E`: a `BRK` when the unpacked track's word `$000B` (the scenery list's offset) is
    0; it is 33,807 or more on all 45 tracks (`track_streams.py`) [S].
  - `$80:85A3`: the IRQ handler (an `RTI`) that `$82:D6ED` installs in `[$73]` for the IRQ vector's
    `JML [$0073]`; no store to `$4200` enables an IRQ (R-0109's vector argument) [L, R].
  - `$81:8583`, `$81:9161-9162`, `$81:A337-A339`: the last bytes of code R-0107 and R-0109 already
    show unreachable (pair 8's clamp, the map word swap) and the `JMP` into the playfield shape
    `0x08` arm (no track's byte 13 is `0x08`) [R, S].
- **Unreferenced** (3,415 bytes in 44 blocks), by kind (`attrib.json` has each with
  what it is):
  - far-call veneers (`JSR x; RTL`) that no `JSL` names: `$80:A096`, `$80:A8A4`, `$80:D20A`,
    `$80:F55B`, `$81:84AD`, `$81:B768`, `$81:B7BA`, `$83:91CC`, and the ones in front of the
    routines below;
  - whole routines with no caller: the 978-byte `$81:A7A7-AB78` (a 17-byte index table and a
    16-way dispatcher on `$0200` and `$0D73,X`, `JMP ($A7E7,X)`), `$81:B3F0-B654` (613 bytes: a
    512-word tile index table and its only reader `$81:B5F4`, which writes `$7E:2132,X`),
    `$82:816C-8293` (296 bytes of the sound upload's family), `$81:B7ED-B8E1` (a 32-bit integer
    square root and a signed multiply), `$82:B225-B295` (a run-length VRAM decoder), three
    cartridge RAM checksum checks (`$83:89D9-8AF6`: `$77:000C-016B` against `$77:016C`,
    `$77:0230-02AF` against `$77:02B0`, and a third from `$77:02B2`), two brightness fades
    (`$81:B704-B75D`), three of the seven pad-button tests at `$80:B830-B93A` (`$80:B855-B8C3`),
    a second copy of the NMI entry (`$80:B0DC-B0EF`), and smaller helpers;
  - fragments after an unconditional transfer: `$80:B93B`, `$80:BFEE`, `$81:946A`, `$81:9693`,
    `$81:9963`, `$82:97ED`, `$82:AE7E`, `$83:8535`, `$83:87E9`, `$83:A4D1`, `$83:EC14`,
    `$83:EDAB`, and the NMI's unused register-restoring tail `$80:859B-85A2`;
  - data: the bit-mask table `$80:8000-8007` (no operand in any bank names it), the text `_tour`
    at `$80:EF37` and four bytes at `$80:EA31`, each after an `RTS`.

### The small arms, grouped by what guards them

`arms.py` finds, for each of R-0109's 286 never-run arms, the observed branch that never goes to it
(its outcomes in the coverage pairs, `pairs.json`) and the instruction that sets that branch's
flags, and groups them; `arms.md` lists every block with its guard and verdict. Eight blocks
(32 bytes) move to code shown unreachable; 278 (1,875 bytes) stay.

| Group (the guard) | Blocks | Bytes | Verdict |
| --- | ---: | ---: | --- |
| state-test (a load or arithmetic tested for zero or sign) | 61 | 565 | open |
| value-test (a compare with an immediate, not a clamp) | 49 | 435 | 2 unreachable (7 bytes), 47 open |
| queue-terminator (the sprite-tile uploaders' stop) | 79 | 237 | open, feature named |
| clamp (a compare, then a constant stored) | 38 | 202 | open |
| flag-bit (`BIT #`/`AND #`) | 19 | 111 | open; 3 with a feature named |
| compare-state (a compare with memory) | 10 | 108 | open |
| after-transfer (after a call that never returned) | 3 | 86 | open |
| call-result (a callee's flags or a shifted-out bit) | 11 | 75 | open |
| overflow (`BVC` after `ADC`) | 2 | 24 | open |
| hud-digits (`CPY #$000A`) | 5 | 24 | open, feature named |
| bank-crossing (a pointer carrying into the next bank) | 4 | 22 | unreachable [S] |
| sign-extension (`BIT #$0080`, then `ORA #$FF00`) | 3 | 15 | open, feature named |
| nested (inside code shown unreachable) | 2 | 3 | unreachable [R] |
| All | 286 | 1,907 | 8 unreachable (32 bytes), 278 open (1,875) |

- **Unreachable** (moved to the class above): the four bank-crossing arms and the shape `0x08`
  `JMP`, the size check's `JML` and the tails of R-0107's code (above).
- **Open, the feature named:**
  - the sprite-tile uploaders' stop (79 blocks, 237 bytes): `$82:B8E3-BF76` and `$82:C53E-CFC7`
    stop at the first slot whose bank byte is negative, the end mark `$FFFF` the writer
    `$83:F1F2-F2C9` stores after the last tile ($83:F2C6). No capture ended the queue at slots
    `$15AB-$15C9` of the first uploader or `$15AB-$15D3` of the second: every frame queued at least
    16 (21) tiles. The writer queues one tile per set bit of the pose's part masks; whether a frame
    can queue fewer is not bounded here. This is the other end of OPEN-BRANCHES' "last slots";
  - HUD numbers of 10 and more (5 blocks: `$81:D218`, `$81:D692`, `$81:DE90`, `$81:DFC1`,
    `$81:EC6F`): the same feature as R-0109's HUD fields (OPEN-BRANCHES);
  - two views in HUNTER's effects (`$83:CEF3`, `$83:D1D4`, `$83:D343`, guarded by `$77:0750` bit 3):
    2P or VS on HUNTER's tracks, which belong to the locked tours (OPEN-BRANCHES' 2P and VS on the
    locked tracks);
  - a rider pose of 4096 or more (`$81:9E2B`): the pose's collision record (`$0F85` times 8) carries
    into bank `$A2` (below);
  - the track menu's signed offsets (`$80:BA7C`, `$80:BB1E`, `$80:BB72`, sign-extension of a byte
    of the menu's table at `$AE`): three of the eight tables (`$80:9F74`, `$80:B6CA`, `$80:D5E1`)
    hold bytes with bit 7 set, so these may be reachable at a cursor position no capture chose; the
    tables' layout is not read here.
- **Open** (the other 187 blocks): each guard tests a state, value, flag or callee result no
  capture reaches; `arms.md` gives the guard of each. They are not read further here.

### What is still unexplained in the code banks

- **Gap-sweep candidates** (34 bytes): `$80:C545-C556`, `$80:C5CB-C5D2` and `$80:C620-C627`, the
  text printer's handlers for control codes `$FA` (print a string by pointer), `$F5` and `$F4`, which its dispatch table
  `$80:C3DC` names. They run when a printed string contains the code; no captured string does, and
  this task did not scan every string.
- **Unknown bytes**: none.
- **Small arms**: 1,875 bytes in 278 blocks (above).

### The data banks

- **How much the original reads** (all 919 access records, `data_reads.py`): 1,716,438 of the
  ROM's 2,097,152 bytes (R-0109: 1,679,396). The data banks `$84-$BF`: 1,704,685 of 1,966,080 read
  (86.7%; R-0109 84.8%); unread padding 90,945, unread other bytes 170,450 (on R-0109's records the
  same rule gives 90,946 and 207,078) [C].
- **Tracks 24 and 31-34** are now read in full but for the 15 header bytes the unpacker skips (from
  2, 2, 0, 0 and 0 bytes): the league races and locked tours among the 533 runs load them [C]. So
  all 45 track streams are read.

R-0093's open entries, as R-0109 lists them:

| Entry | Bytes | Read now | Verdict |
| --- | ---: | ---: | --- |
| scenery `$1B`, `$1C` (track 44) | 2,944 | 2,944 | read, already by R-0109's 118 runs (its list carried them over) [C] |
| scenery `$00`, `$21`, `$23` | 384 | 0 | unused: no track's scenery list names them [S, R-0093] |
| asset `$47`, `$49`, `$4B`, `$4F`, `$51` | 13,664 | 0 | unreachable under the index ranges (below) [L, R] |
| asset `$B7-$BE` (palettes) | 256 | 0 | as `$47` |
| asset `$03`, `$04`, `$3D` (palettes) | 96 | 0 | as `$47` |
| `$94:9147`, `$94:957B` | 3,064 | 0 | unused: sample chain entries 59 and 61, which nothing names [L] |
| `$97:D9F4` | 896 | 0 | unreachable: past the last record its readers can index [L] |
| bank `$A2` | 32,768 | 0 | open: read only for a rider pose of 4096 or more |

- **The scenery entries.** The scenery directory `$82:B7DD` is `[$4F]` only between `$82:E17A` and
  `$82:E352`, and the only lookup in between is `$82:E1BB JSL $82B2AC` with a byte of the unpacked
  track's scenery list [L]. No track's list names `$00`, `$21` or `$23` (R-0093's
  `track_assets.json`, all 45 tracks unpacked) [S].
- **The asset entries.** The directory `$82:B332` is read only by `$82:B2B0`, and it is reached from
  245 call sites, all of them run (`asset_calls.py`): 96 distinct constants, none of them `$03`, `$04`, `$3D`,
  `$47`, `$49`, `$4B`, `$4F`, `$51` or `$B7-$BE`, and computed indices of seven forms: `$06` plus a
  rider number (`$017D`, `$017F`, `$0748`, `$0749`, `$CA` and others; some masked to their low
  nibble), `$26` plus `$017D` or `$017F`, `$1F` or `$54` plus the medal byte `$77:069C,X`, `$93`
  plus `$0230`, `$70` or `$82` plus a value mod 14 (R-0093), and `$C2` plus the track. With the
  values the records and the listing give them (riders 0-7, `$0749` also `$FF`, `$017F` up to 0x10,
  low nibbles 0-15, the medal byte 0-3 by its writers `$83:8827-8832`, `$83:9454`, `$83:9C2A`,
  `$83:9C3D`, `$0230` 0-14, tracks 0-44), the computed indices are `$05-$16`, `$26-$36`, `$1F-$22`,
  `$54-$57`, `$70-$7E`, `$82-$90`, `$93-$A1` and `$C2-$EE`: none of the open entries [L, R]. No run
  of the 919 reads their directory records [C]. The ranges are the records' readings, not bounds
  proved here (Limits).
- **The two `$94` regions.** `$82:812A` finds sound item X by walking the chain of length-prefixed
  items from `$90:8000` (`sample_chain.py`); `$94:9141` and `$94:9575` are items 59 and 61, and
  R-0093's regions are their bodies after a 6-byte header. Items are loaded only by `$82:807E` (36
  call sites, each with a constant X) and by `$82:82A5` (12 call sites, each with a constant
  64-byte list at `$83:FBF5`, `$83:FC35`, `$83:FC75`, `$83:FCF5` or `$83:FD35`); no index and no list
  names 59 or 61 (`sound_uploads.py`) [L].
- **`$97:D9F4`.** It is 896 of a 1,557-byte run of spaces (`$97:D75F-DD73`) after the stunt-name
  table at `$97:CA04`. Its three readers (`$81:BF5E`, `$81:BFE9`, `$81:C05F`, the only
  operands naming it) read the 16-byte record at `$17:CA04 + 16 (n - 1)` with `n` a byte
  (`AND #$00FF`), so they never reach past `$97:D9F3`; the
  corpus reads 17 bytes of the run, all before `$97:D9F4` [L, C].
- **Bank `$A2`.** `$81:9E1B` indexes the collision records at `$21:8000 + 8 x pose` (`$0F85`, set
  from `$0411`/`$0413`), carrying into the next bank (`$81:9E2B`, a small arm never run) for poses
  of 4096 and more. The highest pose any run reads is 2,795 (`$A1:D758-D75F`), so bank `$A1` from
  `$A1:D760` is unread too [C]. Which poses the riders' animations can reach is not bounded here;
  open.

### The headline

R-0109's measure, counted the same way: the bytes that no capture of the original runs or reads,
that no operand or native citation names, and that no argument shows unreachable, are now

**4,276 bytes, 3.3% of the code banks** (3.8% of the 113,496 that are not fill, unreachable or
unreferenced), down from 8,237 (6.3%). They are the 2,361 bytes of named code whose reachability
is open (OPEN-BRANCHES), the 6 bytes of the loop descriptor facing away (reachable, never captured),
the 1,875 bytes of small arms in 278 blocks and the 34 bytes of the printer's three handlers.

If "unreferenced" is not accepted as shown unused, the figure is **7,691 bytes, 5.9%** (6.6% of the
116,911 that are not fill or unreachable code). Either way no unknown byte is left without a
reading.

Outside the code banks, the data banks' unexplained bytes are bank `$A2` and the end of bank
`$A1` (the poses past 2,795, open); every other R-0093 entry is read, unused or unreachable under
the stated ranges.

## Limits

- **Unreferenced is an argument, not a proof.** It assumes every computed transfer takes its
  target from a constant in the ROM (an immediate, a table word or a long pointer); a target
  computed by arithmetic that lands inside one of these blocks would not be found. The listing's
  computed transfers (`JMP ($005D)` and similar, `JML [$0053]`, `JMP ($xxxx,X)`, `JSR ($xxxx,X)`)
  take their targets from tables and immediates. References from inside other undecoded blocks
  are not decoded (`inrefs.py` reads decoded instructions only); every such block is itself
  unreferenced here.
- **The small arms' groups are a reading of the guard**, not of the state behind it: "open" means
  the state was not bounded here. No coverage was captured to chase them (OPEN-BRANCHES' work).
- **The data-bank arguments rest on value ranges** the records give (riders 0-7, medals 0-3,
  `$0230` 0-14), not ranges proved here.
- **The padding split** follows R-0093's rule (runs of 32 or more equal `$00` or `$FF` over the whole
  ROM, `aggregate.py`); on R-0109's records it gives 90,946 unread padding bytes where R-0109 quotes
  90,902, so the split between padding and other bytes differs from R-0109's by 44 bytes.
- **"Code, reachability open (named)"** (2,361 bytes) is left as R-0109 gives it (OPEN-BRANCHES).

## Reproduction

From a checkout of main (`local/` linked to the main checkout's), `E=local/evidence/census-remainder`:

```
zstd -dc local/evidence/coverage-census/manifests.tar.zst | tar -x -C $E   # or coverage-census/make_corpus.py
local/locks/heavy-run.sh CENSUS-REMAINDER $E/access.sh 10   # the 533 access captures, 2 h 43 min
local/locks/heavy-run.sh CENSUS-REMAINDER $E/access-reused.sh 10   # the 32 reused runs, 7 min
python3 $E/pairs.py                                         # observed transfers of the eight inputs
python3 $E/codebank_reads.py; python3 $E/make_attrib.py
python3 $E/census.py local/evidence/race-branch-remainder/listing-after local/evidence/coverage-census/listing $E/census.json
python3 $E/blocks.py local/evidence/coverage-census/census.json local/evidence/coverage-census/listing $E/blocks.json
python3 $E/inrefs.py $E/blocks.json local/evidence/coverage-census/listing $E/inrefs.json
python3 $E/arms.py; python3 $E/table_words.py
python3 $E/track_streams.py; python3 $E/sample_chain.py; python3 $E/sound_uploads.py; python3 $E/asset_calls.py
python3 $E/data_reads.py
python3 $E/report.py > $E/report.md
bash $E/regen.sh                                            # native-symbols and the static map, eight inputs
```

`show.py`, `dis.py` and `refs.py` print a listing range, decode ROM bytes and list the references
to an address; the readings in `attrib.json` cite what they showed.
