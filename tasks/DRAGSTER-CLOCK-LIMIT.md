# DRAGSTER-CLOCK-LIMIT - The 10:00 race limit on DRAGSTER

## Assignment

- Status: reviewed and integrated (implementation approved at `c00dd5d`, review `7f391e1`); acceptance conditional on final-tip CI and remote verification. Closeout: ignored `artifacts/dragster-clock-limit-integration/closeout.json`
- Milestone: follow-up to accepted DRAGSTER gameplay and to DRAGSTER-ORDINARY-CONTROLS; not an M4 milestone gate
- Coordinator: Claude Opus 5 primary session (Claude Code desktop)
- Task provider: Anthropic (Claude Opus 5), per D-0004
- Worker/session/runtime/model: implementation subagent, Claude Opus 5
- Provider quota: plan telemetry through the app usage tool. Session start 2026-09-17T19:50Z: 5-hour window 72% (resets 21:20Z), weekly 17% (resets 2026-09-24T08:00Z), extra usage disabled. **User budget: stop new work at 90% of the five-hour window or 45% of the weekly limit.** No reset, purchase or provider change. At stop (2026-09-17T21:26Z, after the window reset at 21:20Z): five-hour 2% (resets 2026-09-18T02:20Z), weekly 19%. The run peaked at 85% of the pre-reset five-hour window, below the 90% stop.
- Reviewer: fresh Claude Opus 5 subagent in an isolated checkout at the exact candidate (not yet run)
- Dependencies: M4-16 (recovered ZOOM ZOO engine and its clock limit), DRAGSTER-ORDINARY-CONTROLS (DRAGSTER on the shared engine); both on main at `033d4a7`
- Base commit: `033d4a7`
- Branch and isolated worktree: `task/dragster-clock-limit`, `.worktrees/dragster-clock-limit`
- Owned paths: the DRAGSTER clock-limit case and its inventory, `dragster_playable`'s inventory and prefix gate, the DRAGSTER result composition, this record, R-0039, the DRAGSTER capture section of docs/BUILD_AND_VALIDATION.md
- Checkpoint location: this record

## Outcome and boundaries

ZOOM ZOO's race clock holds at 9:59.9 and finishes both riders when it would
reach 10:00 (`$81:C73E-C75B`), and native ZOOM ZOO implements it. DRAGSTER now
runs the same engine, but nobody had tested it and every frozen DRAGSTER
original is far shorter than ten minutes. Establish, from original DRAGSTER
evidence, what the original does at 10:00, make native match it over that
timeline for every declared byte, and keep every accepted gate passing.

Out of scope: DRAGSTER's window enable and HDMA timing (the separate
not-started follow-up in `tasks/NEXT_SESSION.md`), the DRAGSTER renderer's
accepted presentation limits, audio, menus, live play by the user.

## Inputs and prerequisites

Supported PAL ROM and audited bsnes core (identities in docs/STATE.md); the
two-track pack v7 (`b75539a0...`) and the DRAGSTER v1 pack for the historical
matrix; the accepted DRAGSTER menu path
`tests/manifests/replay/race-crawler-dragster-3000.json`; the seven frozen
DRAGSTER originals and the M4-16 ZOOM ZOO boundary captures, copied into this
checkout under ignored `artifacts/dragster-clock-limit/regressions/`.

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| Original evidence at 10:00 | one DRAGSTER timeline that idles to the clock limit, captured twice | identical per-frame WRAM, cartridge RAM and video digests; frozen before any native change | `dragster-clock-limit-idle-incomplete.inventory.json` (done) |
| What the original shows | original frame images at the limit, during the hold and at the settled result | recorded in R-0039 | PNGs under ignored `artifacts/` (done) |
| Native matches over that timeline | `dragster_playable compare --prefix` with fresh-process restores | every declared byte matches over the frozen prefix | gate reports (done) |
| Difference from ZOOM ZOO recorded | compare HUD, captions, no-time total and result screen | stated in R-0039 | R-0039 (done) |
| Four presets | `build` + `ctest` on lab-debug, lab-sanitize, app-debug, app-sanitize | all pass | preset log (done) |
| Both synthetic suites | `test --suite synthetic` | all pass | reports (done) |
| Seven DRAGSTER frozen originals | `dragster_playable compare` on each | all pass unchanged | gate reports (done) |
| M4-16 ZOOM ZOO primary gate | `zoom_zoo_playable compare` against `primary-v11` | passes unchanged | gate report (done) |
| DRAGSTER historical matrix | 20 commands (compare, restore, finish, opponent-first, presentation) | all pass with unchanged expectations | gate logs (done) |
| Independent review, CI | fresh reviewer at the exact tip; hosted CI | approve; green | CI green on all four pushed commits, latest run 35276201283 at `59efeaf` (done); review open |

## Evidence and attempts

