# Next session

**From 23 September 2026, integrate through a pull request** (PR-WORKFLOW):
push the task branch, open a pull request from the template, merge with
`gh pr merge --merge` once its checks are green and the review is done. `main`
refuses direct pushes. The checks run on pull requests only.

**Status on 25 September 2026 UTC (NATIVE-READABILITY accepted): start
[RESULT-TITLE-GLYPHS](RESULT-TITLE-GLYPHS.md).** NATIVE-READABILITY's three parts are
integrated: the rules and the address index, the simulation split by system, and the
presentation split by concern. No function in `src/core` exceeds 80 lines. Follow
`src/core/README.md` "How this code is written" in every task that touches `src/core`; for a
behaviour-neutral refactor, the task record's evidence tools (`equivalence.py`,
`corruption.py`, `verify_split.py`, `citations_kept.py`) are the oracle.

**Status on 25 September 2026 UTC (NATIVE-READABILITY part 2): continue
[NATIVE-READABILITY](NATIVE-READABILITY.md) with part 3 (tier 2: presentation and the runners).**
Part 2 (the simulation, split by system and rewritten) is reviewed and integrated by pull
request. The record's handoff lists part 3's eight functions and the checks: `equivalence.py`
with pictures, the v1 contracts, then `gates.sh`. RESULT-TITLE-GLYPHS stays next after this task.

**Status on 25 September 2026 UTC (NATIVE-READABILITY part 1): continue
[NATIVE-READABILITY](NATIVE-READABILITY.md) with part 2 (tier 1, the simulation).** Part 1
(rules, `src/core/.clang-format`, the native-symbol index) is reviewed and integrated by pull
request #23. The record's handoff has the first step (the reward queue) and the checks
(`local/evidence/native-readability/equivalence.py` against the frozen base binaries, then
`gates.sh`). RESULT-TITLE-GLYPHS stays next after this task.

**Status on 25 September 2026 UTC (after HUNTER-EFFECTS, user request): claim
[NATIVE-READABILITY](NATIVE-READABILITY.md) next, before RESULT-TITLE-GLYPHS.** The user found
the recovered C++ reads as annotated assembly; D-0003 is updated with measurable rules. Three
parts, one pull request each: rules, `.clang-format` and the address-to-native-symbol index
(tier 2); the simulation (tier 1); presentation (tier 2). No behaviour or format change.

**Status on 25 September 2026 UTC (end of HUNTER-EFFECTS): HUNTER-EFFECTS is accepted; claim
[RESULT-TITLE-GLYPHS](RESULT-TITLE-GLYPHS.md) next.** All 36 race tracks match the original; the
HUNTER tag effects are simulated and drawn. Pack v14; other tracks' states are `URTRnn06`. Live
play needs pack v14.

**Status on 25 September 2026 UTC (end of TILE-PAIRS-8-12-26): TILE-PAIRS-8-12-26 is
accepted; claim [HUNTER-EFFECTS](HUNTER-EFFECTS.md) next.** The loop and tile pairs 8, 12 and
28 are recovered; 35 of 36 race tracks match. Pack v13; other tracks' states are `URTRnn05`.
Live play needs pack v13. `coverage static-map` needs the four raw coverage files STATIC-CODE-MAP
names, each after its own `--coverage`, from the main checkout's `local/evidence/`.

**Status on 24 September 2026 UTC (end of LOCKED-TOURS): LOCKED-TOURS is accepted; claim
[TILE-PAIRS-8-12-26](TILE-PAIRS-8-12-26.md) next.** Every track is reachable in the laboratory
(`capture --unlock-tours`); pack v12; other tracks' states are `URTRnn04`. Live play needs pack v12.

**Status on 24 September 2026 UTC (end of RACE-FINISH-BREADTH): RACE-FINISH-BREADTH is
accepted; claim [LOCKED-TOURS](LOCKED-TOURS.md) next.** New tracks now match the original
through their finishes and one-run results; `track_reference` compares through the result
load and `capture --hold` takes a schedule.

