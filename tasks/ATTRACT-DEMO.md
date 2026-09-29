# ATTRACT-DEMO - the next idle demo cycle

## Assignment

- Status: **in progress**, claimed 29 September 2026 09:03 UTC after PR #43 merged.
- Milestone: M4 coverage of the original main menu's idle modes.
- Dependency: [SPLIT-SCREEN-RACE](SPLIT-SCREEN-RACE.md) and its first-cycle evidence
  [R-0069](../docs/research/R-0069-split-screen-race.md).
- Review tier: **1** under D-0008. The next demo cycle chooses a race mode and rider controls;
  its timing and state updates are gameplay claims requiring the full D-0006 review and gates.
- Provider and owner: OpenAI, primary Codex task on `task/attract-demo` in
  `.worktrees/attract-demo`, from `5718fc203d99ae46672bf12fa7d476a8a29f6634`.
  The active runtime identifies itself as GPT-6; its exact model slug and effort
  are not exposed in this task, so neither is inferred. A fresh
  `gpt-5.6-sol`/medium reviewer will inspect the exact candidate in a separate
  checkout under D-0006.
- D-0004 usage at claim: Codex weekly 24% used, 76% remaining, sampled
  29 September 2026 09:03 UTC; reset timestamp 4 October 2026 20:28 UTC.
  This task's discretionary implementation cap is 44% used, with the final
  20% weekly reserve for review and recovery. One reset credit is available
  but is not authorized to redeem. Reassess at 45 minutes and checkpoint at
  least every 10 minutes.
- Evidence home when claimed: `local/evidence/attract-demo/` in the main checkout.

## Outcome and boundary

After the first idle split ZOOM ZOO race returns to the main menu, reproduce the original's next
idle demo cycle natively from power-on through its own return. The original reaches a one-player
track 3 with SRAM `$77:0750=0xC202` by frame 4399; retained pictures at frames 5500 and 6999
differ from the current native build (R-0069). Treat the track and controls as measured inputs,
not an assumed repeat of the first cycle. Keep 2P and VS menu selection, arbitrary demo cycles
beyond this one, and audio outside this task unless a verified dependency requires more scope.

## First experiment and acceptance

1. Start from the pinned PAL ROM/core and `local/evidence/split-screen-race/demo-primary-full/`
   schedule: Start on frames 300-305, then released through frame 6999. Repeat the original cold
   capture independently, record the second cycle's transition, race pairing, selected track,
   controller writes, return frame and every relevant SRAM/WRAM field. Consult the static code
   map's listing before designing a focused trace and cite it for each routine reading.
2. Freeze the original's frame inputs, full per-frame state and picture samples through the
   second cycle. Compare the native transition, race state and all retained pictures against
   that freeze. Investigate the first divergence, including any result or menu fade.
3. Add independently chosen control or timing variations for reached branches, plus save and
   continuation checks wherever the new path serializes state. Define and record the tested
   domain precisely; refuse unrecovered paths explicitly.
4. Keep the first split demo, one-player menus and race freezes equal. Run the complete task
   gates, hosted macOS/Linux CI, fresh independent tier-1 review and the PR merge workflow.

## Handoff from SPLIT-SCREEN-RACE

- The first demo initializes its two-rider ZOOM ZOO race at frame 1448 and requests exit at
  frame 3348; its menu is stable by frame 3500 (R-0069). The second cycle is a distinct
  one-player track 3 by frame 4399.
- Original whole-WRAM/SRAM/video hashes for frames 1300-6999 are in
  `local/evidence/split-screen-race/demo-primary-full/reference.json`; the original capture is
  repeated by `demo-repeat` and `demo-transitions`. The second cycle needs its own focused
  capture and native comparison before any implementation decision.
- Next command: inspect the frame 3500-4399 transitions in that capture and the mode-5 static
  listing, then capture a dense original window across the second race entry and first updates.

## Checkpoint - 29 September 2026 09:13 UTC