| Attempt | Hypothesis | Experiment | Observation | Next decision |
| --- | --- | --- | --- | --- |
| 1 (2026-09-17T19:52-20:00Z) | A DRAGSTER race can be driven to the 10:00 limit within one capture, and native predicts where | Built app-debug, wrote `dragster-clock-limit-idle` (R-0038 primary inputs to 1999, then nothing to 32200) and ran the native runner over it to size the horizon | Native reached the limit at 31534, started result loading at 31775 and settled the loser result at 32016, without aborting. Horizon 32200 covers the settled result with margin | Capture the original twice at horizon 32200 with frame images around the limit, the loading edge and the result |
| 2 (20:00-20:12Z) | The two original runs agree and can be frozen before any native change | Two `dragster_playable_reference` captures in parallel; the first was relaunched because zsh does not word-split an unquoted variable, so its 32 `--frame-image` flags arrived as one argument | Both runs produced identical WRAM `52602f4a...`, SRAM `6d8782df...` and video `3b8bb965...` digests. `freeze` refused the pair: the first new result picture is 31885, where the ordinary DRAGSTER rule expects 31883 | Record it the way M4-16 recorded ZOOM ZOO's stop-timeout: add an `inventory` command for a declared incomplete original, with the exact race and loading prefix, rather than weakening `freeze` |
| 3 (20:12-20:20Z) | The shared engine already applies `$81:C73E-C75B` on DRAGSTER | `dragster_playable explore`, then a full row-by-row census of native from initialization against the original | 30,871 of 30,873 updates match every declared byte. The only two differing rows are 31882 and 31883, and only in `result.player_total`/`result.opponent_total`: native publishes `{60000, 3358}` on the ordinary mode-0 update, the original two updates later. Original events: finish 31534/3214, loading 31775, first visible 31885, loser, settled 32016 | No gameplay change. Check the presentation |
| 4 (20:20-20:30Z) | The app can draw the result the original draws | Rendered native DRAGSTER pictures at 31534, 31600, 32016 and 32100 with `dragster_race_picture_runner` and compared them with the original frames | Race frames render. Both result frames threw `unsupported Classic result composition`: the composition demands that a finished rider's five crossing digits equal its total, and a timed-out player keeps 0:00.70 against the 60000 sentinel. The original shows `MIKE` with ` NO TIME` in the same columns as the three `SOMEONE` rows | Admit that one sentinel and write the original's row |
| 5 (20:30-20:40Z) | The fix is the existing 60000 convention, not a new rule | Changed the composition, added a ROM-free `presentation_tests` case pinning the timed-out map against the ordinary loser map and the empty rows, and two rejections (an opponent sentinel, a player total below the sentinel disagreeing with its digits) | Native renders `DRAGSTER COMPLETE / PLAYER TIME / MIKE NO TIME / SOMEONE NO TIME x3`, matching the original at 32100 within the accepted declared omissions. A deliberate mutant (` NOXTIME`) aborts the test; the restored source passes | Add the prefix gate and run the regressions |
| 6 (20:40-21:20Z) | Nothing accepted regresses | Four presets, both synthetic suites, the 20-command historical matrix from task-local fixture copies, the seven DRAGSTER frozen originals and the M4-16 ZOOM ZOO primary gate on app-debug and app-sanitize, plus the new prefix gate | Everything passes; see "Gate results". One false start: the first frozen-gate run aborted on `source/binary/pack changed during validation` because I was editing docs and rebuilding presets while it ran. Committed first, then reran the whole script on a clean tree | Records, push, CI, review |

## Gate results

Fixtures are task-local copies under ignored
`artifacts/dragster-clock-limit/regressions/`; nothing outside this checkout is
read. Scripts: `artifacts/dragster-clock-limit/historical.sh` and
`artifacts/dragster-clock-limit/frozen-gates.sh` (frozen copies, not edited
while running).

| Gate | Result |
| --- | --- |
| Four presets, `ctest` | 23/23 on lab-debug, lab-sanitize, app-debug, app-sanitize; no skips |
| Both synthetic suites | 409/409 checks on each of the four presets, no skips |
| DRAGSTER historical matrix | 20/20 commands, `ANY FAILURE: 0` (compare 6, restore 4, finish 6, opponent-first 2, presentation 2) |
| Seven DRAGSTER frozen originals | pass on app-debug and app-sanitize with unchanged rows hashes and restore counts: primary 379, reversal 327, random-1 567, random-2 527, random-3 493, countdown-actions-tie 179, landing-held-roll 181 |
| M4-16 ZOOM ZOO primary gate (`primary-v11`) | passes on app-debug and app-sanitize, frames 1376-7600, 757 restores and the full restart each |
| DRAGSTER clock limit prefix gate | passes on app-debug and app-sanitize: frames 1328-31881, 30,554 rows, 36 fresh-process restores each (including 31533/31534/31535 around the limit, 31774/31775/31776 around loading, and 31880) |
| Frozen-gate script total | 18/18 commands, `ANY FAILURE: 0` |

## Mistakes

1. I ran the frozen-gate script while still editing documentation and
   rebuilding presets. `compare` hashes the working tree and the binary at
   both ends of a run, so it correctly refused the second case with
   `source/binary/pack changed during validation`. I committed everything and
   reran the whole script on a clean tree; only the rerun is reported.
