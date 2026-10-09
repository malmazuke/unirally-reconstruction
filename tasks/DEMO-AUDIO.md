# DEMO-AUDIO - the idle demos' sound

## Assignment

- Status: **queued** 10 October 2026 by [MODE-AUDIO](MODE-AUDIO.md) (R-0091). Unclaimed.
- Milestone: M4 (original game coverage).
- Review tier: **1** (the demo's sound cues and the app's audio producer, D-0010).
- Dependencies: MODE-AUDIO (R-0091), IDLE-DEMO-ROTATION (R-0087), AUDIO-ONE-PLAYER (R-0077).

## Why

Native's idle demos report no sound cues, and the app stops its native audio at the first demo
for the rest of the session. The original reloads the title sound set at each demo's title,
plays a slide, and runs the race's dispatches and effects with that set (R-0091's `demo-cold`
schedule: 5,712 frames differ).

## Outcome

- The demo's title sessions (anchored as D-0010 asks), its screens' sounds and its races' cues
  equal to the original's over `demo-cold` and further demo cycles (split and one-view races,
  both view toggles, a pad exit).
- The app's producer continues through the demos and back to the menus.
