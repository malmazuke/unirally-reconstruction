# REPO-LOCAL-STATE-CLEANUP - branches, worktrees and ignored artifacts

## Assignment

- Status: registered 18 September 2026 at the user's request; not started
- Milestone: housekeeping, independent of M4
- Coordinator: Claude Opus 5 primary session
- Base commit: `main` at `b1dd0a3`
- Branch and isolated worktree: `task/repo-local-state-cleanup`,
  `.worktrees/repo-local-state-cleanup`
- Reviewer: fresh Claude Opus 5 subagent at the exact candidate

## Why

The user asked whether work can be called done while local state is floating
about. Measured on 18 September 2026, it cannot:

| | measured |
| --- | --- |
| repository total | 151 GB |
| `.worktrees` | 150 GB, 93 directories (94 registered with git) |
| `.git` | 45 MB |
| `artifacts/` in the main checkout | 384 MB, 175 directories |
| build output across worktrees | 9.9 GB in 77 `build/` directories |
| `artifacts/` across worktrees | 131 GB |
| original captures inside that | 5.7 GB |

The largest single directory is
`.worktrees/m4-16-playable-zoom-zoo/artifacts/m4-16` at 47 GB in 2,790 files:
per-capture WRAM, SRAM and video, so a few very large files rather than many
small ones. Many zero-byte hex-named scratch directories also sit in
`artifacts/` in several checkouts.

Branches, measured the same day: 64 fully merged into `main` and safe to
delete, 49 with commits not on `main` and not on origin, 5 diverged from their
pushed copy. The 49 are mostly `review/*` and `codex/*` branches from earlier
milestones whose review records already live on `main` as `tasks/*-review.md`.

## What is evidence and must not be deleted

This is the whole risk, and a careless sweep would destroy reproducibility:

- **Original captures** (`**/originals/**`, `baseline-*.wram`/`.sram`,
  `reference.json`) are the project's evidence. Gates and differential compares
  read them by absolute path across worktrees - DRAGSTER's gates read
  `.worktrees/dragster-ordinary-controls/.../originals`, and the M4-16 compares
  read `.worktrees/m4-16-playable-zoom-zoo/artifacts/m4-16/boundary-a`. They are
  5.7 GB of the 131 GB and they stay.
- **Closeouts** (`artifacts/*/closeout.json`) are ignored but are cited by
  accepted task records as the acceptance evidence. Tiny; they stay.
- **Gate logs and comparison reports** cited by task records or reviews. Small;
  keep unless the citing record is itself superseded.

Regenerable without judgement: `build/` directories (9.9 GB), zero-byte scratch
directories, and `__pycache__`.

The remaining ~125 GB is intermediate capture and comparison output. Some is
reproducible only by re-running a capture against the user's private ROM, which
is expensive and needs the ROM, so "regenerable in principle" is not the same as
"safe to delete". Each case needs a decision, not a glob.

## Outcome and boundaries

A repository whose local state is either pushed, reproducible, or deliberately
kept, with the rule written down so it does not recur.

1. Branches: for each of the 49, decide with a patch-id comparison - not a
   `git diff main..branch`, which mostly reports main's own progress in reverse
   and produced a misleading 300-400 file count when first tried - whether its
   commits are represented on `main`. Push what is not; delete what is, with the
   user's agreement. Resolve the 5 diverged branches explicitly. Delete the 64
   merged refs.
2. Worktrees: remove those whose branch is merged and whose artifacts are not
   cited. Keep the ones gates read from, or move their originals somewhere
   stable first and repoint the gates - the cross-worktree absolute paths are
   themselves a defect worth fixing while here.
3. Artifacts: delete build output and scratch directories; triage the large
   intermediate captures against the records that cite them.
4. Write the retention rule into `AGENTS.md` so future tasks clean up as they
   close rather than leaving this to accumulate.

Out of scope: rewriting history, touching `.git` objects, or deleting anything
the user has not agreed to.

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| No unpushed work | branch audit by patch-id | every branch pushed or provably represented on `main` | audit table |
| Evidence intact | re-run the DRAGSTER and M4-16 differential gates after cleanup | pass, reading the same originals | gate logs |
| Footprint | `du -sh` before and after | recorded, with what was removed and why | measurement |
| Rule recorded | `AGENTS.md` | retention rule stated | diff |

## Handoff

- Not started. The measurements above are from 18 September 2026 and should be
  retaken first, since worktrees are still being created.
- Do not delete anything before the evidence inventory exists: the gates'
  absolute cross-worktree paths mean a directory that looks abandoned can be a
  live gate input.
