# ZOOM-ZOO-OPPOSING-INPUT - opposing directions on ZOOM ZOO, measured against the original

## Assignment

- Status: reviewed and integrated (implementation `7b4ab4a`, corrections `024bf56`; review `48b2089` approve at `b477a0d`, re-review `2bde823` confirm at `024bf56`, no blocking finding in either round; the four residual items applied on top and integrated by fast-forward of `task/zoom-zoo-opposing-input` onto `main`); acceptance conditional on the final-tip CI and remote verification recorded in the ignored closeout `artifacts/zoom-zoo-opposing-integration/closeout.json` in the main checkout; if absent, `git log --first-parent main -- tasks/ZOOM-ZOO-OPPOSING-INPUT.md` and `gh run list --workflow synthetic.yml --commit <commit>`. Started 19 September 2026 03:00 UTC from `main` at `8acee91`.
- Milestone: follow-up 2 in [NEXT_SESSION](NEXT_SESSION.md), the only open item left
  by DRAGSTER-ORDINARY-CONTROLS and CLASSIC-PRESENTATION-UNIFICATION
- Coordinator: main session
- Task provider: Anthropic (unchanged)
- Worker/session/runtime/model: Claude Code, Claude Opus 5 (the primary chose the task,
  measured the original, implemented and gated it)
- Actual model/reasoning effort, routing rationale and frontier escalation question: Opus 5 as
  the session's model; no frontier consultation needed - the question is answered by the
  reference core's own gamepad source and a bounded original capture
- Provider quota window/baseline (D-0004): task start 03:00 UTC five-hour 0%, weekly all
  models 45%, weekly Fable 36%; D-0004 reserve 20% of the weekly allowance, checkpoint after
  a 20-point rise; no reset, purchase or provider change
- Reviewer: fresh Claude Opus 5 subagent in an isolated checkout at the exact candidate,
  spawned by the primary
- Dependencies and evidence of acceptance: R-0038 (the physical D-pad finding for DRAGSTER),
  M4-16 (the ZOOM ZOO playable gate and its originals), CLASSIC-PRESENTATION-UNIFICATION
  (one shared race engine for both tracks); all integrated on `main`
- Base commit: `main` at `8acee91`
- Branch and isolated worktree: `task/zoom-zoo-opposing-input`, `.worktrees/zoom-zoo-opposing-input`
- Owned paths: `src/core/movement.cpp`, `src/core/zoom_zoo_movement.hpp`,
  `src/core/zoom_zoo_runner.cpp`, `src/app/sdl_main.cpp`, `src/app/dragster_fuzz_runner.cpp`,
  `tests/native/zoom_zoo_*`, `tests/native/dragster_race_tests.cpp`,
  `tests/manifests/native/zoom-zoo-playable-opposing-*`,
  `tools/unirally_lab/native/zoom_zoo_playable_reference.py`, this record,
  `docs/research/R-0041-opposing-directions.md`, the coordinator records
- Claim/checkpoint: this record and ignored `artifacts/zoom-zoo-opposing-input/` in the
  worktree (moved to `local/evidence/zoom-zoo-opposing-input/` at closeout)

## Outcome and boundaries

Close the last open follow-up: native ZOOM ZOO must answer opposing directions
(Left with Right, Up with Down) the way the original console does, with an
original capture over a complete race behind the claim rather than a smoke test.

In scope: the shared race engine's treatment of opposing directions on both
tracks, the runner and app input boundaries, frozen ZOOM ZOO originals with
opposing directions over complete races, and the regression evidence that no
accepted contract moves.

Out of scope: every declared omission in `docs/STATE.md` (audio, decorative
objects and captions, the original HUD and result pixel style, the two-update
late result load after a time-out), the `zoom.*` pack aliases, and any other
input question - this task covers opposing directions only.

## Inputs and prerequisites

- PAL ROM `a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`
  through the private locator `local/rom-location.txt`.
- Reference core `local/emulators/bsnes/bsnes/out/bsnes_libretro.dylib`,
  SHA-256 `e59bf88d4fc922c9fe3b5438e65ff3a6909d24e1628f0f87141c8de17699a91b`.
- Pack `local/classic-crawler-two-tracks-v8.pack`, profile
  `classic.pal.crawler.two-tracks.v8` (56 entries). No pack change is expected:
  this task adds no content.
