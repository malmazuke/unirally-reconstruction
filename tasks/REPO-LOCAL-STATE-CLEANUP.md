# REPO-LOCAL-STATE-CLEANUP - branches, worktrees and ignored artifacts

## Assignment

- Status: reviewed and integrated; acceptance conditional on final-tip CI and
  remote verification. Closeout: ignored
  `artifacts/repo-local-state-cleanup-integration/closeout.json` in the main
  checkout; if absent, `git log --first-parent main -- tasks/REPO-LOCAL-STATE-CLEANUP.md`
  and `gh run list --workflow synthetic.yml --commit <commit>`
- Milestone: housekeeping, independent of M4
- Coordinator: Claude Fable 5.1 primary session (registered on 18 September 2026
  by the Claude Opus 5 session at the user's request; started the same day at
  21:37 UTC on the user's "Next task")
- Task provider: Anthropic (unchanged)
- Worker/session/runtime/model: Claude Fable 5.1, Claude Code desktop, one
  session; no child worker
- Provider quota (D-0004): at start 21:37 UTC five-hour 2%, weekly 38%, weekly
  Fable 24%; the user set the stop rule "continue up to 50% weekly or 50% Fable,
  whichever comes first"; no reset, purchase or provider change
- Reviewer: fresh Claude Opus 5 subagent in an isolated checkout at the exact
  candidate (see Review below)
- Base commit: `main` at `a710388` (the record's `b1dd0a3` was superseded by the
  window-pause integration before this task started)
- Branch and isolated worktree: `task/repo-local-state-cleanup`,
  `.worktrees/repo-local-state-cleanup`
- Owned paths: `AGENTS.md` (retention rule), `docs/BUILD_AND_VALIDATION.md`
  (evidence layout), `docs/STATE.md`, `tasks/NEXT_SESSION.md`, `tasks/README.md`,
  this record, the closeout pointers in two task records, and the two review
  reports `main` lacked (see Branches); ignored: `local/evidence/`,
  `artifacts/*-integration/`, `artifacts/local-state-cleanup/`

## Why

The user asked whether work can be called done while local state is floating
about. Measured on 18 September 2026 (first pass, Opus 5 session), it could not:
151 GB, 93 worktrees, 49 branches with commits not on `main`, and gates that
read original captures from inside other worktrees by absolute path, so a
directory that looked abandoned could be a live gate input.

## What is evidence and must not be deleted

- **Original captures** (`**/originals/**`, `baseline-*.wram`/`.sram`,
  `reference.json`, the reference/repeat pairs behind every frozen contract in
  `tests/manifests/native`). Reproducible only against the user's private ROM,
  at minutes to hours per capture.
- **Closeouts** (`artifacts/*-integration/closeout.json`) and the gate logs
  beside them: cited by task records as the acceptance evidence.
- **Anything a task, review or research record cites by path.**

Regenerable without judgement: `build/`, `__pycache__`, empty directories and
the 32-hex run-report scratch directories `tools/unirally_lab/report.py` makes.

## What was done (18 September 2026, 21:37-22:01 UTC)

Everything below is logged in the ignored `artifacts/local-state-cleanup/`:
`worktree-table-2026-09-18.txt`, `branch-audit-2026-09-18.txt`,
`branches-deleted-2026-09-18.log`, `worktrees-removed-2026-09-18.log`,
`moves-2026-09-18.log`, `repointed-scripts-2026-09-18.log`,
`deletions-2026-09-18.log` and `gates-e5d8f4b/`.

### Inventory first

Measured again before touching anything (the first pass's numbers had moved):

| | 21:37 UTC before | 21:47 UTC after |
| --- | --- | --- |
| repository total | 146 GB | 117 GB |
| `.worktrees` | 145 GB, 94 registered worktrees plus one unregistered clone | 310 MB, one worktree (this task's) |
| `.git` | 46 MB | 35 MB |
| `artifacts/` (main checkout) | 384 MB | 667 MB (seven closeouts moved in) |
| `local/` (main checkout) | 380 MB | 116 GB, of which `local/evidence` 115 GB |
| build output in worktrees | 5 directories, 1.5 GB | 0 |

Every worktree was checked for uncommitted or untracked-but-not-ignored work
(`git status --porcelain`): none had any. No tracked code references
`.worktrees/`; the cross-worktree coupling lived in ignored gate scripts
(`O=`, `M16=`, `IDLE=`, `A=`, `R=` variables pointing into five worktrees) and in
the main checkout's ignored `local/emulators/bsnes/lab-core.json`, whose
`library` pointed at the bsnes core built inside `.worktrees/m4-02-second-track`
(byte-identical, SHA-256 `e59bf88d...`, to the main checkout's own copy).

### Branches: patch-id audit, then push or delete

105 local branches besides `main`, audited with `git merge-base --is-ancestor`
and `git cherry main <branch>` (patch-id), never `git diff main..branch`:

| Class | Count | Action |
| --- | --- | --- |
| ancestor of `main` | 55 | `git branch -d` (four needed `-D` only because their `origin/` copy lagged behind `main`; each is an ancestor of `main`) |
| every commit represented on `main` by patch-id | 41 | `git branch -D`, tips recorded in the audit |
| commits not represented on `main` | 9 | pushed to `origin` first (eight new remote branches; `review/m4-16-c179765-result-restart` was already there), then deleted locally |

The nine are all review work: the M4-16 reviewer's per-candidate reports
(`review/m4-16-*`, `codex/m4-16-7c3e3b6-case-review`) whose text was merged into
`tasks/M4-16-review.md` in edited form, and the two review reports the records
cite but `main` never received: `review/classic-presentation-unification`
(`59f25e4` return, `a7f26c4` approve) and `review/dragster-window-pause`
(`55c33ea` return, `067dbfe` approve). Those four commits are cherry-picked onto
this branch, so `tasks/CLASSIC-PRESENTATION-UNIFICATION-review.md` and
`tasks/DRAGSTER-WINDOW-PAUSE-review.md` now exist on `main` as the registry
already claimed. The diverged branch `task/classic-presentation-unification`
(local 16 ahead / 11 behind its remote after the rebase integration) is an
ancestor of `main` and every remote commit's patch is on `main`; the remote copy
is left as it is. Remaining local branches: `main`, `task/repo-local-state-cleanup`.

### Evidence moved to one place, gates repointed

- `mv .worktrees/<wt>/artifacts local/evidence/<wt>` for all 75 worktrees that
  had one (including the nested `m3-03-rereview` and the unregistered clone
  `m2-01-sampling-review`): 115 GB, nothing altered inside. Relative citations
  such as `artifacts/m4-16/boundary-a` in a task record now resolve under
  `local/evidence/<that task's worktree>/`.
- The seven closeouts that lived in worktrees moved to the main checkout's
  `artifacts/<task>-integration/` beside the nine already there (16 in all):
  unification, dragster-window, dragster-ordinary, m4-16, dragster-palette-cycle,
  dragster-clock-limit, window-pause.
- 12 recorded gate scripts had their absolute input paths rewritten from
  `.worktrees/<wt>/artifacts/...` to `local/evidence/<wt>/...`; their
  `cd .worktrees/<wt>` lines are left as a record of the checkout they ran in.
- `local/emulators/bsnes/lab-core.json` now names the main checkout's own core
  (hash verified equal before the change).

### Deleted (log: `deletions-2026-09-18.log`)

| What | Why safe | Size |
| --- | --- | --- |
| `dragster-clock-limit/.../regressions/{dragster-originals,boundary-a,boundary-b}` | `diff -rq` identical to the canonical DRAGSTER originals and M4-16 boundary captures | 7.6 GB |
| `m4-15-review/*-explore.{wram,sram,json}` (eight exploration captures: delayed, early-jump, early-turns, jumps, lap-two-jump, mid-neutral, one-step, two-step; 24 files) | the reviewer's exploration runs used to choose cases; no record cites them; the frozen `*-a`/`*-b` pairs they led to are kept | 11.0 GB |
| five `build/` directories, 74 `__pycache__`, 15 empty directories, 159 run-report scratch directories (772 KB) | regenerable | 1.5 GB |
| 93 registered worktrees and the unregistered clone, with their per-worktree `local/` copies (toolchains, emulator builds) | branch pushed or represented on `main`; artifacts already moved; checkouts regenerable | about 12 GB |

Kept deliberately, with the reason, so the next pass does not re-decide them:
`m4-16-playable-zoom-zoo/m4-16` (48 GB: the M4-16 primary, the six complete
cases, the reward and trick probes and the ordinary/held/continued-controls
captures the M4-16 record cites), `m4-16-rider-art/m4-16-idle/captures`
(10 GB, gate input), `m4-16-review` (11 GB, the reviewer's independent
originals, cited 17 times), `dragster-clock-limit/{idle-a,idle-b}` (8.3 GB,
the clock-limit inventory gate's inputs), `dragster-ordinary-controls` (8.3 GB:
originals 5.9 GB plus `explore`/`fuzz` output that R-0038 and the task record
cite), `m4-15-review` and `m4-15-race-completion` (11.5 GB, M4-15 matrix
inputs), `dragster-controls-review` (3 GB, the reviewer's withheld originals),
`dragster-window-pause/window-pause` (2.3 GB, the seven pause originals).
These are original captures or cited outputs and are outside a mechanical
sweep; per-capture triage against the records remains possible future work
but nothing requires it.

## Acceptance

| Criterion | Command or experiment | Result | Artifact |
| --- | --- | --- | --- |
| No unpushed work | patch-id audit of 105 branches; every worktree's `git status` | every branch an ancestor of `main`, represented by patch-id, or pushed; no dirty worktree | `branch-audit-2026-09-18.txt`, `branches-deleted-2026-09-18.log` |
| Evidence intact | DRAGSTER primary and reversal `dragster_playable compare`, M4-16 primary and idle late-start `zoom_zoo_playable compare`, run from this worktree at `e5d8f4b` with `--reference`/`--repeat` under `local/evidence/` | all four pass, `status=passed`, restores equal to the records (DRAGSTER primary 379, reversal 327; M4-16 primary 757; idle late start 801); HEAD `e5d8f4b` clean before and after; 21:47-22:00 UTC | `artifacts/local-state-cleanup/gates-e5d8f4b/summary.txt` and the four JSON reports |
| Footprint | `du -sh` before and after | 146 GB to 117 GB, itemised above | `deletions-2026-09-18.log`, `moves-2026-09-18.log` |
| Rule recorded | `AGENTS.md` | retention rule stated; evidence layout in `docs/BUILD_AND_VALIDATION.md` | this branch's diff |

## Review

Pending at the candidate commit: a fresh Claude Opus 5 reviewer in `.worktrees/cleanup-review` (branch `review/repo-local-state-cleanup`) reproduces the branch audit, the review-file identity, the evidence layout and one differential gate. Its report is `tasks/REPO-LOCAL-STATE-CLEANUP-review.md`; the result is recorded here at integration.

## Handoff

- Candidate: `task/repo-local-state-cleanup` (docs and the four cherry-picked
  review commits; no source change). Integration and closeout as recorded in
  the Assignment.
- Not done, by decision: per-capture triage of the kept 115 GB (listed above
  with reasons); moving `local/evidence` off the workstation; any change to
  `.git` objects or remote branches (nothing was force-pushed or deleted on
  `origin`). The recorded gate scripts still `cd` into removed checkouts;
  recreate one at the recorded commit with `git worktree add` before rerunning
  a script verbatim (documented in BUILD_AND_VALIDATION).
- Mistake to avoid: a `find ... -name build` sweep over `.worktrees` also deleted
  this task's own fresh build (rebuilt, 2 minutes); exclude the live worktree.
- Next experiment if the footprint matters again: the 48 GB M4-16 capture set
  is 20 captures of 851 MB (raw WRAM per frame); a per-frame-sampled archive
  format would cut it by an order of magnitude but is a tooling task, not
  cleanup.
