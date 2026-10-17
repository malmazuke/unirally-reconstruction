# PODIUM-AUDIO - the league podium's music

## Assignment

- Status: **accepted** 17 October 2026 (tier 1, [#82](https://github.com/malmazuke/unirally-reconstruction/pull/82),
  review round 2 approved `b7efb93`), integrated by merge commit. Claimed 17 October 2026 on main
  `6c50ac1` (Claude Code desktop app, Claude Opus 5.5 `claude-opus-5-5`; coordinator and
  integrator; a worker subagent implements, a fresh subagent reviews). Queued 16 October 2026 by
  [LEAGUE-TOURS](LEAGUE-TOURS.md) (R-0098). Weekly usage at claim 14%.
- Choice: the smallest player-visible gap in the queue (the league's podium is silent), with its
  original schedule already captured (`tour-cues`); ahead of PODIUM-RETURN-TIMING (needs a
  cycle-count model), RECORD-SCREENS, NAME-CHECK, SPLIT-HUNTER-PICTURES and GATES-AGAINST-MAIN.
  No other session holds a claim (the `task/player-hint-voices` worktree is from 7 October). The
  user asked for the next task.
- Branch `task/podium-audio` in `.worktrees/podium-audio`; evidence in main
  `local/evidence/podium-audio/`.
- Task provider: Anthropic.
- Milestone: M4 (original game coverage).
- Review tier: **1** (sound cues and their frame timing in `src/core`, possibly the pack, D-0010).
- Dependencies: MODE-AUDIO (R-0091), LEAGUE-TOURS (R-0098).

## Why

Native plays nothing on the league's podium, where the original starts `$82:E08E`'s sound session
and its music. That shows in `tour-cues` at 11420 and 11501-11502. The menu fade before the podium
and the title set after it are already native.

## Outcome

- The podium's session and music equal to the original's cue schedule, frame waits included.
- The app's audio producer runs through the podium.

## Checkpoint - 17 October 2026 (implementation worker, Claude Opus 5.5)

Record: [R-0099](../docs/research/R-0099-podium-audio.md). Evidence: main
`local/evidence/podium-audio/`:
- `cues/`: the schedules, the stripped cue captures `*-disp1`/`-disp2`, `tour-cues-full`, the
  derived lists `*.cues.txt` and the scripts;
- `listing/`, the static listing;
- `anchors.json`, `checks.sh` and `app_run.sh`;
- `base-6c50ac1/`, main's binaries.

**What `$82:E08E` is.** "ddcd" was LEAGUE-TOURS' derivation label for the podium routine
`sub_82DDCD`'s score site.
- The session `$82:E06F-E102` loads the medal award's set: driver 50, tables 52, score 58 and the
  samples of `$83:FBF5`. That set is in pack v34, so there is no pack change.
- The session ends with eight polls, effect gain 255 and sixteen polls.
- The podium loop's first three passes (`$82:DEBC-DEFE`, counted in `$1365`) each send effect gain
  255, music gain 127 and the music start, with eight polls.
- Over eight captured podiums, the session is on the entry's frame 19, its upload ends 79 frames
  later, and the passes are 80, 81 and 82 frames after the session.
- LEAGUE-TOURS' derivation hid the first pass's words and counted the loop's polls as the
  session's, so the real difference was 34 lines per podium, not 7.

**Native now** (`67261a2`):
- `AudioSessionLoad::podium` on the entry's frame 19.
- `podium_music_pass`, with `LeagueSetup::podium_music_passes`.
- Three dispatch sites `podium-1`/`-2`/`-3`, each pass at its own anchored clock (the medians:
  session 340,626; passes 84,221, 123,126 and 90,525).

**Measured** (the podium windows, frame waits included):

| Schedule | Podiums equal | Elsewhere: candidate / main | Music start's pass, original / native | Podium PCM within 1 dB (fade to way back) |
|---|---|---|---|---|
| `tour-cues` | 1 of 1 | 1 / 1 | 1 / 2 | 362 of 724 (50.0%) |
| `full-tour` | 1 of 1 | 8 / 8 | 2 / 1 | 393 of 773 (50.8%) |
| `pa-t-crawler` | 1 of 1 | 52 / 52 | 2 / 2 | 750 of 773 (97.0%), median 0.03 dB |
| `pa-tours-b` | 2 of 2 | 62 / 62 | 2 / 2, 2 / 2 | 690 and 698 of 773 (89.3%, 90.3%) |
| `pa-tours-a` | 2 of 2 | 17 / 17 | 2 / 2, 2 / 2 | 682 and 761 of 773 (88.2%, 98.4%) |
| `pa-odd-tours` | 1 of 1 | 2,411 / 2,411 | 2 / 2 | 768 of 773 (99.4%) |

- Each podium window has 35 original lines, all equal; main differs on 4 frames in each. Outside
  the podiums the candidate's differing frames and lines are exactly main's, and its logs add only
  the 34 podium lines per podium.
- `tour-cues` with LEAGUE-TOURS' own derivation differs only at 10,603, as on main (native's log
  is read with that derivation's conventions).
- The conditional driver check on `tour-cues-full` (title, race-62, title, podium as award, title,
  race-63): 2,712,437 ordered DSP/SMP writes and 10,763,786 raw pairs, all equal.
- Whole-schedule anchored PCM: `tour-cues` 94.8% of 16,148 windows within 1 dB (259 of 259
  commands in order).
- The app (`app_run.sh`, app-debug, `--native-title-menu-audio`): a warm boot from a podium-pending
  cartridge reaches the podium from NOW PLAYING.
  - The producer runs 2,980 cued frames and never stops, with no later drops and no underruns. The
    only drop is the boot's silence (2,815 pairs, peak 0).
  - Its cue log equals `front_end_runner`'s (2,414 lines, 25 podium lines).
- Found on the way, outside the task: with LEAGUE's `organic-full-tour-turnaround` inputs the app's
  second league race diverges from `front_end_runner`'s at frame 12,619 and never finishes. Main
  `6c50ac1`'s app does the same (its cue log equals the candidate's through frame 12,999), so the
  app's scripted replay of two-human league races needs its own follow-up.

## Attempts

| # | Hypothesis | Experiment | Result | Next |
| --- | --- | --- | --- | --- |
| 1 | "ddcd" is a new sound program the pack lacks | The listing at `$82:E06F-E102` | No: driver 50, tables 52, score 58 and slots `$83:FBF5` are the award's (`$83:A624-A640`) | No pack change |
| 2 | The 7 lines are the whole difference | `podium_rows.py` on LEAGUE-TOURS' captures | No: the first pass's three words were skipped by the derivation and the loop's polls dropped as the session's | Refine the derivation (`derive_cues2.py`) |
| 3 | `$1365` is 0 at every podium's entry | WRAM in the eight LEAGUE-TOURS probes with podiums | Yes: 0 to 1 on the first pass in every one | The count starts at 0 |
| 4 | One anchor fits the loop's polls | The first polls of 24 passes | No: each pass has its own clock (84k, 123k, 90k; spread under 600 clocks) | Three sites, a median each |
| 5 | The words reach the driver in the original's passes | `podium_commands.py`, `native_commands.sh` | In 6 of 8 podiums. The music start sits on the edge of the first pass's last poll, and the driver's phase after the upload decides it (original pass 1 only in `tour-cues`) | Recorded (D-0010) |

## Decisions

- **Derivation.** The podium's cues are compared with a refined derivation (the session's own
  word count, and the loop's polls as their own sites), not LEAGUE-TOURS' list. `tour-cues` is
  also checked against that list with its conventions.
- **Anchors.** Each of the three passes gets its own anchor (D-0010: a fixed position for each
  dispatch opportunity); one median would put two passes 33,000 clocks off.
- **No pack change.** The award's set (v34) is the podium's.
- **The front end assumes `$1365` = 0** at the podium's entry (attempt 3). A podium entered
  otherwise is recorded as not covered.

## Handoff (worker)

- Branch `task/podium-audio`. Claim `2f9f19b` on main `6c50ac1`, then `67261a2` (native change and
  unit test), and the record commit that holds this section.
- Files: `src/core/audio_cue.hpp`, `audio_cued_scenes.cpp`, `league_completion.cpp`,
  `front_end.hpp`, `tests/native/league_tests.cpp`, `docs/research/R-0099-podium-audio.md`,
  `docs/map/static/*` (regenerated, `regen.sh` with the five raw coverage inputs), and this record.
- Commands: `bash local/evidence/podium-audio/checks.sh OUTDIR`, run from the worktree through
  `local/locks/heavy-run.sh`. It covers the cues against the candidate and main, the anchors, the
  driver check, the PCM and delivery pass per podium, the app run and the league frozen gate.
  The captures are in R-0099's Reproduction.
- Limits:
  - the music start's pass in 2 of 8 podiums (PCM about 50% within 1 dB there);
  - the podium's way back a frame late (PODIUM-RETURN-TIMING);
  - a press on the second or third pass, from the listing only;
  - `$1365` assumed 0.
- Final checks at `63ce259` (`final_checks.sh`, main `local/evidence/podium-audio/final-63ce259.out`
  and `final-63ce259/checks.out`, 23:08-23:31 UTC on 16 October 2026):
  - app-debug, lab-debug and lab-release build; ctest passes 41 of 41 on app-debug and lab-debug;
  - clang-tidy reports 0 warnings and 0 errors on `audio_cued_scenes.cpp` and
    `league_completion.cpp`, with no function over 80 lines; clang-format reports 0 warnings;
  - `coverage native-symbols --check` is current, with 0 uncited addresses, and the map tooling
    tests pass;
  - `checks.sh` gives the tables above, and LEAGUE's frozen gate (`league_frozen.py`, pack v40)
    passes.
  - Not run by the worker: the synthetic suite, the coordinator's gates, and the ASan presets
    (unavailable on this host).
- Evidence housekeeping:
  - The redundant full capture `full-tour-full` is stripped to its CPU rows; its first cue capture
    keeps the same PCM.
  - `tour-cues-full`'s events are gzipped, with `SHA256SUMS` beside them.
  - The evidence directory is about 6 GB.
  - Main's toolchain manifest is unchanged: no bootstrap ran in the worktree (`local/` is linked),
    and a backup is in the evidence directory.
- Next: the coordinator's gates and the tier-1 review. A follow-up worth queueing is the app's
  scripted replay of two-human league races (above).

## Review round 1 (returned on `0e39a79`) - the fixes

Review: https://github.com/malmazuke/unirally-reconstruction/pull/82#pullrequestreview-5556120775
(main `local/evidence/podium-audio-review/review-0e39a79.md`). The reviewer's captures are in main
`local/evidence/podium-audio-review/cues/`: `rv-press1`, `rv-press2` and `rv-up-pad2`, with their
scripts. They are now part of `checks.sh`.

1. **A press on the podium's first pass:** fixed in `c8c358f`.
   - The entry's frame 99 ran the loop's first pass with no pads, but the original reads them on
     every pass (`$82:DE06-DE45`). `league_podium_entry_frame` now takes the frame's pads and stops
     when that pass leaves.
   - `rv-press1` (A held over 11,490-11,509): the podium window is equal. That is 2 original lines,
     the session and the way back at 11,504, with no music. Main differed on 3 frames there.
   - Unit test: `league_tests` presses A on frame 99. There is no music, the screen is the way
     back, and `$1365` stays 0.
2. **The exit test:** fixed in `c8c358f`, at the coordinator's decision. `leaves_podium` is the
   original's predicate, read from the listing:
   - `$80:85E8-85FD` copies `$4218-$421B` each frame.
   - `$82:AA75-AC4C` sets a flag per button and `$0319`/`$031B`, which are 0 for Left, 2 for
     Right and 1 for neither.
   - `$82:DE10-DE5B` leaves on any of A, X, L, R, B, Y, Select and Start of either pad, or while
     Left or Right is held on either pad. Up and Down are never tested.
   - The flags are the pad's state that frame (level, not edge). The pass reads them that frame,
     with no latency beyond native's existing frame mapping; `rv-press1` and `rv-up-pad2` leave on
     the original's frames.
   - The reader's mode flags (`$12DF`, `$12E1`, `$0C6D`, `$7E:212C`, `$1335`) are 0 in all eight
     probed podiums. Native assumes so (R-0099 Not covered).
   - Only the podium's test changed. `any_button_pressed` (`$80:B6D3`) stays for its other users:
     the award, the hunter ending, the stunt and race results.
   - `rv-up-pad2` (Up on pad 1, then Start on pad 2): the podium window is equal, all 35 lines,
     staying through the Up and leaving at 11,604. Main differed on 8 frames.
   - Unit test: `podium_exit_test` presses each of the ten exit buttons on each pad (all leave),
     Up and Down on either pad (none leave), and no press.
3. **R-0099 corrected** (`c8c358f`):
   - finding 3 says the loop leaves on its exit test;
   - the new finding 7 covers the first pass and the predicate, with the review's captures as
     evidence;
   - Not covered now lists the third pass's press as from the listing only, the reader's flags,
     and the way back after an early exit.
- **Advisories, all done in `c8c358f`:**
  - `front_end.hpp`'s comment and R-0099 finding 3 cite the `$1365` evidence. A race leaves it
    0x7F or 0xFF. A work-RAM clear on the entry's frame 7, 12 frames before the session, zeroes it.
    The review's `trace_1365.py`, rerun here, shows this in all 17 probed podiums (15 at 0x7F, 2 at
    0xFF), each with the first pass 92 frames later. The clearing routine is not identified.
  - `$0545`'s skip at `$82:DDDE-DDE7` is in finding 1 and Not covered.
  - `league_tests.cpp` is formatted with `src/core/.clang-format` (0 warnings).
  - `checks.sh` quotes every path, makes OUTDIR absolute, and refuses to run outside a worktree.
    `final_checks.sh` now invokes it by its absolute path.
