# AUDIO-FIRST-RACE - native audio through an ordinary played race

## Assignment

- Status: claimed 1 October 2026 21:02 UTC (2 October Sydney) after verifying
  PR #50's merge, main `be3a6e0` equal to `origin/main` and main's
  `artifacts/audio-title-menu-integration/closeout.json`.
- Primary/coordinator: Claude Code, Claude Opus 5.5 (`claude-opus-5-5`). This
  task starts on Anthropic, so under D-0004 its children and its independent
  reviewer are fresh Claude subagents, not Sol.
- Quota at claim: Claude weekly all-models 0%, five-hour 0% (21:02 UTC; the
  30% in the predecessor's closeout is the separate Codex account). D-0004
  checkpoint at a 20-point increase; the user's standing rule allows
  continuing to 80% weekly, with the last 20% reserved for review/recovery.
- Branch/worktree: `task/audio-first-race`, `.worktrees/audio-first-race`.
- Milestone: M4 (original game coverage); this is not the complete M4 gate.
- Tier: 1. New CPU sound producers, integer work/ordering, sequencer state,
  content extraction and scene continuation require full D-0006 review.
- Provider/model/budget: keep the claiming task and its children within its
  starting provider under D-0004. Record fresh usage, actual model settings,
  the 20-point discretionary boundary and final 20% review/recovery reserve.
  No reset, spending or provider move is authorized by this preparation.
- Reviewer: automatically dispatch fresh explicit Sol/medium in an isolated
  checkout, freeze a coherent source candidate, obtain independently chosen
  cases and handle findings/re-review without asking the user to launch it.
- Dependencies: merged AUDIO-TITLE-MENU, D-0009, R-0075, accepted ordinary 1P
  setup/race/result/continuation and the pinned PAL ROM/reference identities.
- Owned scope: native audio producers and work clock, affected sequencer/data
  extraction, pack rules, SDL scene ownership, focused laboratory tooling,
  tests and task/research/state/validation records. Preserve gameplay gates.
- Evidence/log homes: main `local/evidence/audio-first-race/` and
  `artifacts/audio-first-race-integration/`; no evidence in a worktree.

## Outcome and rationale

From native power-on, play continuous native audio through ordinary 1P setup,
the first CRAWLER DRAGSTER race, its result and a return to the menus. Recover
the music and effect commands this selected played path actually produces,
including overlap, retrigger and stop/restart. Choose a result/replay variation
and a pause/quit return to exercise scene changes rather than accepting one
headless race cue. Capture and implementation remain coupled inside this task.

This is the next coherent outcome because AUDIO-TITLE-MENU stops its bounded
producer at ordinary menu exits. The 2 October physical run confirms audible
music/navigation but reports an aggregate shortfall outside its narrower
continuous-device observations. Ordinary setup/race/result ownership closes
that gap before soundtrack breadth. It does not imply that the shortfall's
exact cause is known, or that all tracks use identical sound producers.

Use recovered native C++ command/sequencer logic, identified local data and
the isolated DSP model. No original CPU/SPC execution, uploaded executable,
captured-event playback or prerecorded soundtrack in the product. Preserve
integer arithmetic, event ordering and timing. Unknown work is research,
not an invented frame budget. Keep optional host latency distinct from original
clocks. Other tracks/modes, full soundtrack breadth and live device save remain
later outcomes unless evidence makes them necessary coupled prerequisites.

## First experiment

1. Verify PR #50's merge/closeout, clean synchronized main, R-0075's identities
   and frozen source checks. Claim an isolated task checkout and record quota.
2. Read the generated static listing before designing captures. Start at the
   ordinary menu selection, 1P setup and race/result sound-call sites, using
   the static/native maps to identify their callers. Cite those readings.
3. Reproduce the accepted title/menu audio with the pinned observation core.
   Define a cold ordinary 1P DRAGSTER schedule and freeze original raw PCM,
   ordered events, CPU/APU clocks, state and A/V before native comparison.
4. Prove instrumentation on/off integrity. Compare a second original run and
   a changed confirm timing before deciding which setup/race work is required.
   Preserve earlier reference locks, raw expectations and gameplay contracts.
5. Recover the first ordinary exit producer, then continue through setup and
   the played race/result. Do not accept an exit-only fragment as this outcome.

## Acceptance and closeout

- Exact declared command/DSP events and raw PCM on frozen primary and changed
  timing schedules, including the recovered scene boundary commands.
- Identified score/instrument/sample extraction with no executable dependency;
  retain source/ROM/core/content identities and exact tested domain.
- Fresh-process native and partial-drain playback continuation at setup,
  race/result and scene boundaries, with complete owned DSP/queue state.
- Visible physical control and human listening across the played path.
  Retain host delivery/shortfall counters and distinguish intentional silence,
  ended producers and runtime underruns using observed evidence.
- Required affected suites, original frozen race/presentation gates, hosted
  macOS/Ubuntu checks including actual Linux sanitizer results. Missing/skipped
  or timed-out checks remain non-passes.
- Fresh isolated tier-1 independent review and withheld cases. Push the branch,
  use the PR template, read every PR comment/line comment, resolve findings,
  merge with a merge commit, synchronize main and clean task-owned outputs.
- Prepare the tracked handoff before merging; ignored closeout holds actual
  final commit, commands/results, review, CI, merge, main equality and cleanup.
  Keep every cited private capture. No release, deployment or M4 tag here.

## Claim experiment, 1 October 2026 21:00-22:15 UTC

Verified (private evidence in main `local/evidence/audio-first-race/`):

- Primary schedule `primary.script.json`: FRONT-END-1P-CONTINUATION's
  `cont-win` cold path truncated to 4,000 frames (1P defaults, DRAGSTER won
  with Right 1500-3299 and Up 2200-2259, result left at 3800, PICK TRACK).
  Captured twice with the v5 observation core (`primary-a`, `primary-ctl`):
  2,562,673 raw stereo pairs each with identical PCM SHA-256
  `c926b53b...` and identical event-kind counts. `primary-ctl` adds SMP
  watches at score dispatch `$0951`/`$0961` only.
- Three IPL sessions: boot (frames 29-38), race load (1249-1260) and the
  post-race reload (3459-3468). Each sends the same 4,445-byte driver (ROM
  `0x9C6B6`, resource 50). The race session sends resource 54 (ROM `0x9ED13`,
  1,589 bytes to `$1600`) and resource 62 (ROM `0xA1F65`, 2,463 bytes to
  `$1D00`) and the sample set selected by the 64-byte table at `$83:FC75`.
  Static `$83:CA08-CBC8` (bank-83 listing) selects one of six race songs
  (resources 64, 62, 63, 64, 65, 66) from cartridge byte `$77:10B1`, which a
  cold cartridge leaves at 1 for the first race.
- Command sequence (`commands.py`): menu effects 2/8 through setup, command 3
  (parameter `0x90`) at frame 1207 fades the menu music before the race load;
  in the race, effect 15 three times (countdown), commands 11/6 with
  parameters 20-43 paired with effects 12-17, and command 3 (`0xA0`) on 61
  consecutive frames from 3393 at the finish.
- The conditional native driver (original CPU port writes at their SMP ticks,
  native IPL/driver/DSP from zero, `conditional.py`) matches the first 773,159
  raw pairs and stops at command 3, outside its recovered table.
- Race dispatch sites: `$83:CD6E` runs 7,250-42,168 and `$83:CD9F`
  167,452-239,482 master clocks after the previous frame boundary; the driver
  polls port 2 about every 178 SMP ticks (~1,850 master clocks). Exact command
  arrival in a race frame therefore depends on the whole race body's CPU
  work. This is the central timing question for the task; see the next section.

Driver readings (private `spc_disasm.py`, a reading aid over the uploaded bytes;
nothing from it enters the product): command 3 (`$063B`) stores
`sign-extend(parameter) * 8` as a music master-volume rate that the music
pass (`$068F-$06B5`) adds to `$DF/$E0` with clamps; commands 6/11
(`$0656`/`$0663`) clear/set bit `p & 7` of byte `p >> 3` in an eight-byte flag
table; score controls `0xA6-0xA8` clear/test those flags, `0x86` sets the fixed
duration, and `0x8C` starts a table-driven gain envelope stepped by `$0B42`
(`$02D0` enable; `$0B8A` skips the software envelope while it is set).
