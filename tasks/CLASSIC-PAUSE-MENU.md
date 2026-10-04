# CLASSIC-PAUSE-MENU - the original's pause menu picture

## Assignment

- Status: **claimed** 5 October 2026 (prepared 3 October 2026 by the AUDIO-ONE-PLAYER session,
  from the user's live check), on main `cbbd9ba` after verifying PR #52's merge, main equal to
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

- Head: `c505074` (implementation, R-0078, regenerated maps) on `task/classic-pause-menu`, pushed;
  [PR #53](https://github.com/malmazuke/unirally-reconstruction/pull/53) open as a draft.
- Verified: R-0078's table; `presentation_tests` covers the menu's cells and drawing and the clear
  (both labels, a split race keeping its cells); clang-tidy finds no function over 80 lines;
  `coverage native-symbols --check` passes with no uncited address.
- Gates: `local/evidence/classic-pause-menu/gates.sh` run on a detached checkout of `c505074`
  (`.worktrees/classic-pause-menu-gates`), output `gates-c505074.out`; base binaries for the
  sweeps are main `cbbd9ba`'s lab-release build, kept in `base-cbbd9ba/`.
- Review round 1 (Claude Opus 5.5 subagent, fresh clone at `c505074`): changes requested, three
  findings, posted on PR #53; its report, trial patch, probe and withheld captures `fadein` and
  `zoomlap` are kept in main `local/evidence/classic-pause-menu/review/`. All three are applied
  (attempt 5).
- The gate script's v1 contract lines were blank on `c505074` (the contract tool resolves the
  `local` symlink and refuses fixtures outside the checkout); both contracts were run by hand on the
  same checkout with fixtures under its `artifacts/` and pass. `gates-v2.sh` keeps them there.

