# M4-16 initial independent review

## Scope and candidate

This is the required initial review of the **unaccepted** product candidate
`84dfa40db896e317022631e66d31b977fda8fbad` in the detached isolated checkout
`.worktrees/m4-16-review`. It is not final acceptance and intentionally does not
run broad, sanitizer, merge or CI suites. The primary remains responsible for
implementation and final evidence; this checkout contains no implementation
fixes.

Review started with shared weekly usage reported as 64% used versus 57% at task
startup. The final 20% review/recovery reserve remains intact. No reset,
purchase, provider switch, child dispatch, push or integration occurred.

The candidate and its task record already disclose incomplete ordinary
brake/trick controls, result/next-race producer inventory, restart/reference and
negative controls, independent cases, frozen visual contracts, live full-race
evidence and broad gates. The findings below identify concrete defects in the
implemented state contract and evidence harness rather than counting those
listed gates again.

## Findings

1. **Required correction — URZZ0005 admits impossible initializer phases that
   affect the next update.** `deserialize_zoom_zoo` validates `fade_level`,
   `start_boost` and `result_updates` separately, but does not validate their
   relationship with frame/countdown and race phase
   (`src/core/movement.cpp:1268-1274`). Starting from the authentic native frame
   1376 state, changing bytes 565-566 from fade 0 to fade 30 leaves countdown
   270 and both start boosts 384. The exact candidate accepts this state. One
   neutral update immediately decrements countdown, whereas the valid state
   first increments fade and retains countdown; the resulting state hashes are
   `aa551364...` and `98a6531c...`. This is a state the initializer cannot
   produce, and it changes future timing. Require phase relations for the
   initialization interval, including the reached fade/countdown/frame relation
   and start-boost availability/consumption, or encode a phase that makes those
   relations explicit. Add URZZ0005 malformed mutations and verify rejected
   operations preserve the caller's input state where that API contract applies.

2. **Required correction — malformed result restores can change the winner and
   graph.** The decoder does not validate completed lap-slot structure or the
   relationship between lap times and totals. From the authentic fully visible
   result state at frame 6839, changing player total bytes 507-508 from 9802 to
   60000 is accepted by both `zoom_zoo_runner` and
   `zoom_zoo_presentation_runner`; changing the first three player lap slots at
   bytes 467-472 to sentinels is also accepted. The valid, wrong-total and
   missing-laps renders have distinct PPM SHA-256 values `c1d2cffe...`,
   `9f9eb356...` and `a4cec35f...`; the first corruption changes the displayed
   outcome and the second changes the result graph. For this fixed three-lap
   scenario, enforce the reached completed-slot/sentinel ordering, total from
   the completed laps, and result-phase finish implications. The focused native
   test at candidate `84dfa40` exercises only the older serialized formats, so
   its pass does not cover either new malformed-state class.

3. **Required evidence correction — the playable comparator manufactures its
   post-load expected rows.** After original result loading begins,
   `tools/unirally_lab/native/zoom_zoo_playable.py:47-53` copies the last race
   archive, advances only frame/result count, and checks only SRAM lap/total
   bytes 467-510. It then compares native output with those manufactured rows
   and can report one `rows_sha256` and `status=passed` over frames 1376-7600.
   That result does not establish that any other post-load state was inventoried
   or recovered. This representation can be a valid product-level result model,
   but its report must distinguish exact race projection from semantic result
   phase and cannot support the task's complete future-state claim until a
   source-backed result/next-action inventory proves which state is future
   relevant. Include negative controls that mutate an omitted result producer
   and the displayed outcome/graph inputs.

4. **Required presentation correction for this candidate — rider art is below
   the accepted DRAGSTER behavior.** `render_zoom_zoo` overwrites both pose
   indices with `0x04f9/0x0263` and forces both reflections true on every frame
   (`src/core/presentation.cpp:990-997`). Position changes remain visible, but
   direction reversals, jumps and pose changes are not represented. This is
   weaker than DRAGSTER's recovered-pose/last-recovered fallback and cannot show
   the requested direction and jump response in the live presentation. The
   primary has separately reported replacing this after `84dfa40`; final review
   must inspect the replacement and its fallback metrics.

## Result and restart assessment

The original result video becomes fully bright at 6839, but it is not a frozen
image: authenticated primary video hashes at 6839/6840 agree, while frames 6900,
7000, 7100 and 7199 all differ. The contract's `stable_result` name is therefore
a phase label derived as `first_visible + 6`, not a tested pixel-stability fact.
Final evidence should call it fully visible or inventory the continuing result
animation before claiming stable visual output. Candidate native rendering
saturates `result_updates` at 115 and then remains static.

The product's explicit **Race Again** action is a defensible restart design.
Authenticated original Start after the result advances tour progression to
STUNT, so it is not an original ZOOM ZOO restart oracle. Reconstructing the same
one-player MIKE/BRONSEN CRAWLER/ZOOM ZOO scenario with
`classic_crawler_zoom_zoo_start`, assigning the whole state, and clearing input
avoids stale race fields. Acceptance still needs to compare the restarted state
with a separate fresh native initialization and continue both with the same
nontrivial controller prefix. The current playable comparator never invokes the
frontend restart and therefore cannot establish that property.

## Preregistered final-candidate variations

These two cases were chosen from the frozen primary timeline without running an
original or native candidate. They must remain untuned until a final candidate
is frozen. If an original case does not complete, retain it as a nonqualifier
and preregister a fresh replacement before native evaluation.

1. `m4-16-review-mid-race-brake-before-reversal`: override frames 3388-3403
   with `[left,y]`, then resume the primary timeline. This adds sixteen updates
   of braking during the long lap-two left segment immediately before its 3409
   reversal. It is distinct from the producer's late 6400-6410 loss case and
   pre-start charge case and exercises brake/contact/throttle ordering while the
   race remains live.
2. `m4-16-review-airborne-rotation-jump`: override frames 1681-1695 with
   `[left,b,r]`, then resume the primary timeline. This combines the primary
   route direction with a jump and positive shoulder rotation in the first-lap
   airborne window. The added rotation makes it materially different from the
   previously tuned B-only landing case.

## Focused commands and results

- `tools/project.py bootstrap --report artifacts/m4-16-review/bootstrap.json`:
  passed, isolated CMake 3.31.10 and Ninja 1.13.2.
- `tools/project.py build --preset app-debug --report
  artifacts/m4-16-review/build.json`: passed configure/build for exact detached
  candidate.
- `ctest --test-dir build/app-debug -R
  'zoom_zoo_trial|content_pack' --output-on-failure`: 2/2 passed
  (`classic_content_pack_identity_rejection` and
  `zoom_zoo_trial_width_state_and_guards`). The content test constructs only the
  legacy 25-entry pack; the exact local 46-entry pack was independently opened
  by the mutation runs through both new runners, but comprehensive extraction
  and corrupt-pack negatives remain for the final candidate.
- A temporary binary mutation runner invoked exact candidate
  `build/app-debug/src/core/zoom_zoo_runner` against the read-only local pack
  `5bd961f6...`. The valid/impossible initializer pair both exited zero and
  diverged on the first continuation update as described in finding 1.
- A second temporary mutation run invoked both exact candidate runners for the
  valid, wrong-total and missing-lap frame-6839 states. All six invocations
  exited zero and produced the distinct render hashes in finding 2.
- Read-only inspection of `artifacts/m4-16/result-audit` found an authenticated
  6725-7200 access capture with 6,770,790 instructions and zero unresolved
  stores, but no completed producer-to-semantic-state inventory. Read-only
  `restart-explore` confirms that post-result Start changes WRAM/SRAM and later
  reaches STUNT, consistent with the task's recorded exclusion.

## Disposition

Reject `84dfa40` as expected for an initial unaccepted candidate. Correct the
two malformed-state classes and the result harness claim before broad suites.
Re-review the updated result inventory, restart differential and DRAGSTER-style
rider fallback on an exact frozen candidate. Do not evaluate the preregistered
controller cases until that candidate is frozen.

## Targeted re-review — candidate 6c1d5ef

Targeted re-review used frozen candidate
`6c1d5effe3ee3f9a392e29eb1ce40800dbdd889e` in the same detached isolated
checkout. Shared weekly usage was reported as 67% used; the 20% final reserve
remains intact. This was a focused correction review while the primary's full
immutable start/result/restart run continued. No broad suites, preregistered
case evaluation, child dispatch, reset, purchase, push or integration occurred.

### Correction assessment

- The original impossible fade/countdown mutation now rejects with
  `inconsistent ZOOM ZOO countdown/fade phase`. The decoder derives the reached
  fade and countdown from the serialized frame, and it constrains unconsumed
  start boosts in the early countdown.
- The completed-lap slot, total-time and new result-publication mutations all
  reject. In an authentic primary result, original and native publication bytes
  agree exactly: update105/frame6829 is all zero; update106/frame6830 publishes
  graph minimum/maximum `0c10/0cd8`; update107/frame6831 additionally publishes
  totals `264a/2652`; all eight bytes remain unchanged through frame7600.
- `--restart-from` on the authentic frame7600 state deserializes in a fresh
  process, calls the shared `restart_zoom_zoo` operation, emits an exact copy of
  the original native frame1376 state, and accepted a neutral 1377-1379
  continuation. The comparator now requires the entire second race to equal a
  clean native start. Early restart is rejected before mutation in the authored
  core test.
- Live ZOOM ZOO now uses the same recovered-pair/last-recovered-pair policy as
  DRAGSTER and resets its cached presentation pair at restart. The renderer also
  applies the original prior-update fade rule. This resolves the fixed reflected
  poses in the reviewed candidate at the design level; final visible evidence
  and fallback metrics remain required.

### Remaining finding

5. **Required correction — result/finish phase can still be transplanted onto
   the initialization clock.** The new checks relate fade/countdown to frame and
   result publications to `result_updates`, but only require nonzero result
   updates to have `finish_delay == 240` (`src/core/movement.cpp:1295-1323`).
   They do not require a finished/result phase to have completed the countdown
   and fade.

   Starting with the authentic V6 frame7600 state, the review retained its
   finished riders, lap archive and `finish_delay=240`, then changed frame to
   1376, countdown to270, fade to0, start boosts to384/384, result updates to1,
   and the not-yet-published result words to zero. Every individually checked
   relation passes. Exact candidate `zoom_zoo_runner --seed` exits zero, emits
   the impossible result at frame1376, and advances it on the next update. This
   can never be produced by the native initializer and bypasses racing from the
   initial frame. Require `finish_delay == 240` or `result_updates > 0` to imply
   countdown zero and completed fade (plus any stronger reached phase relation
   supported by the frozen domain). Add the mutation to the URZZ0006 test.

### Result abstraction and restart boundary

URZZ0006 materially improves the result claim. Before result loading, 571 bytes
remain the exact original race projection. After loading, those bytes are
explicitly an archived final race whose lap/totals are checked against surviving
SRAM; eight appended bytes are actual original graph and displayed-total
publications; the two-byte load counter is explicitly semantic. The report's
`comparison_domains` now makes this distinction rather than presenting the
whole post-load row as original state.

For the chosen product flow this abstraction is defensible once the remaining
phase mutation and producer inventory are closed. The native result renderer
consumes archived lap slots plus the eight authenticated publications, and the
only supported next action discards the archive through a complete fresh-scenario
restart. Original tour selection and its transition to STUNT remain expressly
excluded. This supports a bounded product-state equivalence claim, not byte-exact
equivalence to overwritten original result WRAM. Continuing original result
animation and visual contracts still need their separately declared presentation
evidence.

### Focused re-review commands

- `git switch --detach 6c1d5ef` and `git rev-parse HEAD`: exact candidate
  `6c1d5effe3ee3f9a392e29eb1ce40800dbdd889e`.
- `tools/project.py build --preset app-debug --report
  artifacts/m4-16-review/rereview-build.json`: passed. Reviewed runner SHA-256
  was `dc45bd7d...`; presentation runner was `aa24ee75...`.
- `ctest --test-dir build/app-debug -R
  'zoom_zoo_trial|frontend_contract' --output-on-failure`: the selected
  `zoom_zoo_trial_width_state_and_guards` test passed; no test name matched the
  second expression.
- Temporary mutations of authentic V6 primary state exercised lap slot467,
  total507, graph573/575 and published total577. All rejected; the error was
  either inconsistent completed slots or inconsistent result publication.
- `zoom_zoo_runner --restart-from` on the authentic frame7600 V6 state emitted
  exact fresh state and frames1377-1379. The cross-phase mutation in finding 5
  was accepted by `--seed` and rejected by `--restart-from` only because its
  result count was below the restart gate.

### Targeted disposition

Do not approve `6c1d5ef` yet. Findings 1, 2 and 4 are corrected, finding 3 now
has a defensible explicitly bounded abstraction, and the native restart boundary
is substantially stronger. Correct finding 5, finish the source-backed result
inventory, and re-run the affected focused gate before final-candidate review.
The two preregistered controller variations remain untuned and unevaluated.

## Final-candidate review — candidate 4f8aaad

This review used exact detached candidate
`4f8aaada48a326eefacad264b5de3beebc0ada50`. The candidate built cleanly and
the focused `zoom_zoo_trial_width_state_and_guards` test passed. This remains a
task acceptance review: no implementation was changed here, and broad suites,
merge, CI, push and milestone approval remain with the primary.

### Independent controller cases

The two preregistered cases were captured on the original twice before any
native evaluation of each case.

- `m4-16-review-airborne-rotation-jump` overlays frames 1681–1695 with
  `[left,b,r]`. Both original runs were identical and completed: player/opponent
  finishes were 6483/6488, result loading was 6724, first visible result was
  6832 and the fully visible result boundary was 6838. Its frozen 632-byte
  contract has rows SHA-256 `71392af5...`. Exact candidate comparison passed all
  6,225 states from 1376 through 7600, 480 fresh-process restore points,
  a second clean native initialization, and the full restarted race.
- `m4-16-review-mid-race-brake-before-reversal` overlays frames 3388–3403 with
  `[left,y]`. Two original runs were deterministic, but it did not qualify at
  7600. Two further original-only runs through frame 10000 show a route failure,
  rather than a delayed finish: the opponent finishes at 6488 while the player
  remains at two laps remaining, checkpoint 2, next checkpoint 3, with no finish
  through 10000. The freeze correctly rejects `case must finish both riders`.
  The case and captures are retained as preregistered evidence and were never
  evaluated on native code.
- Following the preregistered replacement rule, I chose
  `m4-16-review-short-mid-race-brake-before-reversal` before native evaluation.
  It retains the material left-plus-Y moving brake at the same lap-two boundary
  but limits it to frames 3388–3395. The two original runs were identical and
  completed at 6481/6488, loaded at 6722, and reached the fully visible result
  boundary at 6836. Its frozen rows SHA-256 is `78bc9241...`. Exact candidate
  comparison passed all 6,225 updates, 476 restore points, repeated clean
  initialization, and the complete restarted race.

### State, result and source assessment

