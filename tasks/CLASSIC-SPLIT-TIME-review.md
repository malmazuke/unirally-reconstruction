# CLASSIC-SPLIT-TIME - independent review

- Candidate: `c3fe84151dc51a31b0d48affd3583bfb180f7ef6` on `task/classic-split-time` (records on
  top of the code candidate `33e12a8`; `git diff 33e12a8..c3fe841 -- src tests` is empty, only
  `src/core/README.md` moved), base `main` at `ee5c132`. Commits under review: `708af4c`, `33e12a8`,
  `c8cd1ad`, `2298dc6`, `c3fe841`.
- Reviewing model: Claude Fable 5.1 (`claude-fable-5-1`), fresh subagent at default effort, no
  inherited conversation with the primary. D-0008 tier 2 as recorded; I did not escalate: the diff
  touches `src/core/presentation.{hpp,cpp}`, `tests/native/presentation_tests.cpp`, the source
  guide and records only. No engine, serialization, pack rule, manifest or gate file changed
  (`git diff --stat ee5c132..c3fe841`: six files).
- Checkout: `.worktrees/classic-split-time-review`, branch `review/classic-split-time`, verified at
  the candidate with `git rev-parse HEAD` before anything ran. Every command below ran from that
  directory against a build made there. The primary's worktree and `local/evidence` were read only;
  this report is the only tracked change.
- ROM `a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e` and core
  `e59bf88d4fc922c9fe3b5438e65ff3a6909d24e1628f0f87141c8de17699a91b`: every recapture below asserts
  both hashes and every replayed frame's video and WRAM digests against the frozen reference.
- Elapsed: about 20 minutes wall clock from `git rev-parse` to the report commit (21:40 to 22:00
  AEST, 11:40Z to 12:00Z). Usage was not sampled (the primary records it).
- Verdict: **approve**, with five should-fix items (none blocking) listed in section 5. The
  mechanism is what R-0044 says it is - I read it out of the ROM myself and re-checked the rule
  against the WRAM of five captures the primary's table does not measure - and native's two centred
  bands are 0 pixels on every frame of every withheld consecutive set I captured, on both tracks,
  including two things the primary never measured: the player's own `+` split on DRAGSTER (which
  R-0044 wrongly says no DRAGSTER original shows) and a DRAGSTER race in which the opponent finishes
  first and the player's split then draws over its standing time.

## What I ran

```sh
git rev-parse HEAD                                             # c3fe84151dc5...
PATH="$PWD/local/toolchain/ninja-1.13.2-darwin-arm64:$PATH" python3 tools/project.py build --preset app-debug > local/build.log 2>&1
#   status=passed elapsed=23.384s
ctest --test-dir build/app-debug > local/ctest.log 2>&1      # 100% tests passed, 0 failed out of 23
PYTHONPATH=. python3 -m tools.unirally_lab.native.gate_identity --since 6e0fad6 \
  --reports "$E/classic-stunt-names-review4/classic-stunt-names-review4/gates-6e0fad6" --expect 11 \
  --pack local/classic-pal-crawler-two-tracks-v9.pack --ninja local/toolchain/ninja-1.13.2-darwin-arm64/ninja
#   comparing 6e0fad6 with c3fe841: every input is byte identical and every report matches; 11 gates
PYTHONPATH=. python3 artifacts/classic-split-time-review/disasm.py 81 C910 CB23   # and EDDC, EF5E, F033, F1B5
python3 artifacts/classic-split-time-review/split_probe.py <five captures>       # local/split-probe-review.log
PYTHONPATH=. python3 artifacts/classic-split-time-review/recapture.py CAPTURE artifacts/classic-split-time-review/orig/NAME RANGES
python3 artifacts/classic-split-time-review/hud_compare.py CAPTURE PICTURES START FIRST_ROW > artifacts/classic-split-time-review/compare-*.txt
python3 artifacts/classic-split-time-review/band_look.py ... 3329 0 256 0 224    # what the whole-picture residual is
c++ ... artifacts/classic-split-time-review/queue_probe.cpp <the six core .a files>   # section 4
```

