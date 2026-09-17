# Project state

Updated 17 September 2026: **M4-16 is reviewed and integrated**; acceptance is
conditional on the final-tip CI and remote verification recorded in the ignored
closeout `artifacts/m4-16-integration/closeout.json` in
`.worktrees/m4-16-playable-zoom-zoo`. If that file is absent, recover the
integration commit with `git log --first-parent main -- tasks/M4-16.md` and its
run with `gh run list --workflow synthetic.yml --commit <commit>`. M4 as a
milestone is **not** accepted and no milestone tag is due. No M4-17.

M4-16 delivers playable native ZOOM ZOO in the desktop app: native
initialization from the authenticated pack through countdown, a three-lap race,
the original's 10:00 time limit, finish, result loading, the result screen and
Race Again, with keyboard and gamepad. Pack `classic.pal.crawler.two-tracks.v7`
(55 entries) is extracted from the user's ROM and reproduces byte for byte;
serialized state is `URZZ000B` (742 bytes). Native play does not execute the
original CPU. The work moved from GPT Astra to Claude Opus 5 under
[D-0004](decisions/D-0004-model-and-usage-budget.md) when those credits ran out,
with fresh Opus 5 independent reviewers.

Evidence, on the reviewed implementation `75626f8` and the integration commit:
exact original agreement over the primary start-to-result gate (6,225 states,
757 fresh-process restores, full restart), six complete cases covering both
outcomes, an idle late-start case (801 restores), seven reward and eight trick
probes, the M4-15 race matrix and the M4-12-M4-14 and DRAGSTER historical
matrix; four-preset tests; bootstrap, pack-only, wrong-ROM and bad-pack gates;
denied-execution autonomy; live keyboard play (earlier candidates) and a live
gamepad playtest with rider art (0 pose fallbacks) through a completed race,
result and restart; and frozen visual scenes with rider art, HUD, fade and
the start-line palette cycle matching the original. Review record:
[M4-16-review](../tasks/M4-16-review.md); task record: [M4-16](../tasks/M4-16.md);
research: [R-0035](research/R-0035-zoom-zoo-playable-recovery.md),
[R-0036](research/R-0036-zoom-zoo-rider-objects.md).

