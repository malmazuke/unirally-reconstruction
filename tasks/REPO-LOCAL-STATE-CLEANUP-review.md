# REPO-LOCAL-STATE-CLEANUP - independent review

## Review of `da3b51263338621941b7924aa76a5fa050f2ab8c` (2026-09-18 UTC)

Reviewing model: Claude Opus 5

Fresh session, no inherited conversation. Isolated checkout
`.worktrees/cleanup-review`, branch `review/repo-local-state-cleanup`, at the
candidate `da3b512` with a clean tree throughout. The main checkout's ignored
`artifacts/local-state-cleanup/` and `local/evidence/` were read by absolute
path only; nothing there was modified. Base for the diff is `main` at
`a710388`; the candidate is documentation plus four cherry-picked review
commits, no source change.

Everything below was rerun here. Where a claim is a point-in-time measurement
that no longer exists (the 146 GB "before" figure, the `diff -rq` on captures
that were then deleted), that is stated rather than taken on the record's word.

### 1. Branch audit - reproduced

The audit records 106 local branches besides `main`; 105 were deleted and the
candidate's own `task/repo-local-state-cleanup` was not.

Eight new remote branches plus one that was already there. All nine
`pushed-to-origin` entries resolve on `origin` at exactly the tip recorded in
`branch-audit-2026-09-18.txt`:

```sh
git ls-remote origin > /tmp/lsremote.txt
grep '^pushed-to-origin' artifacts/local-state-cleanup/branches-deleted-2026-09-18.log |
  while read -r cls br tip; do
    rem=$(grep -E "refs/heads/${br}\$" /tmp/lsremote.txt | awk '{print $1}')
    [ "${rem:0:10}" = "$tip" ] && echo "OK $br" || echo "MISMATCH $br $rem $tip"
  done
```

Result: nine `OK`, no mismatch, for `codex/m4-16-7c3e3b6-case-review`,
`review/classic-presentation-unification`, `review/dragster-window-pause`,
`review/m4-16-2a0ca8c-live-controls`, `review/m4-16-9a3c504-rereview`,
`review/m4-16-a16b88c-confirm`, `review/m4-16-aeb62e0-opponent-reward`,
`review/m4-16-c179765-result-restart` and `review/m4-16-df34615-ai-trick`.

The five spot-checks the review asked for were widened to all 105 recorded
tips, which costs nothing and is a stronger statement:

```sh
grep -v '^#' artifacts/local-state-cleanup/branches-deleted-2026-09-18.log |
  awk '{print $(NF-1), $NF}' |
  while read -r br tip; do
    git cat-file -e "$tip^{commit}" || { echo "MISSING $br"; continue; }
    echo "$br +$(git cherry main "$tip" | grep -c '^+')"
  done
```

Result: every one of the 105 tips still resolves as a commit object, so no
history was lost by the deletions. All 96 non-pushed branches report `+0`, that
is every commit is represented on `main` by patch-id. The nine pushed branches
report `+1` to `+4` as expected, since their commits live on `origin` rather
than on `main`. The five named spot-checks were `audit/M3-04-opponent-first`
(`b1834860d0`), `review/M3-01-native-finish` (`be9b78b367`),
`review/M3-02A-classic-content-pack-fix` (`c86874d50f`),
`review/M4-02-second-track` (`5aceea4dad`) and
`task/M3-02A-classic-content-pack` (`ef2cb101f7`), all `+0`.

Class counts in the deletion log are 55 `merged`, 41
`represented-by-patch-id` and 9 `pushed-to-origin`, matching the record's
table. Four `merged` entries are annotated `(forced; upstream lagged)` and each
is an ancestor of `main`, as the record says.

### 2. Review reports on main - reproduced

Blob identity, not a textual diff:

```sh
R1=$(git ls-remote origin refs/heads/review/classic-presentation-unification | cut -f1)
R2=$(git ls-remote origin refs/heads/review/dragster-window-pause | cut -f1)
git fetch -q origin "$R1"; git fetch -q origin "$R2"
git rev-parse "${R1}:tasks/CLASSIC-PRESENTATION-UNIFICATION-review.md" \
              "HEAD:tasks/CLASSIC-PRESENTATION-UNIFICATION-review.md" \
              "${R2}:tasks/DRAGSTER-WINDOW-PAUSE-review.md" \
              "HEAD:tasks/DRAGSTER-WINDOW-PAUSE-review.md"
```

