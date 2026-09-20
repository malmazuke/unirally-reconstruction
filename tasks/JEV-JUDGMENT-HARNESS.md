# JEV-JUDGMENT-HARNESS - advisory Jev judgments as recorded harness checks

## Assignment

- Status: reviewed and integrated; acceptance conditional on final-tip CI
  and remote verification. Closeout: ignored
  `artifacts/jev-judgment-harness-integration/closeout.json` in the main
  checkout; if absent, `git log --first-parent main -- tasks/JEV-JUDGMENT-HARNESS.md`
  and `gh run list --workflow synthetic.yml --commit <commit>`. Registered 19 September 2026 22:15 UTC from `main` at
  `fd34209`, chosen by the user as the next task while CLASSIC-STUNT-NAMES
  runs in another session; started in the same session that registered it.
- Milestone: harness, independent of M4. Introduces
  [D-0007](../docs/decisions/D-0007-advisory-jev-judgments.md).
- Coordinator: Claude Fable 5.1 primary session (Claude Code desktop)
- Task provider: Anthropic (unchanged)
- Worker/session/runtime/model: Claude Fable 5.1, one session; no child worker
- Actual model/reasoning effort, routing rationale and frontier escalation
  question: Fable 5.1 as the session's model; no frontier consultation - the
  work is tooling with a fixed external contract (the TypeSafe HTTP API)
- Provider quota window/baseline (D-0004): at registration 22:15 UTC five-hour
  13%, weekly all models 55%, weekly Fable 37%; D-0004 reserve 20% of the
  weekly allowance; no reset, purchase or provider change. TypeSafe usage is a
  separate account of the user's; each call's token count is in its artifact.
- Reviewer: fresh Claude Opus 5 subagent in an isolated checkout at the exact
  candidate, spawned by the primary, as D-0006 requires
- Dependencies and evidence of acceptance: none on other tasks. External: the
  user's TypeSafe account and API key (created 19 September 2026, stored in the
  ignored `.env` of the main checkout); the TypeSafe HTTP API as documented at
  https://docs.typesafe.ai/api (read 19 September 2026: `POST /v1/systemone`,
  bearer key, `jev-latest` resolving to `jev-1.13.0`)
- Base commit: `main` at `fd34209`
- Branch and isolated worktree: `task/jev-judgment-harness`,
  `.worktrees/jev-judgment-harness`
- Owned paths: `tools/unirally_lab/judgment.py`,
  `tools/unirally_lab/judge_commands.py`, the `judge` registration in
  `tools/project.py`, the optional `typesafe_api_key` doctor check in
  `tools/unirally_lab/lab_commands.py`, `tests/tooling/test_judgment.py`,
  `.env.example`, `docs/decisions/D-0007-*`, the `judge` rows and section in
  `docs/BUILD_AND_VALIDATION.md`, one D-0007 bullet in `AGENTS.md`, this
  record, and the coordinator records (`tasks/README.md`,
  `tasks/NEXT_SESSION.md`, `docs/STATE.md`) at integration. Not touched:
  `src/`, gates, manifests, the synthetic CI workflow.
- Claim/checkpoint: this record and ignored `artifacts/jev-judgment-harness/`
  in the worktree, moved to `local/evidence/jev-judgment-harness/` at closeout
- Registry note: [CI-FAST-PATH](CI-FAST-PATH.md) was registered on this branch
  on 19 September 2026 at 22:55 UTC as the user's chosen task after this one,
  for the same reason as the next item; it lands with this integration.
- Registry note: the registration lives on the task branch, not `main`,
  because CLASSIC-STUNT-NAMES is being integrated by another session and
  `tasks/NEXT_SESSION.md` is its closeout surface; the coordinator records are
  reconciled once at this task's integration, after that one lands.

## Outcome and boundaries

Give the harness a way to ask TypeSafe's Jev a typed question and keep the
answer as evidence, so that judgments agents now make by hand inside a long
context (does a record name its independent check, does a handoff overclaim,
does a turn drift out of scope) can be offloaded to a sub-second call with a
recorded request and response.

