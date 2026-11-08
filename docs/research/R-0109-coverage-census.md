# R-0109 - How far the native game is from the whole original

Status: research result ([COVERAGE-CENSUS](../../tasks/COVERAGE-CENSUS.md)), 8 November 2026, on
main `dffbcf44`. PAL ROM SHA-256
`a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`, reference core bsnes
`7d5aa1e6` (patched, `tools/locks/emulators.json`, Strict). Evidence in main
`local/evidence/coverage-census/`. It extends [R-0089](R-0089-coverage-gaps.md) (the corpus and
the queue that SAVE-FILES through RACE-BRANCH-REMAINDER worked through) and
[R-0093](R-0093-data-coverage.md) (which data the original reads).

Tags: **[L]** read from the listing (`coverage disassemble` with the eight coverage inputs,
`listing/` in the evidence folder); **[C]** a capture of the original (this task's or a record's);
**[R]** stated by an accepted record; **[S]** a scan of the ROM's own data (the 45 track streams).

## Domain

- **The corpus.** R-0089's 236 cold-start runs (the static map's fifth input, R-0093) plus every
  run an accepted task added since, so the map counts what every record captured:
  - **Replay manifests** under `local/evidence/` not in R-0089's corpus: 149 distinct runs (same
    pads, frames and cartridge RAM writes counted once), from CREDITS-NAME, WIPE-RAM, TWO-HUMAN-STUNTS,
    LEAGUE-TOURS, MENU-INPUT, RECORD-SCREENS, NAME-CHECK, SPLIT-HUNTER-PICTURES,
    TWO-VIEW-POWER-BOUNCE, the two race tasks and their reviews. 20 OPTIONS manifests are among
    them: they are older than R-0089, but R-0089 keyed runs by pads and length only and so merged
    runs that differ only in their cartridge RAM writes.
  - **Among them, warm boots** (5): RECORD-SCREENS' and its review's `warm_boot` manifests and
    LEAGUE-TOURS review's `warm-mid`/`warm-edge`, whose saved images become writes after frame 0
    (below). Two more of that review's manifests marked `warm_start` have no image: cold starts.
  - **track_reference captures** (R-0089 left all of them out): 537 distinct runs, from the track
    sweeps, LOCKED-TOURS, HUNTER-EFFECTS, TILE-PAIRS, RACE-RIDERS-OPPONENTS and the other race
    tasks, 123 of them with a cartridge RAM preload (the unlocked tours, a medal, tutorial bits).
    DRAGSTER-ORDINARY-CONTROLS' 303 held-input runs went in two batches: one in ten first, the
    rest after that sample ran 8 instructions no other run did (`$82:96A9-96C0`).
    Each capture's controller timeline is a manifest; its preload, rebuilt with
    `track_reference.unlocked_sram` and checked against the recorded digest, is written after
    frame 0.
  - Runs whose coverage already existed are reused, not captured again: 32 (OPTIONS', the two race
    tasks' and others' own coverage captures) and 3 track_reference captures that equal R-0089
    runs.
- **Captured here:** 651 coverage captures (`coverage capture`), 3,284,171 frames, every one
  `status=passed`. Merged with the 32 reused captures (683 files) into `coverage/merged.json`, tracked as
  `docs/map/coverage-census.map.json`, the static map's eighth coverage input.
- **A preload written after frame 0 is the same run.** bsnes powers on with cartridge RAM all
  `$FF`; every image byte that is not `$FF` is written after frame 0. Every one of the 534
  track_reference runs (sampled every frame) equals its capture in work RAM and video on every
  frame the capture recorded (from frame 1100), 1,821,379 frames, 0 differences (`same_run.jsonl`)
  [C]. R-0100 showed the same for a warm boot's image from frame 1 [R].