- Source: `5718fc2`, branch `task/attract-demo`; worktree `.worktrees/attract-demo`.
  The main checkout is clean. Its ignored `local/` and `artifacts/` are linked into
  the worktree so evidence still has one home. The isolated `app-debug` build
  passed after linking the existing pinned toolchain. A first build attempt
  before that link correctly reported the toolchain missing, not a pass.
- D-0004 usage: 26% weekly used at 09:13 UTC, from 24% at claim. The 44% used
  discretionary stop and 80% reserve boundary remain; no reset or purchase.
- Original repeat: a fresh Strict PAL run on the pinned ROM/core agrees with
  R-0069's 5,700 whole-WRAM, SRAM and video hashes at frames 1300-6999. A
  separate fresh full-WRAM run authenticated all 1,901 second-race projections
  and the retained prefixes against those hashes (`local/evidence/attract-demo/`
  `authentication.json`, `second-race-reference.txt`).
- Observed: idle reaches zero at 3946; mode 5 starts at 3947. Track 3/race type
  0 are stored in SRAM at 4399; `$0DE1` clears at 4406; rider/opponent become
  6/1 and options `0xC202` at 4407. The race initializes at 4523, `$212C=1`
  at 4523, and the demo timer reaches `0x076B` at 6422; its exit request is
  on 6423. The main menu's idle counter is restored at 6541. This is the
  tested cold-start path, not a general cycle rule.
- Static reading, now dynamically exercised: `artifacts/static-map/bank-83.lst`
  `$83:CD50-CD61` runs `$83:E254` demo controls and then `$83:E082` opponent
  controls when `$0DE1` is clear. A fresh access capture on frames 4523-4526
  (`local/evidence/attract-demo/second-entry-access`) records both calls on
  4524. It sees `$83:E303` write horizontal 2 to `$0319` at instruction index
  285; `$83:E0C2` writes 2 to `$031B` at 356, after the demo routine's write
  at 316. The later `$83:E794/E79F` pad publication writes 1 to both. The
  exact ordering is established; its native reproduction is the next experiment.
- Native baseline: `front_end_runner` still chooses ZOOM ZOO, rider 4/opponent
  14 and initializes the second split race at 4495. A normal native track-3
  start already agrees with original frame 4523's first 565 projected bytes
  except the expected state magic and frame label; its demo-specific tier,
  controls and continuation remain to compare.
- Hypothesis: track-3 setup can reuse the native ordinary race initializer
  with measured pairing 6/1, while the one-player demo needs its own entry
  timing, the demo controller followed by opponent AI, and serializable demo
  clocks. Next: implement the minimum second-cycle dispatch and state timeline,
  then compare the very first changed row before expanding to all 1,901.

## Checkpoint - 29 September 2026 09:36 UTC

- Implementation branch has uncommitted task-scoped changes. The second cycle
  now selects track 3, rider 6 against rider 1 and initializes at original
  frame 4523. The one-view demo runs demo controls followed by opponent AI;
  its second camera's position holds while velocity/lookahead update. Its
  958-byte `URTR0308` state carries demo clocks and pairing without changing
  the ordinary 916-byte track-3 layout or split layouts.
- A first full race comparison found only six one-byte `$0331` jump-input
  mismatches (frames 4767, 4769, 4849, 4851, 4853, 4855). A fresh bounded
  access capture at 4766-4770 showed `$83:E3E3` copying `$0300` to `$0331`
  while the marked rider is descending off the surface. The native controller
  now follows that branch. Its 565-byte projected state and 42-byte demo
  trailer each match every frame 4523-6423: 1,901 of 1,901 rows.
- Restoring native states at frames 4523, 5000, 6300 and 6420 reproduces
  uninterrupted serialization through frame 6423: 1,901, 1,424, 124 and 4
  rows respectively. No change to a frozen expected result was made.
- Of 16 original second-cycle pictures sampled from 3946 through 6999, 15
  match exactly. At frame 4399 the original title is brightness 15 and native
  is 13; the second cycle's fade begins one frame later. A focused fresh
  capture of frames 4399-4406 is running to verify that timing before the
  next visual comparison. The other 15 include race entry, riding, exit and
  stable menu return, all with zero different pixels.