The URZZ0008 corrections resolve the review's state findings. The authored
cross-phase mutation now rejects, and focused exact-result mutations of graph
minimum, published total, either charge flag, queue cursors, cooldown, reward
weight, hint enable/tick/group and empty-display flag all exited 1 with the
appropriate publication, charge or announcement-state error. The independent
case comparisons also restore from initialization, racing, finish, loading,
fully visible result and final state, then restart from the final serialized
state in a fresh process and compare the entire new race with clean native
initialization.

The result abstraction is defensible for the declared fresh-scenario product
flow. Before loading, 622 bytes are original projections. After loading, those
bytes are explicitly the final-race archive, while eight result publication
bytes remain independently projected from original SRAM and the last two bytes
are a semantic loading clock. Rendering consumes the archived lap slots and the
eight publications; the only next action replaces the whole state through the
shared clean restart. This does not claim equivalence to overwritten original
result WRAM or original tour progression. `stable_result` in reports denotes the
fully visible boundary; original result animation continues changing afterward.

The source audit is adequate for the reached event-one producer. The read-only
`trick-long-audit` authenticates 6,394 whole-WRAM frames and its focused
1718–1785 instruction/access capture reports zero unresolved store PCs. The
implemented queue, hint, cooldown and boost paths cite the reached producers at
`$81C598-C5C8`, `$81BEA8-BEF1`, `$81C0CE-C18A`, `$81C02A-C054` and
`$83CDBC-CE43`. This evidence does not establish general multi-event rotations;
the implementation correctly keeps unrecovered reward classes outside its
claim.

### Presentation and acceptance finding

The current race scene is readable and materially improved from the initial
candidate. At frame 3208 the native renderer places the same checker line,
curved green/blue/yellow track, tiled background and both riders in the current
camera scene as the original capture. Live rendering now follows DRAGSTER's
recovered-pair/last-recovered-pair policy and resets that cache on restart, so
its art fallback is no worse than the accepted DRAGSTER design. The headless
renderer's fixed pair is only its fallback when no live art pair is supplied.

**Acceptance blocker:** candidate `4f8aaad` is still not an acceptable M4-16
product completion. The tracked visual contract itself remains marked pending;
the native result is an authored legible layout rather than the original result
art; required both-outcome screenshots and representative visual checks are not
present. More decisively, there is no visible actual-app complete race through
result and restart, nor the required independent live-control exercise. The app
still throws on A/X/Up/Down/Select/Start during racing, and the task has not
shown that those guarded controls are outside ordinary supported play. Headless
byte equality and scripted restart cannot substitute for acceptance items 5 and
6. Keep this candidate unaccepted until the primary supplies the live and visual
evidence and either recovers those ordinary controls or narrows them with
source-backed evidence.

### Focused commands and results

- `tools/project.py build --preset app-debug --report
  artifacts/m4-16-review/candidate-4f8aaad-build.json`: passed on the exact
  candidate; runner SHA-256 `ffde80ce...`, pack SHA-256 `5bd961f6...`.
- Four original-only 7600-horizon captures plus two 10000-horizon extensions
  used `tools.unirally_lab.native.zoom_zoo_playable_reference capture --case`.
  Each repeated pair was identical.
- `tools.unirally_lab.native.zoom_zoo_playable freeze` rejected the long brake
  case at both horizons and froze the two completing cases before native use.
- `tools.unirally_lab.native.zoom_zoo_playable compare` passed the airborne and
  replacement brake contracts with 480 and 476 restore boundaries respectively,
  including each complete fresh restart race.
- `ctest --test-dir build/app-debug -R
  'zoom_zoo_trial|zoom_zoo_runner|presentation_contract' --output-on-failure`:
  the selected `zoom_zoo_trial_width_state_and_guards` test passed; no other test
  name matched the expression.

### Disposition

The state, result, restart, producer-closure and deterministic independent-case
corrections pass this review. Do not accept M4-16 at `4f8aaad`: the required live
full-race/result/restart review and final visual evidence are still absent, and
ordinary guarded controls remain unresolved as a product boundary.

## V9 roll re-review — candidate edec610

This focused review used exact detached unaccepted candidate
`edec61048427d2c38be924c84518b519d606b5f0`. While its full X comparison was
running, the primary froze a newer V10 candidate and directed this reviewer not
to start further V9 comparisons. Accordingly this section records the completed
X gate, V9 state defect, pack/source assessment and additive reviewer freezes;
it does not approve V9 or evaluate the two prior cases on native V9.

### Pack, original projection and completed X gate

The review checkout rebuilt its own pack through the Python pack API and the
current two-track rules. The resulting experimental
`classic.pal.crawler.two-tracks.v3` pack contains 48 entries, has SHA-256
`301c7c74...`, and validates rules SHA-256 `b45e82a0...`. The two new ROM-only
entries are the 128-byte roll-pose table `8d10c152...` and 64-byte roll-direction
table `670f9225...`; accepted DRAGSTER v1 remains available independently.

Before native V9 evaluation, the existing repeated originals for the airborne
R case and replacement moving-brake case were projected into fresh additive
680-byte V9 freezes. Their rows SHA-256 values are `2215eab9...` and
`b8d1af3a...`. They were not compared with native V9 after the primary supplied
the newer-candidate instruction.

The primary's already frozen full X case was compared independently against the
task-local pack. Exact candidate V9 matched all 6,225 states from 1376 through
7600, including the full roll, finishes at 6482/6488, result loading at 6723,
fully visible result at 6837, 1,782 fresh-process restore boundaries, repeated
native initialization and the complete restarted race. Runner SHA-256 was
`168f83c5...`; rows SHA-256 was `f11f998e...`.

### Required state correction

6. **Required correction — V9 accepts an inconsistent active-roll pose base
   that changes the next collision state.** The decoder bounds the signed roll
   step and simple flags, but does not relate `pose_base` to the active step,
   prior orientation/reflection or authenticated roll table
   (`src/core/movement.cpp:1451-1463`).

   In the authentic X state at frame 1712, rider 0 has `step=-1` and
   `pose_base=0x0063`. Flipping only serialized `pose_base` bit 15 at byte 639
   is accepted. Continuing both states with the authentic frame-1713 controller
   row exits zero, but the corrupt state differs immediately at serialized
   gameplay bytes 74, 76, 88-89, 101 and 405-406, as well as the mutated roll
   byte. The bit changes the completion reflection branch and therefore pose and
   collision continuation. Reject an active roll whose pose base cannot be
   derived from the static roll table and its serialized orientation/reflection,
   and add this exact late-step mutation to the malformed-state test.

   V9 also accepts nonzero `held_updates`, `bounce_charge` and
   `held_rotations`, then throws only when the next active roll update reaches
   the explicit unrecovered guard. A frame-1712 `held_updates=1` mutation emits
   the seed state and then fails at frame 1713. Unsupported serialized
   continuation values should reject at decode rather than admit a state that
   cannot continue. The primary reports these paths recovered in V10, so the
   newer decoder should instead validate their reached relationships.

The duplicate support-count field behaves correctly in V9. A mutation rejects
before emitting a row, and the original access audit directly shows `$818E06`
copying `$0F33` to `$054B` and `$818F5A` copying the corresponding opponent
value to `$054D`. This supports treating the two words as redundant mirrors of
the already serialized contact support count, rather than bounce state.

The focused X access audit authenticates all 6,394 WRAM frames and captures
frames 1690–1760 around the roll. It reaches the projected roll producers and
the completion reads at `$8296C6-96E1`. This closes the one complete,
uninterrupted X path only. Held/released and interrupted X, general multi-event
reward behavior, Start and Select remain outside V9's proven domain.

### Focused commands and disposition

- `tools/project.py build --preset app-debug --report
  artifacts/m4-16-review/candidate-edec610-build.json`: passed.
- `tools/project.py content pack --rules
  tests/manifests/content/classic-crawler-two-tracks-pack.json ...`: passed exact
  ROM identity, atomic commit and all 48 entries.
- Two `zoom_zoo_playable freeze` invocations produced the additive reviewer V9
  contracts before native evaluation.
- Full X `zoom_zoo_playable compare`: passed 6,225 states and 1,782 restores,
  including the full restart race.
- `ctest --test-dir build/app-debug -R
  'zoom_zoo_trial_width_state_and_guards|classic_content_pack_identity_rejection'
  --output-on-failure`: 2/2 passed. Those tests do not cover the late active-roll
  pose-base mutation above.

Reject `edec610` as a final candidate. Its reached uninterrupted X behavior and
support mirror pass, but malformed active-roll restore is future-changing, and
held/interrupted X plus the live/visual product gates remain incomplete. Recheck
the pose-base invariant and recovered held/release fields on the newer frozen
candidate.

## V10 correction review — candidate fe02cd86

This final bounded recovery review used exact detached candidate
`fe02cd86d3a7fb0d16ccae89876328812d1a1594`. Shared weekly usage reached the
80% D-0004 reserve boundary; no new feature scope, broad suite, live helper,
integration, push or acceptance work occurred in this checkout. Review was
limited to the V9/V10 restore findings, held-X gate, recovery documents and the
coordinator's documentation-only main pointer.

### Correction assessment

The V9 active-roll defect is corrected. Starting from the authentic full-X
frame 1712 state, each of these mutations rejects before the runner emits a
state:

- toggling `pose_base` bit 15: `inconsistent ZOOM ZOO active roll reflection`;
- toggling one low pose-base bit: `ZOOM ZOO roll base differs from static entry
  pose` during content-dependent runner validation;
- setting bounce charge, bounce-active or prior-step state: `invalid ZOOM ZOO
  roll state`;
- changing the support-count mirror: `inconsistent ZOOM ZOO support-count
  mirror`.

The first V10 candidate still admitted a future-changing held-counter mutation.
Authentic held-X frame 1712 has signed step -5, positive held duration 6 and
accumulated held rotations 6. Changing only duration to 7 was accepted and an X
re-press changed the learned event-17 weight from 6 to 7. Candidate `fe02cd86`
now rejects this seed before output with `inconsistent ZOOM ZOO held roll
counters`.

The correction avoids the over-strict equality initially considered during
review. Source `$829398-9422` retains accumulated rotations on a new pre-landing
roll, while `$82955F-9598` advances positive held duration and accumulated
rotations together. Therefore positive duration may be less than accumulated
rotations, but cannot exceed it before counter wrap. The decoder implements
that inequality and bounds held magnitude, accumulated rotations and completed
rolls by elapsed updates while fewer than 65,536 updates have elapsed. The
authored test accepts duration 6/rotations 7, rejects 7/6 and rejects a counter
larger than elapsed time. This is a sound bounded invariant; negative return
states remain explicitly less constrained rather than being assigned an
unsupported equality.

### Independent held-X result

The complete held-X comparison passed on exact `fe02cd86`. Both original runs
were frozen before implementation. Native matched all 6,225 730-byte states
from frame 1376 through 7600, including finishes at 6483/6488, result loading at
6724 and fully visible result at 6838. It passed 768 fresh-process restore
boundaries, repeated clean initialization and the entire restarted race. The
review binary SHA-256 was `44b5d64a...`; independently rebuilt 49-entry v4 pack
SHA-256 was `83a1c902...`; rows SHA-256 was `c8fb39e2...`.

Focused debug tests for state/guards and content-pack identity passed 2/2. The
primary separately records the corrected primary as 6,225 states, 757 restores
and full restart, plus focused debug and sanitizer passes. Those primary results
were not rerun or reclassified as independent evidence here.

### Recovery documentation and main pointer

Tracked R-0035, task checkpoint and NEXT_SESSION clearly state that M4-16 is an
incomplete experiment. They distinguish the 720 projected/archive bytes, eight
original result publications and semantic two-byte load clock; preserve the
nonqualifying re-press case; identify incomplete producer closure; and list the
live, visual, control, bootstrap and regression gaps. R-0035 also records the
held-counter review finding and bounded correction without claiming a complete
reachability proof. The proposed source-comment correction for the roll
reflection and pose-bit publication, from `$8295B5-95D5` to verified
`$8295D5-95F6`, is accurate and changes provenance text only.

The coordinator's main change at accepted ancestor `ede4c0b3` contains only
`docs/STATE.md`, `tasks/M4-16.md`, `tasks/NEXT_SESSION.md` and
`tasks/README.md`. It states that main retains accepted M4-15 gameplay, labels
M4-16 in progress and unaccepted, points unambiguously to the existing task
branch/worktree and private closeout, and forbids a replacement task or M4-17
dispatch. `git diff --check` passed. This reviewer approves that
documentation-only recovery pointer; it contains no gameplay/source integration
or acceptance claim.

### Commands and disposition

- `tools/project.py build --preset app-debug --report
  artifacts/m4-16-review/candidate-fe02cd86-build.json`: passed.
- Task-local v4 pack extraction from the exact ROM passed all 49 entries before
  the candidate correction and remained byte-identical for this source-only
  change.
- Exact frame-1712 mutation scripts produced the rejection and retained-state
  results above.
- `tools.unirally_lab.native.zoom_zoo_playable compare` on held-X passed 6,225
  states, 768 restores and the full restart race.
- `ctest --test-dir build/app-debug -R
  'zoom_zoo_trial_width_state_and_guards|classic_content_pack_identity_rejection'
  --output-on-failure`: 2/2 passed.

The reviewed V9/V10 restore defects are corrected in `fe02cd86`; no further
defect surfaced in this bounded correction review. M4-16 remains unaccepted.
Bounce and compound rewards, Start behavior, wrong-direction boundary and full
producer closure remain incomplete. Required live complete racing through
result/restart, independent live input, frozen representative visuals, clean
bootstrap/denied-access coverage, latest full regressions, merge and final CI
are also outstanding. Preserve this as recovery work on the task branch; do not
merge the experimental gameplay or create an M4 tag.

## Initialization/result closure review — checkpoint e0d2dba

This read-only follow-up reviewed exact checkpoint
`e0d2dba1ad47e9c0347de9d6f3319dfd917ff7c9`, whose gameplay is unchanged from
the corrected V10 candidate. It inspected the authenticated original captures,
R-0035, the native initializer, result publication and standalone restart. It
did not evaluate new controller cases or change native source.

### Initial scenario inputs

The original initialization capture authenticates the implemented state
producers. At frame 1291 `$82D89D-D904` reads the decompressed fixed track
header and writes rider positions 9200/1488 and camera origin 8944/1232;
`$82DB25-DB7F` writes 60000 into both ten-slot lap arrays and totals;
`$82DB96` reads `$77074B=1` before `$82DBAF/$82DBB2` publishes four line
crossings (three laps), and `$82DBA6` reads `$770744=3` before
`$82DBBA/$82DBBD` publishes cap 448. The same capture directly establishes
countdown 270 at `$82D844`, boosts 384 at `$82D897/$82D89A`, player/opponent
reward weight copies at `$82DB8B/$82DB8F`, tutorial active 1/update 30 at
`$82D95C/$82D975`, and the broad zero clear at `$82D7C6-D7FA`.

