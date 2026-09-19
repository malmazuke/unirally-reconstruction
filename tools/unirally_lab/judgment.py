"""Typed judgments from TypeSafe's System One endpoint (Jev), recorded as evidence.

A judgment is a small semantic decision the tooling cannot make from bytes
alone: does a record name its independent check, does a handoff claim a
skipped check as a pass. Code owns the workflow; the model answers typed
questions (``noul`` probability, ``choice`` distribution, ``score`` level)
over a JSON ``state``. Every call is written to an artifact with the full
request, the response, the model that answered, token usage and elapsed
time, so a record can cite it. Nothing here is required for acceptance:
D-0007 makes every judgment advisory.

Standard library only. The API key comes from ``TYPESAFE_API_KEY`` or the
ignored ``.env`` at the repository root and is never written to a report,
an artifact or a command line.
"""

from __future__ import annotations

import hashlib
import json
import os
import shutil
import ssl
import stat
import tempfile
import time
import urllib.error
import urllib.request
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, Callable

from .procs import run_bounded

API_URL = "https://api.typesafe.ai/v1/systemone"
ENDPOINT_ENV = "UNIRALLY_LAB_JUDGE_ENDPOINT"  # testing hook: a local stub server
KEY_ENV = "TYPESAFE_API_KEY"
DEFAULT_MODEL = "jev-latest"
RETRY_STATUSES = (429, 529)
QUESTION_TYPES = ("noul", "choice", "score")


class JudgmentError(Exception):
    """A call that produced no usable answers. ``outcome`` is a report outcome."""

    def __init__(self, outcome: str, detail: str) -> None:
        super().__init__(detail)
        self.outcome = outcome
        self.detail = detail


# ------------------------------------------------------------------ key


def parse_dotenv(text: str) -> dict[str, str]:
    """``KEY=VALUE`` lines; ``export`` prefixes, comments and quotes tolerated."""
    values: dict[str, str] = {}
    for raw in text.splitlines():
        line = raw.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        if line.startswith("export "):
            line = line[len("export "):].lstrip()
        name, value = line.split("=", 1)
        name, value = name.strip(), value.strip()
        if len(value) >= 2 and value[0] == value[-1] and value[0] in "\"'":
            value = value[1:-1]
        if name:
            values[name] = value
    return values


def key_problem(key: str) -> str | None:
    """Why a key value cannot be sent, or None. Quotes, whitespace and control
    characters would be mangled by curl's config quoting, so they are refused
    before any request rather than silently truncated."""
    if any(ch.isspace() or ord(ch) < 32 or ch in "\"'\\" for ch in key):
        return "contains whitespace, quotes, a backslash or a control character"
    if len(key) < 8:
        return "is too short to be a key"
    return None


def find_api_key(root: Path, environ: dict[str, str] | None = None) -> tuple[str | None, str]:
    """The key and where it came from: ``environment``, ``.env``, ``none`` or
    ``invalid (<source>): <problem>`` when a value was found but cannot be used."""
    env = os.environ if environ is None else environ
    key = (env.get(KEY_ENV) or "").strip()
    source = "environment"
    if not key:
        dotenv = Path(root) / ".env"
        if dotenv.is_file():
            try:
                key = parse_dotenv(dotenv.read_text(encoding="utf-8")).get(KEY_ENV, "").strip()
            except OSError:
                key = ""
            source = ".env"
    if not key:
        return None, "none"
    problem = key_problem(key)
    if problem:
        return None, f"invalid ({source}): the value {problem}"
    return key, source


def redact(value: Any, key: str | None) -> Any:
    """``value`` (JSON-compatible) with every occurrence of ``key`` replaced."""
    if not key:
        return value
    text = json.dumps(value, ensure_ascii=False)
    if key not in text:
        return value
    return json.loads(text.replace(key, "<redacted-api-key>"))


def endpoint_url(environ: dict[str, str] | None = None) -> str:
    env = os.environ if environ is None else environ
    return env.get(ENDPOINT_ENV) or API_URL


# ------------------------------------------------------------- questions