- **Remaining on the review's captures:** after the way back's title session, `rv-press1`,
  `rv-press2` and `rv-up-pad2` differ on 5, 3 and 5 frames (main 10, 3 and 80 there). These are
  frame waits and the slide's sound a frame apart, R-0098's late way back (PODIUM-RETURN-TIMING).

**Checks at `c8c358f`** (`final_checks.sh` under the heavy-run lock, 04:29-04:51 UTC on 17 October;
main `local/evidence/podium-audio/final-c8c358f.out` and `final-c8c358f/checks.out`):
- app-debug, lab-debug and lab-release build; ctest passes 41 of 41 on app-debug and lab-debug.
- clang-tidy reports 0 warnings on `audio_cued_scenes.cpp`, `front_end.cpp` and
  `league_completion.cpp`, and clang-format reports 0.
- `native-symbols --check` is current (1,601 cited addresses, 0 without a record). The map is
  regenerated for the new citations, and its tooling tests pass.
- Every earlier podium window is still equal: 8 of 8, each candidate cue log byte-identical to
  `0e39a79`'s. The review's three podium windows are equal too.
- The driver check, anchors, PCM and the app run are as before (2,980 cued frames, no later drop).
- LEAGUE's frozen gate passes.
- Two untracked logs in the worktree root, `cond.log` and `league-frozen.log` from 14:29 AEDT,
  were the coordinator's: its first gate run invoked `checks.sh` with `bash`, not `zsh` (fixed in
  `gates.sh`). The coordinator deleted them.

