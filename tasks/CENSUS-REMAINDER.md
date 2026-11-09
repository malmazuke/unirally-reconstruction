# CENSUS-REMAINDER - the code-bank and data-bank bytes R-0109 leaves unexplained

## Assignment

- Status: **accepted** 9 November 2026 (tier 3, [#100](https://github.com/malmazuke/unirally-reconstruction/pull/100)),
  integrated by merge commit. Claimed 8 November 2026 21:33 UTC on main `2a38ec3e`, on the user's instruction
  ("Start CENSUS-REMAINDER"). Queued 8 November 2026 by [COVERAGE-CENSUS](COVERAGE-CENSUS.md)
  (R-0109, queue item 3). Claude Code desktop app, Claude Opus 5.5 (`claude-opus-5-5`):
  coordinator and integrator; a worker subagent runs the census. Run under
  [D-0013](../docs/decisions/D-0013-task-time-and-disk-budget.md): claim to merge against the
  6-hour target, new evidence under 1 GB. Usage at the claim: 5-hour 8%, weekly 26%.
- Branch `task/census-remainder` in `.worktrees/census-remainder`; evidence in main
  `local/evidence/census-remainder/`.
- Task provider: Anthropic.
- Milestone: M4. Review tier: **3** (records, laboratory evidence and a regenerated map).

R-0109 accounts for every byte of banks `$80-$83` but leaves a measured remainder: unknown bytes no
capture reads and no operand names, gap-sweep candidates nothing reaches statically, and small branch
arms of run routines; its data reads come from access records of only 354 of the corpus's runs.
Outside the code banks R-0093's open items stay open (entries `$47`, `$49`, `$4B`, `$4F`, `$51`, the
palettes `$B7-$BE`, `$03`, `$04`, `$3D`, bank `$A2`, `$94:957B`, `$94:9147`, `$97:D9F4`). Outcome:
access records for the rest of the corpus, each unexplained block read from the listing (computed
jumps, long pointers, tables reached through a pointer the record shows) and either reclassified with
evidence or left in a smaller, listed remainder; the data banks' open entries shown read, unreachable
or unused.

## Results

[R-0112](../docs/research/R-0112-census-remainder.md) has the tables and arguments.

- **Access records for the whole corpus:** the 533 runs R-0109 did not record, 2,700,228 frames,
  and the 32 runs whose coverage R-0109 reused without an access record (OPTIONS, WIPE-RAM,
  CREDITS-NAME, the race tasks), 96,882 frames: all 565 `status=passed`. With R-0093's 236 and
  R-0109's 118, all 919 runs now have one.
- **Code banks (R-0109's 42,656 bytes):** read 11,255 to 11,665 (the new records read only bytes
  already accounted for, and WIPE RAM's menu text); code shown unreachable 1,037 to 1,555 (a
  routine behind a JSR a BRA skips, the GAME PAK NOT FOUND screen behind the cartridge RAM size
  check, four bank-crossing arms the 45 track streams and the 69-item sample chain never take, a
  BRK no track reaches, the IRQ handler); a new class, unreferenced (no operand, immediate, PEA,
  word or long pointer names it, nothing falls into it), 3,415 bytes in 44 blocks; unknown bytes
  3,095 to 0; gap-sweep candidates 868 to 34 (the printer's handlers for codes `$FA`, `$F5`, `$F4`);
  small arms 1,907 to 1,875 (278 blocks, grouped by guard in `arms.md`; 91 with a named feature).
- **Headline:** 8,237 bytes (6.3%) to **4,276 bytes (3.3% of the code banks)**; 7,691 (5.9%) if
  unreferenced is counted as left.
- **Data banks:** read 84.8% to 86.7% (1,704,685 of 1,966,080); tracks 24 and 31-34 read in full;
  scenery `$1B`/`$1C` were already read; scenery `$00`, `$21`, `$23` and the sound items at
  `$94:9147`/`$94:957B` (chain items 59 and 61) are unused; assets `$47`, `$49`, `$4B`, `$4F`,
  `$51`, `$B7-$BE`, `$03`, `$04`, `$3D` are outside every computed index under the records' value
  ranges; `$97:D9F4` is past the last stunt-name record its readers can index; bank `$A2` (poses of
  4096 and more; the corpus's highest is 2,795) stays open.
- **Follow-up:** the arms with a named feature are added to [OPEN-BRANCHES](OPEN-BRANCHES.md)'s
  record; no new task.

## Decisions (worker)

- **Access records for all 533 runs**, largest first, 10 jobs, R-0093's method unchanged (a copy of
  R-0109's `access.sh` with the folder changed), **and for the 32 runs R-0109 reused**: the recount
  showed WIPE RAM's menu text unread although WIPE RAM's code ran, which led to the runs whose
  coverage R-0109 reused without an access record; 7 more minutes of captures.
- **A new class, unreferenced**, for bytes no reference of any form names and nothing falls into,
  kept apart from "code shown unreachable" because it assumes every computed transfer takes its
  target from a ROM constant; the headline is given with it removed and with it counted.
- **This task's attributions may class unknown bytes** (R-0109's applied to inferred code only), so
  undecoded routines read from the listing are attributed like decoded ones.
- **Small arms by guard, not one by one**: `arms.py` groups the 286 by the branch that never goes to
  them and the instruction that sets its flags; each group gets one verdict, and blocks with a named
  feature or an argument get their own. No coverage was captured to chase them (the task's scope).
- **Data-bank entries argued from the call sites' index forms** (`asset_calls.py`,
  `sound_uploads.py`) and the value ranges the records give the variables; where only the ranges
  carry the argument, R-0112 says so.
- **Follow-up in OPEN-BRANCHES' record**, not a new task: the arms with a named feature are
  OPEN-BRANCHES' kind of work.
- **No tooling change**: every script is evidence; `src/`, `tools/` and `tests/` untouched (`dis.py`
  imports `tools/unirally_lab/coverage/opcodes.py` read-only).

## Evidence

Main `local/evidence/census-remainder/` (28 MB after `compact_evidence.py`): `access.sh`,
`access-ids.txt`, `access.out`, `access-reused.sh`, `access-reused.txt`, `access-reused.out`,
`access/*.json` (the 565 reduced records: `NNN` the 533 runs, `rNN` the 32 reused) and
`access/*.log` (the manifests are `coverage-census/manifests.tar.zst`, extracted for the run and
deleted after), `pairs.py`/`pairs.json`,
`codebank_reads.py`/`.json`, `census.py`/`census.json`, `make_attrib.py`/`attrib.json`,
`blocks.py`/`blocks.json`, `inrefs.py`/`inrefs.json`, `refs.py`, `show.py`, `dis.py`,
`table_words.py`/`.json`, `arms.py`/`arms.json`/`arms.md`, `verdicts.json`, `track_streams.py`/`.json`,
`sample_chain.py`/`.json`, `sound_uploads.py`/`.json`, `asset_calls.py`/`.json`,
`data_reads.py`/`.json`, `report.py`/`report.md`, `regen.sh`.

## Handoff

- Branch `task/census-remainder` on claim `dfc5fb67` (main `2a38ec3e`). Commit: the one that adds
  R-0112 (`docs/research/R-0112-census-remainder.md`), this record's sections, the paragraph added
  to `tasks/OPEN-BRANCHES.md`, and `docs/map/static/` regenerated with the eight inputs (labels from
  the new citations; the map's classes are unchanged). Tier 3: records, evidence and a regenerated
  map; no `src/`, `tools/` or `tests/` change.
- Commands (worktree root, `E=local/evidence/census-remainder`): `local/locks/heavy-run.sh
  CENSUS-REMAINDER $E/access.sh 10` (21:35:00-00:17:38 UTC, 2 h 43 min, 533 passed);
  `heavy-run.sh CENSUS-REMAINDER $E/access-reused.sh 10` (00:19:31-00:26:02 UTC, 32 passed); then
  the scripts in R-0112's Reproduction (`pairs.py`, `codebank_reads.py`, `make_attrib.py`,
  `census.py`, `blocks.py`, `inrefs.py`, `arms.py`, `table_words.py`, `track_streams.py`,
  `sample_chain.py`, `sound_uploads.py`, `asset_calls.py`, `data_reads.py`, `report.py`);
  `bash $E/regen.sh` after the last record edit: native-symbols and static-map `status=passed`, the
  tooling tests `OK`; `coverage native-symbols --check` passed (0 cited addresses without a record).
  `compact_evidence.py --skip-recent 0` on the folder: 28 MB.
- Not run: `ctest`, lints, the gates (records only).
- Not covered (R-0112 Limits): "unreferenced" assumes computed transfers take ROM constants; the
  278 small arms left are grouped, 187 of them without a feature named; the data-bank index argument
  rests on the records' value ranges; bank `$A2` and the poses past 2,795; the printer's codes `$FA`,
  `$F5`, `$F4` (no string scan); the 2,361 bytes of named open code (OPEN-BRANCHES).
- Time, from the clock: worker start 21:34 UTC 8 November, handoff 00:32 UTC 9 November; machine
  time: access captures 2 h 43 min and 7 min, during which the listing work was done.
- Next: [OPEN-BRANCHES](OPEN-BRANCHES.md), tier 1, with the arms R-0112 adds to it.

## Closeout (coordinator)

- Review: tier 3, no independent review (D-0008). The coordinator checked the class table's sums
  (42,656 in both columns; bank `$81` 8,628), the headline (4,276 = 2,361 + 6 + 1,875 + 34; 3.3% of
  131,072, 3.8% of 113,496; without "unreferenced" 7,691 of 116,911), two arguments against the ROM
  (`$81:C68F` is `BRA +3` over `$81:C691 JSR $CB24`, the only operand naming `$CB24` in bank `$81`;
  `$83:8AFA-8B1B` writes `$12` to `$77:0000` and `$3456` to `$77:1FFF`, compares `$77:0000` with
  `$34` and takes `JML $8094EB` only when they differ; the header's `$80:FFD8` is 3) and the diff's
  scope (no `src/`, `tools/` or `tests/` file).
- Gates (D-0012) on `d4f94047`: `only records changed: no local gates (CI's docs-only fast path covers them)`.
- Evidence: main `local/evidence/census-remainder/`, 28 MB after `compact_evidence.py`. Worktree
  and branch removed after the merge.
