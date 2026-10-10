# DATA-COVERAGE - which data the original reads

## Assignment

- Status: **in review** (tier 2). Claimed 9 October 2026 21:26 UTC on main `33f3645`, by the
  session the user asked to run beside the WIPE-RAM session (R-0089's queue item 9). Weekly usage
  at claim 15%. Rebased on main `6b6790a` (WIPE-RAM) after the captures.
- Worker: Claude Code desktop app, Claude Opus 5.5 (`claude-opus-5-5`); coordinator, primary and
  integrator. Branch `task/data-coverage` in `.worktrees/data-coverage`.
- Milestone: M4 (original game coverage).
- Review tier: **2**, escalated from 3 at the first tooling change. The measure needed two tool
  changes: `access capture --resolve-rmw` and `coverage merge`, plus `coverage map`'s 2 MiB limit
  for merged maps. No `src/core`, `src/app` or pack change.
- Task provider: Anthropic.
- Dependencies: [COVERAGE-GAPS](COVERAGE-GAPS.md) (R-0089's corpus) and the static code map
  (R-0045).
- Parallel work: this task ran beside the lane that held WIPE-RAM and then CREDITS-NAME. The
  two sessions agreed these rules on 10 October:
  - this task changes no `src/core`, `src/app` or pack profile;
  - heavy runs (captures, sweeps, gates, the synthetic suite) take `local/locks/heavy-run.sh`,
    a new shared lock in main's `local/` (it waits while another session holds it and clears a
    lock whose holder has died);
  - the static map is regenerated only on top of a main that includes WIPE-RAM;
  - each session edits only its own rows of the shared records;
  - research record numbers are agreed in advance (R-0093 here, R-0094 CREDITS-NAME).
- One pause: the batch was stopped once, at the other session's request, for its 15-minute sweep
  rerun. The in-flight runs were discarded and the batch resumed behind it on the lock.

## Why now

R-0089 measured the code banks `$80-$83` but not the 60 data banks or the sound program. Data
the original reads and native does not extract is missing content. Data nothing reads is either
unused or reached by paths no capture has taken; R-0089 named the 45 entries `$82:B332` `$C2-$EE`.

## Outcome and boundaries

- Over R-0089's corpus, which bytes of the ROM outside the code banks the original reads, by CPU
  instruction, DMA and HDMA, attributed to banks, the directories and the pack's entries.
- The flagged directory entries: whether any captured path loads them, and what they hold.
- The static code map regenerated from this corpus, with the shrink in its unknown share.

## Results

[R-0093](../docs/research/R-0093-data-coverage.md) has the findings and reproduction.

- **Coverage.** 236 runs, 1,666,993 frames, 0 unresolved accesses. The corpus reads 1,562,774 of
  the ROM's 2,097,152 bytes. In the data banks it reads 1,551,791 of 1,966,080. The unread
  non-padding bytes (322,406) are mostly packed content the corpus never shows: rider tiles and
  poses, and the locked tours' tracks.
- **The flagged entries are the 45 track streams.** They load through `$82:B2DD` and `$81:B8E2`.
  `$81:BB7A`, R-0089's "never run" unpacker, is the VRAM copier's compressed branch. None of the
  copier's 69 call sites reaches it (67 constants up to `$C0`; 2 computed within `$70-$7E` and
  `$82-$90`). It looks unreachable, not a missing feature. The corpus loads 25 of the 45 tracks.
- **A second directory.** The track scenery directory at `$82:B7DD` has 41 entries, and each
  track lists its own scenery. The asset directory therefore has 239 entries, not 256.
- **What the original reads and native does not extract:**
  - **Demo sound set:** samples 02, 23 and 34, and a block at `$93:F712`, loaded only when an
    idle demo runs. Recorded for DEMO-AUDIO.
  - **The rest of the sound uploads:** 11,603 bytes; the driver itself is reimplemented by
    design (D-0009).
  - **188 bytes of small pieces.**
- **Unread and unpacked:**
  - **Credits picture:** entries `$BF-$C1`. Passed to CREDITS-NAME, whose PR #70 adds them.
  - **Bank `$A2`:** probably the unreached continuation of the collision-pose table; open.
  - **Assets without a constant call site:** `$47`, `$49`, `$4B`, `$4F`, `$51`, eight palettes
    `$B7-$BE`, and `$03`, `$04`, `$3D`.
- **Static map.** The unknown share goes from 40.4% to 24.0% (observed 41,778 to 86,837 bytes),
  below D-0008's trigger. The map now has 759 routines, and native cites 537 of them (main `ef8e440`).

## Attempts