## Review round 2 (approved `b7efb93`)

Review: https://github.com/malmazuke/unirally-reconstruction/pull/82#pullrequestreview-5557217524
(main `local/evidence/podium-audio-review/review-b7efb93.md`, by the same reviewer subagent).
- Both findings fixed: `rv-press1`, `rv-press2` and `rv-up-pad2` equal the original's podium
  windows, `leaves_podium` checked against `$82:DE10-DE5B` and `$82:AA75-AC4C`, and only the
  podium's predicate changed.
- Two new withheld captures equal the original: `rv-pad2-left` (Down held on pad 2 stays, Left
  held on pad 2 leaves, title session at 11,604) and `rv-press3` (a press on the third pass leaves
  before its music, title session at 11,506).
- The reader's mode flags and the high bytes the 16-bit ORs read are 0 on the first 120 loop
  frames of all 17 probed podiums (`podium_reader_flags.py`); a one-human league is untested.
- The reproduced checks equal the worker's: 8 of 8 earlier podium windows, cue logs byte-identical
  to `0e39a79`'s, the app run clean.
- Advisories (no change required), taken into R-0099 in the records commit: finding 7 counts 17
  podiums, not eight; `$1335` cannot change the exit test; the third pass's press is now captured.

## Gates - head `b7efb93` (main `local/evidence/podium-audio/gates-b7efb93.out`)

