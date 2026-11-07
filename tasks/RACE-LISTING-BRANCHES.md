# RACE-LISTING-BRANCHES - the race branches known only from the listing

## Assignment

- Status: **in progress**. Claimed 7 November 2026 03:39 UTC on main `2d516755`, on the user's
  instruction ("Yes, start it"; Claude Code desktop app, Claude Opus 5.5 `claude-opus-5-5`;
  coordinator and integrator; a worker subagent implements, a fresh subagent reviews). Queued
  9 October 2026 by [COVERAGE-GAPS](COVERAGE-GAPS.md) (R-0089, queue item 8); the record is written
  at the claim. Run under [D-0013](../docs/decisions/D-0013-task-time-and-disk-budget.md): claim to
  merge against the 6-hour target, new evidence under 1 GB. Usage at the claim: 5-hour 1% (the
  window reset at 03:20 UTC), weekly 3%.
- Branch `task/race-listing-branches` in `.worktrees/race-listing-branches`; evidence in main
  `local/evidence/race-listing-branches/`.
- Task provider: Anthropic.
- Milestone: M4 (original game coverage).
- Review tier: **1** (race state, the HUD's integer formatting, tile behaviours).
- Dependencies: R-0051 (loop and tile pairs), R-0068 (stunt HUD), R-0089 (the inventory),
  R-0093 (data coverage, the static map's corpus).

## Why

R-0089 lists race code that the static map knows only from the listing ([L]); no capture of the
original runs it:
- **HUD branches never shown:** lap and split fields drawn for `$FFFF` or `$1253 < 1`
  (`$81:D8D9`, `$81:E94F`, `$81:DB70`; blocks in `$81:D936`, `$81:DA4F`, `$81:DCE6`, `$81:E9AC`)
  and stunt scores of 100 and more (`$81:C3DC`).
- **Tile behaviours** in the table at `$81:82F0` that no capture reaches: `$81:875C` (pair 4,
  R-0051's open case; native throws on it), `$81:84B2`, `$81:831D`, `$81:83E7`, `$81:8565`.

## Outcome

- A census of each listed branch: what condition reaches it, whether the shipped game can reach
  it (which tracks or tile data hold the pair, which race states give the value), and whether
  native models it. Recorded in R-0107 with addresses.
- Each branch the shipped game can reach is captured from the original (preloaded cartridge RAM,
  a warm boot or a state origin where a route needs one), implemented natively where native
  differs or throws, and checked against that capture.
- Branches no shipped data or state reaches are recorded as unreachable, with the reason, and
  not implemented. Native keeps refusing them explicitly rather than inventing behaviour.
- Checks the gate runner can run (`checks.list`) and unit tests for each changed behaviour.

## Checkpoint (worker)

Recorded in [R-0107](../docs/research/R-0107-race-listing-branches.md). R-0089's two groups are
five behaviours. Native already modelled every reachable one, so no native code changed. The
captures made here show each on the original, and native equals them on every row and every
picture. The pack profile and the race state's serialized layout are unchanged.

Census (listing [L], ROM scans of all 45 tracks [S], captures [C], native read where its rows equal
the original's on every update [N]):

| Branch | Reachable? | Evidence | Native before / after |
| --- | --- | --- | --- |
| One-player arrow up/down (`$81:EA5E`, `$81:EB2C`, their erase `$81:E94F` for `$0FD9` = `$FFFF`/`$FFFE`, the blink's `$1253 < 1`) | Yes: up or down markers on tracks 4, 11, 19, 23, 24, 26, 28, 31, 34, 40 [S] | t23-seed102, t04-seed69 (up), t34-seed124 (down) [C]; the cold idle demo on track 26 [C] | Modelled (`race_hud.cpp`); unchanged, exact |
| Two-view arrow up/down (`$81:D8D9-DDA4`) | Yes, in two-view races on those tracks | Upper down: the idle demo on track 26 (warm boot, and a cold start with `$77:10C8`/`$1115` written) [C]; lower up: R-0089's corpus run 069 [C] | Modelled (`draw_split_arrow`); unchanged, exact where captured; other forms not captured |
| `$81:C3DC-C43B` (rider 1's stunt score, not scores of 100) | Yes, two-human stunt events | TWO-HUMAN-STUNTS' twop-bowl, coverage captured [C] | R-0095; unchanged |
| Tile pair 4 (`$81:875C`) | **No**: only track 44's flag table holds it (tiles 203, 204); no fine cell of any map indexes them [S] | `tile_scan.json`, `tile_scan.py --check` | Refused; kept |
| Pairs 30 and more | **No**: no flag table holds one [S] | same | Refused; kept |
| Lift, pair 24 (`$81:84B2`) | Yes: tracks 7, 15, 21, 31, 34, 35, 39, 40 [S] | track 7 plain, tracks 21 and 34 mirrored [N]; t34-seed124 [C] | Modelled; unchanged |
| Push unlocked, pair 28 (`$81:831D`) | Yes: tracks 15, 25, 35 [S] | t25-seed43 (DOWN+UP): plain 20, mirrored 26 [N, C] | Modelled; unchanged |
| Loop's unmirrored entry (`$81:83E7`), refusal and count back | Yes: tracks 15, 25, 35 [S] | locked-tours runner-0 [N]; t25-seed43: 10 entries, 1 refusal (pose override `$07A1`), 20 count-back updates [N, C] | Modelled; unchanged |
| Pair 8's clamp (`$81:8565`) | **No**, by construction: the reset `$81:8594` runs before the dispatch every update, so the count is at most 1 [L] | 65 exact captures and 300 native schedules never count above 1 [N] | Implemented (R-0051), dead code; unchanged |

Results (main `2d516755`'s binaries; the candidate's are the same), pack v41:

| Capture | Original | Native |
| --- | --- | --- |
| t23-seed102 (track 23, up) | 701 rows, 699 pictures | 701/701 exact, 699/699 equal |
| t04-seed69 (track 4, up) | 1,301 rows, 1,299 pictures | 1,301/1,301, 1,299/1,299 |
| t34-seed124 (track 34, down, lift) | 1,401 rows, 1,399 pictures | 1,401/1,401, 1,399/1,399 |
| t25-seed43 (DOWN+UP, push, loop) | 3,002 rows, 3,000 pictures | 3,002/3,002, 3,000/3,000 |
| Warm idle demo, track 26, two views | 1,905 pictures (1433-3338 but the initialization frame) | 1,905/1,905 |
| Corpus run 069, league two-human race | 81 pictures (32780-32860) | 81/81 |
| Cold idle demo `cold-102000`, track 26 | 61 pictures (63440-63500) | 61/61 |
| The 65 kept race captures with work RAM (through the trace build) | every row | 65 of 65 exact over their whole windows |

Each check fails on a broken native (`mutants.txt`): up and down swapped (t23 4 pictures, t34 10,
the warm demo 2), the plain push by 7 (t25 rows from 1199), the mirrored push by -7 (from 1848),
the loop refusal not stored (from 1003). Static map with the six coverage inputs: observed bytes
86,837 to 87,882, inferred 12,737 to 11,692, unknown 24.0%.