All sweeps and compares ran serially, one at a time, after the build and never during one.
Reviewer artifacts are in this checkout's ignored `artifacts/classic-split-time-review/`: the
recaptured originals under `orig/` (`trick-long-a`, `brake-a`, `dragster-reversal-a`,
`dragster-tie-a`, `dragster-random-1-a`), the recapture logs, `compare-*.txt`,
`gates/gate-identity.log` and `queue_probe.cpp`. `local/split-probe-review.log`,
`local/build.log` and `local/ctest.log` hold the WRAM probe, the build and ctest output.

## 1. The rule, from the ROM

Disassembled myself with `disasm.py` (LoROM, `$81:C910-CB23`, then the writers). Each point the
task asked me to check, with the instruction that settles it:

- **Requests.** `$81:C912 LDA $0FFF,y; CMP #$0078; BEQ $C928`: countdown 120 clears the sign flag
  (`STZ $119D`) and stores 1 to `$0349,y` (`$81:C92B-C92E`); `CMP #$0002 ... LDA #$FFFF; STA
  $0349,y` (`$81:C91A-C922`) is the blank at countdown 2. `$81:CB13-CB20` steps Y by 2 and loops, so
  it runs for both riders, player first.
- **Mode 0** (`$81:C931 LDA $119F,y; BNE; JMP $CAAD`): `$81:CAC5-CB10` copies `$0E3F`, `$0E4F`,
  `$0E4B`, `$0E47`, `$0E43` (,y) through `$80:81F4` into `$11B7`, `$11B3`, `$11AF`, `$11AB`, `$11BB`.
- **First through a slot** (`$81:C945 LDA $114D,x; BPL $C94F; JMP $CA38`): `$81:CA3C` clears the
  flag, `$81:CA41-CA5B` writes digit 0 four times and `$80:821F` (`+`, tile `$4C`) to `$11BB,y`,
  `$81:CA5E-CA61 LDA #$0000; STA $0349,y` **clears the request**, and `$81:CA64-CA77` (`$0C6D` set
  and `CPY #$0000` not equal) writes 2 to `$1001` and `$FFFF` to `$034B` - the opponent's own
  countdown and field, cut at once. Then `$81:CA8C-CAA8` stores `$0E19/$0E1D/$0E21/$0E25` at
  `$100D,x` with X = 16 * laps + 4 * checkpoint.
- **Seen slot** (`$81:C94F-C9E0`): tenths `SBC $100D+3,x; BPL; ADC #$0A; INC $02`, seconds `ADC
  #$0A; INC $01`, tens `ADC #$06; INC $00`, minute `SBC $00; BPL; EOR #$FF; INC $119D`. Sign flag
  clear: `$80:821F` into `$11BB,y`. Set (`$81:C9F4-CA33`): `5 - $01`, `9 - $02`, `10 - $03` with
  `CMP #$0A; BNE; LDA #$00`, and `$80:8220` (`-`, `$4D`). `$81:CA13 STA $11B3` is absolute, as
  R-0044 says. The character table at `$80:81F4` begins `0a 01 02 .. 09`, so a digit 0 is tile
  `$0A` and a value past 9 would name `$0B` upward - the refusal in `classic_hud_digit` is right.
- **Before the tick.** The routine reads `$0E19..$0E25` directly; whether that is before the clock's
  own tick is a question for the WRAM, section 2.
- **Writers.** `$81:EDDC LDA $0349; BNE; JMP $F033` / `BMI $EDE9` / `$81:EDE9 LDA $0EFF; BEQ $EDF1;
  JMP $F033`: a negative request for a finished player jumps straight to the opponent's handler
  without clearing `$0349` or writing. `$81:F040 LDA $0F01; BEQ; JMP $F28B` is the same for the
  opponent. The player's split writer `$81:EF69 LDA $11BB` reads the sign byte; the opponent's
  `$81:F1C0 LDA $808220` reads the constant minus. Both confirmed byte for byte.

