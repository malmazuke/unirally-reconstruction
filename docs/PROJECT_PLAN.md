# Project plan

Planning baseline v0.1 — 10 September 2026. Milestones are acceptance gates, not completion estimates. The backlog authorizes no implementation or spending by itself; the user's active work assignment controls execution.

## Product goal and accuracy target

Build a portable reconstruction that can eventually support online racing, authored tracks, an editor and replacement artwork. The near-term target is equivalent gameplay for one identified ROM revision. Recovering original source names or producing a byte-identical rebuilt SNES ROM is not required for this target. Annotated disassembly and matching routines remain useful evidence.

Maintain two explicit behavior profiles within one codebase:

| Profile | Contract |
| --- | --- |
| Classic | Preserve measured original rules, arithmetic, input sampling and game-update timing. Match the original within an explicitly recorded test domain. Track original quirks as behavior, not bugs to silently fix. |
| Extended | Allow custom content and deliberate rule changes, with a separate versioned rules identity and its own regression expectations. State the differences from Classic. |

Both profiles use deterministic simulation. Higher-resolution visuals and smoother display refresh must not silently change either profile's game speed or collision. Neither profile promises exhaustive equivalence merely because a finite suite passes.

### Human-readable source and community contributions

External contributions are not currently accepted; see
[the contribution policy](../CONTRIBUTING.md). The community goals below are
future direction. Authorized internal development and review continue.

Human-readable, maintainable source is an explicit goal from the first native routine. A contributor should be able to understand an update, locate its supporting evidence, and change it while running the relevant checks. Use descriptive domain names where meanings are established, small functions with explicit state/input/content dependencies, documented units and integer semantics, and comments explaining unusual original behavior with ROM addresses and research links. Keep uncertain meanings visibly provisional; do not turn a guess into an authoritative name.

Preserve verified Classic arithmetic and update order while improving structure. Keep processor bookkeeping and extraction offsets out of the public simulation interface where possible. A literal register-level translation may be a useful research intermediate, but delivering it as production code requires a documented reason and reviewable boundaries. Refactor in small steps under frozen differential tests; do not defer basic readability to a future wholesale rewrite.

Community extensions build on the existing Classic/Extended separation and content boundaries. Stable mod APIs, plugin systems and broad engine abstractions wait for demonstrated requirements. This goal does not authorize publication or choose a distribution license. See [D-0003](decisions/D-0003-human-readable-native-code.md).

### Initial scope

The user selected PAL Unirally (European/Australian version). Establish its exact revision and hash in M0. Start with one representative track segment, one rider and a short input recording covering acceleration, airborne movement, a trick and landing. The first native experiment can be headless. A playable version follows once the mechanics can be compared reliably.

Until the playable slice works, defer online services, matchmaking, a general-purpose editor, replacement-art commissioning, multiple ROM revisions and a full rewrite of the sound system. Reserve useful interfaces now; implement features when their milestone needs them.

### Classic content ownership and installation

The public source repository and downloadable native program do not contain the
original ROM or extracted original graphics, audio, tracks or other asset
payloads. For Classic, the user supplies a supported ROM; the project verifies
its exact identity and reproducibly creates a versioned local content pack.
Ordinary play uses logical entries in that pack and does not require the ROM to
remain present after successful extraction. Generated packs and loose extracted
content remain local and ignored. See
[D-0005](decisions/D-0005-classic-content-distribution.md).

Replacement and custom content uses the same logical identity boundary. A
future Extended distribution can be ROM-free when complete distributable
replacement content exists. This technical policy does not itself authorize
publication, select a source/content license or replace legal review.

## Architecture to grow from

Use a small deterministic simulation library with a headless runner. A desktop frontend consumes its state. Research tools run the original in a reference emulator through a separate adapter. Keep the emulator integration replaceable.

```text
ROM + recorded inputs -> reference adapter -> normalized reference state
 |                               |                     |
 |                         trace/snapshots              |
 v                                                     v
verified local extractor -> Classic content pack -> native simulation -> comparator
                                                   |
                                         render/audio frontend
                                                   |
                                     later: editor/network session
```

The proposed native stack is C++20, CMake, Python tooling and SDL3. The environment spike may revise it with a written decision. Avoid writing an engine framework before the first experiment.

Design commitments:

- **Explicit updates:** `step(state, inputs, content, rules)` advances one documented simulation update. M1 must determine how this corresponds to frames and input reads in the selected ROM; do not assume 60 updates per second.
- **Complete state:** inventory RNG, timers, previous input, animation state where gameplay depends on it, and all other future-affecting data. Canonical serialization and state hashing must avoid pointers, padding and host byte-order assumptions.
- **Defined arithmetic:** use explicit widths, signedness, overflow and fixed-point behavior where indicated by the original. Avoid C++ undefined overflow and compiler-dependent conversions. Display interpolation can use floats independently.
- **Content separate from code:** extract original content through reproducible tools into the ignored, versioned local pack defined by D-0005. Give tracks, sprites and animations stable logical identifiers, independent of source ROM offsets. Preserve offset provenance in extraction metadata; reject incomplete, corrupt or incompatible packs.
- **Visual replacement separate from collision:** record original anchors, logical dimensions and animation timing. A larger texture must not enlarge a rider's collider or alter a trick window.
- **Versioned formats:** record rules, state, replay, track and asset schema versions. Reject incompatible inputs clearly. Add migration support only when a format actually changes.

A temporary emulator-assisted build can accelerate discovery and comparison. Label which systems still execute original code. M3's native gameplay acceptance cannot be satisfied by secretly wrapping the original CPU execution in a modern window. Reusing a documented graphics/audio compatibility component is a separate decision, with its provenance and distribution implications recorded.

## Milestones

| Gate | Deliverable | Evidence required to advance |
| --- | --- | --- |
| M0 — Repeatable laboratory | Reproducible toolchain, identified ROM, automated reference run, durable tasks | Clean build on local macOS and Linux; synthetic checks; identical repeated reference playback; intentionally altered input detected; one task resumed by a fresh agent session |
| M1 — Map the relevant systems | Annotated code/data map, player-state schema, track investigation | Validated addresses and meanings for the selected sequence; state sampling point defined; each critical finding has an experiment; unknowns remain explicit |
| M2 — Native mechanics experiment | Native implementation of the short movement/trick sequence | Exact agreement on defined gameplay fields for the primary trace and at least two withheld input variations; native save/restore continuation agrees; first-divergence reports work |
| M3 — Playable native slice | One complete track, controls, rider animation, collision, tricks, race finish, local Classic content extraction and minimal frontend | Full-track recordings match scoped gameplay fields; real play confirms controls and readability; no original CPU execution for delivered gameplay; declared visual/audio omissions; clean checkout plus supported ROM reproducibly creates the pack and runs; a later launch succeeds from the validated pack with the ROM absent |
| M4 — Original game coverage | Remaining tracks, opponents, modes, local multiplayer, menus/progression and audio | Feature-by-feature coverage matrix, regression recordings, persistence checks and release testing on selected desktop platforms; remaining mismatches published |
| M5 — Custom-content release | External track packs, high-resolution texture packs and a usable track editor | Create/save/load/race a new track; replace a sprite/animation without altering gameplay; version compatibility and malformed-content checks; Classic regressions still pass |
| M6 — Online release | Network sessions for agreed player count and rules, including custom-content compatibility | Same state across clients, latency/loss/jitter tests, reconnect/disconnect behavior, content/rules checks, desync diagnostics, tested session flow and deployment plan |

M0–M3 form the first investment decision. Measure progress before estimating M4–M6. M5 format prototypes can run beside M4 after M3 stabilizes. An M6 two-client networking spike can start after M3 if serialization is reliable, but production online work depends on the chosen content/rules contracts. Completing every original menu is not a prerequisite to learning whether rollback is practical.

For M4, inventory actual game features from evidence; the table does not assert that every named system has already been confirmed in this ROM. For M3, maintain a scoped inventory so a polished demo cannot be mistaken for the full game.

### M4-12 investment gate (13 September 2026)

[D-0006](decisions/D-0006-capability-driven-work.md) and
[M4-12](../tasks/M4-12.md) replace one-producer-at-a-time task acceptance with an
Astra/medium trial delivering autonomous native ZOOM ZOO movement. First close
the accepted short window, then an expanded predeclared interval with relevant
independent variations and restore checks. Early native integration is in scope;
remaining gameplay dependencies are internal experiments. Full-track finish,
second-track presentation and the rest of M4 remain later outcomes.

Measure exact native interval/fields and branch coverage, dynamic captured inputs
remaining, review findings and allowance consumed. Accepted research-task counts
are not a proxy for playable coverage. No revised calendar or cost estimate is
supported yet. Historical estimates below remain historical observations.