The fixed scenario nevertheless needs an explicit input ledger before a
producer-closure claim. The initializer also reads fresh persistent bytes
`$770750=00`, `$770748=00`, `$770749=11`, `$77074A=01`, `$77074B=01`,
`$770744=03`, `$771116=0000` and `$77111A=0000`. The native entry point accepts
only the fixed v4 pack and hardcodes the resulting state/presentation. That is
consistent with the declared one-player MIKE/BRONSEN CRAWLER/ZOOM ZOO
three-lap product; it is not support for other riders, modes, tracks, lap
settings, persistent tutorial state or tour records. R-0035 names the scenario
but does not yet map each of these reads to a native field, fixed presentation
choice or excluded menu/tour input. This is an evidence/documentation omission,
not a mismatch in the tested initialization: the full 730-byte frame-1376
comparison and fresh restart already match the two identical original runs.

### Result publication and transient presentation

The eight projected result bytes have closed producers for this fresh scenario:

- `$83904A-90F0` reads both ten-slot current-race lap arrays
  (`$770755-0768`, `$7707BF-07D2`) plus fresh prior-record sentinel
  `$770424=60000`, then writes graph maximum 3288 to `$771071` and minimum
  3088 to `$77106F` at original frame 6830. Native derives the same values at
  result update 106 from the serialized lap archive, using 60000 as the
  declared fresh-record sentinel and enforcing the same 200-centisecond span.
- `$80F88D-F8AF` publishes current-race totals 9802/9810 to
  `$770618/$77061A` at frame 6831. Native publishes the two serialized totals
  at result update 107. The native result renderer derives each current-race
  best lap from the same serialized lap arrays and derives winner from the two
  totals.

Other reached reads are presentation or persistent-tour state rather than a
missing next-race simulation input. `$770748=0`, `$770749=17` and `$77074B=1`
select fixed result labels/layout; native explicitly draws MIKE, BRONSEN,
three laps and ZOOM ZOO. `$770551=4112`, `$77106B=1026`, `$7710AD=1` and the
bulk `$77000C-073A` reads feed the original result graphics/text pipeline.
`$77082B` changes from the fresh 59999 sentinel to best lap 3250 at frame 6831,
and `$771118` changes during the result transition; these are persistent
record/tour side effects deliberately discarded by the reviewed standalone
Race Again design. `$770742` is the original screen/transition control retained
on the stable result. Native replaces that loading/display control with the
bounded semantic counter and only admits Race Again once it reaches 115.

A focused authenticated capture resolved the apparent non-ROM block-move
residual rather than exposing a defect. At frame 6831 generated code
`$000199` executes 48 `MVN` iterations and `$00019C` executes three `RTL`s.
The three 16-byte moves are:

- `$83A003-A012` (`zoom_zoo`, followed by other static labels) to scratch
  `$0000DE-00ED`, entered through `$809B72` with A=15, X=A003, Y=00DE;
- `$77000C-001B` (MIKE label) to the same scratch range through `$809B4E`;
- `$77011C-012B` (BRONSEN label) to the same scratch range through `$809B4E`.

The access report correctly records the ROM move under `rom_reads`, the two
SRAM moves under `accesses`, and the generated operands from the preceding
`$019A/$019B` writes. Its `end_of_frame` label applies to the unchanged opcode
byte, not all source operands. This corrects the review's preliminary concern
that the aggregate had lost the bank-$83 source.

The result audit still has 11,738 unresolved reads, all before frame 6795; there
are none from stable result frame 6839 through 7200 and no unresolved stores.
They prevent a global zero-residual claim, but the inspected data contains no
unaccounted supported-domain simulation input after the race archive. The
remaining closure work is to record a field/read classification for the
loading interval and bind the render-only class to the required frozen visual
checks. Exact original result rendering is not implied by the successful eight
byte/clock comparison: the native result is an authored semantic renderer, and
representative visual acceptance remains outstanding.

### Restart conclusion and commands

For the explicitly chosen standalone fresh-scenario restart, no missing
future-state input was identified. `restart_zoom_zoo` requires stable result
update 115 and reconstructs the same initializer, so it intentionally drops
`$77082B` and tour-transition state rather than pretending to implement the
original Start-to-STUNT continuation. Existing full restart comparisons prove
that all 730 represented fields equal a direct fresh race for the fixed pack.
They do not prove alternate persistent defaults or tour continuity.

Focused commands/results:

- Python inspection of both `boundary-a/b` raw SRAM series confirmed the
  persistent values above and identical transitions at frames 1207, 1291,
  1376, 6725, 6830, 6831, 6839 and 7200.
- Python classification of `initialization-audit/access.json` and
  `result-audit/access.json` mapped the cited PCs, bus addresses, widths and
  frames. Initialization reports 23,518 unresolved reads/zero unresolved
  stores; result reports 11,738/zero, with no unresolved read after 6794.
- `python3 tools/project.py access capture --manifest
  artifacts/m4-16-review/boundary-manifest.json --out
  artifacts/m4-16-review/result-mvn-watch2 --from-frame 6725 --to-frame 6831
  --watch-address 0x019a --watch-address 0x019b --watch-pc 0x000199
  --watch-pc 0x00019c --watch-pc 0x809b4e --watch-pc 0x809b72
  --wram-series-range 0 0x2200 --timeout 600 --report
  artifacts/m4-16-review/result-mvn-watch2-report.json`: passed, 1,690,660
  authenticated instructions and 717,378 accesses; access record SHA-256
  `51a86f6352f1383442cab11edb10537efff4c9a0257c1cb06508a9f3f1cffc8b`.

Disposition: no new native initializer/result/restart bug was found in the
fixed fresh scenario. Producer closure remains incomplete as documented because
the persistent-input ledger and result loading/render classification are not
yet durable acceptance evidence, and frozen result visuals remain absent.
Alternate scenario defaults, persistent record carryover and original tour
continuation remain unsupported by design. M4-16 remains unaccepted while
ordinary controls/rewards, visual/live product evidence and final gates are
unfinished.

## Generic landing reward source audit

This source-only follow-up used the same supported PAL ROM and an authenticated
fresh compound-roll capture. It reviewed `$829B69-9D97`, the enqueue routine
`$81C598-C5C8` and both player/opponent consumer paths `$81C0CE-C357`. It did
not inspect or change the primary's evolving native implementation and does not
accept the task.

### Combination table and voice events

The four five-way landing counts form an exact radix-five index. The three word
tables at `$829D98`, `$829DA2` and `$829DAC` contain respectively
`[0,125,250,375,500]`, `[0,25,50,75,100]` and `[0,5,10,15,20]`; `$829CD8`
adds the fourth count directly. The resulting range is 0-624, so `$829DB6`
is a complete 625-byte (`5^4`) table. Its SHA-256 is
`a6424f66bbd354883f3bfe128487d6cab92add5651adbf969976733a353f0217`;
it contains 404 `FE` bytes, 16 `FF` bytes and 205 values in `00-0F`.

The table byte is a gate, not the queued event. `$829D35` compares its low byte
only with `FE`; `FE` branches past the voice path, while every other value,
including `FF`, permits it. `$829D3A` then overwrites A with direct-page `$A5`,
masks its low nibble and combines it with the rider identity at `$770748/0749`
and base `0x48`. `$A5` is the corrected incoming X-position scratch:
`$818D1B/$818D97` copy player `$0415` to it,
`$818E75/$818EF1` copy opponent `$0417`, and collision correction updates it at
`$82A649-A651`. This agrees with the established position provenance in R-0021.
For the fixed identities, the calculated event is therefore
`72 + (player_x & 15)` for MIKE and `200 + (opponent_x & 15)` for BRONSEN.

The two calls are intentional duplicates. `$829D61` loads only Y and calls the
enqueue routine with the calculated event still in A. After returning,
`$829D6A` reloads the same byte from `$0260`, and `$829D6D` enqueues it again
with the same rider selector. There is no remaining path by which the table
classification byte reaches the first call.

Both consumer halves compare an event with decimal 72. Events 72 and above
jump to the common `$81C357` continuation and bypass reward class, learned
weight, feature-total and boost updates. Thus the duplicate calculated voice
events can affect queue order/timing and presentation, but are not themselves
two gameplay rewards.

That broad original consumer comparison relies on constrained producers. In
this fixed scenario, calculated voice events are only player `72-87` or
opponent `200-215`; it does not justify accepting every byte at least 72 in a
serialized queue. A future frozen candidate should reject forged high queue
entries outside the rider's source range, because even a voice-only entry
changes queue and cooldown timing. This is a prospective malformed-state check,
not a finding against the evolving implementation excluded from this pass.

### Event 18

Event 18 is independent of the combination-table voice path. `$829D0C-D14`
submits scratch `$0236`, which `$829CD8-D18` derives as 17 plus the fourth
landing count; one completed roll therefore submits event 18 before the table
gate is checked. For player event 18, `$81C0FD-C18A` uses class byte `0x18` at
`$81C50A[17]` and reward word `0x0080` at `$81C493[17]`. When its learned
weight `$7E20E8+17` is nonzero, the consumer adds that weight to the feature
total, halves the stored weight with a minimum of one, adds 128 to horizontal
boost and 64 to vertical boost, and updates the two persistent class counters.
This is a gameplay-producing event and cannot be represented as voice-only.

The companion persistent-input/result ledger is
[M4-16-persistent-input-ledger](../docs/research/M4-16-persistent-input-ledger.md).
It records the fixed values, reached uses, eight result publications and the
fresh-restart exclusion rather than treating unresolved access totals as
closure.

### Focused evidence

- ROM byte inspection and a mode-aware static disassembly covered
  `$829B69-9D97`, `$81C598-C5C8` and `$81C0CE-C357`; table sizes, values and
  hashes were computed directly from the identity-gated ROM.
- `tools/project.py access capture --manifest
  artifacts/m4-16-review/compound-roll-manifest.json --out
  artifacts/m4-16-review/compound-reward-watch --from-frame 1695 --to-frame
  1740` with explicit source/queue PC watches and `$7E00A5` watch passed. It
  authenticated 845,988 instructions and 420,746 accesses, with zero unresolved
  stores, zero non-ROM PCs and access SHA-256
  `3f18a4d0060763052b775c7abbb29caedcf937859a7fd798cfb0f3ac03c3e1e2`.
- The capture observes `$829B69` on both riders, player event 14 entering
  `$81C598` at frame 1720, and player consumer events 45, 46 and 47 at frames
  1707, 1720 and 1736. The particular L-shoulder compound trace does not reach
  the new event-18 path, so event-18 behavior above is a source/table result,
  not a claim that this capture exercised it.

Disposition: the source establishes the radix table, duplicate calculated
voice enqueue and event-18 gameplay consumer. It exposes no initializer/result
or standalone-restart defect in the fixed scenario. Native parity for the
primary's separate event-18 compound case remains implementation and frozen
differential work. M4-16 remains unaccepted; source closure, visuals, live
complete play and the other recorded capability gaps remain open.

## Candidate `239ae836` landing, restore and pause re-review

This bounded pass reviewed exact candidate
`239ae83642409bc6613c9c64f01c44ce5e729a76`. It preserves the independent
early compound case frozen before native evaluation and does not accept
M4-16.

### Severity findings

1. **High: the generic opponent reward producer exceeds the recovered native
   consumer domain.** `update_zoom_landing_rewards` submits opponent landing
   events 1 through 21 to `enqueue_zoom_opponent`, but
   `update_reward_queue` still implements a non-leading reward only for event
   1. Original `$81C219-C2C9` has the corresponding opponent learned-weight,
   feature-total, weight-halving and full vertical-boost path. The serialized
   second learned-weight bank is otherwise unused by queue consumption.

   A seed derived from authentic reviewer frame 1718 had opponent queue
   `read=1`, `write=2`, cooldown 8. Setting only entry 2 to event 2 and the
   write cursor to 3 is accepted and emitted, then fails at frame 1722 with
   `reward queue left the recovered event-one domain`. The same intervention
   with event 1 continues successfully. This is a future-state recovery gap,
   rather than proof of a malformed producer: the new generic landing routine
   can produce event 2 for the opponent. A requested artificial original-only
   event-2 probe was stopped before launch when the coordinator requested a
   bounded handoff; it is not claimed as evidence here.

2. **Resolved build blocker:** exact predecessor `db89d586` did not build the
   visible app with the configured warnings-as-errors flags. Local
   `previous_frame` at `src/app/sdl_main.cpp:384` shadowed the rendering-frame
   cache at line 314 (`-Wshadow`). Candidate `239ae836` renames the local,
   builds the complete app target, and records that the earlier chained command
   masked the app failure. This review found no remaining build error.

The new high-event restore constraints behave as intended. A player queue
byte 100 now rejects before output with `invalid ZOOM ZOO player voice event`;
an opponent queue byte 199 rejects with `invalid ZOOM ZOO opponent voice
event`. Voice-source ranges 72-87 for MIKE and 200-215 for BRONSEN remain
consistent with `$829D3A-D6F`. Low opponent reward events cannot be classified
as malformed until the reached consumer domain above is implemented or
excluded with original evidence.

### Independent complete case

`m4-16-review-early-compound-reverse` adds X+R at frames 1681-1720. Its two
pre-native original runs are identical: player/opponent finish 6483/6488,
loading 6724, first visible result 6832, stable result 6838, player win. The
frozen rows SHA-256 is
`3113fa666d37944c965e62aac3925504ecc844a1fdf4082e5d9f6985ff62efd1`.
Exact candidate `239ae836` passes all 6,225 observations, all 742 serialized
bytes, 739 fresh-process restores, repeated clean initialization and the full
fresh restarted race. Runner SHA-256 is
`00fe978961d8f693be1e6104653ec8ab840773b00c255f9a96ac6294394df0f7`;
pack SHA-256 is
`b9c5f0ea3dec6a6458d53ec682128cefbde172c82ca4f1ee27ea5b2b6d852647`.

### Pause/restart and presentation assessment

The authored `RESUME` / `RESTART RACE` pause menu is compatible with the
fixed standalone-race scope. Original Down/Start Retire enters excluded tour
progression; the native label and comment explicitly avoid an emulation claim.
The core replaces the whole state with the shared fresh initializer, and the
app detects the lower simulation frame, clears keyboard/gamepad masks and
resets retained presentation art. This prevents the held event state from
immediately pausing the replacement race. Core tests prove both direct paused
restart and Start-selected paused restart serialize exactly as a clean fresh
initializer. Actual keyboard/gamepad live restart remains necessary product
evidence.

The representative comparison image
`artifacts/m4-16/visual-7c3e3b6/comparison.png` is readable but not a live or
pixel-fidelity approval. Native omits the original large direction arrow and
pink `MORE STUNTS` coaching cue in the early race; rider scale/anchors differ
by roughly 8-14 pixels in representative lap-line scenes. The authored result
communicates winner, totals, best lap and graph, but its small colored graph
points have lower salience than the original rider-icon graph. These are
concrete presentation limitations, while the current track/camera/HUD and
result text remain usable in the captured frames.

### Commands and disposition

- `cmake --build build/app-debug -j4`: passed for `239ae836`; the same command
  failed for `db89d586` at the shadowed local above.
- `ctest --test-dir build/app-debug -R
  'zoom_zoo_trial_width_state_and_guards|classic_content_pack_identity_rejection|frontend_contract'
  --output-on-failure`: 2/2 selected tests passed (the regex selected the two
  named core/content tests; the app itself was covered by the full build).
