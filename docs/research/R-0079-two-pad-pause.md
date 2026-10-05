# R-0079 - The pause in two-pad races

Status: implemented on `task/split-pause-menu` ([SPLIT-PAUSE-MENU](../../tasks/SPLIT-PAUSE-MENU.md)),
5 October 2026; in review. PAL ROM SHA-256
`a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`, audited bsnes core
`7d5aa1e656b9171524d01b1b22917197d8121cb4` (library `e59bf88d4fc9`), Strict serialization. Private
captures and scripts are in main `local/evidence/split-pause-menu/`. It extends
[R-0078](R-0078-pause-menu-picture.md) (the one-player picture) and [R-0060](R-0060-pause-exits.md)
(what the choices do) to the races with a human on each pad: 2P, VS and a league's pairs
([R-0071](R-0071-two-player-versus.md), [R-0073](R-0073-league.md)).

Tags: **[L]** listing only, **[C]** confirmed in a capture below.

## What the original does

- **Either pad pauses.** `$83:CD05-CD28` runs the menu when it is open (`$0EF3`), or pad 1's Start
  (`$0339`) is held while rider 1 has not finished (`$0EFF`), or pad 2's Start (`$033B`) is held
  while rider 2 has not (`$0F01`). Port 2 is a pad in every two-pad race; only a one-player race
  clears it (`$82:AB6F-AB86`). [C: pad 2 opens the menu in 2P, VS and a league pair]
- **The release needs both.** `$83:CD39-CD4B` clears `$0EF3` and `$0EF5` only once neither pad
  holds Start. [C: `twop` 2246-2248, pad 2 still holding Start after CONTINUE GAME: each update
  reopens and closes the menu]
- **The pauser's view.** On an update that finds the menu closed (`$1347` negative), `$83:F6AF-F6F3`
  places it by pad 1's Start: VRAM 0x18A8/0x18ED (the upper view, as in one-player play) when pad 1
  holds it, else 0x1A68/0x1AAD, the same layout 14 rows down in the lower view; `$1347` takes the
  pauser's rider (`$77:0748` or `$77:0749`). [C: `pad2half` at `$83:F6D9` runs only on pad 2's
  openings]
- **Choosing.** `$83:F807-F82E`: pad 1's vertical axis, and pad 2's when pad 1's is centred.
  [C: pad 2's Down and Up move the cursor]
- **Who quits.** `$83:F8D5-F912` writes the quit (0xEA61) or restart (0xEA62) into pad 1's total
  when pad 1 holds Start at the confirming press, else into pad 2's (`$77:07D3`). `$80:88DD` sends
  either total's 0xEA62 back to NOW PLAYING. [C: `twop2` 2840, pad 2 quits, totals 60000/60001, the
  result shows MARTIN QUIT; `vs2` 2140, pad 2 restarts during the countdown, NOW PLAYING]
