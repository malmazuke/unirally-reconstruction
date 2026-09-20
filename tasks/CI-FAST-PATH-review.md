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

## Re-review at afb8914

Candidate `afb89141865ef7c7bcf8ca106282c19a3b09b7a2` on `task/ci-fast-path`
(`a92afb6` the round-1 report, `139261d` the corrections, `89edda0` and
`afb8914` record only). Same reviewer and same isolated checkout, moved with
`git fetch origin task/ci-fast-path && git checkout --detach afb8914`. Nothing
was pushed, re-run, cancelled or dispatched; the only tracked write remains
this file.

### Reproduction

`python3 tools/project.py test --suite synthetic --preset lab-debug
--artifacts artifacts/review2/test --report artifacts/review2/test.json`
gives `status=passed elapsed=46.927s`, 458 checks,
`{"failed":0,"missing":0,"passed":458,"skipped":0,"timeout":0}`, with
`source.commit` and `source_at_finish.commit` both `afb89141...`,
`dirty: false` and `source_changed_during_run: false`. All 13
`py:test_ci_fast_path.*` checks passed, including the three new
`test_rename_out_of_code_takes_full_path`,
`test_data_under_docs_is_not_documentation` and `test_base_run_check` and the
replaced `WorkflowTests.test_every_lab_step_is_accounted_for`.

### Per finding

**Required correction 1 (rename hides the deleted source path) - resolved.**
`.github/scripts/classify_changes.py:97` now reads
`_git("diff", "--name-only", "--no-renames", merge_base, head)`. Re-running my
round-1 case W3 in a fresh throwaway repository, `git mv tools/a.py docs/b.md`
now gives `docs_only=false: 1 non-documentation path(s), first tools/a.py`
where it previously gave `docs_only=true` with `changed: ["docs/b.md"]`. The
mirror case W2 is unchanged. The new test asserts the exact
`["docs/moved.md", "tools/x.py"]` listing.

**Required correction 2 (tracked data under `docs/`) - resolved as raised.**
`is_documentation` at `.github/scripts/classify_changes.py:41-46` now requires
the `.md` extension for anything under `docs/` or `tasks/` and at the root.
Re-running case W1, editing `docs/map/m.map.json` alone gives
`docs_only=false: 1 non-documentation path(s), first docs/map/m.map.json`.
I also confirmed `docs/figure.png`, `tasks/data.csv`, `docs/README`
(extensionless), a root `LICENSE` and `notes/a.md` all take the full path, and
that `.env.example` and `docs/a.md` still do not. The `docs/map/*.md`
summaries are a residual; see new finding "Should fix A".

**Required correction 3 (green tip after a cancelled or failed run) -
resolved.** `base_has_successful_run` at
`.github/scripts/classify_changes.py:49-70` is consulted only after the paths
are all documentation, and every way the check can fail takes the full path. I
exercised it with a stub `gh` on `PATH` in a throwaway repository:

| Case | Stub | Result |
| --- | --- | --- |
| G1 | `echo 2` | `docs_only=true: ...; 2 successful completed run(s) of synthetic.yml on base <sha>` |
| G2 | `echo 0` | `docs_only=false: ... are documentation, but 0 successful completed run(s) ...` |
| G3 | `exit 1` | `docs_only=false: ... but gh api failed: boom` |
| G4 | no `gh` on PATH | `docs_only=false: ... but GITHUB_REPOSITORY or gh unavailable ...` |
| G5 | `GITHUB_REPOSITORY` empty | same as G4 |
| G6 | `echo not-a-number` | `docs_only=false: ... but unparseable gh api output: not-a-number` |
| G7 | code push, stub present | `docs_only=false: 1 non-documentation path(s) ...`, and the stub was never invoked |

The recorded query is
`repos/<repo>/actions/workflows/synthetic.yml/runs?head_sha=<merge_base>&per_page=50`
with a jq selecting `status == "completed" and .conclusion == "success"`, which
is the right shape. Hosted confirmation: run 35482466386 (push of `89edda0`)
took the fast path in 23 s with the `changes` artifact reading
`all 1 changed path(s) are documentation; 1 successful completed run(s) of
synthetic.yml on base 139261d` and a separate `base_run` field, and both `lab`
jobs show 11 skipped steps and 3 successful ones. Run 35482509998 (the
candidate `afb8914`) is the same, gated on `89edda0`. The documentation at
`docs/BUILD_AND_VALIDATION.md:345-366` now states the induction rather than the
bare claim, and the induction is anchored for this branch: `89edda0` rests on
`139261d`, whose run 35482286606 was a full 197 s run with every build and test
step executed.

