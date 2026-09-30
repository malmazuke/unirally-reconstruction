# LEAGUE - native tournament play and scoring

## Assignment

- Status: claimed, 30 September 2026 09:29 UTC.
- Milestone: M4 original game coverage; COVERAGE-ROADMAP after OPTIONS.
- Coordinator/worker: this Codex desktop primary session, OpenAI provider.
- Actual primary model/effort: not exposed by the runtime; repository Sol/medium defaults are not proof of the active setting. No provider move.
- Review tier: 1, because the tournament changes game state, result ordering and SRAM arithmetic, and may add content entries.
- Quota: Codex weekly window at startup 0% used, 100% remaining, reset Unix 1791365217. Discretionary boundary 20% used from this baseline; final 20% reserved for review/recovery under D-0004. No reset redemption, purchase or paid fallback authorized.
- Planning: one bounded Astra/medium consultation on tournament assumptions and discriminating experiments, maximum ten minutes. Direct primary recovery/implementation thereafter.
- Required reviewer (pending): automatically launch fresh gpt-5.6-sol/medium, no inherited conversation, detached isolated exact-candidate checkout, comment review on the task PR.
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

- Head before implementation commit: `223a550`; task branch merged `origin/main` at `ff501d1`. Native WIP, research and authored tests are in this checkout; no PR/reviewer yet and no accepted candidate.
- Current primary: native full CRAWLER two-member tour matches all five finish totals and the saved league projection at all retained snapshots. Original full-tour coverage passes 40,264 frames, 27,838 sites, 30,486 pairs and 642,328,758 instructions without overflow. Source changed during this original capture; it is original evidence, not native exact-source acceptance.
- Exact native current checks are investigative dirty-source reports under main `artifacts/league-integration/`. `full-tour-compare-5.json` has DRAGSTER 3293/3290, ZOOM ZOO 9690/9686, neutral STUNT, DUELLER 8430/8429 and GOING UP 10989/10994, with matching returns 9122/17268/21715/28267/36004. The comparator currently reports differences without a failing exit; it is not an acceptance gate.
- Known presentation limits: some NOW PLAYING/result icon animation, STUNT finish, lower split HUD and one-frame palette/slide offsets differ. Podium pictures match on both bounded cursor intervention and full organic tour. No full-picture claim for unmatched frames.
- Failed attempts and reports are retained: initial route stalls/timeouts, missing synthetic decoration fixture, stale appended-inventory slices, and unused helper parameter after refactoring. Fixes retain earlier expected inputs; no failure is reported as a pass.
- Next: finish build/unit run, strict frozen state/picture comparator, independent original variants, all eleven frozen races and v1 presentation, hidden and real-window app smoke, readability/synthetic checks. Commit/push concrete candidate, open PR and automatically launch the required fresh isolated Sol/medium reviewer. Handle findings and exact-head re-review/CI before merge and cleanup.
- Main `local/evidence/league/` already owns every capture; main `artifacts/league-integration/` owns all logs. No user-created artifacts or pre-existing playtest checkout may be removed.

## Checkpoint - 30 September 2026 09:39 UTC

- Startup app-debug build and front_end_tests pass. OPTIONS define-league-name-t reproduces seven exact pictures and its accepted league SRAM projection; the same hidden OAM residual remains.
- Bounded Astra/medium consultation completed: capture played counters alongside points, persistent participant/track cursors, odd-member selection and cycling track boundary. No implementation was delegated.
- Original organic-two-entry reaches slot T, PICK TOUR, LEAGUE TABLE, COLIN against TONY on DRAGSTER. Coverage gate and task-local listing decode pass. organic-two-race gives TONY 0:32.90, COLIN 0:32.93, then points 10/9 and one played each; next track ZOOM ZOO. No native LEAGUE comparator or acceptance yet.
- Failed initial schedule returned to OPTIONS because main-menu return resets selection to zero; retained as organic-two-recon. Two record-generation commands used the wrong working directory and were corrected; no gate expectations changed.
- Weekly usage 1% used, discretionary boundary 20%; no reset or spending. Next: native standings/setup, played scoring, opposite outcome, odd membership and resume evidence.

## Checkpoint - 30 September 2026 10:15 UTC

