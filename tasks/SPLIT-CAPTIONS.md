# SPLIT-CAPTIONS - rider 1's tutorial hints and the split HUD's upload order

## Assignment

- Status: **accepted** 6 October 2026 (tier 1, [PR #57](https://github.com/malmazuke/unirally-reconstruction/pull/57));
  claimed 5 October 2026 by the session that closed SPLIT-RACE-END, on main `6d8d823`
  after that task's merge (main equal to `origin/main`, closeout written). Prepared the same day by
  the SPLIT-HUD-GAPS session, from R-0080's remaining caption differences.
- Milestone: M4 breadth
- Coordinator: the claiming session is coordinator, primary and integrator
- Task provider: Anthropic
- Worker/session/runtime/model: Claude Code desktop app, Claude Opus 5.5 (`claude-opus-5-5`); one
  session as coordinator, primary and integrator
- Actual model/reasoning effort, routing rationale: **tier 1** (D-0008): rider 1's announcement
  queue (`movement.rewards`) is serialized race state.
- Provider quota window (D-0004): at claim (5 October 2026 about 22:10 Sydney) weekly 47%, five-hour
  29%; standing rule: continue until weekly 80%.
- Dependencies: SPLIT-HUD-GAPS (R-0080), TWO-PLAYER-VS (R-0071), LEAGUE (R-0073), the split demo's
  opponent hints (R-0069).
- Base commit: main `6d8d823`.
- Branch and isolated worktree: `task/split-captions` in `.worktrees/split-captions`.
- Owned paths: `src/core/race_update.cpp` and the announcement queue code (rider 1's hints),
  `ClassicRaceHudClock` and `ClassicRaceHistoryTracker` (the split upload chain), native tests, a
  research record, this record, `docs/STATE.md`, `tasks/README.md`, `tasks/NEXT_SESSION.md`.

## Outcome and boundaries

After SPLIT-HUD-GAPS a two-human split race's picture differs from the original's only in caption
cells (R-0080), for two reasons:

1. **Rider 1's tutorial hints.** A human rider 1 whose tutorial bit is set gets the tutorial
   sentences in its own queue (`$0CEB`, cursors `$0D11`/`$0D13`, hold `$0CA7`, `$11C3`, `$03EF`,
   `$12E5`: `$81:BEF1-BF31`, `$81:C05C-C0CD`), shown in the lower view ("give you", "bigger
   boosts" in `twop-plain`). Native queues hints for rider 1 only in the split demo.
2. **The split chain's upload order.** The split NMI (`$81:D853`) services one field a picture in
   the order left fields, top clock, bottom clock (requested an update after the top), top cells,
   bottom cells, stunt scores, top caption, bottom caption; native's `ClassicRaceHudClock` follows
   the one-player chain, so a caption change can land a picture off.

The same chain times the lower fields at a finish (`drfin` 3967-3968, `leaguefin` 8961-8963), and
the split CONTINUE clear (the pauser's rows from `$130F`, 129 words) takes the lower arrow's cells
on the closing pictures (`leaguefin` 9050-9053).

The split HUD is also still drawn over the riders as flat ink where the original applies the
view's colour math to the object behind it (R-0080: drawing it before the riders mends 68 of the
review's `zzap` pictures and breaks 6 of the split demo's), and the lower centred cells show "-"
where the original shows "+" (`zzap` 7155-7271).

Recover rider 1's hint queue in two-human races, the split chain, the split clear, the split
HUD's layering and the lower cells' sign, and make every caption and HUD cell of R-0079's and
R-0080's captures exact. One-player play does not move.

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| State | Rider 1's queue words against the original's on R-0079's captures | Equal on every race frame | JSON |
| Pictures | `classify.py` over R-0079's captures | No caption class left | JSON |
| Nothing else moves | Gates, sweeps | Unchanged in one-player play | gate logs |
| Review | Tier 1 | Approved | review on the pull request |

## Evidence and attempts

The listing reading is main `local/evidence/split-captions/listing-report.md` (a reader subagent,
22:15-22:35 Sydney); the measurements are main `local/evidence/split-captions/measure.py` and
`measure-83649c0/`, against main 6d8d823's binaries (`base-6d8d823/`).

| Attempt | Hypothesis | Experiment | Observation | Next decision |
| --- | --- | --- | --- | --- |
| 1 | The split cell writer reads the sign | `crossing_cell` keeps the minus for one view only | `zzap` 205 pictures better, 1 worse (7154, the chain's timing) | Layering |
| 2 | A race from the menus makes the objects the sub screen ($212D = 0x10) | Split HUD before the riders unless `demo_ai` | Split demo 326/326 equal; no change on captures without a rider under the ink | Rider 1's hints |
| 3 | Rider 1 has its own hints (`$12E5`, `$12ED`, `$12F1`) | `opponent_hints` in race state; rider 1's group timer, hold, queue-time end, write-back | Rider 1's queue equal on every race frame of all 18 split captures (main: 240-622 frames differ); `twop-plain` 238 → 516 pictures | Restores |
| 4 | The reader needs the flag before the base checks | `TrailerFlags`; counter derived from the clock; runner's restore check takes layouts F/G | Restores equal on `twop-plain`, `league`, `zzap` | The chain |
| 5 | The split chain ($81:D853) | `request_split_fields`, `service_split_clock`, rider 1's caption last | `twop-plain` 530/530; `league` 618 with 13 worse after a pad-2 close | The split clear |
| 6 | CONTINUE clears the view the menu last opened in | `clear_after_pause` for split races, `$130F` remembered | `league` 670, `leaguefin` 120/121, no picture worse anywhere | Messages |
| 7 | A message's `"` is tile 0x261 | Draw nothing for it | `league` 710/710 | Records, gates, review |

Remaining race-time differences are the riders under the ink (R-0082, "Not covered"): a pad-2-idle
rider 1's look in `vs-idle`, a 2-pixel upper-body strip in a few pictures, and rider 0's sprite
near the split line once; none is HUD text.

## Gates

`local/evidence/split-captions/gates.sh` runs against main `6d8d823`'s binaries
(`local/evidence/split-captions/base-6d8d823/`). The final head's run is `gates-35ca787.out`, after
`gates-9e744a4.out`.
- Presets and ctest: 41 of 41 on each of the four.
- Synthetic suite, v1 contracts and eight hidden runs: pass.
- The eleven differential gates: pass.
- Race sweep: 432 runs, 0 differences.
- Cues: all seven identical to main.
- Tooling: 544 OK.
- Rules: no function over 80 lines; native-symbols passes.
- Fuzz: 40 aborts, as on main.
- Every split capture's words, rider 1's queue included: 0 differences.
- Restores: equal.

Front-end sweep: 128 of 179 manifests equal. The other 51 are TWO-PLAYER-VS's 22 and LEAGUE's 29,
checked by `fe_rows.py` and `fe_pictures.py`.
- League races against the computer differ only in the wrapper's last byte.
- Two-human races differ in rider 1's queue, its hints byte, and what the hints hold up: rider 1's
  rewards and finish pose. `vs-late` checks that against the original, with 0 race-time picture
  differences.
- Against the originals' frames: 36 pictures are better, 325 equal and 9 worse. All 9 worse are
  pictures where native already shows another scene (5,766-57,197 pixels off on main), and the
  candidate adds rider 1's hint text there.
- Pictures by capture: `measure-9e744a4/` (all eighteen, none worse than main).

## Review and integration

- Reviewer: a fresh Claude Opus 5.5 subagent in its own clone, tier 1, with preregistered cases.
- Round 1 on `9e744a4`: changes requested.
  - Blocking: a league pair's stunt event can end rider 1's hints with its hold at up to 118
    (`$83:E8B9`), which the reader refused. Fixed in `d64b357`, with a test.
  - Low findings, fixed in `35ca787`: the full-queue drop, the CONTINUE clear with history that
    starts mid-menu, the restore probe for ZOOM ZOO, and named offsets.
  - Records added to R-0082: the league-vs-computer wrapper byte and the residue's extent.
  - The review's withheld captures, `mike1` (rider 1 MIKE: the quirk confirmed) and `p2pause`
    (pad 2 pausing during the hints), are equal in words on every race frame. They are in main
    `local/evidence/split-captions/review/`.
- Round 2 on `35ca787`: approved.
  - One low point left: a pair's stunt state may hold 120 at any time, not only after the finish,
    which only a forged state reaches. It is recorded, not narrowed: the stunt finish's flag is
    read after the base layout's checks.
- Integration: merged by merge commit after the final head's checks; closeout in main
  `artifacts/split-captions-integration/closeout.json`.
