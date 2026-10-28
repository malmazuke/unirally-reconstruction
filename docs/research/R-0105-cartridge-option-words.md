# R-0105 - The cartridge's option words `$77:0742` and `$77:0750`

Status: research result and native change ([CARTRIDGE-OPTION-BITS](../../tasks/CARTRIDGE-OPTION-BITS.md)),
28 October 2026, on main `a6a1b385`. PAL ROM SHA-256
`a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`, reference core bsnes
`7d5aa1e6` (Strict). Evidence in main `local/evidence/cartridge-option-bits/`. It closes R-0089's
queue item 7 and the option-word parts of R-0054, R-0090 and R-0093.

Tags: **[L]** read from the static map's listing, regenerated for this task with the five raw
coverage inputs (`listing/bank-8*.lst` in the evidence folder); **[D]** every recorded access in
the evidence tree's 171 access captures (`census/dynamic_census.py`, output
`census/dynamic_census.txt`), which also count 0 stores through unresolved pointers; **[C]** a
capture made for this task.

## How the census was taken

- Every instruction in banks `$80-$83` whose operand covers `$77:0742-0743` or `$77:0750-0751`:
  129 long-addressed accesses name the words, and `$82:E05A` reads `$77:074F` 16-bit but masks
  `$0750` off (`AND #$000F`) [L]. The mode column decides the width: a 16-bit access to
  `$77:0742` covers bits 8-15 (`$0743`). Nothing
  addresses `$0743` or `$0751` alone, and no absolute access reaches them with the data bank at
  `$77` (`$80:930B`'s `STZ $0742` runs with the data bank `$80`: it is work RAM) [L].
- Whole-image writers: the cold start's wipe (`$83:FB45`, every byte 0) and its defaults
  (`$83:9486`: ten words from `$83:8472` to `$77:073E-0751`, so `$0742` = 0 and `$0750` =
  `0xC28B`) [L, D].
- [D] finds no writer and no reader the listing does not name, and none it names writes a value
  the listing does not explain.

## `$77:0742`

| Bit | Meaning | Set | Cleared | Read | Native |
| ---: | --- | --- | --- | --- | --- |
| 0 | none | - | the 8-bit stores below | - | - |
| 1 | the logo held up | `$80:F51E-F525`, `$80:F542-F549` | `$80:F52E-F535`; every boot `$80:A09F-A0A5` | the NMI hook `$80:F624` (BG1 scroll) | `logo.raised`; now in the image |
| 2 | VS chosen | `$80:BF90-BF98` (`ORA #$04`) | `$80:BC38-BC3F`, `$80:BD6D-BD74`, `$80:BEEC-BEF3` (1P, 2P, league on the way to NOW PLAYING) | `$80:9A05` (into `$0750`) | the race's mode; now in the image |
| 3 | the second rider is a human | `$80:B2BC-B2C2` (NOW PLAYING, `$017F` < 16), `$80:BD0C-BD12`, `$80:BF90-BF98` | `$80:B2F7-B2FD`, `$80:BBC8-BBCE` | `$80:9A05` (into `$0750`) | the race's views; now in the image |
| 4 | the main menu's entry routine | **nothing** | `$80:8891` | `$80:8889` (then `$80:D503`) | read by the audio model (`audio_cpu_menu_reveal.cpp`) |
| 5 | GROUP TABLES shown | `$80:DEAF` | `$80:DF9F` | `$80:DE9E` | `group_table_shown` (R-0097) |
| 6 | none | - | - | - | - |
| 7 | none | **nothing** | `$83:A562-A56C`, `$83:A66F-A679` | - | - |
| 8 | the result's objects parked | `$80:9814-981B`, `$83:A562-A56C` (a tour's ending), `$83:A66F-A679` (the award) | `$80:98CB-98D2` (a lap graph) | `$80:9809` | no picture effect (R-0057); now in the image |
| 9 | pad 1 ignored | `$83:9536` | `$83:954E`, `$83:9560` | the pad tests `$80:B6D6-B929` | `pad_one_ignored` (R-0095); now in the image |
| 10 | pad 2 ignored | `$83:954B` | `$83:9539`, `$83:9563` | the pad tests | `pad_two_ignored`; now in the image |
| 11 | none | - | - | - | - |
| 12 | the last race was lost | `$83:87DC` | `$80:EA1F` | `$80:EA1A` | `race_lost` (R-0057) |
| 13 | a league podium owed | `$80:B021` (`$77:074C` = 0) | `$80:BE81` (the podium), **`$80:9F1E` (leaving the slots)** | `$80:BE7C` | `league_cycle_complete` (R-0098); the slots' clear is new |
| 14, 15 | none | - | - | - | - |