- Source WIP now selects LEAGUE, renders its initial standings and enters native races. Native/PAL organic-two-entry has exact text maps at all retained pictures and exact RGB at 5300/5600/5700/6200; 5850 differs by 79 pixels and 6600 by 951. No accepted candidate yet.
- Original organic-three selects COLIN, TONY and CRAIG. Its first result increments COLIN/TONY played counters, then returns to standings without awards; the next pairing is CRAIG against BRONSEN on the same DRAGSTER. These direct probes still need authenticated coverage gates.
- Failed original attempts are retained: organic-two-events continuous Right never finishes ZOOM ZOO; the translated M4-16 steering and six timing shifts likewise have no finish by frame 18000. organic-three-second-slot and organic-three-slot-two reach an incomplete keyboard, so they do not establish second-slot play. An initial third-member input toggled TONY off; corrected schedule moves two rows before toggling CRAIG.
- Native scoring/award text and persistence fields are being implemented from the task listings. Pack v29 adds standings and award text (468 entries); extraction initially refused overwriting a prior private development pack, so a fresh output path is used. Compilation caught narrow rider assignments and the wrong dispatch pad variable; those are fixed, pending build. No failed attempt is reported as a pass.
- Weekly usage remains 2% used, 20% discretionary boundary. Reassessment: continue this task; research dependencies and checkpoints do not request user intervention. Next: first-result differential, corrected ZOOM ZOO traversal, authenticated three-member and opposite-outcome captures.

## Checkpoint - 30 September 2026 10:37 UTC

- Original organic-three and organic-colin-wins coverage gates pass. The latter delays port 2 until 7600 and reverses the first winner: COLIN 34.50 and 11 points, TONY 38.74 and 8. Earlier 100-frame delay remained inside countdown and did not reverse the winner.
- Original-only marker-guided recon freezes a new controller schedule, organic-two-marker-route. Native consumes that fixed schedule, not original markers. Its second race totals match: COLIN 9690 hundredths, TONY 9686; native first/second initialization labels 7028/11975. The prior translated schedule used the wrong reversal times and is retained as failed evidence.
- First-race native records match every retained projection; the 10700 award and 11100 standings pictures are exact. The runner formerly overwrote a native race image with the stale menu picture at the end of the same frame; fixed task-scoped output routing. Its 7100 DRAGSTER picture now has the established 57-pixel residual, not the earlier 53,874-pixel overwrite. Remaining NOW PLAYING/result icon animation differences are unaccepted presentation residuals.
- Original three-event path reaches two-human STUNT, a zero-score draw, then COLIN 11/TONY 8 event points and cumulative TONY 29/COLIN 28, with three played each. This exposed the solo-only stunt implementation. Native two-human updates, opt-in bonus counters and an additive league save wrapper now build; result rendering, differential and authenticated third-event gate are still pending.
- Weekly usage 3% at 10:29 UTC; 20% discretionary boundary. No reset/spending/provider change. Next: two-human stunt results, restore checks, complete odd-member event without accidental race-time Start, exits/resume and track-cycle boundary.

## Checkpoint - 30 September 2026 11:06 UTC

- Original organic-three-events coverage and dynamically seeded listing pass. Native reproduces all retained event/played/points/pairing projections: first totals 3293/3290, second 9690/9686, then neutral two-human STUNT, cumulative TONY 29/COLIN 28 with three played each. STUNT loading corrected from 128 to measured 127; native return now matches original frame 21715. The settled result/awards/standings retain bounded icon animation differences, and lower race background/finish brightness still require recovery.
- Odd organic-three-clean removes accidental pause Starts. Its isolated CRAIG total remains three hundredths different (5274 native, 5277 original); no acceptance. Original organic-odd-resume returns through slots/main and resumes cursor 2 without tour selection. Two second-slot schedules still reach player selection, not a saved league; retained failed evidence, not passes.
- Controlled saved-track interventions after a played DRAGSTER result at 4 and 43 show next values 5 and 0, mode bit 0x2000, podium, then Keep Scores/Reset Scores/Exit and tour choice. These establish bounded intervention behavior, not a naturally played 44-track cycle. Coupled continuation recovery remains inside LEAGUE.
- Weekly quota remains 3%, discretionary boundary 20%. No reset/spending/provider move. Next: podium continuation, exit/quit/restart, second-slot timing and saved-state restore; then freeze exact acceptance and review.

## Checkpoint - 30 September 2026 11:24 UTC

