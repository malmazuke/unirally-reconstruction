# CLASSIC-STUNT-NAMES - the original's on-screen stunt names

## Assignment

- Status: review. Candidate `9e2570e`; the full matrix passed on it. Started on `task/classic-stunt-names` from `fd34209`. Registered and started
  19 September 2026 07:00 UTC, chosen by the user as the next task after ZOOM-ZOO-OPPOSING-INPUT.
- Milestone: follow-up to M4-16 and CLASSIC-PRESENTATION-UNIFICATION; takes the first item out of
  the "decorative objects and captions" declared omission in [docs/STATE.md](../docs/STATE.md)
- Coordinator: main session
- Task provider: Anthropic (unchanged)
- Worker/session/runtime/model: Claude Code, Claude Opus 5
- Actual model/reasoning effort, routing rationale and frontier escalation question: Opus 5 as the
  session's model; a frontier consultation is not expected - the mechanism is read from captures
  and the ROM, as R-0040 and R-0041 were
- Provider quota window/baseline (D-0004): registration 07:00 UTC five-hour 33%, weekly all models
  49%, weekly Fable 36%; D-0004 reserve 20% of the weekly allowance, checkpoint after a 20-point
  rise. The user set this session's stop rule at **60% weekly**; no reset, purchase or provider change
- Reviewer: fresh Claude Opus 5 subagent in an isolated checkout at the exact candidate, spawned by
  the primary, as D-0006 requires
- Dependencies and evidence of acceptance: M4-16 (the reward events behind the names are recovered;
  their text display is not), R-0036 (rider objects and look tables), R-0040 and
  ZOOM-ZOO-WINDOW-EFFECTS (the channel-6 window family, and the finding that these captions are OBJ
  or BG content rather than windows), CLASSIC-PRESENTATION-UNIFICATION (one renderer for both
  tracks, track content selected from the pack); all integrated on `main`
- Base commit: `main` at `260334d`
- Branch and isolated worktree: `task/classic-stunt-names`, `.worktrees/classic-stunt-names`
- Owned paths (expected; the recovery may move the boundary): `src/core/presentation.{hpp,cpp}`,
  `src/core/rider_look.{hpp,cpp}`, `src/core/classic_race_presentation_runner.cpp`, the content
  pack rules under `tests/manifests/content/` if new entries are needed,
  `tests/native/presentation_tests.cpp`, `tests/native/rider_presentation_tests.cpp`, this record,
  a new `docs/research/R-0042-*`, the coordinator records
- Claim/checkpoint: this record and ignored `artifacts/classic-stunt-names/` in the worktree, moved
  to `local/evidence/classic-stunt-names/` at closeout

## Outcome and boundaries

Draw the original's on-screen stunt names: the captions that name a trick the rider has just
completed. The user raised their absence during live play of M4-16, and the reward events that
drive them are already recovered, so what is missing is when a name appears, which name, where it
is drawn, how it moves or fades, and how long it lasts - measured against the original, not
authored.

In scope: the mechanism behind the captions, whichever of OBJ or BG carries them; the glyph or tile
content they need, added to the pack additively if the existing entries do not already carry it;
native drawing in the shared renderer for both tracks; and frozen original captures behind every
claim.

Out of scope unless the recovery shows they are the same mechanism and come almost free with it:
the start direction arrow, the start ring, the red `MORE STUNTS` hints, the opponent's finish time,
the WINNER caption, the off-screen rider arrows, the animated finish banner and the result-screen
art. Also out of scope: audio, the original HUD and result pixel style, the two-update late result
load after a time-out, and the `zoom.*` pack aliases. If a neighbouring caption shares the
mechanism, take it and say so; do not widen the task to the whole family by default.

## Inputs and prerequisites

- PAL ROM `a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e` through the private
  locator `local/rom-location.txt`; audited bsnes core
  `e59bf88d4fc922c9fe3b5438e65ff3a6909d24e1628f0f87141c8de17699a91b`.
- Pack `classic.pal.crawler.two-tracks.v8` (56 entries). A new profile is only justified if the
  captions need content the pack does not carry; a bump is additive and keeps the accepted v1
  contracts, as v7 and v8 did.
