# DRAGSTER-CLOCK-LIMIT - independent review

## Verdict

**Approve.** Every claim in the handoff that I could test, I reproduced from
the ROM and from fresh captures in this checkout, and none of it depended on
the implementer's fixtures. The only native code change is a strict extension
of the DRAGSTER result composition: it can turn a former exception into a
render, and it cannot alter any render that previously succeeded (argument and
evidence in Finding 1). Three advisory findings are recorded below; none of
them blocks integration and none of them changes a recovered behaviour.

## Candidate and build identity

- Branch `task/dragster-clock-limit`, commit
  `c00dd5d509572e4de720a85907f88d221ef5888f`; `git ls-remote origin
  task/dragster-clock-limit` resolves to that same commit.
- Hosted CI run 35276680047 (`synthetic`) is `completed`/`success` at head SHA
  `c00dd5d5...`.
- Review checkout `.worktrees/dragster-clock-review` on
  `review/dragster-clock-limit` at `c00dd5d`; working tree clean for every gate
  (`source_diff_sha256` is the empty-diff hash `e3b0c442...` in all reports).
- ROM `a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`
  (LoROM FastROM, no copier header, title `UNIRALLY`), core
  `e59bf88d4fc922c9fe3b5438e65ff3a6909d24e1628f0f87141c8de17699a91b`.
- Two-track pack extracted in this checkout,
  `b75539a0adfe13c3cb2541631b0b7107d8ab41a563bed56fdc2096fd8f4fd442`.
- `build/app-debug/src/core/zoom_zoo_runner`
  `2871eef15f6950b89f8b85d612130cbd63abe61eac3c1b9a9c722b99a375c83a`.

Evidence lives under this checkout's ignored `artifacts/review/`.

## What I reproduced

### 1. The original, re-captured from the ROM

Two fresh cold-start captures of the frozen case, run in parallel, about 75
seconds each and 4.3 GB of raw memory each:

```sh
python3 -m tools.unirally_lab.native.dragster_playable_reference --core local/emulators/bsnes/bsnes/out/bsnes_libretro.dylib --case tests/manifests/native/dragster-clock-limit-idle.case.json --horizon 32200 --out artifacts/review/idle-a --frame-image 31533 --frame-image 31534 --frame-image 31600 --frame-image 31884 --frame-image 31885 --frame-image 32016 --frame-image 32100
python3 -m tools.unirally_lab.native.dragster_playable_reference --core local/emulators/bsnes/bsnes/out/bsnes_libretro.dylib --case tests/manifests/native/dragster-clock-limit-idle.case.json --horizon 32200 --out artifacts/review/idle-b
```

Both runs returned the digests the task claims, identical to each other:
WRAM `52602f4a1b74b827eacc2aeb0ff4c149c7857d934b2d31a3af9b53c16ca6b3ed`,
SRAM `6d8782df9fdfe7fdd08d72e83213ce8d71a59f37ade40605366a4c8e07cc2e2e`,
video `3b8bb9651102d5148542c6d711508985194eb69f1306f010e86f9de89cc4c6c3`.

`dragster_playable inventory` over my own pair produced a file **byte-identical
to the tracked contract** `tests/manifests/native/dragster-clock-limit-idle-incomplete.inventory.json`
(compared key by key; no key differs and no key is missing on either side),
including `original_sha256 af0733a2...`, `rows_sha256 ba22a1a2...`,
`race_and_loading_rows_sha256 3fb792a5...` and
`race_and_loading_frames [1328, 31881]`.

Decoding my own capture's 742-byte rows by the tracked field layout confirms
claim 1 in detail:

| Frame | Observation |
| --- | --- |
| 31533 | clock already held at 9:59.9 (`timer` 9/5/9/9), player not finished |
| 31534 | `player.race.finished` 0 -> 1; `laps_remaining` stays 1; digits stay `0,0,0,7,0` (0:00.70); `player.total_time` 60000, `opponent.total_time` 3358 |
| 3214 | the opponent had already finished ordinarily (`finish_frames [31534, 3214]`) |
| 31774/31775 | `finish_delay` reaches 240, then result loading starts (`result_updates` 1) |
| 31884 | the original publishes `result.player_total`/`result.opponent_total` = `{60000, 3358}` (`result_updates` 110) |
| 32016 | `result_updates` 242, the DRAGSTER loser stable count; state unchanged to 32200 |

