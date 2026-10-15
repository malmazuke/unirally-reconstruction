# R-0098 - A league through every tour

Status: research result and native change ([LEAGUE-TOURS](../../tasks/LEAGUE-TOURS.md)), 15 October
2026, on main `ceb50cf`. PAL ROM SHA-256
`a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`, reference core bsnes
`7d5aa1e6`. Evidence in main `local/evidence/league-tours/`. It extends
[R-0073](R-0073-league.md) (the league over its first CRAWLER tour) to the league's other tours.

Tags: **[L]** read from the ROM's bytes (the league task's dynamically seeded listings in main
`local/evidence/league/all-listing/` and `cycle-listing/`, and the static map), **[C]** confirmed
in a capture.

## Domain

Cold-start captures of the original, replayed in process, work RAM `$0000-3FFF` and cartridge RAM
every frame, a picture every tenth frame and every frame around each tour choice:

- `tours-a` (283,943 frames): slot ONE with COLIN and TONY (organic-two-entry's OPTIONS
  presses), MIKE's tour level written to 3 after frame 700 (after the cold format) so PICK TOUR
  offers every tour; then SHUFFLER, RUNNER, HUNTER, JUMPER, WALKER, SPRINTER, HUNTER, SHUFFLER and
  JUMPER: 43 events, KEEP SCORES after each podium but RESET SCORES after the first HUNTER, EXIT
  after the last. Captured in nine windows, one a tour (`probe-tours-a-w0` to `-w8`).
- `tours-b` (72,034): the same league, HOPPER then BOUNDER, then EXIT (two windows).
- `odd-tours` (117,810): a three-member league (COLIN, TONY, CRAIG; organic-three-clean's
  presses) through HUNTER and BOUNDER: two pairs an event, the second a human against the
  computer, 18 races (two windows).
- `t-crawler` (39,395): the same two-member league through CRAWLER and EXIT, the first tour with
  the same tools as the others (R-0073's frozen gate stays the first tour's acceptance).
- `picks-l0`, `picks-l1`, `picks-l2` (7,300 each): PICK TOUR in a league at tour level 0, 1 and
  2, every move tried (locked rows and columns), then the standings and NOW PLAYING.
- `podium-back` (16,000): organic-cycle-five's track-cursor intervention (R-0073) to the podium,
  KEEP SCORES, Y on PICK TOUR, PICK TOUR again, SHUFFLER, the standings and its first race.
