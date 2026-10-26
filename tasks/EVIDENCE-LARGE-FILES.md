# EVIDENCE-LARGE-FILES - the captures too large for transparent compression

## Assignment

- Status: **accepted** 26 October 2026 (tier 1, [#86](https://github.com/malmazuke/unirally-reconstruction/pull/86),
  approved in review round 2 at `2ab91928`), integrated by merge commit. Claimed on main `e3a41066`,
  on the user's instruction (Claude Code
  desktop app, Claude Opus 5.5 `claude-opus-5-5`; coordinator, primary and integrator; a fresh
  subagent reviews). Queued 25 October 2026 by [TASK-EFFICIENCY](TASK-EFFICIENCY.md) (D-0013), after
  [#84](https://github.com/malmazuke/unirally-reconstruction/pull/84), now merged.
- Branch `task/evidence-large-files` in `.worktrees/evidence-large-files`.
- Task provider: Anthropic.
- Milestone: M4 (process).
- Review tier: **1**. It changes how the differential gates read their reference captures.

## Why

After TASK-EFFICIENCY's compaction, about 40 GB of `local/evidence` is in files over 480 MiB, which
macOS does not compress transparently. They are 20 `series.bin` probe captures and 19
`memory.wram` race captures, mostly in `league-tours`, `m4-16-playable-zoom-zoo`,
`league-tours-review` and `zoom-zoo-opposing-input`. The data is per-frame work RAM and compresses
about 144× (D-0013).

## Outcome

- **The readers.** One helper that opens a capture as `NAME` or `NAME.zst`, used by every reader of
  these files:
  - the tracked readers `track_reference.py`, `zoom_zoo_playable.py` and `dragster_playable.py`;
  - the probe scripts that read `series.bin`.
- **The files.** Each of those files is converted to `.zst`, and the raw copy is deleted only after a
  byte-for-byte check.
- **The proof.** The gates give the same row digests before and after; D-0012's runner reports
  `same-as-main`.
- **The target.** `local/evidence` under 15 GB.

## What was done

- **The tracked reader.** `tools/unirally_lab/capture_files.py` (`stored_path`, `open_capture`,
  `read_capture`) reads a capture kept raw or as `NAME.zst`, preferring the raw file.
  - It decodes with `compression.zstd` (Python 3.14) or the `zstd` command.
  - The three race-capture readers (`zoom_zoo_playable.py`, `dragster_playable.py`,
    `track_reference.py`) open `memory.wram` and `memory.sram` through it. Each still checks every
    frame against its recorded SHA-256, so a wrong decode stops the check rather than passing.
  - Tests: `tests/tooling/test_capture_files.py`.
- **The evidence scripts** (local, not tracked). `local/evidence/evidence-large-files/patch_readers.py`
  rewrote the series-capture reads of 93 scripts in place, with backups in `script-backups/` and
  every patched script parsing:
  - whole-file reads `(X).read_bytes()` and `open(X, 'rb').read()` of `series.bin`,
    `wram-series.bin` and `sram-series.bin` became `_capture_bytes(X)`;
  - three seek-and-read streams became `_open_capture(X)`;
  - each patched script gains these two small functions.

  15 scripts name such a file without reading it this way. They are 14 capture writers and
  `classic-pause-menu/pictures.py`, whose captures stay raw.
- **The files.** `local/evidence/evidence-large-files/convert.py` stores each evidence file over
  480 MiB of these kinds as `NAME.zst` (zstd -10): the series and race captures, plus raw cue
  `events.bin` files and one native cond log.
  - Each file is compressed, decompressed again and compared by SHA-256 before the raw file is
    deleted.
  - The digests are in `converted.sha256`, the log in `convert.out`.

## Review round 1 (returned on `c0acb7f5`) - the fixes

[Review](https://github.com/malmazuke/unirally-reconstruction/pull/86#pullrequestreview-5645362122).
1. **The tests' import.** `test_capture_files.py` now puts the checkout root on `sys.path`: the suite
   discovers from `tests/tooling` and loads this module first. Fixed in `5264ecec`; the full gate
   run's synthetic step had failed on it.
2. **Cue `events.bin` files the gates read.** `podium-audio/cues/events.py` reads the three
   `pa-*-disp1` cue captures (steps `pa-anchors` and `pa-pcm-*`).
   - They were restored raw, digest-checked against `converted.sha256`, before the running full
     gate run reached those steps, and the steps match main.
   - The `demo-long` events and the native cond log stay converted: no current check reads them.
3. **Three scripts the patcher broke.** `attract-demo/compare_trailer.py`,
   `compare_camera_trailer.py` and `compare_corrected_trailer.py` had a method call's parentheses
   taken as the read. They are restored from `script-backups/`; their inputs were never converted.
4. **Readers outside the helper.**
   - The three `stunt-events/decode` race captures, read raw by their decode scripts, are restored
     raw.
   - The closeout's automatic `.zst` step for future large captures is dropped (`96be8587`), because
     evidence readers and `track_reference.py`'s `memory.sram` seek still open those files raw.
   - Unpatched scripts that name a series file in other forms read none of the converted captures
     except through the tracked readers. The full gate run, which runs every check, is the proof.
5. **Decoder coverage.** A new test makes the `zstd` command decode with the standard library
   module hidden, wherever the command exists (`47bc0a69`). The evidence scripts' helper also falls
   back to the `zstd` command before Python 3.14.
6. **Nits.** The compaction nits went with the dropped step, and D-0013's "Not done" bullet is
   updated. `open_capture`'s command fallback holding a whole capture in memory is recorded as a
   limit.

## Results

- **Converted.** 46 files, 38.64 GB, became 0.765 GB of `.zst`, each checked against its SHA-256
  (`convert.out`). After the restorations, 40 stay converted:
  - the league-tours, league-tours-review and two-human-stunts probes;
  - the M4-16, opposing-input, rider-art and dragster race captures;
  - the idle demo's series;
  - the `demo-long` cue events and the cond log.
- **Still raw over 480 MiB.** 5.6 GB in nine files:
  - the restored `stunt-events` and podium cue files;
  - M4-16's `baseline-a/b.wram`, which is not a capture kind the readers handle;
  - one `events.bin.gz`.
- **Disk.**
  - Free disk is 117 GB, against 92 GB before this task and 45 GB before TASK-EFFICIENCY.
  - `local/evidence` sums to 36.2 GB counted per path. That counts each of TASK-EFFICIENCY's 586
    APFS clones in full, so the physical figure is lower; APFS gives no per-directory measure of
    it.
- **Gates.** `gates run --full` at `c0acb7f5` took 222.7 min, with a peak step temp of 2.33 GB
  (main `local/evidence/evidence-large-files/gates-full-c0acb7f5.out`).
  - Every one of the 105 checks against the original, and each one's run on main, is
    `same-as-main`, including the LEAGUE-TOURS checks that read the converted probes and the race
    gates that read `memory.wram.zst`.
  - 31 steps passed. The synthetic step failed on finding 1, fixed since.
  - The full run with its main-side twins is the pre-milestone run, not a task's gate.

## Review round 2 (approved `2ab91928`)

[Review](https://github.com/malmazuke/unirally-reconstruction/pull/86#pullrequestreview-5646324173):
approved.
- **Round 1's fixes:** every one checked against the files and the record.
- **The converted captures were really read.** The 11 race gates ran in full (not frozen-digest)
  mode and passed. A main-side twin reads the same converted files, so `same-as-main` alone could
  not show this. Instead, the reviewer compared against the last gate run before the conversion
  (RECORD-SCREENS' `gates-3f8a1ab.out`), and these match it:
  - all 84 LEAGUE-TOURS `lt-*` lines;
  - the idle demo (34 of 34 cycles, 963 of 1,006 pictures);
  - `pa-anchors`.
- **Not caused here:** `lt-odd-tours-w0`'s main side fails as it did in LEAGUE-TOURS' accepted checks.
- **Notes:**
  - `two-human-stunts/podium_spin.py` still opens `series.bin` directly. No current check runs it.
  - The evidence helpers' command fallback was added about 10 minutes into the full run. Both
    versions decode the same way on Python 3.14.
- **The synthetic step** passed on its rerun at `2ab91928` (`gates-synthetic-2ab91928.out`), and
  hosted CI is green on all three checks.

## Handoff (closeout)

- Commits on `task/evidence-large-files`:
  - `3d2c15b7`: the claim;
  - `db5ef228`: the reader and the race readers;
  - `990232ae`: the automatic step, dropped in `96be8587`;
  - `c0acb7f5`: the record;
  - `5264ecec`, `47bc0a69`, `2ab91928`: round 1's fixes and records;
  - then this record.
- Main `local/evidence/evidence-large-files/` keeps:
  - the patcher, its `script-backups/` and the converter;
  - `converted.sha256`;
  - the conversion and gate logs.
- The closing session removes `.worktrees/evidence-large-files` and
  `.worktrees/evidence-large-files-review` and the local branch.
- To read a converted capture without the helpers, run `zstd -d NAME.zst`.
