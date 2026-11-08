# R-0111 - The saved image at every frame the original writes it

Status: research result and native change ([SAVE-IMAGE-EXACT](../../tasks/SAVE-IMAGE-EXACT.md)),
8 November 2026, on main `901018a5`. PAL ROM SHA-256
`a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`, reference core bsnes
`7d5aa1e6` (patched, Strict). Evidence in main `local/evidence/save-image-exact/`. It works through
R-0109's "saved image's last bytes" (R-0090, R-0095, R-0100, R-0105).

Tags: **[L]** read from the static map's listing (`cartridge-option-bits/listing`); **[C]** this
task's captures (the original's cartridge RAM every frame) against native's image every frame;
**[R]** an accepted record.

## Question

Does native's cartridge RAM image (`cartridge_image`, what the runner's `--records` and the app's
save file write) equal the original's byte for byte at every frame the original writes cartridge
RAM, over R-0109's corpus; where not, which bytes, and can they matter?

**Answer: partly.** The image is not byte-equal at every write frame. Outside the bytes no later
boot reads (`residual.txt`, argued below), 709 of 916 routes are equal at every write frame from
frame 406; on the other 207, 14,380 write frames still differ: mostly timing shifts of the same
write (argued below), plus the Value items, which have no cannot-matter argument yet and go to
[SAVE-IMAGE-REMAINDER](../../tasks/SAVE-IMAGE-REMAINDER.md) (personal bests on four
`hunter-effects` routes from a race difference, the endings' own `$10A7`, and the rest).

## Method

- **The corpus** (`routes.py`, `routes.json`): R-0109's warm and cold routes, 919 runs, 5,048,046
  frames: R-0089's 236 cold starts, COVERAGE-CENSUS' 651 built manifests (110 menu manifests, 5
  warm boots, 2 `warm_start` cold starts, 411 track_reference runs, 123 track_reference runs with a
  preload) and the 32 it reused; its 3 reused track_reference runs equal R-0089 runs. An image or
  preload (writes after frame 0 in the census) is written before frame 0 on both sides (the
  warm-boot method, R-0090, R-0100: the runner's `--cartridge-in`); later writes as
  `--record-write`. 3 routes poke bytes the runner has no `--record-write` for (`$1000`, `$10C0-10C8`,
  `$10B1`, `$1115`: `race-listing-branches` t25-seed43 and split-demo-26, R-0089's
  `rolling-contact/w-rev-buttons`) and are not compared.
- **The original** (`original_trace.py`, 26 min, 10 processes): the ROM in the lab core with the
  route's pads; after every frame the bytes of `$77:0000-1FFF` that changed are logged
  (`original/<id>.txt.zst`, 12 MB). **Native**: the runner's new `--cartridge-trace` logs its image
  the same way after every frame (`native_trace.py`; the demo race's frames included).
- **The comparison** (`compare.py`): both images rebuilt frame by frame. A *write frame* is a frame
  on which the original's image changes: 367,329, 363,007 of them from frame 406 (after a cold
  start's wipe). At each, every differing byte is listed. A write of an equal value is not seen;
  since the images can only start to differ on a frame where one of them changes, the logs also give
  every frame (`any_frame`), which is how the spans below are measured. `report.py` groups bytes by
  their meaning, `classify.py` classes each difference as a *shift* (the same value written by the
  other side within 120 frames) or a *value*.
- **Main**: `base-trace/front_end_runner`, commit `e41810f9` (main `901018a5` plus the trace
  option, no other change). **Candidate**: `03717daa`.

## Baseline (main)

At write frames from frame 406: 355,423 of 363,007 differ, on 900 of 916 routes; 4,099 bytes differ
somewhere [C]. Before 406 every cold route differs at frames 24 and 228 (native's image of an
empty power-on held its fields' zeros where the original has `$FF`) and on the wipe's frames 403-404.

## Findings and native changes

Each item: the original's writes [L, C], and native now.

1. **Power-on.** The reference core's cartridge RAM is `$FF`; the boot writes `$0742` bit 1 on frame
   24 (`$80:A0A5`) and the title's flag `$10D0` on 228 (`$80:F55F`) before the check. Native read an
   absent save as zero fields. Now `power_on_cartridge()` is inserted like a save (runner and app),
   a flag native keeps as a bool keeps its loaded byte while it reads the same, and the names'
   checksums keep the loaded bytes while the names are as loaded (`store_records_checksum`).
2. **`$10AD`, every mode.** Stores: `$80:AD18-AD1A` and `$80:BC9B` 0 (bytes), `$80:BBEE` 1, `$80:BD27-BD2A`
   2, `$80:BFA2-BFA5` 3, `$80:BE45-BE48` 4 (words), `$80:9496-9498` 5 (byte); the boot's `$83:8B3B`
   clears the word. Native kept whether it was 1; `store_saved_mode` now stores the byte at each
   site, `one_player` with it. The idle demo's return clears it on its frame 108, before the menu's
   fade, ten frames before native's main menu starts.
3. **`$10A7`, the markers' step,** is the word `$80:E872` and `$80:EB32` store: native writes it with
   each turn (`store_marker_step`). **`$10D1-10D2`**, PICK TRACK's medal and its latch (`$80:E6D5`,
   `$80:EACF`; `$80:E853`, `$80:EAFE`, `$80:BABE`), likewise (`store_tour_medal`,
   `store_medal_latched`). **`$10FD`** is a pending reveal per rider (`$10FD + $CA`, `$83:9F8F`), all
   16 cleared at the boot (`$83:8B41`); native keeps the current rider's at its place.
4. **`$83:9987`'s race words** from the track `$00CE`: `$074A` (word), `$0744` its kind (`$83:A254`,
   the pack's `audio.track-types`), `$074C` the place, `$074B` the race mode. Called at NOW PLAYING's
   entry (`$80:B18D`), the track's choice (`$80:E9F8`), the demo's (`$80:94AD`), the league pair's
   (`$83:98AB`), the league's next track (`$80:B011`), NEXT TRACK (`$80:AEB0`) and a closed tour on
   PICK TOUR (`$80:E74B-E74F`): native's `store_track_words` at each.
5. **The race's setup** `$80:99A4-9A23`, on the frame the race is chosen (the demo's setup frame
   458): `$074D-074E` = `$FFFF` (`$80:99A7`), `$106B` the option word, both riders' seventeen words
   from `$0755` and `$07BF` cleared (`$80:99CA`), `$0753` the qualifying score by `$017D`'s best medal,
   `$0748-0749` = `$017D`/`$017F` (the 1P computer opponent's number, `$80:B346`), `$0750` = the option
   byte's VS and two-view bits (2 more in the demo), `$074A` = `$00CE` (`$80:9A21`).
   `store_race_setup`. The league's podium takes `$80:999F` (all but `$074D`) on its frame 7, after
   `$80:BE8A-BEB1` put the slot's first three places in `$074D-074F` (`store_race_words`).
6. **The idle demo** stores 0x0A in `$0742`'s low byte for its race (`$80:94BC-94C3`, every demo, not
   only split ones as R-0105 had it); the return clears bit 1 on its first frame and restores the
   byte on its frame 105 (`demo_option_byte`). `$83:C8E0` runs a frame after the setup: `$1115`
   flipped, `$0750` bit 3 set for a split race and cleared otherwise, `$0748` the rider and a split
   race's `$0749` its opponent (`$83:C912-C994`), before the name test (`$83:C99E`), so the demo
   that shows the "credits" picture flips it too; native flipped `$1115` at the choice, 8 frames
   sooner.
7. **The picks' `$0750` copies** (R-0105's dead stores, still writes): 1P `$80:BBD2`, 2P's second
   `$80:BD16`, VS's second `$80:BF9C`, VS's challenger `$80:C0C9-C0D3`; a league pair sets or clears
   bit 3 (`$83:98F1`, `$83:9910`).
8. **During the race.** The song counter `$10B1` is stored on the race's sound load (`$83:CA08`),
   `race_sound_load_offset` frames after the choice; native counted at the choice and now names the
   song there and stores it on the load (`update_race_loading`, on the runner's and the app's loading
   frames; a race run without them, the app without audio, stores it on its first update). The
   tutorial bits are set when the hints end (`$83:CE2C`, `$83:CEB1-CEC5`), not at the return
   (`update_race_cartridge`, after each race update, with R-0098's increments).
9. **`$80:F88D` outside a league** (R-0095, R-0067): every mode's lap result (second frame), one-run
   result (third) and stunt tally stores the totals at `$0618 + 16 x $CC + 2 x` the word at
   `$0678 + $CC`, whose high byte is the next slot's cursor (`store_result_totals`; a nonzero one
   sends them 512 bytes on, into the personal bests or past the records: `store_record_word`). The
   stunt result against the computer stores the score alone a frame after its qualifying line.
10. **`$80:C786`** stores 8 x `$0748` and 8 x `$0749` in `$073E` and `$0740` (`$80:C789`).
11. **The other checksums** (`$83:90F4`, 23 call sites) keep the boot's, as on main. Tried and
    reverted: following their records at once (all but the bests') brought them to the original's
    on far fewer routes (`$02B0` on 23 routes instead of 330) but put 9,321 more write frames apart
    (192,449 differing against 183,128; 74 routes equal from frame 406 against 90), since the
    original sums them only at the end of each records update and between those frames the image
    is otherwise equal. No reader, so the measure decides (`03717daa`).

## Results

`report-base-vs-candidate.txt`: routes (of 916) on which a group differs at some write frame from
frame 406 [C].

| Group | Main | Candidate | Group | Main | Candidate |
| --- | ---: | ---: | --- | ---: | ---: |
| `$0618` event totals | 406 | 2 | `$106B` saved option word | 777 | 0 |
| `$073E-0741` | 328 | 12 | `$10A7` marker step | 884 | 12 |
| `$0742-0743` | 120 | 108 | `$10AD` mode | 160 | 6 |
| `$0744-074B` | 789 | 20 | `$10B1` song counter | 764 | 1 |
| `$074C` place | 789 | 7 | `$10D1-10D2` | 142 | 0 |
| `$074D-074F` | 780 | 6 | `$10FD` reveals | 120 | 0 |
| `$0750` race mode | 795 | 7 | `$1115` demo split | 14 | 5 |
| | | | `$1116-1117` tutorial bits | 374 | 4 |

Unchanged: the race words `$0752-0828` and the work RAM copy `$0E6B-106A` (777 routes), the
checksums `$02B0`, `$0420`, `$054E`, `$05E6`, `$073C`, `$0E69` (330, 66, 318, 318, 478, 435),
`$1118-1123` (446), `$10B2-10C7` (80), `$10C9-10CF` (48), `$10AF` (40), `$106D-1072` (37), and the
records with timing residues below.

| Write frames from 406 | Main | Candidate |
| --- | ---: | ---: |
| differing | 355,423 | 181,547 |
| differing outside the bytes no later boot reads (`residual.txt`) | 353,073 | 14,380 |
| routes equal at every write frame from 406 | 16 | 90 |
| routes equal outside those bytes | 16 | 709 |

No frame is equal on every route: the cold start's wipe frames 403-404 and the race words differ.

**Behaviour.** main's runner against the candidate's on all 919 routes, on stdout (each menu
frame's state), stderr, the sound cues, the race, one-player and look timelines and every frame's
picture and VRAM digest; the cartridge RAM digests left out (`behaviour_same.py`, 61 min under the
heavy-run lock). At `bccd3914`: 907 routes equal on every output; 11 routes stop with exit 1 on
both sides alike (8 race-branch manifests whose race timeline cannot hold their split state, and
the 3 routes above whose pokes the runner refuses), their outputs equal up to there; 1 route
differed, `credits-name/demo-credits`: the demo that shows the "credits" picture did not flip
`$1115`, so the next demo chose another rider (`$83:C912-C994` runs before the name test
`$83:C99E`). Fixed in `d6604a38`, which touches only that path; the 26 idle-demo and credits
routes rerun equal on every output (`behaviour-final-demo-credits.json`), and the image sweep is
unchanged on every other route. `03717daa` changes only the image's checksums; on it the gate
check's 15 routes (`behaviour_check.sh`) are equal on every output. So no picture, row or cue
changes, as meant.

**The app** writes its save through the same image (`FrontEndSession::cartridge`) and now drives the
race's loading and update hooks: on two-races (two one-player DRAGSTER races, native audio) its save
after 11 points (the track's choice, the setup, before and after the song counter's store, the race,
its return and result, the second setup and loading, the end) equals the runner's `--records` at the
same frame; on R-0089's `dragster-ordinary-controls/explore/diff-fuzz-135` (one one-player DRAGSTER
race whose hints end at 1599) its save equals the runner's at 1598, 1599 (the tutorial bit set by
the race hook), 2400 and 3199 (`app_save.sh`, review 1).

## The residual, and why

**Cannot matter** (no later boot reads them before they are rewritten) [L, R, C]. R-0090's
perturbation found no read for `$0744-0751`, `$0753-07BB`, `$10A9-10AC`, `$10AF`, `$10B4`,
`$10BF-10CF`, `$1118` and the checksums [R]; it does not cover `$0752`, `$07BC-0828`, `$1119`,
`$10B2-10B3`, `$10B5-10BE` or part of `$10C0-10C7`. Review round 1's perturbation does
(`review-1/rv_perturb.py`, `perturb.json`): on 9 warm routes (`c115`, `c116` leagues through their
awards, `c083`, `c085`, `c077` records, `c175`, `c263`, `c268`, `c290` races) the original booted with
every residual byte XOR `0xA5`, and again set to 0, gives the unperturbed boot's picture, work RAM
and cartridge RAM outside the residual at every frame; positive controls see a read (`$1116-1117`
XOR changes `c263`'s picture from 1601 and `c175`'s from 1664, `$10A7` XOR changes `c115`'s and
`c085`'s work RAM from 640 and 660). The league awards run `$80:AF88`'s read of `$10B6,X`
(`c115`, `c116`) and the statistics stay equal, so `$10B6-10C6` is rewritten
(`$80:8996`, `$83:967C`, `$83:969F`) before it is read there. `$10B2` (readers `$83:8F9A`,
`$83:8FD8`) still has no capture behind it: a hypothesis from the listing. The bytes:
- the race's own words `$0752-0828`: lap and time slots, scores, written by the race; each setup
  clears `$0755-0776` and `$07BF-07E0` and rewrites `$0753` (above), the result reads them in the
  same session;
- the work RAM copy `$0E6B-106A` of `$0000-019D` every race start makes (`$83:9894`), which only the
  race's end reads back (`$80:9A3F`); native keeps no work RAM layout;
- the checksums without a reader `$02B0`, `$0420`, `$054E`, `$05E6`, `$073C`, `$0E69` (only `$016C` is
  read, `$83:89EF`), which keep the boot's (item 11), and `$02BE-02BF`, the league members' sum
  (`$83:9157-916A`, `ADC $02B2,X` over six words), which native writes as its "active members" field
  and which has no reader in banks `$80-$83` (review 1; it alone made `menu-input/scores-carry`
  differ on 1,214 write frames);
- `$073E-0741` (8 x `$0748`, 8 x `$0749`): read only at `$80:C984-CAF5`, inside the records update
  `$80:C786` after its own stores `$80:C793` and `$80:C7A1`;
- `$10F9-10FC` (the VS percentage's words): read only in `$83:9D80`, whose four callers
  (`$80:D84F`, `$80:F986`, `$83:91DD`, `$83:9878`) store both words first (`$80:D841/D84B`,
  `$80:F978/F982`, `$83:91D4/91D9`, `$83:986A/9872`);
- work bytes: `$106D-1072` (league, `$83:908C-90E9`), `$10AF` (`$80:8A1E`), `$10B2-10C7` (`$83:91E1`,
  `$83:96A9`, `$83:9711`), `$10C9-10CF` (the endings' counters from `$83:AFD6`; `$10CF` the league
  pair's swap flag, read only after its race, R-0105), `$1118` (the race's flags, `$83:FAAE-FAD4`).

**Timing** (the same write a few frames apart; `classify.py`'s *shift*, 120 frames) [C]:
- the wipe's three frames: the original clears `$1FFF` down to `$08C0` on frame 403, the rest and
  part of the defaults on 404, the signature on 405 (`$83:FB41`); native writes the whole cold image
  on 403. A save from frame 403 or 404 is unsigned in the original, so a later boot wipes it, and
  complete in native, so a later boot keeps it and reaches the menu 3 frames sooner (R-0090) with
  the same records. The WIPE RAM menu's wipe (2 frames, 5 routes) likewise;
- `$0742-0743` (108 routes): R-0105's bit timing (bit 8 at the result, the pad gate's bits 9 and 10
  around picks, bit 13 between the table and the podium);
- the league's multi-frame writes: pairings, cursors and tracks (75 routes, 3 frames each), the
  levels (41, at most 5 frames), names and records in WIPE RAM and RENAME (10 and 6), the marker step
  on 12 league routes where native's hidden markers stop or run a few frames apart (11,879
  byte-frames);
- the routes whose menus native runs a frame or more apart from the original (the late idle demo
  cycles' short titles, R-0087; league table prints, R-0095): `$10AD`, `$1115`, `$074C`, `$0750`,
  `$10C8`, `$10D0` and the race track words on 1 to 12 routes;
- the demo that shows the "credits" picture runs `$83:C8E0`'s stores (`$1115`, `$0748`, `$0750`) on
  the setup frame, a frame before the original, since native tests the name there (R-0094).

**Value** (still different, bounded) [C]:
- name bytes after a name's terminator (`rv-warn`, `w5`, `warnings`: 3 routes): the keyboard's
  leftovers, never printed; each side's names checksum agrees with its own bytes;
- personal bests on 4 `hunter-effects` routes (`c175`, `c177`, `c210`, `c220`): native's race adds
  to `$085F` (or `$085B`) at frames 1973 and 1991 through a computer opponent's voice class read
  past rider 1's tallies (`race_update.cpp:852`, R-0098); the original writes nothing; main the
  same. A race difference that lands in a saved record (review 1);
- the endings' own `$10A7` stores (`$83:B649` on, 7 routes), which a later boot reads;
- `$0742`'s bit 10 in one fuzz run (7 frames), `$1116` in 3 track_reference runs (one frame each),
  `$10B1` and `$074D` once in `league/organic-full-tour` at its end, and `$0748-0749` of a swapped
  league pair (`$83:95CA` swaps them and the race words after a league race, which native does not
  model; they are not read by a later boot).
All of these go to SAVE-IMAGE-REMAINDER.

## Limits

- Compared: every frame of 916 routes; 3 routes not run natively (above). A write of an unchanged
  value is not seen, and is equal by construction.
- The residual classes rest on R-0090's perturbation, review 1's (9 warm routes) and the listing;
  `$10B2` has no capture behind it.
- Follow-ups: [SAVE-IMAGE-REMAINDER](../../tasks/SAVE-IMAGE-REMAINDER.md) (the Value list) and
  [OPPONENT-POSE-BOUNDS](../../tasks/OPPONENT-POSE-BOUNDS.md): a libc++-hardened build aborts on
  `c175` at `presentation.cpp:243`, a `subspan` past the colour maths for the computer opponent
  `0x14` (main's code, unchanged here; a Linux debug run of that route would abort).
- `--records` and the `R` digests skip the idle demo race's frames (as on main: the runner
  continues before writing them); `--cartridge-trace` covers them.
- The original's logs come from pool processes that each run several cores in turn; `BsnesCore`
  asks for one per process. The cartridge RAM is unaffected (review 1's own capture, other process
  order, is byte-equal on 17 routes), but sound digests from such processes are not reproducible.
- A save whose league cursors the game never writes (above 6) sends `$80:F88D`'s store past the
  8 KiB in a 2P, VS or 1P result: native skips the byte and plays on; the original's store goes
  past `$77:1FFF`, where its mapping is not recovered (review 1).
- A saved `$10A7` of 14 or more (HUNTER's ending's drop) still starts native's markers from 0
  (R-0100); the endings' own `$10A7` stores are not modelled (`$83:B649` on), 7 routes' *value*
  differences.
- The league pair's swap (`$83:95C4`, `$83:95CA`) and the race words it swaps are not modelled.
- The pending reveal is native's one field shown at the current rider's place: a reveal pending when
  the rider changes stays at the old rider's place in the original.
- The app is compared on two scripted routes (three one-player races, no save at start).
- Save format and pack profile unchanged; old saves load as before (the image's layout is the
  original's, and every byte is read as before).

## Reproduction

From a checkout of the candidate (`local/` linked to main's), after building lab-release and
app-debug, `E=local/evidence/save-image-exact`:

```
python3 $E/routes.py <census manifests dir> $E/routes.json   # manifests.tar.zst of coverage-census
local/locks/heavy-run.sh SAVE-IMAGE-EXACT python3 $E/original_trace.py $E/routes.json $E/original 10
local/locks/heavy-run.sh SAVE-IMAGE-EXACT bash $E/sweep.sh $E/base-trace/front_end_runner OUT-main 8
local/locks/heavy-run.sh SAVE-IMAGE-EXACT bash $E/sweep.sh build/lab-release/src/core/front_end_runner OUT-cand 8
python3 $E/report.py OUT-main/compare.json OUT-cand/compare.json
python3 $E/classify.py $E/routes.json $E/original OUT-cand/native classify.json 120 4
python3 $E/behaviour_same.py MAIN_RUNNER build/lab-release/src/core/front_end_runner OUT.json 11
zsh $E/app_save.sh OUT
```
