# R-0059 - A tour's completion: the medal award and the way back to PICK TOUR

Status: recovered and implemented on `task/front-end-tour-end` (FRONT-END-TOUR-END), 26 September
2026. PAL ROM `a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`, audited bsnes
core `e59bf88d4fc922c9fe3b5438e65ff3a6909d24e1628f0f87141c8de17699a91b`. Follows R-0057 (the
one-run result) and R-0058 (the lap result).

A tour is complete when its fifth track is won, or at once when pad 1 holds exactly Select + X + R
as the scoring reads it (`$83:87B6`). Then the original:
- clears the tour's done tracks and raises the rider's medal on it by one;
- shows the medal award screen, a medal falling onto the rider on a podium, until a press;
- rebuilds the menus' screen, applies the unlock rule, and runs PICK TOUR from inside the
  scoring;
- goes on to PICK TRACK whether PICK TOUR ends on a choice or on Y or X.

Native now does the same (`src/core/award.cpp`), frame for frame, for a bronze or silver medal.
The gold medal's endings are not recovered (see "Not recovered").

## Frames

q is the result's exit frame (R-0057). The scoring runs at q + 3, on its pad read (`$80:D1E8`).

| Frame | Address | What happens |
| --- | --- | --- |
| q + 3 | `$83:87B6`, `$83:881B-8832` | The forced test, or a fifth done track; the tour's done tracks cleared (`$83:895B`), the track rounded to the tour's first (`$83:8940`), the medal raised (`$83:8832`) |
| q + 4 to q + 19 | `$83:A4E9` | Fade out: brightness 14 to 0, then forced blank |
| q + 19 to q + 99 | `$83:A614` | The award's sound program; the NMI still runs the palette cycle |
| q + 100 | `$83:A67F-A720`, `$83:AEF6-AF3E` | NMI off; VRAM 0x7000-0x77FF cleared, the text cleared, the text halves and scrolls reset, OAM reset; BGMODE 2, TM 0x13; the medal's colours (asset 0x1F + medal) at 0x80, the rider's at 0x90; the background map (0x53) at VRAM 0 |
| q + 102 | `$83:AF4B` | The background's tiles (0x54) at VRAM 0x3000 |
| q + 103 | `$83:AF54` | OBSEL 0xA3 (32 x 32 and 64 x 64 objects); the podium's map (0x65, palette 1 and priority added) at VRAM 0x1000 |
| q + 104 | `$83:AF69` | The podium's tiles (0x64) at VRAM 0x2000 |
| q + 105 | `$83:AF72-AF92` | The podium's colours (0x3B) at 0x10, the background's (0x54 + medal) at 0; the medal's object tiles (0x5C) at VRAM 0x6000 |
| q + 109 | `$83:AF96-AFB6` | The medal and the rider, entries 0 and 1, from `$83:B120`; the OAM copy |
| q + 110 | `$83:AFBA-AFDD`, `$80:F814` | The rider's pose 0x1340 into its tiles at VRAM 0x7000 (`$83:8E3A`) |
| q + 111 to q + 125 | `$83:A4D2` | Fade in: brightness 1 to 15 |
| from q + 126 | `$83:AFF6-B093` | The animation, three frames a step, until a press |

The screen is mode 2: BG1 the background, BG2 the podium, both 4bpp. Mode 2 takes
offset-per-tile data from BG3's map, which is the background's map at VRAM 0 here. Its words
have bits 13 and 14 clear, so no offset applies, and the native screen refuses a map where one
would.

## The animation

Each step is three frames: a build, an upload with the pad read, a test (`$80:B6D3`).
- **Build** (`$83:B004`): the pose step `$77:10C9` counts up; the rider's pose is `$83:B142`'s
  word for it. The medal step `$77:10CB` counts up from -5. From 0 the medal takes `$83:B1B6`'s
  tile for it, and until 0x1A it falls 4 lines a step.
- **Upload** (`$83:B062`): the pose's tiles (`$80:F814`), the OAM copy and the pads. From medal
  step 0x27 both objects bounce: y = 0x71 less `$83:B129`'s byte for it.
- **Medal step 0x26** takes three more frames (`$83:B096-B111`): the medal's second art from
  `$09:A658` to VRAM 0x6000 and `$09:B658` to 0x6800, the pose again with the medal's tile 0x84
  written into OAM directly, then the bounce's first height.
- **Pose step 0x3A** is a two-frame reset (`$83:AFE8`): the steps go back to 0x2B and 0x26 and
  the test reads the last upload's pads again. From there the last 14 steps repeat every 44
  frames.
- **The test**: any of pad 1's twelve buttons as the last upload read them; pad 2 is ignored
  (`$77:0742` bit 10). A button must be down on an upload frame, so a short press can be missed.

## The way back