`tasks/CLASSIC-PRESENTATION-UNIFICATION-review.md` is
`175ee9d8600727b846b0a2f5714e4e63cbd292b7` on both the candidate and
`origin/review/classic-presentation-unification` at `a7f26c4`.
`tasks/DRAGSTER-WINDOW-PAUSE-review.md` is
`95ea03a7f79937fb9ea005257357165d6ed6e010` on both the candidate and
`origin/review/dragster-window-pause` at `067dbfe`. Byte-identical, no
reflowing or editing on the way onto the branch. `git cherry HEAD a7f26c48b7`
and `git cherry HEAD 067dbfeefa` both report `+0`, so the cherry-picks carry
those branches' whole content.

### 3. Evidence intact - reproduced, with one repointing gap (see should-fix 1)

DRAGSTER originals:

```sh
find local/evidence/dragster-ordinary-controls/dragster-ordinary-controls/originals -type f | wc -l
```

52 files, the seven `*-a`/`*-b` pairs plus their logs. Byte total
6,158,445,682 (5.73 GiB); the record's "5.9 GB" is slightly off either
convention (see advisory 1).

Closeouts: `ls -1 artifacts/*/closeout.json | wc -l` gives 16 in the main
checkout, including the seven the `moves` log records as moved out of
worktrees (unification, dragster-window, dragster-ordinary, m4-16,
dragster-palette-cycle, dragster-clock-limit, window-pause).

No gate script points into another worktree's artifacts:

```sh
grep -rInI "\.worktrees/[^\"' ]*/artifacts" --include='*.sh' artifacts local/evidence
```

No match. The 44 shell scripts under those trees that still mention
`.worktrees/` do so only on a `cd` line, which is what the record and
`docs/BUILD_AND_VALIDATION.md` say is deliberately kept as a note of the
checkout the script ran in. The remaining `.worktrees/<x>/artifacts` strings
are inside historical JSON run reports, which are records of past runs and not
inputs. `local/emulators/bsnes/lab-core.json` now names
`local/emulators/bsnes/bsnes/out/bsnes_libretro.dylib`, which exists and hashes
to `e59bf88d4fc922c9fe3b5438e65ff3a6909d24e1628f0f87141c8de17699a91b`, equal to
the `library_sha256` in the file and to the hash the record verified before the
change.

`gates-e5d8f4b/summary.txt` and the four JSON reports:

- `HEAD e5d8f4b972f920b6004e74b8533ccc1b46013084 dirty=0` before and
  `HEAD after` the same, and `e5d8f4b` is exactly `da3b512^`.
- All four reports carry `"status": "passed"` and
  `source_commit e5d8f4b...`, with
  `source_diff_sha256 e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855`,
  the SHA-256 of the empty input, consistent with the clean tree.
- `len(restore_frames)` is 379, 327, 757 and 801 for dragster-primary,
  dragster-reversal, m4-16-primary and m4-16-idle, equal to the `restores`
  lines in `summary.txt` and to the counts in the task record.

Independent rerun, from this checkout, of the DRAGSTER reversal compare against
the moved evidence. Toolchain and private inputs were copied from the main
checkout into `local/` first:

```sh
python3 tools/project.py bootstrap                      # status=passed
python3 tools/project.py build --preset app-debug --timeout 1500   # status=passed
O="/Users/markfeaver/Projects/Unirally Decompilation/local/evidence/dragster-ordinary-controls/dragster-ordinary-controls/originals"
python3 -m tools.unirally_lab.native.dragster_playable compare \
  --reference "$O/reversal-a" --repeat "$O/reversal-b" \
  --contract tests/manifests/native/dragster-ordinary-reversal.freeze.json \
  --binary build/app-debug/src/core/zoom_zoo_runner \
  --pack local/classic-crawler-two-tracks-v8.pack \
  --out "$SCRATCH/review-dragster-reversal.json"   # written outside the checkout
```

