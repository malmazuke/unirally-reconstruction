# CI-FAST-PATH independent review

## Candidate

`555a62bab9ebf05831089625010eeb11c16fedc4` (branch `task/ci-fast-path`; commits
`0c0e7a1` workflow, classifier and docs, `628f7f9` test, `2ccef4e` and
`555a62b` record only). Base for the diff: `b72b77d`.

## Reviewer

A fresh Claude Opus 5 subagent with no conversation history with the
implementer, working in the isolated detached checkout
`.worktrees/ci-fast-path-review` at the candidate commit. Nothing outside this
checkout was touched; nothing was pushed, re-run, cancelled or dispatched. The
only write to the tracked tree is this report and its commit on
`review/ci-fast-path`.

## Reproduction

All commands were run from
`/Users/markfeaver/Projects/Unirally Decompilation/.worktrees/ci-fast-path-review`.

### 1. Local suite at the candidate

| Command | Outcome | Report |
| --- | --- | --- |
| `python3 tools/project.py bootstrap --report artifacts/review/bootstrap.json` | `status=passed elapsed=1.33s` | `artifacts/review/bootstrap.json` |
| `python3 tools/project.py build --preset lab-debug --report artifacts/review/build.json` | `status=passed elapsed=2.726s` | `artifacts/review/build.json` |
| `python3 tools/project.py test --suite synthetic --preset lab-debug --artifacts artifacts/review/test --report artifacts/review/test.json` | `status=passed elapsed=56.349s`, 455 checks, `{"failed":0,"missing":0,"passed":455,"skipped":0,"timeout":0}` | `artifacts/review/test.json` |

The ten `py:test_ci_fast_path.*` checks are present and every one has
`outcome: passed` (7 `ClassifierTests`, 3 `WorkflowTests`). `python_tooling_tests`
is `passed` with `429 tests run, 429 records, 0 failed; runner exit 0`. The
report records `source.commit` and `source_at_finish.commit` as
`555a62ba...`, `dirty: false`, `untracked_files: []` and
`source_changed_during_run: false`, so the source was clean at the candidate
and unchanged during the run. `git status --short` was empty before and after.

### 2. Classifier against real history in this checkout

Form used: `GITHUB_EVENT_NAME=<event> CLASSIFY_BASE=<sha> CLASSIFY_HEAD=<sha>
CLASSIFY_OUT=artifacts/review/changes-<n>.json python3
.github/scripts/classify_changes.py`.

| n | Event | Base | Head | Printed decision |
| --- | --- | --- | --- | --- |
| 1 | push | `b72b77d` | `555a62b` | `docs_only=false: 3 non-documentation path(s), first .github/scripts/classify_changes.py` |
| 2 | push | `2ccef4e` | `555a62b` | `docs_only=true: all 1 changed path(s) are documentation` |
| 3 | push | 40 zeros | `555a62b` | `docs_only=false: no base commit (new branch or empty before sha)` |
| 4 | push | `deadbeef...` | `555a62b` | `docs_only=false: base commit is not in the checkout` |
| 5 | push | `555a62b` | `555a62b` | `docs_only=false: no changed paths; full path by default` |
| 6 | push | empty | `555a62b` | `docs_only=false: no base commit (new branch or empty before sha)` |
| 7 | workflow_dispatch | `b72b77d` | `555a62b` | `docs_only=false: event workflow_dispatch always takes the full path` |
| 8 | pull_request | `b72b77d` | `555a62b` | `docs_only=false: 3 non-documentation path(s), first .github/scripts/classify_changes.py` |
| 9 | push | `0c0e7a1` | `555a62b` | `docs_only=false: 1 non-documentation path(s), first tests/tooling/test_ci_fast_path.py` |
| 10 | push | `628f7f9` | `555a62b` | `docs_only=true: all 1 changed path(s) are documentation` |

Case 1 lists `.github/scripts/classify_changes.py`,
`.github/workflows/synthetic.yml`, `docs/BUILD_AND_VALIDATION.md`,
`tasks/CI-FAST-PATH.md`, `tests/tooling/test_ci_fast_path.py`. Every one of
these ten matches the behaviour the record and the docs paragraph claim.