- `tour-cues` (16,800): the same path with the continuation's moves and RESET SCORES, captured
  twice for the sound schedule (MENU-INPUT's `cue_captures.sh`).

The two-human races were steered in the original by a closed loop (`drive.py`, LEAGUE's
organic-full-tour-turnaround controller, R-0073: the track's markers `$0FC5` for direction and
jumps, a turnaround on steep slopes) and the resulting pads frozen as the manifest; native runs only
that frozen schedule. Together the captures play all 44 tracks a league can race (0-43, every
tour; track 44 is never a league's, below) with two humans, HUNTER's and BOUNDER's with a computer
opponent too.

Each window is compared with native's front end (`compare_capture.sh`): the menus' words and every
picture (`menu_compare.py`, MENU-INPUT's comparison; the podium's own frames, from its entry's
seventh frame to its exit's 103rd, are left out of the words as a race's are, its pictures are
compared); each league race update by update (`race_compare2p.py`, TWO-HUMAN-STUNTS' comparison
reading the league wrapper `URLG`); the cartridge RAM's league fields every 25 frames
(`league_records.py`: membership `$02B2-02BF`, played and points `$02C0-03BF`, pairings
`$05E8-0617`, event totals `$0618-0677`, cursors `$0678-067D`, tracks `$067E-0683`, bonus and
holder `$0684-069B`, `$0742` bit 13), and the whole image outside R-0090's unkept bytes.

## Findings

1. **The tours a league offers.** PICK TOUR takes, outside 1P, the highest level of the sixteen
   riders (`$83:9F30`), against `$80:E842`'s table (00 01 00 01 00 02 00 02 03 03 by tour 0-9): level 0
   opens CRAWLER, SHUFFLER, WALKER and HOPPER, level 1 JUMPER and BOUNDER, level 2 RUNNER and
   SPRINTER, level 3 HUNTER. Native had it (R-0056). [L, C: `picks-l0/1/2`, `tours-a`]
2. **A tour's events.** The awards advance the league's track (`$80:B000-B00B`): one more, 44
   wrapping to 0 (`CMP #$2C`); `$83:9983` then derives the place (`$77:074C`), the race mode
   (`$074B`: places 0 and 3 one run, 1 and 4 lap races, 2 the stunt event, on every tour) and
   the tour; place 0 sets `$77:0742` bit 13, the podium before the next race (`$80:B015-B024`).
   HUNTER's tour is tracks 40-43 in a league: after 43 the track wraps to 0 and the podium
   follows, so a league never races track 44. [L, C: `tours-a` windows 2 and 6]
3. **From one tour to the next.** The same step stores the next track in `$00CE` and its tour in
   `$00D0` (`$80:B00F-B011`), and the podium's PICK TOUR puts its cursor there: after SHUFFLER
   (track 14) on BOUNDER, after HUNTER on CRAWLER. Native left both at the last race's, so its
   cursor started a tour too early and the same presses chose another tour. A choice stores
   `5 x tour` as the league's track (`$80:BED5-BEDE`) and goes straight to NOW PLAYING
   (`$80:BEE2`); Y or X there goes back to `$80:BE16` (the palettes, the badge tiles and, the pair
   cursor being 0, PICK TOUR again, sliding back, `$00AC` still 1 from the continuation), whose
   choice shows the standings first (`$80:BE3F`, `$80:A547`); native went to the slots. [L, C:
   `tours-a`, `tours-b`, `podium-back`]
4. **What ends a league.** Nothing does: after every podium the continuation (`$80:FBD5`,
   `$00AC` = 1 at `$80:FBD7`) offers
   KEEP SCORES, RESET SCORES (`$80:FC32` clears the census played and points) and EXIT
   (`$80:FC53`, back to the main menu through `$80:BF38`); the league's slot keeps its pairing,
   cursor and track, and re-entering it goes on. [L, C: `tours-a`, `tours-b`, `t-crawler`]
5. **The continuation** is the generic list loop (`$80:FBEE`, `$80:B93C`, entries at `$80:FC2F`'s
   columns): no decorations stepped, the hidden PICK TRACK markers turned, the move's sound, a
   wrap past either end, and Y or X act on the entry like a choice (`$80:FBF1` dispatches on
   `$009B` whatever returned the loop). Its PICK TOUR comes from `$80:E550` without `$80:A858` or
   `$80:A82B` before it and slides back (`$80:BEC6`). Native stepped the decorations, which then
   stayed out of phase for the rest of the league. [L, C]
6. **POINT AWARDS** (`$80:8917`, `$80:8A02-8B6C`). The first pass prints the rows (`$80:8A70`, a
   row a frame from the awards' first frame) without the bonus point into the hidden half, which
   slides in (`$80:8B28`, `$80:E27E`) from frame N + 1 with N rows and shows on frame N + 40; then
   (`$80:8B2B`) the bonus line by the race mode (`$80:8B40-8B5D`: one run "most stunt bonus", lap race "best
   lap bonus", stunt event "least wipeout bonus") and a second pass with the holder's point
   (`$77:10F3` bit 0, `$80:8AC4-8AD3`), a row a frame, which ends on frame 2N + 40 with the icons
   104-111 shown, the pairing reordered and the track stepped (`$80:AFF5-B015`); the decorations
   step from the next frame, and the text reaches the screen with the result's sound 41 frames
   later. Native printed everything in place on the first frame (R-0095's residual). [L, C: two
   rows in `tours-a`, three in `odd-tours`]
7. **The pair's count** (`$80:AF54-AFEF`): each rider of the pair plays once more and, with
   another pair to race (`$80:AFC3-AFE7`), the cursor moves on, in the result's way out in the
   frame before the awards' (or the standings'), whose entry long record placings push a frame on
   (`tours-a` 11465 after two placings, 22585 after a stunt event with none); native counted with
   the awards. [L, C]
8. **The event totals** (`$80:F88D`) are stored by the result, not as the race returns: a lap
   race's in its result's second frame (`$80:9107`), a one-run race's in its third (`$80:D15F`),
   a stunt event's only at the end of its result's tallies:
   after rider 1's (`$80:F2DC-F2E6`), or against the computer after the player's with no second
   word (`$80:F275-F27F`, Y = `$FFFF`, `$80:F8A9-F8AF`). Native stored them all as the race
   returned. [L, C: `tours-a` 11159, 17825, 22403, 33901]
9. **HUNTER's tag reverses both humans.** The reversal at `$82:AC5A-ACA7` swaps port 2's words too
   (`$031B` left and right, `$032B`/`$032F` the rotations, `$0327`/`$031F` Y and A), which a
   second human's pad uses; native reversed only the player. [L, C: `tours-a` track 41, effect 7
   at 73922]
10. **NEON with two views** (`$82:D98F-D9AF`, `$82:DC22-DC4D`): track 42 sets `$131F` instead of
    `$12D1`, so the tag effects run and there is no NEON lighting; and the scenery test leaves
    `$77:0750` in A, which becomes the scenery: 8 (12 in VS, [L] only). Native ran NEON's
    lighting and no tags. [L, C: `tours-a` 80806-84410]
11. **A two-human stunt event's qualifying score** is the first rider's medal on the tour
    (`$80:99E8-99F1`, `$83:9EEB`), as in 1P; a cold image gives every rider HUNTER's medal 2, so
    NEON qualifies at 245. Native used medal 0 in two-human races. [L, C]
12. **A league's computer opponent** (an odd member's pair): the rider's best medal on the tour
    read again (`$80:B336-B345`, BRONSEN to GOLDWYN), ANTI-UNI on HUNTER's races (`$80:B361`, in 1P
    and a league), and in a stunt event NOW PLAYING blanks the VS row and prints no opponent and no
    qualifying score (`$80:B359-B35F`, `$80:B390`, `$80:B567`). Native refused HUNTER's computer
    pairing in a league. [L, C: `odd-tours`: ANTI-UNI on 40, 41, 43, GOLDWYN on NEON, BRONSEN on
    BOUNDER]
