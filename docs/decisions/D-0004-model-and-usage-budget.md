# D-0004 — Model routing and usage conservation

Status: adopted, 12 September 2026. Reassess from measured accepted work.

## M4-16 continuation override — 14 September 2026

After the incomplete recovery checkpoint, the user explicitly instructed:
“You can ignore usage boundaries. Continue”. For this active M4-16 execution,
ignore the percentage stop/reserve boundaries and continue toward acceptance.
Retain usage telemetry for evidence, independent review and task/provider scope.
This overrides the M4-16 percentage limits below; it does not authorize credit
redemption, purchases, paid fallback or a provider change. Actual tool/account
unavailability remains a resource limitation, not an invented percentage cutoff.

The user subsequently requested committing a handover because usage was nearly
exhausted (latest sample94%). Stop this run after recovery/synchronization; the
prior continuation override does not imply automatic continuation past that
new request. Preserve the no-spending/reset/provider-change boundary.

## M4-16 provider move to Claude Opus 5 — 14 September 2026

The user reported that GPT Astra and Fable 5.1 credits are exhausted and
instructed resuming the incomplete M4-16 execution on Claude Opus 5. This
document's rule is that a task and its children stay with the starting provider
*unless the user moves it*; the user has moved it, so Claude Opus 5 is the
authorized primary for the remainder of M4-16. This authorizes no reset, credit
purchase or paid fallback, and it does not reopen any other task's provider.

Two consequences are recorded rather than silently absorbed:

- The automatic independent reviewer required by D-0006 becomes a fresh Claude
  Opus 5 subagent in an isolated checkout, because Sol/medium is not available
  under the new provider. Isolated checkout, no inherited conversation, explicit
  model/effort and exact-candidate review are unchanged; review records must
  name the actual reviewing model.
- Shared weekly percentage telemetry does not carry across providers and this
  harness exposes no equivalent tool. Prior samples stay as OpenAI-account
  history. Continue recording UTC wall clock and session context budget instead
  of inventing a percentage. The M4-16 continuation override already superseded
  the percentage stop and reserve boundaries, so nothing depends on the missing
  figure; a genuine resource or access limit is still reported as one.

## M4-16 playable-track extension

The user authorized preparation of [M4-16](../../tasks/M4-16.md) after M4-15.
Inherit Astra/medium direct ownership, automatic fresh Sol/medium isolated review,
one active child, targeted high escalation only for recorded reasoning difficulty,
no 20-point task cap and the final 20% weekly review/recovery reserve. These
exceptions override general defaults for M4-16. Preparation observed 56% shared
weekly used; sample fresh at startup and checkpoints. No reset, purchase or
provider switch is authorized. All coupled product dependencies stay in the task.
Gameplay starts in the next user-started session; prior no-M4-16 boundaries are
historical. Stop after acceptance or a real resource/access boundary, not a
checkpoint. No automatic M4-17 dispatch.

## M4-15 race-completion extension

The user authorized preparation of [M4-15](../../tasks/M4-15.md) after assessing
M4-14. Inherit its Astra/medium direct ownership, automatic fresh Sol/medium
review, one active child, targeted high escalation only for recorded reasoning
difficulty, no 20-point task cap and final 20% weekly review/recovery reserve.
These exceptions override general defaults for M4-15. Preparation observed 37%
shared weekly used; implementation must sample fresh usage. No reset, purchase
or provider switch is authorized. Reference feasibility is inside the same task;
checkpoints do not end it. Gameplay starts in the next user-started session.
Prior no-M4-15 dispatch statements describe completed assignments; this new
assignment supersedes that boundary. Do not automatically dispatch M4-16.

## M4-14 sustained-traversal extension

After M4-13 and its follow-up, the user selected a more ambitious continuous-riding
assignment: [M4-14](../../tasks/M4-14.md). It inherits M4-13's operational rules
below: Astra/medium direct implementation, targeted high escalation only for a
recorded reasoning difficulty, automatic fresh Sol/medium review, one active
child, no 20-point task cap and a final 20% weekly review/recovery reserve.
These exceptions override general defaults for M4-14 too; earlier task stop
instructions are historical and do not cancel this newly selected assignment.

