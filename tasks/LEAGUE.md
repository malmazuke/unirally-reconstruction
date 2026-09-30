# LEAGUE - native tournament play and scoring

## Assignment

- Status: claimed, 30 September 2026 09:29 UTC.
- Milestone: M4 original game coverage; COVERAGE-ROADMAP after OPTIONS.
- Coordinator/worker: this Codex desktop primary session, OpenAI provider.
- Actual primary model/effort: not exposed by the runtime; repository Sol/medium defaults are not proof of the active setting. No provider move.
- Review tier: 1, because the tournament changes game state, result ordering and SRAM arithmetic, and may add content entries.
- Quota: Codex weekly window at startup 0% used, 100% remaining, reset Unix 1791365217. Discretionary boundary 20% used from this baseline; final 20% reserved for review/recovery under D-0004. No reset redemption, purchase or paid fallback authorized.
- Planning: one bounded Astra/medium consultation on tournament assumptions and discriminating experiments, maximum ten minutes. Direct primary recovery/implementation thereafter.
- Reviewer: automatically launched fresh gpt-5.6-sol/medium, no inherited conversation, detached isolated exact-candidate checkout, comment review on the task PR.
- Dependencies: OPTIONS PR #46 merged at `a2cdb9cb768e8934acb6fd9d682b90179a368af4`; main closeout verified, GitHub merge verified and local main equals freshly fetched origin/main. TWO-PLAYER-VS and all earlier roadmap outcomes are on that base.
- Branch/worktree: `codex/league`, `.worktrees/league`, base `a2cdb9cb768e8934acb6fd9d682b90179a368af4`.
- Ownership: task/research/handoff/registry records; required front-end and race-result native code, runner/app integration, scoped authored tests and content rules, regenerated symbol/static maps. Main owns canonical registry/integration; all implementation occurs in the isolated checkout.
- Local evidence: main `local/evidence/league/`; gate logs/closeout: main `artifacts/league-integration/`. No gate inputs in another worktree.
- Checkpoints: this record every ten minutes and at experiments; reassess after 45 minutes, without turning an internal checkpoint into user intervention. At most one child active.

## Outcome and boundaries

Deliver LEAGUE from native main-menu selection through an existing or newly defined league, tournament setup, played race results, scored standings, continued pairing and exit/re-entry. Recover the tournament's actual termination/resume behavior before freezing the full primary acceptance schedule. The six entries ONE through SIX are league slots; OPTIONS evidence permits two to eight riders per league, so do not assume six competitors. Use the shared native two-human race engine and existing editors. Coupled recovery stays inside this task under D-0006. No execution of original code in the product, invented pairing/scoring, audio, networking, mod framework or whole-game acceptance claim.

## Inputs and prerequisites

- PAL ROM SHA-256 `a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`; private locator `local/rom-location.txt`.
- Pinned bsnes commit `7d5aa1e656b9171524d01b1b22917197d8121cb4`, patch `a719f5ffe2222dad4c1ab04336633319ad85004f74e32fc14893a058be333885`.
- Existing v28 pack: 466 entries, SHA-256 `9fffc9cff7d01bca9e9f1a9feeced7534b4e0b09428a263b989f537489df3def`.
- Read `artifacts/static-map/bank-80.lst` at `$80:BDD4-BF48`, `$80:9B79-9EF7` before capture design. The league handler is still unknown bytes in that listing; dynamic capture must resolve widths and calls. Listing readings are hypotheses, not gameplay evidence.
- Startup doctor passed all required checks. Optional system Ninja missing, isolated Ninja available. Prior macOS sanitizer runtime stalls before main; retest with timeout, never count it as passed. Hosted Linux sanitizer remains required.

## Acceptance

| Criterion | Experiment/check | Required evidence |
| --- | --- | --- |
| Native tournament from power-on | Freeze a shortest complete two-member PAL tournament after reconnaissance, compare projected menus, pairing, race rows, score/persistence words and retained pictures | Manifest, hashes, comparator report; no runtime captured-state inputs |
| Independent domains | Different slot/membership (including more than two), reversed winner, quit/cancel and continuation/resume; reviewer chooses withheld boundaries | Separate frozen originals and independent review |
| Playable application | Hidden app path plus real-window smoke with normal input routes | Frontend reports and visible native league screens |
| Regression/readability | Eleven frozen races, v1 presentation, exact pack inventory, affected native/tooling tests, full app-debug, bounded sanitizer attempt; symbol index/format/function size | Exact immutable source/binary/input identities and honest missing/timeout results |
| Review/integration | Fresh tier-1 reviewer, findings resolved, exact-tip hosted macOS/Ubuntu checks, audit comments, merge PR with merge commit | PR review/check URLs and ignored closeout; local main equals origin/main |

## Evidence and attempts

1. Verified OPTIONS integration on GitHub and in local closeout; fresh origin fetch agrees with main. Existing main and pre-existing playtest checkout are untouched.
2. Static listing shows the league top-level handler at `$80:BDD4` is not yet decoded. Its cold menu capture in `local/evidence/coverage-roadmap/mode-3/` reaches empty slots only, so it cannot establish tournament scoring.

## Handoff

- Head: base above; task claimed, no native league implementation yet.
- Next experiment: reproduce OPTIONS foundation in a local build, then cold LEAGUE selection plus a saved two-member slot. Capture the first tournament setup/pairing and score writes, inspect dynamically decoded calls, freeze full schedule before implementation tuning.
- Native league gameplay remains unaccepted. No PR or reviewer yet. Existing captures/expected results will not be weakened.

## Checkpoint - 30 September 2026 09:39 UTC

- Startup app-debug build and front_end_tests pass. OPTIONS define-league-name-t reproduces seven exact pictures and its accepted league SRAM projection; the same hidden OAM residual remains.
- Bounded Astra/medium consultation completed: capture played counters alongside points, persistent participant/track cursors, odd-member selection and cycling track boundary. No implementation was delegated.
- Original organic-two-entry reaches slot T, PICK TOUR, LEAGUE TABLE, COLIN against TONY on DRAGSTER. Coverage gate and task-local listing decode pass. organic-two-race gives TONY 0:32.90, COLIN 0:32.93, then points 10/9 and one played each; next track ZOOM ZOO. No native LEAGUE comparator or acceptance yet.
- Failed initial schedule returned to OPTIONS because main-menu return resets selection to zero; retained as organic-two-recon. Two record-generation commands used the wrong working directory and were corrected; no gate expectations changed.
- Weekly usage 1% used, discretionary boundary 20%; no reset or spending. Next: native standings/setup, played scoring, opposite outcome, odd membership and resume evidence.
