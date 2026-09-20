#!/usr/bin/env python3
"""Decide whether a push or pull request changed documentation only.

Run inside a checkout with full history. Reads the event name from
``GITHUB_EVENT_NAME`` and the base commit from ``CLASSIFY_BASE`` (the push's
``before`` sha or the pull request's base sha); ``CLASSIFY_HEAD`` overrides
``HEAD`` for tests. Writes ``docs_only=true|false`` to ``GITHUB_OUTPUT`` when
set, a JSON summary to ``CLASSIFY_OUT`` when set, and prints the decision.

Documentation means Markdown files under ``docs/`` or ``tasks/``, Markdown
files at the repository root and ``.env.example``; a tracked data file under
``docs/`` (the code maps are JSON) is not documentation. Everything else,
including this script and the workflow, takes the full path. So does any push
whose base cannot be established: a new branch (all-zero ``before``), a base
absent from the checkout, or a base that is not an ancestor of HEAD (a force
push). Renames are listed as a deletion plus an addition, so moving a source
file under ``docs/`` is not documentation either.

When ``CLASSIFY_REQUIRE_BASE_RUN`` names a workflow file, a docs-only result
also requires that the base commit has a successful completed run of that
workflow (read through ``gh api``); otherwise the fast path would inherit a
cancelled or failed run's gap. Every reason a check cannot be made takes the
full path and says why. A crash of this script fails the ``changes`` job,
which leaves the lab job skipped and the run not green, never silently fast.
"""

from __future__ import annotations

import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

DOC_PREFIXES = ("docs/", "tasks/")
DOC_FILES = frozenset({".env.example"})
FULL_PATH_EVENTS = ("push", "pull_request")


def is_documentation(path: str) -> bool:
    if path in DOC_FILES:
        return True
    if not path.endswith(".md"):
        return False
    return path.startswith(DOC_PREFIXES) or "/" not in path


def base_has_successful_run(base: str) -> tuple[bool | None, str]:
    """None when the check is not configured; otherwise whether the base commit
    has a successful completed run of the named workflow, with the detail."""
    workflow = os.environ.get("CLASSIFY_REQUIRE_BASE_RUN", "").strip()
    if not workflow:
        return None, "base-run check not configured"
    repo = os.environ.get("GITHUB_REPOSITORY", "").strip()
    gh = shutil.which("gh")
    if not repo or not gh:
        return False, "GITHUB_REPOSITORY or gh unavailable for the base-run check"
    result = subprocess.run(
        [gh, "api", f"repos/{repo}/actions/workflows/{workflow}/runs?head_sha={base}&per_page=50",
         "--jq", '[.workflow_runs[] | select(.status == "completed" and .conclusion == "success")] | length'],
        capture_output=True, text=True,
    )
    if result.returncode != 0:
        return False, f"gh api failed: {result.stderr.strip()[-200:]}"
    try:
        count = int(result.stdout.strip())
    except ValueError:
        return False, f"unparseable gh api output: {result.stdout.strip()[:100]}"
    return count > 0, f"{count} successful completed run(s) of {workflow} on base {base[:7]}"


def _git(*args: str) -> str | None:
    result = subprocess.run(["git", *args], capture_output=True, text=True)
    return result.stdout.strip() if result.returncode == 0 else None


def classify(event: str, base: str, head: str) -> dict:
    """The decision with its reason and the changed paths it rests on."""
    decision = {"event": event, "base": base, "head": head, "docs_only": False, "changed": [], "reason": ""}
    if event not in FULL_PATH_EVENTS:
        decision["reason"] = f"event {event or '(none)'} always takes the full path"
        return decision
    if not base or set(base) == {"0"}:
        decision["reason"] = "no base commit (new branch or empty before sha)"
        return decision
    if _git("cat-file", "-e", f"{base}^{{commit}}") is None:
        decision["reason"] = "base commit is not in the checkout"
        return decision
    merge_base = _git("merge-base", base, head)
    if not merge_base:
        decision["reason"] = "base and HEAD share no history"
        return decision
    if event == "push" and merge_base != _git("rev-parse", base):
        decision["reason"] = "base is not an ancestor of HEAD (force push)"
        return decision
    listing = _git("diff", "--name-only", "--no-renames", merge_base, head)
    if listing is None:
        decision["reason"] = "git diff failed"
        return decision
    changed = sorted(p for p in listing.splitlines() if p)
    decision["changed"] = changed
    decision["merge_base"] = merge_base
    if not changed:
        decision["reason"] = "no changed paths; full path by default"
        return decision
    code = [p for p in changed if not is_documentation(p)]
    if code:
        decision["reason"] = f"{len(code)} non-documentation path(s), first {code[0]}"
        return decision
    ok, detail = base_has_successful_run(merge_base)
    decision["base_run"] = detail
    if ok is False:
        decision["reason"] = f"all {len(changed)} changed path(s) are documentation, but {detail}"
        return decision
    decision["docs_only"] = True
    decision["reason"] = f"all {len(changed)} changed path(s) are documentation; {detail}"
    return decision


def main() -> int:
    event = os.environ.get("GITHUB_EVENT_NAME", "")
    base = os.environ.get("CLASSIFY_BASE", "").strip()
    head = os.environ.get("CLASSIFY_HEAD", "").strip() or (_git("rev-parse", "HEAD") or "")
    decision = classify(event, base, head)
    out = os.environ.get("CLASSIFY_OUT")
    if out:
        Path(out).parent.mkdir(parents=True, exist_ok=True)
        Path(out).write_text(json.dumps(decision, indent=2) + "\n", encoding="utf-8")
    github_output = os.environ.get("GITHUB_OUTPUT")
    if github_output:
        with open(github_output, "a", encoding="utf-8") as fh:
            fh.write(f"docs_only={'true' if decision['docs_only'] else 'false'}\n")
    print(f"docs_only={'true' if decision['docs_only'] else 'false'}: {decision['reason']}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
