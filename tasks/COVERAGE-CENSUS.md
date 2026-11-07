# COVERAGE-CENSUS - how far the native game is from the whole original

## Assignment

- Status: **accepted** 8 November 2026 (tier 3, [#96](https://github.com/malmazuke/unirally-reconstruction/pull/96)),
  integrated by merge commit. Claimed 7 November 2026 21:30 UTC on main `dffbcf44`, on the user's instruction
  ("Start", answering whether to start a fresh coverage inventory). Claude Code desktop app,
  Claude Opus 5.5 (`claude-opus-5-5`): coordinator and integrator; a worker subagent runs the
  inventory. Run under [D-0013](../docs/decisions/D-0013-task-time-and-disk-budget.md): claim to
  merge against the 6-hour target, new evidence under 1 GB. Usage at the claim: 5-hour 6%,
  weekly 8%.
- Branch `task/coverage-census` in `.worktrees/coverage-census`; evidence in main
  `local/evidence/coverage-census/`.
- Task provider: Anthropic.
- Milestone: M4 (original game coverage).
- Review tier: **3** (D-0008: records, laboratory evidence and a regenerated map; no `src/core`,
  `src/app`, tool or gameplay change). A queued task carries its own tier. If the inventory needs
  a tooling change, that change moves to a queued task rather than raising this one's tier.
- Dependencies: [COVERAGE-GAPS](COVERAGE-GAPS.md) (R-0089, its corpus scripts and
  `recorded-limits.md`), [DATA-COVERAGE](DATA-COVERAGE.md) (R-0093, the access record and the
  corpus coverage), the static map (R-0045), R-0090 to R-0108.

## Why now

R-0089's queue (9 October) is done: SAVE-FILES through RACE-BRANCH-REMAINDER are integrated, and
no task is queued. The user's goal is the whole game natively ("100% coverage from the ROM").
The static map still classes 31,425 of the code banks' 131,072 bytes (24.0%) as unknown and
11,231 as inferred (decoded but never seen running), and native cites 581 of its 759 routines.
Nobody knows how much of that is data, padding, dead code or real behaviour native lacks. This
task measures it and queues what is left.

## Outcome and boundaries

- **The corpus, brought up to date.** R-0089's corpus plus every replay manifest and coverage
  capture added since (R-0090 to R-0108, warm boots included), so the measure reflects every
  accepted task. Coverage captured only where no existing raw coverage covers a manifest.
- **Every byte of banks `$80-$83` accounted for.** Each unknown and inferred byte is put in one
  class with its evidence: executed code (new captures), data read by the original (R-0093's
  access record and any new one), fill, data reached by a pointer or index the listing shows,
  code shown unreachable, code reachable but never captured (with the feature that reaches it),
  and unexplained. Unexplained is a measured remainder, not a guess.
- **Every routine attributed.** For each of the 759 routines: run by the original and cited by
  native, run but not cited (and whether native implements the behaviour under another citation
  or lacks it), or never run (and why).
- **Outside the code banks.** The data banks' unread bytes (R-0093) and what native does not
  extract, brought up to date; the sound program's status as the records give it.
- **The records' limits, brought up to date.** `recorded-limits.md` refreshed from every record
  accepted since R-0089: what each later task closed, and what each added.
- **The app against the runner.** What the original does that the shipped app (`unirally`) does
  not, as the records state it.
- **Result:** a research record (R-0109) with one headline measure of what is left, what "100%"
  can mean given unreachable code, and a queue of tasks in order, each with its domain and tier.
  Records, evidence and the static map regenerated with any new coverage input only: no native or
  tooling change.

## Results

[R-0109](../docs/research/R-0109-coverage-census.md) has the census, the headline and the queue.

- **Corpus up to date:** R-0089's 236 runs plus 723 more: 149 replay manifests (5 warm boots, 20
  OPTIONS runs R-0089's key had merged) and 537 track_reference captures (123 with a cartridge RAM
  preload), 32 + 3 of them reused from existing coverage. 651 new coverage captures, 3,284,171
  frames, all `status=passed`; every preloaded or warm image written after frame 0, and all 534
  track_reference runs equal their captures' work RAM and video on every recorded frame
  (1,821,379 frames, 0 differences). Merged as the static map's eighth input
  (`docs/map/coverage-census.map.json`).
- **Static map:** observed 88,343 to 94,637, inferred 11,231 to 5,311, data 73 to 70, unknown
  31,425 to 31,054 (24.0% to 23.7%); routines 759 to 762, cited 581 to 584.
- **Every byte:** the 42,656 bytes that were unknown or inferred are executed 6,294, read as data
  11,255, fill 12,610, the rest of a read or named table 2,516, inside a native-cited range 707,
  code shown unreachable 1,037, reachable and never captured 6, reachability open (named) 2,361,
  unexplained 5,870 (1,907 small never-taken arms, 868 gap-sweep candidates, 3,095 unknown).
- **Headline:** 8,237 bytes, 6.3% of the code banks (7.0% of what is not fill or unreachable), are
  neither run, read, named nor shown unreachable; beside them the records' open limits (51 of
  R-0089's, 77 added since) and 11 run routines that write state native cites nowhere.
- **Routines (762):** 581 run and cited; 165 run and not cited (62 trampolines, 16 hardware-only,
  32 direct-page helpers, 44 writing state native cites, 11 writing uncited state, read one by
  one); 16 never run (14 unreachable, 2 open).
- **Outside the code banks:** with 118 targeted access captures the data banks read rise from 78.9%
  to 84.8%; 15 of the 20 track streams R-0093's corpus never unpacked are now read; R-0093's other
  open entries stay open (bank `$A2` still 0 bytes read).
- **Queue:** APP-PARITY (tier 2), SAVE-IMAGE-EXACT (1), CENSUS-REMAINDER (3), OPEN-BRANCHES (1),
  SOUND-PROCESSOR-TIMING (1), CPU-CYCLE-TIMING (1), PICTURE-RESIDUALS (2); a one-paragraph task
  record each, status queued, and a registry row each.

## Decisions (worker)

- **Warm boots and preloads as writes after frame 0** (a manifest cannot preload): every image byte
  that is not `$FF` written after frame 0. Checked on every track_reference run against its own
  capture (above); R-0100 had shown it for a warm image.
- **Runs keyed by pads, length and cartridge RAM writes** (R-0089 keyed by pads and length): the
  finer key found 20 OPTIONS runs R-0089 had merged.
- **DRAGSTER-ORDINARY-CONTROLS in two batches:** one in ten first; that sample ran 8 instructions
  (`$82:96A9-96C0`) no other run did, so the other 268 were captured too (28 more observed bytes).
- **Targeted access captures** (118 runs, 583,943 frames) instead of all 651: an access capture
  runs at about a quarter of a coverage capture's speed; the subset is the runs most likely to read
  what R-0093's corpus did not (locked tours, tile pairs, league tours, warm boots, preloaded
  races, HUNTER-EFFECTS one in six). The rest is CENSUS-REMAINDER's.
