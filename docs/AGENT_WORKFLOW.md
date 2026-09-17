# Model-independent agent workflow

This protocol is intended for humans and agents using different models or runtimes. The repository is the durable record. Provider chat history, hidden memory, model-specific tools and subscription limits are not project dependencies. A scheduler is future implementation, not something already running.

## Roles and ownership

| Role | Responsibility | Write ownership |
| --- | --- | --- |
| Coordinator/integrator | Select ready work, allocate scope, resolve dependencies, accept results and maintain project state | Canonical task registry, integration branch, milestone status |
| Research worker | Recover one bounded behavior, format or routine with evidence | Assigned research/experiment paths on its own branch |
| Implementation worker | Implement a defined contract and its checks | Assigned code/test paths on its own branch |
| Reviewer | Reproduce the claim, inspect evidence, exercise independent cases and identify regressions | Review report; no silent edits to the implementation being reviewed |

Roles do not require four simultaneous agents. For the D-0006 M4-12 through M4-16 trials, the primary is both investigator/implementer and coordinator; it works on a task branch and automatically dispatches fresh Sol/medium independent review before integration. The applicable trial exception in D-0004 takes precedence over the general defaults here. For OpenAI tasks, default to a Sol coordinator and one Sol worker, with explicit model/effort settings. Review uses a fresh sequential session. A second child requires the independent-scope and quota justification in D-0004. A model switch does not change the task's acceptance criteria.

For example, after M0, one worker could investigate track encoding while another identifies rider-state writes. Two workers should not independently rewrite the state schema. The coordinator establishes a small shared interface first and serializes changes to it.

## Model and usage policy

Follow [D-0004](decisions/D-0004-model-and-usage-budget.md) for model routing,
frontier escalation, compact dispatch context and sampled quota guardrails.
Keep the task and all children within its starting provider; only the user
may move work between platforms. Usage exhaustion causes a checkpoint, never
an automatic cross-provider fallback. A user-initiated platform change sets the
provider for that continuation, regardless of the repository's earlier authors.
Sol/medium is the OpenAI default for coordination, implementation and ordinary review;
Anthropic tasks retain Anthropic models as specified in D-0004.
Use frontier models for bounded difficult questions and milestone audits.
Project `.codex/config.toml` supplies defaults for new Codex sessions/children;
explicit runtime settings may override them, so record the actual model.
The budget reserves 20% of weekly allowance and checkpoints discretionary
implementation after a 20 percentage-point increase from its recorded session
baseline. This is the doubled quota and guardrail for tasks started after 12
September 2026. These are agent-enforced limits, not an implemented metering
service. No automatic reset redemption or purchases are authorized.

## Task lifecycle

Use `planned -> ready -> claimed -> in_progress -> review -> accepted`, with `blocked` and `abandoned` as explicit alternatives. `accepted` means merged with required evidence. A worker reporting success only moves work to review. Claims are coordination records, not evidence that work has begun.

The coordinator is the single writer of the canonical task registry. Each task record uses [the template](../tasks/TEMPLATE.md). Before dispatch it needs:

- A concrete outcome and milestone; prerequisites and the exact starting commit.
- Owned paths, interface dependencies, allowed scope and non-goals.
- Acceptance criteria, check commands or the task to implement those commands, and required evidence.
- Required inputs/tools and the fixture-access arrangement.
- A bounded session/time budget, explicit model/effort and routing rationale, starting quota and reserve under D-0004, and any actually authorized monetary limit.
- Worker identity, branch/worktree, claim time, heartbeat and checkpoint location.

On a single workstation, one coordinator can serialize assignments through task files. A later scheduler needs atomic claims and a lease store under ignored runtime state; committing a Markdown file is not a distributed lock. Do not let two coordinators dispatch from independent copies of the same queue.

A stale heartbeat is a reason to inspect the process, not immediately reassign its files. Confirm the former worker is stopped or isolated before issuing a new attempt. Keep attempt history, recovered commits and failure reasons.

## Capability tasks and automatic review

For [D-0006](decisions/D-0006-capability-driven-work.md), task boundaries follow
native outcomes. The primary reproduces a baseline, investigates the first
divergence, implements recovered behavior in native code, compares, and expands
the frozen domain. Coupled routines are internal experiments with small commits.
Keep one sustained primary context; do not require a new worker or review gate
for each checkpoint. Partial research cannot be accepted as a native capability.

Before acceptance, the primary MUST launch one independent review subagent,
explicitly `gpt-5.6-sol`/medium, with no inherited conversation (`fork_turns:
"none"` when using collaboration tools). Create its separate checkout at the
exact candidate before spawning; subagent tools do not isolate files themselves.
Give the reviewer task/evidence paths, owned review output, required independent
cases and the immutable candidate hash. Automatically collect findings, fix on
the implementation branch, request re-review and integrate after approval.
Never make the user start, monitor or relay review. No self-approval fallback.
The primary can do integration preparation while review runs, but must not change
the reviewer's candidate. Use [validation by stage](BUILD_AND_VALIDATION.md#validation-by-stage).

