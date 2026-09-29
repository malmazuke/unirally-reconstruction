# R-0069 - first split-screen demo race

Status: implementation candidate for [SPLIT-SCREEN-RACE](../../tasks/SPLIT-SCREEN-RACE.md).
The tested domain is PAL cold power-on, Start held on frames 300-305 to skip the title,
then both pads released through frame 6999. Acceptance for this task concerns the first
demo race, initialized at frame 1448 and back at the main menu by frame 3500. The
second idle demo cycle after that is outside this task.

## Identities and repeatability

- Original ROM SHA-256: `a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`.
- Pinned bsnes libretro core SHA-256: `e59bf88d4fc922c9fe3b5438e65ff3a6909d24e1628f0f87141c8de17699a91b`.
- Capture method: `local/evidence/split-screen-race/capture_demo.py`, using Strict
  serialization, a fresh process and blank persistent memory for each run. The
  replay schedule is the one stated above. `demo-primary-full/reference.json`
  holds whole-WRAM, SRAM and 256 x 224 video SHA-256 for each frame 1300-6999;
  the WRAM and SRAM prefixes are kept as `wram-series.bin` (0x2200 bytes a frame)
  and `sram-series.bin` (0x2000 bytes a frame).
- `demo-primary-full`, `demo-repeat` and `demo-transitions` agree on all 5700
  whole-WRAM, SRAM and video hashes. SHA-256 of the concatenated WRAM hashes is
  `a06f0e267ff2408cdfa6fd1378b2f834181e133c40541d970fd176dd8359662a`;
  SRAM `8081485cdac82845f380bef567f9a4d68a7355283696a92421d5d1e8d5238e18`;
  video `2e732a43573d2225758d790e667c521370d8afbc80c4541489b9b87f935f9635`.

## Verified observations

1. `local/evidence/split-screen-race/demo-entry/` and `demo-race-start/` are
   bounded access captures on the same replay. The main menu's idle selection
   changes to demo at frame 900. At frame 1351, SRAM `$77:074A` and `$77:074B`
   become 1 (ZOOM ZOO, lap race); menu temporaries `$017D/$017F` become 3/1.
   The actual race pairing at frame 1448 is SRAM `$77:0748/$77:0749=4/14`
   (AMY/ALICE); the demo loader changes it between those observations. At frame
   1358 `$77:0750` becomes `0xC20A`. The race initializes at the end of frame
   1448: both riders start at x `0x23F0`, y `0x05D0`, both cameras x `0x22F0`, y
   `0x04D0`; the countdown is 270. Fade `$0FF1` first advances on frame 1449.
2. At frame 1449, each camera moves to x `0x2300`, y `0x04E0`, and each rider's
   screen publication is offscreen: top `$1513=0x7070`, bottom `$150B=0x3030`.
   `$1225/$1227` are both 1. At frame 1700 top camera is x `0x20D8`, y `0x05C1`;
   bottom is x `0x20D5`, y `0x05C9`. The cameras follow different riders.
3. The first 565 bytes of the original's projected race state at frame 1448 are
   byte-equal to the accepted native one-player ZOOM ZOO start at frame 1376, apart
   from the frame label. The next update first differs in opponent retained OAM x
   (native 101, original 48) and `$1227` (native 0, original 1). On update 2,
   opponent velocity/residue differs. A native seed restored at update 1 with only
   those two serialized bytes changed removes that immediate velocity/residue
   difference. This is a diagnostic experiment, not a full split implementation.
4. The first race's timer requests exit at frame 3348. The menu reappears through
   brightness steps on frames 3457-3463, and frames 3480 and 3500 are stable.
   A later idle demo cycle reaches a one-player track 3 with SRAM
   `$77:0750=0xC202` by frame 4399.
5. The native split camera and `$83:E254-E55B` demo controls, with the split
   speed-limit branch and second rider's initial reward queue, produce equal
   projected state on every one of the 1,901 original frames 1448-3348. The
   early projection has 453 rows (1448-1900); the authenticated late projection
   has 1,448 rows (1901-3348), SHA-256
   `8a983136d8412853017ff78fc008863fddff125e106817f9537953c1777332af`.
   Compare bytes 16-564, which omit only the 8-byte state magic and the frame
   label differing by 72. The first attempt diverged on the opponent's
   offscreen position after one update; then on the first demo input at 1623,
   the contact-phase jump at 1655, the split speed limit at 1659, the
   second rider's first scoring event at 1740, and three checkpoint banners.
   Each was traced to the static routine and checked at the first divergence
   before the final full replay. At frame 3348 the demo exit also matches.
