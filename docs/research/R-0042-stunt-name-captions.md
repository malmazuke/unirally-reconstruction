# R-0042 - the on-screen captions: stunt names, hints and results

Status: **verified finding; native implementation in
[CLASSIC-STUNT-NAMES](../../tasks/CLASSIC-STUNT-NAMES.md)** (in progress).

ROM SHA-256 `a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`;
audited bsnes core `e59bf88d4fc922c9fe3b5438e65ff3a6909d24e1628f0f87141c8de17699a91b`.

## Question

M4-16 recorded that the original's "on-screen stunt names (raised by the user in
live play; the reward events behind them are recovered, their text display is
not)" are absent from native play, and
[ZOOM-ZOO-WINDOW-EFFECTS](../../tasks/ZOOM-ZOO-WINDOW-EFFECTS.md) established
that they are not channel-6 windows. When does the original show a caption,
which text, where, and what does it draw it with?

## The mechanism

All addresses are LoROM: file offset = `(bank & $7F) * $8000 + (address - $8000)`.

**The reward queue is the trigger.** The engine already runs the player's reward
queue (entries `$0CC1`, read cursor `$0CE7`, write cursor `$0CE9`, cooldown
`$0CA5`; consumer `$81:C0CE-C18A`). Every update on which the read cursor
advances, the consumed event id becomes a caption. Nothing else starts one.

**One ASCII table drives every caption.** `$81:BFE9` reads sixteen bytes at
`$17:C9F4 + 16 * event`, plain lowercase ASCII padded with spaces. Measured: on
frame 1599 of the accepted DRAGSTER replay manifest, the update that consumed
event 44, it reads exactly `$17:CCB4-$17:CCC3`, which is that base plus 44
entries. The table holds the stunt names at 1-22 (`roll`, `double roll`,
`treble roll`, `roll city`, the flip, twist and z flip families, `rollout`,
`wipeout`, `last lap`, `head bounce`, `tabletop`, `wrong way`), the cheat and
mode messages at 23-36, `winner`/`draw`/`loser` at 37-39, the hint sentences at
40-71, and the voice lines the consumer diverts above 71.

**A sixteen-byte tile buffer.** `$81:C019` converts the entry and stores one
tile index per character at `$0EA7-$0EB6`, `$81:C034` sets the queue cooldown to
120, and `$81:C057` raises the redraw flag `$0EE7`.

**ASCII to tile.** With `n = ascii - 'a'`, the top tile is `$0B + n` for `n < 5`,
`$20 + n - 5` for `5 <= n < 21`, and `$40 + n - 21` for `n >= 21`; a space is
`$80`. The three runs are 16 apart because each glyph is eight pixels wide and
sixteen tall: the font sheet holds every glyph's top half in one row of sixteen
tiles and its bottom half in the next, so the bottom tile is always the top tile
plus `$10`.

