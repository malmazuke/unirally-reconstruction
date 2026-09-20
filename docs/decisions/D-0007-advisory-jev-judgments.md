# D-0007 - Advisory Jev judgments in the harness

Status: adopted 20 September 2026 with the integration of
[JEV-JUDGMENT-HARNESS](../../tasks/JEV-JUDGMENT-HARNESS.md) (proposed 19
September with that task). Revisit when a judgment is proposed as a required check, when
the model behind `jev-latest` moves, or when the questions are tuned against
labelled records.

## Context

Every semantic judgment in this workflow is made by hand inside a long model
context and recorded in Markdown: whether a record names its independent check,
whether a handoff claims a skipped check as a pass, whether a worker turn touches
an out-of-scope item, which record explains a divergence. These are typed
yes/no or one-of-several decisions over a few kilobytes of text. TypeSafe's Jev
(a System One model) answers such questions with calibrated probabilities in
under a second for a fraction of a cent, through one HTTP endpoint.

The project's rule is that confidence follows evidence, not a model's certainty
([EVIDENCE template](../templates/EVIDENCE.md)), and that only a `passed`
required check counts toward acceptance ([report.py](../../tools/unirally_lab/report.py)).
A judgment is not evidence of a gameplay fact and must not become a gate by
accident.

## Decision

- A Jev judgment is **advisory**. Every check derived from an answer (a lint
  flag, an expected-answer check) is optional, never `required`, so a flagged
  record or handoff informs the reader and never changes a run's status or
  exit code. The one required check a `judge` command carries,
  `judgment:<name>`, records only whether the call itself succeeded; it decides
  that command's own exit code and nothing else consumes it. Promoting an
  answer-derived check to required needs a decision record with the labelled
  evaluation behind the threshold.
- What is sent leaves the machine. Judge tracked records and authored state
  only; never captures, dumps or anything under `local/`, which
  `evidence-lint` refuses outright.
- Every call is **evidence**: the full request (state, questions, model), the
  response, the versioned model that answered, token usage, elapsed time, the
  transport and a SHA-256 of the state are written under `artifacts/` and the
  run report cites the file. A record that cites a judgment links that artifact.
- The tooling stays **standard library only** and calls the HTTP endpoint the
  way the toolchain downloader fetches wheels (curl first, urllib fallback); the
  vendor SDK is not a dependency.
- The key is private: `TYPESAFE_API_KEY` in the environment or the ignored `.env`
  (template `.env.example`), read only by `judge` commands, never on a command
  line, in a report or in an artifact. `doctor` reports its presence as an
  optional check. The synthetic CI has no key and no `judge` step; it is
  unaffected.
- `jev-latest` is the default model; the response's versioned id is what a record
  cites. Pin a version when a threshold has been tuned against it.
- Thresholds in `evidence-lint` (noul below 0.5, an `overclaimed` choice at
  confidence 0.6 or above) are starting points to evaluate on this project's
  records, not calibrated limits.

## Consequences

Agents can offload repeated, bounded judgments (scope drift, stopping rules,
handoff overclaim, evidence-record hygiene, review pre-triage) to a cheap call
with a recorded answer, and reviewers can see exactly what was asked. Nothing
about acceptance, gates, differential compares or native code changes; the
core emulation and the compares remain deterministic and model-free. A network
or vendor outage degrades a `judge` command to `missing` or `failed`, which no
required check depends on.

## Alternatives

- Vendor SDK dependency: rejected while the tooling has no package installs.
- Required lint gates: rejected until thresholds are evaluated on labelled records.
- Keeping all judgments in the primary model's context: the status quo; costly
  per turn and self-judged, which is the conflict of interest this removes.
