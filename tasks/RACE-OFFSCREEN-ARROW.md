# RACE-OFFSCREEN-ARROW - the race's off-screen rider arrows, and the picture residues R-0061 found

## Assignment

- Status: **accepted** (tier 2, pull request #36, merged in `2ffcea0`; round 2 approved). Queued 26 September 2026 (UTC) by RACE-RIDERS-OPPONENTS; claimed
  26 September 2026 at 14:35Z by the Claude Code desktop session that ran FRONT-END-ENDINGS, on
  `16fe22c`.
- Milestone: M4 (original game coverage)
- Coordinator: the claiming session is coordinator, primary and integrator
- Task provider (fixed for all children; record any user-initiated platform change): Anthropic
- Worker/session/runtime/model: Claude Code desktop app, Claude Opus 5.5 (`claude-opus-5-5`)
- Actual model/reasoning effort, routing rationale and frontier escalation question (if any):
  **tier 2** (the race's picture), unless the arrow needs race state the engine does not carry.
- Provider quota window/baseline timestamp, used/remaining or unknown, reserve and session
  allowance (D-0004): at claim the 5-hour window was 19% used and the weekly window 53%. The
  user's allowance: continue until the weekly window reaches 80%.
- Reviewer (primary automatically spawns fresh model/effort, isolated checkout; no user
  trigger): a fresh Anthropic subagent.
- Dependencies and evidence of acceptance: the race HUD (R-0043), the riders and opponents
  (R-0061).
- Base commit: the `main` tip at claim.
- Branch and isolated worktree: `task/race-offscreen-arrow` in `.worktrees/race-offscreen-arrow`.
- Owned paths and shared interfaces: the race's HUD and picture (`race_hud.cpp`,
  `presentation.cpp`), native tests, a research record, this record, `docs/STATE.md`,
  `tasks/README.md`, `tasks/NEXT_SESSION.md`.
- Claim/lease/heartbeat/checkpoint location: this record.
- Session time limit, concurrency allocation and actual spend authorization if relevant: one
  worker plus the review subagent; no monetary spend.

## Outcome and boundaries

(As queued; R-0063 found the arrow is not an off-screen indicator: it shows whenever the player is
behind.) When the other rider is off screen, the original draws an arrow in the HUD's ink, in the race's text layer (BG3),
at rows 14-15: columns 5-7 when it is behind (on the left) and 26-28 when it is ahead (on the
right). Native has never drawn it; R-0043 declared it an omission. R-0061's captures show both
sides: SILVIA and GOLDWYN get ahead of the player, and the arrows differ on 26 to 32 of those
races' 43-51 kept frames. They are the only difference left in those races' pictures. Find its writer,
what it reads (the riders' positions, and maybe the distance), and when it appears and goes, and
draw it frame for frame.

Two more residues from R-0061's `silvia-runner-25` capture belong here:
- **The BG3 upload order.** A new caption reaches the screen a picture late on frame 1846, and a
  blank does after a dry queue (R-0061's measured rule). Both go through the NMI's upload flag
  `$0EE7`, served one task per frame behind the HUD's uploads (`$81:E49F`, `$81:E6F6`,
  `$81:E79B`, then `$81:E831`). Model that order and replace the measured blank rule with it.
- **MIKE's upper body on 1849-1850** shows its next shape two pictures early (63 pixels) when the
  opponent is ahead; with BRONSEN behind the same frames match. Probably the rider look (R-0036).

Out of scope: two-player play.

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| Pictures | `local/evidence/race-riders-opponents/race/pictures.py` on R-0061's seven captures, and the M4-16 primary's kept frames | 0 differing pixels, or each residue explained | logs, research record |
| Timing | Consecutive frames around the arrow's appearance and disappearance, both sides | Equal to the pixel | pictures |
| Nothing moves | The gates of RACE-RIDERS-OPPONENTS | Unchanged apart from the arrow's pixels, each accounted for | logs |

## Result

[R-0063](../docs/research/R-0063-race-arrow-and-bg3-uploads.md). The "off-screen arrow" is a
direction arrow the one-player race NMI draws whenever the player is behind, whatever the screen
shows, with a length that shows the opponent's lead. The late captions come from the NMI uploading
one BG3 field a frame, the caption last. MIKE's early look came from the head point, which the
original keeps from the rider's last contact. All three are native, and the race's pictures now
match the original's on every compared frame, including the M4-16 primary's and the HUNTER tour's.
A research worker decoded them; an implementation worker wrote the code; the primary integrated.
Tier 2 stands: the race's state does not change.

## Evidence

`local/evidence/race-offscreen-arrow/`: `decode/` (the research, its replays and checks),
`checks/` (the implementation's picture sweeps), `base-16fe22c/`, `gates.sh`.

| Criterion | Result |
| --- | --- |
| Pictures | R-0061's seven captures: 0 differing pixels on 307 frames (7,767 before). The M4-16 primary, brake and trick-long kept frames: 0 (9,086 before). The eight HUNTER captures: 0 (4,938 before). |
| Timing | Consecutive frames around the arrow's appearance, disappearance, length cycle and side switch (silvia-dragster, silvia-zoom-zoo, M4-16): 0 (22,521 before); silvia-runner-25 1836-1864: 0 (944 before). |
| Nothing moves | The gates on `21d623b` (`local/evidence/race-offscreen-arrow/gates-21d623b.out`, 18:43-20:28Z, 105 minutes; a first run of the same head stopped when the disk filled, `gates-21d623b-disk-full.out`). The three presets build, ctest 27 of 27, the synthetic suite, both v1 contracts and every hidden app run pass. The eleven differential gates pass. The per-track recompare of both sweeps is identical. Every front-end comparison (main menu, 1P screens, results, the tour's completion, the pause exits, the riders' menus, the endings and reveals) shows no difference and all its pictures equal; the records match at 34 frames. The seven race captures and the track 25 frames: exact on every update and 0 differing pixels. The native equivalence sweep against main's binaries (base `16fe22c`) finds the race state identical on all 1,933,523 updates and 1,047 restarts, and 1,544 of 2,052 pictures different in 334 runs: the new drawing. A sample of 40 of those pictures, rendered on both sides (`checks/equivalence_regions.py`): every differing pixel is in the arrow's cells, except one picture (track 4, `right`, frame 5419) whose 74 pixels are on the player's upper body, the head-point rule. No function is over 80 lines; the address index passes. |

## Review

Tier 2, a fresh Opus 5.5 subagent in an isolated worktree. Round 1 (`085fe28`): approve the code, fix the evidence and records (`review-085fe28.md`): the sweep logs kept, the gate results recorded; and should-fixes (R-0063 on the skipped contact, a HUNTER caption case, two test assertions); answered in `414957f` and `21d623b`. Round 2 (`21d623b`): approved, provided these gate results are recorded (`review-21d623b.md`).

## Handoff

- Exact next experiment/command: after the review and the merge, [HUNTER-ENDING](HUNTER-ENDING.md)
  or [FIFTH-WIN-COMPLETION](FIFTH-WIN-COMPLETION.md).
