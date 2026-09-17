# MIT license adoption

- Date: 14 September 2026; user explicitly selected MIT.
- Base: `571c5d7`; branch: `codex/mit-license`.
- Scope: standard root LICENSE and README license scope. No code, CI, game
  evidence, contribution restrictions, or gameplay handoff changes.
- Copyright notice: 2026 Mark Feaver, matching repository authorship metadata.
- Primary: current OpenAI Astra session. Fresh independent Sol/medium review
  in `.worktrees/mit-license-review` is required before integration.
- Quota: 99% weekly used at startup; bounded documentation maintenance and
  review only. No reset, purchase, provider change, or gameplay work.

## Decision and checks

Use standard MIT text without additional restrictions. The separate README
scope statement distinguishes project-owned material from original game rights,
screenshot artwork, and separately licensed third-party dependencies. Existing
historical decisions deferred license selection; this user instruction now
selects MIT for the rights the project authors hold.

Required checks: MIT text comparison with the published standard, local Markdown
links, ASCII hyphens in added prose, `git diff --check`, independent review,
verified main synchronization, GitHub MIT detection, and existing final-tip CI.
No local gameplay tests are needed for these license/documentation changes;
hosted synthetic checks do not establish original-game rights or accuracy.

## Closeout and resume

Acceptance is conditional on review approval and passing integration checks.
Store final commit, review, commands/results, remote identity and CI URL in
ignored `artifacts/mit-license/closeout.json` and `review.md`. Recover missing
closeout using `git log -- tasks/MIT-LICENSE.md` and
`gh run list --workflow synthetic.yml --commit <commit>`.
No milestone tag is due. The existing `tasks/NEXT_SESSION.md` continues to govern
separate M4-16 work.
