# CI-FAST-PATH - a docs-only fast path and one tooling-test run per CI job

## Assignment

- Status: review. Started 20 September 2026 01:26 UTC. Registered 19 September 2026 22:55 UTC, chosen by the user as the
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
| Nothing else changed | `git diff main -- tools tests src` | empty | the diff |
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

## Handoff

- Current base/head commit and uncommitted state: base `main` at `b72b77d`;
  commits `0c0e7a1` (workflow, classifier, docs), `628f7f9` (test),
  `2ccef4e` (experiments 1-3), then this checkpoint; tree clean apart from
  the ignored `local/`, `build/` and `artifacts/`.
- Verified findings: see Evidence, attempts 1-5. Before: 3-5 minute runs for
  every push. After: a docs-only push runs in 23 s with every build and test
  step skipped and both jobs green; a code push runs in 3.55 min with the
  Python tooling tests once per job (macOS app-debug test step 80 s to under
  5 s; Linux sanitizer step 134 s to 61 s).
- Current hypothesis and failed approaches: none open. The first push's run
  was cancelled by the existing `cancel-in-progress` rule when the second
  push followed 30 s later; its `changes` job had already classified the
  new branch as full path, which is the evidence for that criterion.
- Commands executed, outcomes and report hashes: local suite
  `artifacts/ci-fast-path/test.json` status passed (10 new checks); hosted
  runs 35481483430 (cancelled after classification), 35481505083 (full
  path, success), 35481706436 (fast path, success); their `changes` and
  `reports-*` artifacts are downloaded under `artifacts/ci-run1/` and
  `artifacts/ci-run2/` in the worktree.
- Unavailable/skipped checks: no pull-request event was exercised on the
  hosted runner (the repository takes no external pull requests); the
  pull-request base rule is covered by the classifier's unit test only.
- Exact next experiment/command: independent review of this checkpoint.
- Remaining dependencies: none.
- Runtime needs: GitHub Actions on `origin`; the local suite needs the
  isolated toolchain and a `lab-debug` build.
- Aggregate parent/child time, provider usage before/after: 01:26 UTC to
  01:45 UTC to this checkpoint; five-hour 66% at start, weekly 61%, Fable
  42% (sampled again at closeout).
- Accepted outcome, review/fix rounds and next routing decision: pending
  review.

## Review and integration

- Reviewer and independent reproduction/withheld-case results: pending
- Required changes or acceptance rationale: pending
- Exact merge candidate and required-check results: pending
- Integrated commit and evidence location: pending
- Remote synchronization: pending
- Scope still unverified: all