- `python3 -m tools.unirally_lab.native.zoom_zoo_playable compare --reference
  artifacts/m4-16-review/early-compound-reverse-a --repeat
  artifacts/m4-16-review/early-compound-reverse-b --contract
  artifacts/m4-16-review/rereview-inputs/zoom-zoo-playable-review-early-compound-reverse-v11.freeze.json
  --binary build/app-debug/src/core/zoom_zoo_runner --pack
  local/classic-crawler-two-tracks-v5-review.pack --out
  artifacts/m4-16-review/early-compound-reverse-239ae83-compare.json`: passed,
  6,225 observations and 739 restores.
- Direct `zoom_zoo_runner --seed ... --content-pack ... --inputs ...`
  mutations are preserved under
  `artifacts/m4-16-review/voice-mutation-239ae83/`.

M4-16 remains unaccepted. The opponent learned reward consumer is a concrete
gameplay/future-state omission. The recorded incomplete long-control cases and
the primary's later bounce work are recovery evidence only. A complete visible
live race with ordinary controls, pause/restart and result restart is still
required, together with final candidate gates and independent acceptance
review.

## Candidate `aeb62e0` opponent reward consumer review — 14 September 2026

**Reviewing model: Claude Opus 5** (effort high), a fresh subagent with no
inherited conversation, in the isolated detached checkout
`.worktrees/m4-16-review`. This task's earlier reviews were Sol/medium; the
provider changed by user instruction (recorded in D-0004 and `tasks/M4-16.md`),
so this record names the actual reviewing model as D-0006 requires.

Exact candidate: `aeb62e07a9f1750453d9c6cd8bcb2715a52aed6a` on
`codex/m4-16-playable-zoom-zoo`. Range reviewed `9f4c423..aeb62e0`
(`fbc9cae` documentation, `aeb62e0` implementation). Source was kept immutable
throughout; the complete-case diagnostic re-verified `source_commit`
`aeb62e0`, an empty working diff and unchanged binary/pack at the end of its
own run. This pass also re-reviews the bounce/late-roll restore domain of
`ca46025`, which had had no independent review. It does not accept M4-16.

Reviewer inputs: ROM SHA-256
`a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`; core
`e59bf88d4fc922c9fe3b5438e65ff3a6909d24e1628f0f87141c8de17699a91b`; pack
`local/classic-crawler-two-tracks-v5-review.pack`
(`b9c5f0ea3dec6a6458d53ec682128cefbde172c82ca4f1ee27ea5b2b6d852647`);
reviewer build `build/app-debug`, runner
`f09dfdb47597f094274446ec4bc6196ed3f744ca7eb94aa1db3fdba3cf2b02e8`.
Disclosed honestly: `local/emulators` in this checkout is a symlink to the
shared repository-root emulator directory, so the core bytes are the same
audited `bsnes_libretro.dylib` the primary uses, gated by the SHA above rather
than by an independently built binary. The disassembly below is the reviewer's
own, from a mode-tracking 65816 decoder written for this pass over
`tools/unirally_lab/coverage/opcodes.py`; no primary artifact was used as
evidence.

### Verdict: reject with findings

The recovery itself is correct and I reproduced its central claim
independently. Four findings must be fixed before acceptance; three of them are
a few lines. Nothing here disputes that `$81C219-C2C9` is now modelled.

### Severity findings

1. **Required correction — the source fact justifying the new 200-215 branch
   is false, and the C++ implements a test the original does not.**
   `src/core/movement.cpp:642-644` and `:1663-1666` (and the commit message)
   state that `$81C238` "branches on a *signed* comparison with 72". It does
   not. Disassembled from the routine's own `SEP #$30` at `$81C219`:

   ```
   $81C238: C9 48        CMP #$48
   $81C23A: 30 03        BMI $81C23F
   ```

   `BMI` tests bit 7 of `(A - $48) mod 256`, so the reward path is taken for
   `A` in `[$00,$47]` and `[$C8,$FF]` — 0-71 and 200-255. A signed
   `int8_t < 72` test is taken for 0-71 and **128**-255. `movement.cpp:646`
   implements `static_cast<std::int8_t>(event)<72`, so it diverges from the
   original for events 128-199.

   Refuted empirically, not only on paper. The reward path increments a
   cartridge class counter at `$7707D5+((class*2)&$FF)` (`$81C24E-C258`) before
   any weight test; the voice path at `$81C23C` jumps to `$C357` and touches
   none of it. Using the same artificial original-only intervention shape as the
   candidate's probe (identity-gated ROM/core, pre-intervention frames
   1207-1717 hash-verified against `early-compound-reverse-a`), injecting one
   opponent queue entry at frame 1718 and reading `$7707D5-$7708D4` at 1724:

   | injected event | expected counter if reward path | observed change |
   | --- | --- | --- |
   | 100 (undisputed voice) | `$770807` | none |
   | 150 (disputed) | `$7707F5` | **none** |
   | 205 (undisputed reward) | `$770801` | `$770801` 0 to 1 |

   Event 150 took the voice path. A signed comparison would have put it on the
   reward path. Not reachable today — `deserialize_zoom_zoo` rejects opponent
   entries >=72 outside 200-215 and the landing producer never emits 128-199 —
   so this is not a live gameplay defect. It is a false recovered-source claim
   stated three times as the load-bearing justification for the new branch,
   which AGENTS.md specifically forbids, plus a latent divergence for anyone who
   later widens the restore domain. Fix: `else if(event<72 || event>=200)` and
   restate the comments as the N-flag test they are.

2. **Required correction — the probe can report success without exercising the
   recovered consumer, and nothing enforces the repeat the commit message
   claims.** Reproduced:

   ```
   capture --event 21 --before-frame 1718 --through 1720   -> rc 0
   native  --probe .../rev-probe-vacuity2                  -> "status": "passed", rc 0, 3 observations
   ```

   The injected entry is consumed at frame **1722**, outside that window, so
   the run proves nothing about `$81C219-C2C9`, yet the tool and its exit code
   report success. Nothing asserts that `read_cursor` ever reaches
   `intervention.entry_index`, or that any reward field moved. Related gaps in
   the same tool: `guards_tripped` is computed per frame, stored and never
   allowed to fail a capture; and unlike `zoom_zoo_playable.freeze` there is no
   subcommand that requires two independent captures to agree, so
   "each is captured twice and the repeats are identical" is an unenforced
   manual step. The probe is also self-certifying — its `rows_sha256` comes from
   the same run it compares against, with no external frozen contract. Fix:
   assert consumption inside the window, fail on a tripped guard, and add a
   two-capture freeze step.

3. **Required correction — the candidate removes the last constraint on the
   opponent's `event_one_weight` and adds no deserialize guard, although the
   player's identical field has one.** `deserialize_zoom_zoo` constrains
   `player_announcements.queue.event_one_weight` to {1,2,4} and its cooldown to
   <=120 (`movement.cpp:1600-1602`). `movement.rewards.event_one_weight` and
   `.cooldown` have no domain guard anywhere; `deserialize_movement_state`
   checks only the cursors. Until this candidate, `update_reward_queue` still
   threw on `event_one_weight==0`; it now correctly skips, faithful to
   `$81C260-C266`, leaving the field unconstrained. From the frozen 742-byte
   probe seed with the published opponent entry set to event 1:

   | mutation | result |
   | --- | --- |
   | weight 0 | accepted, no reward, feature total stays 4 |
   | weight 3 | accepted, feature total 4 to 7, weight to 1 |
   | weight 250 | accepted, feature total 4 to 254, weight to 125 |
   | cooldown 60000 | accepted |

   The bank is initialized from `$82D7A4`[0] = `$04` and halved with a floor of
   one, so the opponent's event-one weight can only ever be 4, 2 or 1, and this
   queue's own formula caps its cooldown at 40. Weights 3 and 250 were already
   admitted before this candidate and the cooldown gap is pre-existing; what is
   new is that the incidental zero-weight rejection is gone with nothing in its
   place. Fix: mirror the player's guard on the opponent queue.

4. **Required correction — the `ca46025` bounce restore domain admits states
   the original gates out.** `bounce_charge in {0,160}` is applied to both
   riders. Reproduced against the frozen 742-byte probe seed:

   | mutation (offset) | result |
   | --- | --- |
   | rolls[1].bounce_charge = 160 (668) | accepted, diverges from baseline within 13 frames at opponent offsets 144-210 |
   | rolls[0].bounce_charge = 160 (644), pose_override not `$9A8` | accepted, diverges within 13 frames |
   | rolls[1].bounce_active = 1 (674) | accepted, alters the opponent queue by frame 1730 (offsets 291, 321, 322) |
   | rolls[0].bounce_charge = 161 | rejected, `invalid ZOOM ZOO roll state` |
   | rolls[0].bounce_active = 2 | rejected, `invalid ZOOM ZOO roll state` |

   The original gates the charge on `$829641: LDA $0C6D / BEQ / LDA $0FF9 /
   BNE $96C3` and identically at `$82965E-$829666`, so the opponent
   (`$0FF9`==2) is excluded whenever `$0C6D` is non-zero. Measured over the
   authenticated race window of `early-compound-reverse-a`: `$0C6D` is zero in
   **0 of 5,348** frames 1376-6723 (it is zero only outside the race), so the
   native's hard `index==0` is scenario-correct — and the opponent can never
   carry a charge, yet the restore domain admits one. There is also no relation
   between `bounce_charge==160` and the `pose_override==$9A8` that
   `$829636-$829658` requires to set it. The guards that were added
   (`!=0 && !=160`, `bounce_active>1`, `bounce_charge && step`) are correct and
   do fire. Fix: restrict the charge to rider 0 and tie it to pose_override
   `$9A8`.

5. **Low — the cited justification for the out-of-table exit does not exist.**
   `movement.cpp:657-660` says the overrun weight range `$7E21C9-$7E21D8` is
   one "which the reference guard already holds at zero on every authenticated
   frame". `tests/manifests/native/zoom-zoo-race-guards.reference.json` has 82
   items whose highest address is `0x150B`; none covers `0x21C9-0x21D8`. The
   substance is nevertheless true here: I scanned all 6,394 frames of
   `early-compound-reverse-a` and `$7E21C9-$7E21D8` is zero in every one. But
   nothing enforces it, so a future capture could break the assumption
   silently. Fix: add the 16 bytes to the guard manifest, or restate the
   justification as a measurement rather than a guard.

6. **Low — the restore domain admits a published zero opponent queue entry
   that the next update rejects, where the original consumes it without
   incident.** The frozen probe seed with the published entry at offset 290 set
   to 0 is accepted by `deserialize_zoom_zoo` and then fails at frame 1722 with
   `reward queue holds no published event` (rc 1, 5 frames emitted). Given the
   same intervention — advance `$0D13` over a zero `$0CEB` slot — the original
   consumes it: read cursor 1 to 2, class byte `$81C609` = `$8D`, class counter
   `$7707EF` 0 to 1, then the zero weight at `$7E2201` exits with no reward and
   no error. Pre-existing in effect (the old code threw a different message),
   but the candidate re-asserted the decision while now holding the evidence to
   model the original. Same class as the earlier URZZ0005 finding.

7. **Low — the initializer is content-driven while the restore guard is still
   hard-coded.** `classic_crawler_zoom_zoo_start` now takes both banks'
   event-one weight from `content.reward_weights[0]`, and the new unit test
   exercises a pack with template[0] = 6, but `deserialize_zoom_zoo` still
   hard-codes `q.event_one_weight in {1,2,4}` and `!=4` at frame 1376. With any
   pack whose template[0] is not 4 a freshly started state could not be
   restored. Informational for the frozen pack.

8. **Low, process — the candidate records none of its claimed evidence.**
   `git diff 9f4c423..aeb62e0 -- tasks/M4-16.md` contains only the provider-move
   note from `fbc9cae`. The commit message asserts five probes captured twice,
   six complete cases, 6225 observations, 757 restores, app-sanitize 405 checks
   and six named regressions; `tasks/M4-16.md`, `tasks/NEXT_SESSION.md` and
   `docs/STATE.md` carry none of it. AGENTS.md requires the handoff to carry the
   actual commands and results.

### What I verified as correct

Stated plainly, because most of this candidate is right.

- **Full versus half vertical boost — confirmed, and reproduced.**
  `$81C165-C170` is `LDA $81C493,X / LSR / CLC / ADC $11DF / STA $11DF`;
  `$81C2A5-C2AF` is `LDA $81C493,X / CLC / ADC $11E1 / STA $11E1`, with no
  `LSR`. Injecting opponent event 21 before frame 1718 (consumed at 1722): the
  original's opponent vertical boost goes 79 to **278** — that is 79, minus the
  one-per-frame decay, plus the whole word `$00C8` = 200 — not 178. Horizontal
  boost 80 to 280, feature total 4 to 36, learned weight index 19 (event 21)
  32 to 16. The native matched all 742 bytes across all 43 frames.
- **The probe is a genuine original-only minimal intervention.** It writes
  exactly one entry byte and advances `$0D13` by one, refuses to overwrite a
  pending entry or fill the ring, and asserts that exactly those two of 131,072
  WRAM bytes changed. Its seed row is constructed slice-for-slice identically to
  the frozen playable projection in `zoom_zoo_playable.original()` — I compared
  both constructions field by field and the ordering, widths and the
  `row[7]='B'` marker agree, so the native comparison is not comparing the wrong
  thing. Pre-intervention frames 1207-1717 are hash-checked against the frozen
  original on every run and did check out on all six of my captures.
- **Determinism.** Two independent captures of event 21 produced identical
  `seed_sha256` `0352ee1d...` and `rows_sha256` `6c35df23...`.
- **The probe does discriminate the recovered behaviour.** Negative control: I
  rewrote the expected rows to the halved vertical boost a copy of the player's
  code would have produced, recomputed the report digest, and re-ran the
  comparison. It failed at exactly frame 1722, offsets 181-182, native `$0116`
  (278) against the mutated 178, rc 1.
- **An event the implementer did not probe.** Events 2, 8, 13, 17 and 200 were
  the candidate's. I ran **21** (real class `$1E`, weight 32, word `$00C8` —
  the largest reward in the reachable set) and **215** (the top of the voice
  range, which the candidate probed only at 200). Event 215 is consumed at 1722,
  resets the cooldown to 40 and publishes nothing; native matched 28/28 frames.
- **Branch structure against the real tables.** `$81C50A` is 72 entries:
  events 1-12 and 16-21 carry a real class, 13-15 and 22-72 are `$FF`. Reward
  words `$81C493` give event 1 = `$0080` up to event 21 = `$00C8`, with `$FFFF`
  at 13, 14 and 22 onward. The candidate's class/weight/word branch structure
  matches, and `learned_weights[event-2]` is exactly `$7E2102+(event-1)`.
- **`$82DB87-DB94` justifies the content-driven event-one weight.**
  `$82DB85: LDX #$19`, then `LDA $82D7A4,X / STA $7E20E8,X / STA $7E2102,X /
  DEX / BPL` copies one 26-byte template into both banks; `$82D7A4`[0] = `$04`.
  The initializer change is correct.
