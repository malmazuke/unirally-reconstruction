# HUNTER-EFFECTS - the HUNTER tour's tag effects

## Assignment

- Status: claimed 25 September 2026 by the Claude Code desktop session that ran
  TILE-PAIRS-8-12-26, base `47708b4`.
- Milestone: M4 breadth
- Coordinator: the claiming session is coordinator, primary and integrator
- Task provider (fixed for all children; record any user-initiated platform change): Anthropic
- Worker/session/runtime/model: Claude Code desktop app, Claude Opus 5.5 (`claude-opus-5-5`)
- Actual model/reasoning effort, routing rationale and frontier escalation question (if any):
  the claiming session's model at default effort; **tier 1** (simulation state in
  `src/core/movement.cpp` and the announcement queue).
- Provider quota window/baseline timestamp, used/remaining or unknown, reserve and session
  allowance (D-0004): at claim the 5-hour window was 7% used and the weekly window 12%; the
  user's stop point is 50% weekly.
- Reviewer (primary automatically spawns fresh model/effort, isolated checkout; no user
  trigger): a fresh Anthropic subagent with a withheld HUNTER capture.
- Dependencies and evidence of acceptance: TILE-PAIRS-8-12-26 (accepted); R-0050, R-0051;
  pack v13.
- Base commit: the `main` tip at claim.
- Branch and isolated worktree: `task/hunter-effects` in `.worktrees/hunter-effects`.
- Owned paths and shared interfaces: the race update in `src/core/movement.cpp` and its state
  layout, the announcement queue, presentation of any effect that changes the picture, native
  tests, a new research record, this record, `docs/STATE.md`, `tasks/README.md`,
  `tasks/NEXT_SESSION.md`.
- Claim/lease/heartbeat/checkpoint location: this record.
- Session time limit, concurrency allocation and actual spend authorization if relevant: one
  worker plus the review subagent; no monetary spend.

## Outcome and boundaries

On the HUNTER tour (`$131F` nonzero; tracks 40-44) the race runs `$83:CEC9` every update
(from `$83:CDB1`), which native lacks. R-0051 read it statically:

- `$83:D104` tests whether the two riders' boxes overlap (player x+8 to x+40 and y to y+40
  against the opponent's). It runs only while no effect is running (`$1323`), neither rider
  has finished, and once the riders' progress words `$0FCD` and `$0FCF` have differed by 2 or
  more (latched in `$1325`). An overlap sets `$1321`, `$1323` and one of eight effect flags,
  `$1327 + 2*(player x & 7)`.
- Each effect flag, the update it becomes 1, pushes its announcement to the front of the
  player queue (`$81:C55B`: written at the read cursor, which then steps back), plays sound
  `$021F`, sets `$12AF`, clears the other flags and runs its routine: `$1327` event `$1F`
  (`$83:D474`), `$1329` `$1C` (`$83:D530`), `$132B` `$1E` (`$83:D4AE`), `$132D` `$1B`
  (`$83:D2C9`), `$132F` `$20` (`$83:D3FC`), `$1331` `$1D` (`$83:D4E8`), `$1333` `$21`
  (`$83:D349`), `$1335` `$22` (`$83:D28A`).
- Most routines run a 500-update timer and end the effect with event `$23` (`$83:D275` also
  resets `$128D-$12AD`); the timed words (`$0557`, `$055B`, `$055D`, `$055F`, `$0561`, `$12B5`,
  `$12B7`, `$12CD`, `$7E:2052`, `$7E:2054`, `$1285-$128B`) and a blink table at `$83:D3BC`
  feed consumers elsewhere that are not yet traced. `$12D1` selects a palette fade instead
  (`$83:D1CA`).

Recover the trigger, each effect the HUNTER captures reach, and their consumers; add the state
to the race layout; show the effects in the presentation. Out of scope: stunt events.

## Inputs and prerequisites