- **League messages.** In the menu modes whose `$77:10AD` has bit 2 (a league's pairs; not 2P or
  VS), after the countdown with neither human finished, `$83:F6FD-F791` writes only a message: the
  pauser's rider's sixteen bytes of `$83:F516` (eight messages, the table twice, by rider) in row 5
  (or 19) from column 8, characters through the caption alphabet (`$80:81FE`; `!` `$80:8223`, `"`
  `$80:8224`, a space tile 0x80). `$0EF3` stays 1, so Start only resumes. [C: `league` 7500, COLIN
  (7) "interlude" above; 7600, TONY (9) "mellowin'" below; 7700, both pads at once: pad 1's]
- **The picture.** As in one-player play (R-0078): brightness 7 over both views while open, 15 on
  the picture that closes it; the words in the pauser's view, in that view's ink (red above, the
  lower view's own below), under both riders. [C]

## Native

- `run_pause_menu` takes pad 2's Start in every two-pad race (`state.split_screen`; the demo's
  computer riders never reach it), records the menu's view in `pause.lower_view` on an opening,
  clears it on CONTINUE GAME, and lets pad 2's axis choose when pad 1's is centred. The update
  clears the release only when neither pad holds Start. `update_race_for_menus` writes the
  confirming pad's total in every two-pad race; the menus' return sends either human's restart to
  NOW PLAYING in 2P and VS as in a league.
- `pause.lower_view` is race state: a two-pad state appends one byte, always 1, only while it is
  set, so every state without pad 2's open menu keeps its bytes; a league pair's wrapper does the
  same. The reader refuses the byte without an open menu in a two-pad race.
- The picture: `render_classic_race` draws the original's menu, or a league pair's message, in the
  pauser's view (`classic_pause_menu_cells`, `classic_pause_message_cells`), keeps the split HUD
  (drawn after the riders) out of those cells, and dims split races too. M4-16's authored panel is
  gone. Pack profile v35 adds the messages (`presentation.race.pause-messages.v1`); with an older
  pack a message draws nothing.
- Not followed: the CONTINUE GAME clear in a split race (the 129 words from the pauser's
  `$130F`), which `ClassicRaceHudClock` follows only in one-player play.

## Evidence

Captures of the original (`captures.sh`), every frame with work RAM: 2P (`twop`, `twop2`,
`twop-plain`) and VS (`vs`, `vs2`, `vs-now`) on split DRAGSTER from TWO-PLAYER-VS's `mode-1`/`mode-2`
schedules, and a league pair (`league`, `league-restart`) from LEAGUE's `organic-quit-first`:

| Capture | Presses (port:frame) | What it shows |
| --- | --- | --- |
| `twop`, `vs` | 1:2100 Start, Down, Up, Start; 2:2200 Start, Down, Up, Start (held into 2248); 1:2600 Start, Down, Start | each pad's menu in the countdown, the held reopening, pad 1's QUIT after it |
| `twop2` | 2:2600 Start, Down, Up, Start; 2:2800 Start, Down, Start | pad 2's menu after the countdown, pad 2's QUIT |
| `vs2` | 2:2100 Start, Down, Start | pad 2's restart in the countdown, back to NOW PLAYING |
| `league` | 1:7100 menu; 2:7200 menu; 1:7500, 2:7600 messages; both 7700 | menus in the countdown, both messages, both pads at once |
| `league-restart` | as `league` to 7245, then 1:7400 Start, Down, Start | pad 1's restart in the countdown |
| `twop-plain`, `vs-now` | none | the pictures with no pause, and the cold VS NOW PLAYING |

`split_compare.py` runs the native front end with each capture's pads, the race initialized where
the original's was, and compares on every race frame the pause words (`$0EF3`, `$0EF5`), the
countdown, the camera, both riders' laps, finish flags and checkpoint display counts:

| Capture | Race frames | Differences | Race's end |
| --- | ---: | ---: | --- |
| `twop`, `vs` | 662 each | 0 | 2640, pad 1's quit |
| `twop2` | 862 | 0 | 2840, pad 2's quit |
| `vs2` | 162 | 0 | 2140, pad 2's restart |
| `league` | 771 | 0 | none |
| `league-restart` | 412 | 0 | 7440, pad 1's restart |
| `twop-plain` | 921 | 0 | none |

Main's native, given `twop`'s pads, diverges from frame 2200, where pad 2 pauses. Fresh-process
saves (`front_end_runner --restore-check`) round-trip and continue equal at nine frames of `twop`,
`twop2` and `vs2`, five of them with pad 2's menu open, and refuse a lower-view byte of 2.

Pictures (`classify.py`): a differing picture is classed by where its pixels lie.

| Capture | Paused: equal | caption cells only | arrow or colour math | Racing: equal | differing |
| --- | ---: | ---: | ---: | ---: | ---: |
| `twop`, `vs` | 155 each | 0 | 0 | 292 each | 563 each |
| `twop2` | 0 | 60 | 40 | 108 | 502 |
| `vs2` | 40 | 0 | 0 | 159 | 311 |
| `league` | 61 | 120 | 44 | 262 | 223 |
| `league-restart` | 101 | 0 | 44 | 459 | 6 |
| `twop-plain` | | | | 177 | 353 |

The racing pictures that differ are the three gaps below, the results' medal icons (R-0071) and,
in `vs2`, NOW PLAYING after the restart ("Not covered"). The pictures with
the menu open are equal except where three existing gaps of the split race show, which a race with
no pause shows too: in `twop-plain` the candidate's 530 pictures are byte-identical to main's, and
353 of them differ from the original in the same way.

1. The tutorial captions: the original's split HUD (`$81:D853`) prints them in each view's rows 5-6
   and 19-20, where the menu and the messages also go; native prints the player's in the one-player
   rows 10-11 and the lower rider's not at all (R-0071's residual). The original's menu and message
   replace the pauser's caption, so a paused picture differs only in the other view's.
2. A lower-view side arrow in column 28 of rows 20-21 that native does not draw.
3. Where the lower view's rider covers that view's BG3 ink, the original darkens the rider
   (`league` 7201-7244, 37 pixels; 7600-7639, 24 a picture); native adds the upper view's red
   (R-0042). No 2P or VS paused picture shows the difference.

The league messages' cells themselves are exact: 0 of 81,920 pixels differ over pad 1's 40 message
pictures (7500-7539) and the 40 with both pads (7700-7739); pad 2's 40 (7600-7639) differ only in
gap 3's 960 pixels.

## Not covered

- **The CONTINUE GAME clear in a split race**: the original zeroes the pauser's rows 5-9 (or 19-23)
  from column 8, taking the split HUD's cells there until they are rewritten; native keeps them.
- **After a two-human restart** the original re-enters NOW PLAYING through `$80:BC36-BC45`, inside
  the one-player handler: its NOW PLAYING then shows no win counts (`vs2` 2289 on, up to 1,183
  pixels), and the later flow is not captured. Native shows the counts.
- The three split-race gaps above, and the 2P result's medal icons (R-0071).
- A pause opened by both pads at once in the countdown, pad 2's message while pad 1 has finished,
  and other tracks' two-pad races (only DRAGSTER is captured).