### 3. Hosted runs (read only)

`gh run view <id> --json jobs,conclusion,createdAt,updatedAt` and
`gh run download <id> -n changes -D artifacts/review/run-<id>`.

| Run | Head | Conclusion | Wall clock | Classification artifact |
| --- | --- | --- | --- | --- |
| 35481483430 | `0c0e7a1` | cancelled | 01:28:41 to 01:29:28 | `base` all zeros, `docs_only=false`, `no base commit (new branch or empty before sha)` |
| 35481505083 | `628f7f9` | success | 01:29:11 to 01:32:44 (3.55 min) | `base 0c0e7a1`, `docs_only=false`, `1 non-documentation path(s), first tests/tooling/test_ci_fast_path.py`, `merge_base 0c0e7a1` |
| 35481706436 | `2ccef4e` | success | 01:33:43 to 01:34:06 (23 s) | `base 628f7f9`, `docs_only=true`, `all 1 changed path(s) are documentation` |

Step outcomes match the claims. In 35481706436 both `lab` jobs ran only
`checkout`, `Docs-only fast path` and `Upload reports`; all eleven cache,
doctor, bootstrap, build, test and `--help` steps are `skipped`, and the
uploaded `reports-macos-15` artifact contains
`artifacts/ci/fast-path.json` with `"fast_path": "docs-only"` and
`"commit": "2ccef4e..."`. In 35481505083 every build and test step ran on both
jobs and `Docs-only fast path` is `skipped`. In 35481483430 the `changes` job
succeeded and both `lab` jobs were cancelled mid-build, with `Upload reports`
still succeeding under `if: always()`.

The four `reports-*` test reports from 35481505083 confirm the tooling-test
claim: `test-debug.json` has `python_tooling_tests` `passed`, `required: true`,
`429 tests run`, 429 `py:` checks, 23 `ctest:` checks and
`native_fresh_process_repeatability` `passed`; `test-app-debug.json`,
`test-sanitize.json` and `test-app-sanitize.json` each have
`python_tooling_tests` `skipped` with `required: false` and detail
`disabled by --no-python-tests`, 0 `py:` checks, 23 `ctest:` checks and the
repeatability probe `passed`. Per-step timings from
`gh api .../actions/runs/35481505083/jobs` match the record: macOS lab-debug
test 100 s, macOS app-debug test 1 s, macOS SDL build 53 s, Linux sanitizer
step 61 s.

`gh run list --branch task/ci-fast-path` shows a fourth run the record does not
name: 35481755910 at the candidate `555a62b`, push, success, 01:34:49 to
01:35:09 (20 s), fast path with every build and test step skipped.

### 4. Source inspection

`tools/unirally_lab/lab_commands.py:333` records
`python_tooling_tests` as `skipped` with `required=False` when
`--no-python-tests` is passed and leaves the ctest and repeatability sections
untouched, so the report contract in the docs paragraph is accurate. No
tooling test takes its preset from the invoking run (`tests/tooling/*.py` pass
`lab-debug` or `lab-failure-probe` explicitly), so running them once per job is
not a loss of coverage.

## Findings

### Required correction 1 - a rename into `docs/` or `tasks/` hides the deleted source path

`.github/scripts/classify_changes.py:61` runs
`git diff --name-only <merge_base> <head>`. Rename detection is on by default,
and with `--name-only` git prints only the destination path of a detected
rename. A commit that moves a build or test input into `docs/` or `tasks/` is
therefore classified as documentation, and the full path that would have caught
the removal is skipped.

Reproduced in a throwaway repository
(`$SCRATCH/tw`, case W3): `git mv tools/a.py docs/b.md`, commit, then
`GITHUB_EVENT_NAME=push CLASSIFY_BASE=<parent> CLASSIFY_HEAD=<head>` gives
`docs_only=true: all 1 changed path(s) are documentation`, with
`changed: ["docs/b.md"]`. Confirmed at the git level:
`git diff --name-only <parent> <head>` prints `docs/b.md` alone, while
`git diff --name-only --no-renames <parent> <head>` prints both `docs/b.md` and
`tools/a.py`, and `--name-status` prints `R100 tools/a.py docs/b.md`. The
mirror case W2 (`git mv docs/a.md tools/a.py`) is classified full path only
because the destination happens to be code.

