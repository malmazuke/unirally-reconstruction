# FRONT-END-TOUR-END - a tour's completion: the award, the endings and the unlocks

## Assignment

- Status: **accepted** (pull request #32, merge commit `4b66150`, 26 September 2026 at 04:47Z;
  claim to merge 2 hours 37 minutes). Queued 25 September 2026 (UTC) by FRONT-END-1P-CONTINUATION; claimed
  26 September 2026 at 02:10Z by the same Claude Code desktop session, on FRONT-END-LAP-RESULT's
  head `003a7b3` (pull request #31), rebased onto `main` `d39e444` after it merged.
- Milestone: M4 (original game coverage: menus)
- Coordinator: the claiming session is coordinator, primary and integrator
- Task provider (fixed for all children; record any user-initiated platform change): Anthropic
- Worker/session/runtime/model: Claude Code desktop app, Claude Opus 5.5 (`claude-opus-5-5`)
- Actual model/reasoning effort, routing rationale and frontier escalation question (if any):
  **tier 2** for the screens; **tier 1** for anything that changes the scoring.
  Kept at tier 2: the win test is unchanged, though the scoring now runs on its own frame, q + 3
  (it ran a frame early, q + 2, with no visible difference); the completion's records (the done tracks, the
  medal, the unlock rule) transcribe `$83:881B-88D2`, checked against the listing by the worker
  and the reviewer, against cartridge RAM after two completions (`sram.py`), and by native tests
  of every rule case.
- Provider quota window/baseline timestamp, used/remaining or unknown, reserve and session
  allowance (D-0004): at claim the weekly window was about 42% used. The user's allowance (25
  September 2026): continue until the weekly window reaches 80%.
- Reviewer (primary automatically spawns fresh model/effort, isolated checkout; no user
  trigger): a fresh Anthropic subagent.
- Dependencies and evidence of acceptance: FRONT-END-1P-CONTINUATION (R-0057); PICK TOUR's
  levels and badges (R-0056); the locked tours' preload (R-0050).
- Base commit: the `main` tip at claim.
- Branch and isolated worktree: `task/front-end-tour-end` in `.worktrees/front-end-tour-end`.
- Owned paths and shared interfaces: the front end's files, pack rules and content for the
  screens, native tests, a research record, this record, `docs/STATE.md`, `tasks/README.md`,
  `tasks/NEXT_SESSION.md`.
- Claim/lease/heartbeat/checkpoint location: this record.
- Session time limit, concurrency allocation and actual spend authorization if relevant: one
  worker plus the review subagent; no monetary spend.

## Outcome and boundaries

The fifth done track of a tour completes it (`$83:879A`, `$83:881B-88D2`, R-0057's listing
report):
- the done flags cleared;
- the medal raised by one, up to gold;
- the award screen (`$83:AEF6`) or, at gold, the tour's ending (`$83:88FD`);
- the rider's level (`$77:10D3`) and the pending reveal (`$77:10FD`), by exact counts;
- PICK TOUR called from inside the scoring, with the newly opened tours revealed;
- then PICK TRACK with no test of Y or X.

Pad 1 exactly Select + X + R on the scoring's frame forces the completion for any race kind. This
gives captures a way to reach it without winning five races. A preloaded cartridge RAM with four
done tracks is the other (R-0050's method).

Native refuses a fifth done track today (`score_race`). Make it do what the original does.

Out of scope: the stunt events (STUNT-EVENTS); the endings' music (audio).

**Re-scoped (26 September 2026).** This task delivers the completion itself, the bronze and
silver award screens, the restore, the unlock rule and the way back to PICK TOUR and PICK TRACK.
The gold medal's endings and the reveal of new tours move to
[FRONT-END-ENDINGS](FRONT-END-ENDINGS.md).

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| Pictures and state | Every-frame captures of the original through a forced completion (bronze, then silver, then gold with the ending) and through a completion by a fifth win | Pictures match to the pixel and the menus' words agree frame by frame, or each residue is explained | pictures, logs, research record |
| Records | `sram.py` after each completion | The medals, levels and reveal equal the original's cartridge RAM | log |
| Playable | The app through a completion | The award, the reveal on PICK TOUR, and the next tour | report |
| Nothing moves | ctest, the synthetic suite, the v1 contracts, hidden runs, the differential gates, the equivalence sweep, the front end's comparisons | Unchanged | logs |

The gold endings, the reveal and a completion by a fifth win moved to
[FRONT-END-ENDINGS](FRONT-END-ENDINGS.md) with the re-scope.

## Evidence

R-0059 and `local/evidence/front-end-tour-end/` (`NOTES.md`, `decode/tour-end.md`, `compare.py`,
`sram.py`).

| Criterion | Result |
| --- | --- |
| Pictures and state | A bronze and a silver forced completion, the award looping, its exit, and PICK TOUR's exits by a choice and by Y: no difference in the menus' words, OAM buffer or text map on any frame; 4,161 pictures equal (R-0059) |
| Records | `sram.py`: the medals, done tracks, levels and records equal the original's cartridge RAM after both completions |
| Playable | The app hidden with `forced-bronze-long`'s pads: the race returns at front-end frame 3455, and the run ends on PICK TOUR (screen 7) with CRAWLER's medal 1, so through the forced completion and the award (re-run on the head after `ec64483`, which adds that line to the app's log). A completion by a fifth win is not captured (R-0059) |
| Nothing moves | Gates on `ec64483` (`gates-ec64483.out`): three presets build, ctest 26 of 26; the synthetic suite; both v1 contracts; the hidden runs and the front end's app runs; the eleven differential gates (the same rows digests); the equivalence sweep, 351 runs with 0 differences; both recompares identical; every front-end comparison equal (the main menu, the one-player screens, the one-run and lap results, the five completions); the records equal; 0 functions over 80 lines; the address index current. The head after it changes the app's closing log line, one test's assertion and records; its build, ctest and the app run were re-run |

Not done here, recorded in R-0059: the gold medal's endings and HUNTER's, and the reveal of newly
opened tours (native shows the new level at once).

## Handoff

- Findings: R-0059 and `decode/tour-end.md` (its section 5 outlines the endings).
- Next: [RACE-PAUSE-EXITS](RACE-PAUSE-EXITS.md), then [FRONT-END-ENDINGS](FRONT-END-ENDINGS.md).
- The race scenarios are MIKE's against BRONSEN: after a bronze, PICK TOUR's choice makes the
  medal raced for bronze, so the next race is SILVIA's unless the medal line steps it back
  (`forced-silver` against `forced-silver-bronsen`).
