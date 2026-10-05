# SPLIT-RIDERS-UNDER-INK - the riders under a split race's HUD text

## Assignment

- Status: **claimed** 6 October 2026 by the session that closed SPLIT-CAPTIONS, on main `ec05ea7`
  after that task's merge (main equal to `origin/main`, closeout written). Prepared the same day
  from R-0082's remaining race-time differences.
- Milestone: M4 breadth
- Coordinator: the claiming session is coordinator, primary and integrator
- Task provider: Anthropic
- Worker/session/runtime/model: Claude Code desktop app, Claude Opus 5.5 (`claude-opus-5-5`); one
  session as coordinator, primary and integrator
- Actual model/reasoning effort, routing rationale: prepared as tier 2. **Escalated to tier 1**
  (D-0008) on 6 October 2026: the look's end of a scripted glance clears a rider's idle latch, a
  race-state word, so the look moved into race state and the serialized two-view states.
- Provider quota window (D-0004): at claim weekly 52%, five-hour 23%; standing rule: continue
  until weekly 80%.
- Dependencies: SPLIT-CAPTIONS (R-0082), R-0036 (rider look overlays).
- Base commit: main `ec05ea7`.
- Branch and isolated worktree: `task/split-riders-under-ink` in `.worktrees/split-riders-under-ink`.
- Owned paths: the rider look (`src/core/rider_look.*`, moved into the engine library), race state
  and its serializer (`src/core/zoom_zoo_movement.hpp`, `src/core/zoom_zoo_pack.hpp`,
  `src/core/race_update.cpp`, `src/core/race_state_io.cpp`), the picture (`src/core/presentation.*`),
  the runners (`src/core/front_end_runner.cpp`, `src/core/zoom_zoo_runner.cpp`), native tests,
  R-0083 and R-0036's note, `src/core/README.md`, this record, `docs/STATE.md`, `tasks/README.md`,
  `tasks/NEXT_SESSION.md`.

## Outcome and boundaries

After SPLIT-CAPTIONS, a two-human split race's HUD text is exact during the race. The remaining
race-time picture differences are all riders, measured with main
`local/evidence/split-captions/measure.py` and its per-picture JSON in
`local/evidence/split-captions/measure-9e744a4/`:

1. **An idle rider 1's look.** A pad-2-idle rider 1's seat and head pose differ from the
   original's in `vs-idle` (446 race pictures, main `local/evidence/split-race-end/vs-idle`) and
   `league-idle` (3). A pad-1-idle rider 0 (`vs-opp`) is exact, so the gap is rider 1's look in a
   split race.
2. **Parts of a rider under the ink.** Strips and patches of up to 21 pixels show flat ink where the
   original shows the rider's colours, in a few pictures: `zzap` 9, `zz2p` 3, `leaguefin` 1, and
   13 in the SPLIT-CAPTIONS review's `mike1` and `p2pause`. Likely the upper-body overlays
   (`$0D45`/`$0D47`) missing under the ink.
3. **Rider 0's sprite near the split line.** It shows in the lower view in `zzap` 6317, where the
   original shows background.

Recover each, making every race-time picture of R-0079's, R-0080's, R-0081's and R-0082's captures
equal. One-player play must not move.

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| Pictures | `measure.py` over the split captures | No race-time differing picture | JSON |
| Nothing else moves | Gates, sweeps | Unchanged in one-player play | gate logs |
| Review | Tier 2 (or 1) | Approved | review on the pull request |

## Evidence and attempts

Tools are in main `local/evidence/split-riders-under-ink/`. Base binaries are main ec05ea7's
(`base-ec05ea7/`).

| Attempt | Hypothesis | Experiment | Observation | Next decision |
| --- | --- | --- | --- | --- |
| 1 | The idle rider 1's pose is its look | `look_compare.py` (runner `--look-timeline`) on `vs-idle` | Rider 0 equal throughout. Rider 1 diverges on 2722: native starts its second scripted glance; the original's `$0D5D` latch was cleared on 2720 at the first glance's end (`$82:87AD`) and re-latches on 2810. | The look is race state: tier 1 |
| 2 | The engine runs the look and clears the latch | `ZoomZooState::look`, `advance_rider_look` at `$83:CDA6`, the latch clear; the history reads the previous state's look | The look equals the original's on 5 captures; `vs-idle` 350 → 796 equal pictures, `league-idle` 3/3 | Ink over BG1 |
| 3 | A rider behind a BG1 priority tile still shows through the ink (sub screen = objects) | Skip the BG1 test where ink covers | `leaguefin` 121/121, `zz2p` +3, `zzap` 6,073/6,074, `mike1`/`p2pause` all equal | Serialization |
| 4 | Two-view states must carry the look | A 46-byte look block in the split trailer and the league wrapper, with guards | Restores across the clear equal; 20 captures equal in words (with `$0D5B`/`$0D5D`) and look on every race frame | Records, gates, review |

Left: `zzap` 6317, 22 pixels of rider 0's seat in the lower view (R-0083, "Not covered").
