# AUDIO-DECISION - choose the native audio path and its evidence

## Assignment

- Status: record outcome complete; integration via [PR #49](https://github.com/malmazuke/unirally-reconstruction/pull/49), with actual merge/check/synchronization status in the ignored closeout; claimed 30 September 2026 at 19:50 UTC.
- Milestone: M4 (original game coverage).
- Coordinator and primary: this Codex desktop session.
- Task provider: OpenAI, including children. Primary model/effort is not exposed
  by the runtime; no model switch is claimed.
- Tier: 3. This task changes records only, deciding the next audio outcome and
  correcting the already-delivered STUNT-EVENTS roadmap entry. No game, pack,
  reference adapter or baseline changes are authorized by this record alone.
- Routing: one bounded Astra/medium planning consultation under D-0004 because
  the audio architecture and evidence boundary affect later implementation.
  No independent review is required for this records-only task (D-0008).
- Quota: weekly Codex used 11%, remaining 89% at startup; discretionary boundary
  31% used, final review/recovery reserve 20%. No reset or spending authorized.
  Account-wide changes may include unrelated work.
- Base: `16675d0bd444197913f3460efcf4ae76a6a8c812`; LEAGUE PR #48 merged,
  closeout present, clean main equals origin/main after fetch.
- Branch/worktree: `codex/audio-decision`, `.worktrees/audio-decision`.
- Owned paths: this record, the audio decision/research and first task records,
  COVERAGE-ROADMAP, STATE, NEXT_SESSION, task registry, validation documentation and regenerated static citation maps.
- Evidence: main `local/evidence/audio-decision/`; closeout/gate logs: main
  `artifacts/audio-decision-integration/`. No evidence under the worktree.
- Checkpoint: this record; reassess every 45 minutes, durable checkpoint every
  ten minutes. One child at most. No total elapsed-time stop invented.

## Outcome and boundaries

Choose how native C++ will generate music and effects from locally extracted
content without executing original 65816 or SPC700 code in the product. Inventory
the current audio capture limits, test repeatability on cold original runs and
queue a concrete first implementation outcome with acceptance gates.

STUNT-EVENTS was already delivered by STUNT-EVENT-RACE (#39), STUNT-RESULT (#40)
and STUNT-HUD (#42), before the later menu modes. Their research covers nine
tour stunt events; the roadmap's four were the four cold-start tracks. This task
does not expand their tested domain or claim stunt edge cases are complete.

## Inputs and prerequisites

- PAL ROM SHA-256 `a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`;
  private locator in main `local/rom-location.txt`.
- Pinned bsnes commit `7d5aa1e656b9171524d01b1b22917197d8121cb4`, audited core
  `e59bf88d4fc922c9fe3b5438e65ff3a6909d24e1628f0f87141c8de17699a91b`.
- Static map reading: main `artifacts/static-map/bank-82.lst`,
  `$82:8000-8336` (sound queue, port handshakes and upload); bank-83 listing
  `$83:A614/A721` (award/menu loading callers). Listing generates hypotheses;
  dynamic evidence is required for claims.
- Existing adapter hashes callback PCM but exports no APU RAM or DSP writes.
  D-0001 excludes restored resampled audio from exact acceptance.

## Acceptance

| Criterion | Experiment/check | Expected result | Artifact |
| --- | --- | --- | --- |
| Existing stunt outcome | Inspect merged tasks, R-0066/67/68 and their retained evidence | Queue corrected without a broader accuracy claim | record audit |
| Audio observation | Two fresh-process cold-start runs with pinned core and controller schedule | Exact uninterrupted callback sample counts/digests | private reports |
| Native design | Bounded consultation plus primary source inspection | Explicit driver/data/DSP boundary, timing and restore gates | decision and consultation |
| Next outcome | Scoped task record | Concrete prerequisite and native acceptance, no emulator fallback | queued task |
| Integration | Markdown-only staged diff, hosted required checks, PR merge commit | Green final head; synchronized clean main | closeout |

## Evidence and attempts

- Startup verified the LEAGUE closeout and local/remote main identity.
- R-0066 has seven riding captures across BOWL, JUMPS, HILL CLIMB and DOWNER
  and nine idle stunt captures. R-0067 delivers result/records; R-0068 delivers
  stunt race pictures, including NEON. The queued four-track outcome is stale.
- Read the static listing before designing the new audio observations. Command
  semantic names remain provisional until their dynamic capture is inspected.

## Handoff

- Evidence/code base: `16675d0`; decision/map candidate `3b3d48d48b1501dc9a1a5f56ace67b9dc8976c37` is committed and pushed. The final record tip is recorded in the closeout.
- The cold audio and CPU transport experiments are complete; D-0009 and
  AUDIO-TITLE-MENU record the adopted design and next native capability.
- Next capability: AUDIO-TITLE-MENU. First verify [PR #49](https://github.com/malmazuke/unirally-reconstruction/pull/49) and main `artifacts/audio-decision-integration/closeout.json`, then reproduce R-0074 and implement its coupled raw-audio observation experiment.
- No native audio implementation, playable audio or audio restore claim yet.
- Source and accepted baselines unchanged. No broad native suites rerun for
  the records-only diff; required hosted checks will run on the final tip.

## Review and integration

- Tier 3: independent review not required. Planning consultation is advisory,
  not review or evidence of audio accuracy.
- [PR #49](https://github.com/malmazuke/unirally-reconstruction/pull/49) holds the decision/map candidate and final tracked handoff. Main `artifacts/audio-decision-integration/closeout.json` records the actual final head, required checks, merge commit, synchronization, quota and cleanup. Final-tip CI and merge are pending at this record commit; neither is preclaimed.
- The primary reads all conversation/review and line comments immediately before merging. Tier 3 requires no review comment; the planning consultation is not a substitute for the next capability's tier-1 review.
- All evidence and integration logs already use their canonical main homes. Closing cleanup removes only this session's `.worktrees/audio-decision` (private locator/cache links; no build directory created) and local `codex/audio-decision` after verifying every commit is on origin/main. It preserves all cited original/failed captures and the pre-existing playtest checkout. No evidence move or duplicate deletion is planned.
- No M4 milestone is accepted by this decision; no tag, release or deployment is due.

## Checkpoint - 30 September 2026 20:08 UTC

- Doctor passes required checks; system ninja and optional Jev key absent, with
  the isolated ninja available. No key/reset/spending action taken.
- Three cold PAL schedules run twice: all 2,000 audio count/hash frame rows
  reproduce exactly. Boot 575,040 delivered stereo pairs; both 700-frame menu
  schedules 671,040 each. Down at 450 changes WRAM/audio at 450 and queues
  087F/0203; semantic effect names remain provisional (R-0074).
- Both access captures preserve matching uninterrupted memory/register and A/V
  digests; watched logs untruncated, rings complete. Each declares 12,179
  unresolved accesses and zero unresolved stores. No sub-frame clock/APU claim.
- First access attempt rejected a redundant event port; invalid manifest and
  failed report retained. Corrected schema preserves the intended timeline.
- Fresh bounded Astra/medium consultation completed. Adopted native driver/data/
  hardware/output boundaries, raw PCM/event capture and first title/menu outcome.
  Corrected its prompt's unsupported upload directory address from the listing.
  It performed no source edit, implementation or independent acceptance review.
- Verified merged #39/#40/#42 and their ancestors of main; STUNT-EVENTS was
  already fulfilled. Corrected queue and STUNT-HUD's stale in-review status.
- Weekly usage remains 11%, discretionary boundary 31%; no provider change.
- No native audio, imported DSP source or changed pack/reference contract.
  Tier 3 includes maps regenerated with the same four raw inputs; no independent
  review required. Native/sanitizer suites are not rerun for unchanged code.

## Records candidate - 30 September 2026 20:15 UTC

- D-0009 and R-0074 are complete; AUDIO-TITLE-MENU is ready after integration.
  All original callback/access evidence and the advisory consultation are in main
  local/evidence/audio-decision; manifest hashes are in the integration directory.
- Static map regeneration with the four unchanged raw captures passes: 53,062
  sites, zero disagreements; class totals and routine map unchanged. Two new
  observed citation addresses bring the citation count to 1,505. No new static
  inference or observed gameplay classification is introduced.
- Focused checks: `python3 -m unittest discover -s tests/tooling -p test_static_map.py`
  passes 13 tests; the same command with `test_native_symbols.py` passes 24.
  `git diff --check` passes. Source-map-identity confirms src/tools/tests/build/CI
  files unchanged; only citation totals differ in code-banks.map.json. Its first
  local assertion used the wrong top-level key, corrected after schema inspection;
  no expected results or map output were changed to make it pass.
- Native gameplay/sanitizer/presentation suites are not rerun for this tier-3
  records/citation-only task; no new native pass is claimed. Hosted final-tip
  required checks remain pending until the PR is open.
- `doctor.json` SHA-256 `7cbcf79185507c1e1a84173780f71d11b65c3b9f0395ad51a762e911821dcb21`.
- `boot-verify.json` SHA-256 `38a4b5d6602c8e6cd631531986686464f69b8d8f52fe14d73f50859fddff8242`.
- `menu-base-verify.json` SHA-256 `dae50ef7d72f1074853b1b746c74375a8c77a4ab1e4bc294cb7bb644556b8fa7`.
- `menu-down-verify.json` SHA-256 `3325d9d63c07667733e7d0bf571b0c233094f25faf7140aa878b790e48d4a450`.
- `menu-base-access.json` SHA-256 `71645fc54d43464f8ea7a36f68b217a9a41daa42cd7a1d05cf50080ce2bab900`.
- `menu-down-access-corrected.json` SHA-256 `05fd92dff160c36fa67651cd8281f7dac2ba1f8c49a8dc0c7f65f2c494335d20`.
- `static-map.json` SHA-256 `86f1f336cf15a14b1fe845a66a303370bfba6c96f953cf040afe4c14de63ad69`.
- `source-map-identity.json` SHA-256 `3e79a4f7117d84229a44432287f4c02cef090ab303abdfd8aca0a31dc6e7f218`.
- `evidence-manifest.json` SHA-256 `04ca20ef4cc925456f72081570275e5d9592ee4053f4947690d277ace63be0d4`.

## Final tracked handoff - 30 September 2026 20:20 UTC

- Staged diff inspected: only Markdown and the generated citation map/labels;
  no original content, PCM, ROM bytes, states, credentials or traces tracked.
  Relative Markdown links resolve. Source-map classification/routines and every
  src/tools/tests/build/CI file are unchanged from the evidence base.
- Required hosted checks are running on PR #49; initial read finds no owner
  comments, reviews or line comments. The final read and exact-tip results
  belong in closeout. A future green run is not claimed here.
- Scope completed: D-0009 and first native audio capability queued; stale stunt
  queue reconciled. The product remains silent. No waveform, driver, DSP model
  or native/live audio acceptance exists yet.
