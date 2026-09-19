# ZOOM-ZOO-WINDOW-EFFECTS - ZOOM ZOO's countdown, GO and winner windows from the original's selection

## Assignment

- Status: review (implementation `11d50f6`, `49bd26c` and, after the returned review, `83af471`; re-review pending; local gates rerun at the corrected candidate, see below). Started 18 September 2026 23:05 UTC from `main` at `6adcde8`, the cold-start experiment every prior handoff named.
- Milestone: follow-up to CLASSIC-PRESENTATION-UNIFICATION, DRAGSTER-WINDOW-EFFECTS and DRAGSTER-WINDOW-PAUSE; closes R-0040's "ZOOM ZOO's own window content" bullet
- Coordinator: main session
- Task provider: Anthropic (unchanged)
- Worker/session/runtime/model: Claude Code, Claude Fable 5.1 (the primary chose the task, recovered the rule, implemented and measured it)
- Actual model/reasoning effort, routing rationale and frontier escalation question: Fable 5.1 as the session's model; no frontier consultation was needed, the question was answered by reading existing captures
- Provider quota window/baseline (D-0004): task start 23:05 UTC five-hour 18%, weekly 40%, weekly Fable 28%; user stop rule for this session 50% weekly or 50% Fable; no reset, purchase or provider change
- Reviewer: fresh Claude Opus 5 subagent in an isolated checkout at the exact candidate, spawned by the primary
- Dependencies and evidence of acceptance: R-0040 (mechanism, family, drivers), DRAGSTER-WINDOW-PAUSE (pointer through a pause), CLASSIC-PRESENTATION-UNIFICATION (one renderer, `classic_race_presentation_content`); all integrated on `main`
- Base commit: `main` at `6adcde8`
- Branch and isolated worktree: `task/zoom-zoo-window-effects`, `.worktrees/zoom-zoo-window-effects`
- Owned paths: `src/core/presentation.{hpp,cpp}`, `src/core/rider_look.{hpp,cpp}`, `src/core/movement.cpp`, `src/core/zoom_zoo_movement.hpp`, `src/core/classic_race_presentation_runner.cpp`, `tests/native/presentation_tests.cpp`, `tests/native/rider_presentation_tests.cpp`, this record, R-0040's "Established later" section, the coordinator records
- Claim/checkpoint: this record and ignored `artifacts/zoom-zoo-window-effects/` in the worktree (moved to `local/evidence/zoom-zoo-window-effects/` at closeout)

## Why

CLASSIC-PRESENTATION-UNIFICATION left ZOOM ZOO's countdown and winner windows
omitted "until its members are captured", and named the next experiment: bind
the window family for ZOOM ZOO, render scene 1450 and compare with the
original. R-0040 recorded that the drivers, the pointer and the family are not
DRAGSTER-specific but that which members ZOOM ZOO selects was not captured.

No new capture was needed. Every M4-16 original (`zoom_zoo_playable_reference`)
saved its whole WRAM per frame, so the pointer `$11FD` the vblank setup
`$80:868E` publishes can be read from the existing captures the same way
DRAGSTER-WINDOW-PAUSE read it from DRAGSTER's: frame n shows the value at the
end of frame n-1.

## Recovered

Read from seven ZOOM ZOO originals under `local/evidence/` (the M4-16 primary
`boundary-a`, `loss-a`, the race pauses `pause-a`/`pause-b`, the idle late
start `m4-16-idle/captures/late-start-a` in which the opponent wins, and the
countdown pauses `continued-controls/pause-countdown-a`/`-b`), and from the
ROM:

1. **The same family, the same drivers.** `$11FF` is `$15` and every `$11FD`
   value in every race is `$8000 + 899k` for a member k of the 25-table family
   `presentation.effect.classic.window-tables.v1`, or the `$DB4E` sentinel. The
   countdown thresholds, the GO parity, the banner drivers' arming, stepping,
   360-update life and two-driver order are those R-0040 and
   DRAGSTER-WINDOW-PAUSE recovered. The race vblank publishes from setup frame
   1382 (initialization 1376 + 6).
2. **Only the transition member differs.** Where DRAGSTER draws member 6
   between digits (1382-1402, 1432-1462, 1492-1522, 1552-1582 on ZOOM ZOO's
   clock), ZOOM ZOO draws member 5. The transition sites select `5 + $1229`;
   `$83:CC05-CC08`, in the race setup that also fixes `$1281` (R-0038), stores
   the player's reflection word `$0BA7` in `$1229` once, on the initialization
   frame (DRAGSTER's `$1229` goes 0 to 1 on 1328; ZOOM ZOO's stays 0). `$0BA7`
   at that moment is the start reflection the track header sets
   (`classic_race_start`: even start y word means reflected), 1 on DRAGSTER and
   0 on ZOOM ZOO, so the member is a property of the track content. Members 5
   and 6 are different shapes (a small start sign at x 78-99 and a large arrow
   at x 28-226), not mirrors. Whatever the player does afterwards, `$1229`
   does not change in any capture.
