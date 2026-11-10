# SOUND-PROCESSOR-TIMING - the front end's waits that follow the sound processor

## Assignment

- Status: **claimed** 10 November 2026 10:58 UTC on main `e036458e`, on the user's instruction
  ("Next task", after OPEN-BRANCHES #101). Queued 8 November 2026 by
  [COVERAGE-CENSUS](COVERAGE-CENSUS.md) (R-0109, queue item 5). Claude Code desktop app, Claude
  Opus 5.5 (`claude-opus-5-5`): coordinator and integrator; a worker subagent captures and
  implements, a fresh subagent reviews. Run under
  [D-0013](../docs/decisions/D-0013-task-time-and-disk-budget.md): claim to merge against the
  6-hour target, new evidence under 1 GB. Usage at the claim: 5-hour 1%, weekly 32%.
- Branch `task/sound-processor-timing` in `.worktrees/sound-processor-timing`; evidence in main
  `local/evidence/sound-processor-timing/`.
- Task provider: Anthropic.
- Milestone: M4. Review tier: **1** (frame timing of the front end and races).

The original's CPU waits on the sound processor at uploads and handshakes, so some frames depend on
the driver's state; native uses measured lengths. The records list the cases: the warm power-on's
handshakes, short titles and the two long title starts, held titles' slides, the longer way back
after a demo pad exit, the podium's way back a frame late in about a third of podiums and its music a
pass off in 2 of 8, a race's loading varying by a frame (R-0077, R-0087, R-0088, R-0090, R-0096,
R-0098, R-0099, R-0102). Native now runs its own driver (D-0009) whose port writes equal the
original's. Outcome: the front end takes these waits from the native driver's state instead of
measured lengths, and every listed case equals the original frame for frame, or is recorded with the
reason it cannot.

## Results

Worker: Claude Opus 5.5 (`claude-opus-5-5`) subagent, 10 November 2026 10:59 to 12:45 UTC (clock).
[R-0114](../docs/research/R-0114-sound-processor-timing.md) has the measurements, the table and the
limits.

What the CPU waits on: at a session's FF request (`$82:807E`, `$82:8097`) the running driver lowers its
master volume by 8 on each timer-2 pulse (about 44,200 master clocks) before returning to the IPL (SPC
`$0487-04B9`). Native's driver reproduces that reply to within 102 clocks while its inputs are exact
(the boot, the first demo title, the first race from the menus: 8 of 8). After a race its reply is
-94,980 to +72,880 clocks off, because the race's fade commands reach it at D-0010 anchors and every
later session inherits the error on its timer-2 grid. Taking the waits from it would be worse than the
measured lengths, so the front end keeps them; two of the listed cases were not sound waits at all.

| Case | Verdict |
| --- | --- |
| Warm power-on's handshakes (R-0090) | closed: OPTIONS' entries are already equal on main (every picture, three images); the first idle demo's "blanking 6 frames later" was the menu's fade-out, **native changed**. The rest of the 21 warm runs differs only in the runner's missing picture at a race handover, league-two-events frame 6,098 and the all-gold race (not sound) |
| Two long title starts (R-0088, cycles 10 and 17) | closed, **native changed**: the menu's fade-out `$80:9881-98A3` (brightness 13 to 3 over six pictures, then blank), missing at every title start; 1,717 of 1,717 pictures over five title starts (main 1,687), lap pictures 993 of 1,006 (main 991) |
| Short titles (R-0087), held titles' slides (R-0096) | cannot under D-0010: the session end straddles the frame boundary within 45,000 clocks; native's driver gives the original's frame in 18 of 43 later titles (20 with the demo session's interrupts modelled) against the hold rule's 25 |
| Longer way back after a demo pad exit (`rv-late-split`) | cannot: native's driver also misses it (71 frames against 70); it gets the other 44 of 45 way backs, as the front end's rules do |
| Podium's way back a frame late (about a third) | cannot: native's driver 5 of 8 podiums, the front end's always-on-time rule also 5 of 8 |
| Podium's music a pass off in 2 of 8 | unchanged: already the native driver's own state |
| A race's loading by a frame | cannot: native's driver 38 of 42 races, the measured table 40 of 42 |

