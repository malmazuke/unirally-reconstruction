# CLASSIC-RACE-HUD - the original's in-race HUD

## Assignment

- Status: in_progress
- Milestone: M4 presentation (declared-omission closure), not an M4 acceptance gate
- Coordinator: this session (Claude Opus 5, primary and integrator under D-0006)
- Task provider (fixed for all children; record any user-initiated platform change): Anthropic
- Worker/session/runtime/model: Claude Opus 5, Claude Code desktop session, 20 September 2026 UTC
- Actual model/reasoning effort, routing rationale and frontier escalation question (if any):
  Opus 5, default effort. D-0004's task-specific exceptions cover M4-12 to M4-16; this task follows
  the provider and review rule recorded in `tasks/NEXT_SESSION.md` - Opus 5 primary, a fresh Opus 5
  independent reviewer in an isolated checkout at the exact candidate. No frontier escalation
  question is open.
- Provider quota window/baseline timestamp, used/remaining or unknown, reserve and session
  allowance (D-0004): weekly all-models **64%** used at 2026-09-20T09:27Z (five-hour window 9%,
  resets 14:20Z; weekly resets 2026-09-24T08:00Z). D-0004 reserves 20% of the weekly allowance, so
  the floor for discretionary implementation is 80% used, leaving about 16 percentage points for
  this task **including its independent review**. If the weekly figure reaches 80%, checkpoint and
  hand off rather than continuing; no reset, purchase or provider change is authorized.
- Reviewer (primary automatically spawns fresh model/effort, isolated checkout; no user trigger):
  fresh Opus 5 subagent, separate checkout at the candidate commit, spawned by this session.
- Dependencies and evidence of acceptance: CLASSIC-STUNT-NAMES (the BG3 font, its tile arithmetic
  and the caption compose order), CLASSIC-PRESENTATION-UNIFICATION (one renderer for both tracks),
  ZOOM-ZOO-WINDOW-EFFECTS and DRAGSTER-WINDOW-EFFECTS (the channel-6 members the HUD must compose
  against), M4-16 (the kept original frames and the accepted lap/clock values).
- Base commit: `92f46ba` on `main`.
- Branch and isolated worktree: `task/classic-race-hud` in `.worktrees/classic-race-hud`.
- Owned paths and shared interfaces: `src/core/presentation.cpp`, `src/core/presentation.hpp`,
  native tests under `tests/`, this task record, the research record, `docs/STATE.md`,
  `tasks/README.md` and `tasks/NEXT_SESSION.md`. Shared interface: the 742-byte `ZoomZooState` is
  read-only here; this task changes presentation only.
- Claim/lease/heartbeat/checkpoint location: this record's Evidence and attempts table; local
  artifacts under `artifacts/classic-race-hud/` in the worktree, moved to
  `local/evidence/classic-race-hud/` in the main checkout at integration.
- Session time limit, concurrency allocation and actual spend authorization if relevant: one active
  worker (this session) plus the review subagent; 45-minute reassessment intervals; no monetary
  spend is authorized.

## Outcome and boundaries

Draw the original's in-race HUD. On every ordinary race frame the original shows a red lap counter
at the top left and a red clock at the top right, painted straight over the track with no panel
behind them. Native instead paints an authored dark bar across rows 0-11 and writes its own
five-by-seven font into it. The M4-16 review's scene table records the consequence: on the
`lap_one`, `lap_two` and `finish` scenes the **only** difference between the native picture and the
original's is "HUD glyphs", so this is the last per-frame deviation on an ordinary race frame.

In scope: the mechanism that publishes the HUD - which layer, which tilemap words, which glyph
content, which colour, and the rule for each field's text and position; native drawing of it in the
shared renderer for both tracks; the removal of the authored bar and authored HUD text it replaces;
and frozen original captures behind every claim. The centred finish time and the `FINISH` /
`NO TIME` strings that the same authored HUD writes are in scope where the original draws them from
the same mechanism, because leaving them authored while the corner fields become original would be
a half-converted HUD.

Out of scope unless the recovery shows they are the same mechanism and come almost free with it:
the start direction arrow, the start ring, the off-screen rider arrows, the animated finish banner,
the opponent's finish time and the result-screen art and pixel style. Also out of scope: audio, the
pause menu's own style, the two-update late result load after a time-out, and the `zoom.*` pack
aliases. No gameplay state, serialization or engine behaviour changes here; if the HUD turns out to
need state the 742-byte record does not carry, that is a finding to record, not a licence to widen
the serialized state.