Native (`ClassicRaceHudClock::observe_update`) follows each of these: the crossing recognised by
the next-checkpoint step with a nonzero countdown, mode 0 through `classic_hud_crossing_text`, the
first-seen store with `cell={}`, the seen-slot split from the previous update's clock, `-` forced
for rider 1, the blank at countdown 2, and the finished rider's blank dropped without spending the
update. `classic_hud_split_text` reproduces the borrow chain, the `EOR #$FF` and the `5/9/10`
complements as read. `reset()` clears `slot_times_`, and the app replaces `LivePresentation` on
every restart (`src/app/sdl_main.cpp:346,436`), so the restart path forgets the slot store.

## 2. The rule, from the WRAM

I re-ran `split_probe.py` on five captures: the M4-16 `brake-a` and `trick-long-a` (whole WRAM per
frame) and the DRAGSTER originals `reversal-a`, `regression-countdown-actions-tie-a` and
`random-1-a`. Result: 51 crossing requests; 21 splits, all 21 equal to the rule from the previous
update's clock and only 20 to the current one (`brake-a` 2077 is the discriminating case, as in
R-0044); 12 first-seen stores, all 12 the previous update's clock, 10 the current; 18 mode-0
displays, all equal to the crossing digits; 51 blanks, all at countdown 2 or on a finished rider; 0
negative. This is an independent subset of the primary's 77-capture run and agrees with it.

Two facts I took from the primary's own `split-probe.log` that its records do not draw out: in
`trick-long-a` the riders complete lap 1 one update apart (opponent 3208, player 3209), and in five
DRAGSTER originals (`random-1/2/3-a`, `reversal-a`, `regression-countdown-actions-tie-a`) the
**opponent** is first through checkpoint 2 and the **player's** split is published (`y0 split
cp2`). Both became withheld cases.

## 3. Withheld cases (the validation)

Consecutive originals recaptured on the audited core from timelines the primary did not use for the
split, every frame asserted against the frozen digests, scored with `hud_compare.py` in the four
boxes of R-0043/R-0044 (HUD rows, former bar rows, player band x 104-159 y 39-54, opponent band
y 159-174). Native's build: this checkout's `app-debug` of `c3fe841`.

| Capture (start id, first row) | Frames | What the frames hold | player band | opponent band | HUD + bar rows | whole |
| --- | --- | --- | --- | --- | --- | --- |
| M4-16 `trick-long-a` (`zoom-zoo`, 1376) | 2083-2090, 3205-3222, 3876-3890, 6479-6500 (63, 5294 checked) | the player's `+0:01:5` without a tick; the lap change one update apart (opponent 3208, player 3209: the left field twice, both crossing times `0:32:50`/`0:32:53`); the opponent's **zero** split `-0:00:0` (3878 player first, 3881 opponent second at the same tenth); the finish with the player first (6481) and the opponent 6488 | **0** | **0** | 0 | 285 (36 a frame on 2083-2090, the arrow; 0 elsewhere) |
| M4-16 `brake-a` (loser race) | 3626-3640, 6485-6500 (31, 5294 checked) | player first through cp1 of lap 2, the opponent's `-0:00:1`; the **loser finish**: opponent 6488 first, player 6492, both times standing | **0** | **0** | 0 | 57 (6495 only) |
| DRAGSTER `reversal-a` (`dragster`, 1328) | 3323-3336, 3529-3540, 3647-3653, 4290-4300 (44, 3142 checked) | the **opponent finishes first** on a sprint (3325, `0:33:63` written while `$0D17` stays set); the **player's own `+0:20:8` split** at 3531 over the opponent's standing time; the player's blank at countdown 2 (3649) while the opponent's finished field is skipped; the player's finish 4292 | **0** | **0** | 0 | 2037 (108/72/36 a frame; 417 on 3329, see 5.3) |
| DRAGSTER `regression-countdown-actions-tie-a` | 2395-2402, 2513-2518, 3224-3240 (31, 2082 checked) | the player's `+0:00:1`; its blank at 2515; the **finish tie** at 3226, both riders on one update: `finish`, clock blank, `0:33:58` twice | **0** | **0** | 0 | **0** |
| DRAGSTER `random-1-a` | 2715-2722 (8, 1564 checked) | the player's `+0:06:7` split with a nonzero seconds digit | **0** | **0** | 0 | 684 (108/72 a frame) |

I looked at the original pictures where the bands are claimed 0 to make sure the zero is not
vacuous: `trick-long-a` 3884 shows `-0:00:0` at rows 20-21 and 3213 both `0:32:53` and `0:32:50`;
`reversal-a` 3536 shows `+0:20:8` at rows 5-6 with `0:33:63` standing below. The whole-picture
residual outside the bands is the off-screen arrow (36 a frame on ZOOM ZOO; DRAGSTER's own
residuals of 108/72/36 sit at x 216-227, y 112-126, the same shape) except one frame, 5.3.

## 4. Trying to break the queue model

`queue_probe.cpp` drives `ClassicRaceHudClock` directly against the candidate's library:

- A crossing whose request waits behind the left field **and** the clock (a lap on a tick): the field
  shows three pictures after the crossing (`+1` left, `+2` clock, `+3` field). Consistent with the
  dispatcher order and with `trick-long-a` 3208-3212 measured above. Passed.
- A blank (player, countdown 2) and a draw (opponent's split) requested on the same update: the
  blank spends the update, the split follows one picture later. Passed.
- The finished opponent's field: the finish time stands through the countdown-2 request. Passed;
  also measured on `reversal-a` 3647-3653 and `brake-a` 6485-6500.
- After `reset()` a slot the queue never saw first: nothing drawn (the declared limit). Passed.
- Digit domain: `9:59.9 - 0:00.0` and `0:00.0 - 9:59.9` do not throw; `0:00.0 - 0:00.1` gives
  `-0:00:1`, which is what `EOR #$FF`, `5-T`, `9-s`, `10-t` produce. Passed.
