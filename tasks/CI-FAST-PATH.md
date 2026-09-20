# CI-FAST-PATH - a docs-only fast path and one tooling-test run per CI job

## Assignment

- Status: reviewed and integrated; acceptance conditional on final-tip CI
  and remote verification. Closeout: ignored
  `artifacts/ci-fast-path-integration/closeout.json` in the main checkout;
  if absent, `git log --first-parent main -- tasks/CI-FAST-PATH.md` and
  `gh run list --workflow synthetic.yml --commit <commit>`. Started 20
  September 2026 01:26 UTC. Registered 19 September 2026 22:55 UTC, chosen by the user as the
  task after JEV-JUDGMENT-HARNESS after asking why half the wall clock goes to
  waiting for CI on documentation changes.
- Milestone: harness, independent of M4
- Coordinator: Claude Fable 5.1 primary session (Claude Code desktop)
- Task provider: Anthropic (unchanged)
- Worker/session/runtime/model: Claude Fable 5.1, one session; no child worker
- Actual model/reasoning effort, routing rationale and frontier escalation
  question: Fable 5.1 as the session's model; no frontier consultation - this
  is workflow configuration with a measurable outcome
- Provider quota window/baseline (D-0004): at registration 22:55 UTC five-hour
  32%, weekly all models 57%, weekly Fable 40%; at start 01:26 UTC five-hour
  66%, weekly 61%, Fable 42%; D-0004 reserve 20% of the
  weekly allowance; no reset, purchase or provider change. Sample fresh when
  the task starts.
- Reviewer: fresh Claude Opus 5 subagent in an isolated checkout at the exact
  candidate, spawned by the primary, as D-0006 requires
- Dependencies and evidence of acceptance: JEV-JUDGMENT-HARNESS integrated
  (only so the two tasks do not both edit the coordinator records); hosted
  GitHub Actions on the existing public `origin`
- Base commit: `main` at `b72b77d` (the JEV-JUDGMENT-HARNESS integration)
- Branch and isolated worktree: `task/ci-fast-path`, `.worktrees/ci-fast-path`
- Owned paths: `.github/workflows/synthetic.yml`, the new
  `.github/scripts/classify_changes.py` and its ROM-free test
  `tests/tooling/test_ci_fast_path.py` (the boundary moved at start so the
  classifier is testable in the suite rather than inline shell), the "CI and release
  evidence" section and the `test --suite synthetic` row of
  `docs/BUILD_AND_VALIDATION.md`, the final-tip CI wording in
  `docs/AGENT_WORKFLOW.md` if it needs to name the fast path, this record,
  and the coordinator records at integration. Not touched: `tools/`, `src/`,
  `tests/`, the report schema.
- Claim/checkpoint: this record and ignored `artifacts/ci-fast-path/` in the
  worktree, moved to `local/evidence/ci-fast-path/` at closeout

## Why

Measured on the last 20 `synthetic.yml` runs (19 September 2026): every run
takes 3 to 5 minutes, the macOS job is the long pole at about 4 minutes, and a
task produces 4 to 6 sequential runs (checkpoint, each review round,
integration, closeout), so a task waits 20 to 25 minutes on CI of which
roughly half is for commits that change only records. On the macOS job the two
synthetic test steps take 103 s and 80 s and the pinned SDL build 47 s; the
416 Python tooling tests run inside every `test --suite synthetic` call, so
twice per macOS job and four times per Linux job, although they do not depend
on the preset.

## Outcome and boundaries

Two changes to the hosted workflow, nothing else:

1. **A docs-only fast path that still yields a green run for the tip.** A
   first job diffs the push (or pull request) against its base and decides
   whether anything outside `docs/`, `tasks/`, Markdown files at the root and
   `.env.example` changed. When nothing did, the lab job skips its build and
   test steps and reports success, so a records-only tip still has the green
   run the acceptance rule cites. A push whose base cannot be determined (a
   new branch, a force push, a zero `before` sha) must take the full path, not
   the fast one. The workflow file itself and anything under `tools/`,
   `tests/`, `src/`, `CMake*` and `tools/locks/` always take the full path.
