# CLASSIC-STUNT-NAMES - the original's on-screen stunt names

## Assignment

- Status: in_progress on `task/classic-stunt-names` from `fd34209`. Registered and started
  19 September 2026 07:00 UTC, chosen by the user as the next task after ZOOM-ZOO-OPPOSING-INPUT.
- Milestone: follow-up to M4-16 and CLASSIC-PRESENTATION-UNIFICATION; takes the first item out of
  the "decorative objects and captions" declared omission in [docs/STATE.md](../docs/STATE.md)
- Coordinator: main session
- Task provider: Anthropic (unchanged)
- Worker/session/runtime/model: Claude Code, Claude Opus 5
- Actual model/reasoning effort, routing rationale and frontier escalation question: Opus 5 as the
  session's model; a frontier consultation is not expected - the mechanism is read from captures
  and the ROM, as R-0040 and R-0041 were
- Provider quota window/baseline (D-0004): registration 07:00 UTC five-hour 33%, weekly all models
  49%, weekly Fable 36%; D-0004 reserve 20% of the weekly allowance, checkpoint after a 20-point
  rise from the baseline recorded when work starts; no reset, purchase or provider change
- Reviewer: fresh Claude Opus 5 subagent in an isolated checkout at the exact candidate, spawned by
  the primary, as D-0006 requires
- Dependencies and evidence of acceptance: M4-16 (the reward events behind the names are recovered;
  their text display is not), R-0036 (rider objects and look tables), R-0040 and
  ZOOM-ZOO-WINDOW-EFFECTS (the channel-6 window family, and the finding that these captions are OBJ
  or BG content rather than windows), CLASSIC-PRESENTATION-UNIFICATION (one renderer for both
  tracks, track content selected from the pack); all integrated on `main`
- Base commit: `main` at `260334d`
- Branch and isolated worktree: `task/classic-stunt-names`, `.worktrees/classic-stunt-names`
- Owned paths (expected; the recovery may move the boundary): `src/core/presentation.{hpp,cpp}`,
  `src/core/rider_look.{hpp,cpp}`, `src/core/classic_race_presentation_runner.cpp`, the content
  pack rules under `tests/manifests/content/` if new entries are needed,
  `tests/native/presentation_tests.cpp`, `tests/native/rider_presentation_tests.cpp`, this record,
  a new `docs/research/R-0042-*`, the coordinator records
- Claim/checkpoint: this record and ignored `artifacts/classic-stunt-names/` in the worktree, moved
  to `local/evidence/classic-stunt-names/` at closeout

## Outcome and boundaries

Draw the original's on-screen stunt names: the captions that name a trick the rider has just
completed. The user raised their absence during live play of M4-16, and the reward events that
drive them are already recovered, so what is missing is when a name appears, which name, where it
is drawn, how it moves or fades, and how long it lasts - measured against the original, not
authored.

In scope: the mechanism behind the captions, whichever of OBJ or BG carries them; the glyph or tile
content they need, added to the pack additively if the existing entries do not already carry it;
native drawing in the shared renderer for both tracks; and frozen original captures behind every
claim.

Out of scope unless the recovery shows they are the same mechanism and come almost free with it:
the start direction arrow, the start ring, the red `MORE STUNTS` hints, the opponent's finish time,
the WINNER caption, the off-screen rider arrows, the animated finish banner and the result-screen
art. Also out of scope: audio, the original HUD and result pixel style, the two-update late result
load after a time-out, and the `zoom.*` pack aliases. If a neighbouring caption shares the
mechanism, take it and say so; do not widen the task to the whole family by default.

## Inputs and prerequisites

- PAL ROM `a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e` through the private
  locator `local/rom-location.txt`; audited bsnes core
  `e59bf88d4fc922c9fe3b5438e65ff3a6909d24e1628f0f87141c8de17699a91b`.
- Pack `classic.pal.crawler.two-tracks.v8` (56 entries). A new profile is only justified if the
  captions need content the pack does not carry; a bump is additive and keeps the accepted v1
  contracts, as v7 and v8 did.
- Existing originals to read before capturing anything new: the M4-16 ZOOM ZOO captures under
  `local/evidence/m4-16-playable-zoom-zoo/m4-16` (whole WRAM per frame, with frame images at the
  kept frames), the DRAGSTER originals under
  `local/evidence/dragster-ordinary-controls/dragster-ordinary-controls/originals`, and the
  window-pause and opposing-direction captures. A race that performs tricks is needed: the M4-15
  primary timeline steers only, so the trick-carrying timelines are the DRAGSTER primary (held and
  tapped B jumps, short and long X rolls, L, R and A+R in the air) and the M4-16 trick probes.