**Status on 24 September 2026 UTC (end of RACE-GUARDS): RACE-GUARDS is accepted; claim
[RACE-FINISH-BREADTH](RACE-FINISH-BREADTH.md) next.** All 16 cold-start race tracks now
play without a native guard; their finishes and result loads are the next comparison.
Other tracks' states are `URTRnn03` (836 bytes); pack v11 is unchanged.

**Status on 24 September 2026 UTC (end of SPECIAL-TILE-RESPONSE): SPECIAL-TILE-RESPONSE is
accepted; claim [RACE-GUARDS](RACE-GUARDS.md) next.** Its record has the scope, the stops
and the first command. **Live play needs pack v11** (`local/classic-pal-crawler-tracks-v11.pack`
in the main checkout); rebuild `app-debug` after pulling, since a stale build refuses it. The
user asked (24 September) for tasks to follow one another, each closed out fully through its
pull request, until 50% weekly usage or the five-hour limit; the goal is a fully playable
native Unirally (all tracks, stunt mode, menus).

**Status on 23 September 2026 UTC (end of the TRACK-BREADTH session): TRACK-BREADTH is
accepted; claim [SPECIAL-TILE-RESPONSE](SPECIAL-TILE-RESPONSE.md) next, after the weekly
reset on 2026-09-24T08:00Z or on an explicit user override** (weekly usage was 96%). Its
record has the scope, the stops to recover, the inputs and the exact first command. The other
follow-ups are listed in TRACK-BREADTH's handoff; each is its own task.

Before any build in the main checkout, pull and rebuild `app-debug`: live play needs the v10
pack (`local/classic-pal-crawler-tracks-v10.pack`, already there), and a stale build refuses
it. The paragraphs below are the history of this session's parts.