## Worker loop

1. Read project state, the task, relevant decisions and evidence. Inspect the actual branch and working tree; do not assume they match the handoff.
2. Run the environment check and reproduce the baseline relevant to this task. Record missing prerequisites and pre-existing failures.
3. Write the next hypothesis and the cheapest experiment that could reject it. Inspect traces/code, change one bounded behavior, then run the declared check.
4. Record observations separately from interpretation. Preserve useful failed experiments to prevent another agent repeating them.
5. Commit a coherent change when it is ready for review. Check staged files for accidental local inputs. Update the task handoff with the resulting hash.
6. Submit evidence and known limitations. If blocked, give the smallest concrete dependency or decision needed, and let the coordinator select other ready work.

Do not spend the entire session producing source code before running an experiment. An unsuccessful hypothesis with a reproducible trace can be a valuable accepted research result if that was the assigned outcome. Do not accept a failed gameplay implementation under that interpretation.

## Unattended operation and stopping conditions

Operating defaults, adjustable from measured runs under D-0004 (enforced by agents; no scheduler exists):

- One active child by default; a second independent child needs the recorded D-0004 justification.
- A 45-minute reassessment interval within the active task (no mandatory session restart), checkpoint at least every 10 minutes and before an expensive experiment. Larger tasks become several sessions with durable progress.
- After three attempts at the same hypothesis without new evidence, stop that approach. Narrow the experiment, ask for review, or reassign; do not blindly regenerate implementations.
- Apply explicit process timeouts to builds and tests. Separate timeout, crash, unavailable prerequisite and behavioral mismatch in reports.
- On usage exhaustion, provider error or interruption, persist progress when possible; the coordinator can recover the isolated worktree even if the agent could not write a final message.
- Stop dispatching when the active scope is complete, the authorized run budget is exhausted, or no ready task remains. Never let workers recursively expand the budget by spawning more workers.

The scheduler configuration must state total run duration and concurrency. If money is metered, also set an authorized spend cap using the provider's actual metering facilities. Unknown usage is unknown, not zero. These planning defaults do not authorize additional purchases or paid services.

Notify the user when a milestone is accepted, a scope/budget decision is needed, a material failure prevents progress, or a requested run ends. Do not require user approval for every hypothesis, reversible fix or merge that is already within the active assignment. Runtime-required approvals cannot be replaced by this protocol.

Routine integration can proceed automatically only when the task is in scope, a reviewer has accepted the evidence, and the exact merge candidate passes its required checks. Public release, deployment and new service spending remain distinct actions requiring the user's authorization; do not infer those from the aspiration to have online play.

### Internal prerequisites and continuity

When a worker reaches an internal dependency, it checkpoints the evidence and informs the coordinator. The coordinator normally adds coupled dependencies to the current task experiment list under D-0006. Create a separate prerequisite only for independent ownership, a different outcome or justified review size/risk, then resume the dependent work. Do not impose a separate acceptance cycle per recovered producer. A ready prerequisite is work to do, not a reason to end the user request. The 45-minute sessions above are checkpoint/reassessment intervals; they do not create a user-imposed total budget. Do not invent additional stopping budgets. The usage-conservation defaults adopted in D-0004 respond to the user's explicit budget-management request and take precedence over earlier unlimited-session allocations. Preserve actual spending restrictions and runtime limits, and escalate only dependencies that require the user's access, input or authority.

## Source control and integration

At implementation start, initialize a local Git repository if none exists. Record the first planning baseline, then use a stable `main` and short-lived task branches such as `task/M0-02-reference-adapter`. Create one worktree or checkout per active worker; respect any repository/workspace manager already in use.

- Dispatch from an explicit commit. Workers do not modify `main` or another worker's checkout.
- Keep commits focused. Separate reference-baseline changes from implementation changes so a reviewer can see whether both sides of the comparison moved.
- Before acceptance, inspect the diff, run the relevant suite, and record the source and input hashes. Merge dependencies before dependent tasks.
- The coordinator creates the actual merge candidate and runs affected integration checks on it. Concurrently passing branches can fail when combined.
- Resolve conflicts by understanding the interface change and rerunning checks. Do not choose one side wholesale to finish a merge quickly.
- Retain old reference artifacts by hash. A baseline change needs an explained correction and independent confirmation; never overwrite the previous result in place.
- Use revert commits for accepted changes that later prove wrong. Preserve failed task attempts for diagnosis; avoid force-pushing shared history.
- Add milestone tags only when the gate is met, and link the evidence report from project state.
- For this project's existing public `origin`, an accepted integration is not
  complete until the coordinator pushes `main` and verifies `HEAD` equals
  `origin/main`. Push an accepted milestone tag as part of the same completion
  flow. Push task branches when the task's declared remote review or CI requires
  them. A push failure is a reported synchronization failure, not permission to
  describe an ahead-only local branch as fully complete. Never force-push or
  change remote configuration/visibility under this standing authority.

