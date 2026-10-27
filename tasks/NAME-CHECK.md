# NAME-CHECK - the keyboard's name check

## Assignment

- Status: **accepted** 27 October 2026 (tier 1, [#87](https://github.com/malmazuke/unirally-reconstruction/pull/87),
  accepted in review round 2 at `bd9a920a`), integrated by merge commit. Claimed 04:31 UTC on main
  `1cf871fa`, on the user's instruction
  (Claude Code desktop app, Claude Opus 5.5 `claude-opus-5-5`; coordinator and integrator; a worker
  subagent implements, a fresh subagent reviews). Queued 14 October 2026 by
  [MENU-INPUT](MENU-INPUT.md) (R-0097). The first task under
  [D-0013](../docs/decisions/D-0013-task-time-and-disk-budget.md): its claim-to-merge time is
  measured against the 6-hour target.
- Choice: the next queued player-visible gap (names with listed words are accepted natively).
  PODIUM-RETURN-TIMING needs the same cycle model this task may build, so it follows.
- Branch `task/name-check` in `.worktrees/name-check`; evidence in main `local/evidence/name-check/`.
- Task provider: Anthropic.
- Milestone: M4 (original game coverage).
- Review tier: **1** (menu state and frame timing in `src/core`).
- Dependencies: MENU-INPUT (R-0097), OPTIONS (R-0072).

## Why

The rename keyboard's OK runs `$83:84D9`, which compares the name with 71 words (`$83:8537`) at
every position and on a match prints `$80:A4C9` and refuses it (`$80:A4B8-A4C6`). Native has no
check and accepts every name. The check also takes about 39 scanlines a character, so from about
eight characters OK's frame ends without its frame wait and everything after comes a frame later
(every full-length rider name; `league-name`, review `rv-kb`).

## Outcome

- The refusal: the word list read from the ROM at pack build time (not committed), the message
  and the keyboard staying open, captured with names that contain listed words.
- The lag: a model of the check's run time that predicts the frame OK ends on, checked against
  captures of names of every length; or, if it cannot be predicted from the front end's state,
  a recorded limit with the evidence.

## Checkpoint - 27 October 2026 (implementation worker, Claude Opus 5.5)

Record: [R-0101](../docs/research/R-0101-name-check.md). Evidence: main `local/evidence/name-check/`
(three probe captures `probe-nc-*` with their manifests from `make_names.py`, three sound schedules
`nc-*.cues.txt` with their stripped `*-disp1/2` captures, `cycles-*.json` for all 24 calls of the
check in this task's and MENU-INPUT's captures, the baseline `base-1cf871fa/`, `checks.list` and its
run `checks-run.out`). Candidate on pack v41, main `1cf871fa` on v40.

| Item | Native before | Native after | Captures and result |
|---|---|---|---|
| The refusal (`$83:84D9`, `$80:A4B8-A4C6`) | every name accepted | every position against the 71 words from the pack; a match prints `$80:A4C9` (printer place kept), the keyboard stays open | `nc-refuse` (start, middle, overlapping, full length, near miss) pictures 1,206/1,206 (main 119); `nc-league` 17-letter refusals 910/910 (main 139) |
| The late frames | OK's commit the next frame at any length | the check's cost model (R-0101 finding 2) gives 0, 1 or 2 late frames: no frame wait, only the NMI | `nc-lengths` 1-8 letters 3,402/3,402 (main 3,203); `league-name` 2,400/2,400 (main 2,143); `rv-kb` 2,300/2,300 (main 1,879) |
| The model against the original | none | `model.py`: 24 of 24 calls in their measured frame, CPU clocks exact (22) or within 40 (2); nearest boundary 7.9 lines | `cycles-*.json` |
| Unchanged screens | | | `kb-input`, `w5`, `warnings`, `league-pick` all pictures equal, as main |

Sound schedules (frame waits included, from 620): `nc-refuse` 2,236 lines, `nc-lengths` 4,489,
`nc-league` 2,579, `league-name` 2,273, `kb-input` 1,945, `w5` 3,636: 0 frames differ (main: 161,
5, 97, 5, 0, 0).

Remaining differences (R-0101 Not covered): R-0097's one-frame text buffers (OK's OPTIONS print, one
text frame per commit; GROUP TABLES' first print); the check's start is a constant (over the measured
start range only an accepted 15-letter league name with 118-134 matched characters may be a frame
off, and OK pressed with A on the OK key may start later than the model's start); a refusal's print
across a boundary is not split (league names only: a word starting at the 7th or 8th letter).

## Handoff (worker)

- Branch `task/name-check`, base claim `f9ebf388` on main `1cf871fa`. Commits: `d31e0a52` (pack v41:
  rules, loader, tooling tests), `c278ba98` (the check, the late frames, native tests), `6ecc3b33`
  (R-0101, regenerated `docs/map/static/`), then this record.
- Pack: profile v41 appends `front-end.name-check-words` (`$83:8537`, 578 bytes) and
  `front-end.name-refused-text` (`$80:A4C9`, 18 bytes), from the ROM at build time; no earlier
  payload changes; rules SHA-256 `408ec862...`. Built locally as
  `local/classic-pal-crawler-tracks-v41.pack`. A v40 pack gives main's behaviour (no check, no lag),
  so the catalogue's v40 front-end checks do not exercise it.
- Checks run: `ctest` app-debug 41/41 and lab-debug 41/41 (new `name_check_tests` in
  `front_end_tests`); `clang-tidy` (LLVM 19) clean on `name_check.cpp`, `options_menu.cpp`,
  `front_end.cpp`, `content_pack.cpp`; `clang-format` 19 applied; `coverage native-symbols --check`
  passed, 0 of 1,663 cited addresses without a record (`regen.sh`, the five raw coverage inputs);
  tooling tests 618 OK; `checks.list` 16 of 16 lines rc 0 (`checks-run.out`). Synthetic suite and
  gates not run by the worker.
- Disk: `local/evidence/name-check/` 583 MB before compaction (largest: `nc-lengths-disp1` 129 MB,
  `probe-nc-lengths` 123 MB).
- Time (claim 15:31 AEDT to handoff about 16:45, 1 h 14 min wall clock): reading and planning about
  10 min; captures and measurements (manifests, traces, probe and sound captures, `cycles.py`,
  `model.py`) about 25 min; implementation (pack, native, tests) about 20 min; checks and records
  about 20 min, partly overlapping the runs.
- Next: the coordinator's review and gates. Follow-up for the catalogue: add `checks.list`'s lines
  as NAME-CHECK's checks. PODIUM-RETURN-TIMING can reuse `cycles.py`'s step costing.

## Review (rounds 1 and 2, accepted `bd9a920a`)

A fresh subagent in its own checkout at the PR head.
- [Round 1](https://github.com/malmazuke/unirally-reconstruction/pull/87#pullrequestreview-5662816114)
  at `bc5e1db3`: accept, nothing blocking. Two should-fix findings (the `start_clocks` comment gave
  only this task's start range; the records did not say the calls mix Start and A on the OK key, nor
  bound the ambiguous case) and four notes (12 words, not ten; the frame wait's explanation; the
  refusal print's limit is league-only; `model.py` repeats the native constants). All six fixed in
  `bd9a920a`.
- The reviewer reproduced `nc-refuse` and `nc-league` pictures, the `nc-league` and `nc-lengths`
  sound schedules and `model.py`, rebuilt pack v41 to the same SHA-256, ran `ctest` lab-release
  41/41, linked `name_check.cpp` into a harness that agrees with a plain reading of `$83:84DD-8534`
  on 200,022 names, searched every typeable name for the frames late (`review-1/sweep.py`) and ran
  the candidate on packs v40 and v39 (v40 gives main's sound schedule byte for byte).
- [Round 2](https://github.com/malmazuke/unirally-reconstruction/pull/87#pullrequestreview-5662832517)
  at `bd9a920a`: accept, no new findings.

## Gates (D-0012)

`local/locks/heavy-run.sh NAME-CHECK python3 -m tools.unirally_lab.gates run --task-checks
local/evidence/name-check/checks.list` at `bd9a920a` (main `local/evidence/name-check/gates-1*`).
- The change touches `tools/unirally_lab/content/`, so every step was selected: 261 steps, 2 h 16 min.
- The task's 16 checks passed (pictures and sound schedules as in the Checkpoint).
- 15 steps failed or changed because the runner gave main the candidate's pack v41, which main cannot
  read: each failed step's main-side run ended in 0-2 s. Rerun with `--base-pack
  local/classic-pal-crawler-tracks-v40.pack --only` those 15 (`gates-2*`, 15 min): all passed or
  `same-as-main` except `mi-cues`, changed by this task as expected (`league-name`'s schedule now 0
  frames differ, main 5; `awards-press` 1 frame on both sides, as MENU-INPUT recorded).
  `gates-3*`: `mi-cues` with `--expect-changed mi-cues`, `status=passed`.
- Hosted CI green on all three checks.
- Not done here: the catalogue (`tools/unirally_lab/gates/catalogue.py`) does not yet hold this
  task's checks, and its front-end checks still run pack v40. Adding them changes the gate tooling,
  which selects every step again; it goes with the next change to the catalogue or the next
  `--full --record` run.

## Handoff (closeout)

- Commits on `task/name-check`: `f9ebf388` (claim), `d31e0a52`, `c278ba98`, `6ecc3b33`, `bc5e1db3`
  (worker), `bd9a920a` (review round 1's fixes), then this record.
- Time, claim 04:31 UTC to merge about 08:45 UTC, about 4 h 15 min against D-0013's 6 hours:
  worker 1 h 14 min; PR and review setup 5 min; review round 1 11 min; fixes 5 min, round 2 under
  a minute; gates 2 h 16 min (the whole selection, as the pack tooling changed), the rerun 15 min;
  closeout about 10 min. The gates were half the time; a pack bump that leaves
  `tools/unirally_lab/` alone, or `--base-pack` from the start, would have saved 2 h 30 min.
- Disk: `local/evidence/name-check/` 583 MB from the worker, under the 1 GB budget; gate output
  compacted at closeout.
- The closing session removes `.worktrees/name-check` and `.worktrees/name-check-review` and the
  local branch.