`HEAD da3b512... dirty=0` before and after; 2026-09-18T22:06:37Z to
22:08:24Z. Result `"status": "passed"`, `"acceptance": true`, `restores 327`,
`frames [1328, 5000]`, `state_bytes 742`,
`rows_sha256 f142311ef5a0fff40edd77427ae0f4f0acaf35dcf8122b5b2ab302895d6ab17e`,
`contract_sha256 1a90ebac4d4ad85ba89315d089e92f6cd2990aa908c9b98a8006d12eae339dc5`,
`events` identical to the recorded run. The row hash and contract hash match
the candidate's `dragster-reversal.json` exactly; `pack_sha256` matches too
(`3d1e642b...`, the same `classic-crawler-two-tracks-v8.pack`). Only
`binary_sha256` and `source_commit` differ, as they must for a separate build
of a different commit. Judged by the `status=` line, not the shell exit code.

The kept evidence the record lists is present at the stated sizes:
`m4-16-playable-zoom-zoo/m4-16` 47G, `m4-16-rider-art/m4-16-idle/captures`
9.9G, `m4-16-review` 11G, `dragster-controls-review` 3.0G,
`dragster-window-pause/window-pause` 2.2G, and
`dragster-clock-limit/{idle-a,idle-b}` both exist.

### 4. Deletions - reproduced

`deletions-2026-09-18.log` lists exactly the classes the record claims and
nothing else: three `duplicate (diff -rq identical to canonical)` directories
under `dragster-clock-limit/.../regressions` (5874 + 851 + 851 MB = 7.6 GB),
24 `uncited exploration capture` files, which are the eight `*-explore`
`.wram`/`.sram`/`.json` triples under `m4-15-review` (11.0 GB), five `build`
directories, 74 `__pycache__`, 15 empty directories and 144 hex run-report
scratch directories at 772 KB. The `diff -rq` identity of the three duplicates
cannot be re-verified here because they are gone; what can be and was checked
is that their canonical counterparts survive, namely the 52-file DRAGSTER
originals and `local/evidence/m4-16-playable-zoom-zoo/m4-16/boundary-a` and
`boundary-b`.

No deleted path is cited by a tracked record:

```sh
grep -rn -- "-explore" tasks docs
grep -rn "regressions/dragster-originals" tasks docs
grep -rn "regressions/boundary" tasks docs
for n in delayed early-jump early-turns jumps lap-two-jump mid-neutral one-step two-step; do
  git grep -n "${n}-explore"; done
```

The `regressions/...` greps return nothing. The `-explore` grep returns three
hits, none of which is a deleted path: the cleanup record's own deletion table,
the words "re-explored" in `tasks/DRAGSTER-ORDINARY-CONTROLS.md`, and
`restart-explore` in `tasks/M4-16-review.md`, which still exists at
`local/evidence/m4-16-playable-zoom-zoo/m4-16/restart-explore`. None of the
eight deleted capture names appears anywhere in the tracked tree. The
`m4-15-review` frozen `*-a`/`*-b` pairs the record says it kept are present.
`tasks/M4-14.md` cites `.../m4-13/explore-right.json`, which survives at
`local/evidence/m4-13-player-landing/m4-13/explore-right.json`.

The other logs check out too: `moves-2026-09-18.log` records 75 worktree
`artifacts/` moves totalling 118,155 MB (115.4 GiB), matching the record's
75 worktrees and 115 GB, and `worktrees-removed-2026-09-18.log` records 93
registered worktrees plus the unregistered `m2-01-sampling-review` clone, 94
removals in all. `du -sh` in the main checkout now reports 117G total, 115G
`local/evidence`, 667M `artifacts`, 35M `.git`, which reproduces every "after"
cell of the record's inventory table. The "before" column cannot be
reproduced after the fact and is taken as a recorded measurement.

### 5. Rule - accurate in substance, ASCII-hyphen rule respected