LEAGUE-TOURS' gates plus its and this task's `checks.sh`, against main `6c50ac1`'s binaries
`base-6c50ac1/` (pack v40 for both), from the task worktree, 17 October 2026 04:53-09:25 UTC.
- **Builds and suites.** Four presets build, and ctest passes 41 of 41 on each. Both synthetic
  suites pass. The v1 contracts, hidden app runs and the front end pass. Fuzz is as on main (40
  aborts, a recorded non-pass).
- **Differential gates.** The eleven differential gates pass with main's row digests. The race
  sweep has 432 runs and 0 differences. The front-end sweep has 179 of 179 equal to main.
- **Earlier tasks.** Every earlier comparison, schedule and restore check is as in LEAGUE-TOURS'
  `246cb3e` run. The DEMO-AUDIO app run drops only the boot's silence (2,815 pairs, peak 0; the
  first run on `0e39a79`, under the reviewer's builds, 3,775). LEAGUE-TOURS' `checks.sh` reads
  `tour-cues` with its own derivation, which hides the first pass's words, so it now shows 5
  frames, the podium's 4 of them the lines R-0099 finding 6 explains; with that derivation's
  conventions applied to native's log only 10603 differs, as on main.
- **This task's `checks.sh`.** Every podium window equal in the six schedules and the review's
  three captures; the driver check, anchors, PCM and app run as in R-0099. The first gate run, on
  `0e39a79`, ran `checks.sh` with the wrong shell; the worker's `final_checks.sh` measured that
  head instead.
- CI on `b7efb93`: `changes`, `lab (macos-15)` and `lab (ubuntu-24.04)` pass.

## Closeout

The pull request, its merge commit and check runs are in ignored main
`artifacts/podium-audio-integration/closeout.json`. The closing session:
- keeps main `local/evidence/podium-audio/` (6.0 GB: the schedules' captures `checks.sh` reads,
  `tour-cues-full` for the driver check, `base-6c50ac1/`) and `local/evidence/podium-audio-review/`
  (1.8 GB, the review's captures, which `checks.sh` reads);
- moves the worktree's gate logs (`artifacts/podium-audio/gates-*`, `final-*`) to main
  `artifacts/podium-audio-integration/`, and deletes the rest of the worktree's `artifacts/`
  (scratch: app runs, PCM and disassembly outputs no record cites) and both worktrees' build
  output;
- removes `.worktrees/podium-audio` and `.worktrees/podium-audio-review` and the local branch
  `task/podium-audio` (GitHub deletes the merged head branch).
