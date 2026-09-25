# FRONT-END-1P-CONTINUATION - after a one-player race

## Assignment

- Status: **in review**. Claimed 25 September 2026 at 20:53Z (26 September, 06:53 AEST) by the
  Claude Code desktop session that ran FRONT-END-1P-SETUP, on base `7d92810`. Re-scoped to the
  one-run race on 25 September (see "Outcome and boundaries").
- Milestone: M4 (original game coverage: menus)
- Coordinator: the claiming session is coordinator, primary and integrator
- Task provider (fixed for all children; record any user-initiated platform change): Anthropic
- Worker/session/runtime/model: Claude Code desktop app, Claude Opus 5.5 (`claude-opus-5-5`)
- Actual model/reasoning effort, routing rationale and frontier escalation question (if any):
  **tier 2** for the screens; **tier 1** for anything that changes how a race is scored or
  started. Kept at tier 2: the race engine's scoring and start are unchanged (the differential
  gates, the equivalence sweep and both recompares are identical). The menus' scoring and records
  (`update_records`, `score_race`) are new front-end code, checked against the listing by the
  worker and again by the reviewer, and against the original's cartridge RAM after a win and a
  loss (`sram.py`).
- Provider quota window/baseline timestamp, used/remaining or unknown, reserve and session
  allowance (D-0004): at claim the 5-hour window was 6% used and the weekly window 34%. The
  user's allowance: continue until the weekly window reaches 50%.
- Reviewer (primary automatically spawns fresh model/effort, isolated checkout; no user
  trigger): a fresh Anthropic subagent.
- Dependencies and evidence of acceptance: FRONT-END-1P-SETUP (the one-player screens, R-0055,
  R-0056); the race and its result screen (R-0046, R-0050, R-0053).
- Base commit: the `main` tip at claim.
- Branch and isolated worktree: `task/front-end-1p-continuation` in
  `.worktrees/front-end-1p-continuation`.
- Owned paths and shared interfaces: `src/core/front_end*` and the screen files, the app's race
  end, pack rules and content for the screens, native tests, a research record, this record,
  `docs/STATE.md`, `tasks/README.md`, `tasks/NEXT_SESSION.md`.
- Claim/lease/heartbeat/checkpoint location: this record.
- Session time limit, concurrency allocation and actual spend authorization if relevant: one
  worker plus the review subagent; no monetary spend.

## Outcome and boundaries

After a one-player race the original scores it, then returns to the tour (`$80:BC59-BC98`):
- `$83:879A` compares the times, or the stunt score with the qualifying score;
- a win marks the track done (`$77:1075`), and five done tracks raise the tour's medal and the
  rider's level (`$83:8827`, `$83:8853`);
- a loss sets `$77:0742` bit 12, which PICK TRACK counts down in `$77:1073`;
- then PICK TRACK comes again, with the done markers and the next track.

Make native do the same from the native race's result:
- the result's way back;
- the records: done tracks, medals, levels, best times and record holders;
- the tour's end: the pending reveal on PICK TOUR (`$77:10FD`);
- the game over after the tries run out.

**Re-scoped (25 September 2026).** The listing report (`local/evidence/front-end-1p-continuation/
post-race.md`) shows three result screens, and the tour's end has its own award, endings and
reveal. This task delivers the one-run race (race mode 0):
- the race's return;
- the result screen and its waits;
- the statistics, records, bests, done tracks and the loss flag;
- PICK TRACK again, and the app handing a one-run race back to the menus.

There is **no game over** in the original: nothing reads `$77:1073` (R-0057). The rest moved to
two queued tasks:
- the lap result, and quit and restart: [FRONT-END-LAP-RESULT](FRONT-END-LAP-RESULT.md);
- the tour's completion, medals, levels and reveal: [FRONT-END-TOUR-END](FRONT-END-TOUR-END.md).

Out of scope:
- saving the records across power cycles (SRAM persistence), unless the task finds it small;
- the stunt events' own rules (STUNT-EVENTS);
- the result screen's icons (RESULT-ICONS);
- audio.

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| Pictures and state | Every-frame captures of the original through a won and a lost race and back into the menus, against native | Pictures match to the pixel and the menus' words and records agree frame by frame, or each residue is explained | pictures, logs, research record |
| Records | The records after each captured race | Equal to the original's SRAM words, from a save-state or access capture of bank `$77` | log |
| Playable | The app from power-on through a tour's races | The menus follow each race as the original's do | report |
| Nothing moves | ctest, the synthetic suite, the v1 contracts, hidden runs, the differential gates, the equivalence sweep, the front end's comparisons | Unchanged | logs |

## Evidence

Commits `7b006e2` (the printer's codes), `d6115e4` (the result, records, app and R-0057) and the
review's answers after them. Evidence in `local/evidence/front-end-1p-continuation/`
(`NOTES.md`, `post-race.md`, `compare.py`, `sram.py`, `gates.sh`).

| Criterion | Result |
| --- | --- |
| Pictures and state | `cont-win`, `cont-loss` and their `-early` companions: the native race returns on the original's frame (3454, 4135); no difference in the menus' words, OAM buffer or text map on any frame from r + 101 to the next race; 2,577 distinct pictures equal (R-0057) |
| Records | `sram.py`: the kept cartridge RAM words equal the original's at cont-win 3808 and 4700, cont-loss 5008, 5210 and 5356. The words native does not keep are listed in R-0057 |
| Playable | Met for one-run races: the hidden app with cont-win's and cont-loss's pads returns the race at front-end frame 3455 and chooses the next race at 4808 and 5358. A lap race still ends on the race's own result (FRONT-END-LAP-RESULT); a stunt event is not native |
| Nothing moves | Gates on `d6115e4` (`gates-d6115e4.out`): three presets build, ctest 26 of 26; the synthetic suite; both v1 contracts; the hidden runs; the eleven differential gates (the same rows digests); the equivalence sweep, 351 runs with 0 differences; both recompares identical; the main menu's and the one-player screens' comparisons (the setup `compare.py` now knows the race screens); 0 functions over 80 lines; the address index current |

Review (tier 2): `review-d6115e4.md`, changes required with one must-fix (the rider choice starts
a new run, `$80:BBD6-BBE5`), all findings answered on the pull request.

## Handoff

- Findings: R-0057 (the one-run result, the records, no game over) and the listing report
  `post-race.md` (the lap and stunt results, the tour's completion, the quirks).
- Next: [FRONT-END-LAP-RESULT](FRONT-END-LAP-RESULT.md). A capture of the original winning ZOOM
  ZOO and leaving the lap result to PICK TRACK already exists: `local/evidence/front-end-lap-result/
  lap-won` (M4-16 boundary-a's inputs, then Start at 7600).