**Status on 23 September 2026 UTC (after TRACK-BREADTH part 3): TRACK-BREADTH
is in progress; continue it.** Part 3 is reviewed and merged by pull request
(closeout `artifacts/track-breadth-part3-integration/closeout.json` in the main
checkout). Pack profile **v10** carries the 16 cold-start race tracks, and native
starts any of them by id (`--start classic.track.NN`, `frontend run --track NN`)
on its own scenario. Eight match the original over about 1,500 released-controller
updates. Of the other eight, seven stop at a native guard and PINGPONG diverges first;
some exact tracks also reach a guard later in a race, so play can abort (CROCK and
WARIO PAINT at the special tile, HYBRID at `inverted AI marker is unrecovered`). **Live play needs the v10 pack**:
`local/classic-pal-crawler-tracks-v10.pack` is in the main checkout; rebuild
`app-debug` after pulling, since a stale build refuses it. The user played EAST
(`--track 33`) and LOOPER (`--track 10`) live with a gamepad after the merge: a full
race, the result screen and Race Again on each, with no stop. CROCK, WARIO PAINT and
HYBRID stop later in a race even with the controller released. The user saw two
faults in that play. The LOOPER one is fixed (the BG1 picture did not wrap past the
playfield's right edge, R-0046 observation 17), and so is the result title, which now
names the track (observation 18; checked on EAST's and FLAT FUN's original results;
b, h, i, j, k, q, v, w, x, y, z and the digits follow the font's layout, not yet
compared). Still open: the
hint caption's display timing: every 60 updates native's caption differs from the
original's for one frame (LOOPER from update 1,172, HYBRID 1,352). Next, in order of
reach: the special-tile response (7 of 16 compared races stop there inside their
windows, and CROCK and WARIO PAINT just after), HYBRID's `inverted AI marker is
unrecovered` guard at update 2,053, INFINITY's checkpoint guard, PINGPONG's
`opponent.response_b`, then the five locked tours.

**Status on 23 September 2026 UTC (after TRACK-BREADTH part 2): TRACK-BREADTH
is in progress; continue it.** Part 2 is reviewed and merged by pull request
(closeout `artifacts/track-breadth-part2-integration/closeout.json` in the main
checkout): `track_reference sweep` captures the 20 cold-start tracks through the
menu and compares them with native; 6 match exactly over about 1,500
released-controller updates ([R-0046](../docs/research/R-0046-track-breadth-matrix.md)
observations 6-11 and the matrix). Next: make the race scenario data (mode, laps,
initialization frame per track), a pack profile with the reachable tracks' entries,
and `--track <id>` in the runner and app; then the special-tile response, the most
common stop. Carry forward: the match column is for a released controller only, and
five tours are not on the cold-start menu.

**Status on 23 September 2026 UTC (after TRACK-BREADTH part 1): TRACK-BREADTH
is in progress; continue it.** Part 1 is reviewed and merged by pull request
(closeout `artifacts/track-breadth-part1-integration/closeout.json` in the main
checkout): the 45-stream inventory, the general tile producer, four more
playfield shapes and the native idle matrix
([R-0046](../docs/research/R-0046-track-breadth-matrix.md)). Next is the
reference side: capture track 2 through the menu (Down twice on PICK TRACK),
write a generic 742-byte projection, and fill the match column; the exact
command is in the record's handoff. Carry forward: **"completes 1,200 updates"
is not a match**, every track so far runs on ZOOM ZOO's scenario; and the four
new shapes are static readings until a capture executes them. Usage: claimed at
90% weekly on the user's override; later sessions start after the reset on 24
September 08:00Z unless the user overrides again.

**Status on 23 September 2026 UTC (after STATIC-CODE-MAP): no task is
active; TRACK-BREADTH is ready and next, after the weekly reset on 24
September 08:00Z or an explicit user override.** STATIC-CODE-MAP is reviewed
and integrated (Claude Opus 5.5, claimed at 89% weekly on the user's override;
report `a17e77a` on `review/static-code-map`, should-fix items applied);
closeout `artifacts/static-code-map-integration/closeout.json` in the main
checkout. Before designing any capture, read `docs/map/static/code-banks.md`.
Also run `coverage disassemble --out artifacts/static-map/ --coverage <the four
raw captures>` for the listing; the paths are in the STATIC-CODE-MAP handoff,
under `local/evidence/` in the main checkout. Three things to carry forward:
**the unknown share is 40.4%**, above D-0008's trigger, so extend the map with
dynamic evidence, not more heuristics; **`labels.json` follows the records**,
so regenerate the tracked static map when a record cites new code-bank
addresses; and an `inferred` routine is a static reading, never gameplay
evidence.

**Status on 22 September 2026 UTC (after CLASSIC-SPLIT-TIME): no task is
active; STATIC-CODE-MAP is next under D-0008, after the weekly reset on 24
September 08:00Z or an explicit user override.** CLASSIC-SPLIT-TIME is
reviewed and integrated (approved at the first round, report `a7cfc6b` on `review/classic-split-time`, five should-fix items applied at `e225bfd`); closeout
`artifacts/classic-split-time-integration/closeout.json` in the main checkout.
The centred HUD fields now show crossing times and signed splits as the
original does ([R-0044](../docs/research/R-0044-classic-split-time.md)); the
only declared omission left on an ordinary race frame is the off-screen rider
arrow. Two things to carry forward: **the split reads the clock before the
update's own tick**, which is invisible on four updates in five and was
settled by checking every capture's WRAM (`split_probe.py`) before writing
code; and **this host's ASan runtime hangs at startup since the macOS 27.0 /
Xcode 26.1.1 update**, so `lab-sanitize` and `app-sanitize` are unavailable
locally until a one-line ASan program runs again - record them as unavailable,
never as passed, and cite the hosted Linux job. Usage: the task ran from 80%
to about 88% weekly all-models (account-wide, another session was writing
D-0008's records at the same time); the reserve floor was crossed on the
user's instruction, recorded in the task record.

**Status on 22 September 2026 UTC: no task is active; CLASSIC-SPLIT-TIME is
ready and deliberately not started.** This session (Claude Fable 5.1, a
within-provider model change from the Opus 5 sessions before it) came to pick
the next task and sampled usage first, as D-0004 requires: weekly all-models
**79%** at 10:13Z against the 80% review/recovery reserve, resetting
**2026-09-24T08:00Z**. CLASSIC-RACE-HUD cost eleven points with four returned
review rounds, so starting a task on the last point would have breached the
reserve at its first experiment. The next session claims
[CLASSIC-SPLIT-TIME](CLASSIC-SPLIT-TIME.md) - the original's signed split time
in the two centred cells during the race, the larger of the two omissions
CLASSIC-RACE-HUD declared - **after the reset, or on an explicit user
instruction to ignore the boundary** (as D-0004 records once for M4-16). The
record carries the scope, the acceptance table, the scripts to reuse and the
exact first command; sample fresh usage at claim and write it into the record.
Also done this session: the user's records-only commit `60f8f5c` was pushed
(main had been one ahead of origin) and CLASSIC-RACE-HUD's record status and
integration section were rolled forward from its closeout, which the record had
still listed as "to be recorded".

**Status on 22 September 2026 UTC (later session): CLASSIC-SPLIT-TIME is in
progress in `.worktrees/classic-split-time` (claimed about 10:45Z on the user's
instruction; that session owns its record, registry row and closeout). After
it is integrated, claim [STATIC-CODE-MAP](STATIC-CODE-MAP.md), after the weekly
reset on 2026-09-24T08:00Z or on an explicit user override.** This session (Claude Fable
5.1) answered the user's question about how much of the game is reconstructed
(measurements in [D-0008](../docs/decisions/D-0008-static-map-track-breadth-review-tiers.md)
and `docs/STATE.md`) and, on the user's direction, recorded the pivot: a static
annotated code map of banks `$80`-`$83` seeded by the tracked coverage maps
(STATIC-CODE-MAP, tier 2, ready), then a matrix of every track through the
shared engine ([TRACK-BREADTH](TRACK-BREADTH.md), planned; the ROM holds
exactly 45 RNC streams, contiguous in banks `$98`-`$9F`, DRAGSTER first and
ZOOM ZOO second, inventory and regeneration script in the record), and review
tiers by risk (AGENTS.md, `docs/AGENT_WORKFLOW.md`). The user rejected any
execution of original code inside the product; do not re-propose it.
Weekly usage sampled **84%** at 11:20Z, above the 80% floor, so this session
claimed nothing and did no task work; its only commits are records on `main`. At claim:
sample usage, record the tier, and begin with the agreement check in the
STATIC-CODE-MAP record (every observed site must decode identically before any
static inference is added).

**Status on 20 September 2026 UTC (after CLASSIC-RACE-HUD): no task is active;
nothing is blocked. A new session starts from a new user request.**
CLASSIC-RACE-HUD draws the original's in-race HUD - the lap field (`race` on
DRAGSTER, `finish` at the end), the corner clock and both centred finish times -
on the caption's own BG3 layer, replacing the authored bar. No pack change: the
glyphs were already in `presentation.classic.font.v1`, so the profile stays
`classic.pal.crawler.two-tracks.v9`. Closeout
`artifacts/classic-race-hud-integration/closeout.json` in the main checkout.

**Two things from that task are worth carrying forward.** The original's redraw
queue writes at most one HUD field per update, with the left field first, and
every one of the four returned review rounds was about a consequence of it; the
rule and its evidence are in [R-0043](../docs/research/R-0043-classic-race-hud.md).
And the reason each was missed is the same: the kept original pictures step
twenty frames apart, so a sweep over them can score 0 while the updates between
them are wrong. **Capture consecutive originals across every transition a change
recovers**, with `recapture.py`, and probe both tracks - a rule established on
one track was wrong on the other twice.

**Still open from it**, both declared: the off-screen rider arrow, and the
original's **signed split time**, which occupies the two centred cells during
the race (native draws them only at the finish, where they match exactly). The
cheapest next experiment for the split time is an access capture over the
updates on which `$0349` and the opponent's flag are set mid-race.

**Status on 20 September 2026 UTC (after CI-FAST-PATH): no task is active;
nothing is blocked. A new session starts from a new user request.**
CLASSIC-STUNT-NAMES, JEV-JUDGMENT-HARNESS and CI-FAST-PATH are reviewed and
integrated, each accepted conditional only on its recorded final-tip CI and
remote verification; closeouts are
`artifacts/jev-judgment-harness-integration/closeout.json` and
`artifacts/ci-fast-path-integration/closeout.json` in the main checkout, and
CLASSIC-STUNT-NAMES' wherever that session's handoff says. Two harness
changes are now in force: `python3 tools/project.py judge ping|ask|evidence-lint`
asks TypeSafe's Jev a typed question and keeps the answer as an optional check
and an artifact ([D-0007](../docs/decisions/D-0007-advisory-jev-judgments.md);
key in the ignored `.env`, `doctor` reports it as optional); and the hosted
`synthetic.yml` takes a docs-only fast path (about 23 s, still a green run
for the tip, gated on the base commit having a successful run) for pushes
that change only Markdown under `docs/` or `tasks/`, root Markdown or
`.env.example`, and runs the Python tooling tests once per job on the full
path (about 3.3 min). Record-only commits are therefore cheap; code commits
are not, so fold records into the commit they describe where the workflow
allows.
The stunt names took the hint
sentences and the winner, draw and loser lines with them, because all three are
entries of one table; what remains declared from that omission is the start
arrow and ring, the opponent finish time, the off-screen rider arrows, the
animated finish banner and the result art. A new session starts from a new user
request.

**Live play needs a v9 pack.** The build now accepts
`classic.pal.crawler.two-tracks.v9` only, and the launcher selects by profile,
so a v8 pack is refused with the remedy. `local/classic-pal-crawler-two-tracks-v9.pack`
is in the main checkout; to rebuild it,
`python3 tools/project.py content pack --rules tests/manifests/content/classic-crawler-two-tracks-pack.json --out local/classic-pal-crawler-two-tracks-v9.pack`.
Every started task is reviewed, integrated on `main` and accepted conditional
only on its recorded final-tip CI; ZOOM-ZOO-OPPOSING-INPUT's closeout is
`artifacts/zoom-zoo-opposing-integration/closeout.json` in the main checkout,
and ZOOM-ZOO-WINDOW-EFFECTS' is
`artifacts/zoom-zoo-window-integration/closeout.json`.
The "not done" items listed under the closed follow-ups below are declared
omissions kept as future work (authored HUD and result styles, the pack's
alias entries, the register setup behind the window compose rule, the
decorative objects), not unfinished parts of any task. A new session starts
from a new user request; it does not need to resume anything.

**Where local evidence lives now.** The cleanup moved every worktree's
`artifacts/` to `local/evidence/<worktree>/` in the main checkout and every
closeout to `artifacts/<task>-integration/closeout.json` there; `.worktrees/`
holds only live task checkouts. The gate inputs are
`local/evidence/dragster-ordinary-controls/dragster-ordinary-controls/originals`
(DRAGSTER), `local/evidence/m4-16-playable-zoom-zoo/m4-16` (M4-16, including
`boundary-a`/`boundary-b`), `local/evidence/m4-16-rider-art/m4-16-idle/captures`
(idle late start), `local/evidence/m4-15-race-completion/m4-15` with
`local/evidence/m4-15-review/m4-15-review` (M4-15 matrix). The three
opposing-direction races are at
`local/evidence/zoom-zoo-opposing-input/zoom-zoo-opposing-input/originals`. The retention rule
in AGENTS.md says what a closing task must do with its own state. See
[REPO-LOCAL-STATE-CLEANUP](REPO-LOCAL-STATE-CLEANUP.md) for the audit tables
and the deletion log.

**M4-16 is reviewed and integrated on `main`.** Acceptance is conditional on the
final-tip CI and remote verification in the ignored closeout
`artifacts/m4-16-integration/closeout.json` in the main checkout.
Confirm it first. If absent, find the integration commit with
`git log --first-parent main -- tasks/M4-16.md` and its CI with
`gh run list --workflow synthetic.yml --commit <commit>`. M4 is not accepted, no
milestone tag is due, and M4-17 must not be dispatched automatically.

## Provider and review

Claude Opus 5 is the primary under [D-0004](../docs/decisions/D-0004-model-and-usage-budget.md);
independent review is a fresh Opus 5 subagent in an isolated checkout at the
exact candidate. Percentage usage telemetry does exist under this provider and
D-0004's note that it does not is stale: the session tool reports the five-hour
and weekly windows directly, and both belong in the closeout beside the UTC
wall clock. No reset, purchase or provider change is authorized.

## Ready follow-ups

1. **Closed: DRAGSTER live playtest.** DRAGSTER-ORDINARY-CONTROLS' live
   criterion is met: the user played full DRAGSTER races with the keyboard
   (`c36c3e4`) and with the gamepad (`aedc20e`), using jump, brake, Left and
   tricks, through the result and Race Again.

   **Correction to an earlier version of this entry.** It said a DRAGSTER-only
   v1 pack "no longer plays DRAGSTER (the app refuses it up front with the
   remedy)". The app does not refuse it. On 18 September 2026 the user ran
   `frontend run --track dragster --pack local/classic-crawler-dragster.pack`
   and it validated that pack, then silently ran
   `local/classic-crawler-two-tracks-v7.pack` instead and reported the
   substitution as a passing check. The v1 pack cannot play DRAGSTER, which is
   the intended product change, but the launcher substitutes rather than
   refusing, and it cannot tell a typed `--pack` from its own default because
   they are the same path. Recorded with the fix in
   [CLASSIC-PRESENTATION-UNIFICATION](CLASSIC-PRESENTATION-UNIFICATION.md); do
   not restate the refusal claim without running the command.
2. **Closed: ZOOM ZOO opposing-direction input.** The original cannot publish
   them at all: a SNES pad's rocker leaves the port reporting `left & !right`
   and `up & !down`, and on the original, holding Left+Right or Up+Down leaves
   WRAM byte-identical to a released pad. ZOOM ZOO's native path used to pass
   both bits to the engine and diverge from the first update; DRAGSTER already
   dropped them at its call sites. The shared engine now applies the rocker
   once, for both tracks ([R-0041](../docs/research/R-0041-opposing-directions.md),
   [ZOOM-ZOO-OPPOSING-INPUT](ZOOM-ZOO-OPPOSING-INPUT.md)). Three frozen ZOOM ZOO
   originals hold opposing directions over complete races - a 1,000-update
   riding window, both axes together, and the countdown plus the whole result
   screen - and native matches every 742-byte row and restore. Two of them
   reproduce accepted contracts byte for byte (the idle late-start case and the
   M4-16 primary) although their timelines differ, which is the equivalence
   itself. What this game's own branches would do with both bits set stays
   unrecovered: a standard rocker pad, through the audited core, cannot present
   it.
3. **Closed: DRAGSTER window timing and shape** (implementation `788a877`,
   review `9b54c2a` approve, four advisories dispositioned at `598564f`,
   integrated on `main` at `67b0f28` with green final-tip CI on both platforms).
   DRAGSTER-WINDOW-EFFECTS recovers the
   mechanism (R-0040) and draws the countdown, GO and winner windows from the
   original's own per-frame channel-6 table selection, with no pose-pair gate.
   Native's member equals the original's `$11FD` pointer on all 1,922 frames
   from 1533 to 3454. It adds pack profile
   `classic.pal.crawler.two-tracks.v8` (56 entries); a v8 pack is required for
   the new behaviour and the launcher accepts `-v8.pack` beside a v1 pack.
   Of the pixels the change touches, 584,534 of 604,051 now match the original
   against 70,885 before, and the accepted frames are untouched.
   The opponent-won banner after its 120-update counter was recovered by
   the presentation unification below (R-0040, "Established later"). ZOOM
   ZOO's own window content shares the mechanism and is a separate follow-up.
4. **Closed: DRAGSTER 10:00 race limit.** DRAGSTER inherited the limit with the
   shared engine and needed no gameplay change: native matches the original for
   30,554 consecutive updates, with only the two-update SPC700 result-loading
   wait differing (as in ZOOM ZOO). The result screen now writes NO TIME for a
   timed-out player, admitted only with the clock held at 9:59.9. See
   [DRAGSTER-CLOCK-LIMIT](DRAGSTER-CLOCK-LIMIT.md) and R-0039.
5. **Closed: one renderer for both tracks, track content as data.**
   [CLASSIC-PRESENTATION-UNIFICATION](CLASSIC-PRESENTATION-UNIFICATION.md)
   is reviewed (implementation `f8645d7`, rebased onto `main` as `e59744e`; review `a7f26c4`) and integrated, and the user played a full DRAGSTER race with the gamepad on it with 0 fallback frames (`artifacts/play-dragster.json`);
   acceptance conditional on the final-tip CI in the closeout named in
   `docs/STATE.md`. Read the task record's fifth measurement before trusting
   the fourth: the frozen contract race is the continuous-Right replay and
   its inputs are on record, so no recapture was needed. What remains open
   from it, as recorded in its handoff: the authored HUD and result styles,
   and dropping the eight `zoom.*` engine-table aliases in a later pack
   profile. ZOOM ZOO's own countdown and winner windows are done by item 7.

   For a new track or scenario the pattern is now: engine content through
   `zoom_zoo_content` with the track's own overrides, presentation content
   through `classic_race_presentation_content(pack, track)`, and no new
   renderer or loader.
6. **Closed: the countdown windows kept animating through a pause**, found by
   the user playing that build. [DRAGSTER-WINDOW-PAUSE](DRAGSTER-WINDOW-PAUSE.md)
   is reviewed and integrated (approved at `1d37c6a`, report `067dbfe`);
   the windows now follow the original's drivers update by update, with the
   banner's true two-driver rule recovered on the way (the reviewer's odd-length
   pause capture decided it against a plausible wrong reading). To see it:
   pause during the countdown, and the digit disappears until you resume and
   then carries on where it stopped; lose a race and the banner runs from the
   opponent's finish while you ride, which is what the original does.
7. **ZOOM ZOO's countdown, GO and winner windows.**
   [ZOOM-ZOO-WINDOW-EFFECTS](ZOOM-ZOO-WINDOW-EFFECTS.md) binds the window
   family for ZOOM ZOO: the original's `$11FD`, read from the existing M4-16
   captures, runs the same members and drivers as DRAGSTER's with one
   difference, the countdown transition member (5 on ZOOM ZOO, 6 on DRAGSTER,
   from the player's start reflection latched at initialization). Found on
   the way and fixed for both tracks: Start held after a pause resume keeps
   the window off and the look still, and every window member covers both
   riders (the countdown members were drawn under them). To see it:
   the ZOOM ZOO countdown now shows the original's start sign, digits and GO
   letters instead of the authored READY/GO text, and the winner banner runs
   from the first finish. Reviewed (returned once: my first compose rule kept
   the player's object under the countdown windows, which the originals
   contradict; every member covers both riders) and integrated; acceptance
   conditional on the final-tip CI in its closeout.

## Launch recipe

```sh
python3 tools/project.py frontend run --track zoom-zoo --preset app-debug --report artifacts/FRESH-live.json
```

Without `--pack` the launcher selects the newest pack under `local/` that
carries the supported profile (`classic.pal.crawler.two-tracks.v8`), whatever
its file name; `--track dragster` runs DRAGSTER from the same pack through the
same renderer. A typed `--pack` must carry that profile: a DRAGSTER v1 pack or
a v5-v7 pack is refused with both profiles and the remedy in the message, not
substituted. For a first extraction add `--rom` with the private locator's
ROM; it goes to `local/classic-pal-crawler-two-tracks-v8.pack` (a path named
after the profile, so a later bump never finds an old file in its way). To
rebuild over an incompatible pack pass `--rom PATH --replace-pack`, which
moves the old file aside with its bytes intact. The launcher asks the built
app for its supported profiles first, so a build from before a profile bump
is reported as "the build is stale: run `python3 tools/project.py build
--preset app-debug`" instead of as a bad pack. On macOS, disconnect a gamepad
by turning Bluetooth off; the Xbox button opens the Games overlay.