- Existing evidence read, not regenerated: the M4-16 originals under
  `local/evidence/m4-16-playable-zoom-zoo/m4-16` and the DRAGSTER originals under
  `local/evidence/dragster-ordinary-controls/dragster-ordinary-controls/originals`.
- No known baseline failure: `ctest --test-dir build/app-debug` is 23/23 at `8acee91`.

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| The original drops opposing directions | `python3 artifacts/zoom-zoo-opposing-input/dpad_probe.py` | Left+Right and Up+Down runs have per-frame WRAM identical to a neutral D-pad and different from one direction alone | `dpad-probe.json` |
| Native ZOOM ZOO matches the original over a complete race with opposing directions | `zoom_zoo_playable freeze` on two captures, then `zoom_zoo_playable compare` | every 742-byte row and every restore agrees | `opposing-*.freeze.json` contracts and gate reports |
| A withheld opposing case, chosen by the reviewer or after the fix | same commands on a case not used to develop the change | agreement without retuning | reviewer's report |
| No accepted contract moves | M4-16 primary and idle gates; the DRAGSTER primary, random-1, reversal, random-3, regression-landing-held-roll and regression-countdown-actions-tie gates (the last three added after review finding A3, being the accepted timelines that hold opposing pairs most densely); v1 presentation contracts | all `status=passed`, identical restore counts | gate logs |
| Suites and fuzz | `ctest` on five presets, synthetic suite, hidden runs, `dragster_fuzz_runner` | all pass, 0 aborts | gate logs |
| Hosted CI on the final tip | `gh run list --workflow synthetic.yml --commit <tip>` | both platforms success | closeout |

## Capability and coverage checkpoint

- Native capability delivered: both tracks answer opposing directions the way the console's
  controller port does, measured against three frozen originals over complete races. Still
  missing: nothing in this task's scope; the declared omissions in `docs/STATE.md` are unchanged.
- Frozen exact-match interval, field set and reference/seed identity: the opposing cases'
  frames, 742-byte `URZZ000B` rows, ROM/core/manifest identities in each contract.
