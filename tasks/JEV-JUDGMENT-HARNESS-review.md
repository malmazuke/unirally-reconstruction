# JEV-JUDGMENT-HARNESS - independent review

## Candidate

`b6e457b8069f0fbc312d8cd5dfc977cc9164c385` (implementation `ec64a17` plus the
checkpoint commit that edits only the task record), base `main` at `fd34209`.
Reviewed in the isolated detached checkout
`.worktrees/jev-judgment-harness-review`. No file of the implementation was
modified; the only write is this report.

## Reviewer

A fresh Claude Opus 5 subagent in an isolated checkout, spawned by the primary
with no inherited conversation and no implementation context. Everything below
comes from reading the committed tree and running the commands recorded here.

## Reproduction

All commands were run from the review worktree with the ignored `.env` present.
Reports and artifacts are under `artifacts/review/` in that worktree (ignored;
not committed). Exit codes were captured directly, not through a pipeline.

| # | Command | Outcome |
| --- | --- | --- |
| 1 | `python3 tools/project.py doctor --report artifacts/review/doctor.json` | exit 0, `status=passed`; `[ passed] typesafe_api_key (optional): from .env` |
| 2a | `python3 tools/project.py bootstrap --report artifacts/review/bootstrap.json` | exit 0, `status=passed`, `source=cache` |
| 2b | `python3 tools/project.py build --preset lab-debug --report artifacts/review/build.json` | exit 0, `status=passed`, 2.9 s |
| 2c | `python3 tools/project.py test --suite synthetic --preset lab-debug --artifacts artifacts/review/test --report artifacts/review/test.json` | exit 0, `status=passed`, 57.6 s; 434 checks passed, 0 failed, 0 skipped, 0 missing, 0 timeout; all 23 `py:test_judgment.*` checks passed; `source.commit = b6e457b8...`, `dirty = false`, `source_changed_during_run = false` |
| 3 | `python3 tools/project.py judge ping --out artifacts/review/ping --report artifacts/review/ping/report.json` | exit 0; `judgment:ping passed: jev-1.13.0 answered 2 questions in 0.79s (421 input tokens, attempt 1, curl (/usr/bin/curl))`; optional `ping_answers_as_expected passed` (noul 0.04, `unsupported`). `artifacts/review/ping/ping.json` carries `request`, `response`, `http_status`, `usage` (421/59), `elapsed_seconds` 0.785, `attempts`, `retries`, `transport`, `key_source` `.env`, `endpoint`, `model_requested` `jev-latest`, `model` `jev-1.13.0`, `state_sha256` `139ed5b6...` |
| 4 | `python3 tools/project.py judge evidence-lint --record docs/research/R-0008-track-decode.md --record docs/research/R-0019-dragster-loser-result.md --record docs/research/R-0037-dragster-race-palette-cycle.md --out artifacts/review/lint --report artifacts/review/lint/report.json` | exit 0, `status=passed`. Records chosen by the reviewer, none of the three the implementer used. 18 `lint:*` checks, every one `"required": false`; 3 of them `failed` (`reproducible` at noul 0.21, 0.21, 0.35) and the run still exits 0. Each record has its own artifact with all six answers, answered by `jev-1.13.0`, plus `evidence-lint.json` |
| 6 | `set -a; . ./.env; set +a; grep -rlF -- "$TYPESAFE_API_KEY" artifacts/ tools/ tests/ docs/ tasks/ AGENTS.md .env.example` | grep exit 1, zero files. A repeat over the whole worktree excluding `.git` and `.env` also found zero files |
| 7 | `git diff fd34209 -- .github/` | empty; the synthetic workflow is unchanged |

Key transport hygiene was checked directly, by spying on `procs.run_bounded`
from `judgment._curl_post`: argv is `['/usr/bin/curl', '-K',
'<tempdir>/curl.cfg']` with the key absent from argv, the config file is mode
`0o600` inside a `0o700` `mkdtemp` directory, the key is only inside that file,
and the directory is removed when the call returns. `write_judgment` scans the
serialized record for the key before writing.

CI reasoning for item 7: no workflow file changed; `judge` has no CI step; the
only new check reaching CI is `doctor`'s `typesafe_api_key`, which is
`required=False`, and `_status_from_checks` in `lab_commands.py:35` counts only
required failures, so a keyless runner still reports `status=passed`. The new
tests reach CI through `test --suite synthetic`; they bind a stub server on
127.0.0.1 and redirect the client with `UNIRALLY_LAB_JUDGE_ENDPOINT`, so no key
and no external network are needed. Grepping the tree confirms nothing except
`doctor` and the tests imports `judgment`, and no gate, CMake target or workflow
invokes `judge`, so no judgment can enter a required check of any other command.