The original frame image at 32100 (`artifacts/review/idle-a/frame-32100.png`)
shows `DRAGSTER / COMPLETE / PLAYER TIME / MIKE NO TIME` above the three
`SOMEONE NO TIME` rows, confirming the NO TIME player row.

One correction of emphasis, not of substance: the ROM arm writes the finished
flag for **both** riders at 31534, but the opponent had been finished since
3214. R-0039 states this correctly ("the opponent finished ordinarily at 3214");
the shorter phrasing "finishes both riders at frame 31534" in the handoff can
be read as if the opponent finished there too.

### 2. The row census (claim 2), from my own capture

`artifacts/review/census.py` runs the native runner over the whole frozen
timeline from a fresh native initialization and compares every row:

```
original rows 30873  frames 1328 32200
native rows   30873  exit 0
DIFFERING ROWS: 2
31882 [["result.player_total", 60000, 0], ["result.opponent_total", 3358, 0]]
31883 [["result.player_total", 60000, 0], ["result.opponent_total", 3358, 0]]
```

Exactly the two rows claimed, exactly the two fields claimed, in the claimed
direction: native publishes the totals on the ordinary mode-0 update (31882)
while the original waits two further updates (31884). 30,554 consecutive
updates (1328-31881) and the 317-update tail (31884-32200) are byte-identical
on all 742 declared bytes. This is the same two-update SPC700 wait
`$82:8088-809A` M4-16 already recorded for ZOOM ZOO's time-out, and the
contract excludes audio timing.

The prefix gate over my own captures:

```sh
python3 -m tools.unirally_lab.native.dragster_playable compare --prefix --reference artifacts/review/idle-a --repeat artifacts/review/idle-b --contract tests/manifests/native/dragster-clock-limit-idle-incomplete.inventory.json --binary build/app-debug/src/core/zoom_zoo_runner --pack local/classic-crawler-two-tracks-v7.pack --out artifacts/review/prefix-gate-app-debug.json
```

`status passed`, `acceptance false`, frames `[1328, 31881]`, 36 fresh-process
restores, the declared set including 31533/31534/31535, 31774/31775/31776 and
31880. 102 s.

### 3. The ROM claims, disassembled here

I disassembled the PAL ROM directly (LoROM, bank `$81` at file offset
`$C73E`) with a scratch 65816 decoder built on the project's own opcode table.
`$81:C6C5` onward is the race clock:

```
$81:c6c5  LDA $0e29 / INC / STA $0e29 / CMP #$0001 ... CMP #$0005   (subframe, 5)
$81:c6fb  LDA $0e25 / INC / STA $0e25 / CMP #$000a                  (tenths, 10)
$81:c710  LDA $0e21 / INC / STA $0e21 / CMP #$000a                  (seconds, 10)
$81:c725  LDA $0e1d / INC / STA $0e1d / CMP #$0006                  (tens of seconds, 6)
$81:c737  LDA $0e19 / INC / STA $0e19
$81:c73e  CMP #$000a          <- the 10:00 arm
$81:c741  BNE $c75e
$81:c743  LDA #$0009 / STA $0e25 / STA $0e21 / STA $0e19
$81:c74f  LDA #$0005 / STA $0e1d                                    (hold 9:59.9)
$81:c755  LDA #$0001
$81:c758  STA $0eff           <- player finished flag
$81:c75b  STA $0f01           <- opponent finished flag
```

So `$81:C73E-C75B` is exactly the arm claimed, it holds 9:59.9, and it sets
both rider-finished words. `$0EFF`/`$0F01` are the same words the reference
projection reads as `finish_frames`. There is **no track test anywhere on this
path**: it is the shared race clock, which is why DRAGSTER inherits it.