**The caption's place in the composition.** It is drawn over the track and
under both the rider objects and the channel-6 window members. A member covers
it as it covers everything else (R-0040's "nothing else"). A rider does not: on
the 35 pixels where a sprite lies under a glyph's ink on frame 2100 of the M4-16
primary, and on frames 2120, 2340 and 2600, the original shows the sprite's own
colour with `red = min(31, sprite_red + 13)` and green and blue untouched. That
is colour-math arithmetic, so the caption contributes red to the sprite rather
than being hidden by it or painted over it. Which PPU configuration produces the
add is **not recovered**, and the added 13 is not half of the ink's own 5-bit 28,
so the colour entering the arithmetic is not quite the one the glyphs are drawn
with. The rule is measured, not derived.

**Two tilemap rows.** On the following update `$81:F322` and `$81:F33C` each read
all sixteen buffer bytes and write a row: `$81:F31D` points `$2116` at words
6472-6487 and `$81:F327` writes `$3800 | tile`; `$81:F337` points at 6504-6519,
32 words further on, and `$81:F345` writes `$3800 | (tile + $10)`. `$81:F352`
then clears `$0EE7`. The attribute `$3800` is palette 6 with priority set. With
the tilemap based at word `$1800`, those are rows 10 and 11, columns 8 to 23:
the caption occupies sixteen characters centred across the screen at y 80-95,
which is where the pictures show it.

**Blanking has two halves.** Entry 47 and the other gaps in the table are sixteen
spaces, and a hint sentence is published as consecutive ids - 44 `more stunts`,
45 `give you`, 46 `bigger boosts`, 47 blank - so a sentence ends by publishing a
blank entry. That is not the whole rule: `$81:BEA8-BEF1` also blanks the display
when the queue runs dry. Reaching a read cursor equal to the write cursor, the
consumer takes a ten-update cooldown and raises the engine's `empty_display`,
and the caption area stays blank until the next message. Nothing counts the
caption down in updates; both halves are the queue's own doing.

An earlier version of this record claimed the blank entry was the only rule.
DRAGSTER's four measured captions could not tell the difference; ZOOM ZOO's
frames 3208, 4840 and 6484 did, where the read cursor still points at the last
consumed event (14, then 15) and the original shows nothing.

## Measurements

| What | How | Result |
| --- | --- | --- |
| the trigger | `queue_probe.py` over the DRAGSTER primary capture | 80 consumptions, events 14, 37, 44, 45, 46, 47 |
| the captions on screen | frame images at the consumption updates | 1599 event 44 `MORE STUNTS`, 1633 event 45 `GIVE YOU`, 1647 event 46 `BIGGER BOOSTS`, 1681 event 14 `WIPEOUT` |
| the text is not in WRAM | `text_probe.py` | 64 scattered offsets change across all four caption starts, no run of four |
| the table | `string_search.py`, then the ROM read sites of an access record | `BIGGER BOOSTS` matches two sites in 2 MB, both ASCII; `$81:BFE9` reads 16 bytes at `$17:CCB4` on the consumption update |
| the buffer and the encoding | the capture's own WRAM at `$0EA7` | `  more stunts   ` becomes `80 80 27 29 2C 0F 80 2D 2E 2F 28 2E 2D 80 80 80`, and the other three captions agree letter for letter |
| the drawing | the access record's `$2116`/`$2118` writes | two rows 32 words apart, the second row's tiles exactly `$10` above the first |
| the blanking | native against the M4-16 original's kept frames | at 3208, 4840 and 6484 the cursor points at the last event and the original is blank; honouring `empty_display` makes every frame agree |
| the composition | a sweep of all 274 kept frames of the M4-16 primary, and the review's withheld DRAGSTER frames | every band exact once the caption is drawn below the window members and below the riders with the measured red add; worst mismatch 0 |
| both tracks | `queue_probe.py` over the M4-16 primary original | ZOOM ZOO consumes 93 events - 14, 15, 37 and 44-59 - through the same consumer and table, reaching entries DRAGSTER's race never does |

## Domain and limits

- Measured on both tracks: DRAGSTER through the accepted replay manifest and the
  accepted `dragster-ordinary-primary` case, ZOOM ZOO through the M4-16 primary
  original's own kept frames.
- **Corrected.** An earlier version of this record said the original's start
  ring composes above the caption, and explained the 116 pixels that ZOOM ZOO
  1649 and DRAGSTER 1601 then missed as that declared omission. That was wrong.
  The independent review measured the window members instead and found native
  painting 9,144 member pixels where the original paints 9,260: the difference
  is exactly those 116, and the cause was the caption being composed above the
  channel-6 members rather than below them. There is no start-ring rule here.
- The letters measured against the original's own pictures are those of the six
  captions compared (b, e, g, i, m, n, o, p, r, s, t, u, v, w, y, the space and
  `last lap`'s a and l). The rest follow from the three-run arithmetic, and
  rendering the pack's font with it spells every table entry legibly, but no
  picture has exercised `j`, `k`, `q`, `x` or `z`.
- The font sheet is `presentation.classic.font.v1`, already in the pack: 128
  tiles, 2bpp, using only pixel values 0 and 3, so it is a one-bit font.
- Which BG layer carries the caption is not established. The tilemap word
  addresses and the `$3800` attribute are measured; the tilemap base of `$1800`
  is inferred from them, and the ink is the race CGRAM colour measured from the
  original's frames rather than derived from that attribute.
- The voice lines above entry 71 take the consumer's other path. Whether they
  reach this display, and the audio that goes with them, are out of scope.
