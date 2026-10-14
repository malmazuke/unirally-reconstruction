# R-0097 - Menu input the listing shows and native lacked

Status: research result and native change ([MENU-INPUT](../../tasks/MENU-INPUT.md)), 14 October
2026, on main `e304868`. PAL ROM SHA-256
`a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`, reference core bsnes
`7d5aa1e6`. Evidence in main `local/evidence/menu-input/`. It follows the items R-0091 read from
the listing while placing the menus' sounds, and the two TWO-HUMAN-STUNTS left (R-0095).

Tags: **[L]** read from the ROM's bytes, **[C]** confirmed in a capture.

## Domain

Twelve cold-start captures of the original, replayed in process (`probe.py`: work RAM
`$0000-1FFF` and cartridge RAM every frame from power-on, a picture every frame from the first
listed): the keyboard (`kb-input`, `league-name`), the league's member picker (`league-pick`),
OPTIONS' ways back (`opt-backs`), the two warnings (`warnings`), PLAYER SCORES and GROUP TABLES
(`records-input`, `scores-input`, `scores-carry`, the last two with records written after frame
800), TWO-HUMAN-STUNTS' review capture W5 (`w5`), the VS challenger and the continuation
(`vs-challenger`, `twop-quit`), and a league's POINT AWARDS (`awards-press`). Each is compared
with native's front end on every menu frame (`compare.py`: the arrow, the palette cycle, the
logo, the slides, the decorations, the menus' words, the OAM buffer, the text map) and every
picture; eleven sound schedules are captured twice each (`cue_captures.sh`) and compared with
native's cues frame by frame, frame waits included. `pcs.py` places routines on frames with the
core's instruction ring. The routines were read from the static map's listing and a linear
decode (`disasm.py`) before each capture (D-0008).

The menus' pad tests (`$80:B6D3-B82B`) read pad 1 and pad 2 through the gate `$77:0742` bits 9
and 10 (R-0095): Left `$80:B7B9`, Right `$80:B7DE`, Up `$80:B803` (and `$80:B76F`), Down
`$80:B830`, Down or Select `$80:B794`, a choice (B, Start or A) `$80:B71D`, Y or X `$80:B74A`, A
or Start `$80:B8C4`, Select `$80:B8F1`, Start `$80:B916`, and `$80:B6F8` (B, Y, Select, Start,
Right, A, X, L, R). [L]

## Findings

1. **The keyboard** (`$80:A1F2-A529`). [L, C: `kb-input` 1293-1665, `league-name`]
   - It tests its inputs in one order (`$80:A25F-A294`): Left, Right, Up, Down, Start, Y or X, a
     choice. The first one held is the frame's only input, so a held direction hides every
     button (`kb-input` 1505-1510: A while Left is held types nothing). A frame with none of them
     held clears `$008F` and `$0076` (`$80:A297-A29C`).
   - Start goes straight to OK (`$80:B916` to `$80:A491`), with no latch: an empty name is
     refused with `$80:B0FA`'s sound on every frame Start is held (`kb-input` 1320-1323, four
     refusals). It does not move the arrow.
   - Left and Right act on their press, then while held each time the byte `$0076` counts below
     zero: 18 after the press, 6 after each repeat (`$80:A2A0-A2B8`, `$80:A2DF-A2F7`), so at
     the press, 19 frames later, then every 7 (`kb-input` Left held 1340-1389: moves at 1340,
     1359, 1366, 1373, 1380, 1387). Up and Down move once a press (`$80:A31C`, `$80:A340`).
     Every move plays the navigation sound.
   - Y or X erases once a press (latch `$008F` bit 5, `$80:A364`). A choice stores the latch
     from the accumulator that still holds the arrow's new y target (`$80:A3BF-A3C3`), so the
     target's low byte (0x70 or 0xF0) comes with bit 4, Y-or-X's bit among it: after a choice
     Y and X do nothing until all is released (`kb-input` 1555-1560).
   - The keyboard starts with the choice latched (`$80:A1F4`), places its cursor and copies the
     OAM in its first frame and polls from the next (`$80:A207-A22E`); native copied the OAM
     before placing the cursor (`kb-input` 1294).
   - A league's name takes 17 characters (`$004B` = 18, `$80:9E78`), a rider's 8 (9,
     `$80:D487`); a character is typed while the length plus one is below `$004B`
     (`$80:A44C-A451`). A league's commit copies the buffer's 19 bytes (`$80:9E9D-9EA2`), a
     rider's 8 (`$80:D4A0-D4B1`). The buffer's bytes past the twelfth (`$00E8-$00EE`) held
     underscores in every league capture; their writer is not identified, and native starts
     them as underscores (`league-name`: twelve characters).
