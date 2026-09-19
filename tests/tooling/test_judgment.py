"""ROM-free and network-free checks for the advisory Jev judgments (D-0007).

The client is exercised in process with a fake transport (retry on 429,
rejected key, incomplete answers, key discovery, artifact hygiene) and
through ``project.py judge`` against a local stub HTTP server, so the real
transport path (curl or urllib) and the report/exit-code contract are
covered without the network or a real key.
"""

from __future__ import annotations

import http.server
import json
import os
import subprocess
import sys
import tempfile
import threading
import time
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

from unirally_lab import EXIT_FAILURE, EXIT_INVALID_INPUT, EXIT_MISSING_PREREQUISITE, EXIT_OK, EXIT_TIMEOUT  # noqa: E402
from unirally_lab import judgment  # noqa: E402

PROJECT = ROOT / "tools" / "project.py"
FAKE_KEY = "ts-test-key-0123456789abcdef"

TWO_QUESTIONS = {
    "yes": {"type": "noul", "instructions": "Is it?"},
    "pick": {"type": "choice", "instructions": "Which?", "criteria": {"a": None, "b": "bee"}},
}


def answers_for(questions: dict) -> dict:
    out = {}
    for qid, q in questions.items():
        if q["type"] == "noul":
            out[qid] = {"type": "noul", "noul": 0.04}
        elif q["type"] == "choice":
            options = list(q["criteria"])
            first = "unsupported" if "unsupported" in options else ("overclaimed" if "overclaimed" in options else options[0])
            probs = {o: 0.05 for o in options}
            probs[first] = 0.9
            out[qid] = {"type": "choice", "choice": first, "confidence": 0.85, "probabilities": probs}
        else:
            out[qid] = {"type": "score", "score": 1.0, "probabilities": {}, "confidence": 0.5}
    return out


def ok_body(questions: dict) -> str:
    return json.dumps({"model": "jev-stub", "answers": answers_for(questions),
                       "usage": {"input_tokens": 12, "output_tokens": 3}})


# ------------------------------------------------------------- in process


class FakeTransport:
    def __init__(self, script):
        self.script = list(script)
        self.calls = []

    def __call__(self, url, body, key, timeout):
        self.calls.append({"url": url, "body": json.loads(body), "key": key, "timeout": timeout})
        status, text = self.script.pop(0)
        return status, text, "fake"