- **Not in the corpus:** warm boots driven by scripts rather than manifests (SAVE-FILES' image
  sweeps, CARTRIDGE-OPTION-BITS' and PODIUM-AUDIO's warm routes, APP-LEAGUE-RACE's app runs); the
  code they run is the warm boot and the menus with a populated cartridge, which the warm
  manifests and OPTIONS' writes cover, but no capture of theirs is counted. The audio tasks'
  folders (MODE-AUDIO, DEMO-AUDIO, PODIUM-AUDIO) hold no replay manifest the scan did not already
  count.
- **Data reads** are R-0093's access record of R-0089's corpus, plus this task's access captures
  (R-0093's method: `access capture --resolve-rmw`, `extract.py`) of 118 of the new runs: the
  locked tours, tile pairs, league tours, warm boots, the preloaded races and one in six of
  HUNTER-EFFECTS' (583,943 frames).
- **The universe:** banks `$80-$83` (131,072 bytes), the static map's scope. The 60 data banks and
  the sound program are covered by R-0093's measure and the records (Outside the code banks).

## Findings

### The static map with the eighth input

| Class | Before (7 inputs) | After (8 inputs) |
| --- | ---: | ---: |
| observed | 88,343 | 94,637 |
| inferred | 11,231 | 5,311 |
| data | 73 | 70 |
| unknown | 31,425 | 31,054 |

Unknown share 24.0% before, 23.7% after [C].

The eighth input moves 6,294 bytes to observed: HUNTER's effects (`$83:D1CA-E081`, run by
HUNTER-EFFECTS' unlocked-tour captures; R-0089 counted them as never run), the WIPE RAM menu
(`$80:A9B4`, `$80:AA1B-AAF8`), the "credits" picture (`$83:FADC-FB40`), TRACK RECORDS' record line
(`$80:C269-C2ED`, RECORD-SCREENS' warm boots) and 127 smaller blocks [C]. 0 decoding disagreements; every tracked range tiles
(`coverage static-map` checks).

### Every byte that was unknown or inferred

Each of the 42,656 bytes the map classed unknown (31,425) or inferred (11,231) before this task is
in exactly one class (`census.py`, `census.json`; precedence in this order):

| Class | $80 | $81 | $82 | $83 | All | From unknown | From inferred |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| executed (the eighth input) | 1,240 | 261 | 221 | 4,572 | 6,294 | 308 | 5,986 |
| read as data by the original | 6,147 | 418 | 1,537 | 3,153 | 11,255 | 11,255 | 0 |
| fill (16+ equal bytes) | 856 | 3,351 | 7,431 | 972 | 12,610 | 12,610 | 0 |
| table: the rest of a read or operand-named table | 627 | 169 | 701 | 1,019 | 2,516 | 2,516 | 0 |
| inside a ROM range native cites | 272 | 1 | 31 | 403 | 707 | 707 | 0 |
| code shown unreachable | 7 | 966 | 64 | 0 | 1,037 | 63 | 974 |
| code reachable, never captured | 0 | 6 | 0 | 0 | 6 | 0 | 6 |
| code, reachability open (named) | 28 | 197 | 1,814 | 322 | 2,361 | 0 | 2,361 |
| unexplained: code never run (small arms) | 336 | 655 | 603 | 313 | 1,907 | 3 | 1,904 |
| unexplained: decodes as code (gap-sweep candidate) | 124 | 576 | 11 | 157 | 868 | 868 | 0 |
| unexplained: unknown bytes | 203 | 2,028 | 444 | 420 | 3,095 | 3,095 | 0 |
| All | 9,840 | 8,628 | 12,857 | 11,331 | 42,656 | 31,425 | 11,231 |

Largest blocks per class (`census.json` lists 40 per class):

- **executed (the eighth input)** (131 blocks): `$83:D3FC-E081` 3,206; `$80:AA1B-AAF8` 222; `$80:C269-C2ED` 133; `$83:D2C9-D342` 122; `$83:D345-D3BB` 119; `$83:D220-D289` 106
- **read as data by the original** (337 blocks): `$80:82AB-84CA` 544; `$83:8000-821D` 542; `$80:F2EA-F4B7` 462; `$80:8008-81A8` 417; `$83:9FFA-A17B` 386; `$82:B549-B6C4` 380
- **fill (16+ equal bytes)** (21 blocks): `$82:E35C-FFFF` 7,332; `$81:F358-FFFF` 3,240; `$80:FC6E-FFAF` 834; `$83:FD75-FFFF` 651; `$83:822E-82C9` 156; `$83:83F8-8471` 122
- **table: the rest of a read or operand-named table** (276 blocks): `$83:8330-83F7` 200 (anchor $83:82D4 read by the corpus, named by $83:938F); `$82:9EB3-9F2C` 122 (anchor $82:9EB0 read by the corpus); `$80:E1BA-E232` 121 (anchor $80:E04B read by the corpus, named by $80:DE1C, $80:DE1C); `$83:F516-F585` 112 (anchor $83:F516, named by $83:F731); `$82:9E4A-9EAF` 102 (anchor $82:9E47 read by the corpus); `$82:9F2F-9F84` 86 (anchor $82:9F2D read by the corpus)
- **inside a ROM range native cites** (17 blocks): `$80:AAF9-ABC7` 207; `$83:AA21-AAC4` 164; `$83:9920-9971` 82; `$83:A43F-A468` 42; `$83:A4A8-A4D0` 41; `$83:A9DD-A9FA` 30
- **code shown unreachable** (11 blocks): `$81:BB7A-BD6F` 502; `$81:BDF2-BEA3` 178; `$81:875C-87BE` 99; `$81:BD90-BDCF` 64; `$81:A388-A3C6` 63; `$82:B2FD-B322` 38
- **code reachable, never captured** (2 blocks): `$81:83C4-83C6` 3; `$81:83EC-83EE` 3
- **code, reachability open (named)** (10 blocks): `$82:BFC9-C53C` 1,396; `$82:CFF3-D194` 418; `$83:9C47-9D7F` 313; `$81:D255-D28B` 55; `$81:D6C0-D6E7` 40; `$81:DEBB-DEDC` 34
- **unexplained: code never run (small arms)** (286 blocks): `$82:B0F8-B153` 92; `$81:C9F4-CA37` 68; `$83:8B93-8BC8` 54; `$82:854F-8581` 51; `$83:AAF4-AB1B` 40; `$81:E721-E745` 37
- **unexplained: decodes as code (gap-sweep candidate)** (20 blocks): `$81:CB24-CCD2` 431; `$83:8971-89B9` 73; `$81:B704-B72E` 43; `$80:B855-B879` 37; `$81:B795-B7B9` 37; `$81:962C-964F` 36
- **unexplained: unknown bytes** (42 blocks): `$81:A7A7-AB78` 978; `$81:B3F0-B654` 613; `$82:816C-8293` 296; `$81:B7ED-B8E1` 245; `$83:8A59-8AF6` 158; `$82:B225-B295` 113

How each class is shown:
- **Executed:** observed with the eighth input [C].
- **Read as data:** the corpus reads the byte as data (CPU load, DMA or HDMA source) in R-0093's
  record or this task's access captures [C]. The two read 11,340 code-bank bytes as data (R-0093
  alone 10,983); 11,255 of them lie in bytes the map classed unknown, 64 in data bytes and 21 in
  observed code (an instruction's bytes also read as a table).
- **Fill:** a run of 16 or more equal bytes that is not code. The four bank tails (`$80:FC6E`,
  `$81:F358`, `$82:E35C`, `$83:FD75` to the bank ends, `$FF`) are 12,057 of the 12,610 [S].
- **Table:** not code, from an anchor to the next anchor or code: an anchor is an address an
  instruction names as an operand (absolute operands with the data bank the captures record at
  that pc, else the program bank; long operands) or the first byte of a run the corpus reads. The
  rest of a table whose reader the listing shows [L]. This over-counts where a table ends early
  and something else follows before the next anchor; the largest stretches are listed in
  `census.json`.
- **Inside a ROM range native cites:** native code cites the range as ROM data or code
  (`native-symbols.json` range ends): native reads or implements it, so it is accounted for
  even though no capture shows it [L].
- **Code shown unreachable** (each with its argument in `attrib.json`):
  - `$81:BB7A-BEA3`, the VRAM twin of the track unpacker, and `$82:B20B-B224`, the copier's branch
    to it: no call site asks the copier for a flagged entry [R: R-0093].
  - `$82:B2FD-B322`, `$82:B2DD`'s arm for an unflagged entry: its only caller (`$82:E152`) passes
    entry `$C2` plus the track, and all 45 track entries carry the flag [L, R: R-0093].
  - `$81:A388-A3C6`, the playfield shape `0x08` of `$81:A304`: byte 13 of the 45 unpacked track
    streams is `0x00` (4 tracks), `0x04` (1), `0x10` (4), `0x20` (11), `0x40` (22) or `0x80` (3),
    never `0x08` (`track_shapes.py`) [S]. Native implements it from the listing
    (`race_setup.cpp`'s `track_geometry`).
  - `$81:875C-87BF` (tile pair 4), `$81:8565-8582` (pair 8's clamp), `$81:9140-9160` (the map
    word swap) [R: R-0107]; `$81:83A6` (a loop entry with a loop pose) [R: R-0108].
  - The BRK and IRQ vectors (`$80:857F-8585`): the 16 stores to `$4200` write `$00`, `$01` or
    `$81`, never the H/V IRQ bits, and no capture executes BRK or COP [L, C].
- **Code reachable, never captured:** the loop descriptor facing away (`$81:83C4`, `$81:83EC`,
  6 bytes still inferred) [R: R-0108].
- **Code, reachability open** (named, not shown either way):
  - the rider sprite-tile upload queue's slots past `$1605` (`$82:BFC9-C53C`, and
    `$82:CFF3-D194` in the second uploader): a frame that queues more tiles than any capture; the
    queue's writer is `$83:F1F2-F2C9` (one entry per tile); its bound is not read here [L];
  - the HUD's fields of 10 and more (`$81:D255`, `$81:D6C0`, `$81:DEBB`, `$81:DFEC`, `$81:EC9A`;
    R-0043's two-digit lap fields) [L, R];
  - `$80:9075-9090`, the `$77:0749 = $FF` path (R-0058) [L, R];
  - `$83:9B70-9B78` and the routine it calls, `$83:9C47-9D7F`: `$83:9B49`'s arm for a set bit of
    `$77:02B2,X` (native cites `$83:9C47` in `menu_overruns.cpp`) [L].
- **Unexplained** (the measured remainder):
  - code never run that no record or argument attributes: 1,907 bytes in 286 blocks, almost all
    branch arms inside routines that run (the largest `$82:B0F8-B153` 92, `$81:C9F4-CA37` 68,
    `$83:8B93-8BC8` 54);
  - unknown bytes that decode as code (gap-sweep candidates) and nothing reaches statically:
    868 bytes (the largest `$81:CB24-CCD2`, 431, which begins `LDA $122B`);
  - unknown bytes that are none of the above: 3,095, the largest `$81:A7A7-AB78` (978, a table
    of rising words), `$81:B3F0-B654` (613, words in steps of 2), `$82:816C-8293` (296, code-like:
    `PHP; SEP #$20; LDA $6C`) and `$81:B7ED-B8E1` (245, code-like).

### Every routine

762 routines now (759 before; the eighth input's code adds 3) (`routines.py`, `routines.json`):

| Group | Routines | Bytes |
| --- | ---: | ---: |
| never run, cited | 3 | 475 |
| never run, not cited | 13 | 795 |
| run, cited | 581 | 73,703 |
| run, not cited, caller cited | 97 | 7,933 |
| run, not cited, no caller cited | 68 | 9,197 |

Run: 746 of 762 (by the seven earlier inputs 723); cited 584.

| Run but not cited, by what it writes | Routines | Bytes |
| --- | ---: | ---: |
| hardware registers only (PPU/DMA plumbing) | 16 | 5,113 |
| no absolute stores (helper on direct page or registers) | 32 | 1,219 |
| trampoline or stub (behaviour is its callee or caller) | 62 | 200 |
| writes state native cites | 44 | 9,611 |
| writes state native does not cite | 11 | 987 |

- **Run but not cited** (165 routines): method, every one classed by what its instructions write
  (absolute and long stores in the listing) against the state native code cites
  (`native-symbols.json`'s work-RAM and cartridge-RAM addresses):
  - trampolines and stubs of 4 bytes or fewer (a `JSR` and an `RTL`): the behaviour is the
    callee's;
  - routines that write only hardware registers (PPU, DMA): native draws and loads directly, by
    design;
  - routines that write nothing absolute: helpers on the direct page and registers (shifts and
    similar);
  - routines that write state native cites under another symbol: native keeps that state, and
    their callers are cited in most cases;
  - 11 routines (987 bytes) that write state native cites nowhere. Read one by one [L]: the
    scenery list loader (`$82:E19F`, `$82:E1DC`) and the sound upload (`$82:8082`), replaced by
    the pack and the native driver by design (R-0093, D-0009); race helpers called from cited
    routines (`$81:9479` and `$81:9610` from `$81:912B`, `$82:A288` from `$82:8C78`,
    `$83:F098`) whose words (`$0F31-$0F57`) sit in the rider block; `$83:ED7B` (shifts `$0F6D-$0F73`)
    and `$83:EDF9` (`$0F99`, `$0F9B` from bank `$17`); `$80:E6DD` (a byte of `$83:9B27` chosen by
    `$0191` into `$0A02-$0A0A`); `$81:D845` (clears `$11A3`, `$11A7`).
    Whether native implements each under its caller's citation is not verified here: the race
    rows the race tasks compare frame for frame do include rider state, but this census did not
    check which of these words a compared row holds.
- **Never run** (16): tile pair 4, the VRAM unpacker's ten routines, the shape `0x08` arm and the
  two vectors are unreachable (above); `$83:8B9D` (reached only through `$83:8B93`, an arm never
  taken) and `$83:9C47` are open.

### Outside the code banks

- **How much the original reads** (R-0093's record of R-0089's corpus plus this task's 118 access
  captures, `data_reads.py`): 1,679,396 of the ROM's 2,097,152 bytes (R-0093: 1,562,774). The
  data banks `$84-$BF`: 1,668,056 of 1,966,080 read (84.8%; R-0093 78.9%); unread padding (runs of
  32 or more `$00`/`$FF`) 90,902; unread other bytes 207,122 (R-0093: 322,406) [C].
- **The locked tours' track streams:** 15 of the 20 streams R-0093's corpus never unpacked are now
  read in full (tracks 7, 12, 17, 21, 26-29, 36-39, 42-44); tracks 24 and 31-34 are not read by
  the 118 runs accessed here (R-0098's league races tracks 32-34, but those runs have no access
  record) [C]. The pack has all 45 [R: R-0093].
- **Newly read regions** R-0093 listed as unread: 68 of them, 106,431 bytes, the largest the
  three stream bodies above (`$9C:AF60`, `$9E:8243`, `$9F:8CD7`), `$8F:D05F` (8,826),
  `$8F:84C0` (8,192), the credits picture `$8F:BAE8` and rider sprite tiles in `$B4`-`$B6` [C].
- **Still open from R-0093** (the subagent's reading of R-0094 to R-0108, `recorded-limits.md`
  "Data banks and tooling") [R]:
  - packed since: the demo's sound set and score block `$93:F712` (pack v39, R-0096), the credits
    picture `$BF-$C1` (v37, R-0094); `scenery.14` is NEON with one view, already native (R-0105);
  - not extracted by design: the sound uploads' driver bytes (`$93:C6B2`, `$93:D773`,
    `$93:D80F`, D-0009); whether the native driver needs data from them is open;
  - still unread and unexplained: entries `$47`, `$49`, `$4B`, `$4F`, `$51`, the palettes
    `$B7-$BE`, `$03`, `$04`, `$3D`, bank `$A2` (0 bytes read here too), `$94:957B`, `$94:9147`,
    `$97:D9F4`, scenery entries `$1B`/`$1C` (track 44) and `$00`, `$21`, `$23`.
- **Pack versions since R-0093:** v37 (CREDITS-NAME), v38 (TWO-HUMAN-STUNTS, VS REMATCH text), v39
  (DEMO-AUDIO), v40 (LEAGUE-TOURS, opponent voice classes), v41 (NAME-CHECK); TRACK RECORDS'
  assets 29 and 30 are not packed (R-0100: no picture differs) [R].
- **The sound program** (as the records give it; no new audio work here) [R]: native runs its own
  IPL, driver and DSP from the original's CPU port writes (D-0009); every DSP and SMP write and
  every raw stereo pair equals the original's on `demo-cold-full` (6,406,928 pairs, R-0096) and
  `tour-cues-full` (10,763,786, R-0099). Native audio covers the title and menus, one-player, 2P,
  VS, league, OPTIONS, WIPE RAM, the credits picture, the idle demos, the podium, TRACK RECORDS and
  HIGH SCORES (R-0075 to R-0077, R-0091, R-0092, R-0094, R-0096, R-0097, R-0099 to R-0101). Open:
  PCM after the first anchored command is measured, not exact (D-0010); the timing that follows
  the driver's state (The records' limits); the never-captured effects; a driver effect at or
  past a driver's count takes an unrecovered path native refuses (R-0077).

### The records' limits

`recorded-limits.md` (evidence) carries R-0089's 63 items forward and adds what every record since
states: 12 of R-0089's items closed and 15 partly closed by later records; 93 items added, 16 of
them since closed and 6 partly [R]. What stays open, by system:
- **Timing that follows the sound processor** (no model): the warm power-on's handshake lengths,
  short titles and the two long title starts, the podium's way back a frame late in about a third
  of podiums, held titles' slides, a race's loading varying by a frame (R-0077, R-0087, R-0088,
  R-0090, R-0096, R-0098, R-0099, R-0102).
- **CPU cycle timing** (no model): the multiply hit by HDMA (DOWNER in two views, track 41's
  landing), prints that run past their frame, the knife-edge way outs (R-0095, R-0098, R-0100,
  R-0102).
- **Picture residuals**: split pictures under the HUD, track 42's two-view NEON, line 113 under a
  mosaic, 41 demo sprite pictures, the GO letters, NOW PLAYING's animation and the result icons
  (R-0046, R-0071, R-0073, R-0083, R-0084, R-0088, R-0098, R-0103).
- **The saved image's last bytes**: `$074C-074F`, `$10AD`'s other values, `$10A7`'s write-back,
  `$0750`'s race words, `$10B1`'s and `$1115-1117`'s write timing, a save taken during a race
  (R-0090, R-0095, R-0100, R-0105).
- **Never-captured branches**: the stunt score of 1000, track 13's cell past its flag table, the
  loop descriptor facing away, the never-captured sounds (corkscrew ejection, the 9:5x warning,
  the cheat page reveal, the slow checkpoint, BRONSEN's praise), 2P/VS menus on the locked tours'
  tracks, a race tie, NEXT TRACK's wrap after track 44, demo cycles past the twentieth title
  (R-0077, R-0095, R-0096, R-0107, R-0108).

### The app against the runner

What the original does that the shipped app (`unirally`) does not, as the records state it and
`src/app/sdl_main.cpp` shows [R, L]:
- **Audio is opt-in.** The app is silent unless started with `--native-title-menu-audio`; with it,
  native audio covers every mode (R-0091, R-0096, R-0099) [L].
- **Saves are opt-in.** Without `--save-file PATH` nothing is kept between runs (R-0090) [L].
- **The idle demo's race takes the live pads, not the demo's script** (R-0106).
- **A one-player race after a notice is not run** (R-0106).
- **Short titles:** the app cannot predict which title loads a frame sooner, so after one its slide
  is a frame late until the next title (the runner takes `--short-demo-title`) (R-0088, R-0096).
- **Only cues and totals are compared**: the app reports no race state per frame (R-0106); its
  PCM is the anchored producer's, measured, not exact (D-0010, R-0077).
- Fixed since R-0089: the app's audio through the idle demos (R-0096) and LEAGUE's scripted
  second race (R-0106).
- Since [R-0110](R-0110-app-parity.md) (APP-PARITY): audio and saves by default, the idle demo's
  race and the race after a notice on the script, and per-frame rows and pictures from the app.

## What "100%" can mean

The code banks hold 131,072 bytes. Some of them can never be part of the game a player sees:
- **fill** (12,610) and **code shown unreachable** (1,037): nothing executes or reads them in the
  shipped game; a native game that leaves them out is complete;
- **data that is never read** cannot be shown to matter by any capture: an unread table is either
  unreachable or behind a branch no capture takes, and only the second is a gap.

So "100%" here means: every byte is either run or read by a capture of the original and matched
by native (observed and cited, or data native extracts), or shown unreachable or fill with an
argument. Against that, the **headline measure of what is left** is

**8,237 bytes, 6.3% of the code banks** (7.0% of the 117,425 that are not fill or unreachable
code): bytes that no capture of the original runs or reads, that no operand or native citation
names, and that no argument shows unreachable. Of them, 2,367 are code with a named feature whose
reachability is open (the sprite-tile queue's last slots alone 1,814), 1,907 small never-taken
branch arms of routines that run, 868 gap-sweep candidates and 3,095 unknown bytes. Beside the
bytes, the behaviour still to match is the records' open limits (`recorded-limits.md`: 51 of
R-0089's items and 77 added since), most of them timing that follows the sound processor or the
CPU's cycles, picture residuals and the saved image's last bytes; and 11 routines (987 bytes) that
run and write state native code cites nowhere.

By routine: 746 of 762 run; 581 run and are cited by native; 165 run and are not cited, of which
154 are plumbing, helpers, trampolines or write state native keeps; the 16 that never run are 14
unreachable and 2 open.

## The queue

In order; each is claimed when the one before it is integrated, unless the coordinator records a
reason to run one beside another.

1. **[APP-PARITY](../../tasks/APP-PARITY.md)** (tier 2; domain: the shipped app). The app plays what
   the runner plays: native audio and a save file by default, the idle demo's race on the demo's
   own inputs, the one-player race after a notice, and per-frame race rows so a scripted app run
   is compared with the runner on rows, cues and pictures. First because it is what a player runs.
2. **[SAVE-IMAGE-EXACT](../../tasks/SAVE-IMAGE-EXACT.md)** (tier 1; cartridge RAM). The saved image
   equal to the original's at every frame the original writes it: `$074C-074F`, `$10AD`, `$10A7`,
   `$0750`, the league totals a 2P or VS result writes, the write timing of `$10B1` and
   `$1115-1117`, a save during a race.
3. **[CENSUS-REMAINDER](../../tasks/CENSUS-REMAINDER.md)** (tier 3; the ROM's bytes). Access records
   for the rest of the corpus, and the unexplained code-bank blocks and R-0093's open data-bank
   entries read from the listing and reclassified with evidence, so the headline's remainder
   shrinks to what is really unknown.
4. **[OPEN-BRANCHES](../../tasks/OPEN-BRANCHES.md)** (tier 1; race and front end). Every branch whose
   reachability is open here or never captured in a record: the HUD's fields of 10 and more, the
   `$77:0749 = $FF` path, `$83:9B49`'s arm, the sprite-tile queue's last slots, the loop descriptor
   facing away, a stunt score of 1000, track 13's cell, 2P/VS on the locked tracks, a tie, NEXT
   TRACK's wrap, late demo cycles and the never-captured sounds: captured and compared, or shown
   unreachable.
5. **[SOUND-PROCESSOR-TIMING](../../tasks/SOUND-PROCESSOR-TIMING.md)** (tier 1; front-end and race
   timing). The waits that follow the sound processor (warm power-on, short and long titles,
   held slides, the podium's way back, race loading) taken from the native driver's state instead
   of measured lengths.
6. **[CPU-CYCLE-TIMING](../../tasks/CPU-CYCLE-TIMING.md)** (tier 1; race arithmetic and print timing).
   A cycle-cost model for the multiply hit by HDMA (DOWNER two-view, track 41) and the prints that
   run past their frame.
7. **[PICTURE-RESIDUALS](../../tasks/PICTURE-RESIDUALS.md)** (tier 2; presentation). The picture
   differences the records leave, by system.

Not worth a task (recorded in Limits): the unreachable code and fill above; the sound uploads'
driver bytes (D-0009 by design); the never-run small arms, until a task's captures reach them.

## Limits

- **Data reads of the new runs are partial.** Of the 651 runs captured here only 118 have an
  access record; the others' data reads are not counted, so some "table" and "unexplained" bytes
  may be read by runs this task did not access-capture.
- **The table rule over-counts.** A stretch from an anchor to the next anchor or code is taken
  as one table; where a short table is followed by unrelated bytes, those are counted as table.
- **Script-driven warm boots** are not in the corpus (Domain).
- **Run but not cited** is attributed from what the routines write, not from native's
  behaviour; the 11 routines writing uncited state are read, not checked against native.
- **Coverage is instruction coverage with branch outcomes**; data a routine computes and
  discards is not measured.
- **A preload after frame 0** equals the power-on preload on every recorded frame from 1100 of the
  534 track_reference runs; frames before 1100 (the boot and menus) are not recorded by those
  captures and are not compared.
- The census's classes for unknown bytes are a reading of the listing and the access record;
  "unexplained" is a measured remainder of those rules, not a claim that the bytes are unused.

## Reproduction

From a checkout of main (`local/` linked to the main checkout's), `E=local/evidence/coverage-census`:

```
python3 $E/scan_manifests.py $E/scan.json
python3 $E/make_corpus.py                                  # manifests/, plan.json
local/locks/heavy-run.sh COVERAGE-CENSUS $E/captures.sh 10  # about 1 h 05 min for 3.28 M frames
local/locks/heavy-run.sh COVERAGE-CENSUS $E/access.sh 10    # the 118 access captures
bash $E/merge.sh                                           # coverage/merged.json, docs/map/coverage-census.map.json
bash $E/regen.sh                                           # native-symbols and the static map, eight inputs
python3 tools/project.py coverage disassemble --out $E/listing --coverage <the eight raw captures of regen.sh>
python3 $E/codebank_reads.py; python3 $E/data_reads.py; python3 $E/track_shapes.py
python3 $E/census.py local/evidence/race-branch-remainder/listing-after $E/listing $E/census.json
python3 $E/routines.py $E/listing $E/routines.json
python3 $E/report.py
```