**Should fix 1 (a commit judged by the classifier it introduces) - resolved.**
`.github/workflows/synthetic.yml:37-49` copies
`git show "$CLASSIFY_BASE:.github/scripts/classify_changes.py"` into
`RUNNER_TEMP` and runs that, falling back to the tree only when the base has
no such file. Verified on the hosted side: run 35482286606's `changes`
artifact has no `base_run` field and its reason is the pre-correction wording,
which is exactly what the base `555a62b` copy would produce, and the job log
line is `classifier taken from base 555a62b`. I reproduced the shell selection
locally against real blobs from `555a62b` and `afb8914`. I could not construct
a push where the base copy is weaker than the head copy without the classifier
itself appearing in the diff and forcing the full path, so the remedy does not
reopen Required correction 3 for push events; for pull requests see new
finding "Advisory B".

**Should fix 2 (the "Nothing else changed" row) - resolved.**
`tasks/CI-FAST-PATH.md:89` now reads
`git diff main --stat -- tools src tests`, expected empty apart from the new
test file. Run in this checkout it prints exactly
`tests/tooling/test_ci_fast_path.py | 224 +++...`, one file changed.

**Advisory 1 (docstring) - resolved.**
`.github/scripts/classify_changes.py:19-24` now says a crash fails the
`changes` job, which leaves the lab job skipped and the run not green.

**Advisory 2 (command inventory row) - resolved.**
`docs/BUILD_AND_VALIDATION.md:160` now names `--no-python-tests` and the
skipped optional check.

**Advisory 3 (heavy-step detection) - resolved.**
`test_every_lab_step_is_accounted_for` enumerates every step of `lab` and
requires the checkout, the fast-path marker, the always-on upload or the
guard. I mutation-tested it twice on copies in the scratch directory: removing
the guard from `Build lab-debug` and inserting a new unguarded
`actions/setup-python` step each make the test fail with a message naming the
step.

