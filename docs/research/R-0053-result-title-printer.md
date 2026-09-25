# R-0053 - The result title's text printer

Status: recovered and implemented on `task/result-title-glyphs` (RESULT-TITLE-GLYPHS),
25 September 2026. PAL ROM `a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`,
audited bsnes core `e59bf88d4fc922c9fe3b5438e65ff3a6909d24e1628f0f87141c8de17699a91b`. Answers
R-0051's presentation finding: which glyphs the original draws for DOWN+UP's `+` and BOO!'s `!`.

The one-run result screen titles the race with the track's name. It is drawn by the game's
general text printer, and the printer's own character table decides every glyph. Until now,
native had a layout fitted to the letters seen on results (TRACK-BREADTH), which covered only
letters, digits and the underscore.

## The stream

The result screen's text stream at `$80:D187` begins `F7 $00CE`, then `FC 02`, then `EE`, then
`FC 05 "COMPLETE"`. It is read by the printer `$80:C3BC`, which takes bytes below `$EE` as
characters and bytes from `$EE` up as control codes, dispatched through `$80:C3DC` by
`$FF - code`.

- `F7 addr` ($80:C628-C672) prints a track name. The track index is the word at `addr` (`$CE`).
  `$80:9B55` copies that name from the name table (pointers at `$83:9F96`, names from `$83:9FFA`)
  to `$DE`, behind a two-byte prefix at `$DC`. The prefix is `FC row` when the stream has one
  after the address, else two `FB` (no-ops). The copy is then printed from `$DC`.
- `EE` after the name code (`$80:C661-C667`) calls `$80:F8B9`. That routine rewrites the copy
  in place: `a`-`z` become `A`-`Z` (plus `$E0`), and `0`-`9` become `$16`-`$1F` (plus `$E6`).
  Every other byte is left as it is.
- `FC row` (`$80:C4B8-C4FD`) centres what follows on that row. It counts the width in tiles up
  to the next `FF` or `FB`, two for a big glyph and one for a small one, and starts at
  `(33 - width) / 2`. `$80:8C41` tells big from small by bit 7 of the character's table
  entry. This agrees with native's `16 - width / 2`.

## The glyphs

A character's entry in the table `$80:C709`, indexed by the byte, selects its glyph
(`$80:C414`).

- **Bit 7 clear: a big glyph**, 2 x 2 tiles, from tile `2 x entry` (`$80:C41E-C440`). The map
  words are tile, tile + 1, and tile + `$50` and + `$51` on the row below, each with `$B0` and
  `$2000`. The EE-converted letters and digits all take this path. For all 36 the table gives
  the layout TRACK-BREADTH fitted:
  - digits at `2 x digit`;
  - `o` shares the zero;
  - `a`-`n` from `$14`;
  - `p`-`z` two tiles lower.
- **Bit 7 set: a small glyph**, one tile wide (`$80:C56C-C5C8`). The glyph is tile
  `(entry & $7F) + $9F`, over the tile `$3C` further on. Entries `$64` and `$65` are special
  cases, which no name byte reaches.

The name table's other bytes, which `EE` leaves unchanged:

| Byte | Entry | Small tiles | Names |
| --- | --- | --- | --- |
| `_` | `$AF` | `$CE`, `$10A`: the small font's space | many (FLAT FUN) |
| `!` | `$81` | `$A0`, `$DC` | BOO! (28) |
| `'` | `$84` | `$A3`, `$DF` | TO AND FRO' (44, a lap race: no title) |
| `+` | `$86` | `$A5`, `$E1` | DOWN+UP (25) |

So `+` and `!` are small punctuation glyphs one tile wide, drawn in the title's palette. The
underscore draws the small space. Before this task native left the map's fill tile there, and
on FLAT FUN's result the pictures are identical either way.

## Evidence

Captures are in `local/evidence/result-title-glyphs/`. The original was captured through the
RUNNER tour (row 2, column 1) at positions 0 (DOWN+UP) and 3 (BOO!), with all tours unlocked.
Each capture follows a controller schedule on which the player finishes first. The schedules
were found natively by a beam search over held moves (`beam.py`), scored by progress along the
opponent's route. They run to the stable result, with frame images every 50 frames.

- `explore` against native: exact over all 3,901 (DOWN+UP) and 3,941 (BOO!) rows. That covers
  the finish and the result load.
  - DOWN+UP: the player finishes at frame 4,183 and the result loads at 4,424, while the
    opponent is still riding. BOO!: the finishes are at 4,213 and 4,226.
- Native's result pictures (`render.py`) against the original's 13 frames per track:
  - The title rows (16-31) differ by 0 pixels on the frames in native's palette phase: 3 per
    track.
  - On all 26 frames the title rows map onto the original's through one consistent colour
    mapping (`shape.py`), so the shapes match everywhere and only the palette phase differs.
- Negative controls:
  - `+` and `!` as the small space: 24 and 28 title pixels differ.
  - As the neighbouring tiles: 44 and 58 differ.
- FLAT FUN (TRACK-BREADTH's review capture, `flatfun.py`): the base and the candidate draw
  identical pictures on all 9 captured frames.

## The composition this adds

The DOWN+UP capture shows a winner's result loading while the opponent still rides. The result
loads 240 updates after the player's finish. The screen shows only the player's time and three
NO TIME rows. Native's composition guard required both finishes. It now accepts a winner's
result with the opponent riding. A loser's result still needs the opponent's finish.

## Not recovered

- The tile under `$64`/`$65` (special cases in `$80:C57C-C5C0`), and the rest of the table, for
  bytes no track name uses.
