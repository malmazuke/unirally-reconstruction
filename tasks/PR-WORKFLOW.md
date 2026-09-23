# PR-WORKFLOW - changes reach main through pull requests

## Assignment

- Status: reviewed (approve with should-fix items, all applied) and integrated by
  [pull request #6](https://github.com/malmazuke/unirally-reconstruction/pull/6) (requested by the user on 23 September 2026 after STATIC-CODE-MAP:
  "start using PRs instead of just pushing straight to `main`", checks on pull requests
  rather than on `main`, and pull request descriptions a human can read)
- Coordinator, primary and integrator: Claude Opus 5.5 (`claude-opus-5-5`), Claude Code desktop
- Review tier (D-0008): **2** (CI and process tooling; no game code, packs or gates). One
  independent round by a fresh Anthropic subagent in its own checkout.
- Branch and worktree: `task/pr-workflow` in `.worktrees/pr-workflow`; base `main` `b4c1f4f`.
- Usage (D-0004): weekly all-models 89% at start; the user's instruction covers this work.

## Outcome

1. `synthetic.yml` runs on `pull_request` and `workflow_dispatch` only; pushes run nothing.
2. The docs-only fast path keeps its induction without runs on `main`. A `main` commit is a
   merge commit that never gets a run of its own, so the classifier counts it through its
   second parent, the merged pull request's head. The up-to-date requirement makes that
   run a test of the merge's tree. Tested in `test_ci_fast_path.py`.
3. `.github/pull_request_template.md`: what changed, why, evidence, review, not covered,
   with a style note (human reader, facts, about 30 lines).
4. AGENTS.md, `docs/AGENT_WORKFLOW.md` and `docs/BUILD_AND_VALIDATION.md`: the integration
   flow is branch, pull request, green checks on an up-to-date branch, review, merge commit;
   opening and merging the project's own pull requests is authorized, pushing to `main` is not.
   There is no post-merge records commit and no acceptance "conditional on final-tip CI".
5. Repository settings, applied by the coordinator with the user's approval of 23 September
   2026: a ruleset on `main` (pull request required with 0 approvals, the `changes`,
   `lab (ubuntu-24.04)` and `lab (macos-15)` checks required on an up-to-date branch, no
   force-push or deletion, no bypass); merge commits only; merge commit title and message
   from the pull request; fork workflows need approval for all outside collaborators.

Why 0 approvals: agents act as the repository owner, and GitHub does not let an author
approve their own pull request, so a required approval would put the user in every merge.
The independent review is linked from the pull request. Why merge commits only: records cite
task-branch commit IDs, which squash and rebase would drop from `main`.

## Evidence and attempts

| Attempt | Hypothesis | Experiment | Observation | Next decision |
| --- | --- | --- | --- | --- |
| 1 | The fast path needs a new base rule without runs on `main` | Read `classify_changes.py`: a docs-only result needs a successful run on the base commit | Under merge commits no `main` commit would ever have one, so every pull request would take the full 3-minute path | Count a merge-commit base through its second parent; test both outcomes and the non-merge case |
| 2 (review F1, F2) | The second-parent rule is safe | Reviewer's reproduction: broken code, `git merge main`, then a docs commit; the workflow compared against `github.event.before`, the pull request's own previous head | `docs_only=true`: the merge borrowed `main`'s run and all three checks went green | Classify against the pull request's base only, and accept a second parent's run only when its tree equals the merge's; the reproduction is now a test (17 tests pass) |

## Handoff

- Commands: `python3 -m unittest tests/tooling/test_ci_fast_path.py` (17 tests, OK). The full
  suite ran as the pull request's checks (`changes`, both `lab` jobs, full path) on the
  reviewed head `c4cafb8` and again on the head that merged; it was not run locally, since the
  change touches no native or lab code outside the classifier.
- Review: one fresh Claude Opus 5.5 subagent in `.worktrees/review-pr-workflow` at `c4cafb8`,
  **approve with should-fix items** (report `66da907` on `review/pr-workflow`, pushed). F1 (a
  merge could borrow another commit's run) and F2 (the classifier base was the pull request's
  previous head) fixed as above; F3 (authority sentences moved back into their bullet), F4
  (absolute links; update the Review section before merging), F5 (records final before the
  merge), F6 (push-era wording, `gh pr update-branch`) and F7 (a failing second lookup, now
  tested) applied. The reviewer noted the ruleset field
  `require_extra_approval_for_unattributed_changes: true`, which is a default.
- Cleanup at merge: remove `.worktrees/pr-workflow` and `.worktrees/review-pr-workflow` and both
  local branches after checking `task/pr-workflow` and `review/pr-workflow` on `origin`.
- This pull request is the first under the new flow; its own run exercises the new triggers.
  Its classification uses the base revision's classifier, as designed.
- Next: nothing further is required. The old memory note about dispatching CI for `codex/*`
  branches is obsolete, since every pull request runs the checks.