- **The omitted cartridge class counters really are outside the inventory.**
  The 742-byte projection's cartridge coverage is `0x755-0x76A`, `0x7BB-0x7BD`,
  `0x7BF-0x7D4`, `0x825-0x826`, `0x618-0x61B` and `0x106F-0x1072`. The player's
  counters begin at `0x76B`, immediately after the serialized `0x769-0x76A`
  word, and the opponent's at `0x7D5`. I enumerated every class value reachable
  for events 1-21 and every class byte in the 200-215 overrun and no resulting
  counter address lands inside a serialized byte. The omission is defensible.
- **Legacy DRAGSTER / M4-12-15 path unchanged.** The empty-span branch keeps
  the identical `leading_event` computation and the identical throw condition;
  the one changed expression, `content_word(rotation_reward, 2*(event-1))`,
  evaluates to index 0 there because the surviving guard admits a weight only
  when `event==1`, and `if(weight && *weight)` is implied by that guard's
  `event_one_weight==0` rejection. No rejection was removed. `update_movement`
  passes `{}`; `native_initialization` implies `complete_race`
  (`movement.cpp:1542-1543`), so the 72-entry class table and 144-byte reward
  table are always bound whenever the learned bank is passed.
- **`ca46025` bounce arithmetic is faithful.** `$829655: LDA #$00A0` is the
  literal retained 160; `$829675-C67D` stores the charge back unchanged below
  512, a genuine no-op as commented; `$829685-C693` is `EOR #$FFFF` of
  `min(charge,256)` into `$0FAB`; `$829696-C69C` stores literally 1 into
  `$042B,Y`, so binary `bounce_active` is right; `$8296A1-C6A7` is
  `CMP #$0002 / BPL` on `$0F33`. All match the native.
- Also confirmed the restore domain still rejects the previously accepted
  forged high entries: an opponent entry of 250 is refused at deserialization
  with `invalid ZOOM ZOO opponent voice event`, which the probe surfaced as
  "native did not start from the frozen probe seed".

### Commands, return codes and counts

Every command was run as its own invocation with its own return code inspected;
build and test were never chained.

| Command | rc | Result |
| --- | --- | --- |
| `cmake --build build/app-debug -j4` | 0 | 17 steps; `movement.cpp` recompiled, all 16 targets relinked including `src/app/unirally.app/Contents/MacOS/unirally`. `LAB_WARNINGS_AS_ERRORS=ON`, `LAB_SANITIZERS=OFF`, AppleClang 17.0.0 |
| `cmake --build build/app-debug -j4` (repeat) | 0 | `ninja: no work to do` — nothing stale |
| `ctest --test-dir build/app-debug --output-on-failure` | 0 | **21/21 passed, 0 failed**, 3.77 s |
| `python3 -m tools.unirally_lab.native.zoom_zoo_playable compare --reference .../early-compound-reverse-a --repeat .../early-compound-reverse-b --contract .../zoom-zoo-playable-review-early-compound-reverse-v11.freeze.json --binary build/app-debug/src/core/zoom_zoo_runner --pack local/classic-crawler-two-tracks-v5-review.pack --out .../early-compound-reverse-aeb62e0-compare.json` | 0 | passed; frames 1376-7600 = **6,225 states**, **739 fresh-process restores** plus a repeated fresh initialization and a full fresh restarted race; `rows_sha256` `3113fa66...` equals the frozen contract; recorded `source_commit` `aeb62e0` with an empty working diff |
| `... zoom_zoo_opponent_reward_probe capture --event 21 --before-frame 1718 --through 1760` | 0 | cooldown 10 at 1718, consumed 1722; `rows_sha256` `6c35df23...` |
| same capture, second run | 0 | identical `seed_sha256` and `rows_sha256` — deterministic |
| `... probe native --probe rev-probe-e21-a` | 0 | **passed, 43/43 observations, 742/742 bytes** |
| `... capture --event 215 --before-frame 1718 --through 1745` | 0 | consumed 1722, no reward published |
| `... probe native --probe rev-probe-e215-a` | 0 | **passed, 28/28 observations** |
| `... probe native` on the half-vertical-boost mutated expectation | 1 | **failed at frame 1722, offsets 181-182** — the negative control fires |
| `... capture --event 21 --through 1720` then `native` | 0 | **"passed" with 3 observations and no consumption** — finding 2 |
| reviewer branch discriminator (events 100 / 150 / 205, `$7707D5-$7708D4`) | 0 | only 205 incremented a counter — finding 1 |
| zero-entry restore reproduction via `zoom_zoo_runner --seed` | 1 | `frame 1722: reward queue holds no published event` — finding 6 |
| bounce-domain reproductions via `zoom_zoo_runner --seed` (6 states) | 0 / 1 | 160 on either rider accepted, 161 and `bounce_active` 2 rejected — finding 4 |
| weight/cooldown-domain reproductions via `zoom_zoo_runner --seed` (5 states) | 0 | weights 0, 3, 250 and cooldown 60000 all accepted — finding 3 |
| `python3 tools/project.py native compare --manifest tests/manifests/native/primary.case.json` | 2 | **MISSING PREREQUISITE, not a pass**: `local/native/dragster-idle/runtime.json` is absent from this checkout |

### Not verified, and stated as such

- The DRAGSTER `native compare`, `finish-check`, `opponent-first-check` and
  `restore-check` regressions could not run here: this checkout has no
  `local/native/` tree and the CLI exposes no command that regenerates it.
  Reported as missing prerequisites. The legacy path is covered instead by the
  source equivalence argument above and by the 21/21 ctest suite, which
  includes `movement_state_roundtrip`, `movement_first_update` and
  `movement_restore_continuation`. No unit test asserts that the legacy
  "reward queue left the recovered event-one domain" rejection is preserved.
- `app-sanitize`, the broad/CI suites, live controls, visuals and the full
  restart product evidence were out of this pass's scope and are not claimed.
- The `$1007` bounce-charge write audit is limited to the two writes inside
  `$829398-$8296C3`; no cross-bank write audit of that address was performed.
- `$0C6D` is characterized only by measurement over one authenticated capture;
  its meaning was not recovered.
- Only `early-compound-reverse-a/b` were used as original captures. All probe
  interventions sit at frame 1718 on that timeline.

M4-16 remains unaccepted.

### Addendum — both refuted claims have since propagated into tracked records

Checked after the review above was written. `codex/m4-16-playable-zoom-zoo` has
advanced to `7d1904d`; the three commits after the reviewed candidate
(`5f850a5`, `825adf7`, `7d1904d`) are documentation only and change no source,
so every finding above still applies to the branch tip unchanged. But `5f850a5`
copied both refuted claims out of the code comments and into the project's
tracked evidence:

- `docs/research/R-0035-zoom-zoo-playable-recovery.md`: "The domain test
  `$81C238` is a **signed** comparison with 72." Refuted by finding 1 —
  `CMP #$48` followed by `BMI` is an N-flag test taken for 0-71 and 200-255,
  and injecting event 150 into the original leaves the reward path's class
  counter untouched while event 205 increments `$770801`.
- `tasks/M4-16.md`: the overrun weight range "`$7E21C9-$7E21D8` ... is already
  an explicit zero guard in `zoom-zoo-race-guards.reference.json`". Refuted by
  finding 5 — that file has 82 items whose highest address is `0x150B` and none
  covers `0x21C9-0x21D8`. The parenthetical in the same sentence is correct:
  `$12E5` is guarded at zero (item `{address: 4837, width: 2, value: 0}`),
  which makes the neighbouring false claim easier to mistake for verified.

Findings 1 and 5 therefore escalate: they are no longer only source comments
but the recorded research and task evidence, which is exactly what a later
reader would rely on. Correct both records alongside the code.

## Re-review of the corrections — candidate `9a3c504` — 14 September 2026

**Reviewing model: Claude Opus 5**, the same independent reviewer, same isolated
checkout, no inherited implementation context. Exact candidate
`9a3c5045d0b029057bbfa1ad63359585a6378f8d` on `codex/m4-16-playable-zoom-zoo`,
range `aeb62e0..9a3c504`; the three documentation commits in that range
(`5f850a5`, `825adf7`, `7d1904d`) predate the report above and carry no source.

### Verdict on this round: findings 1-5 are genuinely closed — approve the corrections

Each one was re-derived from the ROM and re-reproduced against the corrected
binary rather than read off the commit message. Two Low residuals remain inside
finding 2, and one factual slip is new. M4-16 itself stays unaccepted for the
reasons its own record already carries.

### Finding-by-finding

1. **Closed.** The predicate is now `event<72 || event>=200`
   (`src/core/movement.cpp:648`), which is exactly the taken set I derived for
   `CMP #$48` / `BMI`, and the comment states the bit-7-of-the-8-bit-difference
   reading and says explicitly that it is deliberately not `int8(event)<72`.
   `docs/research/R-0035` and `tasks/M4-16.md` both now say plainly that the
   earlier revision was wrong, why, and how it was settled. Re-verified rather
   than assumed: my own frozen event-21 and event-215 original rows from the
   previous round still match the **new** binary
   `5f0d9695b519e8ecd17a38c9aff98070efcb36fcfada3e9e1dd88a6cba5613a9`
   byte-exactly (43/43 and 28/28 observations), and the complete case produces
   the identical `rows_sha256` `3113fa66...`, so the correction is
   behaviour-preserving on the reachable domain, which is what a correct fix of
   an unreachable divergence must be.

   One qualification worth keeping in the record: the new event-150 probe does
   **not** on its own discriminate the two predicates. I captured it
   independently — consumed at frame 1722, cooldown reset to 40, no boost, no
   feature total, no weight change — and at the 742-byte projection the voice
   path and the out-of-table reward path are both "consume and publish
   nothing". What settles it is the cartridge class counter, which only the
   reward path touches: event 150 changes nothing in `$7707D5-$7708D4` while
   event 205 increments `$770801`. R-0035 cites that correctly. The native
   refusal of the event-150 seed reproduced exactly as described
   (`invalid ZOOM ZOO opponent voice event`, rc 1), so the recorded
   "source and original-side evidence but no native differential coverage for
   128-199" is an accurate and appropriately hedged statement.

2. **Closed, with two Low residuals.** My exact reproduction
   (`capture --event 21 --through 1720`) now exits **1** with
   `probe window ended before the injected entry was consumed`. Tripped guards
   now fail a capture. A `freeze` subcommand exists and I drove the full
   hardened pipeline end to end on event 8: two captures, identical
   `seed_sha256 c142d8a5...` and `rows_sha256 acafeeb0...`,
   `consumed_at_frame` 1722, freeze rc 0, native rc 0 and 9/9 observations.
   Residuals:
   - `freeze --first X --second X` with the **same directory twice** passes
     rc 0 and emits a record indistinguishable from a genuine pair. Verified.
     The captures carry no per-run identity, so the freeze cannot tell one run
     from two. Fix: reject `first.resolve()==second.resolve()` and record a
     per-capture run nonce.
   - Neither `freeze` nor `native` requires `consumed_at_frame` to be present,
     so a report produced before the hardening still freezes and still reports
     `"status": "passed"` — verified, with `consumed_at_frame: null`, on my two
     pre-correction probe directories against the corrected binary. The
     assertion therefore cannot be satisfied vacuously *when `capture` runs*,
     but it can be bypassed by reusing an older report. Fix: require the field.

3. **Closed, and the bound is tight rather than over-tight.** The guard sits
   inside `if(native_initialization)`, so the legacy DRAGSTER/M4-12-15 and
   `deserialize_movement_state` paths are untouched. Boundary-tested against a
   real 742-byte state: weights 1, 2, 4 and cooldowns 5 and **40** accepted;
   weights 0, 3, 250 and cooldowns 41 and 60000 rejected with
   `invalid ZOOM ZOO opponent reward queue state`. The ceiling is right: across
   **81,528 frames of 12 independent reviewer captures** the maximum opponent
   cooldown `$0CA7` is exactly **40**, while the player's `$0CA5` reaches 120 —
   so the two different ceilings are a real asymmetry, not a copied constant,
   and 40 itself is observed live (frame 1722 of every probe).

4. **Closed, and it cannot reject a reachable state.** `rolls[1].bounce_charge`
   or `bounce_active` now rejects with `invalid ZOOM ZOO opponent bounce
   state`; rider 0's charge 160 and bounce flag 1 still restore. Evidence that
   nothing reachable is lost: `$1007`, `$1009` and `$100B` are zero in all
   81,528 frames of my 12 captures, and the serialized `bounce_active` words
   `$042B`/`$042D` are zero in **every** frame of the projected race window
   1376-6723 — they take `$4C00` only outside it, which the projection never
   includes, so my initial whole-file scan that flagged them was measuring the
   load and result phases. `$0C6D` is itself an existing guard at value 1,
   which is what makes the rider gate at `$829641-9649` / `$82965E-9666`
   sound. The corrected test rejecting both values at offset 674 is right; the
   previous test had encoded the permissive domain.

   Honest limit: none of my 12 captures ever charges a bounce at all, so I have
   no original-side trace of one. My confirmation of the bounce arithmetic is
   from the disassembly only — `$829655 LDA #$00A0`, the no-op below-512 store
   at `$829675-C67D`, `EOR #$FFFF` of `min(charge,256)` at `$829685-C693`, the
   literal 1 into `$042B,Y` at `$829696-C69C`, and `CMP #$0002 / BPL` at
   `$8296A1-C6A7`.

5. **Closed.** Exactly sixteen single-byte zero guards added at
   `0x21C9-0x21D8`; nothing removed, nothing altered, 82 items to 98. `$12E5`
   was indeed already guarded at zero, as the record says.

### The four specific questions

**(a) 216-255 and event 0.** Handled consistently, and I tested rather than
reasoned. Entries **216** and **255** are rejected at deserialization with
`invalid ZOOM ZOO opponent voice event`, so `update_reward_queue` is never
reached for them; the new predicate does admit them to the block, where the
out-of-table branch throws, but that is unreachable defense in depth — the
producer emits only `200+(x&15)`. Note the new guards stop at `$21D8` and do
not cover the `$7E21D9-$7E2200` an event 216-255 would read, which is sound
only because the upstream rejection holds; if anyone ever widens the entry
domain past 215, that guard range must widen with it. Entries **215** and
**71** restore and run normally, confirming both edges of the admitted set.
**Event 0** is unchanged: the native still throws
`reward queue holds no published event` at frame 1722 on a state
`deserialize_zoom_zoo` accepts, while the original consumes it without incident
— I re-measured it (class byte `$81C609` = `$8D`, class counter `$7707EF` 0 to
1, zero weight at `$7E2201`, no error, read cursor 1 to 2). That is finding 6,
still open.

**(b) Can the new bounds reject a reachable state? No, on my evidence.** The
cooldown ceiling and the bounce rejection are each measured above against
81,528 original frames from 12 independent captures, with the boundary values
themselves exercised through the runner. The one caveat is the absence of any
bounce in my captures, stated in finding 4.

**(c) Does the guard addition invalidate a frozen reference? No.** The manifest
is not hash-pinned anywhere — I computed the pre-change digest
`266799e8...` and it appears nowhere in the tree — and its three consumers
(`zoom_zoo_playable`, `zoom_zoo_race_reference`, the probe) read it directly,
so extending it can only make them stricter. The frozen contract's
`rows_sha256` depends on the projection, not on the guards. And the new guards
hold: zero at `$7E21C9-$7E21D8` on every one of the 81,528 frames of all 12 of
my captures, independent of the implementer's 53-capture scan. The complete
case still reproduces `rows_sha256` `3113fa66...` unchanged.

