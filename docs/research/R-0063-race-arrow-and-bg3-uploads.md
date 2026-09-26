# R-0063 - The race's direction arrow, the order of its BG3 uploads, and the look's head point

Status: recovered and implemented on `task/race-offscreen-arrow` (RACE-OFFSCREEN-ARROW),
26 September 2026. PAL ROM `a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`,
audited bsnes core `e59bf88d4fc922c9fe3b5438e65ff3a6909d24e1628f0f87141c8de17699a91b`. Follows R-0043
(the race's HUD), R-0042 and R-0061 (the captions) and R-0036 (the rider look).

Three picture residues were left in the one-player race: the "off-screen rider arrow" (R-0043's
declared omission), a caption that sometimes reaches the screen a picture late, and MIKE's upper
body two pictures early on one track. All three are recovered, and the race's pictures now match
the original's on every compared frame.

## The direction arrow

It is not an off-screen indicator: it reads no position or camera. The one-player race NMI
(`$81:E8C9-EB83`, before the HUD's text uploads) draws it from a word the update computes at
`$82:9822`:
- No arrow when the two riders' progress transition counts, halved, are equal, when the player is
  not behind (the difference's bit 15), when the player has finished, or in a stunt event.
- Otherwise it points the way the player's track marker runs, from the marker word of the previous
  update (this update's contact samples it later): right (BG3 rows 14-15, columns 26-28), left
  (columns 5-7), and up or down variants (columns 15-16 near the top or bottom; listing only).
- Its length, one to three chevrons, shows the opponent's lead d: one chevron under 10; from 10 to
  19 a blink of 2, 1, 0, 2 chevrons; from 20 a cycle of 3, 2, 1, 0. An NMI counter steps the cycle
  every 8 frames.
- The NMI redraws it only on pictures after an update of the alternating phase 1; on the others the
  last arrow stays.

The glyphs are tiles `$46`/`$56` (left), `$47`/`$57` (right), `$48`-`$4B` (up, down) of the caption
font already in the pack. It shows in BRONSEN's races too, on the frames the player falls behind,
and on the HUNTER tour.

## The BG3 upload order

The one-player race NMI uploads at most one text field a frame, in this order: the left field
(`$0D17`), the clock (`$034D`), the player's cells (`$0349`), the opponent's cells (`$034B`), the
stunt event's field (`$12C9`), then the caption (`$0EE7`). The addresses R-0061 named
(`$81:E49F`, `$81:E6F6`, `$81:E79B`, `$81:E831`) are the two-player NMI's.

A caption, or its blank after the queue runs dry, therefore waits a picture for each field ahead of
it. The clock is the usual one: the hints' cadence keeps the queue's consumptions on the updates
where the tenth ticks. Native now requests the caption when the player's queue takes an event (its
read cursor steps forward, or its display leaves the dry state, which also covers an event pushed
to the front of the queue in the same update; on the HUNTER tour also when the carried caption
changes), or on the first dry look after an event was taken, and serves it last.
This replaces R-0061's measured rule for the blank, which fitted only because dry looks fall on tick
updates. The research's first formulation, a request whenever the cooldown rises, failed where the
hints end in the same update as a consumption.

## The look's head point

The look (R-0036) reads each rider's head point, `$1265,Y`, which only the rider's contact routine
writes. While the corkscrew or the loop carries a rider the contact does not run, so the point stays
at the pose of the last contact. Native had taken the current pose's point; in the track 25 race
SILVIA is in the corkscrew and MIKE's look turned two pictures early. Native now keeps point 0 of
the pose of the latest update whose contact ran, plus the current position. The race state does not
store whether the contact ran, so the look infers it from what the update leaves: the corkscrew's
physics hold and the loop's cooldown and step. The captures confirm the corkscrew case; the loop's is
covered by native tests only. A history started from a restored state uses the current pose until
the rider's next contact.

## Evidence

The research (`local/evidence/race-offscreen-arrow/decode/`, `arrow.md`) replayed the captures and
checked each rule against the original's own memory:
- The arrow against VRAM on every frame of eight captures (25,673 frames): 0 mismatches.
- The upload order against the caption rows on every picture of seven captures (20,326 pictures): 0
  mismatches.
- The head point against `$1265-$1268` on all 36,536 rider updates of six captures.

The native pictures (`checks/`: the scripts, and the logs of the base, built from the claim commit,
and of the final code; and the gates), before and after:

| Comparison | Before (differing pixels) | After |
| --- | ---: | ---: |
| R-0061's seven race captures, 307 frames | 7,767 | 0 |
| silvia-runner-25, frames 1836-1864 | 944 | 0 |
| Consecutive frames around the arrow (silvia-dragster 213, silvia-zoom-zoo 242, M4-16 61) | 22,521 | 0 |
| M4-16 primary, brake and trick-long kept frames (274, 274, 192) | 9,086 | 0 |
| The split-time captures' consecutive frames and DRAGSTER's splits | 3,291 | 0 |
| The eight HUNTER tour captures | 4,938 | 0 |

## Not recovered

- The up and down arrows, the player's cells covering the up arrow, a rejected progress transition
  and the 10:00 time-out's one-update lag: listing only, covered by native tests.
- An arrow drawn without history (a single restored state) takes its counter from the frame and
  the update's own marker: an approximation, not measured.
- Whether RESTART RACE resets the NMI counter, and the arrow's colours in the pause picture.
- Two other conditions that skip a rider's contact are not modelled; they were never set in these
  races.
