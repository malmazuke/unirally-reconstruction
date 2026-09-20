"""ROM-free checks for the CI docs-only fast path (CI-FAST-PATH).

The classifier is run against throwaway git repositories for every base
situation the workflow can meet; the workflow file is checked textually for
the guards and for one Python tooling-test run per job.
"""

from __future__ import annotations

import json
import os
import re
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / ".github" / "scripts" / "classify_changes.py"
WORKFLOW = ROOT / ".github" / "workflows" / "synthetic.yml"
ZERO = "0" * 40


def git(cwd: Path, *args: str) -> str:
    return subprocess.run(["git", *args], cwd=cwd, capture_output=True, text=True, check=True).stdout.strip()


class Repo:
    def __init__(self, path: Path) -> None:
        self.path = path
        git(path, "init", "-q", "-b", "main")
        git(path, "config", "user.email", "t@example.invalid")
        git(path, "config", "user.name", "t")

    def commit(self, files: dict[str, str | None], message: str = "c") -> str:
        for rel, content in files.items():
            p = self.path / rel
            if content is None:
                git(self.path, "rm", "-q", rel)
            else:
                p.parent.mkdir(parents=True, exist_ok=True)
                p.write_text(content)
                git(self.path, "add", rel)
        git(self.path, "commit", "-q", "--allow-empty", "-m", message)
        return git(self.path, "rev-parse", "HEAD")

    def classify(self, event: str, base: str, head: str | None = None, base_runs: int | str | None = None) -> tuple[dict, str, str]:
        """``base_runs``: None leaves the base-run check unconfigured; an int is the
        count a stub ``gh`` reports; ``"fail"`` makes the stub exit 1."""
        out = self.path / "changes.json"
        gh_out = self.path / "gh_output"
        env = {**os.environ, "GITHUB_EVENT_NAME": event, "CLASSIFY_BASE": base,
               "CLASSIFY_OUT": str(out), "GITHUB_OUTPUT": str(gh_out)}
        env.pop("CLASSIFY_REQUIRE_BASE_RUN", None)
        if head:
            env["CLASSIFY_HEAD"] = head
        if base_runs is not None:
            stub_dir = self.path / "stub-bin"
            stub_dir.mkdir(exist_ok=True)
            gh = stub_dir / "gh"
            body = "#!/bin/sh\necho \"$@\" > \"$STUB_GH_ARGS\"\n" + ("exit 1\n" if base_runs == "fail" else f"echo {base_runs}\n")
            gh.write_text(body)
            gh.chmod(0o755)
            env.update({"PATH": f"{stub_dir}:{env['PATH']}", "CLASSIFY_REQUIRE_BASE_RUN": "synthetic.yml",
                        "GITHUB_REPOSITORY": "example/repo", "STUB_GH_ARGS": str(self.path / "gh_args")})
        proc = subprocess.run([sys.executable, str(SCRIPT)], cwd=self.path, capture_output=True, text=True, env=env, timeout=60)
        assert proc.returncode == 0, proc.stderr
        # GITHUB_OUTPUT is append-only, as on the runner; the last line is this call's.
        return json.loads(out.read_text()), gh_out.read_text().strip().splitlines()[-1], proc.stdout.strip()


class ClassifierTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.repo = Repo(Path(self.tmp.name))
        self.base = self.repo.commit({"tools/x.py": "1\n", "docs/a.md": "a\n", "README.md": "r\n"}, "base")

    def tearDown(self):
        self.tmp.cleanup()

    def test_docs_only_push(self):
        head = self.repo.commit({"docs/a.md": "b\n", "tasks/T.md": "t\n", "README.md": "rr\n", ".env.example": "K=\n"})
        d, gh, line = self.repo.classify("push", self.base, head)
        self.assertTrue(d["docs_only"], d)
        self.assertEqual(gh, "docs_only=true")
        self.assertTrue(line.startswith("docs_only=true"))
        self.assertEqual(d["changed"], [".env.example", "README.md", "docs/a.md", "tasks/T.md"])

    def test_code_push_takes_full_path(self):
        for files in ({"tools/x.py": "2\n"}, {"docs/a.md": "b\n", "src/n.cpp": "x\n"},
                      {".github/workflows/synthetic.yml": "x\n"}, {"tools/locks/l.json": "{}\n"},
                      {"notes/a.md": "nested markdown is not root docs\n"}):
            with self.subTest(files=files):
                head = self.repo.commit(files)
                d, gh, _ = self.repo.classify("push", self.base, head)
                self.assertFalse(d["docs_only"], d)
                self.assertEqual(gh, "docs_only=false")
                self.assertIn("non-documentation", d["reason"])
                self.base = head

    def test_rename_out_of_code_takes_full_path(self):
        git(self.repo.path, "mv", "tools/x.py", "docs/moved.md")
        head = self.repo.commit({}, "rename")
        d, _, _ = self.repo.classify("push", self.base, head)
        self.assertFalse(d["docs_only"], d)
        self.assertEqual(d["changed"], ["docs/moved.md", "tools/x.py"])

    def test_data_under_docs_is_not_documentation(self):
        for rel in ("docs/map/boot.map.json", "docs/figure.png", "tasks/data.csv", "docs/README"):
            with self.subTest(rel=rel):
                head = self.repo.commit({rel: "x\n"})
                d, _, _ = self.repo.classify("push", self.base, head)
                self.assertFalse(d["docs_only"], d)
                self.base = head

    def test_base_run_check(self):
        head = self.repo.commit({"docs/a.md": "b\n"})
        d, _, _ = self.repo.classify("push", self.base, head, base_runs=2)
        self.assertTrue(d["docs_only"], d)
        self.assertIn("2 successful completed run(s)", d["base_run"])
        args = (self.repo.path / "gh_args").read_text()
        self.assertIn(f"repos/example/repo/actions/workflows/synthetic.yml/runs?head_sha={self.base}", args)
        for base_runs, fragment in ((0, "0 successful completed run(s)"), ("fail", "gh api failed")):
            with self.subTest(base_runs=base_runs):
                d, gh, _ = self.repo.classify("push", self.base, head, base_runs=base_runs)
                self.assertFalse(d["docs_only"], d)
                self.assertIn("are documentation, but", d["reason"])
                self.assertIn(fragment, d["reason"])
                self.assertEqual(gh, "docs_only=false")
        # A code push never consults the base-run check.
        head = self.repo.commit({"tools/x.py": "9\n"})
        d, _, _ = self.repo.classify("push", self.base, head, base_runs="fail")
        self.assertFalse(d["docs_only"])
        self.assertNotIn("base_run", d)

    def test_deleted_documentation_is_still_documentation(self):
        head = self.repo.commit({"docs/a.md": None})
        d, _, _ = self.repo.classify("push", self.base, head)
        self.assertTrue(d["docs_only"], d)

    def test_undeterminable_base_takes_full_path(self):
        head = self.repo.commit({"docs/a.md": "b\n"})
        for base, fragment in ((ZERO, "no base"), ("", "no base"), ("deadbeef" * 5, "not in the checkout")):
            with self.subTest(base=base):
                d, gh, _ = self.repo.classify("push", base, head)
                self.assertFalse(d["docs_only"])
                self.assertIn(fragment, d["reason"])
                self.assertEqual(gh, "docs_only=false")

    def test_force_push_base_takes_full_path(self):
        # A base that is not an ancestor of HEAD: the old tip after a rewrite.
        old_tip = self.repo.commit({"docs/a.md": "old\n"})
        git(self.repo.path, "reset", "-q", "--hard", self.base)
        head = self.repo.commit({"docs/a.md": "new\n"})
        d, _, _ = self.repo.classify("push", old_tip, head)
        self.assertFalse(d["docs_only"])
        self.assertIn("not an ancestor", d["reason"])

    def test_pull_request_uses_merge_base(self):
        # main moves on with code after the branch point; the branch is docs-only.
        git(self.repo.path, "checkout", "-q", "-b", "topic")
        head = self.repo.commit({"docs/a.md": "topic\n"})
        git(self.repo.path, "checkout", "-q", "main")
        main_tip = self.repo.commit({"tools/x.py": "3\n"})
        d, _, _ = self.repo.classify("pull_request", main_tip, head)
        self.assertTrue(d["docs_only"], d)
        self.assertEqual(d["merge_base"], self.base)

    def test_other_events_take_full_path(self):
        head = self.repo.commit({"docs/a.md": "b\n"})
        d, gh, _ = self.repo.classify("workflow_dispatch", self.base, head)
        self.assertFalse(d["docs_only"])
        self.assertIn("always takes the full path", d["reason"])


class WorkflowTests(unittest.TestCase):
    def setUp(self):
        self.text = WORKFLOW.read_text()
        lab = self.text[self.text.index("\n  lab:\n"):]
        self.steps = re.split(r"\n      - (?=name:|uses:)", lab)[1:]

    def test_every_lab_step_is_accounted_for(self):
        """Each step of the lab job is the checkout, the fast-path marker, the
        always-on upload, or guarded by the docs-only output. A new step that
        forgets the guard fails here."""
        self.assertIn("\n  lab:\n    needs: changes\n", self.text)
        self.assertGreaterEqual(len(self.steps), 13)
        for step in self.steps:
            first = step.splitlines()[0]
            head = "\n".join(step.splitlines()[:3])
            if first.startswith("uses: actions/checkout"):
                continue
            if first.startswith("name: Docs-only fast path"):
                self.assertIn("if: needs.changes.outputs.docs_only == 'true'", head)
                continue
            if first.startswith("name: Upload reports"):
                self.assertIn("if: always()", head)
                continue
            self.assertIn("needs.changes.outputs.docs_only != 'true'", head, first)

    def test_python_tooling_tests_run_once_per_job(self):
        runs = re.findall(r"python3 tools/project\.py test [^\n]*", self.text)
        self.assertEqual(len(runs), 4, runs)
        with_python = [r for r in runs if "--no-python-tests" not in r]
        self.assertEqual(len(with_python), 1, runs)
        self.assertIn("--preset lab-debug", with_python[0])
        for r in runs:
            if "--no-python-tests" in r:
                self.assertTrue(any(p in r for p in ("app-debug", "lab-sanitize", "app-sanitize")), r)

    def test_changes_job_shape(self):
        changes = self.text[self.text.index("\n  changes:\n"):self.text.index("\n  lab:\n")]
        self.assertIn("fetch-depth: 0", changes)
        self.assertIn("CLASSIFY_BASE: ${{ github.event.before || github.event.pull_request.base.sha }}", changes)
        self.assertIn('git show "$CLASSIFY_BASE:$script" > "$RUNNER_TEMP/classify_changes.py"', changes)
        self.assertIn("CLASSIFY_REQUIRE_BASE_RUN: synthetic.yml", changes)
        self.assertIn("GH_TOKEN: ${{ github.token }}", changes)
        self.assertIn("docs_only: ${{ steps.classify.outputs.docs_only }}", changes)
        self.assertIn("\n  actions: read", self.text[:self.text.index("jobs:")])


if __name__ == "__main__":
    unittest.main()
