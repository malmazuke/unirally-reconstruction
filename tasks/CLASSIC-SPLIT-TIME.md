# CLASSIC-SPLIT-TIME - the original's signed split time during the race

## Assignment

- Status: ready (prepared 22 September 2026 UTC; implementation deferred to the weekly usage
  reset, see the quota line below)
- Milestone: M4 presentation (declared-omission closure), not an M4 acceptance gate
- Coordinator: the preparing session (Claude Fable 5.1, Claude Code desktop session,
  22 September 2026 UTC); the implementing session is coordinator, primary and integrator under
  D-0006 once it claims this record
- Task provider (fixed for all children; record any user-initiated platform change): Anthropic
- Worker/session/runtime/model: to be recorded at claim. The provider is unchanged from
  CLASSIC-RACE-HUD; the preparing session ran on Claude Fable 5.1 rather than Opus 5, which is a
  within-provider model change and autonomous under D-0004. Record the actual model at claim.
- Actual model/reasoning effort, routing rationale and frontier escalation question (if any):
  the claiming session's model at default effort, following the provider and review rule in
  `tasks/NEXT_SESSION.md`: an Anthropic primary and a fresh Anthropic independent reviewer in an
  isolated checkout at the exact candidate. No frontier escalation question is open; the work is
  additive presentation on a recovered layer (R-0043), not a schema, contract or scope change, so
  it does not qualify for D-0004's proactive planning consultation.
- Provider quota window/baseline timestamp, used/remaining or unknown, reserve and session
  allowance (D-0004): at preparation, weekly all-models **79%** used at 2026-09-22T10:13Z
  (five-hour window 2%, resets 15:10Z; weekly resets **2026-09-24T08:00Z**; the per-model Fable
  window read 47%). D-0004 reserves the final 20% of the weekly allowance for review and recovery,
  so one point stood between this preparation and the floor. That is why the task is prepared and
  not started: CLASSIC-RACE-HUD cost eleven points including four returned review rounds, and
  starting this task would have breached the reserve on its first experiment. **Claim it after the
  weekly reset, or on an explicit user instruction to ignore the boundary** (as recorded once for
  M4-16 in D-0004). Sample fresh usage at claim and record it here; the discretionary allowance is
  20 points from that baseline, the floor is 80%, and no reset, purchase or provider change is
  authorized.