2. I built the first capture's `--frame-image` list into a shell variable and
   expanded it unquoted. zsh does not word-split there, so the whole list
   arrived as a single argument and the run died immediately. Relaunched with
   the flags written out; the second capture was unaffected and both runs
   still agree.
3. I first read the race block of the 742-byte state with `lap_times` as three
   entries per rider, which put `total_times` at the wrong offset and made an
   ordinary DRAGSTER race look as though it published no totals. The header
   says ten entries per rider and twenty checkpoint flags; with the right
   offsets the ordinary and timed-out states agree with the result screen.
4. The first version of the prefix gate reused the ordinary restore set, which
   would have replayed the 30,000-update tail from several hundred boundaries.
   Replaced with a declared bounded set and said so in the tool and R-0039.

## Handoff

Branch `task/dragster-clock-limit`, worktree `.worktrees/dragster-clock-limit`,
based on `033d4a7`.

- `794fc64` freezes the original evidence: the case file, the incomplete
  inventory and `dragster_playable inventory`.
- `8894ada` writes ` NO TIME` in the DRAGSTER result after a time-out, adds the
  ROM-free test and the `compare --prefix` gate.

**The gameplay engine was already right.** The shared `update_zoom_zoo` applies
`$81:C73E-C75B` for DRAGSTER as well, and native equals the original on all 742
declared bytes for frames 1328-31881. The only native change is the result
screen row. The only remaining difference over the whole capture is the
two-update SPC700 result-loading wait at 31882-31883, which M4-16 already
recorded for ZOOM ZOO and which the contract excludes as audio timing.

Gate evidence is under ignored
`artifacts/dragster-clock-limit/gates/` (historical, frozen, synthetic-*) with
`gates/frozen/validation-ledger.jsonl`; the two originals are
`artifacts/dragster-clock-limit/idle-a` and `idle-b` with the original PNGs in
`idle-a`. Hosted CI (Ubuntu GCC `-Werror`, `synthetic.yml`) is green on every
pushed commit of this branch.

**Next step:** a fresh Claude Opus 5 independent reviewer in an isolated
checkout at the exact tip, with the frozen inventory and its two captures;
then hosted CI on the reviewed tip and the consolidated closeout. The reviewer
should re-capture the case themselves (about ten minutes per run) rather than
trust the copied fixtures, and should check the claim that only two rows of the
30,873 differ.

**Not covered, deliberately:** DRAGSTER's original `RACE` and `LOSER` captions,
its glyph clock, the opponent's on-screen finish time and the loser window
shape are recorded in R-0039 as original observations. Native's accepted
DRAGSTER renderer draws none of them, exactly as before this task; the window
follow-up in `tasks/NEXT_SESSION.md` owns that work.

## Independent review and integration

Fresh Claude Opus 5 reviewer, isolated checkout at `c00dd5d`, report
`tasks/DRAGSTER-CLOCK-LIMIT-review.md` at `7f391e1` (pushed).
**Verdict: approve.** It reproduced rather than read: two fresh ROM captures of
the clock-limit case returned the frozen digests and an `inventory` byte
identical to the tracked contract; its own comparison found the same 30,873
rows with exactly two differing (31882-31883, the result totals from the
SPC700 wait); it disassembled `$81:C6C5-C75B` and confirmed the arm has no
track test and that native's `advance_timer_digits` matches it rollover for
rollover; it probed the result relaxation fifteen ways; and it re-ran the four
presets (409/409 synthetic, 23/23 ctest), the DRAGSTER historical matrix
(20/20, both presentation contracts unchanged), the DRAGSTER primary freeze
from its own capture, the M4-16 ZOOM ZOO primary and the new prefix gate on
both app presets.

| Finding | Severity | Disposition |
| --- | --- | --- |
| 1. The relaxation is a strict extension (a consistent time never reaches the sentinel, and the clock holds at 59990) | Note | No action. |
| 2. The sentinel admission was not tied to the held clock, so a no-time total with an ordinary clock would silently draw NO TIME | Advisory | Fixed: `build_dragster_result_map` now requires the 9:59.9 clock, as `movement.cpp` and `zoom_zoo_hud` already do. The result test sets the held clock and a new case keeps the same totals rejected with an ordinary clock; dropping the conjunct fails it. |
| 3. The player's five digits are unchecked in the sentinel branch (unused for drawing, no UB) | Advisory | Recorded, not changed: the digits are not read on that path, and the clock conjunct now bounds the branch. |
| 4. Nits: a latent zero/negative-step edge in `prefix_restore_boundaries`; the prefix restart-check skip is undocumented; `rows_sha256` means different spans in prefix and inventory reports; the capture-time estimate is about ten times high | Advisory | Recorded for the next task in this area; none affects a result or a gate. |

The reviewer also corrected a loose phrase in my handoff: the opponent finished
ordinarily at 3214, not at the clock limit; R-0039 already says so.

Not assessed by the reviewer: the other six frozen DRAGSTER originals (it
re-ran `primary` from its own capture), the SPC700 handshake itself, whether a
non-idle timeline reaches the limit, and live play.