- **Both riders first through one unseen checkpoint on the same update** (a checkpoint tie): native
  draws nothing for either rider. In the ROM `$81:C910` processes Y = 0 before Y = 2 in one pass,
  and Y = 0's `$81:CA3C STA $114D,x` clears the flag before Y = 2 reads it at `$81:C945`, so the
  original stores the player's clock and publishes `-0:00:0` for the opponent with a 120 countdown.
  The engine already orders it that way (`update_zoom_checkpoint` clears `checkpoint_seen` for
  rider 0 first and leaves rider 1's countdown at 120), but the presentation reads `previous`'s
  flag for both riders. **No capture shows a same-update checkpoint tie** (0 of 1,145 requests in
  the primary's log), so this is a should-fix from reading, not a reproduced failure (5.1).

## 5. Findings

No blocking finding. Should-fix, in order of weight:

1. **Same-update checkpoint tie (code, unmeasured).** `observe_update` should mark the slot seen
   once rider 0 has taken it before evaluating rider 1 (a local copy of `previous.race.checkpoint_seen`
   updated between the two riders, or `first = flag unseen && !(rider==1 && rider 0 took this slot
   this update)`), and a unit test should pin it: the player stores, the opponent gets
   `-0:00:0`. Reproduction: `artifacts/classic-split-time-review/queue_probe.cpp` case 1 prints
   `player=<blank> opponent=<blank>`; the ROM order above says `opponent=-0:00:0`. Record it in
   R-0044's limits either way.
2. **R-0044's DRAGSTER domain statement is wrong.** "DRAGSTER's single checkpoint is crossed by the
   player first in every original; no DRAGSTER original shows the player's own split" is
   contradicted by the primary's own `split-probe.log` (`random-1-a` 2717, `random-2-a` 2675,
   `random-3-a` 2474, `reversal-a` 3531, `regression-countdown-actions-tie-a` 2397, all `y0 split
   cp2`). The player's `+` split on DRAGSTER is now measured at 0 in three of those (section 3);
   replace the sentence with that, and the claim in the acceptance table that consecutive recaptures
   cover "every split publication ... on both tracks" is then true.
3. **Whole-picture residual attribution.** R-0044 says the residual outside the bands "is the
   declared off-screen rider arrow". On the DRAGSTER sets it is not only that: the primary's own
   `orig-dragster-splits` 3215/3216 (55/48) and my `reversal-a` 3329 (417) are in the caption rows
   83-93, and the pictures show the original's caption changing one picture **later** than native's
   (`reversal-a`: original 3329 still shows the previous sentence, 3330 `AND ... R`; native shows
   the new one at 3329). That is the dispatcher order R-0043 records - the caption is serviced after
   the two centred fields - which native's caption (R-0042, drawn from the reward queue) does not
   wait for. Not this task's bands and not a regression of this change (native's caption did not
   wait before either), but now that the fields are written mid-race it will show on any checkpoint
   crossing that coincides with a caption change. Record it as a limit here and as a follow-up for
   the caption model; do not describe it as the arrow.
