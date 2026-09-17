# DRAGSTER-PALETTE-CYCLE - Frame-driven DRAGSTER start-line palette

## Assignment

- Status: reviewed and integrated (implementation approved at `2c9dea3` and, with the launcher fix, at `a2aac15`; reviews `223cd0d`, `913175c`); acceptance conditional on final-tip CI and remote verification in the ignored `artifacts/dragster-palette-cycle-integration/closeout.json`
- Milestone: follow-up to accepted DRAGSTER presentation (M3/M4-01); not an M4 milestone gate
- Coordinator: Claude Opus 5 primary session that is also integrating M4-16
- Task provider: Anthropic (Claude Opus 5), as recorded in D-0004 for the current work
- Worker/session/runtime/model: same primary session, Claude Code desktop, Claude Opus 5
- Actual model/reasoning effort, routing rationale: primary; follow-up the user started from an M4-16 finding
- Provider quota window: no percentage telemetry under this provider; UTC wall clock recorded instead
- Reviewer: fresh Claude Opus 5 subagent in an isolated checkout at the exact candidate, before integration
- Dependencies: M4-16 integration. It adds the race palette cycle helper and the two-track pack v7 entry, and edits the same renderer and pack registry, so implementation starts from main after M4-16 lands.
- Base commit: `f599565` (origin/main at 17 September 2026)
- Branch and isolated worktree: `task/dragster-palette-cycle`, `.worktrees/dragster-palette-cycle`
- Owned paths: DRAGSTER branch of `src/core/presentation.cpp` palette construction, DRAGSTER content rules and pack registry (additive version), related tests, this record, R-0037
- Claim/checkpoint location: this record
- Spend authorization: none needed

## Outcome and boundaries

Replace DRAGSTER's pose-keyed late-finish palette with the original race NMI
palette cycle `$82:D382-D496` (R-0037), so DRAGSTER's checkered start/finish
line animates as in the original on every race frame. Out of scope: DRAGSTER's
10:00 clock limit (separate follow-up), ZOOM ZOO (done in M4-16), audio.

## Inputs and prerequisites

Supported ROM (see R-0037 hashes); the private DRAGSTER access captures listed
in R-0037; the accepted DRAGSTER presentation fixtures and
`tests/manifests/presentation/classic-crawler-dragster-v1.json` and
`-loser-v1.json`.

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| Original rule | R-0037 capture analysis | `(n-1334)&15` predicts every first `$0B84` write in six captures | R-0037 (done) |
| Accepted contracts do not regress | `native presentation-check` winner and loser | counts unchanged or lower at every frame | reports |
| Cycle is right beyond frozen frames | original CGRAM read from strict serialized state on every frame of the winner and loser replays, twice each | every racing frame matches; loading rule matches through update 75 | R-0037 table, `artifacts/dragster-palette-cycle/cgram-*.json` |
| Visible effect matches original pictures | native renders with v1 and v7 against original pictures at frames where the window colour changes | v7 equals the original on the changed pixels where window shape agrees | R-0037, `artifacts/dragster-palette-cycle/window-check` |
| Content versioning | decision recorded; v1 packs keep accepted rendering, packs carrying the tables draw the cycle | no new pack version needed (two-track v7 carries the tables); v1 checks unchanged | R-0037, tests |
| Regressions | four-preset tests, DRAGSTER differentials and restores | all pass | gate logs |
| Independent review | fresh reviewer at the exact candidate | approve | review report |

## Evidence and attempts

| Attempt | Hypothesis | Experiment | Observation | Next decision |
| --- | --- | --- | --- | --- |
| 1 | DRAGSTER runs the ZOOM ZOO palette routine | Scan six existing DRAGSTER access captures for `$82:D382-D496` and all `$0B84/$0B92` writes | Routine runs every race frame from 1334; `$0B84` ends frame n at `(n-1333)&15` in every capture; the two fixed arrays are exactly phases 10 and 7; the accepted frames sit on those phases | Implement after M4-16 lands |
| 2 | The cycle holds on every frame, and loading freezes it | Original-only CGRAM probe on the winner and loser scenarios, each twice | Racing frames 1,854/1,854 and 1,959/1,959 match; loading holds the start frame's colours 96-111 with black colour 0 until the result palettes load at loser update 76 | Implement with v1 compatibility |
| 3 | Implementation keeps accepted checks and draws the cycle with tables present | Four presets; v1 winner/loser presentation checks; same cases with pack v7 through the checker's comparison; mutation of the start frame | 22/22 tests on four presets; counts unchanged in both; mutant fails the new test | Independent review |

