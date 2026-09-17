# DRAGSTER-ORDINARY-CONTROLS - independent review

- Reviewer: fresh Claude Opus 5 subagent, no inherited conversation, isolated
  checkout `.worktrees/dragster-controls-review` on `review/dragster-ordinary-controls`.
- Candidate under review (immutable): `task/dragster-ordinary-controls` at
  **`16b8daf02bf0ea8157d5f8000c606a4af0ee2b09`**. The gate candidate the
  implementer declares is `6534241`; `16b8daf` adds records only (verified:
  `git diff 6534241..16b8daf --stat` touches
  `tasks/DRAGSTER-ORDINARY-CONTROLS.md` and nothing else). Every check below
  was built and run at `16b8daf` itself, not at `6534241`.
- Review date: 2026-09-17/18 (session start 18:33Z).
- **Verdict: approve.** No blocking defect found. Five low-severity
  observations and nits are listed below; none of them needs a change before
  integration, and the parked live-play criterion is unchanged.

## Identity of what was reviewed

| Item | Value |
| --- | --- |
| Source | `16b8daf`, clean tree during every run (`dirty=0` recorded by the gate script) |
| ROM | `a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e` (PAL, 2,097,152 bytes, no copier header) |
| bsnes core | `e59bf88d...` - enforced by `dragster_playable_reference`, which refuses a different ROM/core |
| Content pack | extracted by the reviewer from the ROM: `b75539a0adfe13c3cb2541631b0b7107d8ab41a563bed56fdc2096fd8f4fd442` (55 entries), matching the declared prefix |
| DRAGSTER v1 pack | `5c1fc5b00747621ccd2c0a1f6f34cf6ba290c8808e6bb1d92b8c0ad4b0df1529` |
| Toolchain | `python3 tools/project.py doctor` passed; isolated cmake 3.31.10 / ninja 1.13.2, Apple clang 17.0.0 |
| Presets built | `lab-debug`, `lab-sanitize`, `lab-release`, `app-debug`, `app-sanitize`, all `status=passed` |

Reports for everything below are in this checkout under ignored
`artifacts/review/` (`doctor.json`, `build-*.json`, `historical/`, `gates/`,
`withheld-*`, `repro-*`, `*-compare.json`, `fuzz/`).

## 1. Original evidence - reproduced, not read

### 1a. Frozen cases are original-only and reproduce bit for bit

`tools/unirally_lab/native/dragster_playable_reference.py` was read in full: it
loads only the bsnes core and the ROM, replays the frozen menu manifest, then
presents the case's controller-0 buttons. No native binary, seed or state is
involved, and it refuses a ROM/core whose SHA-256 differs. The captures are
therefore genuinely original-only.

Three frozen cases were re-captured from scratch in this checkout and compared
against **both** of the implementer's captures (`-a` and `-b`):

```sh
python3 -m tools.unirally_lab.native.dragster_playable_reference --core local/emulators/bsnes/bsnes/out/bsnes_libretro.dylib \
  --case tests/manifests/native/dragster-ordinary-primary.case.json --horizon 3900 --out artifacts/review/repro-primary
# also: dragster-ordinary-random-3.case.json --horizon 4140, dragster-ordinary-reversal.case.json --horizon 5000
```

| Case | Frames | Every per-frame WRAM/SRAM/video hash identical to `-a` and `-b`? |
| --- | --- | --- |
| primary | 1159-3900 | yes |
| random-3 (the withheld case) | 1159-4140 | yes |
| reversal | 1159-5000 | yes |

Aggregate digests reproduced exactly, e.g. primary
`wram f05b4fe7983d5848...`, `sram 7d3e7bf03c56e6aa...`, `video 767bb88ca5d31fff...`.

Both tracked freeze contracts were then re-validated end to end against the
reviewer's own fresh captures (not the implementer's), which re-derives
`rows_sha256` and the whole frozen inventory from the ROM:

```sh
python3 -m tools.unirally_lab.native.dragster_playable compare \
  --reference artifacts/review/repro-primary --repeat artifacts/review/repro-primary-b \
  --contract tests/manifests/native/dragster-ordinary-primary.freeze.json \
  --binary build/app-debug/src/core/zoom_zoo_runner --pack local/classic-crawler-two-tracks-v7.pack \
  --out artifacts/review/primary-compare.json
```

- primary: `status: passed`, **379 fresh-process restores** (the implementer's
  declared count).
- random-3: `status: passed`, **493 restores** (declared count).

So the tracked expectations were not hand-written: they are exactly what the
original produces on this reviewer's machine.

### 1b. A withheld sequence the implementer did not use

A new case was written for this review
(`artifacts/review/withheld-a/reference.json` carries the timeline; the case
file is reproduced in full at the end of this document). It deliberately
exercises the paths the task is about and that the accepted gates never did:

- countdown held with **X and Left** (tests the `$83:E7A2-E7BF` A/X release),
- **B while riding** at speed, held 30 updates and tapped, four times,
- **X rolls** (tricks) in the air and while reversing, **L and R** rotations,
  one of them held through a landing,
- **Y brake while moving**, then Y alone,
- a **190-update Left reversal** with B and X during it,
- Select, Up, Down, A, and a run to the finish.

Captured twice: both runs produced identical digests
(`wram d3f5738531eba400...`, `sram ecdfe3d6c5eb5956...`,
`video b3a2fedc2dd46abc...`), so the original is deterministic here too.

Native against that original, over the whole race:

```sh
python3 -m tools.unirally_lab.native.dragster_playable explore --reference artifacts/review/withheld-a \
  --binary build/app-debug/src/core/zoom_zoo_runner --pack local/classic-crawler-two-tracks-v7.pack
```

```
"original_rows": 3473, "native_rows": 3473, "native_exit": 0, "native_error": "",
"first_divergence": null,
"events": {"finish_frames": [3673, 3214], "loading_frame": 3914,
           "guard_violations": {}, "outcome": "player_lost",
           "first_visible": 4022, "complete": true}
```

**No divergence anywhere**: all 3,473 updates (1328-4800) of the declared
742-byte projection match, the race completes, the result loads at 3914 and the
first new picture is 4022 = 3914 + 108, as R-0038 predicts for race mode 0.

The full gate was then run on the same case (frozen from the two identical
captures first):

```sh
python3 -m tools.unirally_lab.native.dragster_playable freeze  --reference artifacts/review/withheld-a --repeat artifacts/review/withheld-b --out artifacts/review/withheld.freeze.json
python3 -m tools.unirally_lab.native.dragster_playable compare --reference artifacts/review/withheld-a --repeat artifacts/review/withheld-b \
  --contract artifacts/review/withheld.freeze.json --binary build/app-debug/src/core/zoom_zoo_runner \
  --pack local/classic-crawler-two-tracks-v7.pack --out artifacts/review/withheld-compare.json
```

`status: passed`, `rows_sha256 4a7c91f13e27cbd7...`, **255 fresh-process
restores**, second fresh initialization identical, restart from the final result
state identical.

The gate itself was read rather than trusted: `compare()` runs the native binary
with `--start classic.crawler.dragster --content-pack --inputs` only, so nothing
in the compared path is fed from the emulator; there is no emulator fallback for
supposedly native logic.

## 2. Source claims - disassembled independently

A 65816 disassembler was written for this review and run against the ROM
(LoROM, file offset `(bank & 0x7F) * 0x8000 + addr - 0x8000`). Every cited
address was checked; all of them say what R-0038 says.