From the test that sees a press, t:
- t + 1 to t + 16: fade out, then forced blank.
- t + 16 to t + 87: `$83:A721`'s sound program.
- t + 87 to t + 117: `$83:A721` rebuilds the menus' screen as `$80:D20E` does, without its reset
  of the menus' words (`$80:A16A`). Its registers, colours and VRAM loads run from t + 87 to
  t + 112, the rest at t + 117; native makes them all at t + 117, as the screen is blank until
  t + 118:
  - the main menu's registers (mode 3, OBSEL 0x63), its colours (the rider's at 0xF0 by
    `$83:91FB`) and VRAM;
  - the text cleared and sent to the shown half; the objects laid out (`$80:D2C1`'s layout) and
    copied (`$80:9314`, without a frame wait);
  - the logo held up (`$80:F53B`), NMI on (`$83:A90E`).
  - One difference from `$80:D2C1`: that routine reads the waiting tiles of entries 104-111 with
    `LDA $9B31,X` and the data bank at `$80` (`$80:D318`), which are code bytes. `$83:A721` reads
    them with a long `LDA $839B31,X` (`$83:A8A2`): the decoration animator's wave tiles.
- t + 118 to t + 132: fade in, brightness 1 to 15.
- t + 132: the unlock rule, then PICK TOUR (`$80:E54C`) in the same frame. It is R-0056's entry
  from `$80:E550`, the same as the way back from PICK TRACK. Its slide follows `$00AC` as it was
  saved before the race: 1 slides it back, 2 (NOW PLAYING left with Y or X before the race)
  slides it forward, and PICK TRACK afterwards the other way round (`$80:E92F`).
- PICK TOUR's first interactive frame is t + 179. It returns into the scoring on a choice or on
  Y or X alike (`$83:88D1`), and `$80:BC7B` runs `$80:A858` (two frames) and PICK TRACK, which
  slides forward.

The arrow does not move during the award: its frame waits are `$83:A923`'s. The palette cycle
stops with the NMI at q + 100 and resumes at t + 118.

## The records

- The tour's five done tracks are cleared, and the medal `$77:069C + 16 x tour + rider` is raised
  by one, up to gold. The raise does not look at the medal raced for (`$77:10D1`).
- The unlock rule (`$83:8853`), unless the rider's level is already 3: over tours 0-7 (HUNTER's
  is not counted), all eight gold (a sum of 24) gives level 3; otherwise six tours at silver or
  better give level 2; otherwise four at bronze or better give level 1. The counts must be exact.
  The level goes to `$77:10D3 + rider`, and the same value to `$77:10FD`, the pending reveal,
  whenever a count matches, even when the level does not change.
- A forced completion runs no win test: no done flag and no loss bit are set, whatever the race.
- No checksum is written between q + 3 and t + 132; nothing in the ROM verifies them.

## The pack (profile v20)

Profile v20 (328 rules entries) adds the award's assets 0x3B, 0x53-0x56, 0x5C, 0x64 and 0x65
(`front-end.asset.NNN`), the medal's second art `front-end.award-medal-art` (`$09:A658`, 8,192
bytes) and the award's tables `front-end.award-tables` (`$83:B120`, 203 bytes: the two objects'
first entries, the bounce, the poses, the medal's tiles).

## Evidence

Captures in `local/evidence/front-end-tour-end/` (bsnes, Strict): FRONT-END-1P-CONTINUATION's
`cont-win` to the one-run result (DRAGSTER won), then pad 1 holding exactly Select + X + R from
3799 to 3807, which leaves the result at q = 3800 and completes CRAWLER.
- `forced-bronze`: every picture from 3790 to 5199, the award's animation looping.
- `forced-bronze-long`: Start at 5400 on the award; pictures every 10th frame to 7199, PICK TOUR
  with CRAWLER's bronze.
- `forced-choice` and `forced-back`: then Start (CRAWLER) or Y at 5700 on PICK TOUR, both to
  PICK TRACK; every picture from 5390 to 6299.
- `forced-bronze-long` continued as `forced-silver-bronsen`: CRAWLER, then on PICK TRACK the medal
  line stepped back to none (Up, A, Down), so that BRONSEN races again; DRAGSTER raced and a second
  forced completion, silver. Start at 10290 on the award; pictures every 5th frame from 5700 to
  10899. (In `forced-silver` the medal raced for stayed bronze, so SILVIA raced: the runner stops
  there, as the race scenarios are BRONSEN's.)
- All have work RAM every frame. The listing work is `decode/tour-end.md`; its `sim.py` checks
  the animation rule against every frame of both first captures.

`compare.py` runs the native front end with the captures' pads and the native race between the
menus, and compares the menus' words, the OAM buffer and the text map on every frame, and every
picture.

| Capture | Completion (q + 3) | Frames compared | Differences | Pictures equal |
| --- | --- | --- | --- | --- |
| forced-bronze | 3803 | to 5199 | none | 1,410 of 1,410 |
| forced-bronze-long | 3803 | to 7199 | none | 340 of 340 |
| forced-choice | 3803 | to 6299 | none | 910 of 910 |
| forced-back | 3803 | to 6299 | none | 910 of 910 |
| forced-silver-bronsen | 3803, 8693 | to 10899 | none | 591 of 591 |

`sram.py`: the kept cartridge RAM words (the medals, the done tracks, the levels, the records)
equal the original's at `forced-bronze-long` 3900, 5600 and 7199 and `forced-silver-bronsen`
8800 and 10899. Not kept: the award's counters `$77:10C9` and `$10CB`, the pending reveal
`$77:10FD`, and the checksums.

## Not recovered

- **The reveal.** Whenever the rule's counts match (`$83:8899`, `$83:88AD`, `$83:88C1`), even
  with the level unchanged, the original draws PICK TOUR with level - 1 (`$80:E55B-E560`),
  slides it in, then shows the other tours four frames later (`$80:E588-E5A2`) and clears
  `$77:10FD`. Native shows the level at once and keeps no pending reveal. No capture reaches it
  yet (it needs four completions).
- **The gold medal's endings** (`$83:88FD`, one routine per tour) and HUNTER's (`$83:AB9A`,
  which ends in a soft reset). Native leaves a gold completion at once through the award's way
  out: the fade, the restore, the rule and PICK TOUR. For HUNTER native so keeps the session
  where the original resets.
- **A completion by a fifth win** is not captured. The tour's stunt event is not native, and a
  capture from a preloaded cartridge RAM (R-0050's method, four done tracks) needs the runner to
  start from those records, which it does not yet (FRONT-END-ENDINGS). From `$83:881B` on it is
  the same code as the forced one.
