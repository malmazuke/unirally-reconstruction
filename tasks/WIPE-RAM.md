# WIPE-RAM - the main menu's hidden WIPE RAM menu and factory reset

## Assignment

- Status: **in progress**, claimed 10 October 2026 on main `33f3645`, in the session the user asked
  to keep working until weekly usage reaches 50% (13% at claim). Queued by
  [COVERAGE-GAPS](COVERAGE-GAPS.md) (R-0089, queue item 3).
- Worker: Claude Code desktop app, Claude Opus 5.5 (`claude-opus-5-5`); coordinator, primary and
  integrator. Branch `task/wipe-ram` in `.worktrees/wipe-ram`.
- Milestone: M4 (original game coverage).
- Review tier: **1** (it rewrites the cartridge RAM and changes front-end state).
- Task provider: Anthropic.
- Dependencies: FRONT-END-MAIN-MENU (R-0054), SAVE-FILES (R-0090), MODE-AUDIO (R-0091).

## Why

On the main menu, Left+A+L+R held on either pad (`$02B0`, `$80:ABEB-ABF9`) opens a two-entry
menu, WIPE RAM and MAIN MENU (`$80:A9B4`, text at `$80:A9FA`). Native recognises the code and
stops: the app shows a notice. The original's menu, its warning ("this option will reset your
game pak's memory to its factory default ... you will lose all your records") and the reset
itself are not native.

## Outcome

- The menu, the warning, both answers and the reset native and frame-exact against the original
  from a cold start and from saved images (SAVE-FILES' warm boots), with the menus' sounds.
- After the reset the cartridge image equals the original's, and the app's save file holds it.
