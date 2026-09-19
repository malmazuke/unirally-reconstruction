"""``judge`` subcommands: advisory Jev judgments recorded as evidence (D-0007).

``judge ping`` proves the key and endpoint work; ``judge ask`` evaluates a
caller's state and questions; ``judge evidence-lint`` asks the fixed
questions in :mod:`unirally_lab.judgment` over one or more Markdown
records. Every call writes its request and response under ``--out``
(default ``artifacts/judge/<run-id>/``) and the report cites that file.

Outcomes: a missing key or transport is ``missing`` (exit 2); a rejected key,
invalid request or incomplete answer set is ``failed`` (exit 1); an unreadable
input is invalid input (exit 3). Lint flags are optional checks, so a flagged
record never fails the run: the judgment informs a reader, it does not gate.
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any

import re

from . import EXIT_INVALID_INPUT, EXIT_MISSING_PREREQUISITE, EXIT_OK, EXIT_TIMEOUT
from . import judgment
from . import report as reportmod

ROOT = reportmod.repo_root()
ARTIFACT_NAME = re.compile(r"^[A-Za-z0-9][A-Za-z0-9._-]{0,127}$")
LINT_SUMMARY = "evidence-lint"  # reserved: the per-run summary file in --out

PING_STATE = {
    "record_status": "independently verified",
    "independent_check": "Lack of a check is explicit.",
}
PING_QUESTIONS = {
    "check_named": {
        "type": "noul",
        "instructions": "Does `independent_check` name who or what performed an independent check and its outcome?",
        "criteria": {"true": "A specific checker and outcome are stated",
                     "false": "No checker or outcome is stated, or the text is a template placeholder"},
    },
    "status_supported": {
        "type": "choice",
        "instructions": "Given `record_status` and `independent_check`, is the claimed status supported?",
        "criteria": {"supported": None,
                     "unsupported": "Status claims verification the text does not evidence",
                     "not_checkable": None},
    },
}


def _finish(rep: reportmod.Report, args: argparse.Namespace, status: int, key: str | None = None) -> int:
    rep.finish("passed" if status == EXIT_OK else "failed")
    # A transport error message could in principle echo a header; the report
    # is scanned for the key the same way the artifact is.
    redacted = judgment.redact(rep.data, key)
    if redacted is not rep.data:
        rep.data = redacted
        rep.data["redacted"] = "the API key appeared in report text and was replaced"
    rep.write(Path(args.report) if getattr(args, "report", None) else None)
    reportmod.print_summary(rep)
    return status


def _valid_name(value: str) -> bool:
    """A plain file name: no separators, no ``..``, bounded length."""
    return bool(ARTIFACT_NAME.match(value)) and value not in (".", "..")


def _check_arguments(rep: reportmod.Report, args: argparse.Namespace) -> bool:
    problems = []
    if args.timeout <= 0:
        problems.append("--timeout must be positive")
    if args.attempts < 1:
        problems.append("--attempts must be at least 1")
    name = getattr(args, "name", None)
    if name is not None and not _valid_name(name):
        problems.append("--name must be a plain file name (letters, digits, '.', '_', '-'; no separators or '..')")
    if problems:
        rep.add_check("arguments", "failed", detail="; ".join(problems))
        return False
    return True


def _out_dir(args: argparse.Namespace, rep: reportmod.Report, root: Path) -> Path:
    out = Path(args.out) if args.out else root / "artifacts" / "judge" / rep.data["run_id"]
    out = out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    return out


def _key(rep: reportmod.Report, root: Path) -> tuple[str | None, str]:
    key, source = judgment.find_api_key(root)
    if key:
        rep.add_check("typesafe_api_key", "passed", detail=f"from {source}")
    elif source.startswith("invalid"):
        rep.add_check("typesafe_api_key", "failed", detail=source)
    else:
        rep.add_check("typesafe_api_key", "missing",
                      detail=f"set {judgment.KEY_ENV} in the environment or in {root / '.env'} (see .env.example)")
    return key, source


def _exit_for(outcome: str) -> int:
    if outcome == "missing":
        return EXIT_MISSING_PREREQUISITE
    if outcome == "timeout":
        return EXIT_TIMEOUT
    return 1


def _evaluate(rep: reportmod.Report, args: argparse.Namespace, *, key: str, source: str,
              state: Any, questions: dict[str, Any], out: Path, name: str) -> judgment.Judgment | None:
    """One call plus its artifact; the ``judgment:<name>`` check carries the outcome."""
    try:
        result = judgment.evaluate(state, questions, key=key, key_source=source, model=args.model,
                                   timeout=args.timeout, attempts=args.attempts)
        path = judgment.write_judgment(result, out / f"{name}.json", key)
    except judgment.JudgmentError as exc:
        rep.add_check(f"judgment:{name}", exc.outcome, detail=exc.detail)
        return None
    except OSError as exc:
        rep.add_check(f"judgment:{name}", "failed", detail=f"cannot write the artifact: {exc}")
        return None
    rep.add_artifact("judgment", path)
    usage = result.usage
    rep.add_check(
        f"judgment:{name}", "passed",
        detail=f"{result.model} answered {len(result.answers)} questions in {result.elapsed:.2f}s "
               f"({usage.get('input_tokens', '?')} input tokens, attempt {result.attempts}, {result.transport})",
        model=result.model, elapsed=round(result.elapsed, 3), usage=usage, attempts=result.attempts,
        state_sha256=result.state_sha256(), artifact=str(path),
    )
    return result


def _status(rep: reportmod.Report) -> int:
    outcomes = {c["outcome"] for c in rep.required_failures()}
    if "timeout" in outcomes:
        return EXIT_TIMEOUT
    if "missing" in outcomes:
        return EXIT_MISSING_PREREQUISITE
    return 1 if outcomes else EXIT_OK


# -------------------------------------------------------------------- ping


def cmd_ping(args: argparse.Namespace) -> int:
    rep = reportmod.Report(sys.argv, task_id=args.task)
    root = Path(args.root).resolve()
    if not _check_arguments(rep, args):
        return _finish(rep, args, EXIT_INVALID_INPUT)
    key, source = _key(rep, root)
    if not key:
        return _finish(rep, args, _status(rep))
    out = _out_dir(args, rep, root)
    result = _evaluate(rep, args, key=key, source=source, state=PING_STATE, questions=PING_QUESTIONS,
                       out=out, name="ping")
    if result is not None:
        answers = result.answers
        p = answers["check_named"].get("noul")
        choice = answers["status_supported"].get("choice")
        # The placeholder names no checker, so a calibrated model answers low
        # and "unsupported". Anything else is reported, not failed: it is a
        # property of the model, and D-0007 keeps judgments advisory.
        expected = p is not None and p < 0.5 and choice == "unsupported"
        rep.add_check("ping_answers_as_expected", "passed" if expected else "failed", required=False,
                      detail=f"check_named noul={p}, status_supported={choice}")
        print(json.dumps(answers, indent=2, sort_keys=True))
    return _finish(rep, args, _status(rep), key)


# --------------------------------------------------------------------- ask


def _load_json_arg(label: str, value: str) -> Any:
    """A JSON file path, or ``-`` for stdin."""
    try:
        text = sys.stdin.read() if value == "-" else Path(value).read_text(encoding="utf-8")
    except OSError as exc:
        raise ValueError(f"{label}: cannot read {value}: {exc}") from exc
    try:
        return json.loads(text)
    except ValueError as exc:
        raise ValueError(f"{label}: {value} is not valid JSON: {exc}") from exc


def cmd_ask(args: argparse.Namespace) -> int:
    rep = reportmod.Report(sys.argv, task_id=args.task)
    root = Path(args.root).resolve()
    if not _check_arguments(rep, args):
        return _finish(rep, args, EXIT_INVALID_INPUT)
    if args.state == "-" and args.questions == "-":
        rep.add_check("arguments", "failed", detail="only one of --state and --questions may read stdin")
        return _finish(rep, args, EXIT_INVALID_INPUT)
    try:
        state = _load_json_arg("--state", args.state)
        questions = _load_json_arg("--questions", args.questions)
    except ValueError as exc:
        rep.add_check("arguments", "failed", detail=str(exc))
        return _finish(rep, args, EXIT_INVALID_INPUT)
    problems = judgment.validate_questions(questions)
    if problems:
        rep.add_check("questions", "failed", detail="; ".join(problems))
        return _finish(rep, args, EXIT_INVALID_INPUT)
    rep.add_check("questions", "passed", detail=f"{len(questions)} questions")
    key, source = _key(rep, root)
    if not key:
        return _finish(rep, args, _status(rep))
    out = _out_dir(args, rep, root)
    result = _evaluate(rep, args, key=key, source=source, state=state, questions=questions,
                       out=out, name=args.name)
    if result is not None:
        print(json.dumps(result.answers, indent=2, sort_keys=True))
    return _finish(rep, args, _status(rep), key)


# ------------------------------------------------------------ evidence-lint


def cmd_evidence_lint(args: argparse.Namespace) -> int:
    rep = reportmod.Report(sys.argv, task_id=args.task)
    root = Path(args.root).resolve()
    if not _check_arguments(rep, args):
        return _finish(rep, args, EXIT_INVALID_INPUT)
    private = (root / "local").resolve()
    records: list[tuple[Path, str]] = []
    names: dict[str, Path] = {}
    for value in args.record:
        path = Path(value)
        try:
            if path.resolve().is_relative_to(private):
                # Tracked records only: what is sent leaves the machine, and
                # local/ holds ROM-derived captures and other private inputs.
                rep.add_check("records", "failed", detail=f"{path} is under {private}; judge only tracked records")
                return _finish(rep, args, EXIT_INVALID_INPUT)
            text = path.read_text(encoding="utf-8")
        except OSError as exc:
            rep.add_check("records", "failed", detail=f"cannot read {path}: {exc}")
            return _finish(rep, args, EXIT_INVALID_INPUT)
        if not text.strip():
            rep.add_check("records", "failed", detail=f"{path} is empty")
            return _finish(rep, args, EXIT_INVALID_INPUT)
        name = path.stem
        if not _valid_name(name) or name == LINT_SUMMARY:
            rep.add_check("records", "failed", detail=f"{path}: the file name cannot name an artifact ({LINT_SUMMARY!r} is reserved)")
            return _finish(rep, args, EXIT_INVALID_INPUT)
        if name in names:
            rep.add_check("records", "failed",
                          detail=f"{path} and {names[name]} share the artifact name {name!r}; lint them in separate runs")
            return _finish(rep, args, EXIT_INVALID_INPUT)
        names[name] = path
        records.append((path, text))
        rep.add_input(f"record:{path.name}", path, reportmod.file_sha256(path), size=len(text))
    rep.add_check("records", "passed", detail=f"{len(records)} record(s)")
    key, source = _key(rep, root)
    if not key:
        return _finish(rep, args, _status(rep))
    out = _out_dir(args, rep, root)
    summary: dict[str, Any] = {}
    for path, text in records:
        name = path.stem
        state = judgment.record_state(path, text)
        result = _evaluate(rep, args, key=key, source=source, state=state,
                           questions=judgment.EVIDENCE_QUESTIONS, out=out, name=name)
        if result is None:
            continue
        flags = judgment.evidence_flags(result.answers)
        for flag in flags:
            value = (f"noul={flag['noul']}" if "noul" in flag
                     else f"choice={flag['choice']} confidence={flag['confidence']}")
            rep.add_check(f"lint:{name}:{flag['question']}", "failed" if flag["flagged"] else "passed",
                          required=False, detail=f"{value}; {flag['rule']}")
        summary[str(path)] = {"model": result.model, "flags": flags, "artifact": f"{name}.json"}
    if summary:
        summary_path = out / f"{LINT_SUMMARY}.json"
        summary_path.write_text(json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        rep.add_artifact("evidence_lint_summary", summary_path)
        print(json.dumps(summary, indent=2, sort_keys=True))
    return _finish(rep, args, _status(rep), key)


# ---------------------------------------------------------------- register


def _common(p: argparse.ArgumentParser) -> None:
    p.add_argument("--report", help="write the JSON run report here")
    p.add_argument("--task", help="task ID to record in the report")
    p.add_argument("--root", default=str(ROOT), help="repository root (for .env and artifacts)")
    p.add_argument("--out", help="directory for judgment artifacts (default artifacts/judge/<run-id>)")
    p.add_argument("--model", default=judgment.DEFAULT_MODEL,
                   help="model name or versioned id; the response's model is recorded either way")
    p.add_argument("--timeout", type=float, default=30.0, help="seconds per request")
    p.add_argument("--attempts", type=int, default=3, help="attempts on 429/529 with backoff")


def register(sub: argparse._SubParsersAction) -> None:
    judge = sub.add_parser("judge", help="advisory Jev judgments recorded as evidence (D-0007)")
    js = judge.add_subparsers(dest="judge_command", required=True)

    ping = js.add_parser("ping", help="one fixed request: proves the key, endpoint and answer shape")
    _common(ping)
    ping.set_defaults(func=cmd_ping)

    ask = js.add_parser("ask", help="evaluate a JSON state against a JSON questions map")
    _common(ask)
    ask.add_argument("--state", required=True, help="JSON file with the state, or - for stdin")
    ask.add_argument("--questions", required=True, help="JSON file with the questions map, or - for stdin")
    ask.add_argument("--name", default="ask", help="artifact name: a plain file name without extension (default ask)")
    ask.set_defaults(func=cmd_ask)

    lint = js.add_parser("evidence-lint", help="advisory evidence questions over Markdown records")
    _common(lint)
    lint.add_argument("--record", action="append", required=True,
                      help="tracked Markdown record (never under local/); repeatable; file names must be distinct")
    lint.set_defaults(func=cmd_evidence_lint)
