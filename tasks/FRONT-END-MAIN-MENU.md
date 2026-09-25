# FRONT-END-MAIN-MENU - the app boots to the title and the main menu

## Assignment

- Status: ready (queued 25 September 2026 by COVERAGE-ROADMAP, the first of its queue).
- Milestone: M4 (original game coverage: menus)
- Coordinator: the claiming session is coordinator, primary and integrator
- Task provider (fixed for all children; record any user-initiated platform change): Anthropic
- Worker/session/runtime/model: to be recorded at claim
- Actual model/reasoning effort, routing rationale and frontier escalation question (if any):
  the claiming session's model at default effort; **tier 2** (presentation, content and the
  front end's own state; the race state is unchanged).
- Provider quota window/baseline timestamp, used/remaining or unknown, reserve and session
  allowance (D-0004): sample at claim.
- Reviewer (primary automatically spawns fresh model/effort, isolated checkout; no user
  trigger): a fresh Anthropic subagent.
- Dependencies and evidence of acceptance: COVERAGE-ROADMAP (the mode table and its captures);
  R-0053 (the text printer the menus share).
- Base commit: the `main` tip at claim.
- Branch and isolated worktree: `task/front-end-main-menu` in `.worktrees/front-end-main-menu`.
- Owned paths and shared interfaces: new front-end code in `src/core` (its own files), the app's
  start-up in `src/app`, pack rules and content for the title and menu, native tests, a
  research record, this record, `docs/STATE.md`, `tasks/README.md`, `tasks/NEXT_SESSION.md`.
- Claim/lease/heartbeat/checkpoint location: this record.
- Session time limit, concurrency allocation and actual spend authorization if relevant: one
  worker plus the review subagent; no monetary spend.

## Outcome and boundaries

Today the app starts in a race. The original starts at power-on, shows the title, then the main
menu, and only then a mode. Make the native app do the same up to the choice of mode:

- **Boot and title**: what the original shows from power-on to the main menu, with its timing.
  With no input the main menu appears at frame 419 of a cold start; with Start at 300-305 the
  captures show it by frame 450.
- **Main menu** (`$80:ABC8`, COVERAGE-ROADMAP): the logo over the checked background, the five
  entries 1P, 2P, VS, LEAGUE and OPTIONS, and the arrow cursor. Up and Down move the cursor
  (`$9B`, wrapping 0-4), Start chooses. An idle count of 480 frames (`$89`) starts the demo
  (`$9B = 5`).
- **Choices**: 1P enters the existing native race (DRAGSTER, as the app starts now) until
  FRONT-END-1P-SETUP exists. 2P, VS, LEAGUE, OPTIONS and the demo are not native yet: native
  shows a plain placeholder screen that returns to the main menu, and says so.
- **The two controller combinations** the main menu tests (`$72`/`$74` equal to `$02B0` or
  `$8430`, to `$80:A9B4` and `$80:F0D6`): read what they do and record it; implement them only
  if they stay inside the main menu.
- **Structure**: a small front-end state (the screen, the cursor, the counters) with an update
  per frame from the controller, and a picture drawn from pack content, in the style of the race
  code (D-0003). The text printer of R-0053 is the natural shared piece for menu text; draw the
  menu's text through it if the menu uses it.

Out of scope: the 1P setup screens, the other modes and the demo (later tasks), audio.

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| Pictures | Original frames from power-on through the title and the main menu (idle, cursor moves both ways with the wrap, Start on each entry) against native's | Title and menu frames match to the pixel, or each residue is explained | pictures, research record |
| Timing | The same paths | The main menu appears, the cursor moves and the demo would start on the original's frames | log |
| State | The original's `$9B`, `$89` and cursor words along the paths | Native's front-end state agrees frame by frame | log |
| Playable | The app, with a controller: title, main menu, 1P, a race | Reaches the race; the placeholders return to the menu | report |
| Nothing moves | ctest, the synthetic suite, the v1 contracts, hidden runs (which may now need the front end skipped or driven), the equivalence sweep | Unchanged | logs |

## Handoff

- Exact next experiment/command: capture the original from power-on with a frame image on every
  frame to 900 (`coverage capture` with the manifests in `local/evidence/coverage-roadmap/`,
  more `--frame-image` arguments, or `track_reference`'s capture with images), and read the
  title's and the main menu's code from the static listing (`$80:ABC8` and what it calls).