def validate_questions(questions: Any) -> list[str]:
    """Problems with a questions map, empty when it is well formed."""
    problems: list[str] = []
    if not isinstance(questions, dict) or not questions:
        return ["questions must be a non-empty object keyed by question id"]
    for qid, q in questions.items():
        if not isinstance(q, dict):
            problems.append(f"{qid}: not an object")
            continue
        qtype = q.get("type")
        if qtype not in QUESTION_TYPES:
            problems.append(f"{qid}: type must be one of {', '.join(QUESTION_TYPES)}")
        if not q.get("instructions"):
            problems.append(f"{qid}: instructions are required")
        if qtype == "choice" and not isinstance(q.get("criteria"), dict):
            problems.append(f"{qid}: a choice needs a criteria object of options")
        if qtype == "score" and not isinstance(q.get("criteria"), list):
            problems.append(f"{qid}: a score needs a criteria list of ordered levels")
    return problems


# ------------------------------------------------------------- transport

Transport = Callable[[str, bytes, str, float], tuple[int, str, str]]
"""(url, body, key, timeout) -> (http status, response text, transport name)."""


def _curl_post(url: str, body: bytes, key: str, timeout: float) -> tuple[int, str, str] | None:
    curl = shutil.which("curl")
    if not curl:
        return None
    workdir = Path(tempfile.mkdtemp(prefix="unirally-judge-"))
    try:
        req = workdir / "request.json"
        req.write_bytes(body)
        out = workdir / "response.json"
        # The key goes through a private config file, never argv (visible in ps).
        config = workdir / "curl.cfg"
        config.write_text(
            f'header = "Authorization: Bearer {key}"\n'
            'header = "Content-Type: application/json"\n'
            f'url = "{url}"\n'
            f'data-binary = "@{req}"\n'
            f'output = "{out}"\n'
            'write-out = "%{http_code}"\n'
            f'max-time = {timeout:.3f}\n'
            "silent\nshow-error\n",
            encoding="utf-8",
        )
        os.chmod(config, stat.S_IRUSR | stat.S_IWUSR)
        result = run_bounded([curl, "-K", str(config)], timeout=timeout + 15)
        if result.timed_out:
            raise JudgmentError("timeout", f"curl exceeded {timeout:.0f}s")
        if result.missing:
            return None
        code = result.stdout.strip()[-3:]
        if result.returncode == 28:  # curl: operation timed out (--max-time)
            raise JudgmentError("timeout", f"request exceeded {timeout:.0f}s")
        if result.returncode != 0 or not code.isdigit() or code == "000":
            raise JudgmentError("failed", f"curl exit {result.returncode}: {result.stderr.strip()[-300:] or result.tail(300)}")
        text = out.read_text(encoding="utf-8", errors="replace") if out.is_file() else ""
        return int(code), text, f"curl ({curl})"
    finally:
        shutil.rmtree(workdir, ignore_errors=True)


def _urllib_post(url: str, body: bytes, key: str, timeout: float) -> tuple[int, str, str]:
    request = urllib.request.Request(
        url, data=body, method="POST",
        headers={"Authorization": f"Bearer {key}", "Content-Type": "application/json"},
    )
    try:
        with urllib.request.urlopen(request, timeout=timeout) as response:
            return response.status, response.read().decode("utf-8", "replace"), "urllib"
    except urllib.error.HTTPError as exc:
        return exc.code, exc.read().decode("utf-8", "replace"), "urllib"
    except urllib.error.URLError as exc:
        reason = exc.reason
        if isinstance(reason, ssl.SSLCertVerificationError):
            raise JudgmentError("missing", f"no curl and urllib has no CA bundle: {reason}") from exc
        if isinstance(reason, TimeoutError) or "timed out" in str(reason):
            raise JudgmentError("timeout", f"request exceeded {timeout:.0f}s") from exc
        raise JudgmentError("failed", f"request failed: {reason}") from exc
    except TimeoutError as exc:
        raise JudgmentError("timeout", f"request exceeded {timeout:.0f}s") from exc


def default_transport(url: str, body: bytes, key: str, timeout: float) -> tuple[int, str, str]:
    """curl first (the same reason as the toolchain downloader: python.org builds
    ship without a CA bundle), urllib when curl is absent."""
    result = _curl_post(url, body, key, timeout)
    if result is not None:
        return result
    return _urllib_post(url, body, key, timeout)


# --------------------------------------------------------------- evaluate


