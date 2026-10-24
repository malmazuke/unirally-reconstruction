# D-0012 - Gates that choose their checks and compare with main

- Status: proposed by [GATES-AGAINST-MAIN](../../tasks/GATES-AGAINST-MAIN.md) (tier 1), 24 October
  2026, at the user's request; adopted when that task merges.
- Owner: Claude Opus 5.5 (`claude-opus-5-5`), Claude Code desktop app, a side session of the
  RECORD-SCREENS coordinator.
- Related: [D-0003](D-0003-human-readable-native-code.md) (frozen differential checks),
  [D-0008](D-0008-static-map-track-breadth-review-tiers.md) (review tiers),
  [D-0011](D-0011-evidence-retention.md) (evidence retention).
- Tool: `tools/unirally_lab/gates/` ([BUILD_AND_VALIDATION](../BUILD_AND_VALIDATION.md#the-gate-runner-d-0012)).

## Problem

On 24 October 2026 the user said that hour-long gate checks, and a process that uses hundreds of
GB of disk, are not acceptable.

**Time.** RECORD-SCREENS' pre-merge gates (23 October, `local/evidence/record-screens/gates.sh`,
copied from PODIUM-AUDIO's) ran serially for 5 h 18 min under `local/locks/heavy-run.sh`.

| Part | Minutes |
| --- | --- |
| Builds of four presets, ctest, the synthetic suite | 15 |
| Eight hidden app runs (real time, about 80 s each) | 12 |
| The eleven frozen differential gates | 52 |
| The race equivalence sweep against main | 25 |
| Pause, two-pad, race-end, look and restart comparisons, track sweeps, earlier tasks' captures | 70 |
| MENU-INPUT's `checks.sh` / LEAGUE-TOURS' / PODIUM-AUDIO's | 35 / 57 / 22 |
| The task's own `checks.sh` | 55 |

**Growth.** Each task appends the previous task's `checks.sh` to its `gates.sh`, so every task's
gates are longer than the last.

**Disk.** Comparison scripts render every native picture as a 172 KB PPM into a temp directory and
compare it with the original's PNG. One gate step held 2.2 GB of temp; `wipe-ram/compare.py`
never deletes its directory (about 2 GB until the gate exits); `cond.sh` writes 140-215 MB into
evidence folders. The user saw 3.8 GB free during those gates. `local/evidence` holds 109 GB.

**Where the time goes.** The steps are mostly not checking the candidate:
- the earlier tasks' scripts compare, picture by picture in Python, native output that the
  candidate did not change with the original's frames;
- everything runs one step at a time on a 12-core machine.

The frozen race gates are mostly restore runs: hundreds of fresh processes, each replaying the rest
of a race from a saved state. Re-projecting the captures turned out to be a small part. This
section's first draft guessed otherwise, and the full run below corrected it.

## Decision

### 1. One runner

`python3 -m tools.unirally_lab.gates run`, from the candidate's checkout, under
`local/locks/heavy-run.sh`. It builds main's binaries at the merge base (one persistent worktree,
`.worktrees/gates-base`, built incrementally) and the candidate's presets. It then decides which
steps the change calls for and runs them in parallel where they do not contend. It prints a line
per step with its reason, and writes `report.json` (status, time, peak temp per step, the run's
peak disk). Task `gates.sh` scripts and the appending of earlier tasks' `checks.sh` stop. The step
catalogue (`catalogue.py`) holds every section of the last `gates.sh`.

### 2. What decides "touches its system"

A step names the binaries it drives. A binary can only behave differently if one of these changed:
- **its sources:** one of the repository files ninja recorded as compiled into it. This is
  `gate_identity`'s method, now per target: `front_end_runner`, `zoom_zoo_runner`, the presentation
  runners, `audio_driver_runner`, the fuzz runner and the app;
- **the content pack** it reads;
- **a build-wide file:** CMake files, presets, `third_party/`, the toolchain lock.

Every failed query counts as "changed". A step none of whose binaries changed would replay as it
did on main, so it does not run. Paths are not guessed from file names: a race-engine file is in
`front_end_runner`'s inputs too, because the front end runs races, so a race change also selects
the front-end checks.

### 3. What runs on every change, and what runs when its system changes

| Check | Runs | Against |
| --- | --- | --- |
| Builds of four presets with ctest; the synthetic suite (alone); tooling tests; the function-size and native-symbol rules | every change outside `docs/` and `tasks/` | - |
| The task's own checks | every run | the original (acceptance of new behaviour) |
| Front-end replay: every recorded check input through main's `front_end_runner` and the candidate's, every output on every frame | `front_end_runner` changed | main |
| Race equivalence sweep (47 scenarios x 9 schedules, restarts, pictures, the legacy runner) | a race runner changed | main |
| Eleven frozen differential gates | `zoom_zoo_runner` changed, or their projection or guards changed | the original, by the contract's frozen digest |
| Track sweeps (track-breadth-2, locked tours) | `zoom_zoo_runner` changed | the original and main |
| Restore checks (two-pad, idle demo) | `front_end_runner` changed | the candidate itself |
| v1 presentation contracts; fuzz | their runner changed | the fixtures; none |
| Hidden app runs; the app's audio producer (real time) | the app changed | status, drops |
| Driver check on captured port writes | `audio_driver_runner` changed | the original |
| The checks against the original that drive `front_end_runner`: pause menu, two-pad, race end, look, restart, idle demo, league frozen, TWO-HUMAN-STUNTS, CREDITS-NAME, WIPE-RAM, DEMO-AUDIO, MODE-AUDIO, split demo, MENU-INPUT and LEAGUE-TOURS per capture, PODIUM-AUDIO | their own recorded runs differ from main (section 4), or they have no recorded runs, or a full run | the original |

### 4. Comparing with main in place of the original (GATES-AGAINST-MAIN)

**The argument.** A check against the original runs `front_end_runner` on its inputs and compares
some of the outputs with the original's capture. If the candidate's outputs equal main's on those
same inputs, the check reads exactly what it read on main. That is where it was accepted, so its
verdict cannot change.

**The inputs are the checks' own.** A full run puts a shim in place of the candidate's
`front_end_runner`. It logs each invocation the checks make (the pads, the pack, the race
initialization frames and record writes the scripts derive from the original, the warm-boot
image), then runs the real binary. These logs are the corpus (`local/gates/corpus/`, one log per
step, input files stored by content). Nothing is re-derived, so the corpus cannot drift from what
the checks run. A task's own checks are recorded on every run, so the next task inherits them.

**The outputs are all of them.** The replay drops the outputs a check asked for and asks both
binaries for every output, on every frame:
- the state rows;
- stderr;
- the race, one-player and look timelines;
- the sound cues;
- a digest of the picture, the cartridge RAM and the VRAM (`front_end_runner --frame-digests`, new
  here; one 64-bit digest per frame and kind).

That is a superset of what any check compares, so "equal" is stronger than the check, not weaker.

**Where it differs, the original decides.** A check whose recorded runs differ from main runs
against the original. The candidate changed something the check measures, and only the original
can say whether that is the intended improvement or a regression. Escalation is per check, and
MENU-INPUT's and LEAGUE-TOURS' scripts are split one step per capture, so one changed screen
re-runs only the captures that pass through it.

**What stays against the original whatever main says:**
- **the task's own checks:** acceptance of new behaviour, and the first acceptance of a capability;
- **the eleven frozen race gates.** `--frozen-original` checks the contract's frozen digest of the
  original's rows instead of re-projecting both captures. That is the same comparison with the
  original, and it gives the same rows digests and restore counts. It saves little time; what it
  buys is that a gate no longer needs its 850 MB captures (section 9). They re-project when their
  projection code or guard manifests change, or with `--reproject`;
- **the track sweeps**, which are cheap;
- **the driver check on captured port writes**, whenever the driver changes;
- **everything, in the full run.**

**Known residual differences** need nothing special: main's output carries them, so equal to main
means unchanged.

**Parts not yet native** have no native output to compare, so this decision does not touch how
they are accepted.

### 5. The full run

`run --full --record` runs every step and every check against the original, and records the corpus
again. It is due:
- before each milestone tag;
- after every fifth merged task, or weekly, whichever comes first;
- whenever a check's script or a captured reference changes.

A full run's result is recorded in `docs/STATE.md`.

### 6. Disk

- **Pictures are never stored by the runner.** The replay reduces every stream to one 8-byte digest
  per line, read from pipes and named pipes as the runners write them.
- **Each step has its own `TMPDIR`.** It is measured every 3 seconds together with the step's output
  directory and anything new in the evidence folders the step is known to write into. A step past
  its cap (1 GB by default; 3 GB for LEAGUE-TOURS' captures, which compare pictures in 20,000-frame chunks) is stopped and fails.
- **The caps of the running steps may not sum past 6 GB**, so the run's own temp stays under that.
- **No step starts while free disk is under a 4 GB floor plus its cap.**
- **Each step's temp is deleted when it ends**, which also fixes the leaked `wipe-ram` pictures.
- The report records each step's peak and the run's lowest free disk.

### 7. Parallel, and the synthetic suite

Steps run in three lanes:
- **exclusive:** builds, and the synthetic suite. The suite's timeout probe fails under load
  (18 September and 14 October 2026), so it stays alone rather than being loosened;
- **real time:** the app runs, together with each other and with nothing else, so their late-pair
  counts are not load artefacts;
- **pool:** everything else, up to the job count (cores minus four), with wide steps such as the
  sweeps counted at their worker count.

Steps that write into the same evidence folder do not overlap.

### 8. A task's own checks

A task gives its checks to the runner (`--task-checks`):
- as a script;
- better, as a `.list` of independent commands, one per line, which run in parallel.

New comparison scripts should hash pictures in memory rather than write files. When the task
merges, the next task adds its checks to the catalogue, split per capture as MENU-INPUT's and
LEAGUE-TOURS' are.

### 9. Regenerating an original capture

When an escalated check needs a capture that D-0011 removed, the task regenerates it from its
recipe and checks the recorded digests, as D-0011 says. Since the frozen gates now read only their
`reference.json`, their memory captures (`memory.wram`/`memory.sram`, 850 MB per ZOOM ZOO capture)
are needed only to locate a first mismatch or to re-project. They are candidates for D-0011
deletion with their recipes kept. Deleting them is a separate step, not part of this decision.

## Measured

(Filled from the runs in GATES-AGAINST-MAIN.)

## Consequences

- A typical task's gates are a selective run; the long checks against the original run only where
  the change moved their output.
- Main's native output becomes the reference for "nothing else moved". A regression accepted into
  main would be inherited; the periodic full run, which compares everything with the original, is
  the guard against that.
- The corpus is local and ROM-derived in part (record writes, warm-boot images); it follows the
  evidence rules and is re-recorded by any full run.
- A check whose script is new or changed runs against the original until a full run records it.