| Claim | What the ROM shows | Verdict |
| --- | --- | --- |
| Speed-limiter bound `$1281` from race mode `$77:074B`, `$83:CC59-CC7C` | `LDA $77074B / AND #$FF / CMP #$01 / BPL` -> mode 0 arm `LDA #$60`, mode>=1 arm `LDA #$48`; each `SEC / SBC $1283 / BPL / LDA #$60(48) / STA $1281` | confirmed exactly, including the fall-back to the unsubtracted constant |
| Laps `$0D15` | `$82:DB96` `LDA $77074B ... CMP #$01 / BMI -> LDA #$01`, then `INC / STA $0EFB / STA $0EFD` (= 1 lap + 1); `$81:D670 LDY $0EFB / $81:D673 STY $0D15` | confirmed exactly |
| `$0D15` skips the initial crossing | `$81:8139 LDA $0D15 / SEC / SBC $0EFB,y / DEC / BPL / JMP $8187` guards the slot store `STA $770755,x`; the same test at `$8172-8183` guards the display store `STA $7707BF,x`; `$8197-81A0` guards the final-lap `CMP #$01` | confirmed |
| Playfield geometry `$81:A304-A51B` | `$A304` reads `$7F000D` (decoded track byte 13): 0 -> `JMP $A4C1`, `$40` -> `JMP $A445`; six other arms exist. `$A4C1-A4FD`: X=`$0400` (1024), Y=`$0010` (16), `$0D4F=$FFFF`, `$0D51=$03FF`, `$0425=$FFCF` (-49), `$0427=$0100` (256), `$03F1=0`, `$03F3=$FFE8` (-24), `$03F5=$0019` (25). `$A445-A481`: X=`$0100` (256), Y=`$0040` (64), `$0D4F=$3FFF`, `$03F1=2`, `$03F3=$FFA0` (-96), `$03F5=$0064` (100), `$0425=$FF3C` (-196), `$0427=$0400` (1024). Both branch to the shared tail `$A4FF`, `RTS` at `$A51B`. | confirmed exactly, including the block's end address |
| B/Y mapping `$82:AAE2-AAFB` | `BIT #$80 / STY $0331 : STZ $0331` then `BIT #$40 / STY $0325 : STZ $0325` - pad high byte bit 7 = B -> jump `$0331`, bit 6 = Y -> brake `$0325` | confirmed exactly |

Every one of the nine values in
`tests/manifests/native/dragster-race-guards.reference.json` was checked
against the disassembly above and matches
(`$03F1`=0, `$03F3`=65512, `$03F5`=25, `$0425`=65487, `$0427`=256, `$0D15`=2,
`$0D4F`=65535, `$0D51`=1023, `$1281`=96).

Fuzz corrections (seven were claimed; all seven cited sites were checked, not
three):

| Correction | Cited site | ROM |
| --- | --- | --- |
| Countdown releases A and X | `$83:E7A2-E7BF` | `LDA #$0001 / STA $0325 / STA $0327 / LDA #$0000 / STA $0331 / STA $0333 / STA $0321 / STA $0323 / STA $031D / STA $031F / RTS` - both brakes forced, both jumps, both X and both A cleared. Confirmed, and `$E7BF` is exactly the last byte of `STA $031F`. |
| Supported orientation at target | `$83:F02D-F034` | `LDA $00 / CMP $0F53 / BNE $F037 / JMP $F09E` - target equal to the stored orientation skips the store. Confirmed exactly. |
| A zero roll step completes | `$82:959B`, flip `$82:9439` | `$9439 EOR #$FFFF` one's-complements a negative step, so `-1 -> 0`; `$959B LDA $1249 / BNE $95A3 / JMP $9636` sends a zero step to the same completion target the decrement reaches. Confirmed. |
| Idle wobble below reference 32 | `$82:A1F2-A1FF` | `LDA $0F83 / CMP #$0020 / BMI $A20F / CMP #$003A / BMI $A21B / BRA $A20F` - below 32 does not reach the `LDA #$FFFF / STA $0F77` reset at `$A201`. Confirmed. |
| Moving brake uses the drive routines | `$82:9909-9945` | `LDA $0FA9 / BMI / BEQ / CMP #$0010 / BMI` then `JSR $AA10` (the *opposite* drive routine) `/ STZ $0F5F /` reload `/ BMI -> LDA #$0000 / STA $0FA9 / JMP $9A52`; mirrored with `CMP #$FFF0` and `JSR $A9B3`. Confirmed exactly, including the zero clamp and the early return. |
| Roll bounce keeps velocity | `$82:A9B3`/`AA10`, store guard `$82:AA42-AA49` | `LDY $0FF9 / LDA $042B,y / BNE +5` skips only `STA $0FA9`, then throttle accumulation continues at `$A9EF`/`$AA4C`. Identical mirror at `$AA42-AA49`. Confirmed exactly. |
| Charge latch | `$82:9995-99EF`, `$82:99F1-9A49` | `$9995 LDA $0F61` (step) `/ BEQ $99BE`; with a step the latch `$0F63` ends 0 on both arms; `$99BE LDA $0F5F` continues to the throttle test. `$99F1 LDA $0F65` (previous brake) `/ BEQ $9A4C` (nothing changes) `/ LDA $0F5F / BEQ $9A2B`, else launch `ADC $0FA9 / STZ $0F5F / LDA #$0100 / STA $11F1` then clear the latch. Confirmed. |