- Native two-human STUNT pictures at 19300 and 21000 are now exact. The second background scroll uses the second rider's starting coordinates; the earlier local-scanline experiment was wrong and reverted. Measured split finish bytes at $7E:2065/2069 are 7, recovered at $83:E8F0/EA82; native applies this dimming, with 6291 remaining finish-picture pixels different at 21700. Settled result/awards/standings retain small animation residuals. No whole-picture acceptance claim for that finish frame.
- Native odd-member DRAGSTER now gives CRAIG 5277 and CPU 3358, matching organic-three-clean after measured loading correction 121 to 120. Exact restored continuation at native frame 20000 in the third two-human stunt and native frame 13000 in the odd CPU race passes round-trip and subsequent serialized comparisons through return. Wrapper layout is opt-in; legacy formats unchanged.
- Second-slot cold entry succeeds in organic-second-slot: Down is before the slot confirmation at 1300, and LEAGUE preserves the last edited slot on return. The previous extra Down selected empty THREE; failed evidence retained. Native reproduces its first race totals and records. Expanded all-slot bonus projection exposed wrong unused-slot cold defaults (native EA62, original zero); corrected.
- Native odd-resume tables at 11000/11400/11800/12600/13000/13800 are exact pictures; pending CRAIG pairing persists. Original controlled cycle-five coverage passes: 23459 sites, 25293 pairs, 215256190 instructions, 0 ring overflow. Dynamic listing agrees across 85266 sites, 0 disagreements. Podium/Keep Scores/Reset Scores/Exit remains coupled WIP.
- No PR/reviewer yet; native candidate remains unaccepted. Weekly usage last 3%, discretionary boundary 20%; no reset/spending. Next: native podium/continuation and quit/restart, authenticate and freeze final domains, focused test then fresh review.

## Checkpoint - 30 September 2026 12:06 UTC

- Weekly usage sampled at 11:48 UTC: 5% used, against the 20% discretionary boundary. No reset or spending. Native code remains uncommitted and unaccepted; no PR or reviewer yet.
- The two-human neutral STUNT and odd-member DRAGSTER totals now agree with their originals. The expanded investigative SRAM comparison includes all six slots, pairing rows, event totals, cursor/track/bonus words and tutorial bits.
- An authenticated bounded track-cursor intervention reaches the podium and KEEP SCORES / RESET SCORES / EXIT menu. It proves the track-five boundary and not a naturally played full tour. Native podium picture now has a 602-pixel animation residual at 12000; continuation still has a background phase mismatch. The first podium build command in this checkpoint used the wrong script path and failed; corrected tools/project.py builds pass.
- The paired pause original displays a message and resumes, ignoring Down, while both humans remain unfinished after countdown ($83:F6FD-F791). Countdown still allows restart. Native opt-in league path now handles this and port 2; differential checks pending.
- An original-only marker-guided complete CRAWLER tour capture is running. The native implementation consumes only the resulting frozen controller schedule. A dense podium timing probe is also running; neither is acceptance evidence yet.
- Added ROM-free arithmetic boundary checks for eight members, odd CPU awards, pairing-order restoration, bonus ties, played integer wrap and track 43 wrap. Next: full-tour loading/finish evidence, podium/menu timing, strict comparisons, regression and isolated independent review.

## Checkpoint - 30 September 2026 13:04 UTC

- Weekly usage 7% used, discretionary boundary 20%, no reset/spending. Tier 1 remains required.
- The original valid full-tour schedule is frozen and authenticated. Its DUELLER NMI gap is 196 frames followed by one idle NMI: fade 6 at 23610 fixes initialization 23604 and total loading 197. GOING UP loads 175 frames, fade 1 at 30060.
- `$83:E7C3-E7EA` waits for both split riders before counting the finish display. Native league now does that; earlier first-rider-only behavior returned GOING UP three frames early. Source repair is under differential checks; established non-league frozen domains are preserved.
- Regenerated symbol index has no uncited ROM addresses. v29 inventory checks retain the 466-entry prefix and compare all nine added compiled identities. The complete suite rerun exposed an unused helper parameter, which is being fixed before repeating acceptance.
- Native save tests now check paired STUNT counters, malformed width/flag/split input and the second camera velocity bound. Full app remains pending.

## Checkpoint - 30 September 2026 13:24 UTC

- The full original and five native race totals agree, including reversed-winner return 9413 once both riders finish. A new strict frozen contract covers six scenarios, 100 SRAM snapshots and 59 pictures, all with immutable private reference hashes. Only three established second-view checkpoint pictures permit fourteen pixels; every other listed picture requires exact RGB.
- The broader saved-name check found the OPTIONS editor's hardcoded marker-row suffix. Three-member CRAIG leaves row 0x13 rather than the two-member COLIN row 0x0D. Native now carries the last marker position and preserves the original name suffix/checksum. Failed strict runs remain retained; no expected byte was removed.
- Application smoke, eleven frozen race regressions, full clean-source synthetic and fresh isolated independent review are pending. Synthetic fourth failed two tooling tests (a nested timeout expectation under concurrent builds and a symbol index changed during the run); native tests passed. Native source must be frozen during the next full suite.

- All six frozen scenarios, 100 retained SRAM snapshots and 59 listed pictures now pass the dirty-source rehearsal, including the saved-name suffix repair. That run deliberately fails its clean-source acceptance requirement. The next command is a clean committed-source run, followed by full regression and automatic PR review.
