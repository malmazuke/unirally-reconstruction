# R-0043 - the original's in-race HUD

Status: recovered and implemented; measured on both tracks against the
original's own frames. Owned by [CLASSIC-RACE-HUD](../../tasks/CLASSIC-RACE-HUD.md).

ROM `a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e` (PAL),
audited bsnes core `e59bf88d4fc922c9fe3b5438e65ff3a6909d24e1628f0f87141c8de17699a91b`,
pack `classic.pal.crawler.two-tracks.v9`.

## What the original draws

On every ordinary race frame the original paints two red fields straight over
the track, with no panel behind them: a left field at the top left and a clock
at the top right. After the player finishes it adds the player's finish time,
and after the opponent finishes the opponent's, both centred.

Native drew an authored dark bar across the top twelve rows with its own
five-by-seven font instead. On the `lap_one`, `lap_two` and `finish` scenes of
the M4-16 review's table, "HUD glyphs" was the **only** difference between the
native picture and the original's, so this was the last per-frame deviation on
an ordinary race frame.

## The layer, the cells and the glyphs

The fields are BG3, the same layer and the same font sheet as the captions
([R-0042](R-0042-stunt-name-captions.md)), and they use the same arithmetic: a
character is eight pixels wide and sixteen tall, drawn as the tile the character
names and the tile `$10` above it, and BG3's tilemap is based at word `$1800`
with a vertical scroll of one, so tilemap row `r` appears at screen `y = 8r - 1`.

The cells were measured before any of that was assumed. For every candidate
origin between y 10 and y 21, both 8-row ink masks of each glyph cell of the M4-16
primary's frame 3208 were looked up in an index of every 8-row window of the ROM
read as 2bpp. **y 15 is the only origin at which every cell's halves are found**,
and every hit is 16-byte aligned - so the cells are 8x16 at y 15-30, which is
BG3 rows 2 and 3. The same search against the pack then found every glyph inside
`presentation.classic.font.v1`, with the bottom half `0x100` bytes (sixteen
tiles) after the top. **The HUD needs no new pack content**, and no profile bump
is justified.

The sheet is sixteen glyph tops followed by their sixteen bottoms, four such
bands in 128 tiles, which is why R-0042's letters run `$0b`-`$0f` then `$20`.
Reading it out gives the whole alphabet: `$01`-`$09` are `1`-`9`, `$0a` is `0`,
`$0b`-`$0f` are `a`-`e`, `$20`-`$2f` are `f`-`u`, `$40`-`$44` are `v`-`z`, `$45`
is `:`, `$4e` is `/`, beside R-0042's `-` at `$4d`, `!` at `$60`, `"` at `$61`
and the space at `$80`. The original reaches them through one character table at
`$80:81F4`: index 0-9 the digits, 10-35 `a`-`z`, then the punctuation. Every
writer below reads a byte of that table, or a computed index into it, and
publishes `$3800 | tile` - attribute palette 6 with priority, exactly as the
caption does.

The ink is the caption's own colour. Measured by matching each cell's halves in
every colour present: ZOOM ZOO's frame 3208 reads `1`, `/`, `3` and `0:32:5` in
`#e70000`, and the caption `more stunts` on DRAGSTER's frame 1601 is the same
`#e70000`. Native draws both from `colour(cgram,22)`, and each track's own
palette supplies it.

## The fields

| Field | Tilemap | Screen | Publisher |
| --- | --- | --- | --- |
| left (lap count, `race` or `finish`) | rows 2-3, columns 1-6 | x 8-55, y 15-30 | `$81:EB8E-$81:EC5E`, `$81:D6E8-$81:D76E` |
| clock | rows 2-3, columns 24-29 | x 192-239, y 15-30 | `$81:ED5C-$81:EDD9`, blanked by `$81:ECCF-$81:ED52` |
| player's finish time | rows 5-6, columns 13-19 | x 104-159, y 39-54 | `$81:EE89-$81:EF49`, blanked by `$81:EDF1-$81:EE72` |
| opponent's finish time | rows 20-21, columns 13-19 | x 104-159, y 159-174 | `$81:F0E6-$81:F1A0` |

