# CLASSIC-SPLIT-TIME - the original's signed split time during the race

## Assignment

- Status: reviewed and integrated on `main` at `8752f95` (claimed 22 September 2026 about
  10:45Z by the preparing session on the user's instruction, see the quota line below); accepted
  conditional on the final-tip CI and remote verification recorded in the ignored closeout
  `artifacts/classic-split-time-integration/closeout.json` in the main checkout
- Milestone: M4 presentation (declared-omission closure), not an M4 acceptance gate
- Coordinator: the preparing session (Claude Fable 5.1, Claude Code desktop session,
  22 September 2026 UTC); the implementing session is coordinator, primary and integrator under
  D-0006 once it claims this record
- Task provider (fixed for all children; record any user-initiated platform change): Anthropic
- Worker/session/runtime/model: Claude Fable 5.1 (`claude-fable-5-1`), Claude Code desktop
  session, 22 September 2026 UTC, the same session that prepared the record. The provider is
  unchanged from CLASSIC-RACE-HUD; Fable 5.1 rather than Opus 5 is a within-provider model change,
  autonomous under D-0004.
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
  M4-16 in D-0004). **Claimed on the user's instruction**: after the preparation report, which
  said the task waited for the reset or an explicit instruction, the user wrote "Sorry, you can
  continue"; read as that instruction and recorded here as the override, with the same limits as
  M4-16's (no reset, purchase or provider change; a genuine usage block stops the work). Fresh
  sample at claim: weekly all-models **80%** at 10:44Z, five-hour 4%; **82%** at 11:06Z after the
  implementation, the WRAM probe and the first sweeps, five-hour 22%.