Native's `advance_timer_digits` (`src/core/input_timer.cpp:37`, unchanged by
this task) matches the disassembly rollover for rollover, including the hold,
and takes no track argument. Its caller
`src/core/movement.cpp:2120` is
`if(advance_timer_digits(whole.timer,whole.countdown<68) && state.native_initialization) for(auto& rider:next.race.riders)rider.finished=1;`
- gated on `native_initialization`, not on
`content.movement.sampling.track`, so claim 2's "no gameplay change was
needed" is structurally true, not just empirically true.

The `242` loser stable count is not new in this task: `STABLE_RESULT` in
`tools/unirally_lab/native/dragster_playable.py` is pre-existing (R-0012,
R-0019) and my capture confirms it independently - loading at 31775, the
result state frozen from `result_updates` 242 at 32016 through the 32200
horizon.

### 4. The relaxation in claim 3, probed independently

The change admits one sentinel:
`player_has_no_time = finish.finish_time_centiseconds[0] >= 60000`, which
skips the player's digits/total consistency test and writes ` NO TIME` instead
of the digits.

I probed `build_dragster_result_map` directly with a temporary block appended
to `tests/native/presentation_tests.cpp` in this checkout (built app-debug,
run, then reverted with `git checkout --`; the tree was clean again before any
gate ran):

| Probe | Result |
| --- | --- |
| baseline timed-out state `{60000, 3358}` | ACCEPTED (draws NO TIME) |
| player not finished | rejected |
| opponent not finished | rejected |
| sentinel player declared `PlayerWon` | rejected |
| `outcome` `Pending` | rejected |
| `result_loading_updates` 241 | rejected |
| phase `ResultLoading` with 242 | rejected |
| player 59999 against 0:00.70 digits | rejected |
| player 59990 with agreeing 9:59.90 digits | ACCEPTED (draws the real time) |
| opponent sentinel 60000 | rejected |
| opponent 65535 | rejected |
| player 65535 | ACCEPTED (also at or above the sentinel) |
| player sentinel with out-of-range digits `{77,88,99,12,34}` | ACCEPTED |
| player sentinel with the clock **not** at 9:59.9 | ACCEPTED |

So claim 3's two named rejections hold, and so do several more I added. Two
acceptances are wider than the clock-limit case; see Findings 2 and 3.

**The change cannot alter any previously accepted render.** A time is
"consistent" only when `d0*6000 + d1*1000 + d2*100 + d3*10 + d4` equals the
total with `d0<=9, d1<=5, d2..d4<=9`, whose maximum is 59999; and the race
clock itself holds at 9:59.9 = 59990 centiseconds. So any state that rendered
before had a player total at most 59999, `player_has_no_time` is false for it,
`times_are_consistent` reduces to the original expression and the same `time`
digits are written. The relaxation is a strict extension of the accepted
domain: it only converts former `unsupported Classic result composition`
throws into renders.

That is also borne out empirically - both accepted DRAGSTER presentation
contracts still pass unchanged (see gates below).

### 5. Native draws what the original draws

The native row and the original row at 32016 and 32100 are byte-identical, and
`dragster_race_picture_runner` on the native 32100 state renders
`DRAGSTER / COMPLETE / PLAYER TIME / MIKE NO TIME` with the three `SOMEONE
NO TIME` rows (`artifacts/review/native-32100.png`), matching the original
frame within the accepted declared omissions (the original's `1P>` arrow and
stopwatch icons are pre-existing R-0038 omissions, unchanged by this task).

## Gates I ran in this checkout

| Gate | Result |
| --- | --- |
| Four presets, `build` + `test --suite synthetic` (lab-debug, lab-sanitize, app-debug, app-sanitize) | `409/409` checks passed on each, `23/23` of them `ctest:` checks; **no skipped or missing checks on any preset** |
| DRAGSTER historical matrix, 20 commands, fixtures copied into `artifacts/review/fixtures/` | `TOTAL COMMANDS: 20`, `ANY FAILURE: 0` (6 compare, 4 restore, 6 finish, 2 opponent-first, 2 presentation) |
| DRAGSTER presentation contracts | `classic-crawler-dragster-v1` 7/7 regions within limits; `classic-crawler-dragster-loser-v1` `stable-loser-result-3800` 1073/57344 = 1.871% against the 15% limit - the accepted contracts and thresholds are unchanged |
| DRAGSTER frozen original `dragster-ordinary-primary`, **re-captured by me** (`artifacts/review/regressions/primary-a|b`) | passes on app-debug and app-sanitize, 379 restores, frames `[1328, 3900]`, matching the tracked freeze |
| M4-16 ZOOM ZOO primary gate `primary-v11` | passes on app-debug and app-sanitize, frames `[1376, 7600]`, 757 restores |
| DRAGSTER clock-limit prefix gate, my captures | passes on app-debug and app-sanitize, frames `[1328, 31881]`, 36 restores, `acceptance: false` |
| Frozen-gate script total | `TOTAL COMMANDS: 6`, `ANY FAILURE: 0` |