2. **The league's member picker** (`$80:9C8E-9DEF`). [L, C: `league-pick`]
   - Each choice the rider menu's loop (`$80:CBC3`) returns asks to finish when A or Start is
     held (`$80:B8C4`; A at 1920); otherwise B, Y or X marks or unmarks the rider (X at 1720).
   - The loop's choice latch `$0010` bit 0 (`$80:CD14-CD2B`): B, Start, A, Y or X is taken only
     after all five were released. The picker sets it at its start (`$80:9D11`) and after each
     mark (`$80:9D72`, `$80:9D96`): B held with Y marks once (1800-1815), B held then Start does
     nothing (1890-1899). A refused finish (fewer than two or more than eight members,
     `$80:9DD5`, `$80:9DF2`) does not set it, so A or Start is refused on every frame held
     (1740-1747, eight refusals; 1760-1763, four), the message printed each time.
   - A mark prints `$80:9E0F`'s stream at the rider's place (`$80:9E3C`): an X for a member,
     two underscores once unmarked (`$80:9D66-9D6A`; native cleared the cells). The loop
     uploads the text to the half on screen each frame (`$80:9D1B`), so a mark shows the next
     frame.
   - The picker starts on COLIN (`$80:9D0B`) without writing `$017D`, raises the logo a frame
     after its first frame (`$80:9C34`), and leaves the shadow's high bits (`$0C1F`) to the
     loop (`$80:9CEF-9D1F`).
3. **VS refuses the winner as challenger** (`$80:C0B1-C0BB`, the refusal's sound `$80:B0FA`):
   native since TWO-HUMAN-STUNTS. `vs-challenger`: MARTIN wins; MIKE's pad picks MARTIN (Down
   6800, Start 6850: refused), then rider 4 (Start 7000). [C]
4. **POINT AWARDS take a press only after the result's sound** (`$80:B051`, then `$80:C206`):
   native since TWO-HUMAN-STUNTS. `awards-press`: Start 10640-10645, before the sound on the
   awards' frame 85 (10689), and Start for one frame at 10720 are ignored; Start 10760-10762
   leaves on 10762. [C]
