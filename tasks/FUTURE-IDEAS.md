# FUTURE-IDEAS - preserve deferred ghost racing ideas

## Assignment

- Status: documentation recorded in
  [PR #47](https://github.com/malmazuke/unirally-reconstruction/pull/47),
  claimed 30 September 2026; integration result is in the PR and closeout.
- Milestone: none. This records future ideas without scheduling feature work.
- Coordinator/primary/integrator: current OpenAI Codex session; no children.
- Actual model/effort: not exposed by this runtime; no model override or frontier
  consultation requested for this routine documentation change.
- Review tier: **3**, records only under D-0008; no independent review required.
- D-0004 quota: weekly-used 2%, remaining 98%, sampled at session start and
  recorded 30 September 2026 at 10:16:18 UTC; reset epoch 1791365217. Boundary 22%
  used, with the final 20% reserved. No reset, purchase or other spending.
- Base: `a2cdb9c`, synchronized `main`/`origin/main` at claim.
- Branch/worktree: `codex/future-ideas` in `.worktrees/future-ideas`.
- Owned paths: `docs/FUTURE_IDEAS.md`, `README.md`, this record and its registry
  entry in `tasks/README.md`. The current capability roadmap remains unchanged.
- Checkpoint: this record; one primary, 45-minute reassessment if needed.

## Outcome and boundaries

Preserve the user's deferred direction: personal ghosts, shared leaderboards,
roughly 20 downloaded ghosts in one race and trusted native replay verification.
Record the user's decision to defer proving unaided human play. Make the note
discoverable without creating scheduled ghost/online implementation tasks or
altering current reconstruction priorities.

The user endorsed the preceding design discussion and asked where to document
it because they do not plan to work on it for a while. `docs/FUTURE_IDEAS.md`
keeps that distinction explicit; an architecture decision would prematurely
freeze a design, and a ready task would imply work the user has deferred.

## Acceptance and evidence

- Inspect the note against the discussion: desired experience, verification,
  storage, limitations and human-play deferral are present and marked proposed.
- Check relative Markdown links in the four changed files, new prose for ASCII
  hyphens and the staged diff for scope and accidental private material.
- Run `git diff --check` and the existing CI documentation classifier on the
  exact candidate; require all three hosted checks before merge.
- No gameplay tests or ROM captures are applicable. Hosted documentation fast
  paths skip build/gameplay suites; a green job is not a gameplay test pass.

## Handoff

- Content candidate: `e8643bb`, pushed and captured in PR #47; the closing
  record commit and exact merged head are identified by the closeout below.
- Local results: staged whitespace/scope inspection passed; all four changed
  Markdown files use ASCII hyphens in new prose and their six added local links
  resolve. The existing classifier reports `docs_only=true` for all four paths
  against `a2cdb9c`, inheriting successful CI from merged head `596585b`.
- Commands: `git diff --cached --check`; a one-off Python check of added local
  links and Unicode dashes; `GITHUB_EVENT_NAME=pull_request
  CLASSIFY_BASE=a2cdb9c CLASSIFY_REQUIRE_BASE_RUN=synthetic.yml
  GITHUB_REPOSITORY=malmazuke/unirally-reconstruction python3
  .github/scripts/classify_changes.py`. Re-run the classifier and whitespace
  check on the final tip; hosted job results belong in the closeout.
- Tier 3 needs no reviewer. Read PR #47's conversation/review and line comments
  and require its current-head checks before the authorized merge commit.
- Final source/head, PR, hosted checks, merge commit, main synchronization and
  fresh usage will be recorded in main's ignored
  `artifacts/future-ideas-integration/closeout.json`.
- Cleanup: verify the pushed task head is in merged `main`, remove only this
  session's worktree and branch, and record the deletion in the closeout. No
  captures, builds or other workers' files are created or removed by this task.
- Next: resume the existing reconstruction assignment. Ghost and leaderboard
  implementation remains deferred until the user takes it up.
