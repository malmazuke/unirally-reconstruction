# DRAGSTER-WINDOW-EFFECTS - independent review

## Verdict

**Approve.** Every claim I checked reproduced, from my own disassembly, my own
original captures, my own probe binary and my own gate runs. The four advisory findings
below are documentation and robustness notes, not defects in the shipped code;
none of them changes a measurement, a gate or a pixel.

## Candidate and build identity

- Branch `task/dragster-window-effects`, commit
  `788a877087d1ddec429516364968619b137b899e`, base `9411e59`.
- Reviewed in `.worktrees/dragster-window-review` on
  `review/dragster-window-effects` at that commit; `git status` clean apart from
  the ignored `artifacts/` and `local/` I created.
- ROM SHA-256 `a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`
  (2,097,152 bytes) at the path in `local/rom-location.txt`; audited core
  `e59bf88d4fc922c9...` as reported by the capture harness.
- Pack: I extracted my own
  `local/classic-crawler-two-tracks-v8.pack` and did not reuse the
  implementer's.
- Toolchain: the pinned `local/toolchain/cmake-3.31.10-darwin-arm64` and
  `ninja-1.13.2-darwin-arm64`.

## What I reproduced

### 1. Disassembly of the cited routines (claims 1-3)

I wrote my own 65816 disassembler
(`/private/tmp/claude-501/-Users-markfeaver-Projects-Unirally-Decompilation/9f414bf7-f823-4440-bd4b-f9e7c6fe0fb0/scratchpad/dis65816.py`)
and decoded each cited address directly out of the ROM. LoROM file offset =
`(bank & $7F) * $8000 + (address - $8000)`.

**Publication (`$80:868E-$8699`, `$80:86E9`, `$80:8786`).** Confirmed byte for
byte:

```
$80:868E  AD FD 11  LDA $11FD      (16-bit; A is wide here, SEP #$20 follows)
$80:8691  8D 62 43  STA $4362      A1T6L/A1T6H
$80:8694  E2 20     SEP #$20
$80:8696  AD FF 11  LDA $11FF
$80:8699  8D 64 43  STA $4364      A1B6
$80:86E9  AD FD 11  LDA $11FD      $80:8786 is the identical twin
$80:86EC  C9 4E DB  CMP #$DB4E
$80:86EF  D0 07     BNE $86F8
$80:86F3  A9 BE     LDA #$BE       mask without channel 6 ($8790: #$3C)
$80:86FA  A9 4E DB  LDA #$DB4E / STA $11FD   consume the request
$80:8702  A9 FE     LDA #$FE       mask with channel 6 ($879E: #$7C)
$80:87A9  8D 0C 42  STA $420C
```

`$FE ^ $BE = $40` and `$7C ^ $3C = $40`: the differing bit is exactly channel 6.
(`$80:87A7 AND #$DF` also clears channel 5 unconditionally, and `$80:87A2 LDX
$12D1; BEQ $87D9` can skip the `$420C` write altogether - neither affects
channel 6 in any captured DRAGSTER frame, and neither is claimed.)

**`$82:D572`.** `$82:D570 LDA #$04 / STA $4360` (DMAP6 = mode 4, four registers
written once) and `$82:D56B LDA #$26 / STA $4361` (BBAD6 = `$2126`, WH0..WH3).
Exactly as claimed.

**The family (`$83:E55C`, claim 2).** File offset 124252. The first 32 offsets
are `$0000, $0383, $0706, ...` with a constant difference of **899** (the
sequence breaks only at index 31→32). Members 0-24 at file offsets
688128-710602 (`$15:8000`-`$15:D7CA`) each have `$00` at byte 898; members 25-31
have `$FF` there, so they are not well-formed 898-byte tables, exactly as
R-0040 says. Entry 26's address is `$15:DB4E` - the sentinel. SHA-256 of the
25-member block is `238ff3fc38357b359e99ae3da0647fdf61cb4a6a3680b83faa1b3a8a6a4a2cf0`,
identical to the new pack entry. Member 3 hashes to
`33f19daed02ec2f9...` and member 18 to `b6fddc697a55984d...`, which are the
exact sizes, file offsets (690825, 704310) and digests of the frozen
`presentation.effect.go-window.v1` and `presentation.effect.winner-window.v1`
entries in both pack rule files. **Members 3 and 18 are the two R-0015 entries,
byte for byte.**