## 3. No regression - re-run in this checkout

### DRAGSTER historical matrix (the accepted legacy `update_movement` path)

The implementer's script was copied from
`.../scratchpad/gates/hist2.sh`, its `cd`, `H=` and the two `--fixtures` paths
re-pointed at this checkout (the fixtures were copied from the gate worktree
into `artifacts/review/fixtures/`), and run against the **v1 DRAGSTER-only
pack** `local/classic-crawler-dragster.pack`:

```
== movement differentials (M4-12 family) ==  6 commands, rc=0
== movement restores ==                      4 commands, rc=0
== full-race finish (M3/M4-01 family) ==     8 commands, rc=0 (incl. opponent-first, both presets)
== presentation ==                           2 commands, rc=0 (winner and loser visuals)
TOTAL COMMANDS: 20
ANY FAILURE: 0
```

The legacy path, its `URMV` formats and its presentation expectations are
therefore unchanged and still pass with the DRAGSTER-only pack. No historical
manifest or freeze file is modified by the diff (`git diff abc38e6..16b8daf --
tests/manifests/` adds `dragster-ordinary-*` and `dragster-race-guards` only).

### Presets and synthetic suites

| Command | Result |
| --- | --- |
| `project.py test --suite synthetic --preset lab-debug` | passed, 47.4 s |
| `... lab-sanitize` | passed, 41.1 s |
| `... app-debug` | passed, 40.0 s |
| `... app-sanitize` | passed, 41.7 s |
| `ctest --preset {lab-debug, lab-sanitize, app-debug, app-sanitize}` | all rc=0 |

### M4-16 ZOOM ZOO gates

| Command | Result |
| --- | --- |
| `zoom_zoo_playable compare` against `zoom-zoo-playable-primary-v11.freeze.json`, `build/app-debug/.../zoom_zoo_runner` | passed, **757 restores**, 299 s |
| same with `build/app-sanitize/.../zoom_zoo_runner` | passed, **757 restores**, 799 s |
| `zoom-zoo-playable-idle-late-start-v11.freeze.json`, app-debug | passed, **801 restores**, 301 s |
| gate script total | `ANY FAILURE: 0` |

The M4-16 primary gate passes unchanged with the shared engine now generalised,
which is the main regression risk of this change (the ZOOM ZOO constants 4,
`$3FFF`, -96/100, -196/1024, laps 4 and result update 107 all became
scenario/geometry values; each one was re-derived above from the `$40` arm and
matches).

### Independent ordinary-controls probe

The two aborts the task was opened for were reproduced as controls, not taken
on trust. Each mask was held with `zoom_zoo_runner --start
classic.crawler.dragster` for 2,600 updates from native initialization:

| Mask | Exit | Rows | Outcome |
| --- | --- | --- | --- |
| left only | 0 | 2601 | does not finish (expected) |
| right only | 0 | 2601 | stable result, `result_updates=226`, player finished |
| **right + B (jump while riding)** | **0** | 2601 | stable result, player finished |
| right + Y (brake while riding) | 0 | 2601 | holds the brake, does not finish |
| right + X / L / R / A / Up / Down / Select | 0 | 2601 | stable result, player finished |
| neutral | 0 | 2601 | does not finish (expected) |

No abort on any of them. On `abc38e6` the task records `left` and `right+b` as
hard aborts; both now run a complete race.

### Abort fuzz with fresh seeds

An independent abort fuzz was run on seeds **5001+**, outside the implementer's
1-3000 range:

```sh
build/lab-release/src/app/dragster_fuzz_runner --content-pack local/classic-crawler-two-tracks-v7.pack \
  --first-seed 5001 --seeds 400 --races 3 --max-updates 40000 --failure-cases artifacts/review/fuzz/failures
```

```
summary seeds 400 updates 4902160 completed races 1200 pause restarts 12635 renders 102506 aborts 0
```

**0 aborts** over 4.9 M updates and 1,200 complete races, in 113 s, with no
failure case written. The renders exercise the result screen, so the recovered
result-font digits are covered here too.

## 4. Product change: the v1 DRAGSTER-only pack can no longer play DRAGSTER

Judged **acceptable and correctly documented**, and the refusal is clear and
safe. Verified by running it:

| Path | Observed |
| --- | --- |
| App binary with the v1 pack | `Unirally launch failed: DRAGSTER needs the two-track content pack for jumps, brakes, reversal and tricks; create it from your ROM with: python3 tools/project.py frontend run --track dragster --pack local/classic-crawler-two-tracks.pack --rom PATH`, exit 1, before any gameplay or window work |
| Launcher, v1 pack, two-track pack beside it | `[ passed] dragster_two_track_pack ... using .../classic-crawler-two-tracks-v7.pack; ROM was not opened`, then a normal 200-update run (frame 1528, player x 1088) |
| Launcher, v1 pack, no two-track pack, no `--rom` | `[missing] dragster_two_track_pack (required): DRAGSTER's jumps, brakes, reversal and tricks need the two-track pack; pass --rom PATH once to create local/classic-crawler-two-tracks.pack beside the DRAGSTER pack`, `status=failed`, nothing launched, ROM never opened |

The refusal names the remedy command exactly, never silently replaces an
existing pack, and never opens the ROM unless asked. The upgrade path is
covered by a new tooling test that also exercises the corrupt-two-track-pack
case (`tests/tooling/test_frontend.py`).

Documentation is accurate and honest: README "What works today" says DRAGSTER
"needs the two-track content pack (DRAGSTER ordinary controls, pending
independent review)"; `docs/STATE.md` records the regression that prompted the
task, the recovery, and that live play by the user is still due;
`docs/BUILD_AND_VALIDATION.md` adds the full command set and states that the
legacy commands are unchanged; `src/app/README.md` documents the upgrade,
the refusal, the DRAGSTER controls and the dropped opposing directions.

## 5. Checklist

| Item | Finding |
| --- | --- |
| Changed baselines | None. No existing manifest, freeze or reference file is modified. Both tracked DRAGSTER freeze contracts were re-derived from the reviewer's own ROM captures and match. |
| Weakened comparisons/guards | Three, each justified and documented - see findings 1-3 below. The removed M4-16 hold/rotation restore bound is justified by original evidence (verified independently, below). |
| Masked skips | None. No `skip`/`xfail`/disabled markers were added; every check above ran to completion. |
| Undefined arithmetic | None found; the camera/visibility rewrite actually removes a signed-multiply path (`dx*4`) in favour of an explicit `uint16 -> unsigned` shift with the same low 16 bits. `add_word`/`negative` helpers are used consistently for wrapping. |
| Readability (D-0003) | Good. New code uses named concepts (`ClassicRaceScenario`, `TrackGeometry`, `race_adjustment_limit`, `with_physical_dpad`, `drive_velocity`), documents units and provenance, and carries ROM addresses plus R-0038 links at each recovered behaviour. Literal register-level translation stays inside small helpers. `tests/native/dragster_race_tests.cpp` is ROM-free and pins the geometry, scenario, state identity and restore bounds with the same addresses. |
| ROM-derived bytes in tracked files | None. The diff adds no binary files; the new freeze contracts contain only digests, frame numbers and event frames, and the case files only button timelines. |