**Advisory 4 (the `changes` job's fixed cost) - accepted and recorded** in
attempt 7's decision column.

**Advisory 5 (the candidate's own run) - resolved.** Run 35481755910 is
attempt 6.

### Withheld cases re-checked and added

All twelve round-1 cases were re-run against the new classifier with the
base-run check unconfigured, so the path rules are isolated. W2, W4, W5, W9,
W10, W11 and W12 are unchanged and still correct. W1 (`docs/map/*.map.json`),
W3 (rename into `docs/`), W6 (an executable `tasks/run.sh`) and W7 (a symlink
`docs/link`) now correctly take the full path; W6 and W7 were latent advisories
in round 1 and the extension rule closed them. New cases:

| Case | Situation | Result | Verdict |
| --- | --- | --- | --- |
| N1 | `docs/map/m.md` summary edited alone | `docs_only=true` | wrong, see Should fix A |
| N7 | root `LICENSE` edited alone | `docs_only=false` | correct |
| N8 | `.env.example` edited alone | `docs_only=true` | correct, as designed |
| N9 | `notes/a.md` (Markdown outside `docs/`, `tasks/` and the root) | `docs_only=false` | correct |
| W8 | a symlink named `docs/gen.md` retargeted at code | `docs_only=true` | latent, see Advisory C |
| G1-G7 | the gate's seven outcomes, above | every failure mode is the full path | correct |
| G8 | the base has only a fast-path success | accepted; the API cannot distinguish one | see Advisory A |
| G9 | `head_sha` across branches | `head_sha=b72b77d7...` returns run 35481151613 on `main` and 35480892685 on `task/jev-judgment-harness` | benign: both ran the same tree, and this is what lets an integration push inherit the task branch's success |
| G10 | the base's copy of the classifier is an older revision | reproduced the workflow's `git show` selection with the `555a62b` and `afb8914` blobs | unreachable for push events, see Should fix 1 above and Advisory B |
| G11 | more than 50 runs on one base sha | `per_page=50` caps the page, so a success beyond it would be missed | conservative: the full path, which is the safe direction |

### New findings

**Should fix A - `docs/map/*.md` summaries are still documentation although the
suite validates them.** `.github/scripts/classify_changes.py:41-46` accepts any
`.md` file under `docs/`, so a map summary qualifies. But
`tests/tooling/test_coverage.py:504-506` asserts that each
`docs/map/<scenario>.md` exists and contains the first 16 characters of its
map's `coverage.sha256`. Reproduced: in a copy of `tools/`, `docs/` and
`tests/` in the scratch directory, replacing that 16-character digest in
`docs/map/boot-start-600.md` makes
`python3 -m unittest tests.tooling.test_coverage.TrackedMapTests` fail with
`AssertionError: '1e6c02747d7c1003' not found in ...`, while case N1 shows the
same change classifies as `docs_only=true`. Deleting or renaming a summary has
the same effect through the `assertTrue(summary.is_file())` line. Narrower than
the round-1 finding, because regenerating a map rewrites the JSON as well and
that forces the full path, but the resolution is one line: treat `docs/map/`
as code, for example by rejecting any path starting with `docs/map/` in
`is_documentation`, with a test.

**Should fix B - attempt 10 cites a run id that does not exist.**
`tasks/CI-FAST-PATH.md:116` names run 35482420095 for the push of `89edda0`.
`gh run view 35482420095` returns `HTTP 404: Not Found`, and
`gh run list --branch task/ci-fast-path` shows the actual run for that commit
is 35482466386 (push, success, 23 s), whose artifact and step outcomes match
everything else the row claims. The candidate's own push `afb8914` is run
35482509998 (success, 26 s). Resolution: correct the id, and add the
candidate's run when the record is next touched.

**Should fix C - the handoff still describes the round-1 state.**
`tasks/CI-FAST-PATH.md` "Handoff" says "see Evidence, attempts 1-5", lists the
commits only through `2ccef4e` ("then this checkpoint"), names only the three
round-1 hosted runs and the 10-check local suite, and ends "Accepted outcome
... pending review", while the Evidence table now runs to attempt 10 and the
branch has four more commits. AGENTS.md requires the handoff to carry the
actual commits, commands and results so a fresh agent can resume without the
conversation. Resolution: refresh the handoff with `139261d`, `89edda0` and
`afb8914`, runs 35482286606, 35482466386 and 35482509998, and the 458-check
suite.

**Advisory A - the gate cannot tell a gated fast-path success from an ungated
one.** `base_has_successful_run` counts any completed successful run at the
base, and a fast-path run is one. The induction in the documentation therefore
holds only if every fast-path success in the chain was itself gated. Two
ungated fast-path successes exist in this branch's history: 35481706436 at
`2ccef4e` and 35481755910 at `555a62b`, and I confirmed with
`gh api ...runs?head_sha=555a62ba...` that the latter is the only successful
run on that commit. Nothing reachable depends on them today, because
`89edda0` and `afb8914` are anchored on the full run at `139261d`, but the
argument rests on that history rather than on anything the check verifies.
Worth one sentence in the documentation.

**Advisory B - for a pull request the classifier's revision and the compared
revision are different commits.** `.github/workflows/synthetic.yml:43-46` takes
the classifier copy from `$CLASSIFY_BASE`, which for a pull request is
`github.event.pull_request.base.sha` (the base branch tip), while
`classify()` diffs and gates on the merge base. A base branch whose classifier
differs from the one at the branch point or on the PR head is the version that
decides. The repository takes no external pull requests and `main` will carry
the corrected classifier after integration, so this is a note rather than an
exposure; using the merge base for the `git show` as well would remove it.

**Advisory C - a `.md`-named symlink under `docs/` is still documentation.**
Case W8: a symlink `docs/gen.md` retargeted from `tools/x.py` to
`tools/other.py` classifies `docs_only=true`, since git records only the path
and the extension rule accepts it. No symlink is tracked in the repository
(`git ls-files -s` has no mode `120000` entry), so there is nothing to exploit
today.

**Advisory D - the attempts table is out of order.** In
`tasks/CI-FAST-PATH.md` the attempt 5 row now sits after attempt 10, so the
table reads 1, 2, 3, 4, 6, 7, 8, 9, 10, 5.

### Verdict

confirm - all three required corrections and both should-fix items from round 1
are implemented and independently reproduced, the suite passes at the candidate
with 458 checks and the hosted runs show the gated fast path working; the three
new should-fix items are narrow record and coverage residuals that should be
applied before integration but do not weaken the evidence the task claims.