3. **Start held after the resume keeps the channel off.** In the countdown
   pause original (menu opened on update 1450, Down/Up on 1455/1457, Start
   held 1460-1462 to resume), the original shows no window on frames
   1451-1463, the window returning on 1464. The engine already counts updates
   1450-1462 as suspended (its `$83:CD05-CD35` divert runs while Start is held
   even after the selection returns to zero), but the presentation's
   diverted-update predicate read only the menu selection, so native chose a
   member on updates 1461 and 1462. That is a shared-engine presentation
   defect DRAGSTER's originals did not reach (their Start presses were single
   frames); it affects the rider look overlays too.

4. **Every window member covers both riders.** Found by scoring the pictures:
   on frame 1583 of the primary both riders stand at the line under the GO
   letters (member 4), and with the riders drawn over the window 346 of the
   392 residual pixels were the window colour in the original where native
   drew a rider. Inside a window the original shows nothing but the flat
   window colour: on that frame all 8,691 pixels inside member 4's XOR region
   are one colour in the original (the reviewer's measurement), and the same
   holds for the transition sign over a rider (member 5 on 1450 of the
   primary and on 1583 of the countdown-pause capture, whose pause pushed GO
   back to 1596) and for the opponent-won banner over the still-riding player
   (6724, 6725 and 6800 of the countdown-pause original). R-0040's "0-6
   compose before the riders, 7-24 after them" came from DRAGSTER frames with
   no rider under a countdown member; on DRAGSTER's release-3213 originals a
   rider sits under the digits or the GO letters on 75 frames (1420-1602),
   and all 13,804 pixels that change there now match the original. My first
   reading of this (the player's object showing through members 0-6, the
   SNES colour-math exemption of OBJ palettes 0-3) was wrong and is recorded
   under Mistakes; the register setup behind the compose was not read.

The ZOOM ZOO window timeline on the primary: members 5/0/5/1/5/2/5 on
1382-1582 with the digit thresholds of R-0040, GO alternating 4 and 3 from
1583 (frame 1583 shows update 1582's even-parity choice, 4) to 1651, nothing
from 1652, and from the player's finish at 6484 the banner from 6486 (the
driver's first update 6485 is odd, so member 8 shows first) to loading at
6725, member 19 on the last race vblank. In the opponent-won late start the
opponent's driver runs 6490-6849 and the player's 7420-7659 after its finish
at 7418.

## Implementation (`11d50f6`, compose order `49bd26c` corrected by `83af471`)

- `classic_race_start_reflected(track, rider)` in the engine: the header rule
  `classic_race_start` already applied, exposed so presentation derives the
  same value. `classic_window_transition_member(track)` returns `5 + reflected`.
- `ClassicRacePresentationContent` binds the window family for both tracks
  and carries `window_transition_member`, computed from the track content;
  `classic_countdown_window`, `ClassicWindowPointer::observe_update`,
  `classic_window_table_index` and the runner's `--window-index` mode take it.
  The legacy v1 DRAGSTER path keeps its constant 6. The authored READY/GO
  caption is suppressed where the windows draw, as it already was for DRAGSTER.
- `zoom_zoo_update_was_paused` reads the engine's suspended-update clock
  instead of the selection, so the pointer, the look and the overlays skip
  exactly the updates the engine diverted. Checked against the originals'
  look words with R-0036's harness rebuilt against the candidate
  (`artifacts/zoom-zoo-window-effects/look/`): heads `$0D49/$0D4B` and
  overlays `$0D45/$0D47` equal on every race update of the countdown-pause
  capture (10,696 checks), the primary (10,696) and the race pause (10,714);
  the previous predicate fails 263 of the countdown-pause checks from update
  1572, so it was wrong for the look as well as for the window.
- `render_classic_race` draws both objects first and then applies whichever
  member the frame shows over everything (the corrected compose order, third
  code commit); the legacy v1 renderer's calls are unchanged. The history
  tracker reads the track's transition member once per race.
- Tests: the transition member from a synthetic header (even and odd start y,
  the opponent's word not moving it, a third rider rejected); the digit and GO
  members unchanged under member 5; ZOOM ZOO's selection at the original's
  boundaries (1381/1382, 1402/1403, 1431/1432, 1462/1463, 1491/1492,
  1522/1523, 1551/1552, 1582/1583/1584, 1651/1652) and its banner from a 6484
  finish (nothing on 6484-6485, 8 on 6486-6487, 9 on 6488-6489, 10 on 6490);
  the pause predicate across opening, holding, resuming and Start held after
  the resume; the synthetic pointer script advances the suspended clock.

## Measurements on the candidate

Window pointer agreement, the runner's `--window-index` published column
(`ClassicWindowPointer` with history) against the original's `$11FD`, frames
from setup 1382 to the loading frame; `artifacts/zoom-zoo-window-effects/gates/probe-*`
(`pointer_probe.py`) and `dragster-pause-agreement.log`:

| Original | Frames scored | Agree | Before this task |
| --- | --- | --- | --- |
| ZOOM ZOO primary `boundary-a` (player wins 6484) | 5,344 | 5,344 | 5,230 (114 transition frames were member 6) |
| `loss-a` | 5,344 | 5,344 | 5,230 |
| `pause-a`, `pause-b` (race pause 6000-6010) | 5,353 each | 5,353 each | 5,239 |
| idle `late-start-a` (opponent wins 6488, player 7418) | 6,278 | 6,278 | 6,164 |
| `pause-countdown-a`, `-b` (menu 1450-1462 during digit 1) | 6,219 each | 6,219 each | 6,103 (transitions plus 1462-1463 chosen through the held Start) |
| DRAGSTER `orig-countdown-pause-a`, `banner-pause-a`, `banner-pause-odd-a`, `continuous-right-a` | 2,267 / 2,319 / 2,318 / 2,121 | all | all (unchanged) |

The single-state derivations (`classic_window_table_index` with and without
the tracked opponent finish) agree on the same frames except where they did
for DRAGSTER: without history the opponent-won banner is not drawn before the
player finishes, and after a pause the frame-based derivation is off by the
pause length (both declared in R-0040 and DRAGSTER-WINDOW-PAUSE).

Pictures against the originals' frame PNGs (`picture_compare.py`; rectangle
(0, 28, 256, 196) as in the window-effects measurement; "before" is the
`main` build at `6adcde8`, "after" this candidate; the state-driven renderer
in `--timeline` mode):

| Original frame | Changed pixels | Of those matching the original, before / after | Rectangle mismatch before / after |
| --- | --- | --- | --- |
| primary 1450 (digit 1's transition, member 5, over a rider) | 9,755 | 0 / 9,755 | 9,822 / 67 |
| primary 1583 (first GO frame, member 4, both riders under it) | 9,020 | 0 / 9,020 | 9,066 / 46 |
| primary 1649 (GO, member 4) | 9,020 | 0 / 9,020 | 9,414 / 394 |
| primary 6724 (banner member 19) | 5,268 | 0 / 5,268 | 6,095 / 827 |
| pause-countdown 1583 (member 5, GO pushed back to 1596 by the pause), 1649 (GO), 6724, 6725, 6800 (opponent-won banner over the riding player) | 9,755 / 9,020 / 3,561 / 3,561 / 3,033 | 0 / all | 9,822 / 67; 9,096 / 76; 3,838 / 277; 3,838 / 277; 3,374 / 341 |
| DRAGSTER release-3213, 183 frames with originals, 75 changed (1420-1602, a rider under the digits or GO) | 13,804 | 0 / 13,804 | 716,160 / 702,356; no frame worse |

Every pixel the change touches on those frames now matches the original
(33,063 on the primary, 28,930 of 31,678 on the countdown-pause capture, the
remainder being its frame 1450, where the original's pause menu is compared
with the authored overlay and the whole rectangle differs before and after,
a declared omission). The residual mismatches are the rider overlays and
authored HUD band declared by M4-16 and, on 6724, the original's WINNER
caption object (a declared omission), unchanged by this task; frames without
a window (1377, 1700, 2501, 3208, 4840, 6484, the result screens) are
pixel-identical before and after. Before the compose order was corrected the
primary's 1450, 1583 and 1649 scored 252, 392 and 693 with both riders drawn
over the window (`11d50f6`) and 163, 341 and 693 with the player kept
(`49bd26c`).

Hidden 4,000-update app runs of both tracks exit 0 with `rider-pose fallback
frames: 0`.

## Gates on the candidate

Run at `49bd26c` from this worktree with the ignored evidence under
`local/evidence/` (logs `artifacts/zoom-zoo-window-effects/gates/`; the same
set at `11d50f6` under `gates-11d50f6/`, where the synthetic suite's
`test_failure_and_timeout_are_distinguished` failed once with a second
1-second probe timing out while the differential compares ran concurrently,
and passed on rerun with 411 checks; at `49bd26c` the suites ran serially):

| Gate | Result |
| --- | --- |
| ctest, five presets (lab-debug, lab-release, lab-sanitize, app-debug, app-sanitize) | 23/23 each |
| `test --suite synthetic` (lab-debug) | passed, 411 checks |
| Frozen v1 DRAGSTER contracts (winner and loser `presentation-check`, v1 pack, fixtures copied under `local/v1-fixtures/`) | passed, every visual check within limits and unchanged |
| M4-16 ZOOM ZOO primary and idle late start (`zoom_zoo_playable compare`, v11 freezes) | passed, 757 and 801 restores |
| DRAGSTER primary, random-1, reversal (`dragster_playable compare`) | passed, 379, 567 and 327 restores |
| Window pointer agreement, seven ZOOM ZOO and four DRAGSTER originals | every frame, table above |
| Pictures, three ZOOM ZOO captures and DRAGSTER release-3213 | table above |
| Hidden 4,000-update app runs, both tracks | exit 0, 0 rider-pose fallback frames |
| `dragster_fuzz_runner` (lab-release), 40 seeds, 2 races each | 0 aborts |

## Independent review

Fresh Claude Opus 5 subagent in the isolated checkout
`.worktrees/zoom-zoo-window-review` (branch `review/zoom-zoo-window-effects`)
at `d69af67`, 13 minutes, report `tasks/ZOOM-ZOO-WINDOW-EFFECTS-review.md`
at `5a6a9af`. **Verdict: return.** It reproduced claims 1 and 2 to the frame
from its own build (the `$1229` latch and `$83:CC08` as the sole writer by a
byte search of the ROM, the header derivation on both tracks' real entries,
pointer agreement 5,344/5,344, 6,219/6,219 and 2,267/2,267, the look words
frozen across 1450-1462, every picture score and the DRAGSTER sweep, the
frozen compares and two presets' ctest) and found:

| Finding | Severity | Disposition |
| --- | --- | --- |
| B1. The keep mask preserves the player's object where the original shows the flat window colour: all 96/295/299/100/295 kept pixels on the five frames with a window over the player are fill in the original; covering both riders scores 67/46/394/67/76 against 163/341/693/167/371 | blocking | Fixed in the third code commit: every member composes after both objects; the keep mask, the player mask and the member-7 branch are gone. Re-scored: the table above, DRAGSTER 75 frames and 13,804 pixels all matching, none worse. Record, R-0040 and STATE corrected. |
| S1. The record called the countdown-pause capture's 1583 a GO frame (it is member 5; GO starts at 1596 there) and its "346 of 392 ... where native drew the opponent" did not reconcile | should fix | Fixed in item 4 and the table: 346 of 392 were fill in the original where native drew a rider, with both riders drawn over the window. |
| S2. Start-y word offsets in the new comments were given as words 3-4 / 5-6; they are bytes 5-6 / 9-10 | should fix | Fixed in `zoom_zoo_movement.hpp` and the test comment. |
| A1. The transition member was recomputed on every update | advisory | Fixed: the tracker reads it once per race. |
| A2. The keep mask's stack array and raw pointer parameter | advisory | Moot with B1. |
| A3. The declared omissions are acceptable, but the compose rule was inferred from a score, which is what let B1 through | advisory | Accepted: the register read is the named next experiment, and the rule as now recorded is the measured one. |

Re-review: pending at the corrected candidate.

## Mistakes

- I read a side-by-side crop of frame 1583 as showing the player's object
  over the GO band and the opponent's hidden, and implemented and recorded a
  colour-math palette exemption from it. The reviewer's pixel measurement
  showed every kept pixel to be window fill in the original: the visible
  rider was outside the XOR region, not exempt inside it. A crop is not a
  measurement; the per-pixel classification I had already written would have
  answered it.

## Handoff

- Branch `task/zoom-zoo-window-effects`; base `6adcde8`; head: see `git log`.
- Verified findings: the three recovered items above, with the captures and
  addresses named; probe outputs and picture scores under the ignored
  artifacts directory.
- Failed approaches: none; the first hypothesis (the DRAGSTER rule with the
  family bound) was right except for the transition member and the held Start,
  both read straight from the captures.
- Not done, by decision: ZOOM ZOO's other decorative objects (start ring,
  hints, on-screen stunt names, the WINNER caption and opponent finish time,
  off-screen arrows) are OBJ or BG content, not channel-6 windows (the
  pointer is only ever a family member or the sentinel in these captures),
  and remain declared omissions. The register setup that makes every window
  member cover both objects was not read; the rule is measured, not derived.
  No unit test renders the shared renderer's window over an object because
  the tests have no rider content; the picture scores above are the evidence
  for that rule.
- Exact next experiment if picked up cold: read CGWSEL/CGADSUB/WOBJSEL and
  the TMW/TSW window masks the race setup programs (access capture of
  `$83:E59C` and `$83:EA19` with the register mirrors), to replace the
  measured compose rule with the mechanism.