**(d) Can the consumption assertion be satisfied vacuously? Not by `capture`;
yes by reuse.** Detailed in finding 2. The assertion itself is sound for its
purpose — the read cursor can only reach the injected index by consuming that
slot — but note it guarantees *consumption*, not reward-path coverage: for the
class-255 and out-of-table controls (13, 22, 200, 215) it is satisfied while
nothing in the reward machinery runs, which is correct for a negative control
but should not be read as differential coverage. One unguarded corner: the
capture refuses a full ring (`advanced == read`) but not `write == read`, in
which case `entry_index` would already equal the read cursor; unreachable on
this timeline (read 1, write 2 at frame 1718), worth a line of code anyway.

### New minor finding

**Low — the new evidence table misstates one class number.**
`tasks/M4-16.md` records "event 17 | class 24 counted, zero weight, no reward".
Event 17's class byte at `$81C50A+16` is `$22` = **34**, and its counter is
`$770819`, which I measured going 0 to 1 with the feature total `$770825`
unchanged. **24** is event *18*'s class (`$18`); I measured event 18
incrementing `$770805` 0 to 1, its paired accumulator `$770807` 0 to 4, and the
feature total `$770825` 4 to 8. The substance of the row — class counted, zero
weight, no reward — is correct; the number is not. Relatedly, R-0035 still
lists five probed events where the task record now reports nine.

### My position on findings 6-8

- **6 (zero queue entry accepted by restore, rejected by the next update):
  recommended, not blocking.** It is the only remaining place where the
  742-byte restore domain is not closed under one update, which is the property
  URZZ0005 established. The cheapest close is to reject a published zero entry
  in `deserialize_zoom_zoo`, matching the update; modelling the original's skip
  is also defensible now that the evidence exists. I would fix it, but I would
  not hold acceptance for it alone.
- **7 (initializer content-driven while the restore guard hard-codes 4 and
  {1,2,4}): not blocking.** The values coincide for the frozen pack. It only
  bites if a second scenario's `$82D7A4` template differs. Record the coupling.
- **8 (evidence not recorded): withdrawn.** `5f850a5` and `9a3c504` add 213
  lines to `tasks/M4-16.md` and 63 to `R-0035` carrying the commands, the
  per-event outcome table and the corrections, including an explicit statement
  that the earlier revision was wrong. That satisfies the AGENTS.md handoff
  requirement.

### Commands, return codes and counts for this round

Each invoked separately with its own return code inspected; build and test never
chained.

| Command | rc | Result |
| --- | --- | --- |
| `cmake --build build/app-debug -j4` | 0 | 17 steps, all targets relinked, warnings-as-errors on |
| `ctest --test-dir build/app-debug --output-on-failure` | 0 | **21/21 passed, 0 failed** |
| `zoom_zoo_playable compare` (early-compound-reverse a/b, v11 contract) | 0 | **6,225 states, 739 fresh-process restores**; `source_commit 9a3c504`, empty diff, binary `5f0d9695...`; `rows_sha256 3113fa66...` **unchanged from the pre-correction run** |
| `probe capture --event 8 --through 1726` twice | 0, 0 | identical `seed`/`rows` digests, `consumed_at_frame` 1722; feature total +64, weight 64 to 32, boost +248, vertical **+248** (full word again) |
| `probe freeze --first rev2-probe-e8-a --second rev2-probe-e8-b` | 0 | pair agrees |
| `probe native --probe rev2-probe-e8-a` | 0 | **passed, 9/9 observations, 742 bytes** |
| `probe native` on my frozen event-21 rows, new binary | 0 | **passed, 43/43** |
| `probe native` on my frozen event-215 rows, new binary | 0 | **passed, 28/28** |
| `probe capture --event 150 --through 1730` | 0 | consumed 1722, no reward at all |
| `probe native --probe rev2-probe-e150` | 1 | refused: `invalid ZOOM ZOO opponent voice event` — the recorded original-only control |
| `probe capture --event 21 --through 1720` (the old vacuity repro) | **1** | `probe window ended before the injected entry was consumed` |
| `probe freeze --first X --second X` (same directory) | 0 | **residual hole**, finding 2 |
| `probe native` on a pre-hardening report | 0 | `"passed"` with `consumed_at_frame: null` — **residual bypass** |
| 21 restore-boundary states through `zoom_zoo_runner --seed` | 0 / 1 | all 21 accept/reject outcomes as intended (table above) |
| reviewer branch discriminator, events 17 and 18 | 0 | `$770819` and `$770805`/`$770807`/`$770825` — the class-number slip |
| 12-capture scan, 81,528 frames | 0 | `$7E21C9-$7E21D8` zero everywhere; max `$0CA7` 40, max `$0CA5` 120; `$1007`/`$1009`/`$100B` zero; `$042B`/`$042D` zero in every projected race frame |

### Still not verified in this checkout

Unchanged from the previous round and reported as missing rather than passing:
the DRAGSTER `native compare`, `finish-check`, `opponent-first-check` and
`restore-check` regressions cannot run here — there is no `local/native/` tree
and no CLI command regenerates it. The legacy path remains covered by the
source-equivalence argument and the 21/21 ctest suite. `app-sanitize`, the
broad and CI suites, live controls, visuals and the restart product evidence
were out of scope for this pass and are not claimed. No capture available to me
exercises a bounce.

M4-16 remains unaccepted.

## Narrow confirmation pass — candidate `a16b88c` — 14 September 2026

**Reviewing model: Claude Opus 5**, same reviewer and isolated checkout. Exact
candidate `a16b88c`, range `9a3c504..a16b88c` (`adbbeb3` in that range is
documentation). Scoped as requested: the two probe residuals, finding 6, the two
couplings and the class number. Broad suites deliberately not re-run.

### Verdict: all four confirmed closed. Approve this round.

**Probe residuals — closed, reproduced four ways.**

| reproduction | rc | result |
| --- | --- | --- |
| `freeze --first X --second X` | 1 | `a freeze needs two independent captures, not one directory twice` |
| `freeze` with `consumed_at_frame` stripped from a copied report | 1 | `probe predates the consumption assertion; recapture before freezing` |
| `native` with the field stripped | 1 | `probe predates the consumption assertion; recapture before comparing` |
| `native` on my genuine pre-hardening event-21 report | 1 | same refusal — the bypass I demonstrated last round is gone |
| `freeze` on the genuine event-8 pair | 0 | still accepted; the tightening is not over-tight |

The capture's new cursors-meet refusal cites `$81C5CD-C5D0`, and that citation is
exact: I disassembled it as `CPY $0D11` / `BEQ $81C5ED`, the opponent enqueue's
return-without-publishing when the write cursor already equals the read cursor.
(The same routine at `$81C5D5-C5E3` clears `$0CA7` and `$12E5` for events below
22; `$12E5` is guarded at zero throughout, so that branch is never taken — which
is what justifies `enqueue_zoom_opponent` omitting it, and this time the cited
guard really does exist.)

**Finding 6 — closed at the restore, and provably unable to refuse a reachable
state.**

The check walks `(read_cursor+1)&31` until it meets `write_cursor`. That set is
*exactly* the set the consumer will read before the queue reports empty,
including the degenerate `read == write` case, where both the check and the
consumer traverse the same 31 slots. So it is structurally closed under one
update rather than approximately so.