- **Script-driven warm boots left out** (SAVE-FILES' sweeps, CARTRIDGE-OPTION-BITS', PODIUM-AUDIO's
  and APP-LEAGUE-RACE's routes): not manifest-shaped; recorded as a limit.
- **The census's rules** (precedence executed, read, fill of 16+ equal bytes, table to the next
  anchor, native-cited range, attributed code, unexplained) are `census.py`'s; attributions with
  their arguments are `attrib.json`.
- **No tooling change**: every script is evidence; `src/`, `tools/` and `tests/` untouched.

## Evidence

Main `local/evidence/coverage-census/` (128 MB after `compact_evidence.py`; 177 MB at the handoff): `scan_manifests.py` and `scan.json` (the manifest
scan), `make_corpus.py`, `plan.json` and `manifests.tar.zst` (the 651 manifests; `make_corpus.py`
writes them again byte for byte), `captures.sh`, `captures.out`, `captures-2.out`, `runs/*/`
(logs and capture scripts) and `runs-coverage.tar.zst` (the 651 raw coverage files),
`same_run.py` and `same_run.jsonl`, `merge.sh` and `coverage/merged.json` (the eighth static-map
input), `regen.sh` (eight inputs), `listing/` (the listing with eight inputs), `access.sh`,
`access-ids.txt`, `access/*.json` (the 118 reduced access records), `codebank_reads.py`,
`data_reads.py`, `track_shapes.py`, `listing.py`, `census.py`, `attrib.json`, `census.json`,
`routines.py`, `routines.json`, `report.py`, `report.md`, `recorded-limits.md` (R-0089's list
refreshed from R-0090 to R-0108 by a read-only subagent).

## Handoff

- Branch `task/coverage-census` on claim `b88f49e4` (main `dffbcf44`). Commit: the one that adds this record (its hash is in the closeout)
  (R-0109; `docs/map/coverage-census.map.json` and `.md`; `docs/map/static/` regenerated with eight
  inputs; the seven queued task records and registry rows; this record). Tier 3: records,
  evidence and a regenerated map; no `src/`, `tools/` or `tests/` change.
- Commands (worktree root, `E=local/evidence/coverage-census`): `scan_manifests.py`,
  `make_corpus.py`; `local/locks/heavy-run.sh COVERAGE-CENSUS $E/captures.sh 10` twice (382 runs
  08:34-09:10, 269 runs 09:59-10:28 AEDT); `heavy-run.sh COVERAGE-CENSUS $E/access.sh 10`
  (09:19-09:59); `merge.sh`, `regen.sh`, `coverage disassemble` with eight inputs, then
  `codebank_reads.py`, `data_reads.py`, `track_shapes.py`, `census.py`, `routines.py`,
  `report.py`. `regen.sh` rerun after the last record edit: native-symbols and static-map
  `status=passed`, the tooling tests `OK`; `coverage native-symbols --check` passed (0 cited
  addresses without a record).
- Not run: `ctest`, lints, the gates (records only).
- What moved: the static map's observed bytes (+6,294) from the eighth input; `labels.json` from
  R-0109's citations. Future static-map regenerations take eight `--coverage` inputs; the eighth is
  `local/evidence/coverage-census/coverage/merged.json` (`regen.sh` passes all eight).
- Not covered (R-0109 Limits): access records of 533 of the 651 new runs; script-driven warm boots;
  the 11 uncited-state routines not checked against native; the unexplained remainder.
- Time, measured from timestamps: worker start 08:27 AEDT (21:27 UTC 7 November), handoff
  10:40 AEDT (23:40 UTC), about 2 h 13 min; machine time: captures about 1 h 05 min and
  access captures 40 min,
  during which the scripts and records were written.
- Next: [APP-PARITY](APP-PARITY.md), tier 2. The rest of the queue is in R-0109.

## Closeout (coordinator)

- Review: tier 3, no independent review (D-0008). The coordinator checked the class table's sum
  (42,656), the headline (8,237 = 6 + 2,361 + 1,907 + 868 + 3,095; 6.3% of 131,072, 7.0% of
  117,425), the four bank-tail fill blocks against the ROM (all `$FF`) and the diff's scope (no
  `src/`, `tools/` or `tests/` file).
- Gates (D-0012): GATES-RESULT
- Evidence: `python3 tools/compact_evidence.py --skip-recent 0 local/evidence/coverage-census`
  compressed 7 files (0.05 GB); 128 MB remain. Worktree and branch removed after the merge.