class EvaluateTests(unittest.TestCase):
    def test_answers_and_record(self):
        t = FakeTransport([(200, ok_body(TWO_QUESTIONS))])
        j = judgment.evaluate({"x": 1}, TWO_QUESTIONS, key=FAKE_KEY, transport=t, sleep=lambda s: None)
        self.assertEqual(j.answers["yes"]["noul"], 0.04)
        self.assertEqual(j.model, "jev-stub")
        self.assertEqual(j.attempts, 1)
        self.assertEqual(t.calls[0]["body"]["model"], judgment.DEFAULT_MODEL)
        self.assertEqual(t.calls[0]["key"], FAKE_KEY)
        rec = j.to_record()
        self.assertEqual(rec["state_sha256"], j.state_sha256())
        self.assertNotIn(FAKE_KEY, json.dumps(rec))

    def test_retries_then_succeeds(self):
        t = FakeTransport([(429, "slow down"), (529, "overloaded"), (200, ok_body(TWO_QUESTIONS))])
        naps = []
        j = judgment.evaluate("s", TWO_QUESTIONS, key=FAKE_KEY, transport=t, sleep=naps.append)
        self.assertEqual(j.attempts, 3)
        self.assertEqual([r["http_status"] for r in j.retries], [429, 529])
        self.assertEqual(naps, [0.5, 1.0])

    def test_retries_exhausted_fails(self):
        t = FakeTransport([(429, "a"), (429, "b")])
        with self.assertRaises(judgment.JudgmentError) as cm:
            judgment.evaluate("s", TWO_QUESTIONS, key=FAKE_KEY, transport=t, attempts=2, sleep=lambda s: None)
        self.assertEqual(cm.exception.outcome, "failed")
        self.assertIn("429 after 2 attempts", cm.exception.detail)

    def test_rejected_key_and_bad_request_fail(self):
        for status, fragment in ((401, "rejected"), (422, "422"), (500, "HTTP 500")):
            with self.subTest(status=status):
                t = FakeTransport([(status, "{}")])
                with self.assertRaises(judgment.JudgmentError) as cm:
                    judgment.evaluate("s", TWO_QUESTIONS, key=FAKE_KEY, transport=t)
                self.assertEqual(cm.exception.outcome, "failed")
                self.assertIn(fragment, cm.exception.detail)

    def test_incomplete_or_malformed_answers_fail(self):
        partial = json.dumps({"model": "m", "answers": {"yes": {"type": "noul", "noul": 0.5}}})
        strings = json.dumps({"model": "m", "answers": {"yes": "no", "pick": ["a"]}})
        for text, fragment in ((partial, "missing for: pick"), ("not json", "not JSON"), ('{"model": "m"}', "no answers"),
                               (strings, "not objects for: pick, yes")):
            with self.subTest(text=text):
                with self.assertRaises(judgment.JudgmentError) as cm:
                    judgment.evaluate("s", TWO_QUESTIONS, key=FAKE_KEY, transport=FakeTransport([(200, text)]))
                self.assertIn(fragment, cm.exception.detail)

    def test_invalid_questions_never_reach_transport(self):
        t = FakeTransport([])
        bad = {"q": {"type": "choice", "instructions": "x"}}
        with self.assertRaises(judgment.JudgmentError):
            judgment.evaluate("s", bad, key=FAKE_KEY, transport=t)
        self.assertEqual(t.calls, [])
        self.assertEqual(judgment.validate_questions({}), ["questions must be a non-empty object keyed by question id"])
        self.assertIn("q: a score needs a criteria list of ordered levels",
                      judgment.validate_questions({"q": {"type": "score", "instructions": "x", "criteria": {}}}))
        self.assertEqual(judgment.validate_questions(TWO_QUESTIONS), [])

    def test_endpoint_override(self):
        self.assertEqual(judgment.endpoint_url({}), judgment.API_URL)
        self.assertEqual(judgment.endpoint_url({judgment.ENDPOINT_ENV: "http://127.0.0.1:1/x"}), "http://127.0.0.1:1/x")


