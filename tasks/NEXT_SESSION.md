# Next session

**Status on 22 September 2026 UTC (later session): no task is active; claim
[STATIC-CODE-MAP](STATIC-CODE-MAP.md) first, after the weekly reset on
2026-09-24T08:00Z or on an explicit user override.** This session (Claude Fable
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
CLASSIC-SPLIT-TIME stays ready behind the two new tasks. Weekly usage sampled
**84%** at 11:20Z, above the 80% floor, so nothing was started. At claim:
sample usage, record the tier, and begin with the agreement check in the
STATIC-CODE-MAP record (every observed site must decode identically before any
static inference is added).

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