## Findings

### Required correction 1 - duplicate record stems silently overwrite an evidence artifact and misreport it

`tools/unirally_lab/judge_commands.py:208` derives the artifact name from
`path.stem`, and `:220` records `"artifact": f"{name}.json"` per record path.
Two records with the same stem in different directories produce one artifact
file: the second call overwrites the first, while the report gains two checks
named `judgment:R-0001` and two sets of `lint:R-0001:*`, and
`evidence-lint.json` maps both distinct record paths to the same surviving
file. A reader of the report is told each record has its own recorded call when
only the last one exists. That contradicts this task's own claim of "an evidence
artifact per call" and D-0007's "every call is evidence".

Reproduced with the stub endpoint and two synthetic records, `dupA/R-0001.md`
and `dupB/R-0001.md`: exit 0, `status=passed`, report checks
`judgment:R-0001` twice and each `lint:R-0001:*` twice, output tree
`['out/R-0001.json', 'out/evidence-lint.json', 'report.json']` for two records.

Resolution: make the artifact name unique per record (for example the record's
path relative to the root with separators replaced, or a stem plus a short hash
of the path), and either disambiguate the check names the same way or refuse
duplicate stems as invalid input. Not currently reachable from
`docs/research/`, where every stem is unique, but it is reachable from any
`--record` list a caller writes.

### Required correction 2 - `--name` is unvalidated and writes the artifact outside `--out`

`tools/unirally_lab/judge_commands.py:255` accepts any `--name`, and `:91`
joins it straight into `out / f"{name}.json"`, with
`judgment.write_judgment` creating parent directories as needed
(`judgment.py:313`). A name containing `..` or a separator escapes the
directory the caller named and the report points at the escaped path.

Reproduced against the stub with `judge ask ... --name "../../escaped"`: exit 0,
`status=passed`, check `judgment:../../escaped passed`, nothing under the
`--out` directory, and `escaped.json` created two levels above it (verified at
`$TMPDIR/escaped.json`, then deleted). The same `--name` also lands in a check
name, so the report's check identifier carries path syntax.

Resolution: reject a `--name` that is not a simple file name (no separators, no
`..`, non-empty), as invalid input before the request is sent.

### Should fix 1 - the key-leak guard, and any non-`JudgmentError`, escapes `_evaluate` and kills the command without a report

`tools/unirally_lab/judge_commands.py:91` calls `judgment.write_judgment`
outside the `try/except judgment.JudgmentError` at `:85-90`. The guard at
`judgment.py:310-311` raises `JudgmentError("failed", "refusing to write an
artifact that contains the API key")`, which therefore propagates through
`cmd_ask`/`cmd_ping`/`cmd_evidence_lint` to `project.py:192`, which has no
handler. The process dies with a traceback, writes no report, and exits 1 only
because that is Python's default.

Reproduced against the stub with `judge ask --state <file containing the key>`:
`RC=1`, `unirally_lab.judgment.JudgmentError: refusing to write an artifact that
contains the API key` on stderr, `REPORT EXISTS: False`, empty output tree. The
protective half works (no artifact is written); the reporting half does not.
An `OSError` from `write_judgment` or from `_out_dir`'s `mkdir` behaves the
same way.

Resolution: move `write_judgment` inside the guarded block and map
`JudgmentError` and `OSError` to a failed `judgment:<name>` check, so the run
still produces the report the project treats as its evidence surface.

### Should fix 2 - a structurally valid but non-object answer crashes `judge ping` after the artifact is written

`judgment.evaluate` checks that `answers` is an object and that every question
id is present (`judgment.py:293-298`) but not that each answer is itself an
object. `judge_commands.py:127-128` then does
`answers["check_named"].get("noul")`.

Reproduced with a stub returning `{"answers": {"check_named": "not-a-dict",
"status_supported": "not-a-dict"}}`: `AttributeError: 'str' object has no
attribute 'get'` at `judge_commands.py:127`, `RC=1`, no report written, while
`out/ping.json` had already been written. `judgment.evidence_flags`
(`judgment.py:422-424`) has the same shape: `answers.get(qid) or {}` keeps a
truthy non-object and then calls `.get` on it, so `evidence-lint` fails the same
way.