Read fresh usage at startup (preparation observed 24% used, zero reset credits).
No reset, purchase or provider switch is authorized. Internal mechanics and
routine checkpoints must not become new user-facing task boundaries. Work until
the sustained capability is accepted or a genuine resource/access boundary is
reached; do not dispatch M4-15 automatically. This session prepares the handoff;
gameplay starts in the next user-started Astra/medium implementation session.

## M4-13 trial extension

Following the M4-12 retrospective, the user authorized preparation for one more
capability trial, [M4-13](../../tasks/M4-13.md). This section overrides conflicting
general defaults for M4-13 only; the historical M4-12 exception remains below.

- Astra/medium, standard service, directly owns investigation and implementation.
  No separate planner or routine implementation worker. A recorded unresolved
  reasoning difficulty permits targeted high escalation under the same bounded
  consultation mechanism described for M4-12.
- The primary automatically spawns a fresh Sol/medium reviewer with explicit
  model/effort, no inherited conversation, and an isolated candidate checkout.
  At most one active child. Automatically handle fixes, re-review and integration.
- The 20-percentage-point task cap is waived. Sample usage at start and durable
  checkpoints; retain the final 20% weekly allowance for review/fixes/integration/
  recovery. At 80% used, stop scope expansion; incomplete capability stays
  unaccepted. Track total usage across primary/children and resumed sessions.
- Preparation observed 13% weekly used and zero available reset credits; neither
  observation establishes future capacity. Read fresh telemetry at startup.
  Below the reserve, do recovery only until allowance is restored. Unknown usage
  is not free capacity; recover visibility before discretionary frontier work.
- No reset redemption, purchases, paid API fallback or provider switch is
  authorized. Any future reset requires explicit confirmation for that specific
  credit. An actual provider block stops new dispatch without retry loops.
- Start gameplay only in the next user-started M4-13 session. End at acceptance
  or a genuine resource/access boundary; do not automatically claim M4-14.

## M4-12 trial exception

The user approved preparing the [D-0006](D-0006-capability-driven-work.md) trial
on 13 September 2026. For M4-12 only, this section overrides conflicting defaults
below and earlier handoffs:

- Start the implementation session explicitly on `gpt-6-astra`/medium, standard
  service mode. Astra performs planning, recovery and native implementation
  itself. No mandatory consultation or ten-minute frontier-role limit applies
  to this assigned primary. High is a targeted escalation for a recorded
  unresolved reasoning difficulty, not the initial setting. If the runtime
  cannot change the primary effort, use at most one bounded Astra/high child
  for that specific question; do not claim an effort change that did not occur.
- The primary automatically launches independent review as `gpt-5.6-sol`/medium
  with `fork_turns: "none"`, an explicit candidate and isolated checkout. One
  active child maximum for this trial. No frontier swarm or additional planner.
  Routine Sol defaults remain appropriate outside this trial.
- Waive the 20-percentage-point discretionary task cap for M4-12. Preserve
  usage sampling and the final 20% weekly review/recovery reserve. At 80% used,
  stop expanding implementation scope; use remaining capacity for independent
  review of a viable candidate, corrections, integration and durable recovery.
  If the capability is incomplete, preserve it as unaccepted. Checkpoints and
  child/session changes never reset cumulative trial accounting.
- Freshly sample usage at start; the preparation-time observation was 98% used
  with one reset available, not a baseline for future work. Below 20% remaining,
  do startup/recovery only until allowance is restored. Do not begin the
  substantive trial by treating the current 2% as sufficient capacity.
- Reset availability is not authorization to redeem it. No reset was consumed
  by this preparation. The earlier permission for an actual-block reset belonged
  to the wrapped M4 run; do not assume it transfers. A user-performed reset or
  explicit confirmation to redeem one specific reset is required. The trial may
  use at most one explicitly authorized reset, never buy credits or use a paid
  API fallback. Record the reset outcome and fresh usage baseline if it occurs;
  do not repeat an uncertain redemption with a new attempt identifier.
- Unknown telemetry is unknown capacity: checkpoint and restore visibility
  before new discretionary frontier work. An actual provider block stops new
  dispatch; preserve progress without retry loops. Do not switch providers.

The current request prepares this policy and the next-session handoff; it does
not launch M4-12 or redeem a reset. Its concise documentation review is recovery/
preparation work within the remaining allowance. General model/budget defaults
below continue to apply outside M4-12.

## Context