class KeyTests(unittest.TestCase):
    def test_parse_dotenv(self):
        text = "# c\nexport A=1\nB = 'two'\nC=\"three\"\nbad line\n=nothing\nD=\n"
        self.assertEqual(judgment.parse_dotenv(text), {"A": "1", "B": "two", "C": "three", "D": ""})

    def test_find_api_key_precedence(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self.assertEqual(judgment.find_api_key(root, {}), (None, "none"))
            (root / ".env").write_text(f"{judgment.KEY_ENV}={FAKE_KEY}\n")
            self.assertEqual(judgment.find_api_key(root, {}), (FAKE_KEY, ".env"))
            self.assertEqual(judgment.find_api_key(root, {judgment.KEY_ENV: "env-key-0123456789"}), ("env-key-0123456789", "environment"))
            self.assertEqual(judgment.find_api_key(root, {judgment.KEY_ENV: "  "}), (FAKE_KEY, ".env"))
            (root / ".env").write_text(f"{judgment.KEY_ENV}=\n")
            self.assertEqual(judgment.find_api_key(root, {}), (None, "none"))

    def test_key_problem_and_invalid_sources(self):
        self.assertIsNone(judgment.key_problem(FAKE_KEY))
        for bad in ('ts "quoted" key', "ts\nkey-0123456789", "ts key 0123456789", "ts\\key-0123456789", "short"):
            with self.subTest(bad=bad):
                self.assertIsNotNone(judgment.key_problem(bad))
        with tempfile.TemporaryDirectory() as tmp:
            key, source = judgment.find_api_key(Path(tmp), {judgment.KEY_ENV: 'ts "quoted" 0123456789'})
            self.assertIsNone(key)
            self.assertTrue(source.startswith("invalid (environment)"), source)
            (Path(tmp) / ".env").write_text(f"{judgment.KEY_ENV}=has space 0123456789\n")
            key, source = judgment.find_api_key(Path(tmp), {})
            self.assertIsNone(key)
            self.assertTrue(source.startswith("invalid (.env)"), source)

    def test_redact(self):
        data = {"checks": [{"detail": f"curl said Bearer {FAKE_KEY} twice {FAKE_KEY}"}], "n": 1}
        out = judgment.redact(data, FAKE_KEY)
        self.assertNotIn(FAKE_KEY, json.dumps(out))
        self.assertIn("<redacted-api-key>", out["checks"][0]["detail"])
        self.assertIs(judgment.redact(data, None), data)
        self.assertEqual(judgment.redact({"a": 1}, FAKE_KEY), {"a": 1})

    def test_write_judgment_refuses_key_leak(self):
        j = judgment.evaluate({"leak": FAKE_KEY}, TWO_QUESTIONS, key=FAKE_KEY,
                              transport=FakeTransport([(200, ok_body(TWO_QUESTIONS))]))
        with tempfile.TemporaryDirectory() as tmp:
            with self.assertRaises(judgment.JudgmentError):
                judgment.write_judgment(j, Path(tmp) / "j.json", FAKE_KEY)
            self.assertFalse((Path(tmp) / "j.json").exists())


class EvidenceTests(unittest.TestCase):
    def test_record_state(self):
        text = "# R-9999 - a title\n\nStatus: **verified finding**\n\n## Question\nq\n\n## Measurements\nm\n"
        state = judgment.record_state(Path("docs/research/R-9999.md"), text)
        self.assertEqual(state["title"], "R-9999 - a title")
        self.assertEqual(state["status_line"], "**verified finding**")
        self.assertEqual(state["sections"], ["Question", "Measurements"])
        self.assertEqual(state["text"], text)
        bullet = judgment.record_state(Path("x.md"), "- Status: observed\n")
        self.assertEqual(bullet["status_line"], "observed")

    def test_evidence_flags(self):
        answers = answers_for(judgment.EVIDENCE_QUESTIONS)
        flags = {f["question"]: f for f in judgment.evidence_flags(answers)}
        self.assertEqual(set(flags), set(judgment.EVIDENCE_QUESTIONS))
        self.assertTrue(flags["independent_check_named"]["flagged"])  # 0.04 < 0.5
        self.assertTrue(flags["status_supported"]["flagged"])  # overclaimed at 0.85
        answers["status_supported"]["confidence"] = 0.3
        answers["reproducible"]["noul"] = 0.9
        flags = {f["question"]: f for f in judgment.evidence_flags(answers)}
        self.assertFalse(flags["status_supported"]["flagged"])
        self.assertFalse(flags["reproducible"]["flagged"])
        self.assertFalse(judgment.evidence_flags({})[0]["flagged"])  # no answer: not flagged
        odd = {qid: "not an object" for qid in judgment.EVIDENCE_QUESTIONS}
        self.assertFalse(any(f["flagged"] for f in judgment.evidence_flags(odd)))


# ----------------------------------------------------------- stub server


class StubHandler(http.server.BaseHTTPRequestHandler):
    script: list = []
    requests: list = []
    delay: float = 0.0

    def do_POST(self):  # noqa: N802
        length = int(self.headers.get("Content-Length") or 0)
        body = self.rfile.read(length)
        type(self).requests.append({"path": self.path, "authorization": self.headers.get("Authorization"),
                                    "body": json.loads(body) if body else None})
        if type(self).delay:
            time.sleep(type(self).delay)
        status, text = type(self).script.pop(0) if type(self).script else (500, "script exhausted")
        data = text.encode()
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def log_message(self, *args):  # silence
        pass


class CliTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.server = http.server.ThreadingHTTPServer(("127.0.0.1", 0), StubHandler)
        cls.thread = threading.Thread(target=cls.server.serve_forever, daemon=True)
        cls.thread.start()
        cls.url = f"http://127.0.0.1:{cls.server.server_address[1]}/v1/systemone"

    @classmethod
    def tearDownClass(cls):
        cls.server.shutdown()
        cls.server.server_close()

    def setUp(self):
        StubHandler.script = []
        StubHandler.requests = []
        StubHandler.delay = 0.0
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)
        self.out = self.root / "out"

    def tearDown(self):
        self.tmp.cleanup()

    def run_cli(self, *args, key=FAKE_KEY, dotenv=None, timeout=30.0):
        env = {k: v for k, v in os.environ.items() if k != judgment.KEY_ENV}
        env[judgment.ENDPOINT_ENV] = self.url
        if key is not None:
            env[judgment.KEY_ENV] = key
        if dotenv is not None:
            (self.root / ".env").write_text(dotenv)
        report = self.root / "report.json"
        proc = subprocess.run(
            [sys.executable, str(PROJECT), "judge", *args, "--root", str(self.root), "--out", str(self.out),
             "--report", str(report), "--timeout", str(timeout)],
            capture_output=True, text=True, timeout=120, env=env,
        )
        rep = json.loads(report.read_text()) if report.is_file() else None
        return proc, rep

    @staticmethod
    def checks(rep):
        return {c["name"]: c for c in rep["checks"]}

    def test_ping_passes_and_records_artifact(self):
        StubHandler.script = [(200, json.dumps({"model": "jev-stub", "answers": {
            "check_named": {"type": "noul", "noul": 0.04},
            "status_supported": {"type": "choice", "choice": "unsupported", "confidence": 0.83,
                                 "probabilities": {"unsupported": 0.89, "supported": 0.07, "not_checkable": 0.04}}},
            "usage": {"input_tokens": 421, "output_tokens": 59}}))]
        proc, rep = self.run_cli("ping")
        self.assertEqual(proc.returncode, EXIT_OK, proc.stderr)
        c = self.checks(rep)
        self.assertEqual(c["typesafe_api_key"]["outcome"], "passed")
        self.assertEqual(c["judgment:ping"]["outcome"], "passed")
        self.assertEqual(c["judgment:ping"]["model"], "jev-stub")
        self.assertEqual(c["ping_answers_as_expected"]["outcome"], "passed")
        self.assertFalse(c["ping_answers_as_expected"]["required"])
        self.assertEqual(rep["status"], "passed")
        # The stub saw the bearer header; the artifact and the report never carry the key.
        self.assertEqual(StubHandler.requests[0]["authorization"], f"Bearer {FAKE_KEY}")
        self.assertEqual(StubHandler.requests[0]["body"]["model"], judgment.DEFAULT_MODEL)
        artifact = self.out / "ping.json"
        self.assertTrue(artifact.is_file())
        self.assertNotIn(FAKE_KEY, artifact.read_text())
        self.assertNotIn(FAKE_KEY, (self.root / "report.json").read_text())
        self.assertEqual(Path(rep["artifacts"][0]["path"]).resolve(), artifact.resolve())
        record = json.loads(artifact.read_text())
        self.assertEqual(record["usage"]["input_tokens"], 421)
        self.assertEqual(record["key_source"], "environment")
        self.assertIn("check_named", json.loads(proc.stdout))

    def test_ping_without_key_is_missing_prerequisite(self):
        proc, rep = self.run_cli("ping", key=None)
        self.assertEqual(proc.returncode, EXIT_MISSING_PREREQUISITE)
        self.assertEqual(self.checks(rep)["typesafe_api_key"]["outcome"], "missing")
        self.assertEqual(StubHandler.requests, [])

    def test_key_from_dotenv(self):
        StubHandler.script = [(200, ok_body(judge_ping_questions()))]
        proc, rep = self.run_cli("ping", key=None, dotenv=f"{judgment.KEY_ENV}='{FAKE_KEY}'\n")
        self.assertEqual(proc.returncode, EXIT_OK, proc.stderr)
        self.assertEqual(self.checks(rep)["typesafe_api_key"]["detail"], "from .env")
        self.assertEqual(StubHandler.requests[0]["authorization"], f"Bearer {FAKE_KEY}")

    def test_rejected_key_fails(self):
        StubHandler.script = [(401, '{"error":"unauthorized"}')]
        proc, rep = self.run_cli("ping")
        self.assertEqual(proc.returncode, EXIT_FAILURE)
        self.assertEqual(self.checks(rep)["judgment:ping"]["outcome"], "failed")
        self.assertIn("rejected", self.checks(rep)["judgment:ping"]["detail"])

    def test_retry_then_success_is_recorded(self):
        StubHandler.script = [(429, "busy"), (200, ok_body(judge_ping_questions()))]
        proc, rep = self.run_cli("ping")
        self.assertEqual(proc.returncode, EXIT_OK, proc.stderr)
        self.assertEqual(self.checks(rep)["judgment:ping"]["attempts"], 2)
        self.assertEqual(len(StubHandler.requests), 2)

    def test_timeout_exit_code(self):
        StubHandler.delay = 3.0
        StubHandler.script = [(200, ok_body(judge_ping_questions()))]
        proc, rep = self.run_cli("ping", timeout=1.0)
        self.assertEqual(proc.returncode, EXIT_TIMEOUT, proc.stderr)
        self.assertEqual(self.checks(rep)["judgment:ping"]["outcome"], "timeout")

    def test_ask_invalid_inputs(self):
        (self.root / "state.json").write_text('{"a": 1}')
        (self.root / "bad.json").write_text("{nope")
        (self.root / "badq.json").write_text('{"q": {"type": "riddle", "instructions": "x"}}')
        proc, rep = self.run_cli("ask", "--state", str(self.root / "state.json"), "--questions", str(self.root / "bad.json"))
        self.assertEqual(proc.returncode, EXIT_INVALID_INPUT)
        proc, rep = self.run_cli("ask", "--state", str(self.root / "state.json"), "--questions", str(self.root / "badq.json"))
        self.assertEqual(proc.returncode, EXIT_INVALID_INPUT)
        self.assertIn("type must be one of", self.checks(rep)["questions"]["detail"])
        proc, rep = self.run_cli("ask", "--state", str(self.root / "missing.json"), "--questions", str(self.root / "state.json"))
        self.assertEqual(proc.returncode, EXIT_INVALID_INPUT)
        self.assertEqual(StubHandler.requests, [])

    def test_ask_round_trip(self):
        (self.root / "state.json").write_text(json.dumps({"doc": "hello"}))
        (self.root / "q.json").write_text(json.dumps(TWO_QUESTIONS))
        StubHandler.script = [(200, ok_body(TWO_QUESTIONS))]
        proc, rep = self.run_cli("ask", "--state", str(self.root / "state.json"), "--questions", str(self.root / "q.json"),
                                 "--name", "hello")
        self.assertEqual(proc.returncode, EXIT_OK, proc.stderr)
        self.assertEqual(json.loads(proc.stdout)["pick"]["choice"], "a")
        self.assertTrue((self.out / "hello.json").is_file())
        self.assertEqual(StubHandler.requests[0]["body"]["state"], {"doc": "hello"})
        self.assertEqual(StubHandler.requests[0]["body"]["questions"], TWO_QUESTIONS)

    def test_evidence_lint_flags_are_optional(self):
        record = self.root / "R-9999-synthetic.md"
        record.write_text("# R-9999 - synthetic\n\nStatus: **verified finding**\n\n## Observation\nA value.\n")
        StubHandler.script = [(200, ok_body(judgment.EVIDENCE_QUESTIONS))]
        proc, rep = self.run_cli("evidence-lint", "--record", str(record))
        self.assertEqual(proc.returncode, EXIT_OK, proc.stderr)  # flags never fail the run
        c = self.checks(rep)
        self.assertEqual(c["records"]["detail"], "1 record(s)")
        self.assertEqual(c["judgment:R-9999-synthetic"]["outcome"], "passed")
        flagged = c["lint:R-9999-synthetic:independent_check_named"]
        self.assertEqual(flagged["outcome"], "failed")
        self.assertFalse(flagged["required"])
        self.assertEqual(c["lint:R-9999-synthetic:status_supported"]["outcome"], "failed")
        self.assertEqual(rep["status"], "passed")
        sent = StubHandler.requests[0]["body"]
        self.assertEqual(sent["state"]["status_line"], "**verified finding**")
        self.assertEqual(sent["state"]["sections"], ["Observation"])
        self.assertEqual(set(sent["questions"]), set(judgment.EVIDENCE_QUESTIONS))
        self.assertIn("record:R-9999-synthetic.md", rep["inputs"])
        summary = json.loads((self.out / "evidence-lint.json").read_text())
        self.assertEqual(summary[str(record)]["artifact"], "R-9999-synthetic.json")

    def test_evidence_lint_refuses_duplicate_stems_and_private_records(self):
        for d in ("dupA", "dupB", "local"):
            (self.root / d).mkdir()
            (self.root / d / "R-0001.md").write_text("# R-0001\n\nStatus: observed\n")
        proc, rep = self.run_cli("evidence-lint", "--record", str(self.root / "dupA" / "R-0001.md"),
                                 "--record", str(self.root / "dupB" / "R-0001.md"))
        self.assertEqual(proc.returncode, EXIT_INVALID_INPUT)
        self.assertIn("share the artifact name", self.checks(rep)["records"]["detail"])
        proc, rep = self.run_cli("evidence-lint", "--record", str(self.root / "local" / "R-0001.md"))
        self.assertEqual(proc.returncode, EXIT_INVALID_INPUT)
        self.assertIn("judge only tracked records", self.checks(rep)["records"]["detail"])
        (self.root / "evidence-lint.md").write_text("# reserved\n")
        proc, rep = self.run_cli("evidence-lint", "--record", str(self.root / "evidence-lint.md"))
        self.assertEqual(proc.returncode, EXIT_INVALID_INPUT)
        self.assertIn("reserved", self.checks(rep)["records"]["detail"])
        self.assertEqual(StubHandler.requests, [])
        self.assertFalse(self.out.exists())

    def test_ask_name_and_argument_validation(self):
        (self.root / "state.json").write_text('{"a": 1}')
        (self.root / "q.json").write_text(json.dumps(TWO_QUESTIONS))
        base = ("ask", "--state", str(self.root / "state.json"), "--questions", str(self.root / "q.json"))
        proc, rep = self.run_cli(*base, "--name", "../../escaped")
        self.assertEqual(proc.returncode, EXIT_INVALID_INPUT)
        self.assertIn("--name", self.checks(rep)["arguments"]["detail"])
        self.assertFalse((self.root.parent / "escaped.json").exists())
        proc, rep = self.run_cli(*base, "--attempts", "0")
        self.assertEqual(proc.returncode, EXIT_INVALID_INPUT)
        self.assertIn("--attempts", self.checks(rep)["arguments"]["detail"])
        proc, rep = self.run_cli(*base, timeout=-5)
        self.assertEqual(proc.returncode, EXIT_INVALID_INPUT)
        self.assertIn("--timeout", self.checks(rep)["arguments"]["detail"])
        self.assertEqual(StubHandler.requests, [])
        # A fractional timeout is passed through, not truncated to a whole second.
        StubHandler.script = [(200, ok_body(TWO_QUESTIONS))]
        proc, rep = self.run_cli(*base, "--name", "frac.tion-1", timeout=0.5)
        self.assertEqual(proc.returncode, EXIT_OK, proc.stderr)
        self.assertTrue((self.out / "frac.tion-1.json").is_file())

    def test_key_in_state_fails_with_a_report_and_no_artifact(self):
        (self.root / "state.json").write_text(json.dumps({"leak": FAKE_KEY}))
        (self.root / "q.json").write_text(json.dumps(TWO_QUESTIONS))
        StubHandler.script = [(200, ok_body(TWO_QUESTIONS))]
        proc, rep = self.run_cli("ask", "--state", str(self.root / "state.json"), "--questions", str(self.root / "q.json"))
        self.assertEqual(proc.returncode, EXIT_FAILURE, proc.stderr)
        self.assertIsNotNone(rep)
        self.assertIn("contains the API key", self.checks(rep)["judgment:ask"]["detail"])
        self.assertFalse((self.out / "ask.json").exists())
        self.assertNotIn(FAKE_KEY, (self.root / "report.json").read_text())
        self.assertNotIn("Traceback", proc.stderr)

    def test_non_object_answers_fail_with_a_report(self):
        StubHandler.script = [(200, json.dumps({"model": "m", "answers": {"check_named": "x", "status_supported": "y"}}))]
        proc, rep = self.run_cli("ping")
        self.assertEqual(proc.returncode, EXIT_FAILURE, proc.stderr)
        self.assertIn("not objects", self.checks(rep)["judgment:ping"]["detail"])
        self.assertNotIn("Traceback", proc.stderr)
        self.assertFalse((self.out / "ping.json").exists())

    def test_report_is_redacted_end_to_end(self):
        # A server error whose body echoes the key lands in the check detail;
        # the written report must not carry it.
        StubHandler.script = [(500, json.dumps({"error": f"bad header Bearer {FAKE_KEY}"}))]
        proc, rep = self.run_cli("ping")
        self.assertEqual(proc.returncode, EXIT_FAILURE)
        text = (self.root / "report.json").read_text()
        self.assertNotIn(FAKE_KEY, text)
        self.assertIn("<redacted-api-key>", self.checks(rep)["judgment:ping"]["detail"])
        self.assertIn("redacted", rep)
        self.assertNotIn(FAKE_KEY, proc.stderr)

    def test_malformed_key_is_refused_before_any_request(self):
        proc, rep = self.run_cli("ping", key='ts "quoted" 0123456789')
        self.assertEqual(proc.returncode, EXIT_FAILURE)
        c = self.checks(rep)["typesafe_api_key"]
        self.assertEqual(c["outcome"], "failed")
        self.assertIn("invalid (environment)", c["detail"])
        self.assertNotIn("quoted", c["detail"])
        self.assertEqual(StubHandler.requests, [])

    def test_evidence_lint_unreadable_record(self):
        proc, rep = self.run_cli("evidence-lint", "--record", str(self.root / "nope.md"))
        self.assertEqual(proc.returncode, EXIT_INVALID_INPUT)
        self.assertEqual(StubHandler.requests, [])


