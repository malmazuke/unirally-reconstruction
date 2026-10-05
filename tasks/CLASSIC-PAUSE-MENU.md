# CLASSIC-PAUSE-MENU - the original's pause menu picture

## Assignment

- Status: **accepted** 5 October 2026 (tier 2, [PR #53](https://github.com/malmazuke/unirally-reconstruction/pull/53));
  claimed the same day (prepared 3 October 2026 by the AUDIO-ONE-PLAYER session, from the user's
  live check), on main `cbbd9ba` after verifying PR #52's merge, main equal to
  `origin/main` and `artifacts/audio-one-player-integration/closeout.json`.
- Milestone: M4 breadth
- Coordinator: the claiming session is coordinator, primary and integrator
- Task provider (fixed for all children; record any user-initiated platform change): Anthropic
- Worker/session/runtime/model: Claude Code desktop app, Claude Opus 5.5 (`claude-opus-5-5`); one
  session is coordinator, primary and integrator
- Actual model/reasoning effort, routing rationale and frontier escalation question (if any):
  the claiming session's model at default effort; **tier 2** (D-0008: presentation on recovered
  layers; the race's state, timing and pack do not change. The HUD history it extends,
  `ClassicRaceHudClock`, is presentation-only and not serialized).
- Provider quota window/baseline timestamp, used/remaining or unknown, reserve and session
  allowance (D-0004): at claim (5 October 2026 08:50 Sydney, 4 October 21:50 UTC) the 5-hour window was 14%
  used and the weekly window 34% (Fable weekly 40%); standing rule: continue until weekly 80%.
- Reviewer (primary automatically spawns fresh model/effort, isolated checkout; no user
  trigger): a fresh Anthropic subagent with a withheld capture.
- Dependencies and evidence of acceptance: RACE-PAUSE-EXITS (R-0060), M4-16 (R-0035).
- Base commit: `cbbd9ba` (main at claim).
- Branch and isolated worktree: `task/classic-pause-menu` in `.worktrees/classic-pause-menu`.
- Owned paths and shared interfaces: `draw_race_pause_menu` and its callers in
  `src/core/presentation.*` and `src/core/race_hud.*`, the standalone callers that label the
  second choice (`src/app/sdl_main.cpp`, `src/app/dragster_fuzz_runner.cpp`,
  `src/core/classic_race_presentation_runner.cpp`), the front-end runner's race pictures, the pack
  rules if new tiles are needed (none were), native tests, a research
  record, this record, `docs/STATE.md`, `tasks/README.md`, `tasks/NEXT_SESSION.md`.
- Claim/lease/heartbeat/checkpoint location: this record.
- Session time limit, concurrency allocation and actual spend authorization if relevant: one
  worker plus the review subagent; no monetary spend.

## Outcome and boundaries

The user could not find a way to quit a race (AUDIO-ONE-PLAYER live check 1). The pause menu's
second choice already ends a race from the menus as the original's QUIT does (R-0060), but native
draws M4-16's authored panel: PAUSED, RESUME, RESTART RACE, a ">" marker and UP DOWN - ENTER
over a halved picture. The original (`$83:CD05`, `$83:F63E-F979`) dims the screen to brightness 7
and shows CONTINUE GAME and QUIT with a "<" in column 24.

Draw the original's pause menu in races from the menus, pixel-exact against captures. Keep the
standalone `--track` race's restart, and give it a label that says what it does. Behaviour, timing
and the frozen race gates do not move.

## Inputs and prerequisites

`local/evidence/race-pause-exits/decode/` (`quit`, `restart`, `lapquit`: frames with the menu
open), the static listing for `$83:F63E-F979`, the current pack.

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| Picture | Native pause frames against the captures' | Pixel-exact with the menu open, both choices | JSON |
| Nothing else moves | Gates, race sweep, v1 contracts, hidden runs, ctest, synthetic | Unchanged outside pause frames | gate logs |
| Review | Tier 2 | Approved with a withheld capture | review on the pull request |

## Capability and coverage checkpoint

- Native capability delivered: a paused one-player race, from the menus or on its own, shows the
  original's pause picture (brightness 7, CONTINUE GAME and the second choice in BG3 with the "<"
  cursor; the HUD hidden under the words; CONTINUE GAME clearing the player's cells). The second
  choice reads "quit" from the menus and "restart" on the standalone race. Still missing: the
  split-screen pause (pad 2's menu, the menu modes' messages, pad 2's pause in 2P/VS), queued as
  [SPLIT-PAUSE-MENU](SPLIT-PAUSE-MENU.md).
- Frozen exact-match interval: every captured picture of the six captures in R-0078's table,
  1,931 frames, 586 with the menu open.
- Dynamic captured inputs still consumed: none; presentation only.
- Transitions exercised: opening (on the Start press frame), the cursor both ways and held,
  CONTINUE GAME, Start held through an opening, QUIT after and during the countdown (restart), a
  lap race's quit, the menu over a split and the clear after it.
- First divergence: none left in the captures.

## Evidence and attempts

All times 5 October 2026, Sydney. Captures and scripts in main `local/evidence/classic-pause-menu/`.

| Attempt | Hypothesis | Experiment | Observation | Next decision |
| --- | --- | --- | --- | --- |
| 1 (08:55-09:00) | Native's race pictures from the menus are already exact outside the pause | New captures `menu` and `countdown` (`captures.sh`); `front_end_runner` taught to write one-player race pictures; `pictures.py` over `menu` on main's renderer | 320 of 670 equal: exactly the unpaused frames; all 350 paused pictures differ by about 55,700 pixels | Draw the original's menu |
| 2 (09:00-09:01) | The picture is brightness 7 for every paused update, with the words of `$83:F616`/`$83:F636` in the caption's font and ink over the riders | `render_classic_race` at brightness 7 when `pause.selection` is set, `draw_classic_pause_menu` last | `menu` 670/670, `countdown` 280/280, R-0060's `quit` 206/206, `restart` 271/271, `lapquit` 184/184 | Test the HUD under the menu |
| 3 (09:02-09:05) | The menu's words replace the HUD's in their cells, and CONTINUE GAME's clear (`$83:F915-F92C`) erases the split | A scratch build of the runner printing the HUD history found DRAGSTER's split `+0:03:6` on 2562-2680 with Right from 1750; capture `split` pauses on it (`captures-split.sh`) | The 60 paused pictures equal (the cells are already removed under the menu); the 95 pictures 2650-2744 after CONTINUE GAME differ in the split's cells: the original's are blank until the queue blanks them on 2745 | Follow the clear in `ClassicRaceHudClock` |
| 4 (09:05-09:07) | The clear lasts until the queue next writes the player's cells | `player_cells_cleared` set by the update that closes the menu, reset by the queue's next write of the cells; the up arrow's rows 5-6 taken down until the NMI's next redraw | `split` 320/320; all six captures 1,931/1,931 | Native tests, rules, records, gates |
| 5 (09:38-09:45) | The review's three findings (below) are right | Words drawn in the HUD's layer before the riders and the window; full brightness on the picture whose update closed the menu; "closed" means the menu's own count moved on, so neither a HUNTER skipped update nor the standalone restart counts; tests for both | The review's `fadein` 197 -> 200/200; `zoomlap` 2,259/2,260 (1801 is the same 4 pixels with main's renderer: an existing arrow gap); the six primary captures still 1,931/1,931 | Re-run the presentation gates on the fix, reply on the PR |

## Handoff

- Source candidate `291eb3f` on `task/classic-pause-menu` (implementation `c505074`, the GCC test
  fix `a38d513`, the review's fixes `291eb3f`); records follow it with no source change.
  [PR #53](https://github.com/malmazuke/unirally-reconstruction/pull/53).
- Verified: R-0078's table (the primary's six captures 1,931/1,931; the review's `fadein` 200/200,
  `zoomlap` 2,259/2,260 with an existing arrow gap); `presentation_tests` covers the menu's cells
  and drawing, the clear, a split race, a HUNTER skip and the standalone restart.
- Gates, `local/evidence/classic-pause-menu/gates.sh` on a detached checkout of `c505074`
  (`gates-c505074.out`, logs in main `artifacts/classic-pause-menu-integration/gates-c505074/`):
  lab-debug, lab-release, app-debug and app-release build, ctest 41/41 each; synthetic suite passed;
  v1 winner and loser contracts pass (run by hand with the fixtures under the checkout: the script's
  lines were blank because the contract tool resolves the `local` symlink and refuses fixtures
  outside the checkout; `gates-v2.sh` fixes this, `v1-c505074/`); eight hidden app runs and the
  front end's Start-held run pass with no pose fallbacks; the eleven frozen race gates pass with
  6,023 restores and the same row digests as AUDIO-ONE-PLAYER's; the DRAGSTER fuzz gate aborts 40
  of 40 as on main (recorded non-pass, its own follow-up); tooling 543 tests OK; no function over 80
  lines; native-symbols check passes.
- Gates, `gates-v2.sh` with `DIFFERENTIAL=0` on a detached checkout of `291eb3f`
  (`gates-291eb3f.out`, logs in `artifacts/classic-pause-menu-integration/gates-291eb3f/`): the
  same builds, ctest, synthetic suite, v1 contracts, hidden runs, tooling and rules pass. The fuzz
  gate and the eleven state gates are cited from `c505074`: the source diff `c505074..291eb3f`
  touches only `presentation.cpp` and `race_hud.*`, which those runners do not draw with.
- Race equivalence sweep against main `cbbd9ba`'s binaries (both pack v34), on both candidates:
  432 runs, 2,387,105 updates and 1,290 restarts identical; 179 runs differ only in pictures, and
  `classify_sweep.py` finds every one of the 400 differing pictures on an update with the menu open
  (the random schedules press Start), none elsewhere.
- Front-end sweep against main `cbbd9ba` (`fe_equivalence.py`): 179 of 179 manifests equal in exit
  status, state rows, stderr and race timeline, and 2,302 of 2,302 pictures both runners write; 308
  one-player race pictures only the candidate writes.
- R-0076's seven cue schedules: the candidate's cues are byte-identical to main's (the original
  comparison script reports a difference on main too since AUDIO-ONE-PLAYER's cues).
- Hosted CI (macOS, Ubuntu with sanitizers) passes on `a38d513` and `291eb3f`; `c505074`'s Ubuntu
  job failed to compile the new test under GCC (`std::pair<unsigned, size_t>` against
  `std::pair<unsigned, unsigned>`), fixed in `a38d513`.
- Unavailable: local sanitizer presets (macOS 27 host; Linux CI covers them).
- Usage: weekly 34% at claim, 36% at the records; five-hour 28%.
- Next: SPLIT-PAUSE-MENU (tier 1, queued here), or the other ready tasks.

## Review and integration

- Reviewer: a fresh Claude Opus 5.5 subagent in its own clone, tier 2. Round 1 on `c505074`
  ([review](https://github.com/malmazuke/unirally-reconstruction/pull/53)): changes requested. It
  reproduced R-0078's table, made two withheld captures (`fadein`, `zoomlap`) and found three
  faults: the countdown digit's window must cover the words on the picture that opens the menu
  (`fadein` 1440, 214 pixels); CONTINUE GAME during the fade-in must show at full brightness
  (`fadein` 1346-1347); a HUNTER effect's skipped update was taken for the menu closing and blanked
  a split time (native probe; `$83:CC9A-CCA2`). All three fixed in `291eb3f` (attempt 5); the agent
  reply on the pull request answers each.
- Round 2 on `291eb3f`: approved. `fadein` 200/200, `zoomlap` 2,259/2,260 (the existing 1801 arrow
  gap), the six primary captures equal, ctest 41/41, rules clean. Its note on an uncaptured pause
  during a HUNTER skipping effect is in R-0078's "Not covered".
- Integration: merged by merge commit after the final head's checks; the merge, synchronized `main`
  and cleanup are in main `artifacts/classic-pause-menu-integration/closeout.json`.
- Scope still unverified: R-0078's "Not covered" (split-screen pauses, now SPLIT-PAUSE-MENU; the up
  arrow under the menu; a pause during a HUNTER skipping effect; the left arrow over a rider).