13. **The lap result keeps a second human's personal best** and shows its markers
    (`$80:90AF-90F2`, against `$77:0829 + 2 x (50 x opponent + track)`), as the player's
    (`$80:9017-905D`); native kept the player's only (R-0090's personal-best gap). [L, C]
14. **The standings from PICK TOUR** leave entries 104-111 as they are (the awards' way out and
    NOW PLAYING hide them themselves; `$80:A547` does not). [L, C: `picks-l0`, OAM equal from 6400]
15. **The podium's sounds around the menus**: on its seventh frame the menu music fades at once,
    as before a race (`$80:99B7-99BE` pattern), and its way back loads the title's sound program
    on its fourth frame, as a race's return does (`$80:A0F7`); native loaded it two frames late.
    [C: `tour-cues`]

## Results

At `08e0dc9` (`final-08e0dc9/`), against the original:

| Capture | League races: rows equal | League fields (every 25 frames) | Menu pictures equal (native / main) |
|---|---|---|---|
| (filled from the final checks) | | | |

The league races (all 44 tracks, 88 two-human and computer races) are equal on every compared
update in every capture. The league fields are equal on every compared frame but the frames
listed under Not covered. The front-end sweep against main `ceb50cf`: see the task record.

## Not covered

- **The podium's way back by a frame.** The podium's teardown reloads the title's sound program
  (`$82:8280-83xx`) and waits for it; its end falls in either of two frames, so the menus' restore
  (`$83:987D`) comes on the exit's frame 103 or 104 (traced with `trace_window.py`: in a late case
  the upload runs on into frame 76 and the wait at `$82:8280` takes frame 77 too). Over eleven
  podiums, all left on the podium's phase 2, four were late (`tours-a` windows 1, 5, 6,
  `t-crawler`) and seven on time; R-0095's rule (late on phases 2 and 3) does not predict it, and
  native keeps it (the frozen gate's podium cases rely on it). After a podium the arrow's spin
  `$00C6` and, after a late one, the palette cycle `$00C8-00C9` are a frame apart from the
  original until the league ends, which shows as the arrow's turn and every seventh frame's
  background colours in later tours' menus (most of the pictures differing in windows 1-8).
  Predicting it needs the sound processor's timing on the menus' side; a follow-up
  (PODIUM-RETURN-TIMING).
- **The podium's own music** (`$82:E08E`'s session and its music start, `tour-cues` 11420 and
  11501-11502): native plays none on the podium.