- No known baseline failure: `main` at `260334d` has green CI on both platforms and 23/23 ctest on
  five presets.

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| The trigger is recovered | probe the original's WRAM across a trick-carrying capture, against the recovered reward events | a stated rule for when a caption starts, which name it carries and when it ends, with the addresses and the frames behind it | research record R-0042 |
| The content is authenticated | extract whatever glyph or tile content the captions use from the ROM through the pack rules | byte-exact extraction, additive to the profile, v1 contracts unchanged | pack report |
| Native matches the original where it draws | `classic_race_presentation_runner` pictures against the original's own frames on a trick-carrying race | every pixel the change touches matches the original on those frames; accepted frames untouched | picture scores |
| No accepted contract moves | the eleven differential gates, five preset suites, synthetic, v1 contracts, hidden runs, fuzz | all `status=passed`, restore counts unchanged | gate logs |
| Independent review | fresh Opus 5 subagent in an isolated checkout at the candidate | approve, with its own withheld case | review report |
| Hosted CI on the final tip | `gh run list --workflow synthetic.yml --commit <tip>` | both platforms success | closeout |

## Capability and coverage checkpoint

- Native capability delivered / still missing: to be recorded at the candidate.
- Frozen exact-match interval, field set and reference/seed identity: to be declared with the first
  frozen case.
- Dynamic captured inputs still consumed (must be zero for autonomy): expected zero; the captions
  are presentation, driven by the 742-byte state and the pack.
- Relevant branches/transitions exercised: at least one caption start, its life and its end, on both
  tracks, plus a race with no trick at all.
- First divergence and cheapest next discriminating experiment: to be recorded per attempt.
- Trial-wide usage baseline/current, reserve, reset authorization: in the Assignment block and the
  closeout.

## Evidence and attempts

| Attempt | Hypothesis | Experiment | Observation | Next decision |
| --- | --- | --- | --- | --- |
| 1 (07:05-07:20Z) | The captions are driven by the reward queue the engine already publishes | `queue_probe.py` over the DRAGSTER primary capture: every update where the player's read cursor `$0CE7` advances, with the entry it consumed from `$0CC1` | 80 consumptions between 1599 and 3900, events 14, 37, 44, 45, 46 and 47 only. None is in the 72-199 voice range that `$81:C0CE-C18A` diverts, so that range is not what produces these captions | Look at what the original draws on those updates |
| 2 (07:20-07:30Z) | Each consumed event selects a caption | Recaptured the same case with frame images around the first consumptions (`pictures-a`) and read them | The caption is a red phrase in the middle of the screen. 1599 event 44 shows `MORE STUNTS`; 1633 event 45 shows `GIVE YOU`; 1647 event 46 shows `BIGGER BOOSTS`; 1681 event 14 shows `WIPEOUT`. So consecutive event ids carry the parts of a hint sentence and a separate id names a stunt, and the text is selected by the event id rather than by a separate announcement system | Find the string table the event id indexes, and what draws it |

## Handoff

- Current base/head commit and uncommitted state: registered at `260334d`; no work started.
- Verified findings: attempts 1 and 2. The captions are the on-screen half of the reward queue the
  engine already runs: a consumed event id selects a phrase, drawn in red in the middle of the
  screen. The hint sentence and the stunt name share one display. The recovered
  `ZoomZooPlayerAnnouncements` fields (`hints_active`, `hint_updates`, `hint_group`,
  `empty_display`) are the state behind it; the text and its drawing are what is missing.
- Current hypothesis: an event id indexes a string table in ROM, and the phrase is drawn for a
  bounded number of updates from the consumption. Failed approaches: the 72-199 voice range is not
  involved in these captions; a statistical WRAM-diff hunt for "caption state" ranked bytes whose
  change counts merely happened to sit near consumptions ($15CE-$15D2 change constantly from 1329,
  before any race event), so read the original's code and pictures instead of ranking byte churn.
- Exact next experiment/command: find the string table the event id indexes. Read
  `$81:C0CE-C18A` (the player consumer) and follow where it stores the event for display, then find
  the glyphs. `artifacts/classic-stunt-names/pictures-a` has the frames, captured with
  `dragster_playable_reference --case tests/manifests/native/dragster-ordinary-primary.case.json
  --horizon 3900 --frame-image <n>`.
- Remaining dependencies: none; every prerequisite is integrated on `main`.
- Runtime needs: the private ROM, the audited core, the v8 pack, disk for captures, and roughly an
  hour of machine time for a full gate matrix.

## Review and integration

To be completed by the primary after independent review.