Whole-word accesses [L, D]: the race start saves the word to `$77:106B` (`$80:99C2`) and the
race's return puts it back (`$80:9AA8-9AAC`); the split idle demo stores `0x0A` in the low byte
for its race and restores it (`$80:94BC-94CB`); the menu copies below read the low byte.

## `$77:0750`

Before every race `$80:9A05-9A1D` writes `($0742 & 0x0C) | 2` when `$77:10AD` is 5 (the idle
demo), else `$0742 & 0x0C` [L, D: 43 races in 38 captures, values 0, 8, 0xC and 0xA].

| Bit | Meaning | Read | Native |
| ---: | --- | --- | --- |
| 1 | the idle demo | `$83:C90B` (the demo's view toggle), `$83:C9FA` (no song count), `$83:CBF8` (`$7E:212C`) | the demo (R-0069, R-0070, R-0077) |
| 2 | VS | `$83:EA61`, `$83:EBCF` (a spent life, R-0081) | `versus` |
| 3 | two views | `$81:94AF`, `$81:C57D`, `$81:CD45`, `$81:CE70`, `$82:B15A`, `$82:D5C2`, `$82:D785`, `$82:D99F`, `$82:DC32`, `$83:C9E1`, `$83:CEEE`, `$83:D1D0`, `$83:D33E`, `$83:E944`, `$83:EAD6`; the demo's race toggles it (`$83:C93D-C961`) | two views (R-0069, R-0080, R-0103, R-0104) |

- The other writers are dead stores: 1P's, 2P's and VS's picks copy `$0742`'s low byte
  (`$80:BBD2`, `$80:BD16`, `$80:BF9C`), VS's challenger copies it again (`$80:C0C9-C0D3`, whose
  `ORA #$08` is overwritten by a second `LDA`), and a league pair sets or clears bit 3
  (`$83:98F7`, `$83:9916`). Each runs before `$80:9A0B` rewrites the byte and no reader runs in
  between [L, D].
- Bits 0 and 4-7 have no reader. The default `0x8B` and the copies' logo bit (1) never reach a
  race.
- `$0751` holds the defaults' `0xC2` for good: no instruction stores it alone. Every 16-bit
  reader masks it (`BIT #`, `AND #$00FF`) except the two-view NEON path `$82:DC2E-DC4D`, which
  keeps the word in A as the scenery number; the loaders `$82:B1DB` and `$82:B183` take A's low
  byte, so the scenery is 8 (12 in VS), as R-0098 found [L].

## `$12D1` and `scenery.14`