In scope: a standard-library client (key discovery, curl-first transport with
the key off the command line, bounded retries on 429/529, artifact writing
that refuses to leak the key); `project.py judge ping|ask|evidence-lint`; an
optional doctor check; `.env.example`; tests with a local stub server; the
D-0007 decision that every judgment is advisory; documentation of the command
contract.

Out of scope: any required check backed by a judgment; the scope-drift,
stopping-rule, handoff-overclaim and review pre-triage question sets (each is
a follow-up that reuses `judge ask` and needs its own labelled evaluation);
the vendor SDK; any change to native code, gates, differential compares, the
synthetic CI or how acceptance is decided.

## Inputs and prerequisites

- `TYPESAFE_API_KEY` in the environment or in the ignored `.env` at the
  repository root (`.env.example` is the template). Without it every `judge`
  command reports the key `missing` and exits 2; `doctor` reports it as an
  optional `missing` check. The synthetic CI never has it.
- `curl` on `PATH` (already required by `doctor`; the python.org Python has no
  CA bundle, so urllib is only the fallback).
- Network access to `api.typesafe.ai`. A fresh host needs nothing else; no
  ROM, pack or build is involved in any `judge` command.
- Known baseline: none. The tooling suite passed on `main` at `fd34209`.

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| Client and CLI verified without network or key | `python3 tools/project.py test --suite synthetic --preset lab-debug --report <r>` | `status=passed`; `py:test_judgment.*` all `passed` (stub server covers ping, `.env` key, 401, 429 retry, timeout exit 4, `ask` exit 3 paths, lint flags optional, doctor optional check) | test report and junit under `artifacts/jev-judgment-harness/test-lab-debug/` |
| Real endpoint reachable with the user's key | `python3 tools/project.py judge ping --report <r> --out <dir>` | exit 0; `judgment:ping passed` naming the versioned model; artifact carries request, response, usage, elapsed; no key in report or artifact | `artifacts/jev-judgment-harness/ping/{report.json,ping.json}` |
| Evidence lint over real records | `python3 tools/project.py judge evidence-lint --record <R-0041> --record <R-0001> --record <R-0035> --report <r> --out <dir>` | exit 0; one artifact per record with all six answers; every `lint:*` check optional | `artifacts/jev-judgment-harness/lint/` |
| Key handling | `doctor` with and without `.env`; `grep` of every report and artifact for the key | optional check `missing` then `passed (from .env)`; zero occurrences of the key | doctor reports; the grep in the handoff |
| CI unaffected | hosted `synthetic.yml` on the task branch tip | green on both runners with no key and no `judge` step | run id in the handoff |
| Independent review | fresh Opus 5 reviewer in an isolated checkout at the candidate | approve, or findings fixed and re-review confirm | `tasks/JEV-JUDGMENT-HARNESS-review.md` |

## Capability and coverage checkpoint

- Native capability delivered / still missing: none involved; this is harness
  tooling. Nothing under `src/` changes.
- Frozen exact-match interval, field set and reference/seed identity: not
  applicable.
- Dynamic captured inputs still consumed: none.
- Relevant branches/transitions exercised: key from environment, key from
  `.env`, no key; HTTP 200, 401, 422 (in process), 429 then 200, 429
  exhausted, 500, timeout; malformed and incomplete answer sets; invalid
  questions rejected before any request; artifact refusing to contain the key.
- First divergence and cheapest next discriminating experiment: not applicable.
- Trial-wide usage baseline/current: five-hour 13%, weekly 55%, Fable 37% at
  registration; updated in the handoff.

## Evidence and attempts