A remote repository is optional for early work. If one is established, use the same task record in the PR description, require checks/review on `main`, and use CI as specified in [build and validation](BUILD_AND_VALIDATION.md). A local integration report provides the equivalent review trail before hosting is configured. Pushing ordinary commits and tags to this project's already-configured public remote is authorized source-control synchronization and makes tracked source and documentation public; creating a public release, changing visibility or deploying remains separately authorized work.

### Consolidated closeout

For M4-14 and subsequent explicitly assigned capability work, finish the reviewed
source, reviewed corrections and acceptance/handoff documentation before the
final main push. Include actual local/review results, the tested code identity
and the path/command for verifying pending remote CI. Mark remote acceptance
conditional until those checks actually succeed; never preclaim a future pass.

Run affected merge checks on the exact candidate, push once, verify the configured
remote ref and wait for that tip's CI. Write the actual final SHA, run URL/result,
finish time and fresh usage in ignored `artifacts/<task>-integration/closeout.json`
and the user completion report. The tracked handoff must point to that artifact
and explain how to recover status from git/GitHub if it is absent. Registry may
say "reviewed/local gates passed; acceptance conditional on final-tip CI and
remote verification"; those recorded conditions plus verified closeout establish
acceptance without another documentation commit. On future unrelated updates,
roll that historical status forward normally.

Do not create a post-success documentation-only commit merely to repeat the CI
result or insert its own hash. If a real source/evidence/documentation correction
is needed, commit it and run the relevant checks, including new final-tip CI;
consolidation does not excuse stale or false evidence. CI triggers are unchanged.

## Handoff and model switching

Provider changes are user-controlled. Within-provider model changes remain autonomous. Persist the same fields for a permitted model/platform change, another machine, or a fresh session:

1. Task, milestone, base/head commits, worktree path and uncommitted changes.
2. Verified facts and links to the supporting experiment/artifacts.
3. Current hypothesis, failed approaches, unresolved questions and exact next command.
4. Commands run and outcomes, including skipped/failed checks and affected coverage.
5. Toolchain/ROM/fixture identities, resource usage when available, and location of local-only artifacts.

The incoming agent first reproduces a recorded check. It must not continue from an unverified natural-language summary. On another host, recreate ignored artifacts from their manifests or arrange authorized private transfer. If the required inputs are absent, report that dependency explicitly.

Keep model name/version/runtime in execution metadata for reproducibility, but don't branch game logic on it. Each runtime adapter should demonstrate shell/file/Git access, structured report handling and access to the reference tools before taking a gameplay task. A model-specific entry file, such as `CLAUDE.md` if needed, should be a short pointer to `AGENTS.md`. Dispatch prompts refer to paths and task IDs, not a transcript dump.

### Portable dispatch prompt

> Stay within task provider `<OpenAI/Anthropic>` unless the user explicitly moves this continuation. Use `<runtime/model>` with `<reasoning effort>` under D-0004; quota baseline `<timestamp/window/value or unknown>`, reserve/budget `<limits>`, checkpoint `<path>`. Work on task `<ID>` in `<task file>`, starting from `<commit>` in `<worktree>`. Read AGENTS.md and docs/STATE.md. Verify prerequisites, work within the assigned scope, and use the acceptance criteria in the task. Record reproducible evidence and update the handoff before yielding. Do not mark the task accepted; submit it for review. If blocked, preserve the current state and state the smallest next dependency.

### Reviewer checklist

Confirm the claimed behavior against the frozen reference and inspect whether the implementation covers the task's domain. Check that tests exercised the new code and did not use an emulator fallback for supposedly native logic. Run an independent boundary/withheld case where appropriate. Check for changed baselines, weakened comparisons, masked skips, undefined arithmetic and accidental content commits. Review readability as well: meaningful names, explicit units and state dependencies, navigable evidence, and a justified boundary for any literal register-level translation (D-0003). Approve or return a specific reproducible failure; a second model's agreement alone is not validation.

## Durable records with minimal bureaucracy

`docs/STATE.md` is a brief coordinator-owned summary. Task files hold execution details. Research records hold evidence. Decision records capture choices affecting several tasks. These should link to each other rather than copy large logs.

Record one finding per useful claim, one task per reviewable outcome, and one decision when a real tradeoff is resolved. Avoid creating a large speculative task tree for M4–M6 before M0–M3 establish the game's structure.