2. **One Python tooling-test run per job.** The `app-debug` and the two
   sanitizer test steps pass `--no-python-tests`, which the report already
   records as a skipped optional check; the `lab-debug` step keeps running them.

Out of scope: caching the SDL or lab builds, changing the runner matrix,
touching `tools/project.py test` or the report schema, changing what counts as
acceptance, and a `paths-ignore` filter that would leave a tip with no run at
all. The process-side reduction (fewer record-only commits per task) is a
workflow-document change for the coordinator, not this task.

## Inputs and prerequisites

- Push access to the public `origin` (already authorized by AGENTS.md) so the
  task branch runs on GitHub Actions; a fresh host needs nothing else.
- Baseline timings: run 35474449831 (task/jev-judgment-harness, 4 minutes,
  both jobs success) and the per-step figures above.

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| Docs-only push is fast and green | push a commit to `task/ci-fast-path` that changes only a task record | both jobs `success`; build and test steps `skipped`; run duration under 1 minute per job | run URL and `gh run view --json jobs` output |
| Code push takes the full path | push a commit that touches a file under `tools/` (a comment is enough) | both jobs `success`; every build and test step ran | run URL and jobs output |
| Undeterminable base takes the full path | the first push of the new branch | every build and test step ran | run URL |
| Tooling tests run once per job | the code push's uploaded reports | `python_tooling_tests` `passed` in `test-debug.json`; `skipped` and optional in `test-app-debug.json`, `test-sanitize.json`, `test-app-sanitize.json`; ctest checks present in all four | `artifacts/ci/*.json` from the run's `reports-*` artifact |
| Nothing else changed | `git diff main --stat -- tools src tests` | empty apart from the new `tests/tooling/test_ci_fast_path.py` | the diff |
| Docs say what CI proves | `docs/BUILD_AND_VALIDATION.md` "CI and release evidence" | names the fast path, its trigger rule and that a fast-path run is still the tip's green run | the diff |
| Independent review | fresh Opus 5 reviewer in an isolated checkout | approve, or findings fixed and re-review confirm | `tasks/CI-FAST-PATH-review.md` |

## Capability and coverage checkpoint

- Native capability delivered / still missing: none involved
- Frozen exact-match interval, field set and reference/seed identity: not applicable
- Dynamic captured inputs still consumed: none
- Relevant branches/transitions exercised: docs-only push, code push, first
  push of a branch, pull request, all four test reports
- First divergence and cheapest next discriminating experiment: not applicable
- Trial-wide usage baseline/current: five-hour 32%, weekly 57%, Fable 40% at
  registration; sample fresh at start

## Evidence and attempts

