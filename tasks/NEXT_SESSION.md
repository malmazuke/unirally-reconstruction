# Next session

**M4-16 is reviewed and integrated on `main`.** Acceptance is conditional on the
final-tip CI and remote verification in the ignored closeout
`.worktrees/m4-16-playable-zoom-zoo/artifacts/m4-16-integration/closeout.json`.
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
2. **ZOOM ZOO opposing-direction input, only smoke-tested.** The controls work
   listed this as untested, and an earlier version of this entry wrongly said
   ZOOM ZOO rejects it. It does not: hidden 800-update runs on `6da0fa0` with
   Left+Right, Left alone and Right alone all exit 0. What is still missing is
   evidence that native matches the original for those inputs over a complete
   race; capture original ZOOM ZOO timelines before claiming it.
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
   Still unrecovered: the opponent-won banner after its 120-update
   finish-animation counter, whose cheapest next experiment is in R-0040 and in
   the task record's handoff; the presentation unification task below is the
   natural home for it. ZOOM ZOO's own window content shares the
   mechanism and is a separate follow-up.
4. **Closed: DRAGSTER 10:00 race limit.** DRAGSTER inherited the limit with the
   shared engine and needed no gameplay change: native matches the original for
   30,554 consecutive updates, with only the two-update SPC700 result-loading
   wait differing (as in ZOOM ZOO). The result screen now writes NO TIME for a
   timed-out player, admitted only with the clock held at 9:59.9. See
   [DRAGSTER-CLOCK-LIMIT](DRAGSTER-CLOCK-LIMIT.md) and R-0039.
5. **Next task: one renderer for both tracks.**
   [CLASSIC-PRESENTATION-UNIFICATION](CLASSIC-PRESENTATION-UNIFICATION.md) is
   planned and approved by the user, to start from `main` at `67b0f28`. The
   simulation scaled and the presentation did not: DRAGSTER runs the shared
   recovered engine with track content as data, while presentation still has
   two renderers, so each original routine shown to be track-independent has
   been shared one at a time and reactively. It carries the opponent-won banner
   gap from item 3, the launcher's pack substitution and the content-model
   cleanups. Begin with the measurement its handoff names: what
   `render_zoom_zoo` already reproduces for DRAGSTER's frozen frames when given
   DRAGSTER content, and the first divergence.

## Launch recipe

```sh
python3 tools/project.py frontend run --track zoom-zoo --pack local/classic-crawler-two-tracks-v8.pack --preset app-debug --report artifacts/FRESH-live.json
```

DRAGSTER runs from the same pack with `--track dragster` and then draws the
recovered race palette cycle (R-0037) and, on a v8 pack, the recovered window
effects (R-0040); the DRAGSTER-only v1 pack keeps the accepted colours and the
accepted pose-keyed window placement. Add `--rom` with the private locator's ROM and a fresh pack
path for a first extraction. v5, v6 and v7 packs are rejected. On macOS, disconnect a gamepad by
turning Bluetooth off; the Xbox button opens the Games overlay.