**The countdown driver `$83:E59C` (claim 3).** `$83:CD66 LDA $11C5 / BEQ /
$83:CD6B JSR $E59C` confirms it runs only while `$11C5 != 0`; `$82:D841 LDA
#$010E (270) / STA $11C5` confirms the start value. `$83:E59E CMP #$0005 / BPL`
is the `$0FF1 >= 5` gate. The dispatch is a two-level tree, and I decoded every
arm:

| `$11C5` | arm | member | site |
| --- | --- | --- | --- |
| >= 250 | `$E5D8`→`$E60D` | `5 + $1229` | `$E611`/`$E616` |
| 221-249 | `$E5D8`→`$E5FE` | 0 (`#$8000`) | `$E5FE` |
| 190-220 | `$E62A`→`$E65F` | `5 + $1229` | `$E663` |
| 161-189 | `$E62A`→`$E650` | 1 (`#$8383`) | `$E650` |
| 130-160 | `$E67C`→`$E6C1` | `5 + $1229` | `$E6C5` |
| 101-129 | `$E67C`→`$E6A2` | 2 (`#$8706`) | `$E6A2` |
| 70-100 | `$E6DE`→`$E759` | `5 + $1229` | `$E763` |
| 1-69 | `$E6DE`→`$E728` | 3 (`#$8A89`) if `$0300`, else 4 (`#$8E0C`) | `$E730`/`$E746` |

`$8383-$8000 = $0383 = 899`, `$8706-$8000 = $0706 = 2*899`, `$8A89-$8000 =
3*899`, `$8E0C-$8000 = 4*899`. The thresholds are exactly 250/221/190/161/130/
101/70. `$83:CCE7 LDA $0300 / INC A / AND #$01 / $83:CCED STA $0300` is the
per-frame parity toggle. The "hold at the line" block `$E78C-$E7BD` is skipped
only by the 1-69 arm, as recorded.

**The winner drivers.** `$83:EA19` (player, `$0F03`/`$0F07`) and `$83:EBB0`
(opponent, `$0F05`/`$0F09`) are structurally identical: index 0 means "first
update", which sets the life counter to `$0168` = 360 and takes member 7; the
life counter is decremented every update; `LDY $0300 / BEQ` advances the member
**only on a set parity**; `CMP #$0019 / LDA #$0007` wraps 25 back to 7, so the
cycle is members 7..24, length 18, two frames each. When the life counter is
zero the driver stops requesting (`$EA5D`, and `$EBDA` writes the sentinel
explicitly). `$83:EA19` bails at once if `$0F09` is non-zero and `$83:EBB0`
zeroes `$0F07`, so only the winner's driver runs.

**No pose test.** Nothing in `$80:868E-$87A9`, `$83:E59C-$E7C0`, `$83:EA19` or
`$83:EBB0` reads a rider pose. The selection is a function of `$11C5`, `$1229`,
`$0300`, `$0F03`/`$0F07` (or `$0F05`/`$0F09`) and `$0FF1` only.

**The one-frame delay,** confirmed directly rather than inferred: in my own
capture the winner driver writes `$C2B9` (member 19) to `$11FD` at `$83:EA58`
during frame **3453**, and `$80:8691` publishes `$15:C2B9` at frame **3454**.

### 2. Claim 4, re-derived from my own capture

```sh
cd "/Users/markfeaver/Projects/Unirally Decompilation/.worktrees/dragster-window-review"
python3 tools/project.py access capture \
  --manifest tests/manifests/replay/race-crawler-dragster-12000-review-release-3213-fields.json \
  --out "<scratch>/caps/rev3213" --from-frame 1300 --to-frame 3700 \
  --watch-pc 0x808691 --watch-address 0x0011FD --watch-address 0x000300 --watch-address 0x0011C5
```
`status=passed`, `access.json sha256 de0e7ce8301ec45f`, frames 1300-3700,
43,223,727 instructions, ring not overflowed.

