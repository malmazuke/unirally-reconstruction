# GATES-AGAINST-MAIN - check changes against accepted native code

## Assignment

- Status: **claimed** 24 October 2026 on main `ff1efb6` (Claude Code desktop app, Claude Opus 5.5
  `claude-opus-5-5`; a side session started by the RECORD-SCREENS coordinator). Queued 11 October
  2026 by [EVIDENCE-RETENTION](EVIDENCE-RETENTION.md), at the user's request. Weekly usage at claim
  46%, 5-hour window 27%.
- Why now, and why this task rather than a new one: on 24 October 2026 the user said hour-long
  gate checks and a process using hundreds of GB of disk are not acceptable and asked for a better
  way. RECORD-SCREENS' gates took 5 h 18 min serially and grow with every task, because each task
  appends the earlier tasks' `checks.sh`, and their comparison scripts write every picture to temp.
  Comparing against `main`'s native output is this task's design, so the user's request extends it:
  the decision also chooses which checks run on every change and which only when a change touches
  their system, and the task delivers one gate runner that keeps the run under an hour and its temp
  under a few GB.
- Branch `task/gates-against-main` in `.claude/worktrees/elegant-bhaskara-4abb94`; evidence in main
  `local/evidence/gates-against-main/`.
- Task provider: Anthropic.
- Milestone: M4 (original game coverage); project infrastructure.
- Review tier: **1** (it changes differential gates and reference baselines, D-0008).
- Dependencies: [D-0011](../docs/decisions/D-0011-evidence-retention.md).
- Shared state: heavy runs go through `local/locks/heavy-run.sh`; RECORD-SCREENS' final gates hold
  it until about 23:00 UTC on 23 October 2026, so design and tracked code come first.

## Why

- **What the gates do today.** The gates compare each candidate with stored captures of the
  original: the eleven frozen race gates, the front-end and audio cue comparisons. The folders
  they read hold about 122 GB.
- **Why that may be more than needed.** Once a part is accepted, native was proven equal to the
  original on those inputs. A candidate equal to accepted `main` on the same inputs is therefore
  equal to the original on them, for the same compared fields.
- **What the project already does.** The race sweep and front-end sweep already compare against
  `main`'s binary.
- **The user's question (11 October 2026).** Why keep the original's captures once a part is
  migrated?

## Outcome

- **The design.** A decision record proposes which gates compare against `main`'s native output
  rerun from its commit, and which must stay against the original:
  - changes meant to alter behaviour;
  - parts not yet native;
  - known residual differences;
  - the first acceptance of a capability.
  It also says how a task regenerates the original's capture from its manifest when it needs one.
- **The proof.** The switched gates catch a deliberately introduced regression in each gated
  system (race state, pictures, cues), and the remaining original captures are listed with the
  reason each stays.
- **The payoff.** With the decision adopted, the original captures the switched gates no longer
  read can go under D-0011, keeping their recipes.

## Boundaries

- Acceptance of new behaviour still compares with the original.
- No gate is weakened silently. Each switched gate's compared fields and frame ranges are the
  same as before.

## Design - 24 October 2026

[D-0012](../docs/decisions/D-0012-selective-gates-against-main.md) records the decision; this is
how it was reached.

- **What the 5 h 18 min is spent on.** A read-only survey of every section of RECORD-SCREENS'
  `gates.sh` and the four `checks.sh` it runs (which binary, which options, which inputs, which
  outputs, against what), and the per-file times of its run at `b6fa277`, give three causes:
  - the eleven frozen gates re-project two 850 MB captures each, although their contracts hold
    the digest of the projected rows;
  - the earlier tasks' scripts compare, in Python, native pictures the candidate did not change
    with the original's frames;
  - everything is serial.
- **Choosing checks by system.** The project already proves, with ninja's recorded dependencies,
  that a presentation change cannot move a frozen gate's rows (`gate_identity`). The runner applies
  that per binary, so "touches its system" is measured rather than guessed from paths.
- **Comparing with main, on the checks' own inputs.** The checks derive part of their native input
  from the original: race initialization frames, record writes, warm-boot images, pinned packs.
  So the corpus is recorded by a shim in place of `front_end_runner` while the checks run in a full
  run, rather than re-derived. The replay compares a superset of every check's outputs:
  `--frame-digests` adds per-frame picture, cartridge RAM and VRAM digests to the runner, so no
  picture is stored.
- **The frozen gates keep comparing with the original**, through the contract's frozen digest
  (`--frozen-original`).
- **The long scripts are split per capture** (MENU-INPUT's, LEAGUE-TOURS'; RECORD-SCREENS' own as
  a `.list` for its measurement), so that a difference re-runs only its own capture and the full
  run spreads them over the cores.

## Acceptance

| Criterion | Command or experiment | Expected result |
| --- | --- | --- |
| The runner selects and runs the gates | `python3 -m tools.unirally_lab.gates run` on RECORD-SCREENS' change (`.worktrees/gam-rs`, base the runner's commit) and on PODIUM-AUDIO's (`.worktrees/gam-pa`, base `6c50ac1` with the runner) | under an hour each, temp under a few GB, every selected step's result readable |
| The full run works and records the corpus | `run --full --record --base HEAD` in `.worktrees/gam-main` | every step runs; the corpus covers every check against the original |
| It catches a regression in each gated system | `gam/reg-race`, `gam/reg-picture`, `gam/reg-cue`, `gam/reg-driver` (local, never pushed) | each is selected by its binary and fails a step |
| No check is lost | every section of `gates.sh` is a catalogue step (`tests/tooling/test_gates.py`) | yes |
| Tooling | `python3 -m unittest tests.tooling.test_gates` | passes |
