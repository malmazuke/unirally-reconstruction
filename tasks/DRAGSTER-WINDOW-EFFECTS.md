# DRAGSTER-WINDOW-EFFECTS - draw the countdown and winner windows when and where the original does

## Assignment

- Status: review
- Milestone: M4 follow-up (from the M4-16 findings, after DRAGSTER-PALETTE-CYCLE)
- Coordinator: main session
- Task provider: Anthropic (unchanged)
- Worker/session/runtime/model: Claude Code implementation worker, Claude Opus 5
- Actual model/reasoning effort, routing rationale: Opus 5, high; a bounded
  recovery plus native implementation under D-0006, no frontier escalation
- Provider quota window/baseline: five-hour window 16% at 22:50Z, 20% at start
  of this task, 26% at the gate run; weekly 20% to 21%. Stop new work at 90% of
  the five-hour window or 45% weekly (user-set).
- Reviewer: to be spawned fresh in an isolated checkout at the candidate
- Dependencies and evidence of acceptance:
  [R-0015](../docs/research/R-0015-native-presentation.md) (the frozen window
  entries), [R-0037](../docs/research/R-0037-dragster-race-palette-cycle.md)
  (the fill colour and the open question), R-0038 (native DRAGSTER
  initialization at frame 1328)
- Base commit: `9411e59`
- Branch and isolated worktree: `task/dragster-window-effects` in
  `.worktrees/dragster-window-effects`
- Owned paths: `src/core/presentation.{hpp,cpp}`, `src/core/content_pack.cpp`,
  `src/core/presentation_runner.cpp`, `src/app/frontend.cpp`,
  `src/app/live_presentation_runner.cpp`, `tools/unirally_lab/content/pack.py`,
  `tools/unirally_lab/frontend/commands.py`,
  `tests/manifests/content/classic-crawler-two-tracks-pack.json`,
  `tests/native/presentation_tests.cpp`, `tests/app/frontend_contract_tests.cpp`,
  `docs/research/R-0040-dragster-window-effects.md`, this record,
  `docs/BUILD_AND_VALIDATION.md`, `docs/STATE.md`, `tasks/NEXT_SESSION.md`
- Checkpoint location: ignored `artifacts/dragster-window-effects/`

## Outcome and boundaries

Recover, from original evidence, when the original enables DRAGSTER's channel-6
window effects and which HDMA table each frame uses, and draw them natively
without the rider pose-pair gate.

Inside: the countdown digits, their transitions and the GO letters; the winner
banner; the content the pack needs; ROM-free tests; the gates.
Outside: ZOOM ZOO's own window content (the mechanism is shared and R-0040 says
so, but which members ZOOM ZOO selects was not captured); the other declared
presentation omissions; any change to the accepted DRAGSTER v1 contracts.

## Inputs and prerequisites

- ROM SHA-256 `a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`
  through `local/rom-location.txt`; audited core
  `e59bf88d4fc922c9fe3b5438e65ff3a6909d24e1628f0f87141c8de17699a91b`.
- Packs: `local/classic-crawler-dragster.pack` (v1, unchanged) and the new
  `local/classic-crawler-two-tracks-v8.pack`, extracted with
  `python3 tools/project.py content pack --rules tests/manifests/content/classic-crawler-two-tracks-pack.json --out local/classic-crawler-two-tracks-v8.pack`.
- Fixtures copied into this checkout under
  `artifacts/dragster-window-effects/fixtures/`
  (`m3-02-integration-fixtures`, `m4-01-loser-fixtures`); native states and
  recaptured originals read from
  `.worktrees/dragster-palette-review/artifacts/dragster-palette-review/`
  (`sweep/`, `native-states/`, `orig-extra/`, `probe-review3213-a/`).
- Access captures regenerate from the commands in
  `artifacts/dragster-window-effects/*/access.json` (`regeneration_command`).