@dataclass
class Judgment:
    request: dict[str, Any]
    response: dict[str, Any]
    http_status: int
    model: str | None
    usage: dict[str, Any]
    elapsed: float
    attempts: int
    transport: str
    key_source: str
    endpoint: str
    retries: list[dict[str, Any]] = field(default_factory=list)

    @property
    def answers(self) -> dict[str, Any]:
        return self.response.get("answers", {})

    def state_sha256(self) -> str:
        return hashlib.sha256(canonical_json(self.request["state"]).encode()).hexdigest()

    def to_record(self) -> dict[str, Any]:
        return {
            "schema_version": 1,
            "endpoint": self.endpoint,
            "model_requested": self.request.get("model"),
            "model": self.model,
            "state_sha256": self.state_sha256(),
            "request": self.request,
            "response": self.response,
            "http_status": self.http_status,
            "usage": self.usage,
            "elapsed_seconds": round(self.elapsed, 3),
            "attempts": self.attempts,
            "retries": self.retries,
            "transport": self.transport,
            "key_source": self.key_source,
        }


def canonical_json(value: Any) -> str:
    return json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=False)


def evaluate(
    state: Any,
    questions: dict[str, Any],
    *,
    key: str,
    key_source: str = "environment",
    model: str = DEFAULT_MODEL,
    timeout: float = 30.0,
    attempts: int = 3,
    transport: Transport | None = None,
    sleep: Callable[[float], None] = time.sleep,
    environ: dict[str, str] | None = None,
) -> Judgment:
    """One evaluation of every question over ``state``; retries only 429/529.

    Raises :class:`JudgmentError` with a report outcome when no usable
    answers came back: ``missing`` (no transport), ``timeout`` or ``failed``
    (rejected key, invalid request, overload after every attempt, a body
    without every question's answer).
    """
    problems = validate_questions(questions)
    if problems:
        raise JudgmentError("failed", "; ".join(problems))
    if attempts < 1:
        raise JudgmentError("failed", "attempts must be at least 1")
    send = transport or default_transport
    url = endpoint_url(environ)
    request = {"state": state, "model": model, "questions": questions}
    body = json.dumps(request, ensure_ascii=False).encode("utf-8")
    started = time.monotonic()
    retries: list[dict[str, Any]] = []
    status, text, name = 0, "", ""
    for attempt in range(1, attempts + 1):
        status, text, name = send(url, body, key, timeout)
        if status not in RETRY_STATUSES:
            break
        retries.append({"attempt": attempt, "http_status": status, "body": text[:200]})
        if attempt < attempts:
            sleep(min(0.5 * (2 ** (attempt - 1)), 4.0))
    elapsed = time.monotonic() - started
    if status == 401:
        raise JudgmentError("failed", "401: the API key was rejected")
    if status == 422:
        raise JudgmentError("failed", f"422: the request was rejected: {text[:300]}")
    if status in RETRY_STATUSES:
        raise JudgmentError("failed", f"{status} after {attempts} attempts: {text[:200]}")
    if status != 200:
        raise JudgmentError("failed", f"HTTP {status}: {text[:300]}")
    try:
        response = json.loads(text)
    except ValueError as exc:
        raise JudgmentError("failed", f"response is not JSON: {text[:200]}") from exc
    answers = response.get("answers") if isinstance(response, dict) else None
    if not isinstance(answers, dict):
        raise JudgmentError("failed", "response carries no answers object")
    absent = sorted(set(questions) - set(answers))
    if absent:
        raise JudgmentError("failed", f"answers missing for: {', '.join(absent)}")
    malformed = sorted(qid for qid in questions if not isinstance(answers[qid], dict))
    if malformed:
        raise JudgmentError("failed", f"answers are not objects for: {', '.join(malformed)}")
    return Judgment(
        request=request, response=response, http_status=status,
        model=response.get("model"), usage=response.get("usage") or {},
        elapsed=elapsed, attempts=len(retries) + 1, transport=name,
        key_source=key_source, endpoint=url, retries=retries,
    )


def write_judgment(judgment: Judgment, path: Path, key: str) -> Path:
    """Write the evidence record. Refuses to write a record containing the key."""
    text = json.dumps(judgment.to_record(), indent=2, sort_keys=True, ensure_ascii=False) + "\n"
    if key and key in text:
        raise JudgmentError("failed", "refusing to write an artifact that contains the API key")
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8")
    return path


# ------------------------------------------------------- evidence lint

