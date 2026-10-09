# COVERAGE-GAPS - what the original still does that native does not

## Assignment

- Status: **in progress**, claimed 9 October 2026 (07:00 UTC) on main `1ad7ba4`, in the session the
  user asked to keep working until weekly usage reaches 50% (7% at claim).
- Worker: Claude Code desktop app, Claude Opus 5.5 (`claude-opus-5-5`); coordinator, primary and
  integrator. Branch `task/coverage-gaps` in `.worktrees/coverage-gaps`.
- Milestone: M4 (original game coverage).
- Review tier: **3** (D-0008: records and laboratory evidence only; no `src/core`, tool or
  gameplay change). A queued task carries its own tier.
- Task provider: Anthropic.
- Dependencies: the static code map (R-0045), [COVERAGE-ROADMAP](COVERAGE-ROADMAP.md).

## Why now

COVERAGE-ROADMAP's queue (25 September) is done: the native app plays the title, menus, every
mode, the idle demos and audio. The user's goal is the whole game natively ("100% coverage from
the ROM"). No task is ready, so this one measures what is left and queues it.

## Outcome and boundaries

- Every distinct cold-start replay manifest under `local/evidence/` (237, 1,667,693 frames: the
  captures behind every accepted task) is captured again with instruction coverage. Each of the
  static map's 636 routines is attributed: run by the corpus and cited by native code, run but not
  cited, or never run.
- The routines the corpus never runs are traced through the static listing to the features that
  reach them (callers, the menu's two unread controller combinations at `$80:A9B4` and `$80:F0D6`,
  and so on), and the known limits recorded by accepted tasks are gathered.
- Result: a research record with the inventory and a queue of tasks, each with its domain and
  tier. Records only: no native or tooling change.
