# COVERAGE-ROADMAP - from the race engine to the whole game

## Assignment

- Status: **accepted** 25 September 2026 (tier 3, records and evidence only; [#27](https://github.com/malmazuke/unirally-reconstruction/pull/27)).
  Claimed the same day by the Claude Code desktop session that ran RESULT-TITLE-GLYPHS, on base
  `ce85a2e`.
- Milestone: M4 (original game coverage)
- Coordinator: the claiming session is coordinator, primary and integrator
- Task provider: Anthropic
- Worker/session/runtime/model: Claude Code desktop app, Claude Opus 5.5 (`claude-opus-5-5`)
- Actual model/reasoning effort, routing rationale: the claiming session's model at default
  effort; **tier 3** (D-0008: records and evidence; no code, no independent review).
- Provider quota window (D-0004): at claim the 5-hour window was 33% used and the weekly window 25%. The user's allowance: continue until the
  weekly window reaches 50%.
- Dependencies: the static code map (R-0045) and its four raw coverage captures.
- Branch and isolated worktree: `task/coverage-roadmap` in `.worktrees/coverage-roadmap`.
- Owned paths: this record, the tasks it queues, `docs/STATE.md`, `tasks/README.md`,
  `tasks/NEXT_SESSION.md`.

## Why now

On 25 September 2026 the user stated the goal: "100% coverage from the ROM - basically the full
native c++ implementation, menus and all", and asked for tasks to follow one another. The
native game is a race engine: the app starts in a race. This record inventories what the
original does that native does not, from the code and from new captures, and orders the work.

## What the ROM holds

- All CPU code the captures execute lies in banks `$80-$83` (131,072 bytes), plus small WRAM
  trampolines in bank `$00`. The other 60 banks of the 2 MiB ROM are data: graphics, tracks,
  text, and the sound program and its music (not yet mapped).
- The static code map (R-0045) finds 636 routines in `$80-$83`.

## The game's top level

- Reset (`$80:8858`) initialises and enters the main loop at `$80:8881`. Each pass calls the
  main menu `$80:ABC8`, then runs the game mode `$9B` through the table `$80:88B2`.
- The main menu starts with `$9B = 0` and an idle counter `$89 = 480` frames. Up and Down move
  `$9B` through 0-4 (wrapping); when the counter runs out it sets `$9B = 5`. Two controller
  combinations (`$72`/`$74` equal to `$02B0` or `$8430`) branch to `$80:A9B4` and `$80:F0D6`
  (not yet read; possibly cheats).
- The modes, confirmed by capture (below):

| `$9B` | Handler | Menu entry | First screen |
| --- | --- | --- | --- |
| 0 | `$80:BB9C` | 1P | PICK A PLAYER (16 riders), then tour, track, race |
| 1 | `$80:BCBF` | 2P | PICK A PLAYER |
| 2 | `$80:BF49` | VS | PICK PLAYER ONE |
| 3 | `$80:BDD4` | LEAGUE | six players, ONE to SIX: DEFINE ME |
| 4 | `$80:B626` | OPTIONS | RECORDS, DEFINE PLAYER, RENAME PLAYER, DEFINE LEAGUE, MAIN MENU |
| 5 | `$80:93FB` | (idle 480 frames) | the demo: a split-screen race of two computer riders (AMY and ALICE) |

- With no input, the title passes to the main menu by itself (frame 419 of a cold start) and
  the demo starts at frame 900.

## Coverage today

Routines by where the captures first execute them (the four raw captures behind the static map,
all on the 1P path, plus this task's six):

| Group | Routines | Bytes | Cited by native code |
| --- | ---: | ---: | ---: |
| Boot and title (before frame 300) | 31 | 2,245 | 1 |
| Menus on the 1P path (frames 300-1327) | 167 | 14,691 | 12 |
| Race and result (frame 1328 on) | 260 | 33,381 | 120 |
| Only in the demo (this task) | 40 | 6,018 | 0 |
| Only in LEAGUE or OPTIONS (this task) | 2 | 362 | - |
| Never executed by these captures | 136 | 17,788 | 35 |

Native cites 219 of the 636 routines, almost all in the race. The 2P and VS captures reached only
their first screens in 3,000 frames, so they add no routines yet; their races will.

What native has, by system:

| System | Native | Missing |
| --- | --- | --- |
| Race engine (36 tracks, one human against the computer) | Exact on every track's sweeps | Stunt events (4 tracks); the second human |
| Race picture and HUD | Recovered | - |
| Result screen | Title, times, backgrounds | The `1P` arrow, unicycles and award icons (R-0038) |
| Title, main menu | None: the app starts in a race | All |
| 1P setup (player, tour, track, NOW PLAYING) | None | All |
| 1P continuation (next track, tour standings, tour end, unlocks) | None | All |
| 2P, VS, LEAGUE, OPTIONS | None | All |
| Split-screen race (2P, VS, the demo) | None | All |
| The demo | None | All |
| Persistence (SRAM `$77:xxxx`: players, records, league, unlocks, learned AI weights) | The learned AI word only, in race state | All other fields; native save files |
| Audio (the sound program, music, effects) | None | All |

## Decisions

- **Order.** Make the native app a complete one-player game first, then the modes that reuse
  it, then the rest. The one-player path is the part the race engine already serves, and each
  screen on it is shared by later modes (the rider list, the text printer, the result screen).
- **One task per screen group**, each tier 2 unless it changes race state (then tier 1).
- **Audio needs a decision record first.** The product may not execute original code (user,
  D-0008), and the sound program is original code for the SNES's sound CPU. The likely route is
  a native reimplementation of the sound driver, feeding a model of the sound hardware. That
  choice, and how audio is compared with the original, go in a decision before any audio task.
- **Evidence method for screens.** Each screen task captures the original's frames and WRAM
  along a menu path (the capture tools already drive menus: `track_reference.menu_events`) and
  compares native's picture and state with them, as the race tasks do.

## The queue

In order; each becomes ready when the one before it is integrated, unless noted.

1. [FRONT-END-MAIN-MENU](FRONT-END-MAIN-MENU.md): the app boots to the title and main menu. The
   menu's cursor, its idle count into the demo (until the demo exists, native shows a
   placeholder), and 1P entering the existing race.
2. FRONT-END-1P-SETUP: PICK A PLAYER, PICK TOUR, PICK TRACK, NOW PLAYING, driving the chosen
   rider, tour and track into the race start.
3. FRONT-END-1P-CONTINUATION: after the result, the next track, tour standings, the tour's end,
   unlocks and saving them.
4. RESULT-ICONS: the result screen's `1P` arrow, unicycles and awards.
5. SPLIT-SCREEN-RACE: two riders on a split screen, both computer (the demo) or human.
6. ATTRACT-DEMO: the demo race and its return to the title.
7. TWO-PLAYER and VS modes.
8. OPTIONS and LEAGUE.
9. STUNT-EVENTS: the four stunt tracks' rules and scoring.
10. AUDIO-DECISION, then audio tasks.

ROLLING-CONTACT (ready) can run at any point; it is race-engine polish.

## Evidence

`local/evidence/coverage-roadmap/`:

- `mode-1.json` ... `mode-4.json`, `idle.json`, `title-idle.json`: replay manifests (cold start,
  Start on the title, k Downs on the main menu, Start) and `captures.sh`.
- Their coverage captures (`coverage capture`, 3,000 frames each, frame images at 450 to 2,999).
- `attribute.py`: the routine attributions above. Native cites 35 routines that none of these
  ten captures runs. They are race-engine routines, recovered from other race captures or the
  static listing; this task did not check which.

## Handoff

- Exact next experiment/command: FRONT-END-MAIN-MENU.