Empirically, over **79,500 frames of my 12 independent captures** (1376 to each
capture's last frame):

- opponent pending-zero occurrences with `read != write`: **0**, at pending
  depths up to 30;
- frames with `read == write`: 3,876, of which **640 are inside the race window
  1376-6723**; in every one of those 640 all 31 consumable slots are non-zero
  (they saturate with event 39, the opponent finish publication, from frame
  6565);
- player pending-zero occurrences, as an accepted control: 0.

Driven through the runner rather than only measured:

| state | rc | result |
| --- | --- | --- |
| published zero in the opponent ring | 1 | `empty pending ZOOM ZOO opponent reward` at **restore**, 0 frames emitted — previously accepted, then thrown at frame 1722 |
| same slot holding event 1 | 0 | restores and runs |
| `read == write == 24` with all 31 consumable slots = 39, the exact shape the original reaches at 6565+ | **0** | **accepted and runs** |
| the same state with one of those slots zeroed | 1 | rejected |

That last pair is the point: the rejection discriminates the forged state from
the reachable degenerate one rather than refusing both.

**The 216-255 coupling — recording it is the right call, and I would not widen
the guards.** I agree with the decision and would argue against the alternative.
The race guards assert things about the *original's* authenticated timeline, and
are checked before native evaluation; their purpose is to stop a capture that
reaches unmodelled gameplay. `$7E21C9-$7E21D8` qualifies, because 200-215 is
produced, does reach the reward path, and the model's correctness depends on
those sixteen bytes. `$7E21D9` upward does not: nothing in the original can hand
the consumer an event in 216-255, since the producer emits `200+(x&15)`. A guard
there would block otherwise valid captures for a condition that is not a
modelling gap — a false-positive generator. Guarding exactly the load-bearing
range and recording the coupling is the principled line.

One optional improvement, not blocking: the coupling is stated at the consumer
branch, but the clause that actually enforces it is the entry-domain check
`event>=72 && (event<200 || event>215)` in `deserialize_zoom_zoo`. A one-line
cross-reference there would make the pair discoverable from either side.

**Finding 7's coupling** is now recorded at the player weight bound, naming the
`$82D7A4` template as the thing a future pack change must revisit. Adequate.

**Class number — corrected and independently re-confirmed.** `tasks/M4-16.md`
now reads "event 17 | class 34 counted at `$770819`" and records that 24 is
event 18's class at `$770805`. That matches my measurements exactly: event 17
increments `$770819` 0 to 1 with the feature total `$770825` unchanged; event 18
increments `$770805` 0 to 1, its paired accumulator `$770807` 0 to 4 and the
feature total `$770825` 4 to 8.

### Commands for this pass

| Command | rc | Result |
| --- | --- | --- |
| `cmake --build build/app-debug -j4` | 0 | 17 steps, all targets relinked |
| `ctest --test-dir build/app-debug` | 0 | **21/21 passed, 0 failed** |
| `probe native --probe rev2-probe-e8-a` on the closure binary `ed1de830...` | 0 | **passed, 9/9 observations**, `consumed_at_frame` 1722 — the closures changed no reachable behaviour |
| five restore-domain states through `zoom_zoo_runner --seed` | 0 / 1 | all five as intended, table above |
| four probe-residual reproductions plus the genuine-pair control | 1,1,1,1 / 0 | table above |
| 12-capture pending-ring scan, 79,500 frames | 0 | zero violations; 640 in-race degenerate states all fully published |
| disassembly of `$81C5C9-C5EA` | — | confirms the cited `$81C5CD-C5D0` publication limit |

Broad suites, sanitizers, DRAGSTER native regressions, live controls and visuals
were out of scope for this pass at the coordinator's request and are not
claimed. M4-16 remains unaccepted on its own open product evidence.

## Final independent review — candidate `9b43f3c` — 17 September 2026

Reviewing model: **Claude Opus 5**, fresh subagent, no implementation
conversation. Checkout `.worktrees/m4-16-final-review`, branch
`review/m4-16-final`, at exact `9b43f3c9f0c5aff0299fe66a8412e3f293e1dd6d`
(equal to `origin/codex/m4-16-playable-zoom-zoo` when checked). No
implementation file was changed; other worktrees were only read. Started
07:54:24 UTC; checks finished 08:22 UTC; report committed about 08:30 UTC. Scope: everything since the last
visual review (`bfd41ed` of `06abab7`): `473dbc1` through `9b43f3c`, 27
commits, 31 files.

### Verdict: changes required — one blocking failure

**Blocking.** Hosted CI on this exact candidate fails on Linux (item 7 requires
it). Everything I reproduced for the behaviour and presentation scope passes:
visual criteria 1 and 3 now **pass**, criterion 2 **passes with limitations**,
and the primary and idle-late-start gates, all four presets, the DRAGSTER
presentation gates and my ROM checks of the cited routines agree with the
record. Findings 2-5 below are non-blocking.

1. **High (blocking): the hosted Linux build fails on `9b43f3c`.**
   `gh run view 35196948852`: head `9b43f3c9...`, `lab (macos-15)` success,
   `lab (ubuntu-24.04)` failure at "Build lab-debug". GCC `-Werror`:
   `src/core/presentation.cpp:943:78` misleading-indentation (`ui_text`),
   `:1008:85` sign-conversion (`zoom_zoo_hud` clock sum passed to
   `race_time(unsigned)`), `:1103:9` misleading-indentation (BG1 tile flip
   `if(...)px=15-px;if(...)py=15-py;`). Apple clang 17 accepts the same flags,
   so no local preset shows it. The coordinator reports a formatting-only fix on
   `task/m4-16-linux-build` (`333fb58`, remote ref `36c37f6` when I checked); I
   did not review it. Re-review of the next candidate should confirm
   `git diff 9b43f3c <next> -- src tests tools` is formatting or casts only,
   that both CI jobs pass, and that the four presets still pass. The evidence
   below then carries over if the primary rows digest (`b4a34af7...`) and
   the render hashes listed below are unchanged.
2. **Minor, pre-existing (not introduced in this range): the BG scroll is wrong
   on the pause-entry and unpause pictures.** `render_zoom_zoo` derives the
   previous camera as `camera - camera.velocity`. A paused update does not move
   the camera, so the first picture after the pause is drawn with the
   background and track displaced. `pause-a` frame 6010 (first unpaused
   picture): 31,094 of 54,272 pixels below the HUD band differ, and the best
   whole-frame alignment of native to original is (+8,+4) (95.5% equal), while
   6011-6030 align at (0,0) with only HUD glyph differences. The rider objects
   are correct at 6010. The updates that enter, hold and leave the pause
   (6000-6010) all leave the camera still, so by the same formula the dimmed
   pause pictures carry the displacement too. I inferred that and did not
   measure it, because the dim differs. Undimmed, it shows for one 20 ms
   frame on every resume.
3. **Minor: after a 10:00 time-out the authored clock cycles.** The held clock
   keeps its subframe, so the HUD reads 9:59.98, then 9:59.90, .92, .94, .96,
   .98, repeating until result loading (stop-timeout native frames 31582-31588).
   The original shows a fixed `9:59:9`. The value matches at tenths, as
   everywhere else, but the hundredths visibly run backwards and loop.
4. **Minor, records: stale statements.** `docs/STATE.md` still describes
   `ca46025`, a 50-entry v5 pack, an open opponent-reward finding and incomplete
   live/visual gates. `R-0035` line 101 says "Gamepad is untested", and lines
   96-99 call the rider work unreviewed and awaiting D1-D3 re-review.
   `NEXT_SESSION` cites 21/21 focused tests (22 now). These need refreshing in
   the closeout.
5. **Minor, record precision.** The recorded limitation says the original shows
   the opponent's finish time "while the player is still racing after the
   opponent finishes". The original also shows it after both have finished:
   `1:38:10` low centre at primary frame 6600 (player finished 6484) and at
   wrong-way 6600-6850. Native omits it in every case. It is excluded
   decoration, but the record should describe it as it appears.

### Build and input identity

- Presets `lab-debug`, `lab-sanitize`, `app-debug`, `app-sanitize` via
  `python3 tools/project.py build --preset P`: all rc 0, Apple clang 17.0.0,
  CMake 3.31.10, Ninja 1.13.2, clean source recorded in `lab-build-info.json`.
  `zoom_zoo_runner` (app-debug) `bd8b9136...`, `zoom_zoo_presentation_runner`
  (app-debug) `76e7444a...`, app-sanitize runner `058c5dfa...`,
  app-sanitize `unirally` `91e69ac7...`.
- **Pack v7 extracted by me from the ROM** (`frontend run --track zoom-zoo --rom
  ... --pack local/classic-crawler-two-tracks-v7.pack --preset app-debug
  --updates 20 --hidden`): rc 0, 55 entries, SHA-256 `b75539a0adfe13c3...fd442`.
  That matches the record's `b75539a0...` and is byte-identical to the
  implementer's pack. ROM SHA-256 `a1105819...0fd4e`. A copy of the old v6 pack
  (`adfa974b...`) is refused: rc 3, "existing pack is invalid and was not
  replaced: Classic pack extraction-rules identity is incompatible".
- All 20 frozen original PNGs in `zoom-zoo-playable-v2.json` match their
  SHA-256 values.
- I regenerated every native timeline from native initialization on my build
  (`zoom_zoo_runner --start classic.crawler.zoom-zoo --content-pack PACK
  --inputs ...`). Each matches the original projection rows in full: boundary-a,
  brake-a, loss-a and pause-a 6225/6225, compound-reverse-a 6225/6225,
  wrong-way-a 8125/8125. My four boundary/brake/loss/pause timelines are
  byte-identical to the implementer's `*-native.txt`.
- **Original recaptures** used `recapture.py` (byte-identical copy
  `359ab534...` of the rider-art worktree script), which asserts every
  replayed frame's video and WRAM digest against the frozen `reference.json`.
  Frames checked and PNGs kept: boundary-a 5,518 / 99; compound-reverse-a
  5,284 / 137; wrong-way-a 5,694 / 56; pause-a 4,824 / 36; idle stop-timeout-a
  31,094 / 35. My recaptured boundary-a frames 1376, 1450, 1700, 2501, 3208,
  4840 and 6484 are byte-identical to the frozen PNGs.

### Visual criteria 1-3 on the frozen scenes

Native renders came from `zoom_zoo_presentation_runner PACK --timeline
<my boundary-a|brake-a timeline> FRAME OUT.ppm` (1376 via the single-state
form). "Below HUD" counts pixels equal to the frozen original in rows 12-223.
The original HUD shows tenths only.

| Scene | Frame | Lap orig / native | Clock orig / native | Below-HUD equal | What differs |
| --- | --- | --- | --- | --- | --- |
| initialization | 1376 | - | - | 54,272/54,272 | nothing (both black) |
| countdown | 1450 | 0/3 / 0/3 | 0:00:0 / 0:00.00 | 44,195 | blue start arrow, red HUD glyphs, native `READY` |
| start | 1649 | 0/3 / 0/3 | ?:01:3 / 0:01.32 | 44,400 | start ring, `MORE STUNTS`, `<` |
| reversal | 1700 | 1/3 / 1/3 | 0:02:3 / 0:02.34 | 53,514 | `MORE STUNTS`, `<`, HUD glyphs; rider identical, leaning left in both |
| steep_contact | 2501 | 1/3 / 1/3 | 0:18:3 / 0:18.36 | 53,965 | `>` and HUD glyphs; rider identical, wheel on the wall |
| lap_one | 3208 | 1/3 / 1/3 | 0:32:5 / 0:32.50 | 53,985 | HUD glyphs only |
| lap_two | 4840 | 2/3 / 2/3 | 1:05:1 / 1:05.14 | 54,001 | HUD glyphs only; both riders identical |
| finish | 6484 | 3/3 / 3/3 | 1:38:0 / 1:38.02 | 53,985 | HUD glyphs only |
| player_win | 7000 | - | - | authored | render hash `c1d2cffe...`, identical to the `bfd41ed` render: MIKE 1:38.02, BRONSEN 1:38.10, best laps 0:32.50, WINNER |
| player_loss | 7000 brake | - | - | authored | render `4f4c7a6d...`, identical to `bfd41ed`: MIKE 1:38.18, BRONSEN 1:38.10, RUNNER UP |

Native render SHA-256 prefixes in table order: `0bed1ceb af38a76b 4802eb04
66c713f1 06918740 1d9e5f36 0514f3d7 14c8df70 c1d2cffe 4f4c7a6d`.

- **D5 (HUD latency) closed.** HUD strips over the recaptures: the lap glyph
  changes at 1676, 3209 and 4841 in both pictures, and FINISH appears at 6485
  in both. Clock tenths agree at every inspected frame, for example 3207
  `0:32:4` against `0:32.48`.
- **D4 (post-finish HUD) closed, with the recorded approximation.** From 6485
  native shows FINISH, `1:38.02` and WINNER (primary), and FINISH, `1:38.18`
  and LOSER (brake frame 6724). The values match. Timing differs slightly more
  than recorded: the original keeps the corner clock at 6485 and drops it at
  6486, but its centred `1:38:02` and WINNER first appear at 6488 (recaptured
  6486-6500). Native shows them from 6485, three pictures early. That is
  cosmetic; the record's "one more frame" note should say 6488.
- **D6 (BG1 row) closed.** Race scenes align at (0,0). Outside overlays and HUD
  glyphs, the scene is pixel-identical.
- **D1-D3 (rider art) closed.** Rider objects are pixel-identical wherever no
  original overlay covers them; see the systematic count below.
- **Fade.** Recaptured 1377-1412 differ only in the start arrow (its colour
  cycles through the fade) and the countdown `3`. Brightness first shows at
  1393 in both.
- **Start/finish line palette.** The checker is pixel-identical on consecutive
  frames 3205-3212 and 6480-6485 (primary) and 6470-6477 (compound-reverse, an
  untuned case), which step through different cycle indices.

**Systematic rider-pixel check.** A reviewer harness
(`artifacts/m4-16-final-review/harness/rider_mask.cpp`, linked against the
app-debug libraries) replays each timeline with `ZoomZooRiderLookTracker`, as
the runner does. It renders every recaptured race frame twice, normally and
with both riders moved off-screen. The difference is the set of pixels rider
objects draw. The normal render was asserted equal to the runner output for
every frame. Rider pixels equal to the original:

| Timeline | Frames | Rider pixels | Equal | Every unequal pixel is |
| --- | ---: | ---: | ---: | --- |
| boundary-a (tuned) | 82 | 33,105 | 29,146 | start arrow during fade (single cycling colour per frame), arrow `2626ff` 1450, ring `ceceff` 1583, hint text over the seat 2100, finish flag 6486-6500 and 6724 |
| compound-reverse-a (untuned) | 137 | 43,690 | 41,878 | ring 1650, `MORE STUNTS` text blended over the seat 1685-1701, finish flag 6479-6490 |
| wrong-way-a (untuned) | 56 | 22,040 | 19,036 | flag and wrong-way arrow and caption overlays only |
| pause-a | 36 | 10,993 | 8,323 | only the ten dimmed pause pictures 6000-6009 (pause fidelity excluded) |
| stop-timeout-a (untuned) | 35 | 6,407 | 5,363 | time-out flag 31584-31600 only |

I checked the colour lists and visual crops. None of the unequal pixels is a
rider-art, facing or anchor difference. Both flip states are covered: the
reflected player in all of pause-a and in compound-reverse. Wrong-way-a
exercises sustained leftward riding (6551-6882) with tumbling poses. The facing
matches at 6560, 6590, 6600, 6700 and 6850.

**Criterion 1: PASS.** Geometry, camera, rider positions and facing, lap and
clock at the original's precision, post-finish HUD, and result values for both
outcomes match. **Criterion 2: PASS with limitations.** The authored HUD,
FINISH/WINNER/LOSER, NO TIME and the pause menu are legible. Excluded
decoration: start arrow and ring, countdown digits, hint text, off-screen
arrows, finish flag and captions, the opponent's finish time (finding 5),
result art and graph scale, original pause menu, audio. **Criterion 3: PASS.**
Rider art is recovered rather than a fallback, and anchors are exact at 2501.
The hidden app-sanitize run below reports 0 pose fallback frames.

### Time-out path (untuned original)

- Native rows from initialization equal the original's projection for
  1376-31927 (30,552 rows). The first difference is at 31928, bytes 573-576
  (graph extrema published natively, still 0 in the original). This reproduces
  the recorded two-update-early limitation. The tool is
  `artifacts/m4-16-final-review/tools/timeout_timeline.py`: the gate's
  `original()` with only its +108 visible-timing and 200-stable checks relaxed.
- Pictures: at 31588 the original shows `2/3`, `9:59:9` and LOSER, and native
  shows `2/3`, `9:59.90` and LOSER. The result pictures at 32000 and 32300
  show, in the original: MIKE `NO TIME`, BRONSEN `1:38.10`, both best laps
  `0:32.50`. Native shows the same values under RUNNER UP, and the MIKE graph
  plots only its completed lap, like the original.
- The app-sanitize `zoom_zoo_runner` gives identical output over the whole
  30,925-row time-out timeline with an empty stderr. Its presentation runner
  renders 8 sampled frames (fade, lap, finish flag, result, wrong-way, pause,
  compound, loss HUD) and the time-out result identically to app-debug, with no
  sanitizer report.

### Source claims checked against the ROM

Disassembled from the PAL ROM with a reviewer script built on
`tools/unirally_lab/coverage/opcodes.py`:

- **Clock limit `$81:C73E-C75B`: confirmed.** On minutes reaching 10 it stores
  9 to `$0E25/$0E21/$0E19`, 5 to `$0E1D`, and 1 to `$0EFF` and `$0F01` (the
  finish flags; R-0012). The subframe `$0E29` was already reset, so the hold
  re-triggers every five updates, as `advance_timer_digits` returns true every
  fifth update. The routine writes no other gameplay state; `$81:C75E-C7CD` is
  HUD glyph stores plus a `JSL $828000` sound at 9:5x.
- **Palette cycle `$82:D382-D496`: confirmed.** While `$0B92` is set, CGRAM
  address `$60` receives 16 words from `$80:82AB+32k,$0B84*2`, then address 0
  receives `$80:84AB,$0B84*2`, then `$0B84=($0B84+1)&15`. Pack entry file offset
  683 (`$80:82AB`), 544 bytes. In the frozen WRAM series, `$0B84` ends frame n at
  `(n-1381)&15` for every frame from 1382 until result loading: all 5,343
  frames 1382-6724 in boundary-a and all 5,352 frames 1382-6733 in pause-a,
  including paused 6000-6009. The first departure is at loading.
- **Fade: confirmed.** NMI `$80:883F-8849`: `LDA $0FF1; SEC; SBC #$0F; BPL;
  LDA #0; STA $2100`. `$83:CCC1-CCC9` increments `$0FF1` up to 30.
- **Rider objects `$83:F0FF-F2D9`: confirmed.** The row masks from
  `$83:F2FF-F3F8` and `$83:F40C-F4FF` match the native header decoding bit for
  bit, including the ROR/ROL carry handling. The top clip skips row-0 words for
  both the pose and the overlay frame (`$2C/$2E`, `$18/$1A`). An overlay bit
  consumes the covered pose word (`$83:F237/F249`). Tile decode `$83:F253-F26B`
  matches `decode_rider_tile_reference`: the carry into `ADC #$27` is provably
  0. A slot unused now but used in the previous update receives `$27:8000`
  (`$83:F29C`). No read of `$0EF7` exists in banks `$80-$83`.
- **OAM `$82:ACAC-AE5E` (one-player branch, `$0DE1`=0): confirmed.** The Y
  window is `[$FFD7,$00E1)`. X is shifted by `$03F1` and bounded by
  `$0425/$0427`. OAM X is the low byte of `(x-camera)&$0D4F`, with the ninth bit
  set on a negative scaled X. Attribute bit 6 comes from `$0BA7/$0BA9`. A
  hidden rider stores X/Y `$70` with the ninth bit set. The clip is 1 below
  `$FFE0` and 2 from `$00D0`.
- **Look latch clears: confirmed, and I agree they are unreachable in the
  one-player race except for the recorded opponent caveat.** `$82:857F`
  (`STA $0D5B`, player) and `$82:87AD` (`STA $0D5D`, opponent) are the only
  latch stores in the look routine. Byte-pattern search finds the other
  `$0D5B/$0D5D` stores only in the idle routine's copy-back (`$82:8E16`,
  `$82:9304`). The only `JSR $8927` in bank `$82` is at `$82:8768`, in the
  opponent's copy. So the player's cursor and end stay 0, and its clear needs
  its delay to go from 0 through 65,536 decrements. Any no-target update with
  the head nonzero, or any update without the latch, resets it; a target update
  pauses it. Even if every race update were a player look update, the clock
  limit bounds a race to about 30,450 updates, fewer than 65,536. From the ROM
  tables the shortest opponent sequence is sequence 4 (`$17:C614`
  `$140-$15A`), 145 look updates. My view: the argument is sound for the
  player. For the opponent it depends on the recovered AI never idling mid-race
  for about 290 updates, which is evidenced but not proven. This is acceptable
  as the recorded residual.
- **Audio-upload classification: confirmed.** `$82:812A-8150` sets bank `$65`
  to `$10` and `$63` to `$8000`. It only advances the pointer, and its carry
  path keeps the offset at `$8000` or above. `$82:8151` wraps to `$8000` and
  increments the bank. In `$82:8082-8129` and `$82:82A9-831E`, values read
  through `[$63]` go to `$2141-$2143`, to X or Y loop counts, or to the pushed
  jump address compared with `$FFC0`. The only RAM stores are constants to
  `$7E2000/2002/2004`. `$82:8088-809A` is the `$BBAA` reset handshake wait that
  explains the time-out load delay.

### Gates reproduced

| Command | rc | Result |
| --- | --- | --- |
| `python3 tools/project.py test --suite synthetic --preset P` for lab-debug, lab-sanitize, app-debug, app-sanitize | 0 x4 | **406/406 passed on each, 22 ctest, 0 skipped**, source `9b43f3c` clean |
| `python3 -m tools.unirally_lab.native.zoom_zoo_playable compare --reference <impl>/boundary-a --repeat <impl>/boundary-b --contract tests/manifests/native/zoom-zoo-playable-primary-v11.freeze.json --binary build/app-debug/src/core/zoom_zoo_runner --pack local/classic-crawler-two-tracks-v7.pack --out artifacts/m4-16-final-review/gates/primary-app-debug.json` | 0 | passed: 1376-7600, 742 bytes, rows `b4a34af7...`, **757 fresh restores**, full restart; events 6484/6488, loading 6725, visible 6833, stable 6839; 08:00:45-08:05:47 UTC |
| same with `<rider-art>/artifacts/m4-16-idle/captures/late-start-a` / `-b` and `zoom-zoo-playable-idle-late-start-v11.freeze.json` | 0 | passed: 1376-8100, **801 restores**, restart; player 7418 / opponent 6488, loading 7659, visible 7767, player_lost; 08:06:15-08:11:33 |
| `native presentation-check` DRAGSTER winner and loser (lab-debug, copied fixtures, `classic-crawler-dragster.pack` `5c1fc5b0...`) | 0 / 0 | **36/697/279/445/653/962/961** and **1073**, all unchanged |
| app-sanitize `unirally --content-pack v7 --track zoom-zoo --updates 6000 --hidden --fixed-controller-mask 128` | 0 | 4,288 frames through `LivePresentation::render_zoom`, **rider-pose fallback frames 0**, no sanitizer output; not a completed race (totals 60000/9810) |
| reviewer native timelines, recaptures, rider-pixel harness, time-out diagnostic (commands above) | 0 | as reported |