5. **The two-player continuation wraps** (`$80:B983-BA5E`): native since TWO-HUMAN-STUNTS.
   `twop-quit` (MODE-AUDIO review's `rv-twop-quit`): the fifth Down picks NEXT TRACK; in
   `vs-challenger` Up at 7100 wraps to QUIT and Down at 7140 back. [C]
6. **The second player's pick does not slide** (`$80:CBC3`, R-0095): native since
   TWO-HUMAN-STUNTS; both 2P/VS captures' rider menus are equal on every frame. [C]
7. **GROUP TABLES** (`$80:DDFC`). [L, C: `records-input`, `scores-input`, `scores-carry`]
   - Up shows the next league and Down or Select the one before, wrapping over the six, each
     latched in `$008F` (bits 3 and 2) and with the result's sound (`$80:DF37-DF77`, `$80:DF46`,
     `$80:DF6C`). The league is `$00CC`, the last league slot used: GROUP TABLES starts there
     (`scores-carry`: slot 3 chosen in DEFINE LEAGUE, then GROUP TABLES shows league 3).
   - The pad tests run inside the bars' pass, after the first bar's step (`$80:DEDB-DF96`): an
     action leaves the other bars unmoved for that frame. Y or X or a choice leaves.
   - A change prints the table again without clearing (`$80:DE38-DE96`): the name line and
     highest score (`$80:E184`), the rows' dashes (`$80:E12B`), each member's name (`$80:E1B1`,
     its row advanced only for a member), and the bars' targets: 88 plus the row's share of 128
     pixels (`$80:9AB2`, the share as PLAYER SCORES'). The next frame only uploads the text
     (`$80:DE9B`).
   - The rows are sorted by `$80:EFD4-F024`, a selection sort: each pass takes the last row of the
     lowest score among rows 0 to n (a tie goes to the later row) and swaps it with row n, so tied
     members do not keep their order (review `rv-scores`: league 1 scoring 5/10/10 lists the third
     member above the second; league 2, 3/8/3/8, lists riders 6, 4, 3, 5). [L, C]
   - Each bar's end object steps a pixel a frame toward its target (`$80:DEEF-DF36`) and, off a
     multiple of 8 with x / 8 in 12-26, prints the bar's edge (a filled cell, then an empty
     one) at its column (`$80:DF04`). The loop steps no decorations.
   - The entry: the colours, the objects (`$80:E04B`, `$80:DE16-DE24`), no colour math, the arrow
     away, the text and the table; that print ends in the next frame, which has no frame wait
     (`records-input` 1601-1602); the forward slide from the third frame with `$80:DFC2`'s hook (the
     objects off screen at offset 0x9C, `$80:DFE8`, which also turns the colour math on); then
     `$77:0742` bit 5 set, no colour math, OBJ window 1 inverted at 104-231, and the loop
     (`$80:DEA9-DED9`).
   - The way back (`$80:DF99-DFBE`): bit 5 cleared, the back slide with `$80:E010`'s hook (the
     objects and the window move right, the window's registers written each pass, `$80:E2A2-E2A9`),
     then `$80:D503`, the window off, colour math on, the logo down (`$80:F52B`) and the RECORDS
     loop from the slide's last frame without a new print (`$80:D53F`).
8. **PLAYER SCORES** (`$80:DACF`). [L, C: `records-input`, `scores-input`, `scores-carry`]
   - RECORDS' choice raises the logo with its slide (`$80:D54B`, `$80:F51B`) for both screens.
   - Up is tested first (the next rider), then Down or Select (`$80:B794`, the one before), each
     with the result's sound (`$80:DBE7-DC12`); neither is latched: after a change the figures print
     (`$80:DB59-DB8C`), the rider's colours and text follow (`$80:DB8F-DB9E`), and `$80:C24C` waits
     for both pads' release, the bars still, the decorations stepping (`scores-input`: Up held
     1420-1449). Up and Down together take Up (1520).
   - It leaves on B, Y, Start, Right, A, X, L or R (`$80:B6F8`; Select is Down), after
     `$80:C206`'s press on two frames running: a frame wait, `$80:A8A8` (an OAM copy and the
     base colours), `$80:D420` (the printed objects hidden), the back slide with `$80:DCB6`'s
     hook, and the RECORDS loop from its last frame without a new print (`$80:DC15-DC3C`).
     Native took Y, X, B, Start, A and Right, from pad 1 only.
   - The rider is `$00CA` masked to 0-15 (`$80:DACF`), not reset. Its writers: DEFINE and RENAME
     PLAYER's pick, Y in their pickers too with bit 7 set (`$80:C139-C13B`, `$80:D457-D459`), the
     1P pick (`$80:BBBE-BBC1`), each NOW PLAYING name line of a real rider (`$80:B5C0`: the rider,
     the opponent, the record's holder), PICK TOUR's medals in 1P (`$80:9759-975C`) and the
     race's setup (`$80:99E8-99EB`, the first rider). `scores-carry`: MARTIN after RENAME
     PLAYER's pick; review `rv-ca`: rider 3 after a 1P pick, rider 7 after Y on RENAME PLAYER's
     picker. The traces of `twop-quit`, `vs-challenger` and `awards-press` show
     NOW PLAYING's and the race's setup's writes (`pcs.py`). [L, C]
   - The bars: their end objects (`$80:DDE4`) slide in with `$80:DC74`'s hook (off screen at
     offset 0x74, `$80:DC9A`), then step a pixel a frame toward 48 plus the figure's share of
     128, printing the bar's edge at x / 8 + 1 on a multiple of 8 with x / 8 in 7-21
     (`$80:DBAC-DC71`). Native drew each bar's final length at once.
   - The figures (`$83:8EFE-9045`): WON% and FAILED% through `$83:91D0` and `$83:9D80` (the
     part halved with the races until below 655, times 100 over the races, both left in
     `$77:10F9`/`$77:10FB`), 100 for all races; LOST% is 100 less both (`$83:8FEC-8FF5`, review
     `rv-scores`: 51 for 6 races, 2 won, 1 failed, where 100 x 3 / 6 would give 50). A share is
     0 for none, 128 for the largest, else both words are shifted left until one has bit 15 and
     the value's high byte (times 256) is divided by the largest's (`$80:9719`) and halved
     (`$83:8F2F-8F5D`): exact below 256, not above (review `rv-bars`: 256 points against 257
     aim at 176, where the exact share gives 175). WON and FAILED share 128 by their percentage;
     LOST's share is 128 less both (`$83:8FDE-8FE8`). With no race, every figure but the score is 0. The largest races and
     score are `$83:97BD`'s, from RECORDS' entry. [L, C]
   - The frames: the print (`$80:DACF-DB0C`); the slide from the second (`$80:DB12`), after which
     the loop uploads through `$00A8` to the half on screen (`$80:DB18-DB1D`); the text to the half
     on screen, the window at 64-191, the icon (`$0C1A` = 0x54) and the decorations
     (`$80:DB26-DB53`); an OAM copy and the figures (`$80:DB56-DB8C`); the rider's colours; then
     `$80:C24C`.