def judge_ping_questions() -> dict:
    from unirally_lab import judge_commands
    return judge_commands.PING_QUESTIONS


class DoctorTests(unittest.TestCase):
    def test_doctor_reports_optional_key(self):
        env = {k: v for k, v in os.environ.items() if k != judgment.KEY_ENV}
        with tempfile.TemporaryDirectory() as tmp:
            report = Path(tmp) / "doctor.json"
            subprocess.run([sys.executable, str(PROJECT), "doctor", "--root", tmp, "--report", str(report)],
                           capture_output=True, text=True, timeout=300, env=env)
            check = {c["name"]: c for c in json.loads(report.read_text())["checks"]}["typesafe_api_key"]
            self.assertEqual(check["outcome"], "missing")
            self.assertFalse(check["required"])
            (Path(tmp) / ".env").write_text(f"{judgment.KEY_ENV}={FAKE_KEY}\n")
            subprocess.run([sys.executable, str(PROJECT), "doctor", "--root", tmp, "--report", str(report)],
                           capture_output=True, text=True, timeout=300, env=env)
            text = report.read_text()
            check = {c["name"]: c for c in json.loads(text)["checks"]}["typesafe_api_key"]
            self.assertEqual((check["outcome"], check["detail"]), ("passed", "from .env"))
            self.assertNotIn(FAKE_KEY, text)


if __name__ == "__main__":
    unittest.main()
