# R-0087 - The idle demo rotation

Status: research result and native change ([IDLE-DEMO-ROTATION](../../tasks/IDLE-DEMO-ROTATION.md)),
9 October 2026, on main `c9f0361`. PAL ROM SHA-256
`a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`, reference core built from
bsnes `7d5aa1e656b9171524d01b1b22917197d8121cb4` (library `e59bf88d`). Captures in main
`local/evidence/idle-demo-cycles/`.

Tags: **[L]** listing only, **[C]** confirmed in a capture.

## Domain

A cold power-on with Start on frames 300-305 and both pads released after it. R-0069 and R-0070
covered the first two idle demos; this record covers every later one, through two laps of the
rotation (66 cycles).

## The rotation

- **Track.** The mode-5 handler (`$80:948D-94B2`, unclassified bytes in the static map) sets rider
  3 and opponent 1 (`$017D/$017F`), mode 5 (`$77:10AD`), then advances the cartridge counter
  `$77:10C8` by 1, wrapping at 40, into `$CE`, again while the track is a stunt event (race mode
  2). So the demos take tracks 1, 3, 4, 5, 6, 8, ..., 39, 0 and repeat: 32 races a lap, never
  HUNTER's. [L, C: 66 cycles in `timing-200000.json`]
- **View.** `$83:C91C-C937` (demo only, option bit 1) flips `$77:1115`; 1 is a split race (option
  bit 3, `$0DE1`). With 32 races a lap each track keeps its view: split on tracks 1, 4, 6, 9, ...,
  39, one-view on 3, 5, 8, ..., 38 and 0. [L, C]
- **Pairing.** `$83:9894` copies the direct page `$0000-$019D` to `$77:0E6B`, so `$77:0F34` is
  `$00C9`, the menu palette cycle's phase. `$83:C94A-C994`: the rider is
  `(track + $77:10B1 + $77:0F34) & 15`; a split race's opponent is
  `(track - $77:10B1 - $77:0F34) & 15`, plus 13 if that is the rider; a one-view race keeps
  opponent 1. `$83:C8EF-C8FB` and `$83:C91C-C92A` first put a counter outside its range back to
  0. `$77:0F34` is 3 and `$77:10B1` 0 on all 34 cycles of `cold-102000`. [L, C]
- **Race start.** The race initializes (`$7E:212C` set) exactly `race_sound_load_offset(track)`
  (R-0077) + 7 frames after the track is written, on all 66 cycles. The demo skips the sound
  session (`$83:C9F6-CA05`). [C]

## Menu timing

- The track is written 453 frames after the menu's idle count ends, and the race runs 1,900
  updates to its exit request. [C]
- **The return.** After the timer exits of the races on tracks 20, 24 and 36 the return holds its
  frame 100 one picture longer (both laps); the palette hook starts a picture later. The return
  after track 20 shows an extra sound-port transfer at its frames 72-75 (`access-return-r16b`).
  The mechanism in the sound processor is not recovered. [C]
- **The title.** 6 titles of lap 1 and 4 of lap 2, not the same ones, are 452 frames: the CPU's
  first wait for the sound processor (`$82:8097`, after the driver reset at title frame 58) ends a
  frame sooner (`access-title-c12`, `-c13`). It depends on the sound processor's free-running
  state, which native does not emulate through a demo race. Native keeps 453; the laboratory
  runner's `--short-demo-title CYCLE` replays a capture's short titles. [C]

## Corrections found on the way

- **The one-view demo's tier.** Its computer opponent is character 1. The original subtracts 16
  (`$F1`), above every tested level, so a launch suppresses for 60 (`$83:E175`). Since 7c301dd
  native gave every opponent below 16 the two-human tier, and the second demo differed from R-0070's
  frozen projection on 1,651 of 1,901 rows (`ai.suppression_counter` 60 against 30). A
  `two_humans` scenario flag now marks the local modes. [C]
- **MIKE's hints.** Rider 1's hints run in a split demo but never for MIKE (R-0082): cycle 15
  (track 19, opponent 0). [C]
- **The AI-off marker.** The pads are read before the demo controls (`$82:AB6A`), and the controls
  return at once on a marker with bit 15 (`$83:E2EB`, `$83:E42F`), so that rider's direction stays
  neutral. Native kept the previous direction: cycles 3, 21 and 27 (tracks 4, 26 and 34). [L, C]

## Native result

- `front_end_runner` from power-on with the capture's short titles replayed: all 34 cycles of
  `cold-102000` equal the original on every race frame, race words (bytes 12-564) and the 42-byte
  demo trailer (`compare_cycles.py --trailer`).
- State formats: a split demo off ZOOM ZOO writes layout J on its 916-byte base; a one-view demo
  writes 8 (other tracks) or K/L (DRAGSTER). Restores in 9 cycles (split, one-view, MIKE's, DRAGSTER)
  read back byte-identical and continue in step to the exit.
- Pictures: 906 of the capture's 1,006 retained pictures are equal. The others fall in classes:
  the idle menu (17 of 158, about 23,300 pixels, as on main at frame 7,000), the title's wave (24
  of 151, about 8,500 pixels, as on main at 4,100), two title starts (cycles 10 and 17), the
  countdown's GO letters out of phase in some races (19,155 and 18,889 pixels around update
  220-275), and small sprite differences (4-107 pixels). Queued as presentation work.

## Not covered

Audio; pad presses in cycles after the second; the menus' sound handshake that shortens titles;
a cartridge whose `$77:10C8` or `$77:1115` was left by an interrupted lap is covered by the same
rule but not captured.