Resolution: validate in `evaluate` that each answer is an object (raising
`JudgmentError("failed", ...)`), or read answers defensively in both callers.

### Should fix 3 - `docs/BUILD_AND_VALIDATION.md:179` breaks its own table row

The `judge ask` row writes `` `judge ask --state <json|-> --questions <json|->`
`` with unescaped `|` inside the cell. GitHub-flavored Markdown splits table
cells on `|` even inside backticks, so that row renders with six cells against
the table's four, shifting the status and description columns. The neighbouring
`frontend run` row at `:177` already escapes this correctly as
`dragster\|zoom-zoo`. Verified by counting `|` per row: every other row in the
block has 4, line 179 has 6.

Resolution: escape both as `\|`, matching line 177.

### Should fix 4 - `--timeout` and `--attempts` are not validated, and a fractional timeout is truncated

`judge_commands.py:239-240` accepts any float timeout and any int attempts.
`--attempts 0` reaches `judgment.evaluate`, which reports `judgment:ask failed:
attempts must be at least 1` and exits 1; a bad argument should be invalid
input (exit 3) like the other argument errors in these commands. `--timeout -5`
is worse: `judgment.py:143` writes `max-time = -5` into the curl config and the
run fails with the opaque detail `curl exit 26: 7: 'max-time' expected a
positive numerical parameter / curl: cannot read config from
/var/folders/...`, which also leaks the temporary config path into the report.
Both reproduced at the real endpoint (exit 1 in each case). Separately,
`int(timeout) or 1` truncates: `--timeout 0.5` asks curl for 1 second, twice the
requested bound, although curl accepts fractional `--max-time`.

Resolution: reject `--attempts < 1` and `--timeout <= 0` as invalid input in
the command layer, and pass the timeout to curl without truncating it.

### Advisory 1 - D-0007's "never required" wording does not match the required `judgment:<name>` check

`docs/decisions/D-0007-advisory-jev-judgments.md:27` says a judgment "is
recorded as an optional check (never `required`)". In the implementation the
answer-derived checks are indeed optional (`ping_answers_as_expected`,
`lint:*`, both confirmed `"required": false`), but the call-outcome check
`judgment:<name>` is required (`judge_commands.py:89` and `:94`), which is what
makes `judge ping` exit 1 on a 401. That is the right behaviour for a
standalone diagnostic command, and no other command or gate consumes it, so
nothing is gated by a judgment. The decision text would be more accurate if it
distinguished the transport/call outcome, required within `judge` only, from
the judgment itself, which is always optional.

### Advisory 2 - curl's stderr reaches the report unfiltered, and the report is not key-scanned

`judgment.py:157` puts `result.stderr[-300:]` into the `JudgmentError` detail,
which becomes a report check detail. `write_judgment` scans the artifact for
the key but nothing scans the report. Curl does echo config-file context on a
parse error: the `--timeout -5` run above produced `7: 'max-time' expected a
positive numerical parameter` plus the config path in the report. The key line
is not among the lines curl quoted in any case observed, and I could not
construct a leak, but the guard that exists for artifacts does not exist for
reports. Applying the same `key in text` refusal (or redaction) to the report
detail would close the asymmetry cheaply.

### Advisory 3 - a key containing a double quote is silently truncated