## Inputs and prerequisites

- PAL ROM `a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e` through the private
  locator `local/rom-location.txt`; audited bsnes core
  `e59bf88d4fc922c9fe3b5438e65ff3a6909d24e1628f0f87141c8de17699a91b`.
- Pack `classic.pal.crawler.two-tracks.v9` (57 entries) at
  `local/classic-pal-crawler-two-tracks-v9.pack`. A new profile is justified only if the HUD needs
  content the pack does not carry; the caption font `presentation.classic.font.v1` is already in it
  and is the first candidate for the glyphs.
- Existing originals to read before capturing anything new: the M4-16 ZOOM ZOO captures under
  `local/evidence/m4-16-playable-zoom-zoo/m4-16` (whole WRAM per frame with kept frame images), the
  DRAGSTER originals under
  `local/evidence/dragster-ordinary-controls/dragster-ordinary-controls/originals`, the
  window-pause and opposing-direction captures, and the idle late-start capture under
  `local/evidence/m4-16-rider-art/m4-16-idle/captures` for the 9:59.9 hold and `NO TIME`.
- No known baseline failure: `main` at `92f46ba` is the integrated CI-FAST-PATH tip with green CI on
  both platforms.

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| The mechanism is recovered | access capture over an accepted replay manifest, plus the original's WRAM across a race capture | a stated rule for the layer, the tilemap words, the glyph encoding, the colour and each field's text and position, with the addresses and frames behind it | research record R-0043 |
| The content is authenticated | whatever glyph content the HUD uses is in the pack, or is extracted through the pack rules | byte-exact extraction; any profile bump additive with v1 contracts unchanged | pack report |
| Native matches the original where it draws | a HUD-band sweep of native renders against the original's own kept frames, on both tracks, over every kept race frame that has an original picture | every pixel matches, with no exception claimed | picture scores in the gate logs |
| No accepted contract moves | the eleven differential gates, the preset suites, synthetic, v1 contracts, hidden runs, fuzz | all `status=passed`, restore counts unchanged | gate logs |
| Independent review | fresh Opus 5 subagent in an isolated checkout at the candidate | approve, with its own withheld case | review report |
| Hosted CI on the final tip | `gh run list --workflow synthetic.yml --commit <tip>` | both platforms success | closeout |

## Capability and coverage checkpoint

- Native capability delivered: the original's in-race HUD on both tracks - the left field (the lap
  count on a tour race, `race` otherwise, `finish` once the player's laps run out), the corner clock
  with the original's tenths, its blanking at the finish, and the two centred finish times - drawn
  on the caption's BG3 layer, in the caption's font and colour, composed under the riders with the
  measured red add and under the channel-6 window members. Still missing: the original's **signed
  split time**, which occupies the same two centred cells during the race and is a separate
  mechanism (see below).
- Frozen exact-match interval, field set and reference/seed identity: no new frozen contract. The
  measurement is the picture score against existing frozen originals: all 274 kept frames of the
  M4-16 primary (`boundary-a` timeline, `original-primary` pictures), all 274 of the brake loser
  race, 35 frames of the 10:00 time-out original, and 18 DRAGSTER frames of the accepted
  `race-crawler-dragster-12000-continuous-right-fields` manifest.
- Dynamic captured inputs still consumed (must be zero for autonomy): zero. The HUD is presentation,
  derived from the published 742-byte state, the scenario's race mode and the finish frames the
  presentation history already tracks.
- Relevant branches/transitions exercised, including independent variations: the countdown, ordinary
  riding on both tracks, all three lap changes, the player's finish on both tracks, the loser race
  where the **opponent finishes first** and its time stands alone, the winner banner covering the
  HUD, and the 9:59.9 time-out hold.
- First divergence and cheapest next discriminating experiment: none open for this task. The nearest
  unrecovered thing is the signed split time; the cheapest experiment is an access capture over the
  updates on which `$0349` and the opponent's flag are set mid-race.
- Trial-wide usage baseline/current, reserve, reset authorization/outcome or none: weekly all-models
  64% at 2026-09-20T09:27Z, 65% at 12:0xZ. Reserve floor 80%, not approached. No reset, purchase or
  provider change.

## Evidence and attempts

