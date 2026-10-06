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
  allowance (D-0004): at claim weekly 61%, five-hour 6%. Claimed under a remembered "continue
  until weekly 80%" rule, which the user corrected on 6 October 2026: there is no standing rule
  to chain tasks (D-0004); this task is the session's last.
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

## Evidence and attempts

Tools and evidence are in main `local/evidence/audio-upload-speed/`. Base binaries are main
`1b3a08a`'s (`base-1b3a08a/`). Load times come from `race_audio_runner` with
`UNIRALLY_LOAD_TIMES` (each load frame's wall time against the CPU ticks its session runs) and
no event sink, as the app runs.

| Attempt | Hypothesis | Experiment | Observation | Next decision |
| --- | --- | --- | --- | --- |
| 1 | The upload's cost is the yield | `sample` of the optimized runner in a race load | About 60% of the samples in dyld's unwind-section lookup and libunwind, the rest in the CPU work clock | Yield without an exception |
| 2 | The IPL and driver can stop before the access that would yield | `AudioDriverBus::yield_due` asked before each port access, `advance_clock` returns the force-sync yield; `synchronize()` ends on the return | Every load 16.5x real time optimized, 6.5x debug (main 1.2x and 0.8x); every event and PCM pair equal to main's | Live check, gates |
| 3 | The app keeps up in debug | `app-debug`, hidden, `hill-complete`'s inputs, with each drop's peak in the report | The boot's drop alone (2,815-3,775 pairs, peak 0); no later drop | Records, review |

The rest of an upload's time is the CPU work clock (`AudioCpuWorkClock::step`, its NMI, HDMA
and bus polls): about half of the debug build's samples. Not changed: the target is met.

## Gates

`local/evidence/audio-upload-speed/gates.sh` on `9fdf509` (`gates-9fdf509.out`, and
`gates-9fdf509-supplement.out` for two of the script's own errors: track 22 is a stunt event, and
main's runner had no timing hook; main's load times are `load-times-main-9fdf509.out`, from a
temporary build of `1b3a08a` with this task's runner).
- No source outside the audio engine and the app's audio output changed, so the race gates and
  sweeps were not run.
- Presets and ctest: 41 of 41 on each of the four. Synthetic suite and hidden app runs: pass.
- **Streams.** Every native audio event (port, DSP, RAM and the CPU's side, up to 102 million a
  run) and every PCM pair equal main's on all 65 schedules with native cues (raw mode, optimized),
  and on three in the debug build. The base runners load the third-party SPC_DSP library from
  this worktree's build; it is unchanged.
- **Playback and continuation.** At 48 kHz the full run's PCM, events and final state equal
  main's (`hopper-gold`, `bowl-lose`), and 26 + 11 fresh-process saves resume exactly.
- **Against the original.** The conditional checks on `full-six-quits`, `full-hopper-gold` and
  `full-lap-won` are exact. The anchored measurement equals R-0077's. AUDIO-TITLE-MENU's six
  comparisons stay equal.
- **Speed** (`load-times-*.json`, best of three):

  | Build | Slowest load, main | Slowest load, candidate |
  | --- | ---: | ---: |
  | optimized (`lab-release`) | 1.23x | 16.45x |
  | debug (`lab-debug`, as `app-debug`) | 0.84x | 6.52x |

  Every kind clears 5x: race, title, title after an award, award and ending.
- **Live** (`live-app-debug.json`): `app-debug`, hidden, `hill-complete`'s inputs. 2,815 pairs
  dropped at the boot, peak 0; no later drop; no underrun.
- Tooling 544 OK; no function over 80 lines; the address index is current.
