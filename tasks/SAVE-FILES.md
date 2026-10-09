# SAVE-FILES - the cartridge RAM saved between app runs

## Assignment

- Status: **in progress**, claimed 9 October 2026 (08:15 UTC) on main `3191228`, in the session the
  user asked to keep working until weekly usage reaches 50% (8% at claim). Queued the same day by
  [COVERAGE-GAPS](COVERAGE-GAPS.md) (R-0089, queue item 1).
- Worker: Claude Code desktop app, Claude Opus 5.5 (`claude-opus-5-5`); coordinator, primary and
  integrator. Branch `task/save-files` in `.worktrees/save-files`.
- Task provider: Anthropic.
- Milestone: M4 (original game coverage).
- Review tier: **1** (D-0008: it changes the state the game boots with and the bytes it keeps).
- Dependencies: OPTIONS (R-0072), LEAGUE (R-0073), FRONT-END-1P-CONTINUATION (R-0057).

## Why

The app never writes cartridge RAM. Records, player names, leagues, medals and unlocked tours are
lost when it quits, so the native game cannot be played as the original is: over many sessions.
Native keeps the cartridge RAM as fields (`OnePlayerRecords` in `src/core/front_end.hpp`, with
their `$77:xxxx` addresses), but not as the original's 8 KB image: the record checksum
`$77:054E`, the statistics checksum `$77:02B0` and the score history `$77:0618` are not all kept,
and several bytes have only provisional meanings (R-0072, R-0067).

## Outcome

- One native image of the cartridge RAM (`$77:0000-1FFF`), equal to the original's at every point
  the original writes it, in the tested domain: built from native state and read back into it.
- The app loads it at boot and writes it to disk when the original writes cartridge RAM; with no
  file it starts as a cold start does now. A file the original wrote (an emulator's `.srm` of
  this ROM) loads too.
- The original's boot with populated cartridge RAM (its checksum test and what it does with a bad
  one) captured and native, since every capture so far starts from an empty or preloaded image.

## Evidence to gather

- Captures of the boot with saved cartridge RAM (a state origin, or `cartridge_ram_writes`
  before the boot reads it), with good and bad checksums.
- Whole-image comparisons at the save points of the existing corpus (OPTIONS, LEAGUE, the
  one-player continuation, HUNTER's ending), against `memory.sram` or the capture's SRAM rows.
- Round trips: every accepted front-end manifest's final state saved, reloaded, and played on to
  the same rows.

## Not in scope

The WIPE RAM menu (queue item 3, which builds on this image) and any change to what the game
stores.