DRAGSTER control limit (found in live play on 18 September 2026 local time):
native DRAGSTER only recovers riding right, releasing and its recorded inputs.
Left, or SNES B (jump) held while riding, aborts with a fail-closed domain
message, and Y (the original's brake) is ignored. Recovery is in progress in
`task/dragster-ordinary-controls`.

Declared omissions: audio; the original's decorative objects and captions
(start arrow and ring, hints, on-screen stunt names, opponent finish time,
animated finish banner, off-screen arrows); original HUD and result pixel style;
the two-update later result load after a time-out (audio handshake timing).
Other tracks, riders, modes, menus and multiplayer remain outside the product.

Follow-ups from M4-16 findings. [DRAGSTER-PALETTE-CYCLE](../tasks/DRAGSTER-PALETTE-CYCLE.md)
is reviewed and integrated (acceptance conditional on its final-tip CI, closeout
`artifacts/dragster-palette-cycle-integration/closeout.json` in
`.worktrees/dragster-palette-cycle`): DRAGSTER's race runs the same NMI palette
cycle ([R-0037](research/R-0037-dragster-race-palette-cycle.md)), visible as colour
0 in the GO and winner windows, which now take the cycled colour 0 when the pack
carries the tables (two-track v7), matching the original where their shapes
agree; DRAGSTER v1 packs keep the accepted colours. Not started:
DRAGSTER window timing and shape (still gated on rider poses), and DRAGSTER's
10:00 limit. See [NEXT_SESSION](../tasks/NEXT_SESSION.md).

## Accepted product and evidence

- Milestones M0–M3 are accepted; `m3` remains the latest milestone tag. The
  playable product is the identified PAL one-player CRAWLER/DRAGSTER slice
  through stable winner/loser results. Native play does not execute the original
  CPU. [M3 acceptance](research/R-0017-m3-acceptance.md) and
  [M4-01](../tasks/M4-01.md) define presentation limits.
- Exact user-ROM extraction produces the local 25-entry Classic pack. Audio,
  full rider art, other playable tracks/modes/riders, menus/progression and
  multiplayer remain outside the product. ROM/content/captures remain ignored.
- PAL ROM SHA-256 is
  `a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`;
  the private locator is `local/rom-location.txt`.
- M4-00 through M4-15 are individually accepted; **M4 is not accepted**.
  M4-12 added autonomous native ZOOM ZOO continuation for both riders from
  end-1649 through 1849: 200 updates, exact 395-byte state, Right/neutral input,
  one seed and authenticated static content. [R-0030](research/R-0030-zoom-zoo-native-trial.md).
- M4-13 adds B jump, actual player support loss, landing and recovery in the
  same horizon. Primary B1681–1695 lands at 1745 and matches 104 subsequent
  updates. Two fresh independent cases and two corrected regressions match all
  bytes and restore before/after each full player landing. Captured dynamic
  native inputs remaining: zero. This is still not full-track support and has
  no production ZOOM ZOO frontend/presentation dispatch.
- Fresh Sol/medium [review](../tasks/M4-13-review.md) approved corrected code
  `13f80ff` after two findings in one correction round. App-debug and app-sanitize
  each passed 400 checks with no skips; all M4-12, DRAGSTER, content/replay and
  presentation gates passed. The merged build repeated primary and independent
  cases/restores. Hosted integration CI `34757217687` passed both platforms.
  Hosted Linux is synthetic coverage, not private Linux differential execution.

## M4-14 capability and integration

[R-0033](research/R-0033-sustained-traversal.md) records native continuous Right
from authentic end-1649 through 3299: 1,650 updates / 33 PAL seconds, both riders,
423-byte state (`URZZ0002`), no captured dynamic inputs or original CPU fallback.
Recovery at 2185 leaves 1,114 updates. This is local resumed progress followed
by continued traversal; the rider revisits the same section. It does not prove
monotonic advance, obstacle clearance or full-race completion.

Recovered behavior includes inverted/horizontal probes, steep contact and
landing, tile-selected mode/angle lifecycle, pose/control consumers, and leading
landing reward bookkeeping. The seven additive words per rider preserve old
395-byte `URZZ0001` expectations. Full reference hashes, static inputs and guard
identities are frozen; corrected continuous instruction audit authenticates
30,355,912 instructions and closes the reached future-state inventory.

Fresh Sol/medium [review](../tasks/M4-14-review.md) approved corrected candidate
`e730aaa`. Review found an extra conversion on a full 28-unit landing, missing
binary restore validation and an unclosed audit record. All are resolved; the
failed case remains a regression and a fresh untuned replacement passes 96
restores. A separate material late variation also passes. Primary and cases
retain the complete horizon and frozen recovery requirement.

App-debug and app-sanitize each passed 403 checks, no skips; all accepted
M4-12/M4-13 differential cases/restores and DRAGSTER/content/replay/presentation
gates pass. Denied-ROM/repository execution and negative controls establish the
bounded native runtime's autonomy. M4 remains incomplete; ZOOM ZOO frontend,
presentation/audio, full-track support and private Linux differential execution
are not claimed.

Primary Astra/medium started 13:09 UTC, shared weekly usage 26%; independent
approval around 13:54, usage 35%. These are account-wide observations, not a
controlled model comparison. No reset, purchase or provider switch occurred.
The [task](../tasks/M4-14.md) records the 45-minute reassessment and integration
commands. Final accepted integration is `38c72e8`, 55.61 minutes, shared usage 26% to 37%,
with green exact-tip CI and synchronized private main, recorded in the ignored closeout.
If absent, inspect `38c72e8` in git and `gh run view 34761303975`. Do not create
a second documentation-only CI cycle to transcribe that result.

M4-13 remains accepted at `b288396`; its final closeout took 42.81 minutes and
eight shared usage points (15% to 23%). Historical evidence remains in R-0032
and its task/review. For any future task, retain D-0004/D-0006 budget and automatic
review practices; M4-16 has its own exception and preparation; no M4-17 dispatch is authorized.

## Where to look

- [Task registry](../tasks/README.md), [M4-14](../tasks/M4-14.md) and
  [R-0033](research/R-0033-sustained-traversal.md): actual commits, commands,
  resources, review and next reference experiment.
- [Build and validation](BUILD_AND_VALIDATION.md): implemented CLI and private
  fixture boundaries; [native source guide](../src/core/README.md): source map.
- [Project plan](PROJECT_PLAN.md): longer-term M4–M6 scope; M5/M6 have not started.
- [Workflow](AGENT_WORKFLOW.md): ownership, review and required private-origin sync.

## M4-15 race completion

[R-0034](research/R-0034-zoom-zoo-race-completion.md) and the
[review](../tasks/M4-15-review.md) describe complete native race simulation
from authentic end-1649 through 6724, preserving the original opponent.
Both riders finish (6484/6488); stored times are 9802/9810 centiseconds.
The primary, three corrected regressions and two fresh withheld variations
match every 565-byte state and fresh restores. Debug/sanitizer each pass 405
synthetic checks, and all 28 accepted M4-12–14 differential/restore runs plus
DRAGSTER/content/replay/presentation gates pass. Denied repository/ROM access
and negative controls pass. Exact merged validation and remote/CI passed for `8bc2e71`; closeout evidence is
in ignored `artifacts/m4-15-integration/closeout.json`. Final time was 95.14 minutes,
shared weekly usage 38% to 55%; no reset or purchase. Recover missing evidence
with git and `gh run view 34785933369`.

Recovered coupled behavior includes direction control, checkpoint/lap/time
publication, camera visibility feedback, finish collision poses and continuation.
Review corrected throttle and jump early returns and malformed restore checks.
The finite first-finish announcement-queue invariant is authenticated before
native case evaluation; arbitrary player-queue restores are not covered.
No native race-start initialization, ZOOM ZOO frontend, rendering/audio,
subsequent result-screen loading or universal input coverage is claimed.
Hosted Linux synthetic CI does not establish private Linux differential coverage.
M4 remains incomplete and no milestone tag is due.