9. **OPTIONS' hidden markers** (`$80:EB23`). The generic list loop (`$80:B93C`) turns PICK
   TRACK's done-track markers (OAM entries 64-68, hidden) from its first frame, and OPTIONS,
   RECORDS (`$80:D53F`) and the league slots (`$80:9F09`, `JSR $B93E`) all run it. Every way
   back into OPTIONS (RECORDS' Y, a picker's Y, the keyboard's OK, the league slots' Y, a
   warning left) is `$80:EF4D`'s print, the back slide and the loop from the slide's last frame
   (`$80:B63C`), with no OAM copy before the slide's first pass (`$80:E29F`); native had two
   more frames (a palette and tiles reload, which the original's way back does not have) and an
   OAM copy. [L, C: `w5` 2262, `kb-input`, `opt-backs`, `league-pick`, `records-input`]
10. **DEFINE and RENAME PLAYER's pick** goes to `$00CA` (`$80:C139`, `$80:D457`), not `$017D`,
    so the next rider menu starts on the last rider chosen there (`w5` from 1460 and 2943,
    `kb-input` 2100, `league-pick` 2450). [L, C]
11. **The two warnings** (DEFINE PLAYER `$80:C17B-C197`, DEFINE LEAGUE `$80:9C07-9C23`) read
    both pads' whole words: B, Start, a direction, X, L or R leaves to OPTIONS (DEFINE PLAYER's
    with the logo down, `$80:C1A8-C1AD`; `$80:9C46`), and only Select, Y and A together with
    nothing else confirm. Native had no way out and confirmed on pad 1's word containing the
    three. `warnings`: Y alone at 1500 does nothing, B at 1550 leaves, pad 2's Select+Y+A at
    2300 confirms, Left at 3400 leaves the league's warning. [L, C]
12. **OPTIONS' other screens, found on the way.** [L, C: `scores-carry`, `opt-backs`, `warnings`]
    - RECORDS, the league slots and both warnings slide in after `$80:EF4D`'s or their own print
      with no OAM copy before the slide's first pass (`$80:9BE8-9BEE`, `$80:C15C-C162`); native
      copied it a frame early (`scores-carry` 1501, 2221, 2541, 3001).
    - The league slots' arrow takes its column from `$80:9F74` (2, 2, -2, 0, 0, 2) for the last
      slot used, which `$80:B93E` keeps in `$009B`; native put every slot's arrow at column 2
      (`scores-carry` 2481-2592).
    - RECORDS (`$80:D525-D53C`) keeps the decorations' delay `$0089`; native reset it to 0
      (`scores-carry` from 3000).
    - The colours a rider picker's HDMA wrote stay in CGRAM after OPTIONS' way back; native
      restored the colours from before the picker, which PLAYER SCORES' icon showed
      (`scores-carry` 1703-1743).

