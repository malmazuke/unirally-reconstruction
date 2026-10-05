# SPLIT-RIDERS-UNDER-INK - the riders under a split race's HUD text

## Assignment

- Status: ready (prepared 6 October 2026 by the SPLIT-CAPTIONS session, from R-0082's remaining
  race-time differences).
- Milestone: M4 breadth
- Coordinator: the claiming session is coordinator, primary and integrator
- Task provider: to be recorded at claim
- Worker/session/runtime/model: to be recorded at claim
- Actual model/reasoning effort, routing rationale: **tier 2** (D-0008), if the cause is the rider
  look's presentation history (`ClassicRaceHistoryTracker`, R-0036); escalate to tier 1 if it is
  race state.
- Provider quota window (D-0004): sample at claim.
- Dependencies: SPLIT-CAPTIONS (R-0082), R-0036 (rider look overlays).
- Base commit: the `main` tip at claim.
- Branch and isolated worktree: `task/split-riders-under-ink` in `.worktrees/split-riders-under-ink`.
- Owned paths: the rider look and overlay code (`src/core/presentation.cpp`, the look tables),
  native tests, a research record, this record, `docs/STATE.md`, `tasks/README.md`,
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