The locked-tour captures (`local/evidence/locked-tours/sweep/hunter-*`), the listing under
`artifacts/static-map/` (`bank-83.lst` `$83:CEC9-D600`; the extract is
`local/evidence/tile-pairs/hunter-83CEC9.txt`), pack v13. Held-controller captures are needed
to reach more effects (the player's x at the tag picks the effect).

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| Effects named | Static reading and WRAM at each tag | Trigger, each reached effect, its consumers | research record |
| Match | `recompare --per-track` on the locked sweep; held captures on 40-44 | TWO LOOPS and HUNTER 44 exact past 1,252 and 1,620 | JSON |
| Nothing accepted moves | Gates, v1 contracts, hidden runs, fuzz, ctest, synthetic | Unchanged digests | gate logs |
| Review | Tier 1 | Approved with a withheld capture | review on the pull request |

## Evidence and attempts

| Attempt | Hypothesis | Experiment | Observation | Next decision |
| --- | --- | --- | --- | --- |
| 1 | - | WRAM `$1321-$1335`, `$12AF`, `$0557` at TWO LOOPS' 1,252 | Trigger, dispatch and effect 0's timer in one update | Read every consumer |
| 2 | - | Listing: consumers of the effect words | 0 BG2 axes swap (`$81:AE90`); 1 freeze pulses and 5 slow motion (`$128B` skips the race update, `$83:CC9A`); 2 player landing matrix 0 (`$81:94B9`); 3 screen flip (`$7E:2054`, HDMA, OAM y flip `$83:E04F`); 4 BG1 off (`$055D`, `$80:8638`); 6 mosaic (`$055F`, `$80:8821`); 7 reversed controls (`$82:AC5C`); captions `$1B-$24` ("barf mode on" ... "control reversed", `$23` blank) | Implement the simulation first |
| 3 | - | Simulation (`19b3b57` and before): tag, dispatch, timers, blink table (pack v14), push-front, skips, matrix 0, reversal, OAM flip of `screen_xy`, `$12AF` consumed at `$81:BF46` into the HUD buffer, the HUNTER opponent's character 20 (voices 232-247) | Locked sweep **20 of 20** exact (TWO LOOPS included), cold start 16 of 16; 48 held captures on HUNTER tracks exact, covering all eight effects (`held/`, `effects.py`) | Presentation |
| 4 | The effects need rendering | Pictures through each effect (`captures-pictures.sh`, `pictures.py`, `sidebyside.py`) | Frame 2,800 exact; differences: flip, barf, mosaic, BG1 hidden, the caption after a push-front, the HUNTER opponent's palette | Implement presentation (a) captions, (b) palette, (c) BG1, (d) mosaic, (e) barf, (f) flip |
| 5 | - | (a) the caption row as HUNTER state, canonical by text, projected from `$0EA7`; (b) asset 26 at OBJ palette 4; skipped updates treated as paused by the presenter | Slow motion and hedgehog pictures down to the declared arrow | (c)-(f) |
| 6 | NMI timing | Mosaic counter `$0563` counted at the update's start (the opening NMI); BG1 off and mosaic from the previous update; barf from the update before that (history) | Barf, wobble, invisible match to the arrow; wobble 3,403 differs by the caption transfer arbitration (`$81:F30C`) | (f) |
| 7 | The flip | BG1 rows mirrored and the riders' OAM y flipped from the previous update's blink; the V-flip bit outlives the blink by an update (`$80:876C` resets after the OAM transfer) | All flip frames to the arrow | Regression, records |
| 8 | - | `regression.sh`: 48 held captures, both sweeps, tile-pairs Right captures; 4,000-update hidden app runs on 40, 41, 43, 44 | All exact; no guard violations; app runs clean | Gates, review |

## Capability and coverage checkpoint

- Native capability delivered / still missing: the HUNTER tag and all eight effects, simulated
  and drawn; the opponent's character (palette, voices). Missing: the HUD transfer arbitration
  (one frame), sound, option bit 3's variants, `$12D1`.
- Frozen exact-match interval, field set and reference/seed identity: URTRnn06 (916 bytes); the
  sweeps as captured; 48 held HUNTER captures.
- Dynamic captured inputs still consumed (must be zero for autonomy): none new; the blink table
  and opponent palette are pack content (v14).
- Relevant branches/transitions exercised: every effect's start, run and end; two tags in one
  race; effect 1's growing and shrinking pulses; effect 5's skip pattern; the flip's blink phases.
- First divergence and cheapest next discriminating experiment: none left in the race captures.
- Trial-wide usage baseline/current, reserve, reset authorization/outcome or none: 12% weekly at
  claim, 13% at the records.

## Handoff

- Current base/head commit and uncommitted state: base `47708b4`; on `task/hunter-effects`.
- Verified findings: [R-0052](../docs/research/R-0052-hunter-effects.md).
- Commands executed, outcomes and report hashes: `local/evidence/hunter-effects/` (captures
  scripts, `held/`, `pictures/`, `regression.sh` and its outputs, `recompare-*.json`,
  `pictures-summary.txt`, `hidden-*.json`).
- Unavailable/skipped checks: the ASan presets (host).
- Exact next experiment/command: none for this task; [RESULT-TITLE-GLYPHS](RESULT-TITLE-GLYPHS.md)
  is next.
- Accepted outcome, review/fix rounds and next routing decision: pending review.