- Existing originals to read before capturing anything new: the M4-16 ZOOM ZOO captures under
  `local/evidence/m4-16-playable-zoom-zoo/m4-16` (whole WRAM per frame, with frame images at the
  kept frames), the DRAGSTER originals under
  `local/evidence/dragster-ordinary-controls/dragster-ordinary-controls/originals`, and the
  window-pause and opposing-direction captures. A race that performs tricks is needed: the M4-15
  primary timeline steers only, so the trick-carrying timelines are the DRAGSTER primary (held and
  tapped B jumps, short and long X rolls, L, R and A+R in the air) and the M4-16 trick probes.
- No known baseline failure: `main` at `260334d` has green CI on both platforms and 23/23 ctest on
  five presets.

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| The trigger is recovered | probe the original's WRAM across a trick-carrying capture, against the recovered reward events | a stated rule for when a caption starts, which name it carries and when it ends, with the addresses and the frames behind it | research record R-0042 |
| The content is authenticated | extract whatever glyph or tile content the captions use from the ROM through the pack rules | byte-exact extraction, additive to the profile, v1 contracts unchanged | pack report |
| Native matches the original where it draws | `caption_pictures.py` and `zoom_captions.py`: the caption band of native renders against the original's own frames, on both tracks | every pixel matches except where the start ring, a declared omission, occludes the caption | picture scores in the gate logs |
| No accepted contract moves | the eleven differential gates, five preset suites, synthetic, v1 contracts, hidden runs, fuzz | all `status=passed`, restore counts unchanged | gate logs |
| Independent review | fresh Opus 5 subagent in an isolated checkout at the candidate | approve, with its own withheld case | review report |
| Hosted CI on the final tip | `gh run list --workflow synthetic.yml --commit <tip>` | both platforms success | closeout |

## Capability and coverage checkpoint

- Native capability delivered / still missing: to be recorded at the candidate.
- Frozen exact-match interval, field set and reference/seed identity: to be declared with the first
  frozen case.
- Dynamic captured inputs still consumed (must be zero for autonomy): expected zero; the captions
  are presentation, driven by the 742-byte state and the pack.
- Relevant branches/transitions exercised: at least one caption start, its life and its end, on both
  tracks, plus a race with no trick at all.
- First divergence and cheapest next discriminating experiment: to be recorded per attempt.
- Trial-wide usage baseline/current, reserve, reset authorization: in the Assignment block and the
  closeout.

## Evidence and attempts