Both are reachable and already implemented. `$82:D99B-D9A7` and `$82:DC2E-DC3D` set `$12D1` and
pick scenery 14 when the track is 42 and `$0750` bit 3 is clear: NEON, HUNTER's stunt event in a
one-view race (R-0068, `neon_lighting.cpp`, with R-0098's two-view case). The listing shows them
as inferred only because none of the static map's five raw coverage inputs holds a NEON race;
R-0093's "never-read `scenery.14`" describes its corpus, not the game. Nothing else stores `$12D1`
[L]; its readers are `$80:87A2`, `$80:8816`, `$81:8BA0`, `$82:DC77` and `$83:CECB`.

## `$77:10D0`

The title code sets it (`$80:F609`) and the next title clears it after putting the levels back
(`$80:F564-F580`). `$83:958C` also stores 0 to the word `$77:10CF-10D0` (`$83:958F-9592`) [L, C]. It runs at a
league pair's choice (`$80:BEF9`, before NOW PLAYING) and at a VS race's choice (`$80:BFF9`, after
NOW PLAYING). So after such a race the next title keeps every level at 3, and HUNTER's ending shows
its newspaper pages instead of the CHEAT! page. Native kept the flag (R-0090's "not covered").

## Native changes

- `$83:958C`'s flag clear: `choose_league_pair` and NOW PLAYING's race in VS
  (`league_menu.cpp`, `now_playing.cpp`).
- `$80:9F18-9F21`: leaving the league slots, by a choice or the way back, clears the podium owed
  (`options_menu.cpp`). Before, native showed the podium on the league's next table.
- The saved image now carries `$0742` bits 1 (the logo, or the league keyboard's), 2, 3, 8, 9
  and 10 at the original's sites (`cartridge_ram.cpp`, `rider_menu.cpp`, `now_playing.cpp`,
  `race_result.cpp`, `award.cpp`, `tour_ending.cpp`). An image's gate goes in at power-on,
  survives a soft reset (as cartridge RAM does) and is wiped with the image. This changes the
  save file's bytes; the save format and the pack profile are unchanged.

No later boot reads bits 1, 2, 3, 9 or 10 before rewriting them: every boot clears bit 1
(`$80:A0A5`), every race rewrites `$0750` from bits 2 and 3, and the main menu opens both pads
(`$80:ABC8`) before any gated test runs (the title reads `$72` itself, `$80:F5D3`). Bit 8 is read
at the first result after a boot; R-0057 showed its branch leaves the same OAM buffer.

## Evidence [C]

Warm boots (SAVE-FILES' method: the image written before the first frame) compared with main's
runner (`base-7c19d681`) and the candidate's (`warm_pair.sh`, every picture or every 5th/25th, and
the end image byte for byte):

| Check | Route | Original | Main | Candidate |
| --- | --- | --- | --- | --- |
| `cheat-vs` | `cold-429` image; title code at 275-326; VS race (MIKE/MARTIN) | `$10D0` set at 325, cleared at 1847 | 260/260 pictures; `$10D0`, `$0742-0743` differ | 260/260; `$10D0` and `$0742-0743` equal |
| `cheat-league` | league image, bit 13 set; title code; LEAGUE, slot, table, pair | `$10D0` cleared at 1401 | 281/380 pictures (the podium from 1405); `$10D0` differs | 380/380; equal |
| `league-slot` | as above, without the code | bit 13 cleared at 830 (the slot) | 281/380 (the podium) | 380/380 |

`cheat-vs` also brings the end image's `$0742-0743` (`0x010E`: bits 1, 2, 3 and 8) to the
original's. The remaining differing bytes in these runs are R-0090's known gaps (`$10A7`, `$10AD`,
the never-read checksums and race words).

The image sweep (`bits_sweep.py`: SAVE-FILES' `image_sweep.py` corpus of 236 cold-start
manifests, the original's cartridge RAM against each runner's image after frame 406 and every
500th frame) counts the manifests in which a bit of `$77:0742-0743` differs at some compared frame:

| Bit | 1 | 2 | 3 | 8 | 9 | 10 | 13 | others |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Main | 212 | 19 | 89 | 74 | 54 | 89 | 1 | 0 |
| Candidate | 8 | 0 | 7 | 0 | 0 | 0 | 1 | 0 |

No other byte differs in any manifest where it did not on main, and no runner fails
(`sweep/sweep-1.json`). The residue:

- Seven idle-demo manifests (bits 1 and 3): the split demo's `0x0A` (`$80:94C3`) holds through its
  race. A trace of a warm idle boot (`cold-429`) shows `0x0A` from the race setup (native's demo
  title frame 458) to the return's first frame, when `$80:A09A` clears bit 1, and bit 3 until
  the return's frame 105 (`$80:94CB`). Native's image keeps the menus' value.
- `league/organic-full-tour` (bit 13, main the same): the original clears the bit at `$80:BE84`
  on frame 60757, five frames into the standings' table and with no press (access capture
  `census/acc-organic-full-tour-60740`; `$80:99C2` follows at 60764), while native clears it at
  the podium's entry after the press at 61415.
- The eighth bit-1 manifest is the same league run (the logo's bit through the podium's setup).

## Not covered

- Bit 4's `$80:D503` branch: no code sets the bit, so only an image written outside the game
  reaches it. A power-on image of `$FF` has it set, but the boot's wipe clears it before the main
  menu.
- `$80:F648-F668` (a second logo hook, 1 line a frame to 0x50) and the pad tests `$80:B855`,
  `$80:B87A` and `$80:B89F`: no instruction or byte pair in banks `$80-$83` points at them.
- The timing of the image's bits within a frame: bit 2's clear is at NOW PLAYING's print rather
  than the frame of `$80:BC38`, and bit 8's set at the award's and the ending's sound upload frame.
  Each differs from the original's only within the frames of one screen's entry.
- `$0750`'s and `$0744-0753`'s race words in a saved image: native does not write them, and no
  later boot reads them (R-0090).
- `$77:10CF` (a league pair swapped by `$83:95C4`), read only after the race (`$83:95DB`).
- The image's residue above: the demo race's `0x0A` and the league's bit 13 between the table and
  the podium. Neither is read by a later boot before it is rewritten.
- A later boot from an image whose bit 8 is set skips `$80:9805`'s parking at its first result;
  R-0057's argument that the OAM buffer is the same either way was not re-captured here.
