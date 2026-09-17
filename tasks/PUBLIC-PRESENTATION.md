# Public repository presentation

- Date: 14 September 2026.
- Scope: user-authorized README polish, contribution policy, public-origin workflow
  wording, subtitle, repository rename, and collaborator-only pull requests.
- Base: `46d0858`; branch: `codex/public-presentation`.
- Primary: OpenAI Astra, current session; independent reviewer: Sol/medium in
  `.worktrees/public-presentation-review`. Documentation and workflow maintenance
  only, no gameplay or M4-16 acceptance work.
- Quota: startup reported 97% weekly used. Keep this maintenance/review bounded;
  no reset, purchase, provider switch, or gameplay expansion is authorized.
- Owned paths: README.md, CONTRIBUTING.md, AGENTS.md, docs/AGENT_WORKFLOW.md,
  docs/PROJECT_PLAN.md, tasks/TEMPLATE.md and this task record.

## Decisions and observed settings

The repository is now `malmazuke/unirally-reconstruction`, public, with description
"Reconstructing Unirally in readable C++ - toward a faithful, portable native version."
GitHub PATCH returned `pull_request_creation_policy: collaborators_only`.
Issues remain enabled for feedback and Discussions remain disabled. The local
folder and worktrees retain their existing paths; origin now uses the new SSH URL.
The user explicitly requested regular hyphens and an inline Wikipedia link.

Keep existing agent entry points and historical evidence. Correct the stale README
claim that M4-16 implementation has not started, without claiming it is accepted.
The contribution policy applies to outside submissions; authorized agent work,
independent review, and normal source synchronization remain authorized.

## Validation and handoff

Required: independent documentation review, local Markdown target checks,
`git diff --check`, unchanged source/CI and gameplay handoff, remote metadata/ref
verification, and existing synthetic CI on the integration tip. Gameplay tests
are not rerun locally for this prose/settings-only change; hosted synthetic CI
is not private ROM differential evidence.

Review candidate and approval are recorded in the task commit history and review
report. After approval, fast-forward main and push once. Final exact commit,
review result and CI outcome belong in ignored
`artifacts/public-presentation/closeout.json`. If absent, recover the commit with
`git log -- tasks/PUBLIC-PRESENTATION.md` and inspect its run with
`gh run list --workflow synthetic.yml --commit <commit>`.
Acceptance remains conditional on independent approval, verified synchronization
and passing final-tip CI. No milestone tag is due. Resume gameplay only under
its existing task and provider instructions in `tasks/NEXT_SESSION.md`.

## Public screenshot

The user supplied `/Users/markfeaver/Desktop/Unirally.png` and requested considering
it for the README. Included unchanged at `docs/images/unirally.png`, with descriptive
alt text and a maintainer-supplied gameplay caption. Capture provenance is not
verified, so it makes no claim to show an accepted native build or particular
track. This explicitly requested presentation image is separate from private
reference captures, ROMs, extracted asset packs and acceptance evidence.

## Independent review corrections

Sol/medium reviewed `328b560` and requested two wording corrections: the remaining
private-repository approval boundary in AGENTS.md, and stale ready/unclaimed
M4-16 status in the project plan. Both are corrected. The historical M4-15 sync
is explicitly described as then-private, preserving the fact rather than
retroactively claiming it was public. Re-review is required before integration.