Primary and idle-late-start gates ran on app-debug only. The implementer's
`51c05f6` sanitizer gates were not repeated. Against `51c05f6`, `src`, `tools`
and `tests` differ only in `presentation.cpp` and `presentation_tests.cpp`
(`4df0c29`), so the runner's gameplay path is unchanged since that gated
commit. The app-sanitize suite, time-out rows and renders above exercise the
new clock-limit and presentation paths.

### Reviewer checklist

- **Baselines.** No existing freeze or manifest changed.
  `zoom-zoo-playable-primary-v11.freeze.json`, `zoom-zoo-playable-v2.json`, the
  DRAGSTER presentation manifests and the M4-12 to M4-15 contracts are untouched
  in `06abab7..9b43f3c`. New files are additive: idle late-start freeze, time-out
  inventory marked `"acceptance": false`, two case files. The two-track rules
  file's profile moved v5 to v7 with appended entries. v4-v6 were unaccepted
  experiments, and DRAGSTER v1 stays supported and unchanged.
- **Weakened comparisons.** `zoom_zoo_playable.py` is unchanged. The restore
  validator now admits a lap-short finish only when both riders are finished at
  the held 9:59.9 clock with the 60000 total. That is the original's behaviour;
  tests reject the 9:59.8x, one-finished and summed-total mutants.
- **Masked skips.** None: 406 passed, 0 skipped, on every preset.
- **Undefined arithmetic.** Read every new arithmetic path. Look and object
  code wraps through `std::uint16_t`, and unsigned shifts stay in range.
  `(screen_y-oam.y)&0xff` is defined in C++20. `1U<<button` is guarded below 32.
  UBSan and ASan passed the suite, the time-out run and the renders. The GCC
  sign-conversion error (finding 1) is a diagnostic, not UB.
- **Readability (D-0003).** `rider_object` and `rider_look` are small, named and
  address-cited, with fail-closed bounds. `render_zoom_zoo` keeps the dense
  one-line style that trips GCC (finding 1). That is acceptable once reformatted.
- **Content.** No ROM-derived bytes in tracked files. The diff contains no
  binaries, and no byte runs in source, tests or docs. The rider tests use
  synthetic tables only. Pack entries are offsets and hashes.

### Recorded limitations — judgement

Acceptable as recorded: the time-out result loading two updates early (audio
handshake timing, time-out path only, reproduced exactly); the post-finish
caption timing (native three pictures early, see D4); the opponent latch residual; poses `$1432/$1433` and 5171 and up
failing closed with the held pose in live play; the omitted original overlays;
the single-state runner omitting look overlays (the `--timeline` form is exact).
The gamepad clause rests on the user's witness plus logs. The 05:47 rider-art
playtest report was overwritten, so only its quoted console text survives; I did
not re-exercise live input. DRAGSTER ignoring the 10:00 limit is a real gap in
accepted DRAGSTER work, outside this range; it should get its own tracked
follow-up rather than stay only in this record.

### Not assessed, and why

- The formatting fix `333fb58` / `36c37f6`: not my candidate.
- Visible live window play on this candidate: live evidence was accepted in
  earlier rounds, and no computer-use approval was requested. The hidden
  app-sanitize run covers the live render path, but not keyboard or gamepad
  delivery.
- Primary and idle gates on app-sanitize, the M4-15 matrix, and the
  M4-12 to M4-14 historical matrix on `9b43f3c`: not repeated here.
- Edge cases without original evidence: a result for a rider timed out with no
  completed lap (native would show `NO TIME` best lap); equal finish totals
  (native HUD shows LOSER and the result shows RUNNER UP).
- The time-out restore set (13 app-sanitize restores to 31927) was not
  repeated; I reproduced the row equality instead.

Evidence (ignored): `artifacts/m4-16-final-review/` (`gates/`, `native/`,
`orig/`, `render/`, `render-batch/`, `render-sanitize/`, `sbs/`, `sbs2/`,
`harness/`, `rider-pixel-report.json`, `tools/`).

## Re-review — candidate `75626f8` — 17 September 2026

Reviewing model: **Claude Opus 5**, the same fresh reviewer as the round above,
in `.worktrees/m4-16-final-review`. `review/m4-16-final` could not
fast-forward past my review commit `9559d0b`, so I merged
`origin/codex/m4-16-playable-zoom-zoo` (`75626f85f6b4...`, equal to the remote
ref) as `5ea8444`. `git diff 75626f8 5ea8444 -- src tests tools CMakeLists.txt
CMakePresets.json` is empty; the merge adds only this file. Gate reports record
`5ea8444` with an empty source diff for that reason. Started 08:36:03 UTC,
checks finished 08:57:50 UTC. No implementation file was changed.

### Verdict: approve

Finding 1 (Linux CI) is closed, and hosted CI on the exact candidate passes on
both platforms. Findings 2 and 3 are fixed exactly and change no other picture.
Finding 4 is corrected in R-0035 and the task handover. `docs/STATE.md` is
deferred to integration, which is acceptable if it is done there. Findings 5
and 6 remain as recorded limitations, which I accept (below). No new blocking
finding.

### 1. `333fb58` and `36c37f6` compute the same values

- `ui_text`: the one-line body `{glyph; for row for col if(...)pixel(...); x+=6;}`
  is reformatted into the same statements in the same order. `x+=6` still runs
  once per character, after both loops.
- BG1 tile flip: `if(a)px=15-px;if(b)py=15-py;` is split onto two lines.
  Same statements.
- HUD clock: the timer digits are `std::uint16_t` and promote to `int`, so the
  old sum was computed in `int` and converted once to `unsigned`. The largest
  possible sum is 9*6000+5*1000+9*100+9*10+4*2 = 59,998. It fits both types,
  so `*6000U` and the other unsigned literals give the same value.
- `sdl_main.cpp` gamepad counter: the single-line `if` becomes a braced-style
  three-line block with the same effect.
- `rider_object.cpp` plane read: `tile[at]` is a `uint8_t` in 0..255, and the
  shift `bit` is 0..7. `(int)v >> bit & 1U` and `(unsigned)v >> bit & 1U` give
  the same bit.
- Observed: the app-debug `zoom_zoo_runner` rebuilt at the new candidate is
  byte-identical to the `9b43f3c` build (`bd8b913677a3d197`).

### 2. Background camera change

The candidate uses `previous_update->race.camera` whenever the previous update
is a race state, in place of `camera - velocity`. To check it I linked a
harness (`artifacts/m4-16-final-review/harness/all_frames.cpp`, `56b9d22f...`)
twice: once against the preserved `9b43f3c` app-debug libraries and once
against the new ones. It replays each native timeline as the `--timeline`
runner does and digests every rendered picture. For each frame it also records
whether `previous camera == camera - velocity`.

| Timeline | Pictures | Changed | Frames where camera-velocity is not the previous camera |
| --- | ---: | ---: | ---: |
| boundary-a (primary) | 6,224 | 0 | 0 |
| brake-a | 6,224 | 0 | 0 |
| loss-a | 6,224 | 0 | 0 |
| compound-reverse-a | 6,224 | 0 | 0 |
| wrong-way-a | 8,124 | 0 | 0 |
| pause-a | 6,224 | **11: 6000-6010** | **11: 6000-6010** |
| stop-timeout-a | 30,924 | 192, all in 31584-31822 (time-out HUD, finding 3) | 0 |

So the camera change alters exactly the pictures whose placement was wrong and
nothing else, across 70,168 pictures including every result picture.

- **Resume frame 6010** against my digest-checked recapture: 283 of 54,272
  pixels below the HUD band differ (was 31,094). Their box, `[17,17]-[238,30]`,
  is the same HUD-glyph box as in unpaused 6011, which has the same 283 count.
  Luminance correlation at no shift is 0.9974 (was 0.7677).
- **Paused 6000-6009**: pixel equality stays 0, because the authored dimming
  and menu panel differ from the original. I measured luminance correlation
  outside the native HUD band and pause panel (`tools/lumcorr.py`):

  | Placement | Correlation at no shift | Best shift |
  | --- | --- | --- |
  | New, every paused frame | 0.9633 | (0,0) |
  | New, also excluding the original's text rows | 0.9809 | (0,0) |
  | Old | 0.7289 | 0.9222 at (9,5) |

  My numbers differ from the record's 0.954 / 0.910 at (6,4) because the
  region differs. The conclusion is the same: the background is now in place
  under the menu.
- **Unpaused frames 5995-5999 and 6011-6030** give the same per-frame counts as
  before. The **frozen visual scenes** reproduce every hash recorded in round
  one: `0bed1ceb af38a76b 4802eb04 66c713f1 06918740 1d9e5f36 0514f3d7 14c8df70
  c1d2cffe 4f4c7a6d`, plus brake 6724 `b7706d1d`. So the round-one visual
  verdicts carry over unchanged.
- **Previous state equal to the current state** (single-state runner,
  initialization, live restart): the new rule uses the current camera instead
  of `camera - velocity`. Those pictures are black under the fade (1376
  reproduces `0bed1ceb`) until real previous updates exist from 1393, so
  nothing visible changes.

### 3. Time-out clock (finding 3)

New runner renders of stop-timeout 31582-31700: below the HUD band they are
identical to round one. The HUD reads `9:59.98` at 31582, which shows update
31581, before the limit. From 31583 it holds `9:59.90` at 31584, 31586, 31588,
31600 and 31700, where it had cycled .92/.96/.90/.94/.94. The original shows
`9:59:9` throughout. `presentation_tests` now sets subframe 3 and expects
`9:59.90`.

### 4. Gates and CI on this candidate

| Command | rc | Result |
| --- | --- | --- |
| `python3 tools/project.py build --preset P` for lab-debug, lab-sanitize, app-debug, app-sanitize | 0 x4 | app-debug presentation runner `98d981c3...` (was `76e7444a`), `zoom_zoo_runner` `bd8b9136...` (unchanged); app-sanitize `zoom_zoo_runner` `ecd485a5...` |
| `python3 tools/project.py test --suite synthetic --preset P`, all four presets | 0 x4 | **406/406 passed each, 22 ctest, 0 skipped** |
| `python3 -m tools.unirally_lab.native.zoom_zoo_playable compare --reference <impl>/boundary-a --repeat <impl>/boundary-b --contract tests/manifests/native/zoom-zoo-playable-primary-v11.freeze.json --binary build/app-debug/src/core/zoom_zoo_runner --pack local/classic-crawler-two-tracks-v7.pack --out artifacts/m4-16-final-review/rereview/gates/primary-app-debug.json` | 0 | passed, **rows `b4a34af722b26693...` unchanged**, 1376-7600, 742 bytes, **757 restores**, restart; 6484/6488, loading 6725, visible 6833; 08:38:35-08:44:08 UTC |
| same with `--binary build/app-sanitize/src/core/zoom_zoo_runner` | 0 | passed, rows `b4a34af7...`, **757 restores**, restart, no sanitizer report; 08:44:08-08:57:50 |
| same with `<rider-art>/artifacts/m4-16-idle/captures/late-start-a` / `-b`, the idle late-start v11 freeze and app-debug (`gates/idle-direct-app-debug.json`) | 0 | passed, rows `205d1705...`, 1376-8100, **801 restores**, restart; player 7418 / opponent 6488, loading 7659, visible 7767, player_lost; 08:49:14-08:54:29 |
| app-sanitize `unirally ... --track zoom-zoo --updates 3000 --hidden --fixed-controller-mask 128` | 0 | 2,089 frames through the live renderer, 0 pose fallbacks, no sanitizer output |
| `gh run view 35200514079` | - | head `75626f85f6b4...`, **completed success**: `lab (ubuntu-24.04)` success, including "Build and test with sanitizers (Linux)"; `lab (macos-15)` success. No failed or skipped checks; the only `[missing]` lines are the optional pre-bootstrap doctor items |
| `gh run view 35197851820` (`36c37f6`) | - | success on both platforms |

The idle late-start gate on app-sanitize was still queued behind my gate
script when this section was written, so it is not claimed. The implementer
reports it for the `51c05f6` gameplay code. The runner is unchanged since then,
and its app-debug binary is byte-identical.

### Remaining minor findings: accepted as limitations

- **Opponent finish time also shown after both finish.** Accepted. It is
  original decoration, which criterion 2 excludes, and the record now covers
  the whole period from the opponent's finish.
- **Centred finish text early.** Accepted: the values are correct and it lasts
  two to three pictures. Two precision notes for the record. My round-one
  "6488" was slightly late: in the recapture the finish-time row first appears
  at 6487, partly covered by the flag, and WINNER at 6488. The task record says
  the original's banner "slides in over several frames". Measured, the text
  rows stay at y 43 and 83 from 6487/6488 on; what animates is the flag
  sweeping across them, so the visible width of the text changes. Native shows
  both from 6485.

### Minor residuals, non-blocking, for closeout

- `docs/STATE.md` is still stale; the record assigns it to integration.
- `tasks/NEXT_SESSION.md` line 36 still says "405 synthetic checks"; it is 406.

Evidence (ignored): `artifacts/m4-16-final-review/rereview/` (`gates/`,
`frames/`, `render/`, `render-pause/`, `render-timeout/`, test reports,
`pause-a-6005.png`, `pause-a-6010.png`, `timeout-hud.png`) and the preserved
`harness/libs-9b43f3c/`.

### Addendum — the queued gates finished (09:17 UTC)

- **Idle late-start gate on app-sanitize: passed.** It ran 09:03:01-09:17:38
  UTC at `c5f4566`, the review commit, with an empty diff. Its `src`, `tests`
  and `tools` equal `75626f8`. Rows `205d1705...`, **801 restores**, restart,
  player_lost at 7418/6488. The runner `ecd485a5...` gave no sanitizer report.
  This closes the item marked "not claimed" above.
- **A duplicate idle app-debug run from my queued script aborted, rc 1:**
  "source/binary/pack changed during validation". It ran 08:57:50-09:03:01. I
  committed `c5f4566` at 08:58:57, which moved `HEAD` during the run, and the
  gate's identity guard (`zoom_zoo_playable.py` lines 151-153) rejected it as
  intended. I discard it as a mixed run. It is not a failure of the candidate.
  The app-debug evidence is the direct run above: 08:49:14-08:54:29 at
  `5ea8444`, 801 restores, passed.

The verdict is unchanged: **approve**.
