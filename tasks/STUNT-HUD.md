# STUNT-HUD - a stunt event's race picture

## Assignment

- Status: **reviewed and integrated** by [PR #42](https://github.com/malmazuke/unirally-reconstruction/pull/42), merge `490302a`; status reconciled 30 September 2026 without expanding R-0068's domain. Queued 27 September 2026 (UTC) by STUNT-EVENT-RACE; claimed 27 September
  2026 at 09:30Z by the Claude Code desktop session that ran STUNT-RESULT, on `0173cbb`. The
  implementation worker started on STUNT-RESULT's branch while it was in its gates; its commits
  were moved onto this claim.
- Coordinator: the claiming session is coordinator, primary and integrator
- Worker/session/runtime/model: Claude Code desktop app, Claude Opus 5.5 (`claude-opus-5-5`)
- Provider quota (D-0004): at claim the weekly window was about 76% used; the user asked to stop
  after this task.
- Reviewer: a fresh Anthropic subagent, isolated checkout.
- Branch and isolated worktree: `task/stunt-hud` in `.worktrees/stunt-hud`.
- Milestone: M4 (original game coverage)
- Tier: 1: presentation, plus a race-state finding on NEON (track 42) made during the work.
- Dependencies: STUNT-EVENT-RACE (R-0066), the race HUD (CLASSIC-RACE-HUD), R-0063.

## Outcome and boundaries

The race picture of a stunt event (the decode's section 1.6): `stunt` in place of `race`, the clock
counting down, the score and qualifying field and its NMI slot, no opponent sprite or arrow, the
countdown digits, `$15A3`, track 37's BG1 fetch with `$0FF7` and track 42's scenery case.

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| Pictures | Dense frame windows of bowl-lose and hill-win (the start, a few rewards, the clock's end, the finish captions), captured with frame images | 0 differing pixels | logs |
| Nothing moves | The gates of STUNT-EVENT-RACE | Unchanged | logs |

## Result

[R-0068](../docs/research/R-0068-stunt-hud.md). A stunt event's race picture is native: `stunt` in
the left field, the clock counting down and stopping at 0:00.0, the score and qualifying field
bottom right in the BG3 upload order, the countdown without its waits, no arrow or opponent
object, the finish banner from the finish display, and the rider look's opponent head point held
at zero. HUNTER's stunt event (track 42) is NEON: pack profile v26 adds its scenery and green
levels, and its picture (the lighting under the player's contact) is native. Race state: on NEON
the original runs the lighting in place of the HUNTER tag effects, and every empty palette-7 probe
reads the tile-column table's byte 1 + `$12D1`; native now does both (a tile-angle scan of all 45
column tables shows the probe rule changes nothing off NEON). An implementation worker wrote it
and made the corrections; the primary integrated.

## Evidence

`local/evidence/stunt-hud/` (NOTES.md; captures neon-start, neon-ride, neon-ppu; checks-f31aae8,
checks-b69a83d, front-end-b69a83d).

| Criterion | Result |
| --- | --- |
| Pictures | bowl-lose 2,708 and hill-win 2,779 pictures (every frame of the race), 0 differing pixels; the nine idle stunt tracks, 13,940 frames equal by digest; neon-start 43 pictures and neon-ride 2,702 pictures equal. |
| NEON race state | neon-ride (hill-win's inputs on track 42): 2,808 of 2,808 race rows exact through the result load. |
| Nothing moves | The gates on `9413b30` (`local/evidence/stunt-hud/gates-9413b30.out`, 09:31-11:53Z, 142 minutes): the three presets, ctest 29 of 29, the synthetic suite, both v1 contracts, every hidden app run and the eleven differential gates pass. The equivalence sweep against main `0173cbb`'s binaries: 432 runs, 81 differing, all by design: 73 are pictures of the nine stunt tracks (main draws the race HUD there; this branch the stunt HUD, which equals the original's) and 8 are track 42's rows (NEON, where main still runs the HUNTER tag effects). No race track differs. The per-track recompare's race tracks are identical; the 16 stunt captures exact; R-0061's race captures and HUNTER-EFFECTS' 48 held captures unchanged; every front-end comparison and records check unchanged. The tooling tests (506); no function over 80 lines; the address index passes. After the gates, `neon_lighting.cpp`'s one cast for GCC's `-Wsign-conversion` (Linux CI), no change in behaviour: the presets and ctest 29 of 29. |

## Review

Tier 1, a fresh Opus 5.5 subagent in an isolated worktree, of `72f41e4` (before the move onto
main): approved with should-fixes, no must-fix (`review-72f41e4.md`). It found the contact rule
exactly the original's and gated where the original gates it, and reproduced neon-ride, the
pictures, R-0061's race captures, the per-track recompare and the M4-16 primary gate. Its
should-fixes, made in the corrections commit: the probe reads byte 1 + `$12D1` on every track
without a NEON special case; R-0068's account of X, the tile-angle scan and the 8-bit signed
test; stale comments on HUNTER's stunt event; `$0202` described as shared scratch.

## Handoff

- Exact next experiment/command: the main menu's other modes (COVERAGE-ROADMAP: 2P, VS, LEAGUE,
  OPTIONS, the demo).
