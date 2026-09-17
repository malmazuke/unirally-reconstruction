# Next session — resume incomplete M4-16

**M4-16 is incomplete and unaccepted.** The open high-priority opponent reward
finding is closed and independently approved, but the product acceptance items
below are still open. Nothing is merged to `main` and no milestone tag is due.

## Resume location and identity

Use `.worktrees/m4-16-playable-zoom-zoo`, branch
`codex/m4-16-playable-zoom-zoo`, not main. Main retains accepted M4-15 gameplay
and a documentation pointer at `80cd742`. Inspect actual `git status`/`git log`
before resuming. Read [M4-16](M4-16.md), [review](M4-16-review.md),
[STATE](../docs/STATE.md), AGENTS, AGENT_WORKFLOW, D-0004/D-0006 and
[R-0035](../docs/research/R-0035-zoom-zoo-playable-recovery.md).

Primary is now **Claude Opus 5** by explicit user instruction after GPT Astra
and Fable 5.1 credits were exhausted; D-0004 records the authorized move and its
two consequences. The independent reviewer is a fresh Claude Opus 5 subagent in
`.worktrees/m4-16-review`, which holds four review commits ending at `0945ef2`.
No reset, purchase, paid fallback or further provider change is authorized.
Weekly percentage telemetry does not exist under this provider; record UTC wall
clock instead of inventing a figure.

## What is done and approved

Experimental state is **URZZ000B / 742 bytes**; static pack is
**v5 / 50 entries**, and fresh extraction from the user's ROM reproduces it
byte for byte. The generic opponent reward consumer `$81C219-C2C9` is recovered
and independently approved across three review rounds. Evidence and the exact
commands are in [M4-16](M4-16.md); do not re-derive them.

Passing on the current tip: 21/21 focused tests; six complete cases at 6225
states each covering both outcomes; the primary gate at 6225 observations, 757
fresh-process restores and a full fresh restart; seven reward probes plus the
150 control; 405 synthetic checks on both app presets with no skips; 20
historical M4-12-M4-14 and DRAGSTER commands on both lab presets; bootstrap,
pack-only, wrong-ROM, truncated and corrupt-pack gates; and denied-execution
autonomy with controls proving the denial was in force.

`local/native/dragster-idle` was missing here and was restored from the main
checkout; without it every historical command exits 2, missing prerequisite.

## Remaining work, in order

1. **Closed: the opponent multi-axis AI trick.** A live playthrough aborted with
   `ZOOM ZOO multi-axis AI trick is unrecovered`. `$83E1CB-E21A` sets the
   selector to `x & 7` on a sloped launch; bit 0 is the rotation, bit 1 the
   opponent's A (`$031F`) and bit 2 its X (`$0323`). All six unrecovered values
   are now recovered from original evidence and independently reviewed, and the
   A-dependent rotation rate at `$82A49F-A5F9`, which had been keyed to the
   player's button, now reads each rider's own A. Eight frozen probe cases match
   742 bytes across 41 observations each; a live run then reported **85
   multi-axis updates across selectors 2 and 7** with no abort. Nothing is
   outstanding here; it is listed so the history is not re-derived.

2. **Live controls — demonstrated; only the reviewer's own exercise is due.**
   A complete three-lap race, the RUNNER UP result, the authored pause menu and
   a clean restart were driven with real key events: 5,785 nonzero updates from
   21 presses, PAL cadence measured at 10.16 s of race clock per 10 s wall. Use
   the computer-use per-app approval flow
   (https://code.claude.com/docs/en/computer-use), **not** hand-granted
   Accessibility: Bash subprocesses run under `com.anthropic.claude-code`, a
   different bundle from the granted desktop app, so `CGEvent.postToPid`
   silently delivers nothing and `artifacts/m4-16/live-key` is a dead end. The
   track is a **loop** — one held direction cannot finish it; the original
   alternates direction thirteen times. Ride the whole race in one uninterrupted
   sequence, since gaps let the rider coast and desync. Never substitute a fixed
   mask, replay or terminal runner. Outstanding: the reviewer's independent
   exercise of live controls and a result/restart boundary, gamepad (no
   hardware), and the focus-loss active-clear witness, which reads 0 because
   macOS releases held keys before `SDL_EVENT_WINDOW_FOCUS_LOST`.
3. **Closed: M4-15 race matrix.** The reference pairs were in the M4-15
   worktrees, not missing. All eight `zoom_zoo_race` commands (primary, delayed
   turns, early jump, lap-two jump on app-debug and app-sanitize) pass on clean
   `787c549`; the script is recorded in [M4-16](M4-16.md). Re-run it on the
   exact final tip before integration. Never edit tracked files while it runs:
   the tool refuses the mixed run.
4. **Visual acceptance — independent readability review due.** Original scene
   identities are frozen in
   `tests/manifests/presentation/zoom-zoo-playable-v2.json`; private originals
   are `visual-original-a/b` and `brake-a`, current comparison
   `opus-resume/visual/comparison.png`. The HUD lap counter was one lap behind
   and is fixed in `f39a0a9`; the pause `>` marker and the space glyph are fixed
   in `c179765`/`787c549`. Recorded limitations: missing direction arrow and
   coaching cue, authored HUD style, rider anchors off by roughly 8-14 pixels,
   low-salience result graph points.
5. **Result-loading read classification** against the durable
   [persistent-input ledger](../docs/research/M4-16-persistent-input-ledger.md).
   Zero unresolved stores is not zero unresolved reads.
6. **Final integration**: reviewed exact merge, hosted macOS/Linux CI on the
   exact tip, and synchronized private main. Hosted Linux is synthetic coverage,
   not private Linux differential execution. No M4-17 and no milestone tag.

## Launch recipe

```sh
python3 tools/project.py frontend run --track zoom-zoo --pack local/classic-crawler-two-tracks-v5.pack --preset app-debug --report artifacts/m4-16/FRESH-live.json
```

Add `--rom` with the private locator's ROM and a fresh pack path for a first
extraction. This remains a prototype.