Also found: the cued audio model leaves out the title interrupt during the idle demo's session
(78,108-92,202 clocks short; R-0096's commands 82,000-97,000 clocks early). It changes no frame;
queued in the follow-up.

## Decisions (worker)

- No front-end wait is taken from the driver: on 190 sessions of 21 captures the native driver's
  frames are worse than or equal to the measured lengths for every kind (R-0114's table), so switching
  would add differences. The measurements stay as a check (`sweep`) so a later anchor or driver change
  can be judged the same way.
- The demo title's start was traced instead of assumed to be the sound processor's; it is the menu's
  fade, which native lacked at every title start. The fade shows the palette cycle's colours, so the
  cycle runs on through it; native's count restarts with the title's loads and the riders are paired at
  the setup frame, which keeps the phase 3 the original copies there. A continuous cycle through the
  title was tried first and moved the all-gold warm boot's first demo pairing (the original's
  interrupt misses steps native does not model); the restart is recorded as a native stand-in.
- The demo session's missing interrupt work in the audio model was measured, not fixed: it changes no
  frame, and the phase it needs comes from the front end, a change to the cue format the gates read.
- Verdicts use each session's frames from its own request, so a run whose cues are a frame apart still
  compares; the measured length per kind is the commonest in the sample (a race's per track and song).

## Evidence

Main `local/evidence/sound-processor-timing/` (13 MB): `sessions.py`, `session_ends.sh`, `sweep.sh`,
`sweep.list`, `summary.py`, `carry.py`, `race_loading.py` (the session measurements), `main-sweep/`
(per capture: native's cue log, the sessions as text and JSON, `summary.json`, `carry.txt`),
`trace_session.py`, `warm_waits.py`, `window_compare.py` (traces and picture windows), `check.sh`,
`checks.list`, `checks.out`, `expect-sweep.json`, `expect-warm.json`, `reverted.out` (title-starts and
demo-cycles fail with main's runner), `main-runner/` (main's `front_end_runner`, for the revert),
`warm-sweep-main.json`/`.out` (main's warm runs), `gcc.out`, `regen.sh`.

- `checks.list`: five lines, all pass on the candidate (`checks.out`): `title-starts` 184 s (1,717 of
  1,717 pictures), `demo-cycles` 25 s (34 of 34 cycles, 993 of 1,006 pictures), `warm` 450 s (21 runs,
  the differing pictures exactly `expect-warm.json`), `chain` 60 s (8 sessions within 102 clocks),
  `sweep` 249 s (190 sessions, R-0114's table and carried replies exactly `expect-sweep.json`). Each
  fails on fewer comparisons than recorded. `warm` needs a short temp folder (`/private/tmp/sw`): its
  picture paths otherwise pass ARG_MAX.
- `reverted.out`: with main's runner `title-starts` fails (1,687 of 1,717 pictures: six at each title
  start) and `demo-cycles` fails (991 of 1,006, the two title starts at 57,057 pixels). `chain`, `sweep`
  and `warm` measure what the change does not touch and pass on main by construction.
- Also run: OPEN-BRANCHES' `demo-wrap` check passes; the review's `lap2-150000` retained pictures 563
  of 573 on candidate and main alike.
- Builds `lab-release`, `lab-debug`, `app-debug` passed; `ctest` 41/41 on all three. Linux GCC 13.3
  (`ubuntu:24.04`, CI's flags): `front_end.cpp` compiles plain and with `-fsanitize=address,undefined`.
  clang-format: no new finding (line 810's is main's). ASan not run locally (macOS 27).
- `regen.sh` (eight inputs, unchanged) and `coverage native-symbols --check`: passed; the tooling tests
  pass. No new `front_end_runner` option, so no gate corpus change.

## Handoff

PENDING