- Dynamic captured inputs still consumed: zero (the controller timeline is the case's own).
- Relevant branches/transitions exercised: countdown, riding, both directions, the pause
  menu's vertical navigation, finish and result.
- First divergence and cheapest next discriminating experiment: the first divergence was native
  ZOOM ZOO at update 1650 of the probe window (attempt 2); none remains.
- Trial-wide usage baseline/current: in the Assignment block and the closeout.

## Evidence and attempts

| Attempt | Hypothesis | Experiment | Observation | Next decision |
| --- | --- | --- | --- | --- |
| 1 (03:03-03:12Z) | The original cannot see opposing directions, so native ZOOM ZOO's pass-through is a divergence | `dpad_probe.py`: five bounded original runs (neutral, Left+Right, Up+Down, Left, unchanged) sharing the M4-15 primary timeline, window 1650-2400, per-frame WRAM digests | Left+Right and Up+Down are byte-identical to neutral on all 1,025 frames; Left alone and the unchanged timeline differ from frame 1650. The reference core's `sfc/controller/gamepad` publishes `left & !right` and `up & !down` with the comment that the D-pad physically prevents both | Measure native |
| 2 (03:12-03:15Z) | Native DRAGSTER drops them and native ZOOM ZOO does not | `native_dpad_probe.py`: the same window through `zoom_zoo_runner` on both tracks | ZOOM ZOO diverges from neutral at 1650 for both Left+Right and Up+Down; DRAGSTER is identical to neutral, as `with_physical_dpad` at its call sites intends | Apply the rocker rule in the shared engine, then capture complete originals |
| 3 (03:15-03:20Z) | The rule belongs in the engine, where no caller can forget it | `update_zoom_zoo` applies `with_physical_dpad` after the recovered-domain guard (which keeps reading the requested buttons); the runner, app and fuzz runner stop applying it; ROM-free engine tests for both tracks | The native probe now matches a released pad on both tracks; ctest 23/23 | Freeze complete originals |
| 4 (03:15-03:17Z) | A mid-race opposing window still finishes the race | `case_fit.py` over the native engine: replace riding frames of the M4-15 marker-guided timeline with an opposing pair | Even a five-update window desynchronizes that scripted steering and the rider never finishes inside the horizon; the accepted `constant-left`/`constant-right` inventories are incomplete for the same reason | Hold the pair where the released-pad counterpart is an accepted complete race: the capture tool's `idle` window, and the primary's own neutral frames |
| 5 (03:17-03:20Z) | The original cannot tell an opposing pair from a released pad over a complete race | Three cases captured twice each (`opposing-ride`, `opposing-axes`, `opposing-edges`), frozen with `zoom_zoo_playable freeze` | Each pair of captures is identical. `opposing-ride`'s rows equal the **accepted** idle late-start contract (`205d1705...`) and `opposing-edges`' equal the **accepted** M4-16 primary (`b4a34af7...`), although both timelines hold opposing directions and their timeline hashes differ | Gate native against them |
| 6 (03:20-04:04Z) | Native reproduces those races byte for byte, and nothing accepted moves | `zoom_zoo_playable compare` on the three cases; the accepted M4-16 and DRAGSTER gates; five preset suites; synthetic; v1 contracts; hidden runs including masks 192 and 48; abort fuzz | All eight differential gates `status=passed` at `7b4ab4a` with an empty working diff: opposing 801/781/757 restores, M4-16 757/801, DRAGSTER 379/567/327 (each equal to its previously recorded count). ctest 23/23 on five presets, synthetic passed, both v1 contracts passed, hidden runs rc=0 with 0 pose fallbacks, fuzz 79 races 0 aborts | Record and send for independent review |

## Evidence locations

Ignored, moved to the main checkout at closeout:

| What | Path under `local/evidence/zoom-zoo-opposing-input/zoom-zoo-opposing-input/` |
| --- | --- |
| probes and their reports | `dpad_probe.py`, `native_dpad_probe.py`, `port_words_probe.py`, `case_fit.py` with `dpad-probe.json`, `port-words-probe.json`, `native-dpad-probe-{before,after}.json` |
| original captures (6, about 5.1 GB) | `originals/{ride,axes,edges}-{a,b}` |
| gate scripts and logs | `gates-{a,b}.sh`, `gates-{a,b}.log`, `gates/` (the final tip) and `gates-b477a0d/` (the first candidate) |
| the reviewer's own evidence (11 captures of its own, 12 gates, its withheld case) | `local/evidence/zoom-zoo-opposing-review/review/` |

Tracked: the three cases and their `-v11.freeze.json` contracts under
`tests/manifests/native/`.

## Handoff

- Current base/head commit and uncommitted state: base `main` `8acee91`; code and cases at
  `7b4ab4a`, documentation on top; no uncommitted tracked changes.
- Verified findings: the six attempts above and [R-0041](../docs/research/R-0041-opposing-directions.md).
- Current hypothesis and failed approaches: settled. The failed approach worth keeping is
  attempt 4: opposing windows spliced into the marker-guided riding script never finish, so
  riding coverage over a complete race comes from the `idle` window, not from `changes`.
- Commands executed, outcomes and report hashes: `artifacts/zoom-zoo-opposing-input/gates-{a,b}.log`
  and the per-gate reports listed above; every differential report pins `source_commit`
  `7b4ab4a7` and an empty `source_diff_sha256`.
- Unavailable/skipped checks: none. The pause menu's vertical navigation with Up+Down is covered
  by the ROM-free engine tests only; no captured original presses both there.
- Exact next experiment/command: none outstanding for this task. To re-run the gates from a fresh
  checkout at this commit, `bash artifacts/zoom-zoo-opposing-input/gates-b.sh` then `gates-a.sh`
  after `mkdir -p artifacts/zoom-zoo-opposing-input/gates` and copying the v1 fixtures to
  `local/v1-fixtures` (the presentation check requires fixtures below `local/` or `artifacts/`).
- Remaining dependencies: none.
- Closeout, as the retention rule in AGENTS.md requires. Moved into the main checkout:
  this task's `artifacts/zoom-zoo-opposing-input/` to
  `local/evidence/zoom-zoo-opposing-input/zoom-zoo-opposing-input/` (5.1 GB: the four probe
  scripts and their reports, the six original captures, the three gate directories) and the
  reviewer's `artifacts/review/` to `local/evidence/zoom-zoo-opposing-review/review/` (9.6 GB:
  its eleven own captures, twelve gate reports and its withheld case). The two gate scripts
  were repointed at that location and say how to recreate a checkout to run them. Deleted:
  all three worktrees with their build output (`zoom-zoo-opposing-input`,
  `zoom-zoo-opposing-review` and the throwaway `opposing-before` built at `c2de73e` for the
  before measurement), and the local branches `task/zoom-zoo-opposing-input` and
  `review/zoom-zoo-opposing-input` after a patch-id audit - `git cherry main
  origin/<branch>` shows no `+` commit for either, and both are on `origin`. No capture any
  record cites was deleted; the six captures this task made are all retained. The `c2de73e`
  build behind `native-dpad-probe-before.json` is not retained and R-0041 says how to
  recreate it.