- Reviewer (primary automatically spawns fresh model/effort, isolated checkout; no user trigger):
  a fresh Anthropic subagent (the claiming session's model) in a separate checkout at the candidate
  commit, spawned by the primary. It must recapture consecutive originals across at least one
  split-time transition the primary did not use, on both tracks if the mechanism runs on both.
- Dependencies and evidence of acceptance: CLASSIC-RACE-HUD (the two centred cells, the redraw
  queue and `ClassicRaceHudClock`, which this task extends; R-0043), CLASSIC-STUNT-NAMES (the BG3
  font sheet and `draw_bg3_text`; R-0042), M4-16 and DRAGSTER-ORDINARY-CONTROLS (the kept originals
  and their WRAM). All integrated on `main` with their closeouts under `artifacts/`.
- Base commit: the `main` tip at claim; `60f8f5c` or later (CLASSIC-RACE-HUD integrated at
  `0ead6d3`, closeout `artifacts/classic-race-hud-integration/closeout.json`).
- Branch and isolated worktree: `task/classic-split-time` in `.worktrees/classic-split-time`.
- Owned paths and shared interfaces: `src/core/presentation.cpp`, `src/core/presentation.hpp`,
  native tests under `tests/`, this task record, the research record (R-0044), `docs/STATE.md`,
  `tasks/README.md` and `tasks/NEXT_SESSION.md`. Shared interface: the 742-byte `ZoomZooState` is
  read-only here; this task changes presentation only. If the split needs content the v9 pack does
  not carry (the sign glyph is the first candidate, see below), the pack rules under
  `tests/manifests/content/` and the profile constant are owned too, and the bump is additive.
- Claim/lease/heartbeat/checkpoint location: this record's Evidence and attempts table; local
  artifacts under `artifacts/classic-split-time/` in the worktree, moved to
  `local/evidence/classic-split-time/` in the main checkout at integration.
- Session time limit, concurrency allocation and actual spend authorization if relevant: one active
  worker (the claiming session) plus the review subagent; 45-minute reassessment intervals; no
  monetary spend is authorized.

## Outcome and boundaries

Draw the original's signed split time. During a race the original writes a signed `M:SS:t` value
into the two centred seven-cell fields that CLASSIC-RACE-HUD recovered for the finish times: rows
20-21 columns 13-19 (measured `-0:00:1` on primary frame 3900) and rows 5-6 columns 13-19 (a signed
`0:01:3` on frame 2080), with a sign glyph the caption alphabet does not contain (R-0043, "What is
left"). Native draws those cells only at the finish, where they match the original exactly. This is
the larger of the two declared omissions left by CLASSIC-RACE-HUD, and the last thing the original
draws on BG3 during an ordinary race frame that native does not.

In scope: the mechanism - which event publishes a split (a lap crossing, a checkpoint, the opponent's
crossing, or something else), which rider's time is subtracted from which, the sign rule, which of
the two fields each split goes to and why, how long it stands and what blanks it, the glyph encoding
of the sign, and its place in the one-field-per-update redraw queue (`$0349` and the opponent's
flag are the dispatcher entries R-0043 names); native drawing of it through `ClassicRaceHudClock`
and `draw_bg3_text` for both tracks; and frozen original captures behind every claim, including
consecutive frames across every transition the change recovers.

Out of scope unless the recovery shows it is the same mechanism and comes almost free with it: the
off-screen rider arrow (the other declared omission), the start arrow and ring, the animated finish
banner, the result-screen art and pixel style, audio, the pause menu's own style, the two-update
late result load after a time-out and the `zoom.*` pack aliases. No gameplay state, serialization
or engine behaviour changes; if the split needs a value the 742-byte record does not carry, that is
a finding to record and a reason to stop and report, not a licence to widen the serialized state.

## Inputs and prerequisites

- PAL ROM `a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e` through the private
  locator `local/rom-location.txt`; audited bsnes core
  `e59bf88d4fc922c9fe3b5438e65ff3a6909d24e1628f0f87141c8de17699a91b`.
- Pack `classic.pal.crawler.two-tracks.v9` (57 entries) at
  `local/classic-pal-crawler-two-tracks-v9.pack`. The sign glyph is outside the caption alphabet
  R-0042 and R-0043 mapped (`$01`-`$0f`, `$20`-`$2f`, `$40`-`$45`, `$4d`, `$4e`, `$60`, `$61`,
  `$80`); search `presentation.classic.font.v1` for it first with CLASSIC-RACE-HUD's `hud_search.py`
  before concluding that a profile bump is needed.
- Existing originals to read before capturing anything new: the M4-16 ZOOM ZOO captures under
  `local/evidence/m4-16-playable-zoom-zoo/m4-16` (whole WRAM per frame; kept pictures under
  `local/evidence/m4-16-rider-art/m4-16-rider-art/original-*`), the DRAGSTER originals under
  `local/evidence/dragster-ordinary-controls/dragster-ordinary-controls/originals`, and the
  consecutive recaptures under `local/evidence/classic-race-hud/`. Frames 2080 and 3900 of the
  primary are the two measured split pictures.
- CLASSIC-RACE-HUD's scripts under `local/evidence/classic-race-hud/classic-race-hud/`: `hud_compare.py`
  (band sweep against kept pictures), `recapture.py` (consecutive originals from a capture),
  `queue_probe.py` (which updates set a dirty flag), `band_look.py` (decode a band's glyphs),
  `disasm.py`, and `gates.sh` (everything a presentation change must run).
- No known baseline failure: `main` at `60f8f5c` is a records-only commit on the integrated
  CLASSIC-RACE-HUD tip `0ead6d3`, whose full-path CI passed on both platforms.

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| The mechanism is recovered | `queue_probe.py` extended to `$0349` and the opponent's flag over the M4-16 captures, then an access capture over the updates on which they are set mid-race, then the writers of the two fields disassembled | a stated rule for the publishing event, the operands and sign, the field each split goes to, its lifetime and blanking, the sign glyph's tile, and its queue position, with addresses and frames behind each | research record R-0044 |
| The content is authenticated | the sign glyph found in the pack, or extracted through the pack rules | byte-exact extraction; any profile bump additive with v1 contracts and the v9 entries unchanged | pack report |
| Native matches the original where it draws | `hud_compare.py` over every kept race frame of the M4-16 primary, brake and trick-long races and the DRAGSTER manifests, plus consecutive recaptures across every split publication and blanking on both tracks | every pixel of the two centred bands matches, on kept and consecutive frames, with no exception claimed | picture scores in the gate logs |
| The finish sequence still matches | the six consecutive sets CLASSIC-RACE-HUD's closeout lists, re-run | unchanged at 0 | gate logs |
| No accepted contract moves | `gates.sh`: five presets and ctest, synthetic, both v1 contracts, hidden runs, fuzz, `gate_identity` for the eleven differential compares | all `status=passed`, restore counts unchanged | gate logs |
| Independent review | fresh Anthropic subagent in an isolated checkout at the candidate | approve, with its own consecutive recapture across a split transition the primary did not use | review report |
| Hosted CI on the final tip | `gh run list --workflow synthetic.yml --commit <tip>` | both platforms success | closeout |

## Capability and coverage checkpoint

- Native capability delivered / still missing: nothing delivered yet. Missing: the split time
  itself, and separately the off-screen rider arrow.
- Frozen exact-match interval, field set and reference/seed identity: none new expected; the
  measurement is the picture score against existing frozen originals and consecutive recaptures.
- Dynamic captured inputs still consumed (must be zero for autonomy): zero; presentation only.
- Relevant branches/transitions exercised, including independent variations: to be recorded. At
  minimum each split publication and blanking on both tracks, a split that changes sign, the
  opponent-first race, and the finish sequence after a standing split.
- First divergence and cheapest next discriminating experiment: the two measured pictures, 2080
  (rows 5-6) and 3900 (rows 20-21). Cheapest first experiment: `queue_probe.py` over the primary's
  WRAM for every update on which `$0349` or the opponent's finish-time flag is set before the
  finish, to learn the publishing event before capturing anything.
- Trial-wide usage baseline/current, reserve, reset authorization/outcome or none: preparation at
  79% weekly all-models (2026-09-22T10:13Z); implementation baseline to be sampled at claim.
  Reserve floor 80%. No reset, purchase or provider change.

## Evidence and attempts

| Attempt | Hypothesis | Experiment | Observation | Next decision |
| --- | --- | --- | --- | --- |
| 0 (preparation, 22 Sep 10:00-10:30Z) | The task can start now | Sampled usage before dispatch, as D-0004 requires | Weekly all-models 79% against the 80% reserve floor; the previous task cost eleven points | Prepare the record, defer the claim to the weekly reset (2026-09-24T08:00Z) or an explicit user override |

## Handoff

- Current base/head commit and uncommitted state: no branch or worktree exists yet. Create
  `task/classic-split-time` from the `main` tip at claim.
- Verified findings: only R-0043's two measured pictures and its naming of `$0349` as the player's
  finish-time dirty flag, both from CLASSIC-RACE-HUD. Nothing about the split's publishing event,
  operands or lifetime is verified.
- Current hypothesis and failed approaches: none tried. Provisional reading, to be rejected
  cheaply: each field shows the difference between the two riders' times at the last crossing of a
  lap or checkpoint, signed from the viewer's side, written through the same finish-time dirty flag
  the finish uses, which would make the finish time the last "split" the field receives. Treat this
  as a guess until the flag probe and the access capture say otherwise.
- Commands executed, outcomes and report hashes: none for this task. The usage sample is the only
  measurement.
- Unavailable/skipped checks: none yet.
- Exact next experiment/command: from a claimed worktree with `local/` symlinks in place and `E`
  the main checkout's `local/evidence`:

  ```sh
  cp "$E/classic-race-hud/classic-race-hud/queue_probe.py" artifacts/classic-split-time/split_probe.py
  # edit it to scan $0349 and the opponent's finish-time flag instead of $0D17, then
  python3 artifacts/classic-split-time/split_probe.py "$E/m4-16-playable-zoom-zoo/m4-16/boundary-a"
  # then an access capture over a window it reports, for the $2116/$2118 writers, e.g.
  python3 tools/project.py access capture --manifest <accepted manifest> --out OUT \
      --from-frame <first set - 2> --to-frame <first set + 6>
  ```

- Remaining dependencies: none outside this task; the usage reset is a resource condition, not a
  dependency.
- Runtime needs (network, build time, fixtures, memory): the private ROM through the locator, the
  audited bsnes core, the v9 pack, about 25 s for an app-debug build, about 4 minutes for the whole
  gate script, about 8 minutes per 274-frame picture sweep, and `gh` for the final-tip CI.
- Aggregate parent/child time, provider usage before/after (or unknown), other-account-work caveat:
  preparation about 30 minutes on Fable 5.1, 79% before; implementation to be recorded.
- Accepted outcome, review/fix rounds and next routing decision: none yet.

## Review and integration

- Reviewer and independent reproduction/withheld-case results: to be recorded.
- Required changes or acceptance rationale: to be recorded.
- Exact merge candidate and required-check results: to be recorded.
- Integrated commit and evidence location: to be recorded.
- Remote synchronization: pushed ref(s), verified local/remote commit IDs, or exact push failure:
  to be recorded.
- Scope still unverified: to be recorded.