### The removed M4-16 hold/rotation restore bound

The deleted clause is
`(held>0 && static_cast<unsigned>(held)>roll.held_rotations)` in
`deserialize_zoom_zoo`. It is a genuine weakening, so it was checked against the
originals directly rather than against the claim. Every frozen DRAGSTER original
was decoded through `classic_race_layout` and scanned for states the old bound
would have rejected:

| Original | States with `held_updates > 0` and `> held_rotations` |
| --- | --- |
| primary, random-1, random-2, random-3, reversal, regression-countdown-actions-tie | 0 |
| **regression-landing-held-roll** | **2, first at frame 1623: `held_updates=1`, `held_rotations=0`, `roll_step=0xfffb`, `bounce=0`** |

That is exactly the state R-0038 attributes to fuzz seed 31 at 1623, observed in
an original-only capture. The bound was therefore wrong and its removal is
justified. The narrowing is minimal: `|held_updates|`, `held_rotations` and
`completed_rolls` are all still bounded by the elapsed updates, and
`dragster_race_tests.cpp` pins both the new acceptance and the surviving
elapsed rejection (`held_updates=2001` still rejected).

### The relaxed result-outcome consistency check

`presentation.cpp` `build_result_map` changed
`finish_time_centiseconds[0] < [1]` to `<=` for `PlayerWon`. Checked against the
original: `regression-countdown-actions-tie-a` has both riders finishing on
frame 3226 with `total_time` 3358 each, so an equal-time win is reachable and
the guard as written would have rejected a real state. Justified.

## Findings

All low severity; none blocks integration.

1. **(Low) The shared live camera floor changed for ZOOM ZOO too, and nothing
   pins it.** `src/app/frontend.cpp` `presentation_position` changed
   `std::max<std::int32_t>(0, signed_x - 880)` to `std::max<std::int32_t>(14, ...)`.
   That function is used by the live app for *both* tracks
   (`src/app/sdl_main.cpp:316,441`), so ZOOM ZOO's view also changes whenever
   the player is behind x 894. It is a safety fix - the old floor produced
   `bg1_x = -14`, reading before the accepted BG1 map - and it is documented in
   R-0038 and in a code comment, but the only test of this function
   (`tests/app/frontend_contract_tests.cpp:86`) uses x = 13,696, which is
   unaffected. Reproduce: `presentation_position(800)` now returns
   `camera_x = 14, bg1_x = 0` where it previously returned `0, -14`.
   Suggested (not required): one extra `require` for a behind-894 x.

2. **(Low) The relaxed tie guard also relaxes the legacy DRAGSTER path.**
   The `<=` in `build_result_map` is shared with the accepted M3/M4-01 DRAGSTER
   presentation, where no evidence was offered that a tie is reachable. The
   legacy presentation gates still pass (verified above), and the change can
   only accept states it previously rejected, so this is a note rather than a
   defect.

3. **(Nit) Evidence frame numbers disagree between records.** R-0038 says the
   seed-383 hold-above-rotations state is at 3473; the code comment in
   `src/core/movement.cpp` and `tests/native/dragster_race_tests.cpp` say 3472.
   One of the two is off by one. Neither is load-bearing for any check.

4. **(Nit) The headless runner's message for a v1 pack is not the product's.**
   `build/app-debug/src/core/zoom_zoo_runner --start classic.crawler.dragster
   --content-pack local/classic-crawler-dragster.pack` exits 1 with
   `Classic pack logical entry is absent: zoom.track-data` instead of the
   app's remedy text. It is a laboratory tool, not the product surface the
   claim is about, so this is cosmetic.

