# NATIVE-READABILITY - make the recovered native code read as game code

## Assignment

- Status: **in progress**. Part 1 (tier 2) was reviewed and integrated by
  [pull request #23](https://github.com/malmazuke/unirally-reconstruction/pull/23) (`d97641c`).
  Part 2 (tier 1, the simulation) is in review on its pull request; part 3 follows. Claimed
  25 September 2026 at 05:33Z by the Claude Code desktop session that queued the task, on base
  `83dd9ff`. The user asked for it to start at once ("You can pick this up here now").
- Milestone: M4 breadth (a quality task on the recovered engine; no new mechanics)
- Coordinator: the claiming session is coordinator, primary and integrator
- Task provider (fixed for all children; record any user-initiated platform change): Anthropic
- Worker/session/runtime/model: Claude Code desktop app, Claude Opus 5.5 (`claude-opus-5-5`)
- Actual model/reasoning effort, routing rationale and frontier escalation question (if any):
  the claiming session's model at default effort. **Tiered by part** (D-0008): part 1 is
  tier 2 (tooling, style rules, records); part 2 is **tier 1** because it rewrites simulation
  code in `src/core`, even though its intended semantic change is none; part 3 is tier 2
  (presentation and runners).
- Provider quota window/baseline timestamp, used/remaining or unknown, reserve and session
  allowance (D-0004): at claim (05:33Z) the 5-hour window was 7% used and the weekly window
  17%.
- Reviewer (primary automatically spawns fresh model/effort, isolated checkout; no user
  trigger): a fresh Anthropic subagent per part; for part 2 it also runs the readability
  probe below and picks withheld captures.
- Dependencies and evidence of acceptance: HUNTER-EFFECTS integrated (it adds to
  `movement.cpp`, so this task starts on its result); STATIC-CODE-MAP (R-0045) for the address
  index; every frozen gate and the 36-track matrix as the behaviour oracle.
- Base commit: the `main` tip at claim.
- Branch and isolated worktree: `task/native-readability-<part>` in
  `.worktrees/native-readability`, one pull request per part. Part 1 is
  `task/native-readability-rules`. The frozen base binaries for the equivalence sweep are
  built in a detached `.worktrees/native-readability-base` at `83dd9ff`.
- Owned paths and shared interfaces: `src/core/**` (structure, names, comments, file split;
  not behaviour, not serialized formats), `.clang-format` (placed at `src/core/.clang-format`,
  see Part 1 result), a style section in
  `src/core/README.md`, the native-symbol index and its `coverage static-map` input, the
  reviewer checklist in `docs/AGENT_WORKFLOW.md`, native tests only where a rename or split
  requires it, this record, `docs/STATE.md`, `tasks/README.md`, `tasks/NEXT_SESSION.md`.
- Claim/lease/heartbeat/checkpoint location: this record.
- Session time limit, concurrency allocation and actual spend authorization if relevant: one
  worker plus the review subagent; no monetary spend. No recovery task runs in parallel on
  `src/core` while a part is open.

## Why now

On 25 September 2026 the user pointed at the reward-queue update in `src/core/movement.cpp`
(the `event<72 || event>=200` branch at `47708b4`, lines 658-672) and asked whether it is
readable, and whether to refactor now or after full coverage. It is not readable as game
code: the reader learns that `$81:C238` is `CMP #$48 / BMI` but not what the reward queue is
for; 72, 200, 215, 26, 255, 31 and 40 are bare literals; verification-domain `throw`s sit
between gameplay branches; formatting is dense (`if(!leading_event)weight=&...`). This breaks
D-0003's rule that a research translation "must not silently become the public
architecture". The reviewer checklist has named readability since D-0003, but no acceptance
criterion measured it, so it was not enforced.

Decision (recorded in the D-0003 update of 25 September 2026): do a bounded readability pass
on the recovered code now, under the frozen gates, and make the rules measurable for every
later task. Do not wait for full coverage: at the STATIC-CODE-MAP measurement the recovered
code is about a third of the code banks and an eighth of the game by features, every new task
copies the style of the engine it extends, and the cost of a later big rewrite grows with
each recovered routine. Do not re-architect into game systems yet: which systems the game has
is not known until more than the race engine is recovered.

The user also asked whether this makes the unassessed code harder to reverse engineer. Only
in one way: the C++ stops following the ROM routine by routine, so "where is `$81:C238` in
native" becomes a lookup instead of a text search. The native-symbol index in part 1 is that
lookup, so it is a precondition, not an extra. Recovery itself reads the ROM, the static map
and the research records, which this task does not change, and established names make new
disassembly that touches recovered WRAM easier to read.

## Baseline at `47708b4` (re-measure at claim, after HUNTER-EFFECTS)

- `src/core`: 8,495 lines; `movement.cpp` 2,594, `presentation.cpp` 2,109.
- `clang-tidy -p build/app-debug --checks='-*,readability-function-size'` with
  `LineThreshold: 80` flags 15 functions: `movement.cpp` `update_idle_pose`, `update_pose`,
  `update_movement`, `update_zoom_roll`, `deserialize_classic_race`, `update_zoom_zoo`;
  `presentation.cpp` `build_result_map`, `render_dragster`, `observe_update`,
  `render_classic_race`; `vertical_contact.cpp` `resolve_vertical_contact`; `rider_look.cpp`
  `look_for_rider`; `content_pack.cpp` `ClassicContentPack`; the `main` of
  `classic_race_presentation_runner.cpp` and `zoom_zoo_runner.cpp`.
- 367 lines in `src/core` cite a ROM or WRAM address; no index maps addresses to native
  symbols. There is no `.clang-format`.

## Baseline re-measured at claim (`83dd9ff`)

- `src/core`: 8,834 lines; `movement.cpp` 2,807, `presentation.cpp` 2,148.
- The clang-tidy command flags **17** functions: the 15 above plus `update_hunter_effects` and
  `serialize_zoom_zoo` (both from HUNTER-EFFECTS).
- After part 1's format commit (`f73f06c`): 10,092 lines and **22** functions over 80 lines.
  The formatter spreads one-line branches over several lines, which pushes five more over:
  - `update_reward_queue`, `update_zoom_ai`, `update_zoom_throttle` and `deserialize_zoom_zoo`
    in `movement.cpp`;
  - the `main` of `movement_runner.cpp`.

  The 22 are listed in `src/core/README.md`, and parts 2 and 3 split them.
- Address citations (part 1's index at `def1d22`):
  - 570 distinct addresses cited in `src/core` comments (after review M1): 351 ROM, 198 WRAM,
    7 SRAM and 14 io, by 247 native symbols.
  - 216 of the static map's 636 routines are cited by native code (33,018 routine bytes).
  - 9 cited ROM addresses are cited by no record, directly or inside a cited range.

## Outcome and boundaries

### Part 1 - rules, formatting and the address index (tier 2)

1. **Style rules** in `src/core/README.md`, short and checkable:
   - Name established concepts by their game meaning; keep uncertain ones neutral
     (`unk_`/provenance names are fine) with a link to the record that would settle them.
     Rename only where the evidence establishes the meaning.
   - Numbers with game meaning are named constants next to the state they describe; bare
     literals are for 0, 1 and masks inside a named arithmetic helper.
   - A function fits on a screen: at most 80 lines by clang-tidy's count, with each exception
     listed in the README with its reason (a dispatch table, a serializer's field list).
   - Comments say what the game does first, then give one short evidence line
     (`// $81:C238; R-0035`). The forensic account belongs in the research record; keep in
     code only what stops a maintainer from "fixing" a ROM quirk.
   - ROM-exact quirks (8-bit wraps, `BMI` on a difference, out-of-table reads) live in small
     named helpers, as D-0003 already allows, e.g. `takes_reward_path(event)`.
   - Domain guards that refuse unrecovered states go through one helper and read as guards,
     not as gameplay branches.
2. **`.clang-format`** chosen to fit the code's existing intent, applied in a
   formatting-only commit so later diffs stay reviewable.
3. **Native-symbol index**: a tracked input (for example
   `docs/map/static/native-symbols.json`) mapping each implemented routine and WRAM field to
   its native function or state member and research record. `coverage static-map` reads it so
   the listing names the native symbol beside each implemented routine, and a tooling test
   fails when an address cited in `src/core` is missing from the index or the static labels.
4. **Checklist**: the reviewer checklist in `docs/AGENT_WORKFLOW.md` gains the measurable
   rules (function size, constants, evidence line, index entry) for every later task.

### Part 2 - the simulation (tier 1)

Apply the rules to `movement.cpp`, `vertical_contact.cpp`, `flat_contact.cpp`,
`speed_limits.cpp`, `track_progress.cpp`, `track_sampling.cpp`, `input_timer.cpp`,
`rider_object.cpp` and `content_pack.cpp`. Split `movement.cpp` along the systems it already
contains (the race update, pose, the reward queue and opponent boost, ZOOM ZOO roll, state
serialization) into separate files. Update order, integer widths and every serialized byte
stay the same; renamed state members keep their serialization order.

### Part 3 - presentation and runners (tier 2)

Apply the rules to `presentation.cpp`, `rider_look.cpp` and the runners.

### Out of scope

New mechanics or behaviour changes of any size; serialized format or pack changes; a game
systems architecture (scenes, entities, a frontend framework) or mod APIs, which D-0003 defers
until the recovered code shows the structure; tooling outside the index. A behaviour
difference found while refactoring is recorded and becomes its own task, not fixed here.

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| Nothing moves | Every frozen gate, v1 contracts, presentation contracts and pixel sweeps, hidden runs, fuzz, ctest, synthetic, `recompare --per-track` on the 36-track sweep | Digests and per-track results unchanged from the base commit | gate logs per part |
| Formats unchanged | Serialize the frozen states of every state kind (DRAGSTER, ZOOM ZOO, `URTRnn`) before and after | Byte-identical | log |
| Function size | clang-tidy `readability-function-size`, `LineThreshold: 80`, over `src/core` | Only the listed exceptions remain | log |
| Address index | The index test; `coverage static-map` regenerated | Every address cited in `src/core` resolves; the listing names the native symbol of each implemented routine | test log, regenerated map |
| Readability probe | Part 2 reviewer reads only the refactored code (no research records) and explains three functions it chooses plus the reward-queue update, then checks its explanations against the records | Each explanation matches the recorded behaviour; any function it could not explain is returned as a finding | review on the pull request |
| Review | Tier by part | Approved | reviews on the pull requests |

## Part 1 result (tier 2)

Part 1 is rules, formatting and the index. No behaviour change is intended, and none was found.

- **Format** (`f73f06c`, formatting only):
  - clang-format 19.1.7 over every `src/core` source, using `src/core/.clang-format`: LLVM base,
    4-space indent, 100 columns, short guards kept on one line.
  - Besides whitespace, it only reorders `#include` lines and splits three long string
    literals into adjacent literals.
  - The lab-release objects are identical before and after in every code and data section
    (`objcode-compare.sh 83dd9ff f73f06c`: 15 objects identical). Only the debug line tables
    differ.
- **Rules**: `src/core/README.md` "How this code is written" gives nine short rules:
  - names, constants, size, comments with an evidence line, `$` only for addresses;
  - ROM quirks in named helpers, guards through one helper;
  - the index and the format.

  `src/core/.clang-tidy` holds the 80-line check. The reviewer checklist in
  `docs/AGENT_WORKFLOW.md` names the measurable rules.
- **Index**: `coverage native-symbols` writes `docs/map/static/native-symbols.json`.
  - It lists every address cited in `src/core` comments, with the function, member or
    constant each citation belongs to.
  - The scanner is a small C++ scope reader. On all 269 function definitions `clang-query`
    reports in `src/core/*.cpp`, it finds the same definitions.
  - `--lookup '$81:C238'` answers "where is this in native" and lists the records citing the
    address, or a cited range holding it.
  - `coverage static-map` now names the native symbols beside each routine and label, in the
    tracked map, the labels and the ignored listing.
  - `tests/tooling/test_native_symbols.py` has 24 tests after review. They cover:
    - the scanner on authored snippets;
    - the tracked index, and the static map's `native` fields, against `src/core`;
    - a limit of 8 on cited ROM addresses with no record. Part 2 brings that limit to 0.
- **Equivalence sweep** (new evidence tool, `local/evidence/native-readability/equivalence.py`):
  - It runs every race scenario through a base and a candidate `zoom_zoo_runner`, under six
    controller schedules for 6,000 updates each:
    - DRAGSTER, ZOOM ZOO and the 36 `classic.track.NN`;
    - released, Right, Right with jumps, and three seeded random schedules with rare pauses.
  - It compares the output byte for byte. The runner prints the full serialized state after
    every update, so this checks behaviour and the serialized format together.
  - On the Right schedule it also renders six frames per track through each side's
    presentation runner and compares the pictures.
  - Negative controls:
    - Flooring the opponent's reward weight at 2 instead of 1: 8 of 76 random runs differ.
    - Shifting the race background's palette index by one: 36 of 38 picture sets differ.
  - Part 2 uses the sweep as its main behaviour check, alongside the gates.

Decisions and deviations, with reasons:

- **`.clang-format` lives in `src/core/`, not at the root**, so format-on-save in `src/app`,
  tests and tools does not reformat files outside this task's owned paths.
- **The index is generated from the code, not written by hand**, so it cannot drift from the
  citations. It is still the tracked input `coverage static-map` reads.
- **Record links are computed at lookup, not tracked.** Tracked links would make the index
  stale after any records-only change, and records-only pull requests take the docs-only CI
  fast path, which skips the tooling tests.
- **"Every cited address resolves"** is read as follows:
  - every address has a native symbol, by construction, checked by the staleness test;
  - every ROM address has a record, checked by the limit, which part 2 takes to 0.

  `labels.json` holds only colon-form record citations (STATIC-CODE-MAP's rule, unchanged
  here), so it is not the resolution target.
- **A value written with `$`** (`$0213` for a sound, `$31` for a step) is indexed as an
  address. Rule 5 asks for values in decimal or `0x`, and part 2 rewrites those comments.
- **`docs/BUILD_AND_VALIDATION.md` gains the command's inventory row.** It is outside the owned
  paths, but the command inventory has one home.

## Part 2 result (tier 1)

Part 2 is the simulation, on `task/native-readability-simulation` from `d97641c`. No behaviour,
state or format change is intended, and none was found.

- **The split** (`b665bf7`, a pure move): `movement.cpp`'s 82 functions move, with bodies and
  leading comments unchanged, into one file per system:
  - `race_update`, `rider_motion`, `rider_pose`, `opponent_ai`, `reward_queue`, `trick_roll`,
    `special_tiles`, `hunter_effects`, `race_progress`, `race_camera`, `race_setup`,
    `race_state_io`;
  - `movement.cpp` keeps the legacy URMV state;
  - the internal headers are `word_arithmetic.hpp`, `state_bytes.hpp` and one per system that
    another file calls.

  The only edits are the ones a move needs. `verify_split.py` checks every function's body,
  signature and comment against `d97641c`: 82 of 82. It catches a one-token change.
- **The rewrites**, one commit per system:
  - **Announcements**: `announcements.hpp` names the queue events by their captions (roll,
    flip, twist, z flip, wipeout, last lap, winner, draw, loser, the HUNTER effects and their
    end, the hints, the rider voices).
    - The update the user flagged is now `update_opponent_announcements`, made of small named
      steps: `takes_reward_path` states the `$81:C238` quirk once, the reward weight and the
      reward are their own functions, and guards go through `require`.
    - Players' landings announce their tricks by name (`announce_landing_tricks`).
  - **HUNTER effects**: the eight effects are named (`hunter_effect::barf_mode` ...
    `control_reversed`). There is one function per kind of effect.
  - **The drive**: the drive, brake and throttle run for both riders, so they move to
    `rider_motion.cpp` as `update_drive`, in five steps.
  - **The AI**: `update_opponent_controller` names the marker's flags; the stall, the jump
    decision and the launch are their own functions.
  - **The pose**: `update_pose` is five steps; `update_idle_pose` is its cycle, bias, velocity
    and wobble.
  - **The X trick**: named by its announcements, the X trick is `update_z_flip` (a
    completed spin is a z flip, a held middle pose a tabletop).
  - **State IO**: `race_state_io.cpp` follows the state's layout, with one `write_`/`read_`
    pair per section. The native race's cross-checks are named and run in the original's
    order.
  - **The race update**: `update_zoom_zoo` (513 lines) is its phases. The tile dispatch names
    the flag pairs, with one function per tile.
  - **Legacy DRAGSTER**: `update_movement` is its result phases, the scripted opponent, one
    rider, and the contacts and finish.
  - **Contact**: `resolve_vertical_contact` is the tile pair, the counters, support (a wall,
    a landing, or a slope) and the correction.
  - **The content pack**: its constructor is the header, the required entries, the inventory
    and the payloads.
  - **The smaller systems**: the race progress, camera, special tiles, setup and speed limits
    get named constants and steps.
- **Result**:
  - no function in the simulation exceeds 80 lines; the 8 left are part 3's;
  - all 351 cited ROM addresses have a record (the test's limit is 0);
  - no address citation is lost against `d97641c` (`citations_kept.py`);
  - values once written with `$` are in decimal or 0x (`rewritten-values.txt`).

Decisions and deviations, with reasons:

- **One pull request for part 2**, as planned. The pure-move commit is reviewable on its
  own through `verify_split.py`, and a separate pull request would have cost a second review
  and gate run. The move's own gates at `b665bf7` passed and are kept as evidence
  (`gates-b665bf7.out`).
- **The public API keeps its names** (`update_zoom_zoo`, `ZoomZooState` and the others), since
  `src/app` and the tests use them outside this task's owned paths. Internal names changed.
- **Records gained addresses**: R-0010, R-0035, R-0047 and R-0048 now cite the eight ROM
  addresses the code cited alone, each checked against the static listing. The code's
  `$82:9119` was the wrong call site; it is now `$82:9124`.
- **Rewritten values and corrected citations**: the citation check's list of expected
  removals is `rewritten-values.txt`. It holds values written with `$`, now in decimal or
  0x, and one corrected citation.
- **Left as they were**, already within the rules and clang-tidy clean: `flat_contact.cpp`,
  `track_progress.cpp`, `track_sampling.cpp`, `input_timer.cpp` and `rider_object.cpp`.

## Evidence and attempts

| Attempt | Hypothesis | Experiment | Observation | Next decision |
| --- | --- | --- | --- | --- |
| 1 | - | Baseline clang-tidy at `83dd9ff` | 17 functions; 22 after formatting | Record both |
| 2 | Formatting is token-neutral | Object code of every `src/core` object, before and after | Code and data sections identical in 15 of 15; debug lines differ | Commit as formatting only |
| 3 | A source scanner can attribute citations | Scanner against `clang-query` definitions | First pass missed access specifiers, `= {}` default arguments, empty `//` lines, braced member initialisers; after fixes all 269 definitions match | Index and tests |
| 4 | - | Records linked by exact address only | 60 unlinked ROM addresses; linking through cited ranges leaves 8 | Limit 8, part 2 to 0 |
| 5 | A native-against-native sweep detects change | Two mutants | Simulation mutant in 8 of 76 random runs; picture mutant in 36 of 38 | Use it as part 2's main behaviour check |
| 6 | - | Gates at `def1d22` (below) | Gates at `def1d22`: ctest 25/25 on lab-debug, lab-release and app-debug; v1 winner and loser contracts; six hidden 4,000-update app runs clean; fuzz 40 seeds, 0 aborts; the eleven differential gates passed with row digests identical to HUNTER-EFFECTS' `a3a4701`; equivalence 228 runs, 1,231,010 updates, 0 differences, 228 picture pairs identical; recompare identical to the base on 16 + 20 tracks. The synthetic suite failed 3 `test_tracks` checks (below) | Review |
| 7 | - | The synthetic suite at `def1d22` | 3 `test_tracks` checks parse the compiled pack tables in `content_pack.cpp` with a one-line pattern the format commit's wrapping broke | Pattern takes any whitespace (`1624727`); synthetic suite 508/508 |
| 8 | The move is pure | `verify_split.py` on `b665bf7`; a one-token mutant | 82 of 82 unchanged; the mutant fails | Rewrites |
| 9 | - | Equivalence after each rewrite (subsets), then the full sweep | 0 differences on every run, the full batch 351 runs, 1,933,523 updates, 1,047 restarts, 2,052 pictures | Gates |
| 10 | The state reader keeps its guards | `corruption.py`: 3,000 damaged states | 656 accepted, 46 distinct refusals, same result and message on both sides | Gates |
| 11 | - | Citations after the rewrites | 10 dropped by shortened comments; restored, `citations_kept.py` added | Every commit |
| 12 | - | Gates at `b665bf7` (the move) | Eleven gates with unchanged digests; the sweep; the recompare identical | Part 2 gates |
| 13 | - | Gates at `5a9305a` | ctest 25/25 on three presets, synthetic, v1 contracts, hidden runs, fuzz 40 seeds 0 aborts; eleven gates with unchanged digests; sweep 351 runs, 1,933,523 updates, 1,047 restarts, 2,052 pictures, 0 differences; recompare identical on 16 + 20 tracks; corruption 3,000 cases 0 differences; citations 0 lost; 8 functions over 80 (part 3's); 48 HUNTER held captures identical to the accepted run | Review |

## Handoff

- Current base/head commit and uncommitted state: part 2 on `task/native-readability-simulation`,
  base `d97641c`, candidate `5a9305a` plus these records.
- Verified findings: the Part 1 and Part 2 results above.
- Commands executed, outcomes and report hashes: `local/evidence/native-readability/` in the
  main checkout.
  - `gates.sh` takes the worktree to gate as its argument.
    - `gates-def1d22.out` is part 1's run.
    - `gates-b665bf7.out` is the move's.
    - `gates-5a9305a.out` is part 2's: the eleven gates, the equivalence sweep, recompare, the
      corruption sweep, citations, function size and the 48 HUNTER held captures.
  - Tools: `equivalence.py`, `corruption.py`, `verify_split.py` with `split_movement.py` and
    `movement-d97641c.cpp`, `citations_kept.py` with `rewritten-values.txt`, `regen.sh`.
  - Frozen base binaries: `base-83dd9ff/` (part 1) and `base-d97641c/` (part 2).
  - The gates run in a detached `.worktrees/native-readability-gates`.
- Unavailable/skipped checks: the ASan presets (host; the Linux CI job covers them). Twelve of
  the sweep's picture pairs are refusals on both sides: tracks 25 and 28 cannot draw their
  result title ([RESULT-TITLE-GLYPHS](RESULT-TITLE-GLYPHS.md)). The legacy `movement_runner`
  domain is narrow (Right, B only while still); its random schedules stop at its guards on both
  sides. A schedule that restarts the race from the pause menu ends its run's comparison at
  the first restart: the frame label resets and the harness's next controller row is refused
  on both sides (every `random-3` run stops there; the review's `random-13` at update 1,424 and
  `buttons-12` at 3,973). Restarts are compared to that point, not beyond (review of #24).
- Two citations resolve only to a file comment, both in presentation headers, for part 3:
  `$82:B8AA` (`rider_object.hpp`) and `$0D4B` (`rider_look.hpp`).
- Values the index still reads as addresses, for part 3 (presentation): `$0000` (VRAM),
  `$3D80`, `$7A00`, `$7B00`, and the colours `$4A52`, `$4631`, `$56B5`, `$4210`.
- Exact next experiment/command: part 3 (tier 2). Apply the rules to `presentation.cpp`
  (`build_result_map`, `render_dragster`, `observe_update`, `render_classic_race`),
  `rider_look.cpp` (`look_for_rider`) and the three runners' `main`. Check with
  `equivalence.py` (pictures on every schedule) and the v1 contracts, then `gates.sh`.

## Review and integration

- Part 1 (tier 2): a fresh Claude Opus 5.5 subagent in `.worktrees/native-readability-review`.
  - At `e0f8139` it **returned**
    ([review](https://github.com/malmazuke/unirally-reconstruction/pull/23#pullrequestreview-5314712223)).
    It confirmed:
    - the format commit's token and object neutrality, on 28 files and 15 objects;
    - the index's attributions against `clang-query`: 0 wrong of 307 in-body citations, plus
      the 50 trailing ones;
    - a byte-identical static-map regeneration;
    - the synthetic suite, 508 of 508;
    - an equivalence subset: 76 runs, 0 differences;
    - its own presentation-state mutant, caught in 5 of 38 runs.
  - Findings and responses:
    - **M1**, the index missed citations. Fixed in `444e354`:
      - it now reads bank-less continuations (`$80:84CB/84DB/...`, `$83:E611, E663`) and short
        ranges (`$114D-$119C`);
      - it indexes bank-less `$2000`-`$7FFF` as `io`;
      - `--lookup` finds an address inside a WRAM or SRAM range.

      The index grew from 558 to 570 addresses (351 ROM, 198 WRAM, 7 SRAM, 14 io). One more
      ROM address became visible without a record (`$80:850B`, a fifth transition table that
      R-0010 does not list), so the limit was 9. This record now cites it, so the limit is back
      to 8 (re-review S2). `$1CE` and `$220` are velocity values, not
      addresses: rule 5 has part 2 rewrite them in decimal.
    - **S1**: a test now checks the static map's `native` fields against the index.
    - **S2**: `operator<` and `operator<<`, digit separators and comments inside a wrapped
      signature are read correctly and tested. A requires-clause raises `UnsupportedShape`.
    - **S3**: the `io` entries also include VRAM words and colours written with `$`. Parts 2
      and 3 rewrite them under rule 5.
    - **S4** (records already say integrated): declined. The consolidated closeout rule
      finishes the records before the merge, and the merge waits for the review and checks.
    - **S5**: the swapped hashes are corrected.
    - **S6**: the limit test can be moved by a records-only change that removes a citation.
      Recorded here; such an edit is rare, and the failure message names the address.
    - **S7** (the sweep's gaps for part 2): taken into part 2's plan in the handoff.
    - **S8**: the README now names the app-debug build for the size command.
  - At `fe918d6` it **approved**
    ([review](https://github.com/malmazuke/unirally-reconstruction/pull/23#pullrequestreview-5314803988)).
    It confirmed M1 on every lookup that failed before and on all 24 new addresses, and found
    0 wrong attributions on 312 in-body citation lines. Its non-blocking findings were fixed
    in `53f067c`, which is tooling only and covered by tests, so there was no further
    review:
    - **S1**: the static map was regenerated after the final records.
    - **S2**: the limit is 8, the actual count.
    - **S3**: the citation rules and the scanner are tightened, with tests:
      - a spaced dash needs a `$` range end;
      - a comma continuation must end its list item;
      - a continuation in a code bank must be `$8000` or above;
      - digit separators only inside numbers, so `U'x'` is read correctly;
      - `operator bool` is named correctly.
    - **S4**: the non-address entries are listed in the handoff for parts 2 and 3.
- Part 2 (tier 1): a fresh Claude Opus 5.5 subagent in `.worktrees/native-readability-review2`.
  - At `89979973` it **approved**
    ([review](https://github.com/malmazuke/unirally-reconstruction/pull/24#pullrequestreview-5316410640)).
  - **Readability probe**: from the code alone, it explained `update_opponent_announcements`,
    `update_mud_tile`, `update_loop_tile` and `update_drive`. All four matched R-0035,
    R-0042, R-0047, R-0051, R-0038 and R-0011.
  - **Its independent checks**:
    - `verify_split` 82 of 82;
    - equivalence with withheld seeds and against its own base build, 0 differences; its own
      mutant differs in 48 of 117 runs;
    - corruption with two more seeds, 0 differences;
    - diff audits of `race_update`, `race_state_io`, pose, contact and legacy code: an
      old-against-new fuzz of about 5.8 million calls and 227,845 damaged states, 0
      mismatches;
    - the records' new addresses, against a regenerated listing.
  - **Findings S1-S7, fixed in `FIX3_SHA`**:
    - S1: the cooldown units (they fall by 2 a update);
    - S2: the HUNTER struct's comment placement, and three file-only citations moved to
      functions;
    - S3: the AI's suppression word is only tested;
    - S4: the loop's velocity y points down;
    - S5: the asymmetric braking test is now `braking_fast_enough`;
    - S6: `roll` is the X trick's state, its completions z flips;
    - S7: the jump, gravity, lift, drive step, loop top and options word are named.
  - **Its evidence gap**: runs end at a pause-menu restart. Recorded in the handoff.
