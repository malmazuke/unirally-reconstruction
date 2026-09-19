# Project state

Updated 17 September 2026: **M4-16 is reviewed and integrated**; acceptance is
conditional on the final-tip CI and remote verification recorded in the ignored
closeout `artifacts/m4-16-integration/closeout.json` in the main checkout
(moved there by REPO-LOCAL-STATE-CLEANUP). If that file is absent, recover the
integration commit with `git log --first-parent main -- tasks/M4-16.md` and its
run with `gh run list --workflow synthetic.yml --commit <commit>`. M4 as a
milestone is **not** accepted and no milestone tag is due. No M4-17.

M4-16 delivers playable native ZOOM ZOO in the desktop app: native
initialization from the authenticated pack through countdown, a three-lap race,
the original's 10:00 time limit, finish, result loading, the result screen and
Race Again, with keyboard and gamepad. Pack `classic.pal.crawler.two-tracks.v8`
(56 entries; v5-v7 are refused) is extracted from the user's ROM and reproduces
byte for byte;
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

DRAGSTER ordinary controls (found in live play on 18 September 2026 local
time: Left or B while riding aborted and Y was ignored): recovered on
`task/dragster-ordinary-controls`, reviewed (`f427098`) and integrated; its
acceptance is conditional on the final-tip CI and the user's live playtest.
DRAGSTER runs the shared race engine from native initialization with its own
track content ([R-0038](research/R-0038-dragster-ordinary-controls.md)); seven
frozen originals (win, loss, tie, reversal, pause, random input) match every
742-byte state with fresh-process restores, and randomized ordinary-input
fuzzing over complete races finds no abort. Shared-engine corrections found on
the way (countdown A/X release, charge latch, roll bounce drive, zero-step roll
completion, the hold/rotation restore bound) keep the ZOOM ZOO gates. The legacy
DRAGSTER path and its historical gates are unchanged; live play needs the
two-track pack. Live play by the user is still due.

DRAGSTER's 10:00 race limit: candidate on `task/dragster-clock-limit`, not yet
reviewed ([R-0039](research/R-0039-dragster-clock-limit.md)). The shared engine
already applied `$81:C73E-C75B` for DRAGSTER, and native equals one original
idle timeline on all 742 declared bytes for frames 1328-31881, including the
9:59.9 hold, both riders finishing at 31534 with the 60000 no-time total and
the start of result loading. What changed is the result screen: it refused to
draw a timed-out result and now writes `MIKE ... NO TIME`, as the original
does. Its result loading is two updates late behind the SPC700 reset handshake,
as ZOOM ZOO's time-out already was, so the case is a declared incomplete
inventory rather than an acceptance freeze.

Declared omissions: audio; the original's decorative objects and captions
(start arrow and ring, hints, on-screen stunt names, opponent finish time,
WINNER caption, off-screen arrows) except both tracks' countdown, GO and
winner windows, recovered in R-0040 and ZOOM-ZOO-WINDOW-EFFECTS; original
HUD and result pixel style;
the two-update later result load after a time-out (audio handshake timing).
Other tracks, riders, modes, menus and multiplayer remain outside the product.

Follow-ups from M4-16 findings. [DRAGSTER-PALETTE-CYCLE](../tasks/DRAGSTER-PALETTE-CYCLE.md)
is reviewed and integrated (acceptance conditional on its final-tip CI, closeout
`artifacts/dragster-palette-cycle-integration/closeout.json` in the main
checkout): DRAGSTER's race runs the same NMI palette
cycle ([R-0037](research/R-0037-dragster-race-palette-cycle.md)), visible as colour
0 in the GO and winner windows, which now take the cycled colour 0 when the pack
carries the tables (the two-track pack), matching the original where their
shapes agree; DRAGSTER v1 packs keep the accepted colours.
[DRAGSTER-WINDOW-EFFECTS](../tasks/DRAGSTER-WINDOW-EFFECTS.md) then recovered
when those windows appear and what shape each frame uses
([R-0040](research/R-0040-dragster-window-effects.md)): the original picks one
of a 25-table channel-6 family at `$15:8000` every frame, the countdown driver
`$83:E59C` from `$11C5` and the winner driver `$83:EA19` cycling members 7-24
from the winning rider's finish, and the vblank setup `$80:868E-$80:8699`
publishes the previous frame's choice. Native now draws the countdown digits,
their transitions, the alternating GO letters and the cycling winner banner
with no rider pose-pair gate, and its member equals the original's own pointer
on all 1,922 frames from 1533 to 3454 of a captured race. The family enters the
two-track pack additively as `presentation.effect.classic.window-tables.v1`;
the profile is now `classic.pal.crawler.two-tracks.v8` with 56 entries and
DRAGSTER v1 packs keep the accepted pose-keyed placement, so the accepted v1
contracts and the historical matrix are unchanged. That work is reviewed and
integrated (acceptance conditional on its final-tip CI, closeout
`artifacts/dragster-window-integration/closeout.json` in the main
checkout). The opponent-won banner beyond its
120-update counter is recovered by CLASSIC-PRESENTATION-UNIFICATION below. See
[NEXT_SESSION](../tasks/NEXT_SESSION.md).

