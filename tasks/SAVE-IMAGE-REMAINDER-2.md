# SAVE-IMAGE-REMAINDER-2 - the saved image's differences R-0117 leaves open

- Status: **queued** 12 November 2026 by [SAVE-IMAGE-REMAINDER](SAVE-IMAGE-REMAINDER.md) (R-0117).
- Milestone: M4. Review tier: **1** (front-end behaviour and cartridge RAM a later boot reads).

[R-0117](../docs/research/R-0117-save-image-remainder.md) closes R-0111's Value list but four items.
Each is a native difference from the original over R-0111's corpus (evidence and tools in main
`local/evidence/save-image-remainder/`; the check is its `check_image.sh`):

1. **A fuzz run's menus** (`c385`, `dragster-ordinary-controls` diff-fuzz 155): after the race the
   fuzz pads drive the menus; the original raises the logo (`$77:0742` bit 1) at 3,527, native at
   3,570 together with the pad gate's bit 10, and from there the screens differ (the original's
   PICK TRACK markers step `$10A7` at 3,678-3,737, native's do not). Find the first differing menu
   frame (pictures, `$008F`/`$009B` state) and the input native takes differently.
2. **The league editor's buffer tail** (`c039`, `menu-input-review/rv-pick`): a one-letter league
   name commits `$00E1-00E2` as `5F 5F` in the original, `58 FB` natively (native starts the buffer
   with `$80:9E0F`'s bytes, R-0097). Bytes after a league name's `FF FF` are read: XOR-perturbing
   them changes `c116`'s picture from frame 6,757 (`perturb-leagues-after-terminator.json`). Find
   what wrote `$00E1-00E2` on this path and what reads the tail.
3. **A saved marker step of 14 to 0x7FFF**: a save made during a gold ending holds the ending's
   counter (`$77:10A7`, up to 0x0020; R-0117 item 2). A later boot's markers count it down
   (`$80:EB28-EB32`, a word) reading `$80:EB61,X` and `$80:EB61 + 7,X` past the 14-byte table into
   code; native starts from 0 (`cartridge_ram.cpp`). Model the word and the bytes it reads (pack
   content), with a warm-boot capture from such an image.
4. **The league markers' edges** (`c029`, `c032`, `g069`): 1 to 5 write frames each where the
   hidden markers stop or run a frame apart (R-0111's timing class); show they are the same write
   or model the edge.

Outcome: each item equal at every write frame of the corpus (`check_image.sh`, `expected.json`
re-recorded) or recorded with why it cannot matter.