| Attempt | Hypothesis | Experiment | Observation | Next decision |
| --- | --- | --- | --- | --- |
| 1 (09:40-09:50Z) | The HUD glyphs sit on a tile grid, so their cell origin is measurable | `hud_crop.py` over the M4-16 primary original's frame 3208: the top-left and top-right bands as a colour map | The lap reads `1/3` in three 8-pixel cells from x 16 and the clock `0:32:5` in six from x 192; every glyph's ink is `#e70000`, painted straight over the track with no panel | Find the cell's vertical origin and the glyph content |
| 2 (09:50-10:00Z) | The glyphs are 8x16, two tiles `$10` apart, like the captions (R-0042) | `hud_align.py`: for every candidate origin y 10-21, look each cell's two 8-row ink masks up in an index of every 8-row window of the ROM read as 2bpp | **y 15 is the only origin where every cell's halves are found**, and every hit is 16-byte aligned. So the cells are 8x16 at y 15-30, which is BG3 row 2 (rows appear at `row*8 - 1`, as the caption at rows 10-11 appears at y 79) | Find whether the pack already carries the glyphs |
| 3 (10:00-10:05Z) | The HUD needs new pack content | `hud_search.py` against every entry of the v9 pack at the corrected origin | Every HUD glyph is in `presentation.classic.font.v1`, the caption font, with the bottom half `0x100` bytes (16 tiles) after the top: the same sheet and the same `+$10` arithmetic R-0042 recovered. **No pack change is needed and no profile bump is justified** | Read the sheet's glyph map |
| 4 (10:05-10:10Z) | The sheet's index order continues the caption alphabet | `font_map.py` renders every 8x16 glyph of the sheet | `$01`-`$09` are `1`-`9`, `$0a` is `0`, `$0b`-`$0f` are `a`-`e`, `$20`-`$2f` are `f`-`u`, `$40`-`$44` are `v`-`z`, `$45` is `:`, `$4e` is `/` (R-0042 already had `-` at `$4d`, `!` at `$60`, `"` at `$61`, space at `$80`). The sheet is 16-glyph rows: tops then bottoms, which is why the caption's letters run `..$0f`, `$20..` | Confirm the measured glyphs against the sheet |
| 5 (10:10-10:15Z) | Each cell's ink identifies its glyph | `hud_ink.py`: for every distinct colour in a cell, match both halves against the sheet | ZOOM ZOO 3208 reads `1`,`/`,`3` at columns 2-4 and `0`,`:`,`3`,`2`,`:`,`5` at columns 24-29, all `#e70000`. The same probe on the caption row of DRAGSTER 1601 reads `more stunts` in the same `#e70000`, so the HUD ink is the caption's own colour (native's `colour(cgram,22)`), not a second one | Find the code that publishes it |
| 6 (10:15-10:25Z) | The publisher is near the caption's `$81:F322` | `access capture` over frames 1595-1615 of the accepted DRAGSTER replay manifest, then the `$2116`/`$2118` writes | `$81:ED5C-$81:EDD9` writes exactly four digit pairs every five frames: words `$1858`/`$1878` from `$0E2F`, `$185A`/`$187A` from `$0E33`, `$185B`/`$187B` from `$0E37`, `$185D`/`$187D` from `$0E3B`, each as `$3800 \| tile` and `+$10`. Those are BG3 rows 2 and 3, columns 24, 26, 27 and 29: minutes, tens of seconds, seconds and tenths, with the colons at 25 and 28 left standing | Disassemble the whole dispatcher |
| 7 (10:25-10:40Z) | One routine owns the whole HUD | `disasm.py` (a linear 65816 disassembly written on the project's own opcode table) over `$81:EB40-$81:EDE0`, then a ROM-wide search for every `LDY/LDA #$18xx` naming a row 2 or row 3 word | The left field is dispatched by two flags: `$0D17` selects it at all, and `$0EFB` chooses between the lap number (`$0D15 - $0EFB`, ones digit at column 2, a tens digit at column 1 above 9, through the digit table at `$81:81F4`) and a six-letter word at columns 1-6 whose tiles are read one by one from the letter table at `$80:8200` - `f i n i s h`. The clock is dispatched by `$034D`: positive redraws the digits, **negative blanks columns 24-30 of both rows** with the space tile `$80`, which is how the corner clock disappears at the finish. The ROM-wide search also finds a seven-cell field at columns 13-19 (`$81:E263-$81:E471`) and further writers in `$81:CFF6-$81:E11C`, so the HUD is a family of fields, not one routine | Bound the family by what actually executes in this product's race |
| 8 (10:40-10:55Z) | Only a few of that family run in this product's race | `access capture` over three windows of the accepted manifests - mid-race, the countdown and the finish - and the `$2116` writers in each | Mid-race only the clock writer runs. At the ZOOM ZOO finish the writers run **one field per update**: 6485 the left field, 6486 the blanked clock, 6487 the player's finish time at rows 5-6 columns 13-19, 6488 the caption, 6490 the opponent's time at rows 20-21. The dispatcher is a chain of dirty flags (`$0D17`, `$034D`, `$0349`, ...) each of which redraws one field and returns, which is exactly that cadence | Read each field's content rule |
| 9 (10:55-11:10Z) | The left field is a lap counter on both tracks | Decoded the whole HUD row of every kept frame of the M4-16 primary, the time-out original and the DRAGSTER originals with `hud_sweep.py` | ZOOM ZOO reads `0/3` through `3/3` and `finish`; **DRAGSTER reads `race`** on every frame from 1380 to 3213 and `finish` at 3453. `$81:D6E8` writes `r a c e` and sets `$053F` so the lap number is never redrawn, and `$81:D620` writes `0`, `/` and the total instead; the branch is the race mode `$77:074B`, which the scenario already carries as `tour_race`. At the 10:00 time-out `$0EFB` is not zero, so the original keeps `2/3` and the held `9:59:9` and shows no finish field | Implement it |
| 10 (11:10-11:35Z) | The HUD is the caption's layer, so it can share its machinery | `draw_bg3_text` extracted from the caption; the HUD's four fields drawn through it into the same ink mask, at the same point in the compose order; the authored bar, its font and `classic_race_hud` removed | Builds; 23/23 ctest on app-debug with the new field tests | Measure against the originals |
| 11 (11:35-12:00Z) | Native's HUD equals the original's where it draws | `hud_compare.py` over all 274 kept frames of the M4-16 primary and all 274 of the brake (loser) race, and `hud_compare_manifest.py` over 18 DRAGSTER frames of the accepted continuous-Right manifest | **The HUD rows (y 15-30) differ by 0 pixels on every frame of all three, and so do the rows the authored bar used to cover (y 0-14).** DRAGSTER frames 1400-3453 are **pixel-identical over the whole picture**, the finish frame included. Every post-finish frame of both ZOOM ZOO races matches in the finish-time bands too, including the loser race where the opponent finishes first and its time stands alone | Look at what is left |
| 12 (12:00-12:15Z) | The residual in the two centred bands is mine | `band_look.py` on mid-race frames 3220 and 3900 of the primary | It is not: mid-race the original shows a **signed split time** in those cells - `-0:00:1` at rows 20-21 on 3900 and `?0:01:3` at rows 5-6 on 2080, a sign glyph the caption alphabet does not contain followed by `M:SS:t` - which native has never drawn and this task does not recover. The same cells hold the finish times after the finish, where native matches exactly. Recorded as a newly described omission rather than folded in | Leave the split display out, say so, and measure the change against the base commit |

## Handoff

- Current base/head commit and uncommitted state: base `main` at `92f46ba`; work in progress.
- Verified findings: to be recorded.
- Current hypothesis and failed approaches: the HUD is BG3, like the captions recovered in R-0042,
  drawn from the same font sheet with a different palette and different tilemap rows. Unverified.
- Commands executed, outcomes and report hashes: to be recorded.
- Unavailable/skipped checks: none yet.
- Exact next experiment/command: read the original's BG3 tilemap around the HUD rows out of a kept
  capture and find the code that writes it.
- Remaining dependencies: none outside this task.
- Runtime needs (network, build time, fixtures, memory): the private ROM through the locator, the
  audited bsnes core, the v9 pack, about 3 minutes for an app-debug build.
- Aggregate parent/child time, provider usage before/after (or unknown), other-account-work caveat:
  in the closeout.
- Accepted outcome, review/fix rounds and next routing decision: to be recorded.

## Review and integration

- Reviewer and independent reproduction/withheld-case results: to be recorded.
- Required changes or acceptance rationale: to be recorded.
- Exact merge candidate and required-check results: to be recorded.
- Integrated commit and evidence location: to be recorded.
- Remote synchronization: pushed ref(s), verified local/remote commit IDs, or exact push failure:
  to be recorded.
- Scope still unverified: to be recorded.
