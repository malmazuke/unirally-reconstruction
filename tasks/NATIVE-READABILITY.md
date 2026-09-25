# NATIVE-READABILITY - make the recovered native code read as game code

## Assignment

- Status: ready (prepared 25 September 2026 at the user's request; claim it after
  HUNTER-EFFECTS is integrated, before RESULT-TITLE-GLYPHS).
- Milestone: M4 breadth (a quality task on the recovered engine; no new mechanics)
- Coordinator: the claiming session is coordinator, primary and integrator
- Task provider (fixed for all children; record any user-initiated platform change): Anthropic
- Worker/session/runtime/model: to be recorded at claim
- Actual model/reasoning effort, routing rationale and frontier escalation question (if any):
  the claiming session's model at default effort. **Tiered by part** (D-0008): part 1 is
  tier 2 (tooling, style rules, records); part 2 is **tier 1** because it rewrites simulation
  code in `src/core`, even though its intended semantic change is none; part 3 is tier 2
  (presentation and runners).
- Provider quota window/baseline timestamp, used/remaining or unknown, reserve and session
  allowance (D-0004): sample at claim.
- Reviewer (primary automatically spawns fresh model/effort, isolated checkout; no user
  trigger): a fresh Anthropic subagent per part; for part 2 it also runs the readability
  probe below and picks withheld captures.
- Dependencies and evidence of acceptance: HUNTER-EFFECTS integrated (it adds to
  `movement.cpp`, so this task starts on its result); STATIC-CODE-MAP (R-0045) for the address
  index; every frozen gate and the 36-track matrix as the behaviour oracle.
- Base commit: the `main` tip at claim.
- Branch and isolated worktree: `task/native-readability-<part>` in
  `.worktrees/native-readability`; one pull request per part.
- Owned paths and shared interfaces: `src/core/**` (structure, names, comments, file split;
  not behaviour, not serialized formats), `.clang-format`, a style section in
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

## Handoff

- Exact next experiment/command: after claiming on the HUNTER-EFFECTS result, re-run the
  baseline clang-tidy command above over `src/core` and record the count here, then draft the
  style rules and the `.clang-format` (part 1).