- Runtime needs: the private ROM through `local/rom-location.txt`, the audited bsnes core, the
  v8 pack, about 6 GB of disk for the captures and roughly 45 minutes for the whole gate matrix.
- Aggregate time and provider usage: task start 03:00 UTC (five-hour 0%, weekly 45%, weekly
  Fable 36%); readings at the candidate and at integration are in the closeout.
- Accepted outcome, review/fix rounds and next routing decision: pending independent review.

## Review and integration

- Reviewer and independent reproduction: a fresh Claude Opus 5 subagent with no inherited
  conversation, in the isolated checkout `.worktrees/zoom-zoo-opposing-review`
  (branch `review/zoom-zoo-opposing-input`) at the exact candidate `b477a0d`, spawned by the
  primary. Verdict **approve**, no blocking finding, report `f76a81e`, 66 minutes.
  It recaptured all six originals from the ROM rather than reading the primary's, ran twelve
  differential gates (the three opposing cases 801/781/757, M4-16 757/801, DRAGSTER
  primary/random-1/reversal 379/567/327 and three further DRAGSTER contracts whose own
  timelines hold opposing pairs, 493/181/179), reproduced all three probes, the five preset
  suites at 23/23, the synthetic suite, both v1 contracts, six hidden runs and the fuzz, and
  checked both row-equality claims offline from the tracked contracts alone.
- Withheld case: `review-pause-opposing-saturated`, built from the accepted `pause-shifted`
  race (a countdown pause with menu navigation, the branch this task's three cases never
  reach) by adding the opposing pair to every axis-neutral frame from 1377 to 7600 - 6,222
  vertical and 1,141 horizontal. It predicted offline that the port publication is unchanged,
  then captured twice identically and froze: `rows_sha256 5d83cbdd...`, exactly the accepted
  contract, with a different timeline. Native compare passed with 777 restores. A second probe
  held Up+Down for 1,200 updates from 3240 against the same window released: identical 7,025-row
  streams from different timelines, and neither finishes, confirming attempt 4 with no
  counterexample.
- Required changes and their disposition (no blocking finding; all five should-fix items and
  every advisory are applied in the correction commit):

