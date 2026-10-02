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

- Rewritten under [D-0010](../docs/decisions/D-0010-frame-anchored-sound-commands.md)
  (1 October 2026), which replaced "exact raw PCM" outside the cycle-modelled
  title/menu scenes:
  - Native producer commands equal the original's by content, order and
    frame on the frozen primary and changed schedules, including the
    recovered scene boundary commands (loads, fades, pause/quit).
  - Driver, score, samples and DSP are exact for supplied arrivals: all
    DSP/port events and raw PCM, conditionally on the original's port writes.
  - Title/menu raw PCM stays exact up to the first anchored command. After it,
    reports state the measured arrival error and PCM agreement, never "exact".
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

## 1 October 2026 23:50 UTC checkpoint

- `2a6ebd3`: the native driver plays the race sound set. Conditionally on the
  original CPU's port writes, all 703,154 DSP/port events and 2,562,673 raw
  pairs of `primary-a` match (R-0076 observation 6; `cond-race-2.json`). It
  also corrects the software envelope's zero-length decay/release division
  (observation 5); AUDIO-TITLE-MENU's six frozen comparisons are unchanged
  (`regression-1.json`). State format URAU0005.
- Perturbation (`perturb.py`): shifting race command arrivals by up to 50 SMP
  ticks changes about 160,000 post-race pairs; exact audio does not reconverge
  after the race. Coverage captures count 12,055 instruction sites through the
  race load and about 4,700 more in the race. [D-0010](../docs/decisions/D-0010-frame-anchored-sound-commands.md)
  therefore adopts frame-anchored dispatch clocks outside the cycle-modelled
  title/menu scenes; it changes this task's PCM criterion from exact to
  measured (see Acceptance below once rewritten).
- `c42c537`: pack profile v32 with the race sound set (15 entries; the 509 v31
  entries pinned by digest).
- Anchored transport (`audio_cued_scenes.cpp`, `race_audio_runner`): the exact
  title/menu model runs to the 1P menu exit (frame 620), then per-frame cues.
  With cues derived from the original's enqueue and dispatch-site watches
  (`derive_cues.py`, captures `primary-disp1/2`), all 120 commands reach the
  driver in the original's frames and order; arrival clocks differ by -43,072
  to +26,596 master clocks; the race upload's first command is exact.
- Next: native producers in the front end and race engine emitting the same
  per-frame cues; then the app, saves, variations and gates.

## 2 October 2026 UTC: candidate, review 1 and its fixes

- Candidate `b5bd54f` (PR #51): app integration, pause cues, playback save/restore mode,
  URAU0005 restore validation. Frozen gates started 22:48 UTC; build/ctest 41/41 on three
  presets, synthetic suite, both v1 contracts, nine hidden app runs and the first two
  differential gates passed before the run was stopped for review 1's changes. The DRAGSTER
  fuzz gate reported 40/40 aborts ("inconsistent initial ZOOM ZOO announcements"); the
  reviewer reproduced the same aborts on main `be3a6e0`, so it predates this task. It is a
  non-pass, offered to the user as its own follow-up task.
- Review 1 (fresh Claude Opus 5.5 subagent, isolated checkout, PR comment): changes required,
  2 blocking and 8 advisory findings. The driver arithmetic and cycle costs, the producers it
  read and 16 withheld save points held. Dispositions, all in `db28f77`:
  1. Blocking, fixed: the rider menu, PICK TRACK and PICK TOUR navigation sounds.
  2. Blocking, fixed: BRONSEN's voices 200-215 (reward-path test, 256-byte voice table).
  3. Fixed: the -256 checkpoint speed clears flag 20.
  4. Fixed: R-0076 now lists what is static-only; captures removed most of it (below).
  5. Fixed within a bound: restore rejects a clock too far past the cued frame's end; a
     smaller lag is not detectable from the saved state (recorded in the code). The first
     bound (16 frames) was wrong; see review 2.
  6. Fixed: frame-boundary formula; the pause anchors are listed with their captures.
  7. Fixed: score flags 64 and up are rejected (unit test).
  8. Fixed: `race_audio_runner` takes unresolved `R` cues; the app's raw cue log gives
     PCM and events identical to the resolved cues, and saves at 1600, 1613 and 1614
     (opponent latch set) continue exactly.
  9. Fixed: the app plays cued audio only for a first DRAGSTER race.
  10. Recorded as a non-pass with a follow-up (above).
- New original schedules (`variety`, `continue`, `back`, each with both dispatch-watch
  captures) and the reviewer's `nav` all match the native cues line for line. They confirm
  the PICK TOUR double sound, the rider menu's top/bottom-row sounds, both PICK TRACK wraps,
  the player's rotation flag 42, skid clears, the back slide and CONTINUE. CONTINUE's
  single shared pause anchor delivered its fade-in a frame late (continue: 4 of 132
  commands); separate measured anchors for the pause's three dispatch groups fix it.
- Measured agreement after the fixes (R-0076 table): every command of every schedule with
  kept events reaches the driver in the original's frame (primary 120, quit 46, loss 120,
  variety 180, continue 132, back 26); median per-window level differences 0.00-0.48 dB.
- Candidate for the second review: `db28f77` plus record-only commits. Frozen gates run in a
  separate detached checkout (`.worktrees/afr-gates`) so records can change meanwhile.

## 2 October 2026 UTC: review 2

- Re-review of `ff4845d` (same reviewer, resumed): changes required, 1 blocking and 2
  advisory findings. Every review-1 fix was confirmed against the listings and by running it;
  the reviewer's two new captures (`rev2`: another rider, a pause during the countdown,
  Start held; `rev3`: six NOW PLAYING navigation sounds) match the native cues (6,256 of
  6,256; all 35 non-wait cues), and rev2's 128 commands all arrive in frame.
  1. Blocking, fixed: the 16-frame restore bound counted only the driver/score transfer.
     The sample upload keeps the clock busy until frame 1329 after the race load (79.8 frames
     past the load frame's end) and 68.3 frames after the title reload, so every save inside
     a load was refused. The bound is now 81 frames, from those measurements.
  2. Fixed: `continuation/run.sh` exited at the first failed restore under `set -e`, hiding
     FAIL lines; a failed save or restore is now reported, and its default saves include the
     load windows (1249-1329, 3458-3527).
  3. Fixed: the stale private measurement record (`measure-review1.txt`) is rewritten.
- With the 81-frame bound, a fresh-process continuation sweep of 19 saves on the primary
  (700; 1249, 1250, 1251, 1255, 1260, 1300, 1328, 1329 inside the race load; 1360, 2000,
  3400; 3458, 3459, 3460, 3500, 3526, 3527 inside the title reload; 3700) passes 19 of 19:
  saved plus restored PCM and events equal the uninterrupted run, with an equal final state.
