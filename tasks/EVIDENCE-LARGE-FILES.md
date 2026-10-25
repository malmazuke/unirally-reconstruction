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
