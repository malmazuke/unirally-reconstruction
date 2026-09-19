# ZOOM-ZOO-OPPOSING-INPUT - opposing directions on ZOOM ZOO, measured against the original

## Assignment

- Status: in_progress. Started 19 September 2026 03:00 UTC from `main` at `8acee91`.
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
  `tests/manifests/native/zoom-zoo-playable-opposing-*`, this record,
  `docs/research/R-0041-*`, the coordinator records
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
| No accepted contract moves | M4-16 primary and idle gates, DRAGSTER primary/random-1/reversal gates, v1 presentation contracts | all `status=passed`, identical restore counts | gate logs |
| Suites and fuzz | `ctest` on five presets, synthetic suite, hidden runs, `dragster_fuzz_runner` | all pass, 0 aborts | gate logs |
| Hosted CI on the final tip | `gh run list --workflow synthetic.yml --commit <tip>` | both platforms success | closeout |

## Capability and coverage checkpoint

- Native capability delivered / still missing: to be recorded at the candidate.
- Frozen exact-match interval, field set and reference/seed identity: the opposing cases'
  frames, 742-byte `URZZ000B` rows, ROM/core/manifest identities in each contract.
- Dynamic captured inputs still consumed: zero (the controller timeline is the case's own).
- Relevant branches/transitions exercised: countdown, riding, both directions, the pause
  menu's vertical navigation, finish and result.
- First divergence and cheapest next discriminating experiment: recorded per attempt below.
- Trial-wide usage baseline/current: in the Assignment block and the closeout.

## Evidence and attempts

| Attempt | Hypothesis | Experiment | Observation | Next decision |
| --- | --- | --- | --- | --- |
| 1 (03:03-03:12Z) | The original cannot see opposing directions, so native ZOOM ZOO's pass-through is a divergence | `dpad_probe.py`: five bounded original runs (neutral, Left+Right, Up+Down, Left, unchanged) sharing the M4-15 primary timeline, window 1650-2400, per-frame WRAM digests | Left+Right and Up+Down are byte-identical to neutral on all 1,025 frames; Left alone and the unchanged timeline differ from frame 1650. The reference core's `sfc/controller/gamepad` publishes `left & !right` and `up & !down` with the comment that the D-pad physically prevents both | Measure native |
| 2 (03:12-03:15Z) | Native DRAGSTER drops them and native ZOOM ZOO does not | `native_dpad_probe.py`: the same window through `zoom_zoo_runner` on both tracks | ZOOM ZOO diverges from neutral at 1650 for both Left+Right and Up+Down; DRAGSTER is identical to neutral, as `with_physical_dpad` at its call sites intends | Apply the rocker rule in the shared engine, then capture complete originals |

## Handoff

- Current base/head commit and uncommitted state: recorded at each checkpoint below.
- Verified findings: attempts 1 and 2 above.
- Current hypothesis: the rule belongs in the shared engine, where every caller gets it, with
  the historical recovered-domain guard still reading the requested buttons.
- Commands executed, outcomes and report hashes: in `artifacts/zoom-zoo-opposing-input/`.
- Unavailable/skipped checks: none so far.
- Exact next experiment/command: capture the opposing cases twice each and freeze them.
- Remaining dependencies: none.

## Review and integration

To be completed by the primary after independent review.
