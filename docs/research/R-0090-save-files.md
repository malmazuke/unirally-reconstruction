# R-0090 - The cartridge RAM saved between runs

Status: research result and native change ([SAVE-FILES](../../tasks/SAVE-FILES.md)), 9 October
2026, on main `3191228`. PAL ROM SHA-256
`a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`, reference core bsnes
`7d5aa1e6` (patched, `tools/locks/emulators.json`). Evidence in main `local/evidence/save-files/`.

Tags: **[L]** listing only, **[C]** confirmed in a capture.

## Domain

- The cartridge RAM is 8 KiB at `$77:0000-1FFF`. The reference core powers on with `$FF` in every
  byte. [C]
- **Corpus.** The 236 cold-start manifests of R-0089 (1,666,993 frames). The original's image and
  native's were compared after frame 406 and every 500th frame from there, and at the last frame
  (`image_sweep.py`). [C]
- **Warm boots.** Power-on with three saved images the original wrote: the ends of
  `front-end-endings/decode/all-gold` (tours open, gold medals), `league/organic-full-tour` and
  `idle-demo-cycles/cold-49220`. Seven input scripts were taken from corpus manifests, with their
  presses from frame 404 moved 3 frames earlier: idle, a one-player race and its continuation, the
  1P setup screens, OPTIONS' track records and high scores, a league's first events, and a VS
  race. Every 5th picture and the final image were compared (`warm_compare.py`,
  `warm_sweep.py`). [C]

## Findings

- **The boot.** After the mirror test, which leaves `$56` at `$77:1FFF` (`$83:8AF7`), `$80:8C4E`
  compares the first 12 bytes with `$83:8000` (`ASJIver3.31`, then `$FF`). On a match it returns
  and the records stay. Otherwise `$83:FB41` clears all 8 KiB and the defaults are written on
  frames 403-405. A kept image skips those three frames, as after a soft reset: the main menu's
  idle count starts on frame 416 instead of 419. [L, C]
- **From power-on.** The original reads cartridge RAM before the check: the title reads the
  cheat flag `$10D0` and puts back the levels (`$80:F55F`), and `$83:91FB` gives the arrow the
  rider's colours while `$10AD` is 1 (a save made after a one-player race). Native reads an
  image's fields when it goes in (`insert_cartridge`), so a session that ends before the check
  saves them unchanged; the check then keeps them or wipes them, and clears `$10AD-10AE`
  (`$83:8B3B`). Native keeps `$10AD` as whether it is 1. [L, C]
- **The cold image.** Native builds it with the audio model's own default writes (R-0075). Its
  image after frame 429 equals the original's byte for byte. [C]