I built my own probe against the candidate's `libunirally_presentation.a` that
prints `dragster_window_table_index` for a serialized state, and ran it over all
**2,147** native states of the release-3213 sweep
(`.worktrees/dragster-palette-review/artifacts/dragster-palette-review/sweep`).

- **1,922 of 1,922 frames from 1533 to 3454 agree**, including every frame on
  which neither side draws. Frame 3679 agrees that there is none.
- The only 224 disagreements are frames **3455-3678**, all result loading.

The countdown boundaries my capture produced, independently of R-0040, are
exactly the recorded ones: none before 1334; 6 at 1334-1354; 0 at 1355-1383; 6
at 1384-1414; 1 at 1415-1443; 6 at 1444-1474; 2 at 1475-1503; 6 at 1504-1534;
4/3 alternating 1535-1603 (4 on odd frames); none from 1604; banner 7 at 3215
advancing every second frame to 19 at 3454.

**The result-loading exception is genuine, and stronger than the record claims.**
From the same capture's HDMA log, `$420C` is written on frames 1334-3454 and
**never again** up to 3700, and channel 6 (bit 6) is set on exactly frames
**1334-1603** and **3215-3454**. `$80:8691` executes on frames 1334-3454 and on
no frame after that, so the race vblank really does stop on loading update 1 -
and because nothing rewrites `$420C`, channel 6 stays enabled with `A1T6` still
pointing at member 19. The original therefore keeps showing member 19 through
the result-loading fade, which is precisely what native does by freezing the
loading-start frame. The 224 "differences" are an artifact of scoring against a
pointer the original stops republishing, not a visual divergence, and this is
consistent with the loading frames *improving* in the pixel comparison. It is
not masking anything.

### 3. Claim 5, on frames I chose

I rebuilt the "before" side myself: `git archive 9411e59 src CMakeLists.txt`
into a fresh directory (`diff -r` against the implementer's baseline tree:
identical) and compiled `live_presentation_runner` from those sources with the
system compiler. "After" is the candidate's `build/app-debug`
`live_presentation_runner`. Packs: `local/classic-crawler-two-tracks-v7.pack`
for before, my own v8 for after. Rectangle (0, 28, 256, 196); "changed" means
the two renders differ there.

| frame | what it exercises | changed | before matching original | after |
| --- | --- | --- | --- | --- |
| 1533 | countdown transition, old drew **nothing** | 9,278 | 0 | **9,278** |
| 1534 | the same | 9,278 | 0 | **9,278** |
| 1550 | GO letters, old drew nothing | 8,846 | 0 | **8,846** |
| 1560 | GO letters | 8,881 | 0 | **8,881** |
| 1570 | GO letters | 9,061 | 0 | **9,061** |
| 3215 | first banner frame, old drew nothing | 2,810 | 0 | **2,810** |
| 3216 | banner | 3,719 | 0 | **3,719** |
| 3322 | banner, old drew the **wrong shape** | 6,165 | 0 | 5,688 |
| 3450 | banner | 4,034 | 0 | **4,034** |
| 3452 | accepted contract frame | **0** | - | rect mismatch 653 before and after |
| 3453 | accepted contract frame | **0** | - | rect mismatch 653 before and after |
| 3455 | result loading | 2,601 | 1,162 | 1,439 |
| 3460 | result loading | 2,601 | 1,162 | 1,439 |

Rectangle mismatch falls from 11,496 to 2,218 at 1533, 9,532 to 686 at 1550,
3,914 to 1,104 at 3215, 6,744 to 1,056 at 3322 and 5,040 to 1,006 at 3450. The
3215 and 3450 figures (2,810 of 2,810 and 4,034 of 4,034) are exactly the ones
R-0040 quotes. **Frames 3452 and 3453 are bit-identical between the two
implementations** and keep the accepted 653, so the accepted contract frames are
untouched.

Frame 3322 - R-0037's "wrong shape" case, original 2,810 against native's fixed
5,001 - is the only one of my frames that does not reach 100%: 477 of 6,165
changed pixels still differ. That is not a selection error: my capture publishes
member 7 at 3322 and native returns 7 (and 24 at 3321, 7 at 3323, both matching).
R-0040's own banner row (164,753 changed, 164,276 matching) reports the same
477-pixel residual, so it is disclosed, and it sits inside the already declared
rider-art differences.