| # | Hypothesis | Experiment | Result | Next |
| --- | --- | --- | --- | --- |
| 1 | The access record's ROM bitmap measures data reads | `access capture` of run 052 (470 frames) | 138,074 bytes read; 12,179 unresolved accesses, at `$82:80D1`/`$82:82F0` | Read the listing there |
| 2 | Those are a pointer advanced by RMW | `$82:8151`: `INC $63` on the `LDA [$63]` pointer | Confirmed; the derivation treats every RMW's result as unknown | Compute RMW results, opt-in |
| 3 | `--resolve-rmw` resolves them | Run 052 again | 0 unresolved, 149,963 bytes read; existing tests unchanged; four new tests | Capture the corpus |
| 4 | The static map can take the corpus as one more map | `coverage merge` and `coverage map` | Map 1,149,327 bytes, over M1-01's 1 MiB; half the corpus still gives 1,091,108 | Allow 2 MiB for merged maps only |
| 5 | The flagged entries are an unused graphics format | Static: the copier's 31 call sites; the track loader `$82:E140`; measured reads | They are the tracks, loaded through `$81:B8E2`; `$81:BB7A` is unreachable | Recorded |
| 6 | Track lists name asset-directory entries | `track_assets.py` | The indices are small (`$01-$28`): `$82:E17A` installs a second directory at `$82:B7DD` | Parse both directories |

## Evidence

Main `local/evidence/data-coverage/`:

- **Scripts:** `extract.py`, `captures.sh`, `followup.sh`, `track_assets.py`, `aggregate.py`.
- **Per-run reads:** `runs/*.json`, 236 runs.
- **Coverage:** `cov/*/coverage.json`, 236 runs; `corpus-coverage.list`; `corpus-coverage.json`.
- **Logs and outputs:** `logs/`, `captures.out`, `aggregate.json`, `aggregate.out`,
  `track_assets.json`.

Tracked:

- **Corpus map:** `docs/map/data-coverage-corpus.{map.json,md}`.
- **Static map:** the regenerated `docs/map/static/*`.

## Checks

- Access tests: 31 pass (four new). Coverage tests: 34 pass (two new merge tests). Both suites run
  by `python3 -m unittest tests.tooling.test_access tests.tooling.test_coverage`.
- `coverage static-map`: every check passes. 2,612 ranges tile, and 95,618 sites over five
  captures agree.
- Synthetic suite, lab-debug, on `1525263` under the lock: passed, 605 checks.

## Review - round 1 (approved)

- [Review](https://github.com/malmazuke/unirally-reconstruction/pull/71#pullrequestreview-5476987817)
  of `1525263` (Claude, tier 2): approve, no blocking finding.
- **What the reviewer checked:**
  - Two runs re-extracted byte for byte (014, an idle-demo run, and 053).
  - On run 003, every one of 7,290 sound-upload loads equals the ROM bytes at its resolved
    address.
  - Its own synthetic RMW chain: INC wrap, ROR carry, bank INC, DEC, a three-byte ASL/ROL chain,
    TSB/TRB, LSR.
  - The merge and both maps reproduced byte for byte.
  - The listing claims and the three samples' positions confirmed.
- **Findings and what was done:**
  1. **Call-site counts.** The corpus listing has 69 copier call sites (67 constants up to
     `$C0`; computed indices within `$70-$7E`/`$82-$90`). Fixed in R-0093 and here. The `$82:DC2E`
     path, which picks entries `$7E`/`$90` (`scenery.14`), is noted for CARTRIDGE-OPTION-BITS.
  2. **The `$93` table** lists the unpacked parts of three uploads, not four uploaded blocks;
     `$93:F712` is read in 10 runs. Reworded in R-0093.
  3. **Map size** is 1,149,327 bytes. Fixed.
  4. **Discarded resolution rounds** can leak ROM reads into the bitmap. Pre-existing; recorded
     as a limit in R-0093, and a follow-up task is offered.
  5. **Advisories:**
     - The five-capture recipe is now stated in R-0093 and the state paragraphs.
     - `merge_documents` counts first frames from each run's own start; new test.
     - A single-input merge keeps the 1 MiB limit.

## Handoff

- Branch `task/data-coverage` on main `6b6790a`.
- **Next:** hosted CI on the final head, then merge (PR #71).
- **Follow-up for other tasks:**
  - DEMO-AUDIO needs samples 02/23/34 and the `$93:F712` block (noted in its record).
  - Bank `$A2` and the unreferenced assets are open questions for RACE-LISTING-BRANCHES and
    CARTRIDGE-OPTION-BITS.
