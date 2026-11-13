# R-0118 - A computer opponent's colour maths, and its result row

Status: research result and native change ([OPPONENT-POSE-BOUNDS](../../tasks/OPPONENT-POSE-BOUNDS.md)),
13 November 2026, on main `e11cbe11` (claim `272990e6`). PAL ROM SHA-256
`a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`, reference core bsnes `7d5aa1e6`
(patched, Strict). Domain: the race presentation's colour maths (HDMA channel 5) and the one-run
result screen's rows, for a race against a computer opponent (BRONSEN 0x11, SILVIA 0x12, GOLDWYN
0x13, ANTI-UNI 0x14). Evidence in main `local/evidence/opponent-pose-bounds/`.

Tags: **[L]** read from the static map's listing (main `local/evidence/static-code-map/static-map/`)
or the ROM's bytes; **[C]** the original's own memory or trace (an existing capture, or the coverage
census's trace of the original, R-0109); **[R]** an accepted record.

## Question and answer

A libc++-hardened debug runner aborts on corpus route `c175` (`hunter-effects`, opponent 0x14): the
race's content takes the opponent's four bytes of `$82:D4DC` at `4 x 0x14`, past the 64-byte
table. What does the original read there? **Answer: nothing.** The table has an entry for each of
the 16 rider characters only, and the original indexes it with the second rider only in a race with
two views, where both riders are rider characters; a race against a computer opponent gives the
lower lines the player's entry again. Native now does the same, and never indexes the table with a
computer opponent. A hardened run of the whole front-end corpus found one more read of the same
kind, the one-run result's row for a computer opponent, which the original skips; native now skips
it too. No other out-of-bounds access remains on the corpus.

## Findings

### 1. The colour maths table ends at the rider characters [L, C]

- `$82:D4DC-D51B` is 64 bytes, four per rider character (COLDATA's three writes and CGADSUB), and
  `$82:D51C` is the start of `sub_82D51C`'s code (`E2 30 9C 0C 42`, `SEP #$30; STZ $420C`) [L]. Four
  times 0x14 is `$82:D52C`, inside that routine's instructions.
- `$82:D57F-D5A2` takes the upper lines' entry from `$77:0748` (the player). `$82:D5BE-D5CC` takes the
  lower lines' entry from `$77:0749` (the second rider) only when `$77:0750` bit 3 (two views) is set
  (`BIT #$08; BEQ $D5CC`), and from `$77:0748` again otherwise [L]. `$82:D5C6` is never executed in
  the static map's coverage, which has no two-view race [L].
- A race with two views is a race between two humans (2P, VS, a league's human pair, the split idle
  demo), so `$0749` is a rider character whenever the original reads its entry [R: R-0071, R-0105,
  `league_menu.cpp` `$83:98F1-98F7`].
- **The original's HUNTER race against ANTI-UNI** (`hunter-effects/review-withheld/w-rev-buttons`,
  read by `colour_math_original.py` into `colour-math-original.txt`; the only retained capture of
  a race against a computer opponent with work and cartridge RAM per frame): at frames 1395, 1400,
  1500 and 2500, `$77:0748` = 0 (MIKE), `$0749` = 0x14, `$0750` = 0, and the channel-5 table
  `$7E:2084-2095` is `01 04 2F 01 04 40 6D 04 80` twice: both halves MIKE's entry `2F 40 80 04`
  [C].

### 2. Native read past the table on every race against a computer opponent [C]

- `classic_race_presentation_content` took `subspan(4 x opponent, 4)` of the 64-byte pack entry. For
  0x11-0x14 the offset is 68-80: past the span. A release build read the bytes after the entry in
  the pack and used them only for the lower view's ink and CGADSUB, which native draws only in a
  split race, so no picture showed them.
- Hardened main (`sweep-main-nopics-p*.json`): 1,238 of the corpus's 1,924 cases abort on this
  subspan, the first race against a computer opponent in each; `c175` is one.

### 3. The one-run result's row for a computer opponent [L, C]

- `$80:CF43-CF4E` gives the opponent's row the time 0xEA62 (no row) when `$017F` is 16 or more, so it
  sorts after the three records. The row loop (`$80:CF68-CF8C`) runs the first five sorted rows'
  handlers; the opponent's (`$80:D007`) stores `$017F` in `$00CA` and returns at once
  (`CMP #$0010; BCS $D071`) for a computer opponent: no best-time update, no OAM markers, no colours
  (`$80:D163`), and `$60` stays [L].
- The coverage census's trace of the original (`docs/map/coverage-census.map.json`, R-0109): the
  handler `$80:D007` is entered 327 times; the branch to `$80:D071` is taken 299 times and the human
  path returns from `$83:9E47` at `$80:D01A` 28 times [C].
- Native printed the row: its best-time update indexed `best[50 x second_rider + track]`, past the
  800 words with `second_rider` 16 (a league against the computer, `league_menu.cpp:629`; the one
  case examined had BRONSEN against rider 11), and on every one-run result against a computer
  opponent it placed the new-best markers 97 and 99 on the fifth row's line, set entries 101 and 103's
  attributes and loaded the fifth row's colours (0xC0-0xCF). The hardened sweep caught the index past
  the table on 16 cases (in the three eighths of the corpus swept before this fix); in 1P,
  `second_rider` is 0 and the index stays in bounds, but the row is still not the original's. The
  best time itself was never written: no row's 0xEA62 is below a stored time.
- **The original's OAM at the result** (`oam-cont-win`, `oam-cont-loss`: front-end-1p-continuation's
  manifests, MIKE against BRONSEN on CRAWLER's first track, won and lost: a new capture of
  `$0A00-$162F` every frame (0xC20-byte rows), of which `result_oam.py` compares the OAM buffer
  `$0A00-$0C1F`): main's OAM differs on 241 of 243 and 760 of 762 result frames
  (`$0B85`, `$0B8D`: entries 97 and 99's y, 184 against the original's 1; `$0B97`, `$0B9F`: 101 and
  103's attributes 25 against 0); the candidate's equals the original's on every result frame [C].
  CGRAM is not in the capture; the colours follow the listing [L].
- **Main against the candidate** (`replay-main-vs-candidate.json`, the gates' replay of the whole
  corpus, 23.9 million frames, 474,666 pictures): 408 of 1,924 cases differ, all in stdout only, no
  picture, cartridge RAM, cue or timeline; the fields are the OAM buffer (entries 97 and 99's y, 101
  and 103's attributes) and CGRAM bytes 0x180-0x19F, colours 0xC0-0xCF (`stdout-fields.json`), from
  the result screen until a later screen rewrites them.

### 4. The rest of the corpus [C]

- After both fixes the hardened runner (`sweep-final-pics.json`: all 1,924 cases, each with its
  pictures drawn on the frames the checks compare) exits 0 on every case except 11
  (`expected-refusals.json`), which exit 1 with the runner's own refusals (`--record-write:
  unsupported record offset`, 3; `split race state requires ...`, 8), as main's hardened runner does
  on the same 11 with the same messages.
- Other indexing by a pairing's opponent in `src/core` (read for this task): `character_palette`
  (asset 6 + character, a bounded pack lookup: 0x11-0x14 are assets 23-26, the computer opponents'
  palettes), `asset()` lookups in NOW PLAYING and the stunt result (bounded, `front_end.cpp`),
  `lap_result.cpp` (`bronsen_crawler`, bounded), `update_records` (`statistics` guarded by
  `rider_opponent`), `race_state_io.cpp` (refuses opponents past 15 where it indexes), and
  `race_setup.cpp` (an AI level, not an index). None indexes a table by a computer opponent.

## Limits

- The two-view selection's other branch (`$82:D5C6`, `$0749`'s entry) has no capture of the
  original in this task; native's split races keep the entry they had (`two-view` tests and the
  split checks are unchanged).
- The result-row skip is shown on two one-player results against BRONSEN (OAM); a league's result
  against the computer, where native indexed past the table, has no capture of the original here (the
  listing and the census decide it); CGRAM is from the listing.
- The sweep covers the corpus's front_end_runner cases only; the race and presentation runners, the
  app and the unit tests were not run hardened.