The user reports exhausting a weekly Codex allowance in about one hour with
Astra, compared with about 24 hours of work with Fable 5.1 in Claude Code.
These are observations across different providers and workloads, not a
controlled model-cost benchmark. The M2 continuation used a frontier parent
and several children inheriting that model. Parallelism shortened elapsed time
while increasing aggregate usage. Three child sessions subsequently returned
usage-limit errors. The usage snapshot conflicted with those errors; it cannot
support a numerical burn-rate estimate.

## Routing decision

**Keep each task within the provider the user started it with.** An OpenAI
task uses only OpenAI models for coordination, implementation, subagents and
review; an Anthropic task uses only Anthropic models. Model/effort changes within
that provider remain autonomous. This applies to prerequisite tasks and fresh
worker sessions too: creating a child does not authorize crossing providers.
Only the user may move work between platforms. A user-initiated continuation
on another platform establishes the provider for that continuation; historical
commits from another provider do not force a switch back.

The user manages the two subscription allowances separately and reports no
Anthropic weekly allowance remaining at this adjustment. Never assume the other
provider has spare capacity. If the task's provider runs out, checkpoint and
report the resource condition; do not fall back to another provider or ask the
user to make a routine routing decision.

The following defaults apply to OpenAI tasks. For Anthropic tasks, keep routine
work in the selected Anthropic runtime (Opus is the proposed worker candidate),
with bounded Fable consultation/review where justified; never dispatch Sol or
Astra from that task. Record actual supported model settings in either runtime.

| Work | Default | Escalation |
| --- | --- | --- |
| Coordination, task selection, routine planning | Sol, medium reasoning | One bounded frontier consultation before complicated, consequential work, or for a consequential unresolved design question |
| Implementation and ordinary research | Sol, medium reasoning | Narrow the experiment after two unsuccessful bounded attempts; use high reasoning or a frontier consultation when the record explains why |
| Independent component review | Fresh Sol reviewer; medium normally, high for difficult arithmetic | Frontier review for unresolved reviewer disagreement or critical uncertainty |
| Milestone architecture/accuracy audit | Bounded Astra review | Produce findings and a decision, then return execution to the default model |
| Mechanical, low-risk chores | Sol initially; Terra/Luna optional | Adopt only when measured results justify the change |

The frontier consultation is also a **proactive planning role**, not only a
recovery mechanism. Before implementation, use one bounded Astra consultation
when the upcoming work is both complicated and consequential. A task normally
qualifies when it will freeze or replace reference evidence, establish or change
an architecture/schema/content identity, alter accepted behavioral boundaries,
or make a scope decision whose error would invalidate substantial downstream
work. Routine additive implementation, mechanical follow-ups and execution of
an already reviewed plan do not qualify merely because they are lengthy.

The planning consultation receives the task record, relevant evidence and one
concrete request: identify hidden assumptions and failure modes, order the
cheapest discriminating experiments, and recommend pre-implementation gates and
scope boundaries. Record its findings and the coordinator's decisions in the
task. Keep the consultation bounded to ten minutes and return evidence recovery,
implementation and ordinary review to Sol unless a separate escalation criterion
is later met. This is a planning checkpoint, not permission for continuous
frontier coordination.

Use explicit provider model IDs and reasoning settings in each dispatch. Codex
IDs currently used here are `gpt-5.6-sol` and `gpt-6-astra`; do not infer IDs from
marketing names. Fable and Opus run through an available Claude Code runtime,
not as fictional native Codex subagent IDs. Opus is a candidate implementation
or review worker, but its name alone establishes neither cost nor suitability.
Use the user's observed Fable throughput as a reason to compare it, not a
promise of future hours. Never create a paid provider account to switch models.

A frontier consultation receives one question, exact evidence paths and a
required output. This includes the proactive planning consultation above.
Reassess within ten minutes and checkpoint if unresolved;
do not turn it into continuous frontier coordination, routine coding or polling.
A second consultation needs a recorded new question or new evidence. Routine
model selection and reversible adjustments need no user confirmation.

## Starting future sessions

Outside the M4-12/M4-13/M4-14 exceptions above, start new OpenAI work sessions with Sol/medium; start Anthropic sessions with
Opus as the provisional routine coordinator/worker choice. Keep the coordinator
stable and use compact, same-provider frontier consultations when justified.
Do not start every task on a frontier model merely to plan it before switching.