Resolution: pass `--no-renames` (or `-M0`) to the `git diff` call, and add a
classifier test that renames a tracked source file into `docs/` and asserts the
full path.

### Required correction 2 - tracked non-prose data under `docs/` is classified as documentation although the suite validates it

`.github/scripts/classify_changes.py:31-35` treats every path under `docs/` as
documentation. `docs/map/` holds eight tracked files that are not prose:
`boot-start-600.map.json` and three other `*.map.json` maps with their `*.md`
summaries. `tests/tooling/test_coverage.py:30` and
`tests/tooling/test_coverage.py:485-504` (`TrackedMapTests`) are the only check
on them, and that check is not cosmetic: besides the internal totals and the
summary cross-reference it asserts that no tracked map contains `"opcode"`,
`"mnemonic"` or `"bytes_hex"`, which is the guard for the AGENTS.md rule that
extracted game content stays out of tracked files. A commit that edits a map or
its summary is `docs_only`, so that guard never runs and the tip is green.

Reproduced in two parts. Classification: throwaway case W1, editing
`docs/map/m.map.json` alone gives
`docs_only=true: all 1 changed path(s) are documentation`. Sensitivity: a copy
of `tools/`, `docs/` and `tests/` in the scratch directory with
`docs/map/boot-start-600.map.json` perturbed by one `opcode_bytes` count and a
stray `"opcode"` key makes
`python3 -m unittest tests.tooling.test_coverage.TrackedMapTests` fail with
`AssertionError: 2086 != 2085`. Under the candidate workflow that failure never
reaches CI.

Resolution: define documentation by extension rather than by directory, for
example `docs/**` and `tasks/**` limited to `*.md` plus `docs/images/*`, with
`.env.example` and root `*.md` unchanged; everything else under `docs/`,
including `docs/map/*.map.json`, then takes the full path. That also removes
the hypothetical exposure from withheld cases W6, W7 and W8 below.

### Required correction 3 - the documented guarantee that a fast-path run "is still the tip's green run" does not hold after a cancelled or failed run

`docs/BUILD_AND_VALIDATION.md:351-353` states "That run is still the tip's green
run for the acceptance rule: the tree it covers differs from the last full run
only in documentation." The classifier compares against
`github.event.before`, which is the previously pushed tip, not the last tip
whose run actually completed successfully. With
`.github/workflows/synthetic.yml:10-12` (`cancel-in-progress: true`), a code
push whose run is cancelled or fails, followed by a records-only push, produces
a green tip for code that no completed run ever built or tested. That is the
"masked skip" the reviewer checklist asks about, and it directly weakens the
acceptance evidence that `docs/AGENT_WORKFLOW.md` ("wait for that tip's CI",
"never preclaim a future pass") depends on.

Reproduced from this branch's own history rather than a constructed case: run
35481483430 at `0c0e7a1` was cancelled 47 s in, mid `Build lab-debug` on macOS
and mid `Test synthetic (lab-debug)` on Linux, because the next push arrived
30 s later. Had that next push been records-only instead of `628f7f9`, the
classifier would have reported `docs_only=true` for `before=0c0e7a1` (verified:
case 2 and case 10 above show exactly this shape) and the tip would have been
green with the new workflow and classifier never having completed a build. The
same holds for a red run followed by a records commit, which is the ordinary
rhythm of this project's task branches.

Resolution: either make the fast path conditional on the base commit having a
completed successful `synthetic` run (a `gh api` lookup in the `changes` job,
falling back to the full path when it is absent, cancelled or failed), or
correct the documentation and the task record to state the actual guarantee and
the coordinator's obligation to confirm the base's run completed before citing
a fast-path run as the tip's evidence.

### Should fix 1 - the classifier is read from the tree it classifies

