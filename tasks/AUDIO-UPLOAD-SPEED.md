# AUDIO-UPLOAD-SPEED - a sound-set upload well above real time

## Assignment

- Status: **claimed** 6 October 2026 by the session that closed TWO-HUMAN-RESTART, on main
  `1b3a08a` after that task's merge (main equal to `origin/main`, closeout written). Prepared
  3 October 2026 by the AUDIO-ONE-PLAYER session, from its live check.
- Milestone: M4 breadth
- Coordinator: the claiming session is coordinator, primary and integrator
- Task provider (fixed for all children; record any user-initiated platform change): Anthropic
- Worker/session/runtime/model: Claude Code desktop app, Claude Opus 5.5 (`claude-opus-5-5`); one
  session as coordinator, primary and integrator
- Actual model/reasoning effort, routing rationale and frontier escalation question (if any):
  the claiming session's model at default effort; **tier 1** (`src/core/audio_engine.cpp`, the
  IPL and driver run loops).
- Provider quota window/baseline timestamp, used/remaining or unknown, reserve and session
  allowance (D-0004): at claim weekly 61%, five-hour 6%; standing rule: continue until weekly 80%.
- Reviewer (primary automatically spawns fresh model/effort, isolated checkout; no user
  trigger): a fresh Anthropic subagent.
- Dependencies and evidence of acceptance: AUDIO-ONE-PLAYER (R-0077).
- Base commit: main `1b3a08a`.
- Branch and isolated worktree: `task/audio-upload-speed` in `.worktrees/audio-upload-speed`.
- Owned paths and shared interfaces: the native audio engine, IPL handshake and driver run loops,
  native tests, this record, `docs/STATE.md`, `tasks/README.md`, `tasks/NEXT_SESSION.md`.
- Claim/lease/heartbeat/checkpoint location: this record.
- Session time limit, concurrency allocation and actual spend authorization if relevant: one
  worker plus the review subagent; no monetary spend.

## Outcome and boundaries

A race's sound-set load (`load_session`) runs the original's upload through the native CPU, IPL
and driver. Timed in the optimized app on HILL CLIMB's load: the driver and tables 289 ms for 15.6
frames, the samples 1,186 ms for 63 frames, about 1.07x real time; the debug build about 0.8x.
Every port access calls `NativeAudioEngine::synchronize()`, and the sound processor yields back to
the CPU by throwing `CpuYield`. In a debug build the producer falls behind at every load, and the
app's late-output trim drops the race music's first 0.66 s.

Make the upload at least 5x real time in both builds without changing a single port write, DSP
write or PCM pair.

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| Speed | Time `load_session` for a race, the title and an ending, debug and optimized | At least 5x real time | JSON |
| Nothing moves | Conditional checks (`cond.sh full-*`), anchored measurement, continuation, audio unit tests | Identical port, DSP and PCM streams | logs |
| Live | The app-debug app's scripted race (`hill-complete` inputs), with the dropped pairs' peak level reported (add it to the app's report) | Every drop after the boot's silent (peak 0); the boot alone drops about 10,000 silent pairs | JSON |
| Review | Tier 1 | Approved | review on the pull request |