Trial outcome: M4-12 accepted 200 exact native updates for both riders from one
seed, with 395-byte state, two untuned independent variations, three restore
boundaries and zero later captured runtime inputs. One independent finding was
corrected and re-reviewed. Startup-to-approval elapsed about 60 minutes and
account-wide usage grew 11 percentage points; unrelated work may contribute.
[R-0030](research/R-0030-zoom-zoo-native-trial.md) records the measured boundary.
This single trial does not support a causal model-performance claim or a revised
full-game estimate. M4-12 is complete. The user authorized preparation for
[M4-13](../tasks/M4-13.md) following [R-0031](research/R-0031-m4-12-retrospective.md):
a native player support-loss, landing and recovery sequence plus at least 100
subsequent updates, with two independent timing variations and restores around
landing. Freeze the event/horizon before tuning. Necessary control/camera
producers stay inside this task; finish/presentation stay excluded. Preserve
Astra/medium ownership and automatic Sol review, defer broad checks until initial
review findings are addressed, and reassess before M4-14. M4-13 is now accepted: player landing at 1745 plus 104 subsequent updates,
395 exact bytes for both riders, two fresh independent cases and all landing
restores. Two review boundary findings were corrected; all frozen gates pass.
See [R-0032](research/R-0032-player-landing-recovery.md). The user subsequently
selected [M4-14](../tasks/M4-14.md): continuous Right from end-1649 through at
least 3299 (1,650 updates / 33 seconds), extending if needed for 200 updates of
resumed progress after reference-defined recovery. Keep newly reached mechanics
inside one sustained assignment; no substitute early-jump case or shorter prefix.
This is a larger traversal gate toward full-race support, not a full-game claim.
Prepare docs before the final push to avoid redundant closeout CI. M4-14 is
accepted at `38c72e8`, but repeated the same section. The next
[M4-15](../tasks/M4-15.md) now demonstrates complete native race simulation
from that seed: both finishes and 240 player post-finish updates, with independent
review and debug/sanitizer gates. No fallback was used; `8bc2e71` is accepted
with synchronization to the then-private origin and final-tip CI.
[M4-16](../tasks/M4-16.md) is reviewed and integrated, with acceptance recorded in
[project state](STATE.md); see [the current handoff](../tasks/NEXT_SESSION.md). Its outcome is playable ZOOM ZOO: native initialization, live controls, readable
track/riders/HUD, correct result and restart, plus clean extraction and pack-only
relaunch. Recover all coupled dependencies inside this assignment; no seed-based
or headless fallback. Audio and broader menus/modes remain outside scope. No
automatic M4-17 dispatch or M4 milestone acceptance.

### Next-stage estimate, revised from observed effort

M2-01 checkpoint note (12 September 2026): the original small-riding-update estimate did not account for the demonstrated opponent-jump/contact dependency of player speed. Sampling and progress components are implemented, but the full task remains blocked on [M2-01A](../tasks/M2-01A.md). Re-estimate autonomous movement after that contract is recovered; do not treat the earlier 1–5 h range as a promise for the expanded work.

