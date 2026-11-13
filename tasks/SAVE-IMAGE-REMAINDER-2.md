# SAVE-IMAGE-REMAINDER-2 - the saved image's differences R-0117 leaves open

## Assignment

- Status: **claimed** 13 November 2026 05:47 UTC on main `aa647f41`, on the user's instruction ("next
  task", after OPPONENT-POSE-BOUNDS #106). Queued 12 November 2026 by
  [SAVE-IMAGE-REMAINDER](SAVE-IMAGE-REMAINDER.md) (R-0117). Claude Code desktop app, Claude Opus 5.5
  (`claude-opus-5-5`): coordinator and integrator; a worker subagent captures and implements, a fresh
  subagent reviews. Run under [D-0013](../docs/decisions/D-0013-task-time-and-disk-budget.md): claim
  to merge against the 6-hour target, new evidence under 1 GB. Usage at the claim: 5-hour 9%,
  weekly 7%.
- Branch `task/save-image-remainder-2` in `.worktrees/save-image-remainder-2`; evidence in main
  `local/evidence/save-image-remainder-2/`.
- Task provider: Anthropic.
- Milestone: M4. Review tier: **1** (front-end behaviour and cartridge RAM a later boot reads).

[R-0117](../docs/research/R-0117-save-image-remainder.md) closes R-0111's Value list but five items.
Each is a native difference from the original over R-0111's corpus (evidence and tools in main
`local/evidence/save-image-remainder/`; the check is its `check_image.sh`):

1. **A fuzz run's menus** (`c385`, `dragster-ordinary-controls` diff-fuzz 155): after the race the
   fuzz pads drive the menus; the original raises the logo (`$77:0742` bit 1) at 3,527, native at
   3,570 together with the pad gate's bit 10, and from there the screens differ (the original's
   PICK TRACK markers step `$10A7` at 3,678-3,737, native's do not). Find the first differing menu
   frame (pictures, `$008F`/`$009B` state) and the input native takes differently.