**The clock.** `$81:ED5C-$81:EDD9` writes exactly four digit pairs: words
`$1858`/`$1878` from `$0E2F`, `$185A`/`$187A` from `$0E33`, `$185B`/`$187B`
from `$0E37` and `$185D`/`$187D` from `$0E3B` - columns 24, 26, 27 and 29, the
minutes, the tens of seconds, the seconds and the tenths. The colons at columns
25 and 28 are written once at race setup and never touched again. The clock
shows tenths and no more; the subframe never reaches it.

**The left field.** `$81:EB86` reads the dirty flag `$0D17`; when it is set,
`$0EFB` (the player's laps remaining) chooses. Zero writes `f i n i s h` to
columns 1-6 from the character table. Otherwise `$053F` decides: when it is
clear the lap number is `$0D15 - $0EFB`, its ones digit at column 2 and a tens
digit at column 1 above nine, and when it is set the field is left alone. Race
setup wrote it: `$81:D620-$81:D6E5` puts `0` at column 2, `/` at column 3 and
the total laps (`$0EFB - 1`) at column 4, and `$81:D6E8-$81:D76E` instead writes
`r a c e` to columns 2-5 and sets `$053F`. Which of the two runs is decided by
the race mode `$77:074B`, which this project already carries as the scenario's
`tour_race` (1 for the ZOOM ZOO tour race, 0 for DRAGSTER). So ZOOM ZOO counts
laps and DRAGSTER shows the word `race` - measured on DRAGSTER originals 1380,
1400, 1420, 2000 and 3213, which all read `race`, and on ZOOM ZOO's own frames,
which read `0/3` through `3/3`.

**The redraw queue.** `$81:EB86` -> `$81:ECC2` -> `$81:EDDC` -> `$81:F033` is a
chain of dirty flags, each of which redraws **one** field and returns
(`JMP $F357`). At most one field is rewritten per update, in that fixed order:
the left field (`$0D17`), the clock (`$034D`: positive redraws the digits,
negative blanks columns 24-30 of both rows with the space tile), the player's
finish time (`$0349`), the opponent's, then the caption (`$0EE7`, R-0042). That
queue is what spaces the finish out over consecutive updates.

## The finish, update by update

Read from the ZOOM ZOO primary, where the player finishes on update 6484 and the
opponent on 6488. The tilemap write of update N is in the picture of frame N.

| Update | Write | Picture |
| --- | --- | --- |
| 6485 | `$81:EBAB-$81:EC58` to columns 1-6 of rows 2-3 | `finish` replaces `3/3`, clock still `1:38:0` |
| 6486 | `$81:ECD8-$81:ED4D` writes the space tile to columns 24-30 | the corner clock is gone |
| 6487 | `$81:EE8F-$81:EF49` to columns 13-19 of rows 5-6 | `1:38:02` appears |
| 6488 | `$81:F31D/$81:F337` (R-0042) | the caption `winner` appears |
| 6489 | the left field again, the opponent's finish having dirtied it | unchanged |
| 6490 | `$81:F0E6-$81:F1A0` to columns 13-19 of rows 20-21 | `1:38:10` appears |

So the player's finish sequence is one field per update in the queue's order,
and the opponent's own time follows two updates after the opponent finishes -
**in that race**. Those numbers are the queue's output only while nothing else
wants it. When the opponent's counter steps on the update right after the
player finishes, `$0D17` is set again while `$0EFB` is already zero, so the
queue spends that update writing `finish` a second time and every field behind
it waits: measured on two DRAGSTER races, the clock is still standing in the
picture where the primary's fixed offsets had already blanked it, and both
finish times follow a picture later (review 4). Native therefore follows the
queue itself rather than counting from the finish: `ClassicRaceHudClock` keeps
the pending set and services the first pending field each update, in the order
the dispatcher reads them.

**Every one of those numbers is a picture, not a state.** Picture N is drawn
from the state after update N-1, so a field the queue writes on update F shows
in picture F and the state the renderer reads for it carries
`finish_delay == F - finish - 1`. The first version of this work applied the
update numbers to the state directly and drew the blanked clock and both finish
times one picture late; the kept pictures step 20 frames apart and jump 6484 to
6500, so the whole transition fell in the gap and the sweep could not see it.
The independent review found it by recapturing the originals consecutively.

**The clock cells hold what the queue last wrote.** The queue rewrites at most
one field per update, the left field goes first, and an update on which the left
field is **written** spends that update, so the clock digits stand for one more
picture. Two paths write it, and only those two: a tour race whose dirty flag
`$0D17` is set, and the player's laps reaching zero, which writes `finish` on
either track. Both end at `$81:ECBC`, which clears the flag and returns through
`$81:F357`.

A dirty flag on its own is not enough, and that is the `$053F` branch this
record describes above: on the mode-0 race, whose field is the fixed word
`race`, `$81:EB93` falls through at `$81:EB9B` to the clock handler instead of
returning, and nothing ever clears `$0D17`. Measured on DRAGSTER captures, the
flag therefore stands set for **1,639 to 2,061 consecutive updates** from the
first crossing while the clock goes on being republished every update, against
seven or eight discrete sets in a whole ZOOM ZOO race.

What sets the flag on a tour race is **either rider's lap counter stepping**:
`$0EFB` for the player and `$0EFD` for the opponent, whose crossing dirties the
field without changing what it displays. Three readings of this rule were
measured, in this order, and the first two were wrong - each in a way the
evidence then in hand could not see:

- The discriminating case against "only at the finish" is `compound-reverse`
  update **3212**, which ticks the tenth and turns the player's lap together on
  a plain mid-race lap change: the original's picture 3213 keeps `0:32:5` where
  the state already says `0:32:6`, and 3214 has caught up.
- The discriminating case against "only the player's counter" is
  `ordinary-controls/down-a`, where the player crosses on update **3207** and
  the opponent on **3208**. `$0D17` is set on both and clears only on 3209, so
  the clock is held twice and the original's picture 3209 still reads `0:32:4`
  while the state says `0:32:5`. Holding on the player's field alone drew that
  picture one tenth early; the independent review found it.

- The discriminating case against "any counter step, on either track" is
  DRAGSTER itself, where the original holds on none of them: at
  `regression-landing-held-roll-a` update **1599** the player crosses the start
  line and the tenth ticks together, 1,600 updates before the finish, and the
  original's picture 1600 shows the **new** tenth. The independent review found
  that as a regression against the previous candidate.

Native follows this with `ClassicRaceHudClock`, which publishes the digits on
every observed update except one on which the left field was written, and lags
one update like the rider overlays. Without the history the digits are derived from the drawn state
instead, which differs from the original on the pictures after the updates that
write the field - at most four a tour race and one a sprint, and only where a
tenth ticks on one of them, which is none or one picture in most races and was
two in the worst measured. Earlier drafts of this record said "one picture" and
then "six or seven", and both were wrong. The only caller without the
history is the single-state debug form of `classic_race_presentation_runner`,
which already omits the rider overlays and the opponent-won banner for the same
reason; the app and the `--timeline` form both keep the tracker.

The rule was checked on both tracks, after two rounds of checking it on one.
Over five ZOOM ZOO captures (`boundary-a`, `brake-a`, `down-a`,
`compound-reverse-a`, `trick-long-a`) `$0D17` is set on seven or eight updates
inside the race proper and every one is a lap counter stepping
(`queue_probe.py`); over the DRAGSTER captures the flag is sticky and the
counters step four times a race, none of which the original holds
(`dragster_probe.py`). A probe run on one track cannot establish a rule for
both - that is how this was missed twice. The independent review then
confirmed the rule from the ROM rather than from this record: `$81:818D` is
the only instruction that sets `$0D17`, `$053F` has four writers and no
clearer and never changes inside a race, and a frame-by-frame comparison of
the ROM's own condition against native's predicate over 125 captures
disagrees nowhere.

A tour race therefore writes the left field seven or eight times, and a sprint
twice - once at its first crossing, which is not held, and once at the finish.
An earlier draft of this record said "at most four a tour race and one a
sprint"; that was wrong in both halves.

## The 10:00 time-out

At the limit (`$81:C73E-C75B`) the player is finished with laps left, so `$0EFB`
is not zero and none of this runs: the left field keeps the lap, the clock keeps
the held 9:59.9, and no finish time appears. The stop-timeout original reads
`2/3` and `9:59:9` on every kept frame from 31578 until the picture goes blank
for result loading at 31933.

## Composition

The HUD is the caption's layer, so it composes the caption's way, recovered in
R-0042 and corrected there by review: over the track, **under** the riders -
where a sprite covers a glyph the original shows `red = min(31, sprite_red + 13)`
with green and blue untouched - and **under** the channel-6 window members, which
paint the flat window colour over everything inside them. Native draws the HUD
into the same ink mask as the caption at the same point of the compose order, so
one rule covers both.

## Measurement

`hud_compare.py` regenerates the native timeline from an original capture's own
input timeline and renders every kept original frame; `hud_compare_manifest.py`
does the same from a replay manifest's controller events. Four boxes are scored:
the HUD's own rows (y 15-30), the rows the authored bar used to cover (y 0-14),
and the two finish-time bands (x 104-159 at y 39-54 and y 159-174). The same
sweep was run against a build of the base commit `92f46ba`, so the change is
measured rather than asserted.

| Sweep | Box | Base `92f46ba` | Candidate |
| --- | --- | ---: | ---: |
| M4-16 primary, 274 kept frames | HUD rows | 75,154 | **0** |
| | rows the bar covered | 838,656 | **0** |
| | whole picture | 938,565 | 18,601 |
| brake (loser) race, 274 kept frames | HUD rows | 75,154 | **0** |
| | rows the bar covered | 838,656 | **0** |
| DRAGSTER, 12 race frames of the continuous-Right manifest | HUD rows | 3,994 | **0** |
| | rows the bar covered | 36,864 | **0** |
| | whole picture | 42,820 | 403 |
| 10:00 time-out, 24 frames of the 9:59.9 hold | HUD rows | not run at the base | **0** |
| | whole picture | not run at the base | 144 |

DRAGSTER's frames 1400, 1500, 1601, 1800, 2000, 3000, 3213, 3400, 3450 and the
finish frame 3453 are **pixel-identical over the whole picture**. The six frames
from 3454 differ by about 50,000 pixels in both builds: the original has turned
the screen off for result loading and native has not, which is the known
transition timing and nothing to do with the HUD. The DRAGSTER row above is the
twelve race frames, and the arithmetic is on the face of the sweep: its own
totals cover all eighteen frames, each blank frame contributes the whole of both
boxes (4,096 and 3,840 pixels), and 24,576 and 23,040 are exactly six of each,
so the twelve race frames contribute nothing. The same holds for the time-out
sweep, where the 35 frames are 24 of the 9:59.9 hold and 11 blank ones from
31933, the two-update-late result load already declared in `docs/STATE.md`.

Outside those four boxes the primary's residual is 36 pixels a frame from 1620
on, and they are one shape: the red off-screen rider arrow at x 40-47, y
112-126, which the M4-16 review already listed as a declared omission. The
picture is therefore fully accounted for.

What is left in the two finish-time bands mid-race is the original's own
**signed split time**, which native has never drawn: `-0:00:1` at rows 20-21 on
primary frame 3900 and a signed `0:01:3` at rows 5-6 on 2080, a sign glyph
outside the caption alphabet followed by `M:SS:t`. Those same cells hold the
finish times after the finish, where native matches the original exactly,
including in the loser race where the opponent finishes first and its time
stands alone. The split display is a separate mechanism and is **not** recovered
here; before this task neither use of those cells was drawn at all.

## Domain and limits

- Measured on both tracks: ZOOM ZOO through the M4-16 primary original's 275
  kept frames and the ZOOM ZOO finish capture, DRAGSTER through the accepted
  replay manifests and the window-pause baseline's frames.
- The lap field's two-digit forms (a lap above 9, or a race of more than nine
  laps) are read from `$81:EC94-$81:ECBC` and `$81:D6B4-$81:D6E5` but are not
  exercised: both tracks of this product run one lap and three.
- `$77:074B` is the race mode, not a lap count. This record does not claim that
  a hypothetical one-lap tour race would show `race`; it claims that the mode
  the scenario already carries selects between the two fields, which is what
  the original's branch does.
- The two-player layout writes the same fields to tilemap rows 1-2 and a second
  copy at rows 15-16 (`$81:D1B2-$81:D30C` and `$81:EB44-$81:EB83`). Two-player
  is outside this product and none of it is implemented.
- The pre-race and result screens have their own writers in `$81:CFF6-$81:E4xx`;
  only the in-race fields above are recovered here.