Revised 12 September 2026 from the M1-01 to M1-03 handoffs (harness-measured figures where the worker's own reading was wrong); the figures and their derivation are in [R-0009](research/R-0009-m1-acceptance.md). The previous revision (11 September 2026, from M0-01 to M0-05, [R-0005](research/R-0005-m0-acceptance.md)) observed about 9 h 55 min of worker sessions over five tasks with 11 review rounds, 6 of them returning a material finding, and estimated M1-01 to M1-03 at roughly 12 h and 6 to 15 rounds. Observed for M1: about **2 h 18 min of worker sessions** across the three tasks (about 30, 53 and 55 min), **no fix rounds** after any first candidate, and **3 review rounds** (10.5, 15.5 and 14.7 min), **every one of which returned a finding that changed a record** (a frame cited while the screen was black; two required corrections to the state schema; a mismatch class attributed to the wrong cause) and none of which required a code, manifest or digest change. M1 therefore took about a fifth of the estimated worker time and half of the fewest estimated rounds. Three patterns, all from this small sample:

- The M0 pattern "fix rounds cost as much as the first candidate" did not recur. Every M1 unknown was answered by a capture on the *unchanged* pinned core (the core-patch fallbacks of M1-01 and D-0002 were never needed), so no check had to be redefined; M0-03's five rounds were spent redefining checks.
- Reviews still find something every time, and what they find is in the reading of the evidence (a relation claimed for every frame that holds only while stationary; a mismatch explanation that missed the largest class), not in the tooling. One review round per task is the observed minimum, not a target.
- Workers misreport their own duration: M1-01's handoff overstated the session by more than four times and M1-03's clock reading during the session more than doubled it. The harness figure or `date` lines at the start and end are the record.

M2-01 (native movement experiment) is the first task that writes game code and reads arithmetic from the ROM rather than observing the core: the physics routine `$82:8AB1`–`$82:8DF8`, its reads of the decoded per-tile table and of the undecoded ROM tables in banks $20/$21, reproduced per PAL frame against the eleven declared fields with withheld input variations frozen first. Its closest analogue is M0-03 (4 h 20 min and five review rounds, each redefining a check), not the M1 tasks, because a native routine that disagrees with the reference at some frame needs the check itself examined. Taking the M1 pace (about 1 h per task, one round) as the floor and the M0-03 pattern as the ceiling, M2-01 is estimated at roughly **1 to 5 h of worker sessions and 1 to 5 review rounds**; if the bank $20/$21 tables must be decoded before the displacement arithmetic can be reconstructed, the task splits and the first half can absorb the whole estimate. M2-02 (state restore and portability), a tooling task like M0-04 but with the pinned core never yet built on Linux, is estimated at **1 to 2 h and 1 to 2 rounds** plus whatever a Linux build of the core costs, for which there is no figure. Together M2 is estimated at roughly **2 to 7 h of worker sessions and 2 to 7 review rounds**, with the spread, not the midpoint, as the honest figure.

These are effort figures for planning task order and concurrency. They are not a calendar date, a delivery promise or a cost: sessions are not metered here, no spend is authorized by this plan, and eight tasks on one host by two models is too small a sample to extrapolate beyond M2. Re-estimate M3 from M2's observed figures rather than from this one.

M2 acceptance update (12 September 2026): M2-01 expanded once into the explicit
M2-01A opponent/contact prerequisite; its reviews found one semantic translation
error and one GCC portability error. M2-02 completed in one implementation and
one independent review round with no remaining finding. PR5 then passed the full
macOS/Linux matrix and M2 was accepted; see
[R-0011](research/R-0011-m2-acceptance.md). M3 crosses more interfaces than M2
(complete-race reference coverage, remaining mechanics, controls, rendering and
frontend integration), so its initial planning range is **6 to 20 worker hours
and 3 to 8 review rounds**, split into bounded evidence tasks rather than one
long implementation. This is a prioritization range, not a date or cost promise.

M3 acceptance update (13 September 2026): M3-00 through M3-04 are accepted and
annotated tag `m3` identifies the final acceptance state. The supported PAL
one-player CRAWLER/DRAGSTER slice now installs from an exact-gated user ROM,
relaunches from its validated local pack with the ROM absent, runs entirely in
native gameplay code, accepts live controls and reaches a stable result. The
finite differential, restore, presentation, sanitizer and hosted macOS/Linux
gates are mapped in [R-0017](research/R-0017-m3-acceptance.md). This does not
expand acceptance to M4's remaining tracks, opponents, modes, local multiplayer,
menus/progression or audio; inventory those features before estimating M4.

## Future features without premature implementation

### Online play

Start with two players as a planning default, subject to product choice. Early replay and save/restore work prepares for network synchronization, but does not prove network viability. Benchmark state size, restore cost and simulation speed before choosing input delay, rollback, or an authoritative server approach.

A networking spike must model input sequencing, prediction/correction, random seed, session start, version/content hashes, and desync recovery. Rollback must reconcile presentation events so audio or effects are not duplicated. Test mismatched builds, jitter, packet loss, prolonged stalls and disconnects. Competitive ranking, anti-cheat, accounts, public servers and matchmaking are separate scope decisions with operating costs; do not assume them into the first online milestone.

### Tracks and editor

Use the original track decoder to learn which geometry and gameplay properties are necessary. Build one validated external track format before choosing a full editor framework. Keep editable source data separate from compiled runtime data. The first editor needs geometry placement, starts/checkpoints/finish, undo, validation, save/load and playtest. Sharing, discovery and collaborative editing can follow.

### High-resolution assets

Use an asset manifest mapping logical identities to replacement images, pivots, frame order and timing. Specify fallback to original art extracted locally under D-0005 and texture-size/memory limits. Check that a visual-only pack leaves simulation hashes unchanged. A public pack gallery or hosting service is later scope.

## Risks and decision rules

| Uncertainty | Early experiment | Response if it fails |
| --- | --- | --- |
| Emulator cannot be automated reliably | M0 cold-start replay and capture | Try one alternate adapter or a small pinned-core modification; keep gameplay work dependent on a working reference |
| State is incomplete or misidentified | M1 perturbations and independent traces | Narrow the task to the first changing field and its writers; revise the schema |
| Custom compression or track representation is difficult | Decode a small region and validate against runtime use | Preserve raw provenance; defer a general extractor until the small case is explained |
| Native physics appears plausible but diverges | M2 field-level differential comparison | Find the first divergence; investigate arithmetic and update order before tuning constants |
| Agents consume time without learning | Per-task hypothesis and experiment log | After repeated unproductive attempts, checkpoint and narrow or reassign the task |
| Parallel work causes integration drift | Small tasks, isolated worktrees and a single integration owner | Reduce concurrency around shared interfaces; merge prerequisites first |
| Cross-platform determinism breaks | Native replay on two hosts | Investigate serialization, arithmetic, iteration order and compiler behavior before networking |

Track accepted tasks, validated mechanics, replay coverage and resolved divergences. Do not use lines of code, tool-call count, or a guessed percent of ROM bytes as the headline progress metric. Report observed cost/time per accepted task when available, and revise the next milestone estimate from those measurements.