- No pre-existing baseline failure was found.

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| The enable condition and per-frame table come from the ROM and original captures, not pixel fitting | `python3 tools/project.py access capture --manifest tests/manifests/replay/race-crawler-dragster-12000-continuous-right-fields.json --out <dir> --from-frame 1200 --to-frame 3700 --watch-pc 0x808691 ...`, twice | identical digests; the routines and thresholds in R-0040 predict every observed selection | `artifacts/dragster-window-effects/{win-a,win-b,lose-a,rel3213}/access.json` |
| Native selects the original's own table every frame | evaluate `dragster_window_table_index` on every native state of the release-3213 sweep and compare with `$80:8691` | agreement on every racing frame | `artifacts/dragster-window-effects/index-agreement.json` |
| The new shapes match original pictures where the old code drew nothing or the wrong shape | `python3 artifacts/dragster-window-effects/window_compare.py` | large improvement, no accepted frame regressed | `artifacts/dragster-window-effects/window-compare.json` |
| New static content enters the pack additively with provenance | `tools/project.py content pack --rules ...` | 56 entries, v7 entries unchanged | `local/classic-crawler-two-tracks-v8.pack` |
| A ROM-free test fails if the rule is ignored | `ctest --test-dir build/<preset>`; mutation probes | every mutation caught | `artifacts/dragster-window-effects/test-*.log` |
| Four presets | `cmake --preset P && cmake --build build/P && ctest --test-dir build/P` | 23/23 on each | `artifacts/dragster-window-effects/{build,test}-*.log` |
| The accepted DRAGSTER presentation contracts do not regress | `artifacts/dragster-window-effects/hist.sh` | the accepted counts unchanged | `artifacts/dragster-window-effects/historical/` |
| The DRAGSTER historical matrix | the same script | 20/20 rc=0 | the same |
| The M4-16 ZOOM ZOO primary gate and two DRAGSTER frozen originals | `artifacts/dragster-window-effects/gates2.sh` | passed | `artifacts/dragster-window-effects/gates/` |

## Capability and coverage checkpoint

- Native capability delivered: DRAGSTER draws the countdown digits, their
  transitions, the alternating GO letters and the cycling winner banner from the
  original's own channel-6 table family, with no pose-pair gate, for every
  racing frame of a player-won race and the first 120 updates of an
  opponent-won one.
- Still missing: the opponent-won banner after its 120-update finish-animation
  counter runs out (R-0040 "Not established" names the cheapest next
  experiment); ZOOM ZOO's own window content.
- Frozen exact-match interval: frames 1533-3454 of the release-3213 timeline,
  compared on the selected table member; 742-byte state formats unchanged.
- Dynamic captured inputs consumed at run time: zero. The rule is a function of
  the serialized state; the tables are pack content.
- Branches exercised: no window; each countdown digit and each transition; both
  GO parities; the banner from both riders' finishes; result loading; the
  result screen; and the DRAGSTER v1 fallback.
