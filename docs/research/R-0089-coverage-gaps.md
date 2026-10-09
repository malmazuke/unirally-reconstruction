# R-0089 - What the original still does that native does not

Status: research result ([COVERAGE-GAPS](../../tasks/COVERAGE-GAPS.md)), 9 October 2026, on main
`1ad7ba4`. PAL ROM SHA-256 `a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`,
reference core bsnes `7d5aa1e6` (patched, `tools/locks/emulators.json`). Evidence in main
`local/evidence/coverage-gaps/`.

Tags: **[L]** listing only, **[C]** confirmed in a capture, **[R]** stated by an accepted record.

## Domain

- **The corpus.** Every distinct cold-start replay manifest under `local/evidence/` (same pads and
  length counted once): 237, the captures behind every accepted task from the boot to the idle
  demo rotation. Each was run again through `coverage capture` (instruction coverage, one sample
  at the end): 236 complete, 1,666,993 frames. The one left out is `audio-decision`'s
  deliberately invalid manifest. Four `m4-02-review` withheld runs fail their stored sample
  digest only because the resampled manifest samples differently; their coverage is complete.
  Five league manifests press buttons past their last frame; those presses are dropped.
- **Not in the corpus.** Captures made by `track_reference capture` (the track sweeps, locked
  tours, HUNTER-EFFECTS' held runs) preload cartridge RAM before power-on, which a replay
  manifest cannot express, so the race engine on unlocked tours is under-counted here. Native
  code that cites routines the corpus never runs is recovered from those captures or from the
  listing.
- **The listing.** `coverage disassemble` of main with the static map's four raw captures
  (`artifacts/listing`, regenerated): 32,066 code instructions (observed or inferred), 78,099
  bytes, in banks `$80-$83`; 636 routines. The 60 data banks and the sound program are outside
  this measure.

## Findings

- **Coverage of the known code.** The corpus executes 27,360 of the 32,066 instructions, 66,948
  of 78,099 bytes (85.7%), and 600 of the 636 routines (426 cited by native code, 174 not). [C]
- **More code than the map knows.** 6,353 executed instructions lie in bytes the static map
  classes as unknown (its unknown share is 40.4%, R-0045): the corpus finds code the four
  original map captures never ran. A static map regenerated from these captures would shrink the
  unknown share; it is not done here. [C]
- **What is never executed.** 11,151 bytes in 369 blocks; native cites 4,401 of them. By kind:
  - *Unrolled loop tails* (1,814 bytes): `$82:B8AE` and `$82:C53D` upload sprite tiles
    one slot after another until a negative entry; the corpus never fills the last slots
    (`$82:BFC9-C53C`, 1,396 bytes; `$82:CFF3-D194`, 418). Not a feature. [L, C]
  - *HUNTER's effects* (`$83:D1CA-E081`, about 4,000 bytes, cited): run by HUNTER-EFFECTS'
    unlocked-tour captures, not by this corpus. Native implements them (R-0052). [R]
  - *A second graphics format*: the copier `$82:B1DB` takes a directory entry whose bank byte
    has bit 7 set to `$81:BB7A-BEA3` (about 750 bytes). Bank `$18` begins `RNC 01` (Rob Northen
    compression); directory entries `$C2-$EE` (45) at `$82:B332` carry the bit and point into
    banks `$18-$1F`. No code constant names them and the corpus never runs the unpacker.
    Whether any path loads them is open. [L, C]
  - *The "credits" name cheat* (R-0060): `$82:DDDE` tests `$0545` and calls `$83:FADC`, which
    loads entries `$BF-$C1` and holds the picture 500 frames (`$83:FB31`). Never captured. [L]
  - *The WIPE RAM menu* (R-0054): `$80:A9B4` and its menu `$80:B93C`; the code at `$80:AAFA`
    prints "this option will reset your game pak's memory to its factory default... you will
    lose all your records". Native recognises the pad code and stops. [L]
  - *HUD branches never shown*: lap and split fields drawn for `$FFFF` or `$1253 < 1`
    (`$81:D8D9`, `$81:E94F`, `$81:DB70`, blocks in `$81:D936`, `$81:DA4F`, `$81:DCE6`,
    `$81:E9AC`), stunt scores of 100 and more (`$81:C3DC`). Partly cited. [L]
  - *Tile behaviours* in the table at `$81:82F0` no capture reaches (`$81:875C`, `$81:84B2`,
    `$81:831D`, `$81:83E7`, `$81:8565`; tile pair 4 is R-0051's open case). [L]
  - Small branches (310 blocks under 30 bytes, 1,825 bytes) and the BRK/COP vectors. [L]
- **What the records say is missing** (`recorded-limits.md`, about 75 items from every accepted
  record). The whole features: native audio stops in 2P, VS, league, OPTIONS and the idle demos
  (R-0077); a stunt event with two humans is refused (R-0066-68, R-0082); league is covered for
  the first CRAWLER tour only (R-0073); HUNTER's cartridge option bit 3 ("invisible unis") and
  the `$12D1` palette mode (R-0052) and the paths behind `$77:0742` bits 1, 4, 9 and 10 (R-0054)
  are not recovered. [R]
- **No saves.** The app never writes cartridge RAM: records, players, leagues and unlocked tours
  are lost when it quits (`src/app/sdl_main.cpp` writes only audio cue logs). Whole-SRAM equality
  is not claimed (R-0072): the record checksum `$77:054E` and the score history `$77:0618` are
  not kept natively. [L, R]

## The queue

In order. Each is claimed when the one before it is integrated, unless the coordinator records a
reason to run one beside another.

1. **SAVE-FILES** (tier 1): the cartridge RAM as one native image, kept equal to the original's
   at its save points (checksums and score history included), written to disk when the original
   writes and loaded at boot; a file from the original loads too.
2. **MODE-AUDIO** (tier 1): native audio in 2P, VS, league, OPTIONS and the idle demos, captured
   as R-0077 did for one-player.
3. **WIPE-RAM** (tier 1): the Left+A+L+R menu, its warning, and the factory reset (needs
   SAVE-FILES' whole image).
4. **CREDITS-NAME** (tier 1): the "credits" first rider: the 500-frame picture, the name
   becoming `mike____`, the race ending as a restart.
5. **TWO-HUMAN-STUNTS** (tier 1): stunt events in 2P and VS, their result and the second
   rider's pass.
6. **LEAGUE-TOURS** (tier 1): league beyond the first CRAWLER tour.
7. **CARTRIDGE-OPTION-BITS** (tier 1): what sets `$77:0742`'s bits and HUNTER's option bit 3,
   and their effects ("invisible unis", the `$12D1` palette mode, the pad and NMI gates).
8. **RACE-LISTING-BRANCHES** (tier 1): the race branches known only from the listing (the HUD
   fields above, tile pair 4 and the unreached tile behaviours, scores of 1000), captured with
   preloaded cartridge RAM or a state origin.
9. **DATA-COVERAGE** (tier 3): which bytes of the 60 data banks the corpus reads, the RNC entries,
   and a static map regenerated from this corpus.

The picture residuals and single-frame timing limits in `recorded-limits.md` stay with the
records that state them; a later polish task can take them by system.

## Reproduction

From a checkout of main (with `local/` linked to the main checkout's):

```
python3 local/evidence/coverage-gaps/make_manifests.py
local/evidence/coverage-gaps/captures.sh 8          # about 50 minutes on 8 cores
python3 tools/project.py coverage disassemble --out artifacts/listing --coverage <the four raw captures of tasks/STATIC-CODE-MAP.md>
python3 local/evidence/coverage-gaps/attribute.py local/evidence/coverage-gaps/routines.json
python3 local/evidence/coverage-gaps/blocks.py artifacts/listing local/evidence/coverage-gaps/blocks.json
```