- **HUNTER's effects in a two-view picture**: the effect captions in rider 1's view, BG effects per
  view, and NEON's scenery with the tags; the race state is equal. These are SPLIT-HUNTER-PICTURES'
  domain, and `tours-a` windows 2 and 6 and `odd-tours` window 0 are captures for it.
- **Split-screen race pictures on the other tracks**: a few 10-22 pixel differences where a rider
  passes under the HUD (SHUFFLER and HOPPER tracks; SPLIT-RIDERS-UNDER-INK's class).
- **Cycle-timed print overruns**, as R-0095 records for CRAWLER: the standings after an odd
  league's first pair start a frame late in the original (`odd-tours` 27880), and the awards'
  header is printed in the result's way out; the awards' rows enter the text buffer a frame
  apart (no picture differs).
- **A race's loading a frame off** in `odd-tours` (NEON and the HUNTER race after it with the
  song counter there; R-0077's race-loading limit): native's own loading starts them a frame
  early; the comparisons pass the original's initialization.
- The odd league's computer race result leaves entries 96-103 at other places (hidden; no picture
  differs), and its event total row for the computer after a stunt (Y = `$FFFF`) keeps an older
  word.
- VS's NEON scenery 12 and the HUNTER stunt event against the computer outside a league are read
  from the listing only.

## Reproduction

From the worktree (local/ linked), each through `local/locks/heavy-run.sh`:

1. Captures: `python3 local/evidence/league-tours/drive.py tours-a --tours 2,4,6,1,3,5,7,8,0
   --continue keep,keep,reset,keep,keep,keep,keep,keep,exit --level-write` (the first run's
   moves assumed the cursor on the last tour; the frozen `tours-a.json` is what it played), the
   same for `tours-b --tours 6,3 --continue keep,exit` and `odd-tours --base
   local/evidence/league/organic-three-clean.json:5900 --first-pick 5950 --tours 8,3 --continue
   keep,exit`; then `probe_windows.py NAME START:END...` (the windows in each `probe-*/probe.json`),
   `probe_captures.sh picks-l0:5600 picks-l1:5600 picks-l2:5600 podium-back:10500`, and
   `cues/cue_captures.sh tour-cues`.
2. Comparisons: `bash local/evidence/league-tours/checks.sh WORKTREE OUTDIR` (every capture with
   the candidate, six with main `ceb50cf`'s `base-ceb50cf/`, and the sound schedule).
3. The worker's final run: `final_checks.sh OUTDIR` (builds, ctest, clang-tidy, the symbol index,
   `checks.sh`, the front-end sweep against main and its classification).
