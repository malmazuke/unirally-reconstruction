# FRONT-END-1P-SETUP - the one-player setup screens

## Assignment

- Status: **accepted** 26 September 2026 (tier 2,
  [#29](https://github.com/malmazuke/unirally-reconstruction/pull/29)). Claimed 26 September 2026 at 16:25Z by the Claude Code desktop
  session that ran FRONT-END-MAIN-MENU, on base `a76961e`. Done in two parts on two branches, one
  pull request: part 1 (PICK YOUR UNI) on `task/front-end-1p-setup`, part 2 (PICK TOUR, PICK
  TRACK, NOW PLAYING, the race start) on `task/front-end-1p-setup-2`, which carries part 1.
- Milestone: M4 (original game coverage: menus)
- Coordinator: the claiming session is coordinator, primary and integrator
- Task provider (fixed for all children; record any user-initiated platform change): Anthropic
- Worker/session/runtime/model: Claude Code desktop app, Claude Opus 5.5 (`claude-opus-5-5`)
- Actual model/reasoning effort, routing rationale and frontier escalation question (if any):
  the claiming session's model at default effort; **tier 2** unless the chosen rider, tour or
  track changes race state in a way the race tasks have not accepted (then tier 1).
- Provider quota window/baseline timestamp, used/remaining or unknown, reserve and session
  allowance (D-0004): at claim the 5-hour window was 14% used and the weekly window 28%. The
  user's allowance: continue until the weekly window reaches 50%.
- Reviewer (primary automatically spawns fresh model/effort, isolated checkout; no user
  trigger): a fresh Anthropic subagent.
- Dependencies and evidence of acceptance: FRONT-END-MAIN-MENU (the front end, the SNES screen,
  the text printer; R-0054); the race scenarios of every track (R-0046, R-0050).
- Base commit: the `main` tip at claim.
- Branch and isolated worktree: `task/front-end-1p-setup` in `.worktrees/front-end-1p-setup`.
- Owned paths and shared interfaces: `src/core/front_end*`, the app's start of a race, pack rules
  and content for the screens, native tests, a research record, this record, `docs/STATE.md`,
  `tasks/README.md`, `tasks/NEXT_SESSION.md`.
- Claim/lease/heartbeat/checkpoint location: this record.
- Session time limit, concurrency allocation and actual spend authorization if relevant: one
  worker plus the review subagent; no monetary spend.

## Outcome and boundaries

After 1P on the main menu (mode 0, `$80:BB9C`, entered once and running the whole one-player
flow), the original shows these screens, then the race:
- PICK A PLAYER: 16 riders, MIKE first;
- PICK TOUR: CRAWLER first; the other tours as SRAM unlocks them;
- PICK TRACK: the tour's five tracks;
- NOW PLAYING: the rider against the opponent, with Race and Exit.

Make native do the same. The rider, tour and track chosen start that race through the existing
race scenarios; a choice the race engine does not cover yet (a stunt event, a rider whose
content is not packed) is refused with a notice rather than approximated. Back and cancel paths
return where the original returns.

Out of scope: what follows a race (FRONT-END-1P-CONTINUATION), persistence beyond reading the
unlocked tours of a cold start, audio.

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| Pictures and state | Every-frame captures of the original through each screen, with cursor moves both ways, back paths, and choices of several riders, tours and tracks, against native's `front_end_runner` | Pictures match to the pixel and the screens' state words agree frame by frame, or each residue is explained | pictures, logs, research record |
| Race start | For each choice captured, the race native starts | The race state equals the scenario's start that the race gates already accept | log |
| Playable | The app from power-on to a race of a chosen track | Reaches the race | report |
| Nothing moves | ctest, the synthetic suite, the v1 contracts, hidden runs, the differential gates, the equivalence sweep, the main menu's comparisons | Unchanged | logs |

## Result

The app now runs the whole one-player setup from the main menu to the race, frame for frame
against the original ([R-0055](../docs/research/R-0055-rider-menu.md),
[R-0056](../docs/research/R-0056-tour-track-now-playing.md)).

- **PICK YOUR UNI** (`src/core/rider_menu.cpp`):
  - the logo slides up; the names slide in (`screen_slide.cpp`, with the main menu's decoration
    animator);
  - 16 riders with an animated unicycle icon each, the arrow on the last rider chosen;
  - an HDMA palette split gives 16 riders their colours from 8 object palettes. The SNES screen
    now applies colours written during the picture (`SnesLineColour`).
- **PICK TOUR** (`tour_menu.cpp`):
  - the badges ("?" for a locked tour) and the medals;
  - moves limited to the open tours, and the arrow's mirroring.
- **PICK TRACK** (`track_menu.cpp`):
  - twelve pictures of the tour and the five tracks;
  - the medal line, which steps the medal to race for;
  - the done-track markers;
  - the generic list menu's wrap.
- **NOW PLAYING** (`now_playing.cpp`):
  - the rider's and the opponent's lines with their icons and times;
  - "racing on", "over N laps on" or "doing stunts on" with the qualifying score;
  - the record line; Race and Exit.
- **Paths back**:
  - Y or X on every screen slides the one before back in;
  - Y on PICK YOUR UNI and Exit on NOW PLAYING slide the main menu back in.
- **The text printer** gains F7 (a track's name), FD (a five-digit number) and EF (an object at the
  cursor).
- **The app**: NOW PLAYING's Race starts the chosen race where a race scenario has it: MIKE
  against BRONSEN, not a stunt event. Other choices show a notice and return to the main menu.
  `--front-end-inputs` replays an input script through the front end (a smoke-test aid).
- **Packs**: v16 adds the rider menu's content, v17 the other three screens'. All raw ROM.

Decisions and deviations, with reasons:

- **One pull request for both parts.** Part 2 was ready while part 1's gates ran, so the gates,
  the review and CI run once, on the whole task.
- **The race scenarios decide what the app races.** They are MIKE against BRONSEN. Another rider,
  a stunt event, or a medal above bronze (unreachable on a cold start) shows a notice, as the
  task's boundary asks.
- **`$008F` is one byte.** Every menu shares it: whole-byte writes in three menus, bits 3 and 2
  in two. Modelling it as one struct keeps the carried-over latches exact between screens.
- **Scratch words are compared as the original means them.** `$0076` is a slide's countdown in
  one screen and an intro counter in another. `$0090` gains 0x4C00 a pass that cancels in the
  scroll. The comparison checks each by its meaning.
- **The title's unlock code is not modelled.** On a cold start the SRAM set-up clears it right
  after the title. Unlocked tours need persistence, which is out of scope.

## Evidence and attempts

| Attempt | Hypothesis | Experiment | Observation | Next decision |
| --- | --- | --- | --- | --- |
| 1 | - | `defaults`: every-frame images 600-1399, per-frame work RAM, loads and PPU writes | Four screens; CGRAM loads only; BG1 and BG2 scrolls; HDMA on PICK YOUR UNI | Read the handler |
| 2 | - | A read of `$80:BB9C`-`$80:CD46` (subagent, `pick-your-uni.md`) | The slide, the icons, the HDMA split, the pad rules, the back path | Captures |
| 3 | - | `moves`, `back`, `held` | Pad 1 only; the edges clamp; Y back to the main menu at c + 45 | Native |
| 4 | Native matches | `compare.py` on the four captures | 0 differences in every word, the OAM buffer, the text map and 1,255 pictures, first run | Part 2 |
| 5 | - | Reads of PICK TOUR, PICK TRACK and NOW PLAYING (subagents); `tour-moves`, `tour-back`, `tour-code`, `track-moves`, `track-race` | The title code is cleared by the SRAM set-up; `$008F` is shared; NOW PLAYING's Exit returns to the main menu at c + 42 | Native |
| 6 | Native matches | `compare.py` on all nine captures | `$009B` is each screen's own cursor; the race's first frame's work RAM is the race's. Otherwise 0 differences, 6,166 pictures equal | App |
| 7 | - | The app, hidden, Start held | DRAGSTER chosen after 613 frames, then the race | Gates |
| 8 | - | Review of `493e97d` (M1: no evidence that a race other than the start-up one starts, nor from this build) | The app gains `--front-end-inputs` (the runner's input script, by the front end's frame). With `track-race`'s inputs from power-on it chooses race 13 after 1,008 frames, the original's race frame. After 1,392 race updates with no input its state (frame 2784, player x 2192) equals `--track 13`'s, the scenario the race gates accept. The rule for racing or a notice is unit-tested (`native_one_player_race`), and the runner reports the chosen race | Fixes S1-S8, N1-N7; gates |
| 9 | - | Gates at `89556bd` (`local/evidence/front-end-1p-setup/gates-89556bd.out`; runs at `d8271f0` and `493e97d` were stopped when later commits superseded them) | ctest 26/26 on three presets; synthetic and both v1 contracts pass; eight hidden race runs pass with 0 fallback frames; from power-on the app races DRAGSTER with Start held, and FLAT FUN with `track-race`'s inputs (its state after 1,392 updates equals `--track 13`'s); the idle notice returns; the eleven differential gates pass on pack v17 with unchanged digests; the equivalence sweep against main's binaries: 351 runs, 1,933,523 updates, 2,052 pictures, 0 differences; recompare identical on 20 + 25 tracks; the main menu's captures 1,000/1,000, 901/901, 301/301; the nine one-player captures equal on every frame with 6,166 of 6,166 pictures; 0 functions over 80 lines; the index check passes | CI |
| 10 | - | CI on `89556bd` | The Linux job's GCC flagged a sign conversion (`rider_menu.cpp:134`, a complement of a byte constant meeting unsigned operands); two such expressions made unsigned. ctest and three captures re-checked on macOS | Merge |

## Handoff

- Integrated through [#29](https://github.com/malmazuke/unirally-reconstruction/pull/29).
- Next task: FRONT-END-1P-CONTINUATION (COVERAGE-ROADMAP step 3): after the race, the result's
  way back into the tour, the next track, the tour's end, unlocks and saving them. It needs the
  race's result handed back to the front end (`$80:BC59-BC98`, `$83:879A`), and captures of the
  races after this task's menus.

## Review and integration

Tier 2, by a fresh Claude Opus 5.5 subagent in its own checkout, of `493e97d` (posted on #29).
It rebuilt, ran ctest and the tooling tests, re-ran six captures (all equal on every frame), and
regenerated the pack entries from the ROM (all equal). It also checked the models against the
listing.

It returned the task for M1: no evidence that the app starts a race other than its start-up one.
Answered in `89556bd`: the app's `--front-end-inputs` reaches FLAT FUN from power-on with the
race start `--track 13` has, and the race-or-notice rule is unit-tested.

Its should-fix and note items are fixed:
- the all-won PICK TRACK refused;
- the listing-only paths listed in R-0056;
- the printer's codes and the formats tested;
- named high-table helpers and shared constants;
- the arrow's palette set as one field;
- stale comments;
- consistent statuses;
- the chosen race's track in the app's report;
- `compare.py` on v17.
