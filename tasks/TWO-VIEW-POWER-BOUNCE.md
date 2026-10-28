# TWO-VIEW-POWER-BOUNCE - power bounce for both riders in a two-view race

## Assignment

- Status: **claimed** 28 October 2026 09:28 UTC on main `5f161c9c`, on the user's instruction
  (Claude Code desktop app, Claude Opus 5.5 `claude-opus-5-5`; coordinator and integrator; a worker
  subagent implements, a fresh subagent reviews). Queued 28 October 2026 by
  [SPLIT-HUNTER-PICTURES](SPLIT-HUNTER-PICTURES.md) (R-0103, review round 1). Run under
  [D-0013](../docs/decisions/D-0013-task-time-and-disk-budget.md): claim to merge against the
  6-hour target, new evidence under 1 GB.
- Branch `task/two-view-power-bounce` in `.worktrees/two-view-power-bounce`; evidence in main
  `local/evidence/two-view-power-bounce/`.
- Milestone: M4 (original game coverage).
- Review tier: **1** (race state).
- Dependencies: HUNTER-EFFECTS (R-0052), CREDITS-NAME (R-0094), SPLIT-HUNTER-PICTURES (R-0103).

## Why

In a two-view race (2P, VS, the split demo, a two-view league) under HUNTER's power bounce
(effect 2, `$132B`), native's race state diverges from the original's in rider 1's velocity:
SPLIT-HUNTER-PICTURES' `twop-104` from frame 5548 (update 3521) and `vs-124` from frame 4981
(update 2954). Native passes the landing matrix 0 to rider 0 only (`race_update.cpp:807`,
`index == 0`). The listing at `$81:94A8-94B2` skips the test that excludes rider 1 when the race
has two views (option bit 3), so both riders land with matrix `$0572`; native's own comment at
`vertical_contact.cpp:338` says the same. The original's RAM at both divergence frames shows a
two-view race with power bounce on.

## Outcome

Rider 1 lands with power bounce's matrix in a two-view race as the original does: race rows
equal past the divergence on `twop-104` and `vs-124` (captures and manifests in main
`local/evidence/split-hunter-pictures/`), with one-view races and the split demo unchanged.
Native's pictures of those races after the divergence then compare too.
