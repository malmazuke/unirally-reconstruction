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

1. **DRAGSTER-PALETTE-CYCLE, in progress.** Branch
   `task/dragster-palette-cycle` in `.worktrees/dragster-palette-cycle` holds the
   verified finding (its `tasks/DRAGSTER-PALETTE-CYCLE.md` and R-0037): DRAGSTER's race runs the same `$82:D382-D496`
   palette cycle, frame n drawing index `(n-1334)&15`, and its two fixed palette
   arrays are exactly phases 10 and 7. Next: merge `main`, add the tables to an
   additive DRAGSTER content version, draw the cycle, and show the accepted
   presentation-check counts do not regress. Then independent review.
2. **DRAGSTER 10:00 race limit, not started.** ZOOM ZOO's `$81:C73E-C75B` finishes
   both riders at 9:59.9; native DRAGSTER ignores the limit. Needs original
   DRAGSTER evidence before any change.

## Launch recipe

```sh
python3 tools/project.py frontend run --track zoom-zoo --pack local/classic-crawler-two-tracks-v7.pack --preset app-debug --report artifacts/FRESH-live.json
```

Add `--rom` with the private locator's ROM and a fresh pack path for a first
extraction. v5 and v6 packs are rejected. On macOS, disconnect a gamepad by
turning Bluetooth off; the Xbox button opens the Games overlay.