Scripts: `artifacts/review/presets.sh`, `artifacts/review/historical.sh`
(the M4-16 script with its `cd`, `H=` and fixture paths repointed at this
checkout), `artifacts/review/frozen.sh`. All fixtures live inside this
checkout's `artifacts/review/`.

Provenance of the inputs I did not capture myself: the two presentation
fixture directories and the ZOOM ZOO `boundary-a|b` captures are copies of the
implementer's ignored artifacts. That is not a trust hole - `zoom_zoo_playable
compare` re-derives the capture and row digests and checks them against the
tracked `zoom-zoo-playable-primary-v11.freeze.json` (`original_sha256
d51e959d...`, `rows_sha256 b4a34af7...`) before comparing anything, and
`presentation-check` identity-checks each fixture against the tracked contract,
so a substituted fixture would fail rather than pass. The clock-limit captures
and the DRAGSTER primary captures are my own.

I also confirmed the new relaxation cannot leak into the accepted gates:
`compare --prefix` refuses any contract whose `kind` is not
`dragster_clock_limit_incomplete_original_inventory` or whose `acceptance` is
not `false`, and a plain `compare` on this case still fails in `original()`
because its result load timing is not the ordinary `loading+108`.

## Findings

### Finding 1 - the result-composition relaxation is sound (no action)

Severity: none. Recorded because the checklist asks for the weakened guard to
be examined. The guard is weaker in form but, per the arithmetic above, the
change is a strict extension: no state that rendered before renders
differently now. The opponent side cannot be relaxed at all, because a
consistent opponent time can never reach 60000, so an opponent sentinel is
structurally rejected rather than rejected by an ad-hoc test. The accepted
DRAGSTER winner and loser presentation contracts pass unchanged.

### Finding 2 - the sentinel admission is not tied to the clock limit

Severity: advisory (no change required for this task).

`player_has_no_time` tests only the sentinel total. A state where the player's
total is the sentinel but the clock is **not** held at 9:59.9 is accepted and
silently draws ` NO TIME` (probe row 14). Before this change such a state threw.
The differential gates would still catch a lost total on any frozen timeline,
but on live play or a new timeline the screen would be wrong silently rather
than loudly.

The project already uses a tighter predicate for the same condition elsewhere:
`src/core/movement.cpp:1697` gates its analogous restore relaxation on
`clock_expired` (timer 9/5/9/9), and `zoom_zoo_hud` at
`src/core/presentation.cpp:1064` defines `timed_out` as
`finished && laps_remaining!=0`. `build_result_map` already receives
`sample.movement.timer`, so the same clock test is one conjunct away:

```cpp
const auto &clock = sample.movement.timer;
const bool clock_expired = clock.minutes == 9 && clock.tens_seconds == 5 &&
                           clock.seconds == 9 && clock.tenths == 9;
const bool player_has_no_time =
    finish.finish_time_centiseconds[0] >= 60000 && clock_expired;