The AGENTS.md bullet and the new `docs/BUILD_AND_VALIDATION.md` "Local evidence
layout" section describe what I observed. Every path either section names as an
example exists:
`local/evidence/dragster-ordinary-controls/dragster-ordinary-controls/originals/primary-a`,
`local/evidence/m4-16-playable-zoom-zoo/m4-16/boundary-a`,
`artifacts/m4-16-integration/closeout.json`,
`artifacts/window-pause-integration/closeout.json`,
`artifacts/dragster-ordinary-integration/gates-b452170/gates-frozen.sh`,
`local/classic-crawler-two-tracks-v8.pack` and `local/native/dragster-idle`.
The worked command in that section is the shape I ran successfully for the
reversal case, and its caveat about `git rev-parse --show-toplevel` resolving
to the worktree rather than the main checkout is correctly stated. The
statement that the scripts' `cd .worktrees/<name>` lines name checkouts that no
longer exist, so one must be recreated with `git worktree add` before rerunning
a script verbatim, is accurate.

ASCII hyphens:

```sh
git diff main...HEAD -- . ':(exclude)tasks/CLASSIC-PRESENTATION-UNIFICATION-review.md' \
  ':(exclude)tasks/DRAGSTER-WINDOW-PAUSE-review.md' | grep '^+' | grep -P '[\x{2013}\x{2014}]'
```

No match, and no match in the two cherry-picked reports either. The rule is
respected across the whole diff.

Two wording problems are in should-fix 2 below.

### 6. Records - mostly match the logs; three claims do not

`docs/STATE.md`, `tasks/NEXT_SESSION.md` and `tasks/README.md` agree with the
logs on the moved 115 GB, the seven moved closeouts, the 105 deleted branches,
the 7.6 GB of duplicates, the 11 GB of exploration captures, the 146 GB to
117 GB footprint and the kept gate inputs, all of which I checked above.
`NEXT_SESSION.md`'s four named gate-input locations all exist. The three
claims that do not match are should-fix 3, 4 and 5.

## Findings

### Blocking

None. No evidence was lost, no history is unrecoverable, no gate was weakened
or masked, and the one differential gate I reran reproduces the recorded run
byte for byte from the new evidence location.

### Should-fix

1. **A recorded gate script still points at the deleted duplicates, and was not
   repointed.**
   `local/evidence/dragster-clock-limit/dragster-clock-limit/frozen-gates.sh`
   sets `O="artifacts/dragster-clock-limit/regressions/dragster-originals"` and
   `R="artifacts/dragster-clock-limit/regressions"`, and uses `$O/$c-a`,
   `$O/$c-b`, `$R/boundary-a` and `$R/boundary-b` for 16 comparisons. Those are
   exactly the three directories `deletions-2026-09-18.log` removed as verified
   duplicates. The script is not in `repointed-scripts-2026-09-18.log`, because
   the audit looked for absolute `.worktrees/<wt>/artifacts/...` paths and these
   are relative, so nothing rewrote them and nothing records the substitution.
   No unique evidence was lost, since the canonical originals and boundary
   captures survive, but `docs/BUILD_AND_VALIDATION.md`'s claim that recorded
   gate scripts have "their input paths rewritten to `local/evidence/...`" and
   the record's "12 recorded gate scripts had their absolute input paths
   rewritten" are not true of this one. Rewrite `O` and `R` in that ignored
   script to
   `/Users/markfeaver/Projects/Unirally Decompilation/local/evidence/dragster-ordinary-controls/dragster-ordinary-controls/originals`
   and `.../local/evidence/m4-16-playable-zoom-zoo/m4-16`, and add it to the
   repointed log; or state in the record that this script's inputs were deleted
   as duplicates and name the canonical replacements.

2. **The AGENTS.md rule is absolute where the repository is not.** "nothing
   under `.worktrees/` is evidence, and gate scripts and records never point
   into another worktree" reads as a statement of the current state as well as
   a rule, and neither half holds literally: 44 recorded gate scripts keep a
   `cd "/Users/.../.worktrees/<name>"` line on purpose, and tracked records
   still cite `.worktrees/...` paths, including
   `tasks/DRAGSTER-WINDOW-PAUSE-review.md` lines 35 and 276, a file this very
   commit adds to `main`. `docs/BUILD_AND_VALIDATION.md` documents the `cd`
   exception, but AGENTS.md does not, so a reader of the rule alone would call
   the retained lines a violation. Make the AGENTS.md bullet forward-looking
   about inputs specifically, for example "gate scripts read their inputs from
   `local/evidence/`, never from another worktree; a historical `cd` line
   recording the checkout a script ran in is kept", and note in the record that
   historical `.worktrees/...` citations in older records translate by the
   mapping in `docs/BUILD_AND_VALIDATION.md`.