4. **The time-out row says "see below" and nothing follows.** My run of the same pair
   (`m4-16-idle/captures/stop-timeout-a` against `m4-16-final-review/orig/stop-timeout-a`) gives
   the same totals as the primary's `compare-timeout.txt`: the 24 frames through 31582 are 0 in all
   four bands, and the 11 frames from 31933 differ over the whole picture (57,344 each) because they
   are result-screen pictures the HUD compare does not model. Write those numbers into the table.
5. **Comment nit.** `slot_times_`'s comment names the original's `$100D + 16 * laps + 4 * checkpoint`
   layout while the native index is `laps * 4 + checkpoint` (the `checkpoint_seen` layout); say
   both so a reader does not look for a 16-stride.

Advisory: `classic_hud_digit` throws on a value past 9 inside `observe_update`, which the app calls
every update; the split arithmetic cannot produce one (minutes stay within 0-9 under the 10:00
limit, the borrowed digits are bounded) and `time_digits` are engine-written digits, so I accept
the refusal, but a comment at the call site saying why it is unreachable would spare the next
reviewer the derivation.

## 6. Regressions and gates

- Finish sequence: `brake-a` 6485-6500 (opponent first) and `trick-long-a` 6479-6500 (player
  first) 0 in all bands, section 3; the primary's `orig-primary-splits` (60 frames) and
  `orig-dragster-splits` (34 frames) re-scored from my build: 0 in all bands, whole 1,504 and 103,
  identical to `compare-consec-*-splits.txt`.
- Kept-frame sweep re-run: `brake-a` against `original-brake`, 274 frames, 0 in all four boxes,
  whole 2,374 - the record's number.
- 10:00 hold: item 5.4, 24 frames at 0.
- `gate_identity`: 11 gates cited from `6e0fad6`, inputs byte-identical at `c3fe841`, restore
  counts unchanged (379, 567, 493, 179, 181, 327, 801, 757, 781, 757, 801).
- The primary's `gates-c3fe841/` logs are from a run on `2298dc6` with two uncommitted record files
  (its `gates-run.log` says so and `gate_identity` refused there); the code is identical to
  `c3fe841`, and the primary re-ran `gate_identity` at the candidate. `lab-sanitize` and
  `app-sanitize` are unavailable on this host (ASan hangs before `main` on macOS 27.0 / Xcode
  26.1.1, reproduced by the primary with a hello-world); I did not run them and report nothing for
  them. The hosted Linux job covers them and must be green on the final tip before integration.
- ctest on `app-debug`: 23 of 23.

## 7. Readability (D-0003)

Names carry their units and origin: `checkpoint_display_countdown` (updates), `slot_times_` (minutes,
tens, seconds, tenths), `classic_hud_clock_digits`, `ClassicHudCellRequest::Kind`. Every literal
register in the comments is tied to an address I could open (`$81:C910`, `$81:CA38-CA61`,
`$81:CA6E`, `$81:EDE9`, `$81:F040`, `$81:F1B5-F1C6`, `$80:8220`) and the reason it matters is stated
next to it, which is the justified boundary D-0003 asks for. The split arithmetic is a free function
with the borrow chain spelled in the same order as the ROM and a test per branch, including the
unmeasured negative path labelled as such. The tests exercise the new code through the queue (the
crossing requests the field; no fixed offsets remain) and use no emulator fallback.

## Verdict

**Approve** `c3fe841`. Apply the should-fix items on top (1 is a small local change with a test;
2-4 are record corrections; 5 a comment). None needs a re-review of the mechanism; a confirming
re-read of the record edits and a green final-tip CI including the sanitizer presets on Linux is
enough.
