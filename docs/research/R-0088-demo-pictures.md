# R-0088 - The idle demos' pictures

Status: research result and native change ([DEMO-PICTURES](../../tasks/DEMO-PICTURES.md)),
9 October 2026, on main `6be4093`. PAL ROM SHA-256
`a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`, reference core library
`e59bf88d`. Captures in main `local/evidence/idle-demo-cycles/`.

Tags: **[L]** listing only, **[C]** confirmed in a capture.

## Domain

R-0087's cold lap: `cold-102000` (Start on frames 300-305, pads released), its 1,006 retained
pictures, with the capture's short titles replayed (`--short-demo-title 13 16 18 21 33`). Before
this record 906 were equal; now 963.

## Findings

- **GO.** `$83:E728` picks between the two GO tables on the contact phase `$0300`. Native derived
  it from the updates since the race's initialization, which equals `$0300` from the menus (0 at
  the boundary) but not in an idle demo, whose race starts with whatever phase the demo leaves.
  `go-c4` (every second frame of cycle 4's GO) differed by 19,155 pixels on every frame of updates
  214-274; with `$0300` read from the race state it is equal. [L, C]
- **The menu's palette cycle after a timer return.** The hook (`$80:FA60`: `DEC $C8`, below 0 set
  6 and step `$C9`) runs twice on its first frame: `$00C8/$00C9` go 0/0, then 5/3. An NMI is
  pending when the return enables them. Exceptions: the first demo's return and every cycle whose
  title was short step once, so the sound processor's timing decides both. Native was a picture
  behind through every idle period; now all 16,432 idle-menu frames' `$00C8/$00C9` equal. [L, C]
- **The title's wave.** Every title after the first holds its blank frame 133 once while the
  sound program loads; the wave starts at 134 a picture later. The short titles of R-0087 are
  exactly those without the hold. Native was a picture ahead (about 8,500 pixels at title frames
  140-200); it now holds the frame, and the fade and the choice keep the first title's script
  frames. [C]

## Not covered

- **Two title starts** (cycles 10 and 17): the original keeps the menu on screen six frames
  longer before blanking (`tstart-c10`: native blank from 28,399, the original from 28,405), and
  still writes the track on the same frame. The title's first frames wait on the sound processor;
  like the short titles this is its state, which native does not emulate.
- **Small race sprite differences**: 41 pictures of 1-107 pixels in demo races, as on main (frame
  1,800: 42 pixels in the first split demo). Not investigated.
- The product cannot know a short title; it takes every title as long, so after a short title in
  a real run its pictures are a frame off until the next title.
