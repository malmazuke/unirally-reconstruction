# D-0011 - Keep recipes, not historical captures

- Status: adopted on the user's instruction, 11 October 2026 (EVIDENCE-RETENTION session, Claude
  Opus 5.5)
- Related: [EVIDENCE-RETENTION](../../tasks/EVIDENCE-RETENTION.md),
  [GATES-AGAINST-MAIN](../../tasks/GATES-AGAINST-MAIN.md), AGENTS.md (local state),
  [REPO-LOCAL-STATE-CLEANUP](../../tasks/REPO-LOCAL-STATE-CLEANUP.md)

## Problem

- **The disk is nearly full.** `local/evidence` held 284 GB on 11 October 2026, and the shared
  disk had 5-10 GB free. That is too little for new captures or gate runs.
- **The old rule keeps everything.** AGENTS.md said never to delete a capture a record cites.
  Every accepted record cites its captures, so nothing could ever go.
- **Most of it no current check reads.** 135 of the 161 folders (163 GB) are read by no current
  gate script and no tracked manifest. They hold the captures behind accepted, merged tasks:
  reviewers' withheld runs, explorations, and early audio and HUNTER captures.
- **Captures are reproducible.** Each one is the output of a recipe: a replay manifest or capture
  script with its settings, run on the pinned emulator against the identity-checked ROM. The
  emulator is deterministic, and recorded digests prove a rerun identical.
- **The user's view:** once a part of the game is native and accepted, its captures are no longer
  needed day to day.

## Decision

1. **What must stay.** A capture is retained only while one of these is true:
   - a current gate script or tracked manifest reads it;
   - a task in flight uses it;
   - it belongs to a task merged in the last few days (the next tasks still build on it);
   - it is a reference input of a tool:
     - the static code map's raw coverage files;
     - the `core_check` captures that `tools/locks/verified-cores.json` names;
     - the emulator builds (`.dylib`) that capture tools load.
2. **What goes.** Any other capture may be deleted, provided its recipe stays. Recipes are kept in
   place:
   - replay manifests and capture scripts;
   - capture settings;
   - digest files and reports;
   - in practice, every file under 1 MiB.
   A record that cites a deleted capture still holds. Anyone who needs the capture regenerates it
   from the recipe and checks it against the recorded digests.
3. **Unclear recipes.** A folder with no recognisable recipe is kept and listed for the user.
4. **Delete safely.** Deletion goes through a staging folder: move first, check that the tools
   which could read the files still run, then delete. The inventory of deleted files is kept.
5. **Next step.** [GATES-AGAINST-MAIN](../../tasks/GATES-AGAINST-MAIN.md) will propose comparing
   each change against the accepted native code instead of stored captures of the original. If
   adopted, it would shrink the retained set further.

## Consequences

- About 131 GB is freed now, and the rule no longer grows the disk without bound.
- **Regeneration has a cost.** A task reopening old ground pays CPU time to regenerate captures
  (up to a few hours for the large sets) and must check them against the recorded digests.
- A record's citation can now point at a recipe whose capture is absent. The record's claim does
  not change.