```

I did not require this because the current form is faithful to the original
for every state the engine can produce, and because tightening it would want
its own frozen evidence for the exact clock a settled DRAGSTER result carries
(my capture shows 9/5/9/9 held through 32200, so the conjunct would hold, but
that is one timeline). Worth a line in R-0039's "Not established" or a
follow-up.

### Finding 3 - the player's five digits stop being range-checked in the sentinel case

Severity: advisory.

With the sentinel admitted, `time_is_consistent` is skipped for the player, so
`finish_time_digits[0]` is no longer bounded (probe row 13 accepts
`{77,88,99,12,34}`). The digits are unused in that branch, so the drawn map is
unaffected, and the `time` array built from them is discarded; the conversion
`static_cast<char>('0' + digits[k])` from `int` is well defined, so there is no
undefined arithmetic. It is only a narrower guard than the comment implies. If
Finding 2 is taken up, adding an explicit digit-range check alongside it would
restore the bound.

### Finding 4 - small tool-side nits

Severity: advisory, `tools/unirally_lab/native/dragster_playable.py` only.

- `prefix_restore_boundaries` computes `range(2000, player, (player-2000)//8)`.
  For this case the step is 3691, but a player finish under frame 2008 would
  make the step 0 and raise `ValueError` from `range`, and a finish at or
  before 2000 would make it negative. It is a single-purpose helper, so this is
  a latent edge rather than a defect; a guard or a named constant would be
  clearer.
- `compare(..., prefix=True)` silently skips the
  `execute(first, actual[-1], True)` restart check. Skipping it is right (the
  prefix ends mid-loading, so there is no settled state to restart from) and
  the report declares `acceptance: false`, but the skip is not mentioned in the
  docstring next to the other prefix semantics.
- In a prefix report, `rows_sha256` holds the **prefix** digest while the same
  key in the inventory holds the whole-capture digest. `frames` disambiguates
  it, but the two files read as if they were the same quantity.
- The `boundaries = [...]` line chains two conditional expressions and a
  filter; splitting it would read better under D-0003's spirit.
- R-0039 and `docs/BUILD_AND_VALIDATION.md` both say a capture takes "about ten
  minutes". Both of mine finished in about 75 seconds each, run in parallel, on
  this machine. Harmless, but the estimate is an order of magnitude high.

## Checklist items with no finding

- **Changed baselines:** none. The diff adds two new manifests
  (`dragster-clock-limit-idle.case.json`,
  `dragster-clock-limit-idle-incomplete.inventory.json`) and edits no existing
  freeze, contract, threshold or expectation. All accepted contracts pass
  unchanged.
- **Masked skips:** the synthetic suites report 409 passes and zero skipped or
  missing checks on all four presets. The one deliberate skip
  (`compare --prefix` omitting the restart check) is declared in the report.
- **Undefined arithmetic:** none found in the C++ change; see Finding 4 for the
  Python edge.
- **Emulator fallback:** none. Every native row in the census and in the gates
  comes from `zoom_zoo_runner` started with `--start classic.crawler.dragster`
  from native initialization with a validated pack, never from original state.
  Restores seed from native rows only.
- **ROM-derived bytes in tracked files:** none. The only long hex strings in
  the whole `033d4a7..c00dd5d` diff are nine 64-character SHA-256 digests; the
  case file contains button names and frame numbers only. Raw memories, PNGs
  and gate reports all stay under ignored `artifacts/`.
- **Readability and evidence links:** the presentation change carries its ROM
  citation and the original's frame range; R-0039 separates observation from
  what is not established, and its "Not established" list correctly keeps the
  SPC700 wait, the DRAGSTER captions and the untested alternative timelines out
  of the accepted claims.

## Not assessed

- The other **six** frozen DRAGSTER originals (reversal, random-1..3,
  countdown-actions-tie, landing-held-roll). I ran the primary from my own
  fresh capture; the rest were covered by the implementer.
- The **two-update SPC700 wait itself** (`$82:8088-809A`). I confirmed its
  effect is exactly two updates and confined to two fields, and that M4-16
  recorded the same wait for ZOOM ZOO, but I did not disassemble the handshake
  or establish why it is longer only after a time-out.
- **Whether any non-idle DRAGSTER timeline reaches the limit.** Only the frozen
  case exists, as R-0039 says.
- **Live play** and the DRAGSTER window/HDMA follow-up, both out of scope.
- **Hosted CI on this review commit** (the review adds no source change).

## Usage at stop

5-hour window 11%, weekly 20% (limits: stop new work at 90% / 45%). Well
inside budget; the review finished on evidence, not on quota.