| # | Class | Finding | Disposition |
| --- | --- | --- | --- |
| S1 | should-fix | `NEXT_SESSION.md` at the candidate already claimed the task was integrated with a closeout that does not exist | Corrected to the state at that commit; the integrated wording and the closeout path go in the integration commit, as the workflow's consolidated closeout intends |
| S2 | should-fix | `native-dpad-probe.json` recorded post-fix results stamped with the pre-fix commit, so attempt 2's divergence was not reproducible from the artifact | The probe now takes the runner to measure and records its path, SHA-256 and the commit it was built from. Both builds are measured: `native-dpad-probe-before.json` from a build at `c2de73e` (ZOOM ZOO differs at 1650 on both axes) and `native-dpad-probe-after.json` from the candidate (identical on both tracks). R-0041 carries the table |
| S3 | should-fix | the declaration of `update_zoom_zoo` did not say the engine applies the rocker itself | The header now names the parameter `requested_buttons` and says what the engine does with it |
| S4 | should-fix | R-0041 said the both-bits behaviour is unrecovered while `sample_controller` carries a recovered contradictory-direction precedence, still live on `update_movement` | R-0041 now has "The legacy path keeps its own answer, deliberately": what that precedence does, why the two entry points differ, and why the precedence is not evidence about the port |
| S5 | should-fix | records cited `local/evidence/zoom-zoo-opposing-input/...` as present fact before closeout | Both records now say the evidence is in the task worktree until closeout |
| A1 | advisory | "unreachable on the console" overreaches | Reworded everywhere to a standard rocker pad through the audited core, and R-0041's limits now carry the reviewer's own caveat: the core's gamepad is the only path to the ROM and already drops the pairs, so measurement 1 is close to a tautology; what it establishes is that nothing else in the machine leaks the raw request |
| A2 | advisory | the `idle` variation's held `buttons` were unconstrained, so `["left"]` would give a non-idle "idle" window | The variation now refuses any held set the port would publish: each axis must be both or neither, and nothing else may be held. Every tracked case, including the two accepted ones, still produces its contract's `timeline_sha256` |
| A3 | advisory | the DRAGSTER contracts whose timelines hold opposing pairs most densely were not re-gated | Added to this task's gate set and run here as well as by the reviewer: `regression-landing-held-roll`, `random-3` and `regression-countdown-actions-tie` |
| A4 | advisory | `requested` was not purely the request inside `update_zoom_zoo` | Renamed to `published`, with the comment saying the fade gate has already zeroed it and that the guard deliberately reads it before the rocker |
| A5 | advisory | the three case manifests were inconsistently formatted and none named the contract its rows should equal | All three are now one compact line like the accepted idle case, with the parsed variation unchanged; the expected equalities are recorded in `docs/BUILD_AND_VALIDATION.md` and R-0041, with the reason a case file cannot carry them |

- Re-review: the same reviewer, at `024bf56`, verdict **confirm**, report `f53f42b` appended to its
  report, 33 minutes (99 minutes of review across both rounds). It re-derived the S2 "before"
  measurement itself by checking out `c2de73e`, building it (a distinct runner SHA-256) and running
  the corrected probe against that binary; exercised the A2 guard over ten held sets and confirmed
  all six tracked cases still produce their contracts' `timeline_sha256`; and demonstrated A5's
  justification by injecting an annotation key into a capture and watching `original_sha256` move.
  On its own build at `024bf56`: five presets 23/23, synthetic, both v1 contracts, four hidden runs,
  the fuzz, and four differential gates including its withheld case at 777 restores.
- Its four residual items, applied in the final commit:

| # | Class | Finding | Disposition |
| --- | --- | --- | --- |
| R1 | should-fix | `docs/STATE.md` still said "an input the console cannot produce", the same unmeasured hardware premise A1 removed elsewhere, in the summary a fresh agent reads first | Now "an input no rocker pad can deliver" |
| R2 | advisory | the acceptance table listed the gate set without the three DRAGSTER contracts added for A3 | The table names all six DRAGSTER contracts and says why the last three are there |
| R3 | advisory | `published` named the request *before* the rocker, while what the port publishes is `buttons`, after it | Renamed to `gated_request`, and the comment now says `buttons` is the publication and the guard deliberately reads the request. The rename is provably behaviour-neutral: compiling `movement.cpp` with `-O2 -g0` before and after gives byte-identical object code, SHA-256 `6fd8a490...`, so no gate result can move. The suites and three differential gates were rerun anyway |
| R4 | advisory | the new `BUILD_AND_VALIDATION.md` paragraph ran into the pre-existing sentence | Split |

- Exact merge candidate and required-check results: the merge candidate is the tip of
  `task/zoom-zoo-opposing-input`. Its full matrix ran at `024bf56` (eleven differential gates, five
  preset suites, synthetic, both v1 contracts, six hidden runs, fuzz); the only source change after
  that is R3's rename, whose object code is identical, and the suites and the opposing-ride, M4-16
  primary and DRAGSTER random-1 gates were rerun on the final tip to confirm it.
- Integrated commit and evidence location: recorded in the integration commit and the ignored
  closeout `artifacts/zoom-zoo-opposing-integration/closeout.json` in the main checkout.
- Remote synchronization: recorded in the closeout.
- Scope still unverified: what the game's branches would do with both bits set at the port, which no
  path this project has to the ROM can present; the pause menu's vertical navigation with an opposing
  pair is covered by the reviewer's saturated case and the ROM-free engine tests, not by a
  primary-captured original of its own.