[CLASSIC-PRESENTATION-UNIFICATION](../tasks/CLASSIC-PRESENTATION-UNIFICATION.md)
is reviewed and integrated (implementation `f8645d7`, rebased onto `main` as `e59744e`; review `a7f26c4`);
acceptance conditional on the final-tip CI and remote verification recorded in
the ignored closeout `artifacts/unification-integration/closeout.json` in the
main checkout. Presentation now scales the way
the simulation does: one renderer (`render_classic_race`) draws both tracks
from the 742-byte shared race state with track content selected by track from
the pack; the DRAGSTER-only renderer, its held rider art, its approximate fade
and its narrowing call path are gone from live play, and the accepted M3 v1
renderer remains only behind the frozen v1 contracts. The frozen DRAGSTER
contract race turned out to be the continuous-Right replay, whose inputs are
on record, so the shared engine reaches all seven frozen frames within their
thresholds; against the release-3213 originals the rectangle mismatch on the
132 window-effects frames falls from 757,274 to 680,807; the ten M4-16 ZOOM
ZOO scenes are pixel-identical to before; hidden runs of both tracks report 0
rider-pose fallback frames. The opponent-won winner banner is recovered over
its full length (presentation history plus a finish-time derivation, equal to
the original's `$80:868E` pointer on 2,226 of 2,226 frames of the opponent-won
capture). The shared engine reads its eight track-independent tables under
the neutral `physics.*` names. The launcher selects packs by the profile
recorded inside them, never by file name or track, refuses a typed pack of
another profile with the remedy, and reports a stale build before launch; the
four launcher defects from the 18 September playtest are closed.

[DRAGSTER-WINDOW-PAUSE](../tasks/DRAGSTER-WINDOW-PAUSE.md), from the user's
first play on that build, is reviewed and integrated (approved at `1d37c6a`,
report `067dbfe`, after one returned review; the re-review's items applied on
top); acceptance conditional on the final-tip CI in the ignored closeout
`artifacts/window-pause-integration/closeout.json` in the main checkout. The
countdown and winner windows are now
followed update by update the way the original's drivers keep `$11FD`
(`ClassicWindowPointer`): a pause disables the window and freezes the
countdown and the banner, and each rider's finish arms its own banner driver
with a 360-update life, the earliest-armed live driver owning the pointer.
Measured against seven originals captured with the user's ROM (two pauses,
an odd-length pause, two one-frame-apart finishes, the three accepted races),
native equals the original's pointer on every frame. R-0040's 360-frame
bound and "only the winner's driver runs" are corrected there.

[REPO-LOCAL-STATE-CLEANUP](../tasks/REPO-LOCAL-STATE-CLEANUP.md) is done on
`task/repo-local-state-cleanup` (18 September 2026 UTC), reviewed and
integrated; acceptance conditional on its final-tip CI in the ignored closeout
`artifacts/repo-local-state-cleanup-integration/closeout.json` in the main
checkout. Local state now has one home per kind: every worktree's `artifacts/`
moved intact to `local/evidence/<worktree>/` in the main checkout (115 GB,
including the DRAGSTER originals and the M4-16, M4-15 and idle captures the
gates read), the seven closeouts that lived in worktrees moved to
`artifacts/<task>-integration/` beside the earlier ones, and every recorded
gate script was repointed. All 93 registered worktrees and one unregistered
clone were removed, and 105 local branches were deleted after a patch-id audit
(nine with commits not represented on `main`, all review reports, were on
`origin` first, eight of them newly pushed; the two review reports the records cite but `main` lacked,
CLASSIC-PRESENTATION-UNIFICATION's and DRAGSTER-WINDOW-PAUSE's, are now on
`main`). Deleted: verified duplicate captures (7.6 GB), the M4-15 reviewer's
uncited exploration captures (11 GB), build output, caches and run-report
scratch. The repository went from 146 GB to about 117 GB; the DRAGSTER and
M4-16 differential gates pass from the new location. AGENTS.md now carries the
retention rule so a closing task leaves nothing behind in its worktree.

[ZOOM-ZOO-WINDOW-EFFECTS](../tasks/ZOOM-ZOO-WINDOW-EFFECTS.md) (18-19
September 2026 UTC, `task/zoom-zoo-window-effects`) binds the channel-6
window family for ZOOM ZOO, so its countdown digits, GO and winner banner now
draw from the original's own per-frame selection like DRAGSTER's. Read from
the existing M4-16 originals' WRAM, ZOOM ZOO's `$11FD` runs the same members
on the same drivers; the one difference is the countdown transition member,
`5 + $1229`, which race setup latches from the player's start reflection
(member 6 on DRAGSTER, 5 on ZOOM ZOO; derived from the track header). Two
shared corrections came with it: Start held after a pause resume keeps the
window off and the look still (the predicate now reads the engine's
suspended-update clock), and every window member composes after both riders
(the countdown members were drawn under them), which also fixes 75 DRAGSTER
frames where a rider sits under the digits or GO. Native's published
member equals the original's pointer on every frame of seven ZOOM ZOO
originals and the four DRAGSTER pause originals; every pixel the change
touches on the frames with original pictures matches the original. Status
and review: see the [registry](../tasks/README.md) and the task record.

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