- First divergence: result-loading update 2, by construction (the original's
  race vblank stops and native holds update 1's member).

## Evidence and attempts

| Attempt | Hypothesis | Experiment | Observation | Next decision |
| --- | --- | --- | --- | --- |
| 1 | The channel-6 table is chosen per frame from work RAM, not from poses | Disassemble `$80:8691` (R-0015's "selects the table address and bank") and search the ROM for writers of `$11FD` | `$80:868E-$8699` publishes `$00:11FD`/`$00:11FF`; all sixteen writers are in bank `$83` and index the pointer list `$83:E55C` | Read those drivers |
| 2 | The pointer list defines a family of equal-sized tables | Decode `$83:E55C` and parse each candidate at `$15:8000 + 899k` | stride exactly 899; members 0-24 are valid 898-byte tables with a `$00` terminator; members 3 and 18 are the two frozen entries byte for byte | Freeze members 0-24 as one additive entry |
| 3 | The countdown driver keys on a single counter | Capture DRAGSTER watching `$80:8691`, `$11C5`, `$1229`, `$0300` and `$0F01-$0F09`; two fresh processes | byte-identical captures; `$11C5` runs 270 down to 1 and the observed boundaries match the ROM thresholds exactly | Implement the countdown rule |
| 4 | The GO letters alternate on a frame-parity flag | Read `$0300` per frame and the two GO sites | `$83:CCED` toggles `$0300` every frame; member 3 on even display frames, 4 on odd | Implement |
| 5 | The banner is a fixed table | Compare the captured members around the finish | member starts at 7 and advances every second frame, wrapping 24 to 7, for 360 frames | Replace the fixed table with the cycle |
| 6 | The banner starts on the finish frame | First draft used `start = frame - player_finish_delay` | wrong by one frame: pixel comparison got *worse* at 3214, 3452 and 3453 | Re-derive |
| 7 | The driver's first update is the one after `record_finish`, and its choice shows one frame later | Probe `dragster_window_table_index` on the fixture states and compare with the capture | with `start = frame + 1 - since`, agreement on every racing frame of all three captures | Keep |
| 8 | Result loading keeps drawing the banner | First draft advanced the member during loading | the original's `$80:8691` stops after loading update 1 | Freeze at the loading-start frame, as `apply_dragster_palette_cycle` does |
| 9 | The test catches a wrong rule | 16 mutations of the rule and 3 of its wiring | 14 caught first time; "loading freeze removed" and "selection ignored: always index 3" survived | Add an odd-offset loading assertion and render checks at frames whose member is 4 and 0; all 19 then caught |

### Measurements

Index-level, the strongest check: on all 2,147 native states of the
release-3213 sweep, native's member equals the original's `$80:8691` pointer on
**1,922 of 1,922 frames from 1533 to 3454** and on frame 3679; the 224
differences are result-loading updates 2 and later, which the original no longer
publishes. The same rule reproduces the continuous-right (241 frames) and
opponent-won (345 frames) captures with no disagreement.

Pixel-level, over 132 frames with both a native state and a recaptured original,
scored on the frozen rectangle (0, 28, 256, 196), previous implementation
(pack v7, parent-commit binary) against this one (pack v8):

| Segment | Frames | Differently drawn pixels | matching the original before | after |
| --- | --- | --- | --- | --- |
| countdown transition 1510-1534 | 2 | 18,556 | 0 | **18,556** |
| GO 1535-1603 | 44 | 384,328 | 0 | **384,328** |
| winner banner 3214-3453 | 46 | 164,753 | 0 | **164,276** |
| result loading 3454-3677 | 14 | 36,414 | 14,080 | 17,374 |
| all 132 | 132 | 604,051 | 70,885 | **584,534** |

Rectangle mismatches fall from 1,327,728 to 757,274 of 6,623,232; 100 frames
improve, 30 are unchanged, and 3600 and 3677 are worse by 13 and 17 pixels
inside the already declared "native draws the race while the original fades in
the result screen" difference.

### Gates

| Gate | Result |
| --- | --- |
| `cmake --preset P && cmake --build build/P && ctest --test-dir build/P` for lab-debug, lab-release, lab-sanitize, app-debug, app-sanitize | 23/23 on each, 0 failures |
| DRAGSTER historical matrix (`artifacts/dragster-window-effects/hist.sh`, the adapted `hist2.sh`) | 20/20 rc=0, ANY FAILURE: 0 |
| accepted DRAGSTER presentation contracts with the v1 pack | winner 36/697/279/445/653/962/961, loser 1,073: identical to the accepted figures |
| M4-16 ZOOM ZOO primary gate and DRAGSTER frozen originals primary, random-1, reversal (`artifacts/dragster-window-effects/gates2.sh`) | see the handoff |

## Mistakes

- The first winner-banner draft started the cycle on the finish frame instead of
  the update after it. The pixel comparison caught it as a *regression* at
  frames 3214, 3452 and 3453 before any of it was committed; the index-level
  comparison against `$80:8691` then located it exactly. Scoring against the
  original's own pointer, not only against pictures, is what made the off-by-one
  obvious, and it is now the primary measurement.
- The first mutation probe left two survivors because both test frames happened
  to be insensitive to the mutation (an even loading offset, and a GO frame
  whose member is 3). Mutation-checking a test is not optional; a test that
  passes is not evidence until a wrong implementation fails it.
- The first render-level test assertion could not see its own window because the
  synthetic all-zero palette makes colour 0, the background fill and the GO
  window all white. The fix was to give the test frame the pose pair whose
  colour 0 is not white, not to weaken the assertion.

## Handoff

- Base `9411e59`; head: see `git log` on `task/dragster-window-effects`. No
  uncommitted state at handoff.
- Verified findings: R-0040, with the addresses, the enable condition, the table
  provenance and the measurements above.
- Failed approaches: starting the banner on the finish frame (attempt 6);
  advancing the banner through result loading (attempt 8).
- Commands executed and outcomes: the acceptance table above; logs and reports
  under ignored `artifacts/dragster-window-effects/`.
- Unavailable/skipped checks: none skipped. Live play by the user is still due
  for DRAGSTER generally (a standing item from DRAGSTER-ORDINARY-CONTROLS), and
  now also covers the countdown and banner.
- Exact next experiment: recover the opponent-won banner beyond its 120-update
  counter. Produce an opponent-won native DRAGSTER timeline with per-frame
  states (`python3 -m tools.unirally_lab.native.dragster_playable` against the
  `race-crawler-dragster-12000-release-3000-3299-fields` scenario), then test
  whether `finish_time_centiseconds[0] - finish_time_centiseconds[1]` recovers
  the frame gap between the two finishes exactly, given the `frame & 1` term of
  `finish_centiseconds`. If it does, derive the opponent's finish frame from
  `player_finish_delay` and that gap once the player has also finished, and
  extend `updates_since_winner_finish`; if it does not, record why and consider
  a state version that stores the winner's finish frame.
- Remaining dependencies: none.