2. **The league editor's buffer tail** (`c039`, `menu-input-review/rv-pick`): a one-letter league
   name commits `$00E1-00E2` as `5F 5F` in the original, `58 FB` natively (native starts the buffer
   with `$80:9E0F`'s bytes, R-0097). Bytes after a league name's `FF FF` are read: XOR-perturbing
   them changes `c116`'s picture from frame 6,757 (`perturb-leagues-after-terminator.json`). Find
   what wrote `$00E1-00E2` on this path and what reads the tail.
3. **A saved marker step of 14 to 0x7FFF**: a save made during a gold ending holds the ending's
   counter (`$77:10A7`, up to 0x0020; R-0117 item 2). A later boot's markers count it down
   (`$80:EB28-EB32`, a word) reading `$80:EB61,X` and `$80:EB61 + 7,X` past the 14-byte table into
   code; native starts from 0 (`cartridge_ram.cpp`). Model the word and the bytes it reads (pack
   content), with a warm-boot capture from such an image.
4. **The league markers' edges** (`c029`, `c032`, `g069`): 1 to 5 write frames each where the
   hidden markers stop or run a frame apart (R-0111's timing class); show they are the same write
   or model the edge.

5. **The race start on HUNTER's preloaded tours** (R-0117 item 1, review 1): native's front end
   starts the race one frame early on the 42 track_reference runs of tracks 41 and 43 with
   `$77:10B1` = 0 (two preloaded starts), and one late on the track-8 run (`race-starts.json`: 484
   equal, 42 early, 1 late), because the original's sound upload took 81 frames where R-0077's
   table has 80 (D-0010 keeps the table). In play this reaches a save a later boot reads: the
   personal bests on `c175`, `c177`, `c210`, `c220` (`$085F`/`$085B`) and `$1116` on `c209`, `c217`,
   `c218`, `c272`, which `check_image.sh` no longer sees (it takes the captures' starts);
   `check_race_starts.sh` pins native's start. Model what makes the upload a frame longer on these
   paths (the sound processor's state after the warm boot and HUNTER's tour), or record why not.

Outcome: each item equal at every write frame of the corpus (`check_image.sh`, `expected.json`
re-recorded) or recorded with why it cannot matter.

## Results

Worker: Claude Opus 5.5 (claude-opus-5-5) subagent, 05:48-06:52 UTC (harness clock), 1 h 04 min, 13 November
2026, in `.worktrees/save-image-remainder-2`. About 45 min of captures and analysis, 15 min of
native changes and tests, the rest records and checks.
[R-0119](../docs/research/R-0119-save-image-remainder-2.md) holds the findings.

- **Item 1, `c385`'s menus: fixed.**
  - On 3,527 pad 1 presses B, Right and Up together on RECORDS. The original chooses the entry the
    arrow is on, because its list loop tests Y/X and B/Start/A before Down/Select and Up
    (`$80:B983-B99E`). Native moved up to EXIT first and went back to the main menu.
  - Native's OPTIONS, league-slot and RECORDS loops now move only when no exit button is down.
  - `c385`: 119 differing write frames to 4 (`$0743`'s known bit timing). `c434`: 1 to 0.
  - `g078` and `g079` change menu state only: `$009B` stays 0, as in the original.
- **Item 2, `c039`'s league name tail: fixed.**
  - An unmarking in the league picker leaves two underscores at `$00E1-00E2` (`$80:9D66-9D6A`), and
    the editor keeps them. Native kept only the X.
  - Native now keeps the last marker's two characters. `c039`: 170 to 1 (the league pair's 3-frame
    write at 1,970).
- **Item 3, a saved marker step of 14 or more: fixed for the image.**
  - Warm-boot captures from c175's image with `$10A7` set, on OPTIONS paths. The step is a word
    counted down to 13, reading tiles past the table into code. 0x8000 counts on from 0x7FFF; R-0117
    said 13.
  - The tiles go only to hidden entries: the pictures are equal on every frame on OPTIONS, the
    league slots, the rider picker, the keyboard and RECORDS (review 1 added RECORDS and the
    longer runs). The league's continue screen and the local modes' loops are not checked.
  - Native loads and stores the whole word and counts it as `$80:EB28-EB32` does. A tile past the
    pack's 21 bytes leaves its hidden entry unchanged.
  - Six warm boots are equal to the original at every frame (`check_marker_step.sh`).
- **Item 4, the league markers' edges: localized, left open.**
  - The original's continue screen after the league's podium starts a frame after native's on
    `c029`, `c032`, `g069`, `c100` and `c101` (11 other routes are equal).
  - The cause is the title-return sound session (`$80:A0F7`), requested at the same clock but about
    31,000 master clocks behind at the same step of its sample upload on `c032` against `g066`.
  - Native's sound model does not separate the routes. Queued.
  - A later boot: a save made after the continue screen holds a step one off the original's (both
    0-13). The boot's list loops turn their hidden markers one step apart until PICK TRACK stores 0
    before its first turn; nothing else reads the word.
- **Item 5, HUNTER's preloaded race start: explained, checked, left open.**
  - The warm boot changes the sound processor's phase. The upload's polls end 2,256 clocks before
    the frame's end, and the race setup's tail (5,214 clocks to the frame wait `$00:8587`) slips the
    first update a frame.
  - Native's own sound model, run from power-on with the run's image, predicts the original's start
    on all 527 track_reference runs (R-0077's table: 484).
  - The front end does not consult the sound model (D-0010), and changing that is a design decision.
    Queued as [SAVE-IMAGE-REMAINDER-3](SAVE-IMAGE-REMAINDER-3.md). In play the personal bests and
    `$1116` on the 8 routes can still differ.
- **Totals.** Write frames from 406 differing outside the residual: main 11,879 (724 of 916 routes
  equal), candidate 11,594 (725).

## Decisions (worker)

- **Item 5: no table change, no product change.**
  - The table's cells are right for the cold six-quits measurements. The warm-boot cells would hold
    only for these exact frames.
  - Following the sound model needs D-0010 amended and the sound model run beside every front end:
    about 1.8 s of CPU per 1,400 frames.
  - Recorded and queued instead of a hack. `check_audio_starts.sh` pins the evidence: the sound
    model's rule on all 527 runs, and the table off on 43.
- **Item 3: no pack profile v42 for the 18 code bytes past the marker table.**
  - The gate runner and every session take the newest `local/` pack, and main's runner cannot load
    a v42 pack. The bytes reach only hidden OAM entries.
  - Native leaves those entries' tiles unchanged. Recorded as a limit.
- **Item 4: localized, not fixed.** The two routes' sessions differ in the original with identical
  native cues, so a fix needs a capture of the original's port writes on `c032` and `g066`. That is
  beyond this task's budget.
- **Item 1: the loop order is fixed on all three loops native runs on `$80:B93C`** (OPTIONS, the
  league slots, RECORDS), not only RECORDS. The listing is the same routine for all three.
- **Two environment options for `race_audio_runner`** (a test tool), not new arguments, so that
  `load_times.py`'s parsing and the gate scripts that call it are unchanged.
- **Expected results.** This task's `expected.json` is this corpus's re-recorded result. It equals
  what `save-image-remainder/check_image.sh` now gives; `expected-save-image-exact.json` is
  SAVE-IMAGE-EXACT's re-recorded result. Neither shared file is overwritten while this branch is in
  review.

## Evidence

All in main `local/evidence/save-image-remainder-2/` (2.3 MB).

- **Item 5:**
  - `audio_starts.py` and `audio-starts.json`: 527 runs, the session end, margin and frames.
  - `traces/c175.vtrace.txt` and `traces/c175-1311.vtrace.txt` (`vtrace.py`): the request at
    `$83:CA77` on 1,311 line 70; the polls' end `$83:CBF2` on 1,390 line 223 dot 472; the frame wait
    `$00:8587` on 1,391 line 227.
- **Item 4:**
  - `traces/c029*.vtrace.txt`, `traces/c032-pod.vtrace.txt`, `traces/g066-pod.vtrace.txt`,
    `traces/c032-all.vtrace.txt`, `traces/g066-all.vtrace.txt`;
  - `traces/item4-continue-visits.txt`: the 16 routes reaching the continue screen;
  - `traces/item4-native-session-ends.txt`: `ends.py`, native's sound model.
- **Items 1 and 2:** `traces/c385-wram-009B-0072.txt` and `traces/c039-wram-00DC.txt`
  (`wram_watch.py`).
- **Item 3:**
  - `orig_markers.py`;
  - `marker_routes.py`, with `marker-step/routes.json` and `marker-step/original/` (R-0111's change
    logs, `original_trace.py`);
  - `marker-step/pictures-c039-pads.json` (`orig_marker_pictures.py`): 0 differing pictures in
    1,101 frames, 386 OAM-buffer frames.
- **Behaviour:** `stdout-main-vs-candidate.json` (`stdout_same.py`, 919 routes). Four differ: `c385`
  from 3,527, `c434` from 4,368, `g078` from 1,950, `g079` from 1,750.
- **Checks** (`checks.list`, run with `$G` containing a space):
  - all three PASS on the candidate (`checks-candidate.log`);
  - all three FAIL with main's runners `base/front_end_runner` and `base/race_audio_runner`
    (`checks-main.log`). The failures: `check_image` on `c039`, `c385`, `c434`; `check_marker_step`
    on three routes; `check_audio_starts` on all runs, since main's `race_audio_runner` has no
    `UNIRALLY_LOAD_ENDS`.
- **The previous tasks' checks** on the candidate:
  - `save-image-remainder/check_race_starts.sh` PASS;
  - `save-image-remainder/check_image.sh` FAIL on `c039`, `c385`, `c434` (toward the original);
  - `save-image-exact/check_image.sh` FAIL on the same three.
- **Builds and tests:**
  - `project.py build` `status=passed` for lab-release, lab-debug and app-debug;
  - `ctest` on lab-debug: 41 of 41;
  - `clang-format` applied to `src/core`;
  - `clang-tidy -p build/lab-debug` clean on the four changed `.cpp`;
  - GCC 13.3 (`ubuntu:24.04`, CI's warnings as errors) plain and sanitized OK on the six changed
    files and `front_end_tests.cpp` (`gcc.sh`, `gcc.out`);
  - `coverage-census/regen.sh` regenerated the static map and native symbols; tooling tests OK;
  - `coverage native-symbols --check` `status=passed` (1,807 addresses, 0 without a record).
  - ASan presets not run (they hang on this host).

## Handoff

- **Commits on `task/save-image-remainder-2`:**
  - `4d5f2d48`: native, tests, the test runner's options, native symbols and the static map;
  - then the records commit: R-0119, this record, SAVE-IMAGE-REMAINDER-3 and its README row.
- **Done:** items 1, 2 and 3. Items 4 and 5 are explained and queued
  ([SAVE-IMAGE-REMAINDER-3](SAVE-IMAGE-REMAINDER-3.md)). Not pushed, no PR, no review yet (tier 1).
- **At merge:** replace
  - `local/evidence/save-image-remainder/expected.json` with this task's `expected.json`;
  - `local/evidence/save-image-exact/expected.json` with `expected-save-image-exact.json`, keeping
    the old ones beside them.
- **What the gates should expect**
  (`--task-checks local/evidence/save-image-remainder-2/checks.list`):
  - **this task's three checks:** PASS on the candidate, FAIL on main;
  - **SAVE-IMAGE-REMAINDER's `check_image.sh`:** FAIL on the candidate on `c039`, `c385`, `c434`,
    each toward the original, until its `expected.json` is replaced. Its `check_race_starts.sh`
    passes (native's race start is unchanged);
  - **SAVE-IMAGE-EXACT's `check_image.sh`:** FAIL on the same three routes. Its re-recorded result
    is `expected-save-image-exact.json` here;
  - **front-end steps (`fe-equivalence` and its classifier) and any check replaying a
    `front_end_runner` invocation:** outputs change only where the invocation has one of:
    - a frame on OPTIONS, the league slots or RECORDS where an exit button (Y, X, B, Start, A) and
      Down, Select or Up are down together: the arrow no longer moves before the exit;
    - a league named with one or two letters after an unmarking: the record's bytes after its
      terminator;
    - an image whose `$10A7-10A8` holds 14 or more, or a nonzero high byte, before any PICK TRACK:
      the step and the hidden markers' OAM tiles.

    On R-0111's 919 routes this is the menu state of `c385`, `c434`, `g078`, `g079`, and the
    cartridge RAM of those and `c039`. Pictures can change only on those four routes, from the
    frames above;
  - **audio steps:** unchanged. `race_audio_runner` behaves as before unless the new environment
    variables are set, and `load_times.py`'s format is the same.

## Review round 1 fixes (worker)

Review round 1 on PR #107 (no blocking findings), fixed 13 November 2026 07:03-07:10 UTC by the same
worker (Claude Opus 5.5).

1. R-0119 cites `$80:9D3E-9D49` (the copy starts at `LDY #7`) and says it copies 9 bytes to
   `$00DC-00E4` in 16-bit words. The regenerated `labels.json` has `loc_809D3E` in place of
   `mid_809D3F`.
2. `turn_markers`' comment and the records now name only the captured screens. Review 1's captures
   (`marker-step/pictures-8000-*.json`, saved step 0x8000 against 0) add the league slots and their
   warning, the rider picker, the keyboard (`c039`'s pads, 2,600 frames) and RECORDS (`c075`'s,
   1,700): 0 differing pictures. On `c078`'s and `c081`'s pads (two-player and VS menus) no turn ran.
   The league's continue screen and the local modes' loops are recorded as unchecked.
3. `cartridge_ram.cpp` reads the step with `front_end_screens::saved_marker_step`, beside
   `store_marker_step`; the local constant is gone.
4. `front_end_tests.cpp`: OPTIONS with Y and Select leaves for the main menu with the arrow on 0, and
   Select alone moves it; the league slots outside a league with Y and Down go back to OPTIONS from
   the slot the arrow is on.
5. R-0119 item 4 and this record say what item 4 does to a later boot: a save after the continue
   screen holds a step one off (both 0-13), which turns hidden markers one step apart until PICK
   TRACK stores 0.

Rerun: `project.py build` passed on lab-release and lab-debug; `ctest` 41/41 on lab-debug;
`clang-format` applied; `clang-tidy` clean on the two changed `.cpp`; GCC 13.3 plain and sanitized
OK (`gcc-r1.out`); `coverage-census/regen.sh` regenerated the static map and native symbols (tooling
tests OK); `native-symbols --check` passed (1,807 addresses, 0 without a record). `checks.list` not
rerun (the gates run it): the native changes are a refactor of the load and comments, so its outputs do not change.

## Closeout (coordinator)

- Review: tier 1, one round on PR #107 at `f21b8865`. No blocking findings; five non-blocking (a
  citation starting mid-instruction, a code comment claiming screens the captures did not cover, a
  duplicated `$10A7` constant, no OPTIONS or league-slot test of exit before move, item 4's effect on
  a later boot unstated). All fixed in `67bd4eea` (replies on the PR). The reviewer reran the three
  task checks (PASS; `audio-starts.json` byte-identical) and the lab-debug ctest (41/41).
- Gates (D-0012) on `67bd4eea`, TMPDIR `/private/tmp/sir2g`:
  - `gates-1` (2 h 35 min): 101 steps same-as-main, 20 passed, 103 against the original, 19 skipped;
    the three task checks PASS (`check_image`: 11,594 differing write frames, 725 routes equal;
    `check_marker_step`: 6 of 6; `check_audio_starts`: 527 runs, 0 unlike the original).
  - Changed, both read: `fe-equivalence` and `fe-classify`. Main's and the candidate's runners
    differ on `league/organic-slot-selector` and `league/organic-slot-two`, which press Select, Y and A
    together on the league slots (frames 1,950 and 1,750). Main moved the arrow to the second slot
    before leaving; the candidate leaves from the first. The original's snapshot at 2,000
    (`organic-slot-selector/snapshots/wram-02000.bin`) has the arrow at x 1,211, y 1,408, spin 7:
    the candidate's values (main's: x 1,135, y 1,444). `organic-slot-two` is the same press on the
    same screen and the runners agree again from frame 1,815.
  - `fe-replay` also showed three corpus cases (`mi-cues`, `mi-rv-pick`, `rs-mi-review-rv-pick`)
    differing in the cartridge RAM dump only, at frame 2,091: offsets `0x173-0x174` hold `__` (item 2's
    league-name tail) and `0x22E-0x22F` the checksum. No step reads that stream; R-0111's corpus
    (`check_image`) is equal to the original there.
  - `gates-2` (`--only` and `--expect-changed` the two, 53 s): `status=passed`.
- CI on `67bd4eea`: macOS and Ubuntu lab jobs pass.
- Registered as **accepted in part**: items 1-3 are done; items 4 and 5 are explained and queued as
  [SAVE-IMAGE-REMAINDER-3](SAVE-IMAGE-REMAINDER-3.md) (item 5 needs a D-0010 amendment).
- Time: claim `e51bc84b` 05:47 UTC on 13 November 2026. Worker 1 h 04 min plus 7 min of review
  fixes, review 11 min, gates 2 h 35 min + 53 s; the gates finished at 10:05 UTC. The coordinator's
  reading of the gate changes then stopped from about 10:20 until the user's "Continue" at about
  21:30 UTC, so claim to merge is about 16 h on the clock but about 4 h 50 min of work, inside
  D-0013's 6-hour target only if the idle stretch is left out.
- At merge, `local/evidence/save-image-remainder/expected.json` and
  `local/evidence/save-image-exact/expected.json` are replaced by this task's `expected.json` and
  `expected-save-image-exact.json` (the old ones kept as `expected-before-sir2.json`).
- Evidence: main `local/evidence/save-image-remainder-2/`, 2.3 MB after `compact_evidence.py`. Worktree and
  branch removed after the merge; gate reports in main `artifacts/sir2-integration/`.