| Attempt | Hypothesis | Experiment | Observation | Next decision |
| --- | --- | --- | --- | --- |
| 1 | The classifier reads real history correctly | `GITHUB_EVENT_NAME=push CLASSIFY_BASE=<main~1> python3 .github/scripts/classify_changes.py` in this checkout | `docs_only=true: all 6 changed path(s) are documentation` for the JEV-JUDGMENT-HARNESS integration commit; an empty range gives `docs_only=false: no changed paths` | keep the empty-range default as full path |
| 2 | A first push takes the full path | first push of `task/ci-fast-path` at `0c0e7a1` (workflow, classifier, docs, this record), run 35481483430 | the `changes` job succeeded with `no base commit (new branch or empty before sha)` and `docs_only=false` (artifact `changes.json`); both lab jobs ran doctor and bootstrap and were building when the next push cancelled the run under the workflow's `cancel-in-progress` rule | the cancelled run still evidences the classification; the full path is proven by attempt 3 |
| 3 | A code push with a determinable base takes the full path and runs the tooling tests once per job | push of `628f7f9` (the test file only), run 35481505083 | success in 3.55 min; `changes` 6 s, `1 non-documentation path(s), first tests/tooling/test_ci_fast_path.py`; both lab jobs 180 s; every build and test step ran; in the uploaded reports `python_tooling_tests` is `passed` (429 py checks) in `test-debug.json` and `skipped` optional with 0 py checks in `test-app-debug.json`, `test-sanitize.json`, `test-app-sanitize.json`, each still with 23 ctest checks and the repeatability probe passed; macOS steps: lab-debug test 100 s, app-debug test under 5 s (was 80 s), SDL build 53 s; Linux sanitizers 61 s (was 134 s) | record and push this docs-only commit as attempt 4 |
| 4 | A docs-only push takes the fast path and stays green | push of `2ccef4e` (this record only), run 35481706436 | success in 0.38 min (23 s): `changes` 7 s, lab macOS 6 s, lab Linux 5 s; on both lab jobs only checkout, `Docs-only fast path` and `Upload reports` ran and all eleven build and test steps were skipped; the `changes` artifact says `all 1 changed path(s) are documentation` | review |
| 5 | The suite passes locally with the new test | `bootstrap`, `build --preset lab-debug`, `test --suite synthetic --preset lab-debug` in the worktree (`artifacts/ci-fast-path/test.json`) | `status=passed`, 10 `py:test_ci_fast_path.*` checks passed | none |
| 6 | The candidate's own push takes the fast path | push of `555a62b` (record only), run 35481755910 | success in under a minute on the fast path; the reviewer noted the run was missing from this record | recorded here |
| 7 | Independent review of `555a62b` | fresh Claude Opus 5 reviewer in `.worktrees/ci-fast-path-review`, report [CI-FAST-PATH-review](CI-FAST-PATH-review.md) (commit `da745f6`, cherry-picked here) | verdict return: three required corrections (a rename hid its deleted source path because `git diff` folds renames; tracked JSON code maps under `docs/` counted as documentation although the suite checks them; the "still the tip's green run" claim did not hold after a cancelled run because the classifier only compared trees), two should-fix (a commit was judged by the classifier it introduced; the acceptance row about `git diff main` was wrong), five advisories; twelve withheld cases, every recorded outcome reproduced | apply: `--no-renames`; documentation defined as Markdown by extension plus `.env.example`; a base-run check through `gh api` requiring a successful completed run of the workflow on the base commit before the fast path, so a fast-path success differs from the last full success only in documentation by induction; the base revision's classifier copy; the workflow test enumerates every lab step; docstring, inventory row and record corrected; the 27 s of the `changes` job accepted |
| 8 | The corrections take the full path and are judged by the base's classifier | push of `139261d` (classifier, workflow, docs, tests, record), run 35482286606 | success in 3.28 min; the `changes` job logged `classifier taken from base 555a62b` and `docs_only=false: 3 non-documentation path(s), first .github/scripts/classify_changes.py`; both lab jobs about 180 s with every build and test step run | push a records-only commit so the new classifier and the base-run gate run on the hosted runner |
| 9 | The suite passes on the corrections commit | `test --suite synthetic --preset lab-debug` in the worktree (`artifacts/ci-fast-path/test-corrections.json`) | `status=passed`, 458 checks, 13 `py:test_ci_fast_path.*`, source clean at `139261d` and unchanged during the run | none |
| 10 | A records-only push after a successful full run takes the fast path through the new gate | push of `89edda0` (record only), run 35482466386 | success in 0.38 min: `changes` 5 s, both lab jobs 7 s with only checkout, `Docs-only fast path` and `Upload reports` run; the `changes` artifact reads `all 1 changed path(s) are documentation; 1 successful completed run(s) of synthetic.yml on base 139261d` | re-review |
| 11 | Re-review of `afb8914` | the same reviewer at the exact candidate; section "Re-review at afb8914" of the report (commit `147b685`, cherry-picked here) | verdict confirm: every round-1 item resolved (stub-gh cases G1-G7, mutation tests on the step enumeration, hosted runs 35482286606 and 35482466386 read back); three residual should-fix (the `docs/map/*.md` summaries carry a digest the suite checks; this record cited a wrong run id for `89edda0`; the handoff was stale) and four advisories (the gate counts any successful run at the base, including a fast-path one, so the induction is anchored by history; a pull request's classifier copy comes from the base sha while the diff uses the merge base; a `.md` symlink under `docs/` counted as documentation; attempt rows out of order) | apply: `docs/map/` and symlinks (from `git diff --raw` modes) are never documentation, run id and row order corrected, handoff rewritten; advisories A and B accepted and recorded |
| 12 | The residual items hold | 15 classifier and workflow tests; suite on the integration tip (`local/evidence/ci-fast-path/test-final.json` after closeout) | filled in the closeout | integrate |

## Handoff

- Current base/head commit and uncommitted state: base `main` at `b72b77d`;
  branch commits `0c0e7a1` (workflow, classifier, docs), `628f7f9` (test),
  `2ccef4e`, `555a62b` (record), `a92afb6` (review), `139261d`
  (corrections), `89edda0`, `afb8914` (record), the cherry-picked re-review,
  then this integration commit (residual items plus the coordinator
  records); tree clean apart from the ignored `local/`, `build/` and
  `artifacts/`.
- Verified findings: Evidence attempts 1-12. Before: 3-5 minute runs for
  every push. After: a records-only push runs in about 23 s with every build
  and test step skipped and both jobs green (runs 35481706436, 35481755910,
  35482466386, 35482509998); a code push runs in about 3.3 min with the
  Python tooling tests once per job (runs 35481505083, 35482286606).
- Current hypothesis and failed approaches: none open.
- Commands executed, outcomes and report hashes: local suites
  `test.json` (455 checks), `test-corrections.json` (458) and `test-final.json`
  (on the integration tip, recorded in the closeout), all `status=passed`;
  the hosted runs above; their `changes` and `reports-*` artifacts under
  `ci-run1/`, `ci-run2/` and `ci-run6/`.
- Unavailable/skipped checks: no pull-request event was exercised on the
  hosted runner (the repository takes no external pull requests); the
  pull-request base rule is covered by the classifier's unit test only.
- Exact next experiment/command: none; the task is integrated.
- Remaining dependencies: none.
- Runtime needs: GitHub Actions on `origin` with `actions: read` for the
  base-run check; the local suite needs the isolated toolchain and a
  `lab-debug` build.
- Aggregate parent/child time, provider usage before/after: 01:26 UTC to
  about 02:05 UTC on 20 September 2026 including two review rounds (review
  about 9 minutes, re-review about 8); five-hour 66% to 79%, weekly 61% to
  63%, Fable 42% to 44% (the window also carried the JEV closeout).
- Accepted outcome, review/fix rounds and next routing decision: the two
  workflow changes as specified, after one returned review and a confirming
  re-review with residuals applied; no next task is dispatched.
- Closeout moves (AGENTS.md retention rule): the worktree's
  `artifacts/ci-fast-path/` (local suites), `artifacts/ci-run1/`,
  `artifacts/ci-run2/`, `artifacts/ci-run6/` (downloaded hosted artifacts)
  and `artifacts/ci-fast-path-run*.log` (run watch logs) move to
  `local/evidence/ci-fast-path/`, and the review checkout's `artifacts/` to
  `local/evidence/ci-fast-path-review/`, in the main checkout; the worktree's
  `build/` is deleted; the worktrees `.worktrees/ci-fast-path` and
  `.worktrees/ci-fast-path-review` and the local branches `task/ci-fast-path`
  and `review/ci-fast-path` are removed after integration, both branches
  staying on `origin`.

## Review and integration

- Reviewer and independent reproduction/withheld-case results: a fresh
  Claude Opus 5 subagent in `.worktrees/ci-fast-path-review`; round 1 at
  `555a62b` returned (report `da745f6`, twelve withheld cases, every recorded
  outcome reproduced except the acceptance row it corrected); re-review at
  `afb8914` confirm (report `147b685`, seven gate cases and two mutation tests)
- Required changes or acceptance rationale: the three required and two
  should-fix items and advisories 1, 2, 3 and 5 applied at `139261d`;
  advisory 4 (the `changes` job's 27 s) accepted as the price of the gate
- Exact merge candidate and required-check results: this integration
  commit on the branch (residuals plus records); its hosted run and the
  local suite are in the closeout.
- Integrated commit and evidence location: fast-forward of `task/ci-fast-path`
  onto `main`; evidence under `local/evidence/ci-fast-path/` and
  `local/evidence/ci-fast-path-review/`.
- Remote synchronization: recorded in the closeout after the push of `main`.
- Scope still unverified: the pull-request path on the hosted runner.
