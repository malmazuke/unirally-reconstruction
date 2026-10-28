# SPLIT-HUNTER-PICTURES - HUNTER's effects in a two-view race's picture

## Assignment

- Status: **accepted** 28 October 2026 (tier 2, [#89](https://github.com/malmazuke/unirally-reconstruction/pull/89),
  approved in review round 1 at `87bd3254`), integrated by merge commit. Claimed 01:07 UTC on main
  `9980a38a`, on the user's instruction
  (Claude Code desktop app, Claude Opus 5.5 `claude-opus-5-5`; coordinator and integrator; a worker
  subagent implements, a fresh subagent reviews). Queued 10 October 2026 by
  [CREDITS-NAME](CREDITS-NAME.md) (R-0094). Run under
  [D-0013](../docs/decisions/D-0013-task-time-and-disk-budget.md): claim to merge against the
  6-hour target, new evidence under 1 GB.
- Choice: the next queued player-visible gap after PODIUM-RETURN-TIMING.
- Branch `task/split-hunter-pictures` in `.worktrees/split-hunter-pictures`; evidence in main
  `local/evidence/split-hunter-pictures/`.
- Milestone: M4 (original game coverage).
- Review tier: **2** (presentation only: the race state already matches, R-0094; one independent
  round, escalated to tier 1 if the fix touches race state).
- Dependencies: CREDITS-NAME (R-0094), HUNTER-EFFECTS (R-0052), SPLIT-SCREEN-RACE (R-0069).

## Why

"faedine" puts any race into HUNTER mode, two-view races included. Their state matches the
original (R-0094's `demo-faedine-0`: 2,201 of 2,201 rows), but the split renderer does not draw
HUNTER's effects: pictures equal on 1,194 of 2,254 frames against 2,129 with the name "mike"
(wobble mode's mosaic for its 500 frames, parts of the invisible track and hedgehog speed, the
"invisible unis" span at frames 3448-3645, the two caption rows).

## Outcome

The split picture draws every HUNTER effect as the original does, over the review's
`demo-faedine-0` and further two-view captures (2P, VS, league races with tags).

## Checkpoint (worker, 28 October 2026)

What the split picture missed, and what native now does ([R-0103](../docs/research/R-0103-split-hunter-pictures.md)):

1. **Invisible unis.** A two-view race's screen flip builds no BG1 table and turns no object over
   (`$83:D337-D343`); its visibility routine points both riders' objects at blank tiles while the
   blink is on (`$82:B154-B17F`). Native flipped BG1 and the riders; now BG1 is drawn as usual and
   no rider shows.
2. **Wobble mode's mosaic** counts its rows from the picture's first line, also in the lower view.
3. **Rider 1's caption row:** a front-of-queue push is not a take; a take and an effect's end in
   one update keep the taken event; the dry look copies the HUD message buffer, the effect's name
   (`$81:BFB9-BFD0`).
4. **Skipped updates** (hedgehog speed, slow motion) ask for no lower clock and keep the arrow
   word's marker (the skipped race update runs neither `$81:C6D1` nor `$82:9822`).
5. **The idle demo's caption rows** are flat ink over the riders (`$212D` = 0), like its HUD: also
   the "mike" demo's 125 unequal pictures on main.

No race state, serialized format or pack change. Captures (main `local/evidence/split-hunter-pictures/cap/`):
`demo-pic` (the review's `demo-faedine-0`, re-probed with work RAM to `$3FFF`), `twop-104`,
`vs-124` and `vs-4` (2P and VS ZOOM ZOO, seeds from a native search, `search.py`), `one-67` (one
player, slow motion and hedgehog speed), and the review's `demo-pic-mike`.

Pictures equal to the original's (`checks.list`, run once at `70aa9390`, `checks-70aa9390.out`;
candidate and main `9980a38a`, pack v41):

| Capture | Candidate | Main |
| --- | ---: | ---: |
| `demo-pic` | 2,245 of 2,254 | 1,194 |
| `demo-pic-mike` | 2,254 of 2,254 | 2,129 |
| `twop-104`, to 5547 | 3,648 of 3,648 | 2,658 |
| `vs-124`, to 4980 | 3,081 of 3,081 | 2,567 |
| `vs-4` | 6,095 of 6,100 | 4,398 |
| `one-67` | 3,650 of 3,650 | 3,650 |
| LEAGUE-TOURS `tours-a` w2 (every tenth frame) | 2,557 of 3,889; 459 of 664 while an effect runs | 2,235; 139 |
| LEAGUE-TOURS `tours-a` w6 | 2,032 of 3,890; 460 of 664 | 1,706; 137 |

The 14 unequal candidate pictures of this task's captures differ on line 113 only, at a six-line
mosaic block reaching into the upper view. The cause is a hypothesis: a BG1 map row native does
not model (R-0103's Not covered names the VRAM capture that would test it). In the
league windows no picture is worse than main; the effect pictures still unequal are on track 42
(NEON's split picture, unequal before its first tag as on main) but one (8 pixels, as on main).

**Found, not fixed (race state, tier 1):** in a 2P or VS race, power bounce changes rider 1's
landings too. `twop-104` diverges at frame 5548 (update 3521) and `vs-124` at 4981 (update 2954),
rider 1's velocity, both under effect 2; native gives landing matrix 0 to rider 0 only
(`race_update.cpp`, `index == 0`). The checks stop before it.

## Handoff (worker)

- Branch `task/split-hunter-pictures` on claim `7823a613` (main `9980a38a`). Commits: `ca0120af`
  (native: `presentation.cpp`, `presentation.hpp`, `race_hud.cpp`, `presentation_tests.cpp`'s
  `split_hunter_captions`), `c49a3e34` (clang-format, regenerated `docs/map/static/`), then the
  records commit (R-0103, this record, the maps again).
- Checks run: `ctest` lab-debug 41/41 and app-debug 41/41; `clang-tidy` (LLVM 19) clean on
  `presentation.cpp` and `race_hud.cpp`; `clang-format` 19 applied to the `src/core` files (the
  test file keeps its own style); `regen.sh` (five raw coverage inputs) and `coverage
  native-symbols --check` passed, 0 cited addresses without a record; `checks.list` 8 of 8 lines
  rc 0 at `70aa9390` (`checks-70aa9390.out`, 24 min under the heavy-run lock; the code is
  `c49a3e34`'s, which gave the same numbers). Synthetic suite and gates not run by the worker.
- Commands: `zsh local/evidence/split-hunter-pictures/captures.sh NAME...`;
  `local/locks/heavy-run.sh SPLIT-HUNTER-PICTURES zsh local/evidence/split-hunter-pictures/run_checks.sh OUT`;
  `python3 local/evidence/split-hunter-pictures/race_check.py CAPTURE BINARY PACK --boundary`.
- Evidence: main `local/evidence/split-hunter-pictures/` 168 MB after the captures' work RAM
  series were stored as `series.bin.zst` (verified byte for byte); `base-9980a38a/` is main's
  runner. Native picture folders deleted.
- Not covered: line 113 (14 pictures); power bounce in two-human races (above); the league's
  two-view races on LEAGUE-TOURS' sampled pictures only, and track 42's split picture (NEON)
  unequal with or without effects; the NMI's channel 4 reading under the blink is not reconciled
  with the unflipped BG1 the pictures show.
- Time (claim 01:07 UTC to handoff 03:14 UTC, 2 h 07 min, `date -u`): reading and baseline
  25 min; captures (five, about 3 min each in process) and the native search 15 min; diagnosis and
  native code and its test about 45 min; checks (two full runs, about 25 min each), records and maps
  the rest.
- Next: the tier-2 review, then the gates (`--task-checks local/evidence/split-hunter-pictures/checks.list`).

## Review (round 1, approved `87bd3254`)

A fresh subagent in its own checkout at the PR head
([review](https://github.com/malmazuke/unirally-reconstruction/pull/89#pullrequestreview-5677756737)):
approve, no blocking findings, no escalation (no race state touched).
- It recaptured `demo-pic`, `vs-4` and `twop-104` from the manifests, byte for byte the worker's,
  with the same picture counts.
- It replayed the gate corpus (503 runs) through main and the candidate: only pictures differ, in
  24 runs, and none is worse than main against the original (idle demo 963 to 991 of 1,006; four
  league windows gain 9-480).
- Should-fix, both fixed in `c2900f86` (records only): the line-113 cause is now labelled a
  hypothesis with a VRAM capture as the next experiment; the power-bounce note now cites
  `$81:94A8-94B2` and the original's RAM, and is queued as
  [TWO-VIEW-POWER-BOUNCE](TWO-VIEW-POWER-BOUNCE.md).
- Notes left for the next change to `race_hud.cpp`: `classic_race_lower_arrow` has only test
  callers; the kept arrow marker and `invisible_riders` have capture checks but no unit test.

## Gates (D-0012)

`local/locks/heavy-run.sh python3 -m tools.unirally_lab.gates run --task-checks
local/evidence/split-hunter-pictures/checks.list --expect-changed <the review's list>` at
`c2900f86` (main `local/evidence/split-hunter-pictures/gates-1*`, 2 h 26 min): `status=passed`.
- The task's 8 checks ran and passed.
- 8 steps changed as expected: `idle-demo`, `lt-tours-a-w2`, `-w6`, `-w8`, `lt-tours-b-w1`,
  `lt-odd-tours-w0`, `-w1`, `lt-five-w1`; `lt-t-crawler` same as main. Every other selected step
  passed or matched main.
- Hosted CI green on all three checks.
- The gate catalogue does not hold this task's checks yet (as for NAME-CHECK and
  PODIUM-RETURN-TIMING).

## Handoff (closeout)

- Commits on `task/split-hunter-pictures`: `6120ab48`, `7823a613` (claim), `ca0120af`, `c49a3e34`,
  `70aa9390`, `87bd3254` (worker), `c2900f86` (review round 1's records fixes), then this record.
- Time, claim 01:07 UTC to merge about 07:00 UTC, about 5 h 55 min against D-0013's 6 hours:
  worker 2 h 07 min; PR and review setup 5 min; review 1 h 08 min; fixes 10 min; gates 2 h 26 min;
  closeout about 10 min. The one review round and no fix-round reruns kept it inside the target.
- Disk: main `local/evidence/split-hunter-pictures/` 262 MB with the review and gate output, under
  the 1 GB budget; compacted at closeout.
- The closing session removes `.worktrees/split-hunter-pictures` and
  `.worktrees/split-hunter-pictures-review` and the local branch.