`.github/workflows/synthetic.yml:27-36` checks out the pushed head and runs
`.github/scripts/classify_changes.py` from that checkout. The rule that a change
to the classifier or the workflow takes the full path is therefore enforced by
the changed classifier itself. A commit that widens `DOC_PREFIXES` (for example
by adding `.github/`) is judged by its own new rule, so the very push that
disables the guard is the one whose full path is skipped; the docs paragraph
presents that guard as unconditional. No exploit is needed for this to bite, a
mistaken edit is enough.

Resolution: run the base revision's classifier, for example
`git show "$CLASSIFY_BASE:.github/scripts/classify_changes.py" > /tmp/classify.py`
with a fall back to the full path when that fails, and assert the shape in
`WorkflowTests`.

### Should fix 2 - the acceptance table's "Nothing else changed" row is contradicted by the candidate

`tasks/CI-FAST-PATH.md:89` gives `git diff main -- tools tests src` with the
expected result `empty`. Run in this checkout, `git diff main --name-only --
tools tests src` prints `tests/tooling/test_ci_fast_path.py`. The record's own
"Owned paths" bullet explains why (the boundary moved at start so the
classifier is testable in the suite), but the acceptance row was not updated, so
the table states a criterion the candidate does not meet.

Resolution: change the row to `git diff main -- tools src` with the expected
result `empty`, plus `git diff main --name-only -- tests` equal to
`tests/tooling/test_ci_fast_path.py`.

### Advisory 1 - the classifier docstring overstates its failure behaviour

`.github/scripts/classify_changes.py:15` says "The decision never fails the
job; an error takes the full path and says why". That is true for git failures,
which `_git` converts to `None`, but not for an unexpected exception: an
unwritable `CLASSIFY_OUT`, a missing `GITHUB_OUTPUT` directory or a missing
`git` binary raises and exits non zero. The resulting behaviour is safe, which I
checked separately: a failing `changes` job leaves `lab` skipped and the run
concluded `failure`, never a silent success, and an empty `docs_only` output
would still satisfy `!= 'true'` on every guarded step. Only the sentence is
wrong.

### Advisory 2 - the command inventory row was not updated

`docs/BUILD_AND_VALIDATION.md:160` still describes `test --suite synthetic` as
running "Python tooling tests, ctest, fresh-process repeatability" with no
mention of `--no-python-tests`, although `tasks/CI-FAST-PATH.md:31` lists
that row among the owned paths. The new paragraph at line 345 covers the flag,
so this is presentation only.

### Advisory 3 - the workflow test recognises heavy steps by substring

`tests/tooling/test_ci_fast_path.py:138` selects guarded steps by the presence
of `tools/project.py`, `apt-get`, `--help` or `actions/cache`. A future heavy
step that matches none of those (a `setup-python`, an upload of a build tree, a
new tool invocation) would not be required to carry the
`needs.changes.outputs.docs_only != 'true'` guard and the test would still pass.
The three-line window built at line 141 is also fragile if a step's `env:` block
moves the `if:` further down. Consider asserting that every step in `lab` other
than the checkout, the fast-path step and the `if: always()` uploads carries the
guard.

### Advisory 4 - the `changes` job serialises ahead of every full run

`.github/workflows/synthetic.yml:47` makes `lab` wait for `changes`. On run
35481505083 that cost about 27 s of wall clock before the first `lab` job
started (run created 01:29:11, `changes` completed 01:29:36, `lab` started
01:29:38 and 01:29:43). The net effect is still strongly positive (about 152 s
saved on the two jobs by `--no-python-tests`), but the record's "3.55 min"
figure includes this new fixed cost and it is worth naming when the timing is
cited later.

### Advisory 5 - the candidate's own run is not in the record

`tasks/CI-FAST-PATH.md:130-133` lists runs 35481483430, 35481505083 and
35481706436. The candidate `555a62b` also has run 35481755910 (fast path,
success, 20 s). That is unavoidable for a record commit, but the closeout
handoff should name the tip's actual run.

## Withheld cases

Run against `.github/scripts/classify_changes.py` in a throwaway git repository
in the scratch directory, with `docs/`, `docs/map/`, `docsrc/`, `tasks/`,
`tools/`, `src/`, a root `README.md`, `.gitignore` and `CMakeLists.txt`.