### 4. Content and reproducibility

- `python3 tools/project.py content pack --rules tests/manifests/content/classic-crawler-two-tracks-pack.json --out <scratch>/v8-{a,b}.pack`
  run twice in separate processes: both
  `3d1e642be851a139407f4159aa1aa1b171ac0df550e5a96c8751d0660aa9672a`,
  `entry_inventory 56 logical entries`, `status=passed`. **Reproducible.**
- `python3 tools/project.py frontend run --track zoom-zoo --rom "$(cat local/rom-location.txt)" --pack local/classic-crawler-two-tracks-v8.pack --preset app-debug --updates 20 --hidden`
  → `classic_pack (required): validated existing pack ...; ROM was not opened`,
  `status=passed`. The binary's own profile, entry-count and per-entry digest
  checks accept my pack, which confirms its identity against the task record.
- Provenance: the new entry is a single `raw` piece at file offset 688128,
  length 22,475 - exactly the 25-member block I derived from `$83:E55C`. The
  manifest diff changes only `profile_id` and appends that one entry, so every
  v7 entry is unchanged by construction; `content_pack.cpp` carries its digest
  in `zoom_required` (30 → 31 entries).
- The DRAGSTER v1 pack is unchanged and does not carry the family, so
  `content.window_tables` is empty and `render_dragster` keeps the accepted
  pose-keyed placement. I confirmed this end to end: the whole historical matrix
  runs on `local/classic-crawler-dragster.pack` and reproduces the accepted
  numbers exactly (below).
- No accepted expectation or contract file changed:
  `git diff --name-only 9411e59..788a877 | grep -E "freeze|expected|reference"`
  is empty.
- No ROM-derived bytes entered tracked files: the added diff contains no byte
  arrays, only offsets, sizes and digests.

### 5. Regressions

All run in this checkout, with the pinned toolchain and fixtures copied inside
the checkout (`artifacts/dragster-window-review/fixtures`, verified `diff -r`
identical to the accepted `m4-16/broad-4f8aaad` originals).

| Gate | Command | Result |
| --- | --- | --- |
| five presets | `python3 tools/project.py build --preset P` then `ctest --test-dir build/P` | build rc=0 and **23/23, 0 failures** on lab-debug, lab-release, lab-sanitize, app-debug, app-sanitize |
| DRAGSTER historical matrix | my adapted `hist.sh` (from `<scratch>/gates/hist2.sh`, repointed at this checkout, every path quoted, fixtures inside the checkout, `test -f`/`test -d` preconditions that `exit 2` rather than skip) | **TOTAL COMMANDS: 20, ANY FAILURE: 0** |
| accepted presentation contracts (v1 pack) | the same script | winner **36/697/279/445/653/962/961**, loser **1,073** - identical to the accepted figures |
| M4-16 ZOOM ZOO primary and three DRAGSTER frozen originals | my adapted `gates2.sh` | **TOTAL COMMANDS: 4, ANY FAILURE: 0** |

The gate outputs carry the claimed identities exactly:

| Gate | frames | `rows_sha256` | restores |
| --- | --- | --- | --- |
| M4-16 ZOOM ZOO primary (`zoom-zoo-playable-primary-v11.freeze.json`) | 1376-7600 | `b4a34af722b266931bcf48815ed96ed39236f8331e6fb2b5ecc96c574eeb36aa` | **757** |
| DRAGSTER frozen original, primary | 1328-3900 | `dac612cc71a1a2155e4f47bb3e86e1aee9e61dbb55a1fad74aed2171df2a2469` | 379 |
| DRAGSTER frozen original, random-1 | 1328-4340 | `7df2f728949c6ed95cb5c57156dafec55af5ccd3db6e42374f3ed08d13aff0cd` | 567 |
| DRAGSTER frozen original, reversal | 1328-5000 | `f142311ef5a0fff40edd77427ae0f4f0acaf35dcf8122b5b2ab302895d6ab17e` | 327 |

I ran all three DRAGSTER frozen originals rather than the two required.

### 6. My own mutation probes