Historical adjustment (superseded for the M4-12 start above): the completed OpenAI run kept Astra as coordinator. At its
clean checkpoint, the user requested preparation for a cheaper coordinator; the
next continuation starts on Sol using tasks/NEXT_SESSION.md.
The new-session default does not require a mid-task coordinator switch. Bounded
Sol workers remain available under the existing scope and quota rules. This is
a continuity preference, not a measured claim that switching models would cost
more: the actual context sent, compaction and caching determine input overhead,
and this session's cross-model cache behavior has not been established.

## Execution and usage guardrails

- Default to the primary plus at most one active child. Sequence implementation
  and independent review. A second child requires a recorded independent scope,
  expected benefit and usable quota; a frontier swarm is not the default.
- Start children with a compact task handoff: exact base, owned paths, relevant
  evidence and acceptance checks. Avoid copying full conversation history. For
  Codex collaboration calls with a model override, use `fork_turns: "none"` and
  supply that self-contained prompt. Explicitly set model and reasoning effort;
  an inherited frontier model is not an acceptable accidental default.
- Sample available account usage before dispatch and at checkpoints. Record the
  provider/window, timestamp, used/remaining percentages and reset, or `unknown`.
  Compare aggregate work across parent and children; account-wide deltas may
  include unrelated work and cannot be attributed precisely to this project.
- Updated policy for tasks started after 12 September 2026: reserve the final
  20% of a weekly allowance for review/recovery; limit discretionary
  implementation to a 20 percentage-point increase from a recorded
  work-session start. This doubles the former 10-point task quota and its
  enforcement guardrail; the recovery reserve is unchanged. Do not reset that
  baseline by spawning a child, rotating tasks or starting another automatic session. At
  either threshold, checkpoint and end discretionary implementation with the
  resource condition recorded. Do not switch providers to bypass the limit.
  These are project defaults chosen in response to the user's request, not
  provider guarantees or newly authorized purchases.
- Unknown or contradictory telemetry is not free capacity: avoid new frontier
  dispatch, use at most one default-model child if the runtime permits, and
  reassess at each checkpoint. An actual usage-limit error stops new dispatches
  to that provider; do not repeatedly retry or respawn. Preserve partial work.
- Never automatically redeem reset credits, buy credits, enable paid API usage
  or upgrade a plan. Existing authority to work is not authority to spend.
- Keep 45-minute reassessment sessions and ten-minute durable checkpoints.
  Use focused checks during edits and the required complete checks on the exact
  review/integration candidate. Do not rerun unchanged broad suites simply to
  keep agents occupied. Preserve independent reproduction, withheld cases and
  frozen expectations; cheaper execution does not weaken acceptance.

The percentage guardrails are sampled, agent-enforced policy. No hard token or
spend limiter, quota monitor, automatic model router or scheduler is implemented.
If a provider exposes an actual spend cap, use it within existing authorization.
Time limits alone cannot guarantee token limits.

## Configuration and measurement

[Project Codex configuration](../../.codex/config.toml) defaults new sessions to
Sol/medium and one Sol/medium child. Explicit session, spawn or custom-agent
settings can override those defaults. It does not switch an already running
frontier session. Other runtimes follow this decision through AGENTS.md and the
portable dispatch record; game code and evidence remain provider-independent.
Use standard service mode; do not opt into Fast mode for routine project work.

Record model, effort, reason, accepted outcome, review/fix rounds, aggregate
agent time and observed quota delta in the task. After three comparable bounded
tasks, compare accepted outcomes per allowance consumed and adjust the defaults.
Do not benchmark with extra artificial workloads or infer efficiency from wall
clock time alone.

User adjustment, 12 September 2026: double the quota for all future tasks,
including the guardrail. This changes the per-task discretionary allowance from
10 to 20 percentage points. It does not authorize reset redemption, purchases,
provider switching or use of the final 20% review/recovery reserve.

Official references checked on 12 September 2026:
[Codex subagent configuration](https://learn.chatgpt.com/docs/agent-configuration/subagents)
and [Codex pricing and usage](https://learn.chatgpt.com/docs/pricing).
The published credit rates imply 2.5 times the credits for Astra versus Sol for
the same input/cache/output token mix. That is not a prediction of subscription
runtime, and does not explain the user's entire observed difference.