- Remaining: complete the fade comparison and independent variations, freeze
  a coherent candidate, run affected and complete gates, independent tier-1
  review and PR integration. D-0004 weekly usage was 29% at 09:31 UTC, up
  five points from claim; next sample at review preparation.

## Checkpoint - 29 September 2026 09:57 UTC

- The second title fade now begins at its observed frame. All 22 retained
  original pictures across the second cycle, including frames 4399-4406,
  differ from native by zero pixels. See [R-0070](../docs/research/R-0070-attract-demo.md)
  for the source identities, captures, static reading and tested domain.
- Fresh early-exit runs press port 1 A at frame 5000 or port 2 B at frame 5200.
  Both exit on the pressed frame. Their original projected race state matches
  native through exit on 478 and 678 rows. The interrupted return holds black
  for two extra frames and starts its palette hook one frame earlier relative
  to the timer return. A 53-picture dense port 1 return and seven port 2
  pictures are pixel equal after modeling those timings. The original's
  `$00C8/$00C9` values independently locate the palette-cycle phase.
- The normal timer exit still has zero different pixels on its 22 retained
  pictures. The original and native comparison covers observed pictures,
  not every video frame. D-0004 usage is 31% weekly used, 13 points below the
  task's discretionary cap; one reset credit remains untouched.
- Remaining: format and inspect the source, run focused checks, freeze a
  review candidate and PR, obtain fresh tier-1 review, run full private and
  hosted gates, then integrate and close out. A 45-minute reassessment has
  been made; this checkpoint does not end the task.

## Review candidate - 29 September 2026 10:07 UTC

- Candidate implementation commit `d4d9ea6280bafac9f87ec3e3a65bbb1fba16325f`.
  `origin/main` remains the claimed base `5718fc2`. Only tracked source,
  documentation and the regenerated native-symbol index are committed; the
  staged diff was inspected and did not contain ROMs, captures or symlinks.
- `python3 tools/project.py build --preset app-debug` passed. Nine focused
  CTest cases passed, including race pairing, state serialization and the
  frontend scheduler. `clang-tidy -p build/app-debug src/core/*.cpp` reports
  no warnings; changed core files pass clang-format dry-run and
  `coverage native-symbols --check` passes.
- The rebuilt runner matches the first demo's 1,901 projected race rows and
  the second demo's 1,901 projected rows and 42 trailer fields. A hidden-window
  app run (`build/app-debug/src/app/unirally.app/Contents/MacOS/unirally
  --content-pack local/classic-pal-crawler-tracks-v26.pack --updates 7000
  --hidden --front-end-inputs
  local/evidence/split-screen-race/title-start-300-305.inputs`) completed 7,000
  updates: first and second demos returned at front-end frames 3349 and 6424,
  two returns, zero notices and zero pose fallback frames. The ignored log is
  `artifacts/attract-demo-integration/live-app.log` in the main checkout.
- Next: push the up-to-date branch, open a PR, spawn the required fresh
  `gpt-5.6-sol`/medium reviewer in an isolated checkout, answer findings,
  then run the full private/app-debug/app-sanitize/hosted gate matrix.

## Review correction - 29 September 2026

