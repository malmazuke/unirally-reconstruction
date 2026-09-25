# R-0052 - The HUNTER tour's tag effects

Status: recovered and implemented on `task/hunter-effects` (HUNTER-EFFECTS), 25 September 2026.
PAL ROM `a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`, audited bsnes
core `e59bf88d4fc922c9fe3b5438e65ff3a6909d24e1628f0f87141c8de17699a91b`. Answers R-0051's
HUNTER finding (TWO LOOPS' divergence at update 1,252).

On the HUNTER tour (`$131F` nonzero, race tracks 40, 41, 43 and 44) every race update ends in
`$83:CEC9` (called from `$83:CDAA`). The HUNTER opponent can tag the player, and each tag starts
one of eight timed effects. Addresses are the original's; the effect number k is the flag
`$1327 + 2k`.

## The tag

`$83:D104-D1C2` runs while no effect is running (`$1323`) and neither rider has finished.

- First the riders' progress counts (`$0FCD`, `$0FCF`) must have differed by 2 or more once,
  latched in `$1325` (N-flag compares).
- Then the riders' boxes are tested for overlap: x+8 to x+40 and y to y+40 for each (`$0415`,
  `$0419`, `$0417`, `$041B`).
- An overlap sets `$1323` and effect `player x & 7`.

## The dispatch and the announcements

`$83:CED9-D100` takes the first set flag in the order 3, 1, 5, 2, 0, 4, 6, 7 and clears the
others. On the update it is picked (flag 1, set to 2):

- It pushes its event to the **front** of the player's queue (`$81:C55B`). The push writes at
  the read cursor, which then steps back, so it shows next. A full queue takes nothing.
- It names the HUD message `$12AF`, except effect 1, and plays sound `$021F` (not played
  natively).

Every update the effect's own routine runs. Most count a 500-update timer. At 0 the routine ends
the effect with event `$23` (sixteen spaces) and resets the HUD message buffer `$128D-$12AD` to
blanks (`$83:D275`).

The captions (`$17:CA04`) are:

| k | Flag | Event | Caption | What it does |
| ---: | --- | --- | --- | --- |
| 0 | `$1327` | `$1F` | barf mode on | BG2 scrolls by the camera's full y horizontally and full x vertically (`$81:AE90`) |
| 1 | `$1329` | `$1C` | hedgehog speed | freezes: 1, 2 ... 20 skipped race updates, then 18 ... 2; ends unannounced (`$83:D530`) |
| 2 | `$132B` | `$1E` | power bounce on | the player's landings use landing matrix 0 (`$0572`), even where none would apply (`$81:94B9`) |
| 3 | `$132D` | `$1B` | screen flip on | BG1 upside down through a per-line scroll table, and the riders' objects turned over (`$83:D2C9`, `$83:D581-E081`) |
| 4 | `$132F` | `$20` | invisible track | BG1 off the main screen (`$055D`, `$80:8638`) |
| 5 | `$1331` | `$1D` | slow motion | three race updates in four skipped (`$83:D4E8`) |
| 6 | `$1333` | `$21` | wobble mode on | BG1 and BG2 mosaic, its size the low three bits of a counter the NMI advances (`$055F`, `$0563`, `$80:8821`) |
| 7 | `$1335` | `$22` | control reversed | the port reader swaps left and right, the two rotations, and Y with A (`$82:AC5A-ACA7`) |

Effects 3, 4 and 6 blink over their first 50 updates and last 50 (60) through a 64-byte table
at `$83:D3BC`, pack entry `zoom.hunter-blink`. With cartridge option bit 3 (outside the
recovered domain) effect 3 would announce `$24`, "invisible unis".

## Skipped updates

`$128B`, set by effects 1 and 5, makes the next frame jump from `$83:CC9A` to `$83:CDAA`.

- The race update, its clocks and phases, the controller reader and the pause menu wait.
- The HUNTER routine and the hint timer (`$83:CDBC`) still run.
- Native carries `skip_update` and does the same. The presentation treats a skipped update like
  a paused one: no look step and no window drivers.

## The HUD message and the caption row

The player-announcement consumer `$81:BEA8` sets `$11C1` when it shows an entry.

- When the queue next runs dry with `$11C1` set, `$81:BF32-BFB7` converts the pending message
  `$12AF` into the HUD buffer `$128D` and clears it.
- It then copies the buffer to the caption row `$0EA7`, so the effect's name reappears whenever
  the queue is idle.
