# EVIDENCE-LARGE-FILES - the captures too large for transparent compression

## Assignment

- Status: **claimed** 26 October 2026 on main `e3a41066`, on the user's instruction (Claude Code
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
  `events.bin` files and one native cond log, which no current check reads.
  - Each file is compressed, decompressed again and compared by SHA-256 before the raw file is
    deleted.
  - The digests are in `converted.sha256`, the log in `convert.out`.
- **Future captures.** `tools/compact_evidence.py` now does the same at every closeout: a capture
  kind over `--max-compress` becomes `NAME.zst`, checked the same way, and identical `.zst` files
  are cloned.