| 4 | (Review round 1) The cycle is visible in native renders | Review swept every native frame 1533-3679 with v1 and v7 | **No pixel differed**: native uses only phase-invariant colours from 96-111, and the original shows the cycle through colour 0 in the GO and winner windows, which native filled with fixed colours. The checkered-line claim was wrong. The acceptance row had been weakened to CGRAM only | Fill windows from the cycled colour 0; restore a picture-level check |
| 5 | Windows show colour 0 | Render-level test; v1 and v7 renders against original pictures on the review's native states | 61,239 of 75,015 changed pixels equal the original (0 before), most on black loading frames; racing-phase winner window 5,001/5,001 at 3452 and 823/823 overlapping at 3322; v1 and v7 frozen counts unchanged; test fails if the windows ignore the tables | Re-review |

### Independent review, round 1

Fresh Claude Opus 5 reviewer, `.worktrees/dragster-palette-review` at
`a685712`, report `tasks/DRAGSTER-PALETTE-CYCLE-review.md` at `5d5cd7d`
(10:22-10:54 UTC). Independently confirmed: the CGRAM offset (by locating VRAM
and a controlled palette edit), the racing rule on three scenarios including
one I did not use (2,120/2,120, 2,225/2,225, 1,666/1,666), loading start and
the update-75 boundary in both outcomes, the disassembly, unchanged v1 and v7
presentation counts, 406/406 on four presets, the M4-16 primary gate, the ZOOM
ZOO refactor over frames 0-200,000, and CI run 35210129769. **Verdict: changes
required.**

| Finding | Severity | Disposition |
| --- | --- | --- |
| F1: the recovered cycle has no visible effect in native renders; the visible effect is colour 0 in the GO and winner windows, which native drew in fixed pose-keyed colours; the checkered-line claim is wrong; the pixel acceptance row was weakened; no render-level test | High | Fixed. Windows take the cycled colour 0 when the tables are present; R-0037 corrected; picture-level check restored with results above; render-level test added and mutation-checked. Window timing and shape stay pose-gated, recorded as outside this task. |
| F2: loading arithmetic wraps if the loading counter exceeds the frame | Low | Fixed: the palette is left unchanged in that unreachable case. |
| F3: comment said the routine stops at loading update 1; winner only documented to update 17; loading rule invisible | Low | Corrected in the code comment and R-0037. |
| F4: saving state every frame perturbs emulation depending on start frame | Info | Recorded; the probes started at 1600, where the review found video identical and only loading stack bytes affected. |

### Independent re-review, round 2

Same reviewer, `2c9dea3`, report section at `223cd0d` (10:59-11:09 UTC).
**Verdict: approve.** Independently verified:
- **Winner window against original pictures:** v7 matches every overlapping pixel at 3322 (823/823) and 3452 (5,001/5,001), and v1 matches none.
- **GO window:** checked through relabelled states only, since accepted runs draw it natively only at 1600.
- **Full-race sweep:** v1 and v7 now differ on 226 of 2,147 frames, where round 1 found 0.
- **Frame 3452 contract-style score:** falls from 5,654 to 653 mismatches.
- **Accepted counts:** unchanged with v1 and v7, and the renders are byte-identical in all eight frozen cases.
- **Mutation checks:** five window mutants fail the test.
- **ZOOM ZOO:** unchanged over frames 0-200,000; the M4-16 primary gate passes.
- **Presets and CI:** 406/406 on four presets; CI run 35213305070 green on both platforms.

Integration fixes it requested, applied before merge:
- **Loading-screen wording in R-0037:** the original screen is black only on loading updates 1-108, and the 3600/3677 mismatches are native drawing the race until update 225.
- **Stale header comment** in `presentation.hpp`.
- **A test for the loading-counter guard.**