To check that the new ROM-free test is not vacuous I mutated the recovered rule
in `src/core/presentation.cpp` myself, eight ways I chose, rebuilding
`presentation_tests` and running `ctest -R presentation` after each, then
`git checkout --` the file
(`artifacts/dragster-window-review/mutations/`). Each probe aborts if its
pattern is absent or the build fails, so a mutation that never applied cannot be
reported as caught.

| Mutation | Result |
| --- | --- |
| first threshold 250 → 249 | caught |
| GO parity flipped (`even → 4`, `odd → 3`) | caught |
| result-loading freeze removed | caught |
| banner first member 7 → 8 | caught |
| banner cycle length 18 → 17 | caught |
| banner start `frame + 1 - since` → `frame + 2 - since` | caught |
| race setup frame 1334 → 1333 | caught |
| transition member 6 → 5 | caught |

**Eight of eight caught.** The tree was clean afterwards and lab-debug returned
to 23/23.

## Findings

### A. Advisory - R-0040 overstates the opponent-won gap by 17 frames

R-0040's "Not established" section says native draws no banner where the
original still shows one "in `lose-a`, display frames 3334-3576". My own capture
of the same scenario
(`race-crawler-dragster-12000-release-3000-3299-fields`, frames 3100-3860,
watching `$80:8691`) shows channel 6 enabled on frames **3216-3559** only: the
race vblank runs 3100-3559 and stops there (result loading), so the original's
banner ends at 3559, not 3576. The implementer's own `lose-a/access.json`
agrees with mine (`pc frames 1334..3559`). The real gap is display frames
**3334-3559**, 226 frames of 344. Reproduce with the capture above, then group
the `$80:8691` rows by frame.

This makes the residual slightly *smaller* than recorded; it does not change any
measurement or gate.

### B. Advisory - "the original's race vblank stops" is right, but the reason matters

The task record and R-0040 justify the 224-frame result-loading difference as
"the original's race vblank no longer runs and native holds update 1's member by
design". That is correct (`$80:8691` executes on no frame after 3454), but the
records stop short of the fact that makes the design choice *right* rather than
merely declared: because nothing rewrites `$420C` after 3454, channel 6 stays
enabled with `A1T6` still pointing at member 19, so the original also keeps
showing member 19 for the whole fade. Worth a sentence in R-0040, since as
written a reader could reasonably conclude native draws a window the original
does not.

### C. Advisory - the 360-frame banner bound is unreachable, and off by one for one parity

`dragster_window_table_index` accepts `2 <= since <= 361`, and
`presentation_tests.cpp` pins it with `won(3574, 361)` / `won(3575, 362)`. Two
observations:

1. Real states cannot reach it. `deserialize_movement_state` rejects
   `player_finish_delay > 240` (`movement.cpp:755`) and the opponent path is
   bounded by 120, so `since` is at most 240. The bound is defensive only.
2. For the parity the captured player-won race actually takes, the ROM driver
   runs one update longer than the bound allows. When `$0300` is clear on the
   driver's first update, `$0F03` is not stored (the `BEQ $EA4E` path skips
   `STA $0F03`), so the next update re-enters the "first update" arm and resets
   `$0F07` to 360 again; the driver's last drawing update is then u=361, i.e.
   `since` 362. With `$0300` set on the first update the bound is exact.

