#!/usr/bin/env python3
"""Decide whether a push or pull request changed documentation only.

Run inside a checkout with full history. Reads the event name from
``GITHUB_EVENT_NAME`` and the base commit from ``CLASSIFY_BASE`` (the push's
``before`` sha or the pull request's base sha); ``CLASSIFY_HEAD`` overrides
``HEAD`` for tests. Writes ``docs_only=true|false`` to ``GITHUB_OUTPUT`` when
set, a JSON summary to ``CLASSIFY_OUT`` when set, and prints the decision.

Documentation means paths under ``docs/`` or ``tasks/``, Markdown files at the
repository root and ``.env.example``. Everything else, including this script
and the workflow, takes the full path. So does any push whose base cannot be
established: a new branch (all-zero ``before``), a base absent from the
checkout, or a base that is not an ancestor of HEAD (a force push). The
decision never fails the job; an error takes the full path and says why.
"""

from __future__ import annotations

import json
import os
import subprocess
import sys
from pathlib import Path

DOC_PREFIXES = ("docs/", "tasks/")
DOC_FILES = frozenset({".env.example"})
FULL_PATH_EVENTS = ("push", "pull_request")


def is_documentation(path: str) -> bool:
    if path.startswith(DOC_PREFIXES) or path in DOC_FILES:
        return True
    return path.endswith(".md") and "/" not in path


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
    listing = _git("diff", "--name-only", merge_base, head)
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
    else:
        decision["docs_only"] = True
        decision["reason"] = f"all {len(changed)} changed path(s) are documentation"
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