| Attempt | Hypothesis | Experiment | Observation | Next decision |
| --- | --- | --- | --- | --- |
| 1 (07:05-07:20Z) | The captions are driven by the reward queue the engine already publishes | `queue_probe.py` over the DRAGSTER primary capture: every update where the player's read cursor `$0CE7` advances, with the entry it consumed from `$0CC1` | 80 consumptions between 1599 and 3900, events 14, 37, 44, 45, 46 and 47 only. None is in the 72-199 voice range that `$81:C0CE-C18A` diverts, so that range is not what produces these captions | Look at what the original draws on those updates |
| 2 (07:20-07:30Z) | Each consumed event selects a caption | Recaptured the same case with frame images around the first consumptions (`pictures-a`) and read them | The caption is a red phrase in the middle of the screen. 1599 event 44 shows `MORE STUNTS`; 1633 event 45 shows `GIVE YOU`; 1647 event 46 shows `BIGGER BOOSTS`; 1681 event 14 shows `WIPEOUT`. So consecutive event ids carry the parts of a hint sentence and a separate id names a stunt, and the text is selected by the event id rather than by a separate announcement system | Find the string table the event id indexes, and what draws it |
| 3 (07:30-07:40Z) | The phrase is held in WRAM where the drawing code can find it | `text_probe.py`: the WRAM bytes that change across all four caption starts | 64 scattered offsets, no run of four consecutive bytes. The text is not staged in WRAM at all, so it goes to VRAM from ROM | Find the ROM read instead |
| 4 (07:40-07:50Z) | The phrase is in the ROM, and its letters constrain where | `string_search.py`: search the whole ROM for any byte run whose equal and unequal positions match a known phrase, at stride 1 and 2, encoding unknown | `BIGGER BOOSTS` has exactly two candidate sites in 2 MB, both plain lowercase ASCII (`bigger boosts`). The captions are stored as ASCII text, not as tile indices | Find the table's base and the code that reads it |
| 5 (07:50-08:00Z) | The event id indexes fixed-size entries | `access capture` over frames 1590-1612 of the accepted DRAGSTER replay manifest, then the ROM read sites in bank `$17` | `$81:BFE9` reads exactly 16 bytes at `$17:CCB4`, once, on frame 1599 - the update that consumed event 44. So entries are 16 bytes and the base is `$17:CCB4 - 44*16` = **`$17:C9F4`** (file `$0BC9F4`). Decoding by index gives every caption: 1-22 the stunt names (`roll`, `double roll`, `treble roll`, `roll city`, the flip, twist and z flip families, `rollout`, `wipeout`, `last lap`, `head bounce`, `tabletop`, `wrong way`), 23-36 the cheat and mode messages, 37-39 `winner`/`draw`/`loser`, 40-71 the hint sentences, and beyond 71 the voice lines the consumer diverts. Entry 47 and other gaps are 16 spaces, which is how a one-line pause between sentence parts is spelled | Recover the display rule: layer, position, colour, duration, and how ASCII becomes tiles |
| 6 (08:00-08:20Z) | The 16 bytes become tiles somewhere | Followed the accesses around the table read in the same access record | `$81:C019` stores one tile index per character at `$0EA7-$0EB6`, `$81:C034` sets the queue cooldown to 120 and `$81:C057` raises a redraw flag `$0EE7`. Reading the buffer out of the capture gives `80 80 27 29 2C 0F 80 2D 2E 2F 28 2E 2D 80 80 80` for `  more stunts   `, and the other three captions agree letter for letter. The encoding is three runs of consecutive tiles, 16 apart | Find what consumes the buffer |
| 7 (08:20-08:30Z) | Something draws the buffer on a later update | The accesses that read `$0EA7-$0EB6` and the `$2116`/`$2118` writes around them | On the next update `$81:F322` and `$81:F33C` each read all 16 bytes and write a tilemap row: words 6472-6487 with `$3800 | tile`, then words 6504-6519 - 32 words further on - with `$3800 | (tile + $10)`, and `$81:F352` clears the redraw flag. So each glyph is 8x16, drawn as two rows whose tiles differ by exactly `$10`, which is also why the letter runs are 16 apart. With the tilemap based at word `$1800` that is rows 10 and 11, columns 8 to 23, palette 6 with priority: sixteen characters centred at y 80-95, where the pictures show them | Recovered enough to record; next the font glyphs and the blanking rule |
| 8 (08:30-08:35Z) | Something counts the caption down | Read the table's gaps against the hint sequence | Entry 47 and the other gaps are sixteen spaces, and the hint sentence is published as consecutive ids 44, 45, 46, 47. The caption is cleared by publishing a blank entry, not by a timer | Nothing to time; the queue already carries it |
| 9 (08:35-08:50Z) | The pack already carries the font | Rendered `presentation.classic.font.v1` as 2bpp and as 4bpp and looked at the tiles the captions call | 2bpp is right: the sheet is 128 tiles, and rendering `more stunts`, `bigger boosts`, `wipeout` and `flip city` with the recovered arithmetic spells them, including `a`, `c`, `f`, `l` and `y`, which no measured caption contains. Only values 0 and 3 occur in the sheet, so it is a one-bit font stored as 2bpp | Add only the text to the pack |
| 10 (08:50-09:05Z) | The text belongs in the pack additively | New entry `presentation.classic.captions.v1`, entries 1 to 255 of the table; profile `classic.pal.crawler.two-tracks.v9`; native manifest, rules hash and supported profiles updated; pack re-extracted from the ROM | 57 logical entries, exact ROM identity, the app validates the v9 pack and reports it as supported; 23/23 ctest | Draw it |
| 11 (09:05-09:20Z) | The renderer can derive the caption from the state alone | `draw_classic_caption` in the shared renderer: the event under the player's read cursor indexes the table, each glyph draws as two 8x8 tiles `$10` apart at x 64, y 79, in the race CGRAM's colour 22 | The first attempt drew nothing: `movement.rewards` is the *opponent's* queue ($0D11/$0D13), and the captions follow the player's, which is `player_announcements.queue`. With that corrected the caption appears | Compare against the original |
| 13 (10:15-10:20Z) | ZOOM ZOO drives the same captions | `queue_probe.py` over the M4-16 primary original | 93 consumptions, events 14, 15, 37 and 44-59: the same consumer and the same table as DRAGSTER, reaching `last lap` and the longer hint sentences DRAGSTER's race never does | Compare its pictures too |
| 14 (11:00-11:10Z) | The caption persists until the next message | Native against the six kept frames of the M4-16 original | Three disagreed: at 3208, 4840 and 6484 the read cursor still points at the last consumed event and the original shows nothing. `$81:BEA8-BEF1` blanks the display one cooldown after the queue empties, and the engine already carries that as `empty_display`, recovered in M4-16 and unused until now. Honouring it, every caption frame of both tracks matches except where the start ring occludes the caption. DRAGSTER's four frames could not have found this | Rerun the matrix on the corrected candidate |
| 12 (09:20-09:30Z) | Native matches the original where it draws | Rendered the native timeline of the same DRAGSTER case at the four caption frames and compared the caption band (x 64-191, y 78-95) with the original's own frames | Frames 1637 `GIVE YOU`, 1652 `BIGGER BOOSTS` and 1685 `WIPEOUT`: **2304 of 2304 pixels identical**, with exactly the same ink pixels. Frame 1601 `MORE STUNTS`: 2188 of 2304, and every one of the 116 differing pixels is the original's pale pink - the start ring, a declared omission, which composes *above* the caption and occludes it | Gates, then review |