| Attempt | Hypothesis | Experiment | Observation | Next decision |
| --- | --- | --- | --- | --- |
| 1 | The endpoint accepts a bearer key from `.env` and returns typed answers | urllib POST from python.org Python 3.14 | `CERTIFICATE_VERIFY_FAILED`: no CA bundle, the same failure `toolchain.py` documents for wheel downloads | curl first, urllib fallback, as the downloader does |
| 2 | Same request through curl | two-question smoke request (noul on a placeholder independent check, choice on status support) | HTTP 200 in 685 ms; `jev-1.13.0`; noul 0.04, choice `unsupported` at confidence 0.83; 421 input tokens | the shape is what the docs describe; build the client on it |
| 3 | `judge ping` reproduces attempt 2 through the tooling | `judge ping --out artifacts/jev-judgment-harness/ping` | exit 0, `judgment:ping passed` in 0.69 s via curl, answers as expected, artifact without the key | keep the fixed ping state as the smoke contract |
| 4 | The six evidence questions run over real records in one request each | `judge evidence-lint` over R-0041, R-0001, R-0035 | 0.66-0.83 s per record, 2.4k-5.9k input tokens; no flags; lowest readings `reproducible` 0.55-0.56 on R-0041 and R-0035, `independent_check_named` 0.64 on R-0041 | thresholds stay as starting points (D-0007); a labelled pass over more records is follow-up work |
| 5 | curl's own timeout surfaces as the `timeout` outcome | stub server sleeping 3 s, `--timeout 1` | curl exit 28 with `000` was reported as `HTTP 0`, exit 1 | map curl exit 28 to `timeout` (exit 4); any other nonzero curl exit is `failed` with curl's stderr |
| 6 | The thresholds behave sensibly across the whole research corpus | `judge evidence-lint` over all 46 `docs/research/R-*.md` in one run (`artifacts/jev-judgment-harness/lint-all/`) | 46 of 46 answered by `jev-1.13.0`, 237,302 input tokens, 36.6 s total, 1.00 s slowest; 31 optional flags on 24 records: 22 `reproducible` (lowest 0.07, R-0031 retrospective), 6 `independent_check_named`, 2 `identity_stated`, 1 `falsifiable`, 2 `status_supported` overclaimed (R-0025 at 0.87, R-0026 at 0.83); R-0025's own text carries "accepted" in its status line and "review pending fresh focused re-review" at line 191, so that flag reads a real inconsistency; exit 0 throughout because every flag is optional | thresholds unchanged; the record-by-record reading is follow-up work, not this task's |
| 7 | The tooling suite passes with the new tests | `bootstrap`, `build --preset lab-debug`, `test --suite synthetic --preset lab-debug` in the worktree | all exit 0; 408 tooling tests (23 new) and 434 checks passed, none failed, skipped, missing or timed out; the report notes the source changed during the run because this record was being written, so the suite is rerun on the committed candidate below | rerun on the commit |
| 8 | Independent review of `b6e457b` | fresh Claude Opus 5 reviewer in `.worktrees/jev-judgment-harness-review`, report [JEV-JUDGMENT-HARNESS-review](JEV-JUDGMENT-HARNESS-review.md) (commit `8d11fe2`, cherry-picked here) | verdict return: two required corrections (duplicate `--record` stems silently overwrote one artifact and duplicated check names; `--name` accepted path syntax and wrote outside `--out`), four should-fix (artifact write outside the guarded block so the key-leak guard crashed without a report; a non-object answer crashed after the artifact was written; an unescaped pipe broke the `judge ask` table row; `--attempts 0` and negative or fractional `--timeout` unvalidated or truncated), six advisories; every claimed outcome reproduced | apply all required and should-fix items and the cheap advisories (D-0007 wording, report key redaction, malformed keys refused, records under `local/` refused, record ordering, README status) |
| 9 | The corrections hold | seven new tests (duplicate stems and `local/` refused, `--name` and argument validation with a fractional timeout, key in state fails with a report and no artifact, non-object answers fail with a report, malformed key refused before any request, redaction) plus the full suite | 30 judgment tests and 415 tooling tests passed; suite `status=passed` (`artifacts/jev-judgment-harness/test-fixes.json`) | re-review |
| 10 | Re-review of `f391a3f` | the same reviewer, moved to the exact candidate; section "Re-review at f391a3f" of the report (commit `a6eaf19`, cherry-picked here) | verdict confirm: every required, should-fix and advisory item resolved except the accepted advisory 4; suite at the candidate 441 checks passed; one residual should-fix (a record named `evidence-lint` collided with the summary file) and two advisories (the `local/` refusal compared an unresolved path; redaction was only unit-tested) | apply the three residual items |
| 11 | The residual items hold | `evidence-lint` reserved as a record name, `local/` resolved before the comparison, an end-to-end test that plants the key in a 500 body and checks the written report; suite rerun | 31 judgment tests and 416 tooling tests passed; suite `status=passed` (`artifacts/jev-judgment-harness/test-residual.json`); hosted CI on `f391a3f` green on both runners (run 35473295813's successor, see the closeout) | integrate after CLASSIC-STUNT-NAMES lands on `main` |

Artifacts: `artifacts/jev-judgment-harness/` in the worktree (ping, lint,
doctor, bootstrap, build and test reports and logs).

## Handoff

- Current base/head commit and uncommitted state: implementation `ec64a17` on
  `task/jev-judgment-harness` (base `main` at `fd34209`); this checkpoint
  commit adds only this record; the tree is clean apart from the ignored
  `.env`, `local/`, `build/` and `artifacts/`.
- Verified findings: the endpoint answers a bearer-key request from this host
  through curl in 0.66-1.00 s per record and rejects urllib from the python.org
  Python for want of a CA bundle; `jev-latest` resolved to `jev-1.13.0` on
  19 September 2026; all six evidence questions come back in one request per
  record at 2.4k-5.9k input tokens; every flag is optional and the exit code
  never depends on one.
- Current hypothesis and failed approaches: none open. The one defect found on
  the way (curl's timeout reported as `HTTP 0`) is fixed and covered by
  `test_timeout_exit_code`.
- Commands executed, outcomes and report hashes (SHA-256 prefixes; all under
  `artifacts/jev-judgment-harness/` in the worktree, exit 0 unless stated):
  `doctor` `377db1aebc2e0fc7` (`typesafe_api_key` optional, `from .env`);
  `judge ping` report `a89b8c8e1d553b75`, artifact `ping.json`
  `1409d40719c10d6a`; `judge evidence-lint` over R-0041, R-0001, R-0035 report
  `69a042f8a371f22b`; the 46-record sweep report `ad13fdd1ece4653b`, summary
  `evidence-lint.json` `2416dd166d052eaf`; `bootstrap`, `build --preset
  lab-debug`, then `test --suite synthetic --preset lab-debug` on the committed
  candidate `ec64a17`: report `test-candidate.json` `ec8812c0830d9261`, status
  passed, 434 checks passed, none failed, skipped, missing or timed out, source
  clean and unchanged during the run.
- Key hygiene: `grep -rlF` of the key value over `artifacts/`, `tools/`,
  `tests/`, `docs/`, `tasks/`, `AGENTS.md` and `.env.example` found 0 files;
  `.env` is ignored and was never staged (checked before the commit).
- Unavailable/skipped checks: hosted CI on the branch tip runs after the push
  (recorded under Review and integration); no check was skipped locally.
- Exact next experiment/command: integration onto `main` once
  CLASSIC-STUNT-NAMES has landed there, with the coordinator records
  reconciled in that commit; CI on the final tip.
- Remaining dependencies: none.
- Runtime needs: network to `api.typesafe.ai` and the key for the two
  real-endpoint criteria; `curl`; the isolated toolchain and a `lab-debug`
  build for the suite's native part (bootstrap from the shared wheel cache
  took the ordinary route, `source=cache`).
- Aggregate parent/child time, provider usage before/after: one session,
  22:15 to 22:25 UTC to this checkpoint (about ten minutes of wall clock,
  excluding the earlier conversation that chose the task); provider usage
  five-hour 13% to 20% at the first checkpoint (22:25 UTC) and to the figure
  in the closeout at integration; weekly all models 55% to 56%, weekly Fable
  37% to 39% at that checkpoint;
  TypeSafe usage for the task 246,437 input tokens across 50 requests (ping,
  three-record lint, 46-record sweep), all recorded in the artifacts. No other
  account work is excluded from those figures; the other session's stunt-names
  work shares the same weekly windows.
- Accepted outcome, review/fix rounds and next routing decision: the
  advisory judgment harness as specified, after one returned review and one
  confirming re-review; next task CI-FAST-PATH, chosen by the user.
- Integration and closeout moves (AGENTS.md retention rule): the branch was
  rebased onto `main` at `2f952ab` (after CLASSIC-STUNT-NAMES) with the only
  conflicts in `tasks/README.md` rows, resolved by keeping `main`'s rows and
  this task's two rows; the suite on the rebased tip `141d67f` passed (445
  checks, `test-rebased.json`) and `judge ping` still answered. Moved:
  the worktree's `artifacts/jev-judgment-harness/` (2.0 MB: doctor, bootstrap,
  build, five suite runs, ping, the three-record lint, the 46-record sweep,
  CI watch logs) to `local/evidence/jev-judgment-harness/` in the main
  checkout, and the review checkout's `artifacts/` (376 KB: the reviewer's
  doctor, suite, ping, lint and withheld-case runs for both rounds) to
  `local/evidence/jev-judgment-harness-review/`. Deleted: six run-id
  directories the tooling tests left under the worktree's `artifacts/`,
  which no record cites, and the worktree's `build/`. At closeout the session
  removes `.worktrees/jev-judgment-harness` and
  `.worktrees/jev-judgment-harness-review` and the local branches
  `task/jev-judgment-harness` and `review/jev-judgment-harness`; both stay on
  `origin`, and `git cherry main origin/review/jev-judgment-harness` shows
  every review commit on `main` by cherry-pick. The closeout JSON records the
  final `main` commit, the remote verification, the CI run and the usage.
