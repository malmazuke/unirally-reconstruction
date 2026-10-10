# EVIDENCE-RETENTION - delete historical captures, keep their recipes

## Assignment

- Status: **done, in review as records** (tier 3) 11 October 2026, on the user's instruction ("Delete them, but keep the
  manifests"). Claimed on main `cd36d9b` by the session that ran DATA-COVERAGE.
- Worker: Claude Code desktop app, Claude Opus 5.5 (`claude-opus-5-5`); coordinator, primary and
  integrator. Branch `task/evidence-retention` in `.worktrees/evidence-retention`.
- Review tier: **3** (records and local housekeeping; no source, tool or gate change).
- Task provider: Anthropic.
- Parallel work: the TWO-HUMAN-STUNTS session asked for its folders, the recently merged tasks'
  evidence and the shared plumbing to be left alone. It held new gate runs and captures until
  the staging was deleted.

## Why now

The shared disk was down to 5-10 GB free. `local/evidence` held 284 GB, and 163 GB of it is read
by no current gate. The user decided that historical captures go and their recipes stay, which
[D-0011](../docs/decisions/D-0011-evidence-retention.md) records. The user also asked for a
follow-up that would retire most of the remaining original captures:
[GATES-AGAINST-MAIN](GATES-AGAINST-MAIN.md), queued here.

## Method

`local/evidence/evidence-retention/inventory.py` classes each of the 161 folders.

- **A folder is kept whole when:**
  - a current gate or tracked manifest reads it. That is anything named, directly or through
    the scripts they call, by the latest gate scripts (CREDITS-NAME, TWO-HUMAN-STUNTS and its
    audio pass, WIPE-RAM's sweeps) or by any manifest under `tests/manifests`;
  - it is protected:
    - TWO-HUMAN-STUNTS and what its checks name: `stunt-*`, `classic-stunt-names*`,
      `two-player-vs`;
    - SAVE-FILES, MODE-AUDIO, WIPE-RAM and CREDITS-NAME, with their review folders;
    - ACCESS-ROUND-READS, DATA-COVERAGE, COVERAGE-GAPS and the earlier disk cleanup;
  - something in it changed in the last 48 hours;
  - no recipe is found in it.
- **In every other folder,** files of 1 MiB or more are deleted. The exceptions:
  - recipes: manifests, scripts, `.inputs`, `.case.json`;
  - digests, records, packs, `base-*` builds and emulator builds (`.dylib`, `.so`);
  - the static map's four raw coverage files;
  - the `core_check` captures named by `tools/locks/verified-cores.json` (`rolling-contact`'s
    three seeds, both `w-rev-buttons` copies).
- **Deletion is staged.** `stage.py` moves the files to `local/evidence-staging/` (a rename on the
  same disk). Then the tools that could read them run, and only then is the staging deleted.

## Results

| Folders | Size | Outcome |
| ---: | ---: | --- |
| 75 | 134.6 GB | large captures deleted: 2,594 files, 131.1 GB |
| 26 | 124.2 GB | kept: read by a current gate or tracked manifest |
| 20 | 14.2 GB | kept: protected |
| 32 | 14.2 GB | kept: no recipe found, listed below for the user |
| 3 | 1.5 GB | kept: changed in the last 48 hours |
| 5 | 0.1 GB | nothing large |

- **Largest deletions:**

| Folder | Deleted |
| --- | ---: |
| `hunter-effects` | 25.1 GB |
| `audio-title-menu` | 24.6 GB (its observation core stays) |
| `m4-16-review` | 11.5 GB |
| `zoom-zoo-opposing-review` | 10.3 GB |
| `race-finish-breadth` | 9.0 GB |
| `dragster-clock-limit` | 8.7 GB |
| `tile-pairs` | 6.7 GB |

  The per-file list is in `local/evidence/evidence-retention/inventory.json`.
- **No recipe found, kept:** `ci-fast-path`, `ci-fast-path-review`, `dragster-controls-review`,
  `dragster-ordinary-gates`, `dragster-window-review`, `jev-judgment-harness-review`, the M2-01,
  M2-02, M3-01, M3-02, M3-02A, M3-03, M4-00, M4-01, M4-03, M4-12 to M4-15 review folders,
  `repo-local-state-cleanup` and `static-code-map`. These are mostly early reports, logs and
  native outputs with no replay manifest beside them.

## Checks

- **Cross-references.** A scan of every script and manifest in the kept folders for paths into
  the deleted ones found only historical gate scripts no current gate runs, plus provenance text.
  It also found three references that are now protected: `audio-title-menu`'s observation core,
  the core-check captures, and the stunt folders.
- **The other session's check.** It confirmed that none of the evidence TWO-HUMAN-STUNTS' gates
  read was staged.
- **After staging:**
  - The static map, regenerated from its five raw captures, is byte-identical to the tracked
    `code-banks.map.json`, `code-banks.md` and `labels.json`.
  - The five `core_check` captures are intact: nothing staged, `reference.json` present.
    `core_check` itself refuses to run here by design, because this host's library made them.
  - `audio-title-menu/observation-core-v5.dylib` is in place (2,991,960 bytes).
- **Deleted:** the staging folder, at 122 GiB. Free space went from 20 GiB to 108 GiB.
  - The 34 GiB gap is consistent with deleted files that were APFS clones of kept ones (the
    9 October cleanup cloned identical captures), but that is not verified.
  - `local/evidence` is now 149 GB.

## Handoff

- Branch `task/evidence-retention`: D-0011, AGENTS.md's retention rule, this record and
  GATES-AGAINST-MAIN.
- The inventory and its scripts are in main `local/evidence/evidence-retention/`: `inventory.py`,
  `inventory.json` (every deleted file) and `stage.py`.
- Next: [GATES-AGAINST-MAIN](GATES-AGAINST-MAIN.md), tier 1, after the TWO-HUMAN-STUNTS lane.
  The 32 folders without a recipe (14.2 GB) wait for the user.