# Advisory thresholds (D-0007): starting points to evaluate against the
# project's own records, not calibrated limits. A noul below FLAG_BELOW
# flags; a choice flags on the named option at or above FLAG_CONFIDENCE.
FLAG_BELOW = 0.5
FLAG_CONFIDENCE = 0.6

EVIDENCE_QUESTIONS: dict[str, dict[str, Any]] = {
    "independent_check_named": {
        "type": "noul",
        "instructions": (
            "Does the record in `text` state who or what performed an independent "
            "check of its finding (a different agent, a fresh clone, a reviewer, a "
            "second observer, a traced writer/reader) and what that check found?"
        ),
        "criteria": {
            "true": "A specific checker and the outcome of the check are stated",
            "false": "No check is described, or the record only says a check is lacking or pending",
        },
    },
    "observation_distinct": {
        "type": "noul",
        "instructions": (
            "Are the measured observations in `text` (values, addresses, frames, hashes, "
            "what was seen) kept distinct from the interpretation of what they mean?"
        ),
        "criteria": {
            "true": "A reader can tell which statements were measured and which are inferred",
            "false": "Measurements and inferences are blended so a reader cannot separate them",
        },
    },
    "reproducible": {
        "type": "noul",
        "instructions": (
            "Does `text` give enough to repeat the experiment: an exact command or "
            "procedure, the source commit or branch, and where the artifact lives?"
        ),
        "criteria": {
            "true": "Command or procedure, source identity and artifact location are all present",
            "false": "At least one of those is absent, so repeating the experiment needs guesswork",
        },
    },
    "identity_stated": {
        "type": "noul",
        "instructions": (
            "Does `text` state the identity of what was measured: the ROM hash or the "
            "emulator core revision, and the addresses, frames or state fields involved?"
        ),
        "criteria": {
            "true": "ROM or core identity and the concrete addresses, frames or fields are named",
            "false": "The identity of the measured thing is left implicit",
        },
    },
    "falsifiable": {
        "type": "noul",
        "instructions": (
            "Does `text` say what observation would contradict its interpretation, "
            "or name the plausible alternatives it rules out?"
        ),
        "criteria": {
            "true": "A falsifying observation or ruled-out alternative is named",
            "false": "The interpretation is asserted without saying what would refute it",
        },
    },
    "status_supported": {
        "type": "choice",
        "instructions": (
            "`status_line` is the status the record claims for itself. Judged against "
            "the evidence in `text`, is that status supported?"
        ),
        "criteria": {
            "supported": "The evidence in the record justifies the claimed status",
            "overclaimed": "The status claims more verification, acceptance or completeness than the record evidences",
            "not_checkable": "The record carries no status, or the status cannot be judged from the text",
        },
    },
}


def record_state(path: Path, text: str) -> dict[str, Any]:
    """The state sent for one Markdown record: path, title, status line, section list, full text."""
    lines = text.splitlines()
    title = next((ln.lstrip("# ").strip() for ln in lines if ln.startswith("# ")), "")
    status = ""
    for ln in lines:
        stripped = ln.strip().lstrip("- ").strip()
        if stripped.lower().startswith("status:"):
            status = stripped[len("status:"):].strip()
            break
    sections = [ln.lstrip("#").strip() for ln in lines if ln.startswith("## ")]
    return {
        "record_path": str(path),
        "title": title,
        "status_line": status,
        "sections": sections,
        "text": text,
    }


def evidence_flags(answers: dict[str, Any]) -> list[dict[str, Any]]:
    """Per question: the flag decision with the probability behind it."""
    flags: list[dict[str, Any]] = []
    for qid, question in EVIDENCE_QUESTIONS.items():
        answer = answers.get(qid)
        if not isinstance(answer, dict):
            answer = {}
        if question["type"] == "noul":
            p = answer.get("noul")
            flagged = p is not None and p < FLAG_BELOW
            flags.append({"question": qid, "flagged": bool(flagged), "noul": p,
                          "rule": f"flag when noul < {FLAG_BELOW}"})
        else:
            choice = answer.get("choice")
            confidence = answer.get("confidence")
            flagged = choice == "overclaimed" and (confidence or 0.0) >= FLAG_CONFIDENCE
            flags.append({"question": qid, "flagged": bool(flagged), "choice": choice,
                          "confidence": confidence, "probabilities": answer.get("probabilities"),
                          "rule": f"flag when choice is overclaimed with confidence >= {FLAG_CONFIDENCE}"})
    return flags