- Because a front-of-queue push overwrites the queue slot the row was drawn from, native carries
  the row, identified by the smallest event with the same text (0 blank).

## The opponent is character 20

`$77:0749` is 17 on every tour but HUNTER's, where it is 20. Two consequences:

- The opponent's trick voice events are `72 + 16 * (character >> 1) + (x & 15)` (`$82:9D3A-9D5B`),
  so 232-247 on HUNTER.
  - Their reward lookup (`$81:C25C-C260`, `$7E2102 + event - 1`) indexes the learned bank past
    its end, at `$7E21E9-21F8`. Those bytes are zero on all 129,053 frames of 53 HUNTER captures
    (review), so the original takes a zero-weight exit.
  - `track_reference` now guards them, as the frozen manifest guards `$7E21C9-21D8` for 200-215.
- `$82:DDB0-DDBC` loads the opponent's sprite palette from asset 6 + character: asset 26 on
  HUNTER, pack entry `presentation.classic.hunter-opponent-palette.v1`.

## State

HunterEffects, 31 words, closes the other tracks' layout: `URTRnn06`, 916 bytes.
`classic_race_layout.hunter_bytes` projects each from WRAM.

- `$11C1` and the caption row are compared on the HUNTER tour only.
- The HUD buffer and the caption row are decoded to their events through the caption table
  (`hud_captions`).

Pack profile v14 adds the blink table and the opponent palette.

## Evidence

- `track_reference recompare --per-track`: locked-tour sweep **20 of 20** exact over the whole
  window (TWO LOOPS joins), and cold start 16 of 16. **All 36 race tracks now match.**
- 48 held-input captures on the HUNTER tracks (`local/evidence/hunter-effects/held`) are exact
  over their whole windows. They reach all eight effects: two races tag twice, and effects 1 and
  5 skip 300 and 375 updates.
- Pictures (`pictures/`), through each visible effect on TWO LOOPS, 36 frames. They match with
  0 differing pixels outside the declared off-screen rider arrow (36-pixel multiples in its box
  at y 112-126), except one frame below.
- Hidden app runs of 4,000 updates with Right held on tracks 40, 41, 43 and 44 are clean.
- The review's withheld captures (`review-withheld/` and the reviewer's worktree) found two
  faults, fixed: a skipped update still takes the pad images the NMI publishes (`$0311-$0314`);
  the HUNTER learned-bank guard is `$7E21E9-21F8`. With them, Start and B pressed on skipped
  updates (`w-hedgehog-pause`, `w-slow-pause`) are exact over 2,210 updates, and
  `track_reference` understands the reversed and skipped publications.
- Other withheld captures, all exact:
  - `w-bounce-jumps`: 8 landings under power bounce.
  - `w-p1-long`: four tags over 4,610 updates.
  - `w-flip-left`, and four untagged runs.

## Limits

- When the queue runs dry and the effect's name returns to the caption row (`$81:BF32`), native
  can show it a frame early: the original's caption transfer (`$81:F30C`) waits while another
  HUD cell transfer is pending that frame. Seen on wobble 3,403 and, in the review, screen-flip
  frames 3,403 and 5,503. That arbitration is general HUD behaviour, not modelled.
- A known divergence outside the effects: the review's `w-rev-buttons` (TWO LOOPS, effect 7
  with Y, A, L, R, B, X and Left) matches to update 1,737, where the player's velocity y is 72
  natively and 0 in the original. It is the second contact of a rolling rider after a 32-update
  shoulder-button rotation; no HUNTER word is read there, so it is attributed to the shared
  contact code ([ROLLING-CONTACT](../../tasks/ROLLING-CONTACT.md)). "All 36 race tracks match"
  holds for the sweeps and the held captures, not for that input.
- The sound `$021F` is not played (audio is a declared omission).
- Cartridge option bit 3's variants (the opponent queue push, event `$24`, `$82:B15E`) are
  outside the domain.
- `$12D1` (a palette mode that replaces the tag, `$83:D1CA`) is zero on every HUNTER race and
  not implemented.

## Reproduction

```sh
local/evidence/hunter-effects/captures.sh; local/evidence/hunter-effects/captures-2.sh; local/evidence/hunter-effects/captures-3.sh
local/evidence/hunter-effects/regression.sh
local/evidence/hunter-effects/captures-pictures.sh   # then pictures.py per effect
```
