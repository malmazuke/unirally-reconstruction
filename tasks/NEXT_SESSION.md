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
exact candidate. There is no percentage usage telemetry under this provider;
record UTC wall clock. No reset, purchase or provider change is authorized.

## Ready follow-ups

1. **DRAGSTER live playtest, waiting on the user.** DRAGSTER-ORDINARY-CONTROLS
   is reviewed and integrated but its live criterion is unmet: play a full
   DRAGSTER race with keyboard and with the gamepad, using jump, brake, Left
   and tricks, through the result and Race Again, and record the run. Also
   confirm the deliberate product change: a DRAGSTER-only v1 pack no longer
   plays DRAGSTER (the app refuses it up front with the remedy; the launcher
   upgrades to or extracts a two-track pack).
2. **ZOOM ZOO opposing-direction input, only smoke-tested.** The controls work
   listed this as untested, and an earlier version of this entry wrongly said
   ZOOM ZOO rejects it. It does not: hidden 800-update runs on `6da0fa0` with
   Left+Right, Left alone and Right alone all exit 0. What is still missing is
   evidence that native matches the original for those inputs over a complete
   race; capture original ZOOM ZOO timelines before claiming it.
3. **DRAGSTER window timing and shape, candidate awaiting review.**
   DRAGSTER-WINDOW-EFFECTS on `task/dragster-window-effects` recovers the
   mechanism (R-0040) and draws the countdown, GO and winner windows from the
   original's own per-frame channel-6 table selection, with no pose-pair gate.
   Native's member equals the original's `$11FD` pointer on all 1,922 frames
   from 1533 to 3454. It adds pack profile
   `classic.pal.crawler.two-tracks.v8` (56 entries); a v8 pack is required for
   the new behaviour and the launcher accepts `-v8.pack` beside a v1 pack.
   Still unrecovered: the opponent-won banner after its 120-update
   finish-animation counter, whose cheapest next experiment is in R-0040 and in
   the task record's handoff. ZOOM ZOO's own window content shares the
   mechanism and is a separate follow-up.
4. **Closed: DRAGSTER 10:00 race limit.** DRAGSTER inherited the limit with the
   shared engine and needed no gameplay change: native matches the original for
   30,554 consecutive updates, with only the two-update SPC700 result-loading
   wait differing (as in ZOOM ZOO). The result screen now writes NO TIME for a
   timed-out player, admitted only with the clock held at 9:59.9. See
   [DRAGSTER-CLOCK-LIMIT](DRAGSTER-CLOCK-LIMIT.md) and R-0039.

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