No action needed while the field is capped at 240, but the test's `won(3575,
362) == none` encodes behaviour the ROM does not have, so it should not be
treated as a recovered fact.

### D. Advisory - unsigned wraparound on states that cannot occur

`const auto first_driver_frame = frame + 1U - *since;` wraps if `*since >
frame + 1` (for instance a hand-made state with `frame = 0` and
`player_finish_delay = 100` in `FinishDelay`). The arithmetic is defined, the
result stays in `[7, 24]` and `dragster_window_table` bounds-checks the index,
so there is no undefined behaviour and no out-of-range read; real states always
have `frame >= 1334`. Noted only for completeness.

## Judgement on the stated gap

The question put to me: the opponent-won banner past its first 120 updates draws
nothing where the original still shows one. Is that correctly scoped and
recorded, or should it block?

**Correctly scoped and recorded; it should not block.** My reasons, in order of
weight:

1. **It is not a regression.** The previous implementation drew the winner
   window only on the pose pair `(0x04fe, 0x037c)`, which an opponent-won race
   does not reach, so those frames drew nothing before this change either. What
   the change does is add a banner for display frames 3216-3333 that was
   previously absent; the remaining frames are unchanged. The accepted loser
   contract (`visual:stable-loser-result-3800`, 1,073) reproduces exactly, and
   the loser fixtures use the v1 pack, which is untouched.
2. **The boundary is a property of the serialized state, not of the rule.** The
   rule itself is exact - my capture confirms the opponent-won selection frame by
   frame where native draws at all (first member 8 at display frame 3216,
   advancing every second frame). Native simply cannot locate the opponent's
   finish frame once `finish_animation_countdown[1]` has run out, because that is
   the only field in the 742-byte state that carries it. That is an honest
   limitation of the state format, and it is the kind of thing a state version
   bump fixes, not a mistaken recovery.
3. **It is recorded where a fresh agent will find it**, at three levels of
   detail: the task record's "Still missing" line and its explicit "first 120
   updates of an opponent-won one" capability statement, R-0040's "Not
   established" section with the cheapest next experiment spelled out
   (`finish_time_centiseconds[0] - finish_time_centiseconds[1]` against the
   `frame & 1` term, with the state-version fallback named), the handoff's "Exact
   next experiment", and `docs/STATE.md`. The residual is bounded, named and
   reproducible.
4. **The blast radius is a scenario the product does not yet claim.** DRAGSTER's
   declared presentation omissions already cover the surrounding result-screen
   behaviour, and no accepted contract, gate or expectation depends on the
   missing frames.

The one correction it needs is finding A: the recorded extent (3334-3576) is
wrong; the original's banner ends at 3559, so the gap is 3334-3559. R-0040
should be amended, but that is a one-line documentation fix, not a reason to
return the candidate.

## Reviewer checklist

- **Changed baselines:** none. No `*.freeze.json`, `*.expected.json` or
  `*.reference.json` changed; the accepted presentation contract numbers
  reproduce exactly; the pack manifest change is additive (profile id plus one
  entry).
- **Weakened guards:** none. The only removed assertion text is a `require()`
  message being *improved* to name the failing index. `render_dragster` gained a
  size check for the family, and `dragster_window_table` checks length, index
  range and the run terminator.
- **Masked skips:** none found. The candidate's gate scripts and my adapted ones
  use `test -f`/`test -d` preconditions that `exit 2`; no gate step is guarded by
  a conditional that can pass silently. The new test's mutation-survivor history
  is recorded honestly in the task record.
- **Undefined arithmetic:** none (see finding D for a defined-but-nonsensical
  wraparound on impossible states).
- **Readability:** the rule reads as the ROM does - one named constant per ROM
  fact (`dragster_race_setup_frame`, `dragster_window_countdown_start`,
  `dragster_window_transition_index`, `winner_window_first/cycle/frames`), each
  carrying the address it came from, and a helper whose name says what it
  returns. The v1/v8 fork is explicit and commented. The `>= 7` / `<= 6` split
  between the pre-rider and post-rider draw sites inherits the accepted M3-02
  placement and is documented as such.
- **Content in tracked files:** none. Only offsets, sizes and digests.
- **Tests exercise the new code, not an emulator fallback:** the new block is
  ROM-free - it builds a synthetic 25-member family in memory, asserts the index
  at every band boundary, both GO parities, the banner start, the loading freeze
  (including an odd-offset frame that would step the banner without it) and the
  result screen, and then renders and mutates individual family members to prove
  the *chosen* member is the one drawn.

## Not assessed

- I did not re-run the implementer's own 19 mutation probes; I ran eight of my
  own instead (all caught), so the *count* 19 is taken on the record's word even
  though the property it claims is independently established.
- `$1229`/`$0BA7` (whether the transition member is ever anything but 6),
  `$0FF1`, `$77:074B` and `$77:0750`: recorded as "not established" in R-0040 and
  left there. My disassembly confirms the `$77:074B == 2` arm exists and goes
  straight to the digit's own table, which one-player DRAGSTER never takes.
- ZOOM ZOO's own window content: explicitly out of scope.
- Live play by a human, which remains a standing item for DRAGSTER.
- Hosted CI: I read the recorded run reference rather than dispatching my own.