**My own correction while applying them:** R-0037 and attempt 5 had described the winner window as matching "at every sampled phase". Most of those frames are loading frames where both screens are black. The racing-phase evidence is 3452 and the overlap at 3322, and the records now say so.

### Launcher gap found by the integration gates

The affected gates on the integration candidate `829a7c9` passed everything
except one hidden launch: `frontend run --track dragster --pack
local/classic-crawler-two-tracks-v7.pack` exited 3, "extraction-rules identity
is incompatible". The app binary accepts the two-track pack for DRAGSTER (exit
0), but the launcher checked every existing DRAGSTER pack against the v1 rules.
The approved change was therefore unreachable through the documented launcher,
and my records' claim that launching DRAGSTER with the two-track pack gives the
recovered palette was true only for the bare app. The reviewer's checks
rendered headlessly and could not see this.

Fix: when `--track dragster` uses the default DRAGSTER rules and an existing
pack fails any of their checks, the launcher validates it against the two-track
rules instead. If that also fails for a reason other than identity, it reports
that more specific diagnosis. Launcher checks
(`artifacts/dragster-palette-cycle/launcher-check.sh`):

| Case | Result |
| --- | --- |
| DRAGSTER, v1 pack | exit 0 |
| DRAGSTER, two-track v7 pack | exit 0 (was 3) |
| ZOOM ZOO, v7 pack | exit 0 |
| ZOOM ZOO, DRAGSTER v1 pack | exit 3, identity incompatible (unchanged) |
| DRAGSTER, v7 pack with one byte flipped | exit 3, entry payload hash differs |
| DRAGSTER, v1 pack with one byte flipped | exit 3, entry payload hash differs |

This is a new change after approval, so it goes back to the reviewer before
integration.

### Independent re-review, round 3 (launcher)

Same reviewer, `a2aac15`, report section at `913175c` (11:21-11:34 UTC).
**Verdict: approve.**
- **19 launcher cases behaved correctly:** fallback only with `--track dragster` and the default rules path, including symlinks; no fallback for a copied rules file or in reverse; a missing pack with `--rom` still extracts a byte-identical v1 pack; corrupt, truncated, wrong-source and non-pack inputs are refused.
- **The launcher reaches the new colours at other phases:** redraw counters from 2,146-update hidden launches matched its harness prediction exactly for both packs. v1 gave 382 identical redraws (longest run 225), and v7 gave 380 (223), the difference coming from frames 3452 and 3454.
- **No regressions:** v1 counts are unchanged and v1/v7 frozen renders byte-identical; `zoom_zoo_runner` is byte-identical; 406/406 on four presets; CI run 35215252623 green.

Residuals, closed before integration:
- **R3-1, medium: no automated test ran the fallback.** `test_dragster_launch_accepts_a_valid_two_track_pack` now authors DRAGSTER and two-track rule sets and checks four behaviours: acceptance, no fallback for a non-default rules path, a missing pack still needing a ROM, and a corrupt two-track pack reporting its own failure. It fails against the pre-fix launcher.
- **R3-2, wording:** the fallback follows any v1 failure, not only identity. Corrected in R-0037 and above.
- **R3-3, wording:** STATE said the windows "animate"; they take the cycled colour 0. Corrected.

**Decision (17 September 2026):** no new DRAGSTER pack version. The two-track
pack v7 already carries every DRAGSTER entry and the cycle tables, so DRAGSTER
draws the recovered cycle from it. DRAGSTER v1 packs keep the accepted
pose-keyed palette, so no accepted contract, fixture or historical gate changes.
The default DRAGSTER-only pack is unchanged; launching DRAGSTER with the
two-track pack gives the recovered palette. A later DRAGSTER pack version could
add the tables if the DRAGSTER-only pack remains a supported product path.

## Handoff

- Current head: implementation, tests, R-0037 and this record on `task/dragster-palette-cycle`.
- Verified findings: R-0037.
- Next: re-review of the corrections, then integration with CI on the exact tip.
- Not done: recovering when the GO and winner windows appear and their shapes; the palette after loading update 75.