6. The separate second camera's x, y, horizontal/vertical velocity, lookahead
   and screen OAM word, plus demo elapsed and exit flag, match all 1,901 frames.
   Before correcting the setup OAM, its only difference was frame 1448:
   original `$150B=0x2065`, native `0xE0E0`. `$82:D76D-D774` sets `0x2065`.
7. The native front end and two-view renderer equal the original RGB pixels at
   all 328 retained pictures of the first demo path (325 distinct frame labels,
   including 81 in `demo-hud-frames/` on frames 2470-2520 and 3320-3349).
   The source directories are `demo-primary-full`, `demo-title-frames`,
   `demo-title-exit-frames`, `demo-exit-pictures`, `demo-menu-return-frames`
   and `demo-hud-frames`. The only two unequal retained pictures are frames
   5500 and 6999, both from the out-of-scope next demo cycle. This compares
   retained pictures, not every picture in the race. The late HUD
   check required the lower clock's upload delay, the lower ROLL caption, the
   split arrow at row 6 and BG3 over the riders.
8. `capture_human_variations.py` starts fresh PAL emulation runs, clears only
   WRAM `$7E:212C` before frame 1650 to stop the demo AI, then applies either
   port 1 Right+B or port 2 Left+A on frames 1650-1660. An identically injected
   neutral run is the control. Each variation changes its own pad image and
   rider state versus that control. The native two-port path produces equal RGB
   pixels on every one of frames 1650-1680 for both variations (62 pictures).
   The intervention isolates the shared split race controller; these runs do
   not establish the unimplemented 2P menu path.
9. Split native states use layout `URZZ000F` (784 bytes) or `URZZ000G`
   (836 bytes if special-tile words are live). Their trailer keeps the second
   camera, demo controls, pairing and opponent tier. Restoring at original
   frame labels 1650, 2500 and 3320 reproduces the uninterrupted serialized
   state through frame 3348. The separate 1P layouts stay unchanged.

## Static reading and implementation implications

The static code map's `bank-81.lst` reads `$81:9FB0-A16D` and
`$81:A23C-A2D5` as the two camera follows and vertical calculations. With `$0DE1`
nonzero, the top camera's vertical branch at `$81:A0D4-A16A` differs from the
one-player branch and uses the other rider's y. The second camera has its own
`$041F/$0423`, `$04FB/$04FF`, `$0555` words. The 1,901-frame camera-state
comparison above tests this reading on the first demo path.

`bank-82.lst` reads `$82:ACAC-B05A` as two-rider visibility. The split path
uses vertical offsets -41 through 111 in each 112-line viewport. Its top
offscreen publication is `0x7070`; its bottom offscreen publication is `0x3030`.
The bottom on-screen y adds `0x70`. The prior update's publication influences
physics, so split visibility must be part of the native simulation.

`bank-83.lst` reads `$83:CD50-CD61` as a call to `$83:E254-E55B` when the demo
flag `$7E:212C` is set; the split flag `$0DE1` then suppresses the normal
opponent AI. The demo routine writes both riders' controls, advances `$1387` and
can exit the demo. Its two controller paths, at `$83:E2C4` and `$83:E417`, use
each rider's marker, motion and trick state. A marker-only approximation would
invent behavior; implement this listing and compare it dynamically.

At `$82:A77E-A7D6`, `$0C6D` selects the one-player opponent's catch-up term.
The original split demo holds `$0C6D=0`, so both riders use their own progress
adjustments. `$81:C5D5-C5E1` clears the second rider's announcement wait on
the first scoring event while `$12E5` is set; original frame 1740 does so and
halves its event-one weight from 4 to 2. `$81:8288` writes 120 to a checkpoint
banner; the split demo keeps the full 120 for the second rider at frames
2080-2199, 2327-2446 and 3054-3173. The one-player comparison had shortened
an unseen opponent's banner to 2.

## Boundary

This record tests the first idle demo race on ZOOM ZOO and the two injected
control variations at frames 1650-1680. It does not claim a 2P or VS setup
screen, arbitrary pairs/tracks, the next idle demo cycle, or audio.