- Wall clock and usage: registration 22:15 UTC to the integration commit at
  about 01:20 UTC on 20 September, of which 22:50 to 01:11 was waiting for
  CLASSIC-STUNT-NAMES to land (the user chose to keep that order); about 55
  minutes of work. Provider usage five-hour 13% to 63% (the window also
  carried the other session), weekly all models 55% to 61%, weekly Fable 37%
  to 41%. TypeSafe usage 246,858 input tokens over 51 requests by the
  primary plus the reviewer's own runs, all in the artifacts.

## Review and integration

- Reviewer and independent reproduction/withheld-case results: a fresh
  Claude Opus 5 subagent in `.worktrees/jev-judgment-harness-review`; round 1
  at `b6e457b` returned (report `8d11fe2`, every claimed outcome reproduced,
  10 minutes); re-review at `f391a3f` confirm (report `a6eaf19`, 6 minutes);
  the withheld cases are in the report
- Required changes or acceptance rationale: the two required and four
  should-fix items applied at `f391a3f`; the re-review's residual should-fix
  and two advisories applied in the commit after the cherry-picked re-review;
  advisory 4 accepted as is
- Exact merge candidate and required-check results: the rebased tip
  `141d67f` plus this integration-records commit; suite passed at `141d67f`
  (445 checks); hosted CI on the pre-rebase tips `f391a3f`, `39473ec` and
  `ac0891b` green on both runners; the run on the integration tip is recorded
  in the closeout.
- Integrated commit and evidence location: fast-forward of
  `task/jev-judgment-harness` onto `main`; evidence under
  `local/evidence/jev-judgment-harness/` and
  `local/evidence/jev-judgment-harness-review/` in the main checkout.
- Remote synchronization: recorded in the closeout after the push of `main`.
- Scope still unverified: the follow-up question sets named under boundaries