5. **(Note, not a defect) Declared limits are real and correctly stated.**
   The held rider art outside the five recovered pose pairs, the approximate
   fade, the authored pause menu (RESTART RACE where the original retires to
   the tour), the static result background and the un-emulated result-screen
   exit on other buttons are all declared in R-0038 and the READMEs. My own
   200-update DRAGSTER launch reported `rider-pose fallback frames: 201`,
   consistent with that declaration.

## Not assessed by this review

- **Live play with a real window, keyboard and gamepad** through result and
  Race Again. This is the task's own parked criterion and needs the user; a
  reviewer subagent cannot supply real input events. Everything here is
  headless or hidden-window.
- The remaining M4-16 case set beyond the primary and idle-late-start gates
  (the six complete ZOOM ZOO cases, the seven reward and eight trick probes)
  and the M4-15 race matrix: not re-run here. The implementer's
  `candidate-6534241` reports cover them; the primary gate, the full synthetic
  suites on four presets and the 20-command historical matrix were the
  higher-value independent checks and all passed.
- The 3,000-seed abort fuzz and the 150-seed differential fuzz were not
  repeated at their declared sizes; a fresh independent abort fuzz was run at a
  smaller size (see above).
- The result-font pixel comparison against original result frames, and the
  claimed 961/57,344-pixel tie-picture similarity: not re-measured. The digit
  coverage is exercised indirectly - the reviewer's withheld race and the fresh
  abort fuzz reach stable result screens without the missing-glyph abort.
- Hosted CI runs 35250987766 / 35258884539 were not re-inspected; CI on
  `16b8daf` is the implementer's and integrator's to confirm.
- The DRAGSTER 10:00 time limit (no case reaches it) and ZOOM ZOO's physical
  D-pad remain declared follow-ups, as recorded in the task.

## The reviewer's withheld case

```json
{"id": "review-withheld-mixed", "changes": [
  {"from": 1329, "to": 1400, "buttons": ["x", "left"]},
  {"from": 1401, "to": 1450, "buttons": ["a", "right"]},
  {"from": 1451, "to": 1500, "buttons": ["right"]},
  {"from": 1501, "to": 1530, "buttons": ["b", "right"]},
  {"from": 1531, "to": 1545, "buttons": ["b", "right", "x"]},
  {"from": 1546, "to": 1560, "buttons": ["right", "x"]},
  {"from": 1561, "to": 1600, "buttons": ["l", "right"]},
  {"from": 1601, "to": 1650, "buttons": ["right"]},
  {"from": 1651, "to": 1663, "buttons": ["b", "right"]},
  {"from": 1664, "to": 1680, "buttons": ["r", "right"]},
  {"from": 1681, "to": 1700, "buttons": ["right"]},
  {"from": 1701, "to": 1760, "buttons": ["right", "y"]},
  {"from": 1761, "to": 1800, "buttons": ["y"]},
  {"from": 1801, "to": 1990, "buttons": ["left"]},
  {"from": 1991, "to": 2002, "buttons": ["b", "left"]},
  {"from": 2003, "to": 2020, "buttons": ["left", "x"]},
  {"from": 2021, "to": 2090, "buttons": ["left"]},
  {"from": 2091, "to": 2100, "buttons": ["select", "up"]},
  {"from": 2101, "to": 2120, "buttons": ["down", "right"]},
  {"from": 2121, "to": 2400, "buttons": ["right"]},
  {"from": 2401, "to": 2412, "buttons": ["b", "right"]},
  {"from": 2413, "to": 2440, "buttons": ["right", "x"]},
  {"from": 2441, "to": 2600, "buttons": ["right"]},
  {"from": 2601, "to": 2615, "buttons": ["b", "right"]},
  {"from": 2616, "to": 2640, "buttons": ["l", "right"]},
  {"from": 2641, "to": 4800, "buttons": ["right"]}
]}
```

Horizon 4800. Capture with `dragster_playable_reference`, then `explore` or
`freeze` + `compare` as in section 1b.