| Case | Change | Decision | Verdict |
| --- | --- | --- | --- |
| W1 | edit `docs/map/m.map.json` only | `docs_only=true` | wrong, see Required correction 2 |
| W2 | `git mv docs/a.md tools/a.py` | `docs_only=false`, first `tools/a.py` | correct, but only because the destination is code |
| W3 | `git mv tools/a.py docs/b.md` | `docs_only=true`, `changed: ["docs/b.md"]` | wrong, see Required correction 1 |
| W4 | add `docsrc/z.py` (prefix trap) | `docs_only=false` | correct; the trailing slash in `DOC_PREFIXES` is right |
| W5 | edit `.gitignore` only | `docs_only=false` | correct |
| W6 | add executable `tasks/run.sh` | `docs_only=true` | latent; nothing executable is tracked under `tasks/` today |
| W7 | add symlink `docs/link -> ../tools` | `docs_only=true` | latent; no symlink is tracked in the repository (`git ls-files -s` has no mode `120000` entry) |
| W8 | symlink `tools/gen.py -> ../docs/gen.py`, then edit `docs/gen.py` only | `docs_only=true` | latent; executed code would change with no full run |
| W9 | merge commit on the branch bringing `src/n.cpp` | `docs_only=false`, first `src/n.cpp` | correct; the two-argument diff sees the net tree change |
| W10 | `pull_request` whose base moved ahead with code after the branch point | `docs_only=true`, `merge_base` at the branch point | correct; the merge base is the right comparison |
| W11 | push whose `before` is newer than HEAD (a rewind force push) | `docs_only=false: base is not an ancestor of HEAD (force push)` | correct |
| W12 | mode change only on `docs/t.md` | `docs_only=true` | correct |

Two cases from the brief are not representable and were not exercised: git
normalises paths, so a `docs/../tools/x.py` entry cannot appear in
`diff --name-only` output, and git refuses a tree holding both a blob and a
tree under the same name, so a root Markdown file cannot coexist with a
directory of the same name.

Workflow expressions reasoned through and checked against the hosted runs:
`github.event.before || github.event.pull_request.base.sha` resolves to the
push `before` on push, to the PR base on `pull_request` where `before` is
undefined and falsy, and to an empty string on `workflow_dispatch`, which the
event guard rejects before the empty base ever matters. `needs: changes` with a
failing, timed-out or cancelled `changes` job leaves `lab` skipped and the run
not successful, so there is no silent green; an empty `docs_only` output
satisfies `!= 'true'` and takes the full path. `if: always()` on the
classification upload is right and demonstrably useful: it produced the
`changes` artifact for the cancelled run 35481483430, and `if-no-files-found:
warn` keeps a missing file from failing the job. The fast-path step masks
nothing on its own, since it runs only under `docs_only == 'true'` and a
`GITHUB_SHA` lookup failure would fail the job. The `--no-python-tests` steps
still run ctest and the repeatability probe, and the skipped optional check is
recorded, which I verified both in `tools/unirally_lab/lab_commands.py:333` and
in the four uploaded reports.

Scope discipline: the diff against `b72b77d` touches
`.github/scripts/classify_changes.py`, `.github/workflows/synthetic.yml`,
`docs/BUILD_AND_VALIDATION.md`, `tasks/CI-FAST-PATH.md` and
`tests/tooling/test_ci_fast_path.py`, all within the record's owned paths; the
new documentation sits inside the "CI and release evidence" section
(`docs/BUILD_AND_VALIDATION.md:293`). `tools/`, `src/` and the report schema are
untouched. New Markdown, Python and YAML contain no en or em dashes and no non
ASCII characters.

## Verdict

return - the classifier silently accepts two classes of change that alter build
or test behaviour (a rename into `docs/` or `tasks/`, and the tracked
`docs/map/*.map.json` data the suite validates), and the documented guarantee
that a fast-path run is still the tip's green run does not hold after a
cancelled or failed run, which is exactly the sequence this branch already
produced.