## Handoff

- Current base/head commit and uncommitted state: registered at `260334d`; no work started.
- Verified findings: attempts 1 to 5. The captions are the on-screen half of the reward queue the
  engine already runs: a consumed event id indexes a 16-byte ASCII entry in the table at
  `$17:C9F4`, read by `$81:BFE9` on the consumption update, and the phrase is drawn in red in the
  middle of the screen. The stunt names, the hint sentences and the winner/draw/loser captions are
  all entries of that one table. The recovered `ZoomZooPlayerAnnouncements` fields
  (`hints_active`, `hint_updates`, `hint_group`, `empty_display`) are the state behind which entry
  is chosen; the text and its drawing are what is missing.
- Current hypothesis: the phrase is drawn for a bounded number of updates from the consumption,
  as a row of glyph tiles on a BG layer. Failed approaches: the 72-199 voice range is not
  involved in these captions; a statistical WRAM-diff hunt for "caption state" ranked bytes whose
  change counts merely happened to sit near consumptions ($15CE-$15D2 change constantly from 1329,
  before any race event), so read the original's code and pictures instead of ranking byte churn.
- Exact next experiment/command: find the font's glyph tiles. The caption's tiles are BG character
  indices, so the sheet is whatever the layer's character base points at; find the DMA or the ROM
  region that fills it, add it to the pack additively, and render `  more stunts   ` with the
  recovered mapping to compare against the original's own frame 1601. Then confirm the letters the
  four captions do not contain (`a`, `f`, `l` and the digits first) against a picture, and measure
  a ZOOM ZOO caption, since only DRAGSTER has been measured.
- Recovered so far: [R-0042](../docs/research/R-0042-stunt-name-captions.md) holds the mechanism -
  the trigger, the ASCII table at `$17:C9F4`, the tile buffer at `$0EA7`, the ASCII-to-tile
  arithmetic, the two tilemap rows and the blank-entry rule.
- Remaining dependencies: none; every prerequisite is integrated on `main`.
- Runtime needs: the private ROM, the audited core, the v8 pack, disk for captures, and roughly an
  hour of machine time for a full gate matrix.

## Review and integration

To be completed by the primary after independent review.