- [PR #44](https://github.com/malmazuke/unirally-reconstruction/pull/44) is open.
  Its first candidate `d63714e` passed hosted macOS/Linux CI after the static
  map was regenerated with the renamed camera initializer. The fresh
  `gpt-5.6-sol`/medium independent reviewer posted an initial acceptance and
  then a correction review that superseded it. Its withheld late Left press at
  frame 6400 found 17 projected race-byte differences at the exit and a
  different return picture at 6600. The primary independently reproduced the
  failure and captured the full 42-byte trailer for this and the two earlier
  pad variations, finding an off-by-one demo timer on all three exits.
- Verified original ordering from the static listing `$83:E254-E2BB`, the
  bounded `late-press-access` trace and fresh cold PAL captures: timeout
  compares the incremented clock without storing it; a pad exit stores the
  incremented value before checking the pad. The exit preserves the pressed
  direction for the last race update. At the late warning threshold, the
  return holds one fewer blank picture than an earlier pad interruption.
- The corrected source matches projected race bytes 12-564 and all 42 trailer
  bytes on 478 port 1 A, 678 port 2 B and 1,878 port 1 Left rows through
  their respective exits. Native pictures match 57, 7 and 57 retained original
  pictures on their returns. The first split demo and the normal second demo
  still each match 1,901 projected rows; the normal second demo also matches
  all 1,901 trailers and 22 retained pictures. These are exact measured
  domains, not a claim for arbitrary pads or all video frames.
- The ignored evidence in the main checkout is under
  `local/evidence/attract-demo/trailer-*`, `late-press-access`,
  `interrupt-*-fixed2`, `interrupt-*-dense-native-*` and
  `interrupt-report.json`. Source changes and [R-0070](../docs/research/R-0070-attract-demo.md)
  are awaiting a corrected candidate commit. Next: obtain re-review on that
  head, run the broad private/sanitizer and hosted CI gates, then merge and
  close out. Codex weekly usage is 33% at the correction checkpoint, below
  the 44% discretionary cap; no reset was used.

## Integration handoff - 29 September 2026 11:06 UTC

- Reviewed runtime correction `ad76af5b24f63036c80167d1200700f61a1b3d9d`;
  its map-only successor is `97a60980003687e9350fc7d3613917867977a2aa`.
  The fresh independent tier-1 reviewer accepted that exact head in
  [PR #44's final review](https://github.com/malmazuke/unirally-reconstruction/pull/44#pullrequestreview-5351284026).
  The prior gameplay and generated-map findings each have an Agent reply on
  the PR. The four raw-coverage static-map regeneration checked 53,062 sites
  with zero disagreements; native-symbol and its 24 tests pass.
- On the corrected code, the first and second normal demos each match 1,901
  original projected race rows. The second also matches its 42-byte trailer
  on all 1,901 rows and all 22 retained pictures. Three independent pad exits
  match 478, 678 and 1,878 projected-plus-trailer rows; their 57, 7 and 57
  retained return pictures are pixel equal. A fresh late-Left resume from
  frame 6399 reproduces serialized frame 6400 byte for byte. The hidden app
  run reaches both demo returns through 7,000 updates with no notices or pose
  fallbacks.
- The final source/map head `97a6098` passed all 11 private frozen native
  comparisons under `artifacts/attract-demo-integration/private-97a6098/`.
  Local `lab-debug` passed 506 Python tooling tests, 29 CTests and the
  fresh-process repeatability probe after using a real worktree `artifacts`
  directory. `app-debug` passed 29 CTests and repeatability on the identical
  runtime source; `lab-release` passed 29 CTests. The v1 presentation winner
  and loser contracts passed all seven and one visual cases, respectively.
  The pack v26 has 433 validated entries under the track rules. Hosted
  [macOS and Linux checks](https://github.com/malmazuke/unirally-reconstruction/actions/runs/36557279664)
  pass at `97a6098`, including the Linux sanitizer step.
- The local macOS `app-sanitize` build passed, but its complete CTest exceeded
  600 seconds and two focused tests each exceeded 180 seconds. These are
  timeouts, not passes. The first local Python tooling run had 11 setup
  failures because a linked `artifacts` directory resolves outside its
  checkout; the rerun with a real directory passed all 506. An attempted
  `content pack-inspect` used its default older DRAGSTER rules and failed on
  v26; direct validation with the tracked v26 rules passed. No reference
  results or thresholds changed.
- Next integration steps: commit this record-only handoff, get current-head
  CI and inspect every PR review, issue comment and line comment, update the
  PR body, merge with a merge commit, fast-forward local `main`, then write
  `artifacts/attract-demo-integration/closeout.json` with the actual merge
  commit and cleanup list. Preserve the cited captures under
  `local/evidence/attract-demo/`. After this task, the roadmap's next native
  outcome is the 2P and VS modes; create its task record before claiming it.
  Weekly Codex usage is 37% at 11:06 UTC, below the 44% discretionary cap;
  no reset credit was redeemed.