## Results

| Capture | Frames | Pictures equal (native / main `e304868`) | Other differing frames (native) | Sound schedule |
|---|---:|---|---|---|
| `kb-input` | 2,500 | 2,100 of 2,100 / 920 | text 1661 (OK's print) | 1,945 lines equal |
| `league-pick` | 2,800 | 2,400 of 2,400 / 1,280 | none | 2,247 equal |
| `league-name` | 2,800 | 2,143 of 2,400 / 1,280 | 2151-2229, the long name's lag (Not covered) | 5 frames differ, the same lag |
| `opt-backs` | 2,700 | 2,300 of 2,300 / 2,300 | none (main: OAM on 1,168) | 2,110 equal |
| `warnings` | 3,600 | 3,200 of 3,200 / 1,345 | text 2651 (OK's print) | 3,016 equal |
| `records-input` | 2,050 | 1,650 of 1,650 / 770 | text 1601 (GROUP TABLES' first print) | 1,469 equal |
| `scores-input` | 2,350 | 1,950 of 1,950 / n/a | text 1801 (the same) | 1,777 equal |
| `scores-carry` | 3,500 | 3,100 of 3,100 / n/a | text 1361, 3201 (the same two) | 2,938 equal |
| `w5` | 4,200 | 3,800 of 3,800 / 2,592 | text 2221 (OK's print) | 3,636 equal |
| `vs-challenger` | 7,300 | 700 of 700 from 6600 / 700 | R-0095's result and VS CHAMPIONS frames, as main | 9,129 equal |
| `twop-quit` | 7,600 | 1,000 of 1,000 from 6600 / 1,000 | R-0095's result frame, as main | `rv-twop-quit`'s 9,434 equal |
| `awards-press` | 11,200 | 615 of 700 from 10500 / 615 | the awards' slide 10604-10688 and its words, as main | 1 frame: a frame wait at 10603, as main |

Review round 1's captures (`local/evidence/menu-input-review/`), at `cc2e2bf`: `rv-ca` 3,400 of
3,400 (main 2,271), `rv-scores` 2,300 of 2,300 (text 1, GROUP TABLES' first print), `rv-bars`
1,400 of 1,400 (main 828), `rv-pick` 2,200 of 2,200, `rv-warn` 6,700 of 6,700, `rv-kb` 1,879 of
2,300 (the long-name lag from 1832), `rv-records` 1,141 of 2,100 (main 1,060; HIGH SCORES and
TRACK RECORDS, Not covered); sound `rv-pick` and `rv-scores` equal.

Main's runner refuses the league members `scores-input` and `scores-carry` write after frame 800
(n/a). Native's cue lists hold the same lines from frame 620 as the original's.

The front-end sweep against main (every front-end manifest in `local/evidence`, main's and
native's runner state on every frame and a picture every 500 frames, `fe_equivalence.py`):
179 manifests, 1,269,921 frames: 108 equal, 71 differ, none with a different exit
(`sweep/fe-equivalence-cc2e2bf.json`; the same counts at `adb6356`). `classify.py`
(`sweep/classify-cc2e2bf.json`) compares the 545 frame images the differing manifests' folders hold:
64 change, in 12 manifests, each closer to the original (63 now equal, 1 with fewer pixels:
`league/organic-slot-selector` 2000, 49,353 to 505), none further. The fields that differ are this
task's: the hidden markers' OAM in OPTIONS, RECORDS and the league slots (every OPTIONS and league
manifest); `$017D`, the league picker's logo and `$009B` (the league manifests, which define a
league first); OPTIONS' way back two frames shorter (`screen`, `script`); the decorations' delay in
RECORDS; the league slots' arrow column (`organic-slot-two`, `organic-slot-selector`); PLAYER SCORES
and GROUP TABLES frame by frame (the `player-scores-*`, `group-tables*` and
`defined-league-group-populated` manifests); the keyboard's Start (`rename-start-default`).

## Not covered

- **The name check.** OK runs `$83:84D9`, which compares the name with 71 words
  (`$83:8537`) at every position and, on a match, prints `$80:A4C9` and refuses the name
  (`$80:A4B8-A4C6`). Native has no check and accepts every name.
- **Long names run past their frame.** That check takes about 39 scanlines a character: from
  about eight characters OK's frame ends without a frame wait and everything after comes a
  frame later. Every full-length rider name (8, DEFINE or RENAME PLAYER) does it (review
  `rv-kb`: native a frame early from 1831), as does a long league name (`league-name`, twelve
  characters: the check ends in frame 2151 at scanline 93; native's way back from 2151 is a
  frame early, 2152-2229). With one or two characters it ends inside OK's frame (`w5`,
  `kb-input`, `league-pick`). Predicting it needs the check's CPU cycles.
- **One-frame text buffers.** After OK, `$80:EF4D`'s print of OPTIONS runs past its frame by an
  amount that depends on the name; native prints a fixed part (R-0072), so the text buffer
  differs on that frame (`kb-input` 1661, `w5` 2221, `warnings` 2651); no picture differs.
  GROUP TABLES' first print ends in the next frame; native prints it at once (`records-input`
  1601, `scores-input` 1801).
- POINT AWARDS still print in place where the original slides them in (R-0095): `awards-press`
  10604-10688, as on main.
  Native also ends the result's way out (frame 10603) with a frame wait the original's league
  scoring runs past (R-0095's placings), as on main.
- HIGH SCORES and TRACK RECORDS keep their earlier implementation (R-0072); their entry's logo,
  frame waits and way back were not recaptured (review `rv-records`: 1,141 of 2,100 pictures
  equal, main 1,060). TRACK RECORDS now reads either pad through the gate (`$80:E449-E4B7`,
  `$80:D70F-D714`). HIGH SCORES waits with `$80:C206` (`$80:D9B2`): any button of either pad
  on two frames running; native leaves on pad 1's B, Y, X, Start or A at once.
- `$00CA`'s other writers, from the listing: the result rows (`$80:CFA9`, `$80:D00F`; traced in
  `twop-quit`, they leave the first rider), the lap result's (`$80:9041`, `$80:90D9`), the stunt
  result's (`$80:F227`, `$80:F2AC`), the 1P scoring (`$83:879E`, the first rider) and NOW
  PLAYING's Race (`$80:B4C0`, a value pulled from the stack and overwritten by the race's setup 7
  frames later). After a VS challenger pick the original runs NOW PLAYING's print
  (`$80:B18D`, `vs-challenger` 7000-7002), whose name lines write it; native does not. A 2P or
  VS lap or stunt result may therefore leave another rider for a later PLAYER SCORES.
- `$80:FBEE`'s menu (the league's continuation after the podium) runs the generic list loop, so
  it also turns the hidden markers and plays the move's sound; native's does neither. Not
  captured.
- PLAYER SCORES on a record whose wins equal its races but whose failed count is above 0: the
  original skips the FAILED figure, native computes it (review round 2). A played game does not
  write such a record.
- The buffer's tail (`$00E8-$00EE`) before a league's naming: underscores in every capture, the
  writer not identified.

## Reproduction

From a worktree of the candidate with `local/` linked, under `local/locks/heavy-run.sh`:

```
E=local/evidence/menu-input; B=$E/base-e304868/front_end_runner
C=build/lab-release/src/core/front_end_runner
P=local/classic-pal-crawler-tracks-v39.pack
python3 $E/build_manifest.py NAME FRAMES DESCRIPTION SPEC...  # each NAME.json keeps its description
zsh $E/probe_captures.sh kb-input:400 league-pick:400 ...     # the original's runs
python3 $E/schedule.py NAME --manifest $E/NAME.json
zsh $E/cue_captures.sh NAME                                     # two sound captures, the cue list
zsh $E/checks.sh                                                # every comparison, native and main
python3 local/evidence/classic-pause-menu/fe_equivalence.py $B $C $P $P OUT.json 4
python3 $E/classify.py OUT.json $B $C $P CLASSES.json           # the sweep's differences
```

`pcs.py MANIFEST FROM TO BB:AAAA=label...` lists the frames and scanlines a routine runs on;
`peek.py`, `pixdiff.py` and `sidebyside.py` show one capture's words and pictures beside
native's.