- **What a later boot reads.** Each range where native's image differed from the original's
  somewhere in the corpus was XORed with `$5A` in a saved image, and the warm boot's work RAM
  compared, frame by frame, with the unchanged boot's (`perturb.py`, 3 images, 7 scripts). [C]
  - Read before they are written: the signature, the names and league names, the record times,
    the personal bests, `$0742-0743` (the mode word), `$074C-074F`, `$10A7`, `$10AD`, `$10B1`,
    `$10C8`, `$10D0`, `$1115` and `$1116-1117`.
  - Read, but not by these scripts: nothing found by the review's five further saves and five
    scripts (renames, PICK TOUR, OPTIONS scores and tables, HUNTER's ending, the title code).
  - Never read: the checksums `$02B0`, `$0420`, `$054E`, `$05E6`, `$073C` and `$0E69` (only
    `$016C` has a reader, `$83:89EF`), the copy of work RAM `$0000-019D` that every race start
    makes at `$0E6B` and the race's end restores (`$80:9A27`, `$80:9A3F`), the race's own words
    `$0744-0751` and `$0753-07BB`, `$106B-1073`, `$10A9-10AC`, `$10AF`, `$10B4`, `$10BF-10CF`,
    `$10D1`, `$1118`, `$111C-1123` and `$1FFF`.
- **Native's image.** It is native's fields written over the bytes native keeps no field for, as
  the cold start or the loaded image left them. In 130 of the 236 runs every record field is equal
  at every compared frame. In the others: [C]
  - *Write timing.* The race song counter `$10B1` is counted when native picks the song (19 runs),
    and `$1115-1117` (the demo's view toggle and the tutorial bits) are written at the demo's
    choice or at the race's return (38 runs). The original writes them at the race's setup or the
    moment the hints end. The values agree again once the race returns.
  - *An overlap.* `$0618-061B` hold league slot 0's first event totals and also the one-player
    score history (R-0067), which native does not keep (78 runs). Nothing reads it on a later boot.
  - *Other modes' gaps.* League races leave native's personal bests (`$0AE7`, `$0AED`: 7 runs)
    and tutorial bits unwritten, and one league run (`organic-slot-two`) differs in its names,
    members and pairings. After a two-human restart (`twop-restart`, `vs-restart`) the track
    record and personal best differ (`$0422`, `$0486`, `$0829`, `$08F1`). VS mode 2 writes
    counters at `$0380-0404` that native does not keep (8 runs).
- **Warm boots.** The screens show the same records, names, medals and open tours, and the end
  images agree on every record field but one VS race's personal best (the two-human gap above).
  The read bytes that differ at the end are the ones native keeps no field for (`$0742-0743`,
  `$074C`, `$10A7`, `$10AD`; below). The pictures differ only around the sound program's
  handshakes. [C]
  - The first idle demo's blanking comes 6 frames later in the original; OPTIONS' entry waits
    about 10 frames longer for the sound processor (`$82:80xx`); a league's and a VS race's entry
    are also late. The same class as R-0087's short titles and R-0088's title starts: native takes
    the cold boot's measured lengths. Once each screen settles, its pictures are equal again.
  - The all-gold save's second race starts a frame early in native (R-0077's race-loading limit;
    the save opens a different tour and song).
- **The app.** `--save-file PATH` loads the image at power-on, hands it to the audio model too,
  and writes it through a temporary file and a rename whenever it changes. A hidden run without a
  file writes native's image; with the original's all-gold save the file after 600 frames equals
  the runner's image. [C]

## Not covered

- Bytes native keeps no field for stay as loaded or as the cold start left them: `$0742` bits 2,
  3, 5, 8, 9 and 10, `$074C-074F`, `$10A7`, and `$10AD`'s values other than 1 (2P, VS, league,
  the demo). A later boot reads them. Each is rewritten
  on the paths the warm boots took, but a native save leaves them at the boot's values, not the
  session's.
- League and VS races clear `$10CF-10D0` (`$83:958C`), so the title code's levels stay after them;
  native keeps the flag. Leaving the league slot screen clears `$0742` bit 13 (`$80:9F21`); native
  keeps it. Both read from the listing.
- The sound program's handshake lengths after a warm power-on (above).
- A save taken during a race lacks what the original writes during it until the race returns.
- Twelve corpus manifests write cartridge RAM during their run (`cartridge_ram_writes`, such as
  OPTIONS' populated records); the image sweep ran them without those writes on both sides, so
  they test a cold start's path.

## Queued

The other modes' gaps go to their queued tasks (R-0089): the league's bests, tutorial bits and
slot-two run to LEAGUE-TOURS; the two-human restart's records and VS mode 2's counters to
TWO-HUMAN-STUNTS; `$10D0`, bit 13 and the unmodelled bytes to CARTRIDGE-OPTION-BITS.

## Reproduction

From a checkout of the candidate (with `local/` linked to the main checkout's), after building
lab-release:

```
python3 local/evidence/save-files/image_sweep.py build/lab-release/src/core/front_end_runner local/classic-pal-crawler-tracks-v35.pack OUT 8 500
python3 local/evidence/save-files/perturb.py OUT.json 8
python3 local/evidence/save-files/warm_sweep.py build/lab-release/src/core/front_end_runner local/classic-pal-crawler-tracks-v35.pack 5 OUT.json 8
```
