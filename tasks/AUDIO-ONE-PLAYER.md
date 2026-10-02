# AUDIO-ONE-PLAYER - native audio through the whole one-player game

## Assignment

- Status: claimed 2 October 2026 05:42 UTC (15:42 Sydney) after verifying PR #51
  merged as `df0bf14`, main equal to `origin/main` and main's
  `artifacts/audio-first-race-integration/closeout.json`
  (`accepted_integrated_and_cleaned`, next ready task AUDIO-ONE-PLAYER).
- Primary/coordinator: Claude Code, Claude Fable 5.1 (`claude-fable-5-1`). This task
  starts on Anthropic, so under D-0004 its children and its independent reviewer are
  fresh Claude subagents, not Sol.
- Quota at claim: Claude weekly all-models 16% (14% at this session's start), five-hour
  54%. D-0004 checkpoint at a 20-point increase (36%); the user's standing rule allows
  continuing to 80% weekly, with the last 20% reserved for review and recovery.
- Branch/worktree: `task/audio-one-player`, `.worktrees/audio-one-player`, from main
  `df0bf14` (pack v32, the three presets built, ctest not yet rerun here).
- Milestone: M4 (original game coverage); not the complete M4 gate.
- Tier: 1. New sound producers, song selection state, content extraction and
  scene continuation require the full D-0006 review process.
- Provider/model/budget: under D-0004 the claiming session's provider owns the
  task, its children and its independent reviewer. Record usage at claim and
  every checkpoint; no reset, spending or provider change.
- Dependencies: AUDIO-FIRST-RACE, D-0009, D-0010, R-0075, R-0076; the accepted
  native one-player front end (setup, tours, results, awards, endings, HUNTER)
  and the stunt events.
- Owned scope: native sound producers in the race engine and front end, the
  audio transport's anchors for new dispatch sites, affected sound sets and
  pack rules, SDL audio scene ownership, laboratory tooling, tests and records.
- Evidence/log homes: main `local/evidence/audio-one-player/` and
  `artifacts/audio-one-player-integration/`.

## Outcome and rationale

From native power-on, play continuous native audio through every one-player
path the native game already plays: all tours' races and their songs, lap
races, the four stunt events, results, awards, tour endings and the HUNTER
ending, with Race Again, pause/restart/quit and the return to the menus.
AUDIO-FIRST-RACE stops the producer after the first race because later races
play other songs (`$83:CA08-CBC8` selects resources 64, 62, 63, 64, 65, 66 from
the cartridge counter `$77:10B1`) and other tracks may use other sample sets.

Use the D-0010 pattern: native producers with exact command content, order and
frame; declared frame-anchored dispatch clocks; the recovered driver, score,
samples and DSP. No original CPU/SPC execution, captured events or
prerecorded audio in the product.

## First experiment

1. Verify PR #51's merge and closeout and claim an isolated checkout.
2. Read the static listing for the song and sample-set selection, the stunt
   and lap race sound sites and the result/award/ending producers; cite them.
3. Capture a cold one-player schedule through two consecutive races with
   dispatch/enqueue watches (AUDIO-FIRST-RACE's `capture.py`,
   `derive_cues.py`), then extend the producers until native cues match.
4. Prove the driver exact for each new song and sample set conditionally on
   the original's port writes before measuring anchored agreement.

## Acceptance and closeout

- Native cues equal the original's on frozen schedules covering every song,
  each sample set, a lap race, a stunt event, an award and an ending.
- Conditional exact driver/DSP output for every new sound set; measured
  arrival error and PCM agreement for the product path (D-0010).
- Fresh-process partial-drain continuation at race, result and scene loads.
- Front-end and race "nothing moves" sweeps against main's binaries, the
  frozen race gates, app checks and hosted macOS/Ubuntu CI.
- Fresh isolated tier-1 review, PR with green checks, live listening with
  retained delivery counters, merge commit, synchronized main and cleanup.