`judgment.py:137` interpolates the key into `header = "Authorization: Bearer
{key}"` in the curl config. With a key of `ts-fake"SECRETTAIL` the stub server
received `Bearer ts-fake`, and the run reported success against a permissive
stub; against the real endpoint it would be an unexplained 401. An interior
newline truncates similarly (stub saw `Bearer ts-part1`). Real TypeSafe keys
are alphanumeric so this is theoretical, but rejecting a key containing `"`,
`\` or a newline would turn a confusing failure into a clear one.

### Advisory 4 - retry policy

Retries cover only 429 and 529, with `min(0.5 * 2**(attempt-1), 4.0)` backoff
and no `Retry-After` handling; 5xx and transport-level failures are not
retried. This is bounded and matches the documentation, so it is a note rather
than a defect: worst-case wall clock is `attempts * (timeout + 15)` plus at
most 1.5 s of sleeps, all bounded. In `evaluate` the final attempt still
appends to `retries` before the loop ends, so the `attempts` field would be one
too high; it only affects the exhausted-retries path, which raises, so nothing
observable depends on it.

### Advisory 5 - task record and registry consistency

The acceptance table and the handoff are accurate against what I reproduced:
status, check counts, the versioned model, the artifact fields, optionality of
every `lint:*` check and the zero-hit key grep all match. Three smaller points:
the Evidence table lists attempt 6 before attempt 5, which reads as a
transcription slip; `tasks/README.md:55` says "in progress" while the record's
Status line says "review"; and the recorded report SHA-256 prefixes (for
example `doctor` `377db1aebc2e0fc7`) refer to ignored artifacts in the
implementer's own worktree, so they cannot be verified from what is committed.
I reproduced the outcomes rather than the hashes. Nothing claimed as done was
found undone; the CI criterion is correctly left as pending rather than
preclaimed.

### Advisory 6 - what gets sent to a third party

`evidence-lint` sends the full text of each named record, and `judge ask` sends
whatever state file the caller passes, to `api.typesafe.ai`. For tracked
`docs/research/` records in a public repository this is unremarkable, but
nothing in the code or in D-0007 discourages pointing these commands at a file
under `local/`, where ROM-derived captures live. One sentence in D-0007 or in
the "Advisory judgments" section would settle it.

### Scope, style and readability

The diff touches only owned paths: `.env.example`, `AGENTS.md` (one bullet),
`docs/BUILD_AND_VALIDATION.md` (three rows and one section),
`docs/decisions/D-0007-*`, `tasks/JEV-JUDGMENT-HARNESS.md`, `tasks/README.md`
(one row), `tests/tooling/test_judgment.py`, `tools/project.py` (two lines),
`tools/unirally_lab/judge_commands.py`, `tools/unirally_lab/judgment.py`,
`tools/unirally_lab/lab_commands.py` (one check). Nothing under `src/`, no
gate, no manifest, no workflow. No en dash or em dash appears in any new or
changed Markdown line. Names, units and the module docstrings are clear, and
the tests assert real behaviour rather than restating it: I read each assertion
and they check exit codes, check names, the `required` flag, the bearer header
the stub actually received, the request body's state and questions, artifact
contents and the absence of the key in both artifact and report. The CLI tests
exercise the real curl transport against a loopback stub rather than a fake, so
the transport path is genuinely covered. Coverage gaps worth noting: no test
covers `-` on stdin, the both-stdin rejection, `--name`, duplicate record
stems, or a non-object answer.

## Withheld cases

Designed by the reviewer, not present in the task record.

1. `judge ask` at the real endpoint with a reviewer-written state (a handoff
   sentence that reports a skipped gate as a pass) and a two-question file
   mixing a `noul` with a `score` carrying an ordered criteria list. Exit 0;
   `jev-1.13.0` returned `overclaims` noul 0.46 and `severity` score 1.61 with
   a legend and probabilities. This is the only exercise of the `score` type
   end to end; it works.
2. Same state and questions with `--state -` on stdin. Exit 0, identical
   answers, artifact written under `--name stdin-ask`.
3. `--state -` together with `--questions -`. Exit 3, check `arguments failed:
   only one of --state and --questions may read stdin`, no request sent.
4. A malformed questions file that is valid JSON but gives a `choice` a list of
   criteria. Exit 3, `questions failed: q: a choice needs a criteria object of
   options`, no request sent.
5. `TYPESAFE_API_KEY=invalid-key-for-review python3 tools/project.py judge ping
   ...` against the real endpoint. Exit 1, `typesafe_api_key passed: from
   environment`, `judgment:ping failed: 401: the API key was rejected`, report
   written, no key anywhere in it. Environment precedence over `.env` confirmed
   at the same time.
6. `--attempts 0` and `--timeout -5` at the real endpoint (see Should fix 4).
7. Stub-server cases for the crash and naming paths: non-object answers,
   `--name ../../escaped`, duplicate record stems, a state containing the key,
   a key containing a double quote and a key containing a newline (see the
   findings above).
8. `judge` with no subcommand exits 3; `judge --help` and `judge ask --help`
   exit 0 with usable usage text.

## Verdict

return - the command contract, the key hygiene and the advisory-only policy all
hold up under independent reproduction, but two reproducible defects in new code
let the artifact path be taken from unvalidated input, so a run can silently
overwrite one record's evidence or write it outside `--out` while the report
claims otherwise.