- Reviewer (primary automatically spawns fresh model/effort, isolated checkout; no user trigger):
  a fresh Anthropic subagent (the claiming session's model) in a separate checkout at the candidate
  commit, spawned by the primary. It must recapture consecutive originals across at least one
  split-time transition the primary did not use, on both tracks if the mechanism runs on both.
- Review tier ([D-0008](../docs/decisions/D-0008-static-map-track-breadth-review-tiers.md),
  recorded at the 11:15Z checkpoint; the decision landed on `main` at `0e5440b`/`9e423a5` while
  this task ran): **tier 2**, presentation on the recovered BG3 layer. The diff touches
  `src/core/presentation.{hpp,cpp}`, the presentation tests and records only; no simulation
  state, arithmetic, ordering, serialization, pack rule, gate or baseline changes (the split
  arithmetic reproduced is display arithmetic on published digits). So one independent review
  round by a fresh subagent on the exact candidate, the frozen gates and pixel sweeps as the
  evidence, and one re-review if a finding is returned. The reviewer may escalate the tier if the
  diff touches something this classification does not admit. The D-0008 session also sampled
  weekly all-models usage at 84% at 11:20Z, account-wide (its own records work included).
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

- Native capability delivered / still missing: delivered - both centred fields as the original
  draws them through the race: each rider's crossing time (`M:SS:th`) after a lap and at the
  finish, the signed split (`+M:SS:t`, the opponent's always `-`) at a checkpoint the other rider
  has already passed, nothing for the first rider through a slot, the opponent's first-seen cut,
  and the blank 118 updates after each crossing unless the rider has finished, all through the
  one-field-per-update redraw queue (R-0044). Still missing on an ordinary race frame: the
  off-screen rider arrow only.
- Frozen exact-match interval, field set and reference/seed identity: no new frozen contract; the
  measurement is the picture score against frozen originals and consecutive recaptures asserted
  against the frozen digests (R-0044's table).
- Dynamic captured inputs still consumed (must be zero for autonomy): zero; presentation only,
  derived from the published 742-byte state and the queue's own history.
- Relevant branches/transitions exercised, including independent variations: the opponent first
  through a slot (cut), the player first (nothing drawn), the player's split with the clock's
  tick in the same update, the opponent's split with the constant `-`, the blank at 118, a lap
  change with both riders' crossing times, the finish sequence, the 10:00 time-out hold, and
  DRAGSTER's checkpoint 2, blank and finish. Not exercised: a negative split (unreachable with
  one shared clock, see R-0044), a split with a minute digit above nine, and the queue's
  behaviour on a mid-race restore without slot history (declared).
- First divergence and cheapest next discriminating experiment: none open for this task. The
  off-screen rider arrow is the nearest unrecovered thing; its sprite writes would show in an
  access capture over a window where a rider leaves the screen.
- Trial-wide usage baseline/current, reserve, reset authorization/outcome or none: weekly
  all-models 79% at preparation (10:13Z), 80% at claim (10:44Z), 82% at 11:06Z, 86% at 11:41Z
  (account-wide, with the D-0008 session writing records concurrently). The 80% reserve floor was
  crossed on the user's instruction ("Sorry, you can continue"); no reset, purchase or provider
  change.

## Evidence and attempts

| Attempt | Hypothesis | Experiment | Observation | Next decision |
| --- | --- | --- | --- | --- |
| 0 (preparation, 22 Sep 10:00-10:30Z) | The task can start now | Sampled usage before dispatch, as D-0004 requires | Weekly all-models 79% against the 80% reserve floor; the previous task cost eleven points | Prepare the record, defer the claim to the weekly reset (2026-09-24T08:00Z) or an explicit user override |
| 1 (10:45-10:48Z) | The two flags R-0043 names are set on identifiable updates mid-race | `flag_probe.py` over the primary's WRAM for every change in `$0340-$0351` | `$0349` goes to 1 on 2077, 2306, 3014, 3208, 4840 and to -1 118 updates after each; `$034B` likewise; 3208 is the lap crossing. So the fields are requested on crossings and blanked 118 updates later | Disassemble the two handlers |
| 2 (10:48-10:50Z) | One routine draws the split and the finish time | `disasm.py` over `$81:EDDC-F033` and `$81:F033-F28B` | Each field has two writers selected by `$119F,y`: the finish layout from `$11BB/$11AB/$11AF/$11B3/$11B7` and a split layout with the colons moved, whose first cell is `$11BB` for the player and the constant `$80:8220` for the opponent. A negative request blanks unless `$0EFF,y` is set | Find who sets the digit bytes and `$119F` |
| 3 (10:50-10:53Z) | The lap routine computes the split | ROM-wide search for the writers (indexed stores included), then `$81:C900-CB23` and `$81:8050-82B6` disassembled | `$119F,y` is the checkpoint mode from the lap routine; `$81:C910` requests the field when the countdown `$0FFF,y` reads 120 and blanks it at 2, and computes the split digit by digit from the clock `$0E19..$0E25` against the slot store `$100D[16 laps + 4 cp]`, the first rider through a slot storing its clock and clearing its own request. Native's engine already keeps the countdown, mode, digits and the shared first-seen flags; only the slot store is missing | Verify the arithmetic against WRAM before writing code |
| 4 (10:53-10:55Z) | The split reads the clock after the update's tick | `split_probe.py` over the primary | `+0:01:3` on 2077 is 0:09.8 minus 0:08.5, and the clock reads 0:09.9 after that update: the request runs before the tick. 9 of 9 splits and 6 of 6 first-seen stores match the previous update's clock; 8 and 4 the current one | Use the previous update's clock; run the probe over every capture |
| 5 (10:55-10:58Z) | The rule holds on every capture, both tracks | `split_probe.py` over 77 captures with whole WRAM (M4-16, idle, opposing-input, DRAGSTER originals) | 1,145 requests; 509 splits all from the previous update's clock (448 from the current), 0 negative; 260 first-seen stores all the previous clock; 376 lap or finish displays all the crossing digits; 1,145 blanks all at countdown 2. DRAGSTER runs it with one checkpoint | Implement in `ClassicRaceHudClock` |
| 6 (10:58-11:02Z) | The queue can carry the fields as requests | `ClassicRaceHudClock` keeps a request per field and the slot times; crossing text and split arithmetic as free functions; `+` added to the glyph map; the finish tests driven through the crossing | Builds; two test errors of my own (a first-seen crossing recognised by the countdown alone misses the opponent's cut-to-2 case, so crossings are recognised by the next-checkpoint step; and my expected negative-path strings were not the original's ten's complement). 23/23 ctest on app-debug at `708af4c` | Measure |
| 7 (11:02-11:04Z) | Native matches the original where it draws | `hud_compare.py` over the primary's 274 kept frames and three consecutive sets from the previous task | Player band 6,347 -> 0; opponent band 9,880 -> 476, **14 pixels on every frame on which the opponent's split shows**: the `+`/`-` glyph difference. The opponent's writer reads the constant `-`, which attempt 2 had noted and the implementation had not applied. Consecutive lap change, crossing and finish sets 0 in all four bands | Apply the constant, with a test from the primary's 3881 |
| 8 (11:05-11:10Z) | The constant closes the band | Rebuilt at `33e12a8`; one serial sweep chain over every set (an earlier pair of chains had been killed mid-loop and their shells went on writing the same files, so all sweeps were re-run once, serially, `sweeps.sh`) | **0 differing pixels in all four bands on every frame of every set**: the primary's 274 kept frames (whole picture 18,601 -> 2,374), the brake race's 274, trick-long's 192, the 79 primary consecutive frames, the 60 new consecutive frames across six split transitions, compound-reverse-a's lap change (18) and finish (36), down-a's crossing (17) and DRAGSTER primary-a's 34 across its checkpoint, blank and finish. R-0044's table has the per-set numbers | Gates, then review |
| 9 (11:10-11:35Z) | The gate script runs as it did two days ago | `gates.sh` on `2298dc6` (code identical to `33e12a8`) | lab-debug and lab-release 23/23, then **every lab-sanitize test timed out at 30 s and the next spun at 99% CPU for 19 minutes**. Sampled: the process never reaches `main`; it loops in ASan's own `InitializeShadowMemory -> get_dyld_hdr`. The host changed under this session (Darwin 24.6 at its start, macOS 27.0 build 26A428 with Xcode 26.1.1 by now), and a one-line hello-world built with `-fsanitize=address` hangs the same way while the plain build runs. Not a property of the candidate | Record lab-sanitize and app-sanitize as **unavailable on this host** (not passed, not failed); the hosted Linux job builds and tests both; rerun the remaining gates |
| 10 (11:33-11:41Z) | Everything else a presentation change must run still passes | `gates.sh` (sanitizers recorded unavailable) on the tree of `2298dc6`, then `gate_identity` alone on the committed `c3fe841` after the run had refused a dirty tree; the time-out sweep after the gates | lab-debug, lab-release, app-debug 23/23 ctest; synthetic `rc=0 status=passed`; both v1 contracts passed; hidden DRAGSTER and ZOOM ZOO runs 0 rider-pose fallback frames; fuzz 40 seeds / 79 completed races / 0 aborts; `gate_identity` finds every engine input byte-identical to `6e0fad6` so the eleven differential gates are citable. The 10:00 time-out sweep is byte-identical to CLASSIC-RACE-HUD's (bands differ only on the 11 screen-off frames, 0 on the 24 hold frames). Logs in `gates-c3fe841/` | Dispatch the independent review at `c3fe841` |
| 11 (11:41-12:00Z) | - | Fresh Fable 5.1 reviewer spawned in `.worktrees/classic-split-time-review` at `c3fe841` on `review/classic-split-time`, with the prompt in `artifacts/classic-split-time/review-prompt-filled.md` (withheld consecutive recapture required; D-0008 tier 2, one round). Weekly all-models usage **86%** at 11:41Z (account-wide; the D-0008 session was writing records at the same time), five-hour 55% | **Approve** at `a7cfc6b` (report `tasks/CLASSIC-SPLIT-TIME-review.md` on `review/classic-split-time`), about 20 minutes. The reviewer read the routines out of the ROM itself, re-ran the WRAM probe on five captures (21/21 splits from the previous clock, 0 negative), re-ran `gate_identity`, and recaptured **five withheld originals**: trick-long-a (63 frames: a lap change one update apart, the opponent's zero split `-0:00:0`), brake-a (31: the loser finish), DRAGSTER reversal-a (44: the opponent finishing first on a sprint, the player's `+0:20:8`), the DRAGSTER finish tie (31, whole picture 0) and random-1-a (8) - **0 in both bands on every frame**. It also drove the queue directly (a request behind the left field and the clock, a blank over a pending draw, restart) and found one unmeasured gap | Apply the five should-fix items |
| 12 (12:00-12:10Z) | The should-fix items change no measured frame | Applied at `e225bfd`: (1) both riders through one unseen slot on the same update - `$81:CB13` runs the player first, so the opponent finds the slot seen and publishes `-0:00:0`; native's queue now counts a slot stored this update as seen (unit test; no capture shows the case); (2) R-0044 wrongly said no DRAGSTER original shows the player's split - five do, now named; (3) the DRAGSTER whole-picture residual is not only the arrow but the caption serviced a picture later than native's, recorded as a limit and a caption-model follow-up; (4) the time-out row's numbers; (5) the `slot_times_` comment. 23/23 ctest | Re-run the gates and every sweep on the final candidate, then integrate |

## Handoff

- Current base/head commit and uncommitted state: base `main` at `9e423a5` (rebased from
  `ee5c132` after D-0008's two records-only commits; the code diff is byte-identical to the
  reviewed candidate `c3fe841`, which the reviewer's worktree holds). Implementation `2db997a`
  and `7ce8558` (originally `708af4c` and `33e12a8`); records on top.
- Verified findings: attempts 1 to 10 and [R-0044](../docs/research/R-0044-classic-split-time.md).
- Current hypothesis and failed approaches: settled. Two of my own errors are worth keeping: a
  crossing recognised by "the countdown became 120" misses the opponent's first-seen crossing,
  which the engine sets to 120 and cuts to 2 within one update (found by my own test, fixed by
  recognising the next-checkpoint step); and the opponent's constant `-`, read in the
  disassembly at attempt 2 and not applied until the kept-frame sweep showed the same 14 pixels
  on every frame with an opponent split. Also: two sweep chains killed mid-loop went on writing
  the same files, so every sweep was re-run once, serially (`sweeps.sh`).
- Commands executed, outcomes and report hashes. Scripts under
  `local/evidence/classic-split-time/classic-split-time/` after integration; run from a checkout
  of this branch with its `local/` inputs in place. `E` is the main checkout's `local/evidence`.

  ```sh
  # the WRAM check of the rule over every capture (list in captures.txt)
  tr '\n' '\0' < artifacts/classic-split-time/captures.txt | xargs -0 python3 artifacts/classic-split-time/split_probe.py
  # consecutive originals across the split transitions, asserted against the frozen digests
  PYTHONPATH=. python3 artifacts/classic-split-time/recapture.py "$E/m4-16-playable-zoom-zoo/m4-16/boundary-a" \
      artifacts/classic-split-time/orig-primary-splits 2004-2012,2075-2082,2193-2198,3873-3886,4631-4642,6275-6285
  PYTHONPATH=. python3 artifacts/classic-split-time/recapture.py \
      "$E/dragster-ordinary-controls/dragster-ordinary-controls/originals/primary-a" \
      artifacts/classic-split-time/orig-dragster-splits 2378-2392,2496-2502,3205-3216
  # every picture sweep, serially, on the app-debug build
  ./artifacts/classic-split-time/sweeps.sh
  # everything a presentation change must run (sanitizers recorded unavailable on this host)
  ./artifacts/classic-split-time/gates.sh
  ```

  Outcomes: `split-probe.log`, `compare-*.txt`, `gates-c3fe841/`, `gates-run.log`.
- Unavailable/skipped checks: `lab-sanitize` and `app-sanitize` are unavailable on this host
  (attempt 9; the hosted Linux job builds and tests both). The eleven differential compares are
  cited through `gate_identity`, not re-run. No live playtest in this task.
- Exact next experiment/command: none open. For the off-screen arrow, an access capture over a
  window where a rider leaves the screen, for the OAM writes.
- Remaining dependencies: none.
- Runtime needs (network, build time, fixtures, memory): the private ROM through the locator, the
  audited bsnes core, the v9 pack, about 25 s for an app-debug build, about 4 minutes for the gate
  script without the sanitizers, about 90 s per 274-frame sweep on this host, and `gh` for CI.
- Aggregate parent/child time, provider usage before/after (or unknown), other-account-work
  caveat: in the closeout.
- Accepted outcome, review/fix rounds and next routing decision: approved at the first and only
  round (D-0008 tier 2), five should-fix items applied on top; integrate on `main`, then
  STATIC-CODE-MAP is next under D-0008.

## Review and integration

- Reviewer and independent reproduction/withheld-case results: one fresh Claude Fable 5.1 subagent
  (D-0008 tier 2, one round) in `.worktrees/classic-split-time-review` at the exact candidate
  `c3fe841`, about 20 minutes: **approve** at `a7cfc6b` (report
  [CLASSIC-SPLIT-TIME-review](CLASSIC-SPLIT-TIME-review.md) on `review/classic-split-time`, pushed
  to `origin`). It rebuilt and tested the candidate, read the routines from the ROM, re-ran the
  WRAM probe on five captures, re-ran `gate_identity`, and recaptured five withheld originals
  (trick-long-a 63 frames, brake-a 31, DRAGSTER reversal-a 44, the DRAGSTER finish tie 31,
  random-1-a 8) - 0 differing pixels in both centred bands on every frame; it drove the queue
  directly for late service, a blank over a pending draw and restart.
- Required changes or acceptance rationale: no blocking finding; five should-fix items, all applied
  at `e225bfd` (attempt 12): the same-update slot tie, the record's false DRAGSTER claim, the
  caption residual named as a follow-up, the time-out numbers and a comment.
- Exact merge candidate and required-check results: `8752f95` (the rebased branch, code identical
  to the reviewed `c3fe841` plus the tie correction); `gates.sh` on it: lab-debug, lab-release and
  app-debug 23/23, synthetic passed, both v1 contracts passed, hidden runs 0 fallback frames, fuzz
  40 seeds / 79 races / 0 aborts, the eleven differential gates citable; every sweep re-run at 0
  in the bands; the two sanitizer presets unavailable on this host (attempt 9), covered by the
  hosted Linux job.
- Integrated commit and evidence location: `8752f95` by fast-forward of `main` from `9e423a5`.
  Task evidence at `local/evidence/classic-split-time/classic-split-time/` (scripts, logs,
  `gates-8752f95/`, the consecutive originals), the review's at
  `local/evidence/classic-split-time-review/classic-split-time-review/`; closeout as above. Both
  worktrees removed with their build output, both local branches deleted.
- Remote synchronization: pushed ref(s), verified local/remote commit IDs, or exact push failure:
  `refs/heads/main` at `8752f95f6e1ed408bdea8eaee0c142936e728311`, verified equal to local;
  `review/classic-split-time` at `a7cfc6b` on `origin`. Final-tip CI: in the closeout.
- Scope still unverified: the off-screen rider arrow; the caption's one-picture wait behind the
  centred fields (R-0044, a caption-model follow-up); the negative split path and a same-update
  slot tie, implemented from the ROM and unit-tested but shown by no capture; a mid-race restore
  without slot history (declared).