3. **`docs/STATE.md` says one worktree too many was removed.** It says "All 94
   registered worktrees and one unregistered clone were removed".
   `worktrees-removed-2026-09-18.log` has 93 registered entries plus the clone,
   and the task record itself says 93 plus the clone; the 94th registered
   worktree is the task's own, which still exists, as does this review's. Change
   to 93.

4. **`docs/STATE.md` undercounts the pushed branches.** It says "eight with
   commits not represented on `main`, all review reports, were pushed to
   `origin` first". Nine branches have a non-zero unrepresented count in
   `branch-audit-2026-09-18.txt`, nine are logged `pushed-to-origin`, and I
   confirmed nine on `origin`. Eight is the number of *new* remote branches, as
   the task record correctly says, because
   `review/m4-16-c179765-result-restart` was already there. Say nine pushed, of
   which eight were new.

5. **Three records assert a review that had not happened at the candidate.**
   `tasks/README.md` registers REPO-LOCAL-STATE-CLEANUP as "reviewed and
   integrated", `docs/STATE.md` says "reviewed and integrated" and the task
   record's own Assignment says "Status: reviewed and integrated", while the
   same record's Review section says "Pending at the candidate commit". At the
   comparable point in the previous task, `git show f8645d7:tasks/README.md`
   still read "next; planned and approved by the user", so the house practice is
   to write the status at integration, not in the candidate.
   `docs/AGENT_WORKFLOW.md` says never to preclaim a future pass. The
   forward-looking closeout pointer to
   `artifacts/repo-local-state-cleanup-integration/closeout.json`, which does
   not exist yet, is fine under the consolidated-closeout convention; the
   completed-review wording is not. Reword to "in review at `da3b512`" in the
   candidate and let the integration commit record this report's result. Since
   this report approves, the integration commit can simply make the existing
   wording true, which is the cheapest resolution.

6. **Two counts in the record disagree with their own logs.** The record's
   deletion table says "159 run-report scratch directories (772 KB)" where
   `deletions-2026-09-18.log` says "144 dirs, 772 KB total"; 159 looks like 144
   plus the 15 empty directories the same row already counts separately. The
   record says "12 recorded gate scripts had their absolute input paths
   rewritten" where `repointed-scripts-2026-09-18.log` lists 11. Correct both to
   the logged numbers, or say what the twelfth was.

### Advisory

1. The record gives the DRAGSTER originals as 5.9 GB. The directory is
   6,158,445,682 bytes, that is 5.73 GiB or 6.16 GB, so neither reading gives
   5.9. Elsewhere the record's GB figures are consistently GiB (115 GB against
   115.4 GiB of moves, 117 GB against `du -sh` 117G), so this one is simply
   stale. Not worth a commit on its own.

2. The record says "105 local branches besides `main`" were audited and the
   class table sums to 105, but `branch-audit-2026-09-18.txt` has 106 entries:
   the 105 deleted plus the task's own `task/repo-local-state-cleanup`. The
   sentence is about what was acted on rather than what was audited, which is a
   fair reading, but "105 audited, 105 deleted, plus this task's own branch"
   would be unambiguous.

3. `repointed-scripts-2026-09-18.log` lists eight of its 11 scripts at
   `local/evidence/<wt>/<task>-integration/...` paths that no longer exist,
   because the closeout directories moved to `artifacts/<task>-integration/`
   two minutes later. The files are all present at their new paths; only the
   log's paths are stale. A one-line note at the top of that log would save the
   next reader the lookup.

4. Two 4 KB scratch directories survived the sweep, presumably because the
   pattern matched a bare 32-hex name only:
   `local/evidence/m4-16-review/native-228fc76267a84793a1de20a6a0c1bb67` and
   `local/evidence/m2-01/replay/f0e03463ae1f4bfbb53806699ee07835`. Immaterial
   to the footprint; mentioned only so a later pass does not read them as
   evidence.

## Verdict

APPROVE - the branch audit, the review-report identity, the evidence layout and
the DRAGSTER reversal differential gate all reproduce independently; apply the
six should-fix items before or as part of integration.
