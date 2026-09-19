# JEV-JUDGMENT-HARNESS - advisory Jev judgments as recorded harness checks

## Assignment

- Status: in_progress. Registered 19 September 2026 22:15 UTC from `main` at
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
| 6 | The thresholds behave sensibly across the whole research corpus | `judge evidence-lint` over all 46 `docs/research/R-*.md` in one run (`artifacts/jev-judgment-harness/lint-all/`) | 46 of 46 answered by `jev-1.13.0`, 237,302 input tokens, 36.6 s total, 1.00 s slowest; 31 optional flags on 24 records: 22 `reproducible` (lowest 0.07, R-0031 retrospective), 6 `independent_check_named`, 2 `identity_stated`, 1 `falsifiable`, 2 `status_supported` overclaimed (R-0025 at 0.87, R-0026 at 0.83); R-0025's own text carries "accepted" in its status line and "review pending fresh focused re-review" at line 191, so that flag reads a real inconsistency; exit 0 throughout because every flag is optional | thresholds unchanged; the record-by-record reading is follow-up work, not this task's |
| 7 | The tooling suite passes with the new tests | `bootstrap`, `build --preset lab-debug`, `test --suite synthetic --preset lab-debug` in the worktree | all exit 0; 408 tooling tests (23 new) and 434 checks passed, none failed, skipped, missing or timed out; the report notes the source changed during the run because this record was being written, so the suite is rerun on the committed candidate below | rerun on the commit |
| 5 | curl's own timeout surfaces as the `timeout` outcome | stub server sleeping 3 s, `--timeout 1` | curl exit 28 with `000` was reported as `HTTP 0`, exit 1 | map curl exit 28 to `timeout` (exit 4); any other nonzero curl exit is `failed` with curl's stderr |

Artifacts: `artifacts/jev-judgment-harness/` in the worktree (ping, lint,
doctor, bootstrap, build and test reports and logs).

## Handoff

- Current base/head commit and uncommitted state: filled at checkpoint
- Verified findings: see Evidence
- Current hypothesis and failed approaches: none open
- Commands executed, outcomes and report hashes: filled at checkpoint
- Unavailable/skipped checks: hosted CI runs after the push
- Exact next experiment/command: review
- Remaining dependencies: none
- Runtime needs: network to `api.typesafe.ai` for the two real-endpoint
  criteria only; `curl`; the tooling suite needs the isolated toolchain and a
  `lab-debug` build for its native part
- Aggregate parent/child time, provider usage before/after: filled at checkpoint
- Accepted outcome, review/fix rounds and next routing decision: filled at closeout

## Review and integration

- Reviewer and independent reproduction/withheld-case results: pending
- Required changes or acceptance rationale: pending
- Exact merge candidate and required-check results: pending
- Integrated commit and evidence location: pending
- Remote synchronization: pending
- Scope still unverified: the follow-up question sets named under boundaries
