# R-0077 - native audio through the one-player game

Status: validated for [AUDIO-ONE-PLAYER](../../tasks/AUDIO-ONE-PLAYER.md), 3 October 2026; in review.
PAL ROM SHA-256 `a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`; bsnes
`7d5aa1e656b9171524d01b1b22917197d8121cb4` with R-0075's audio observation core v5, default
options, Strict synchronization, a fresh private save directory per process. Private captures and
scripts are in main `local/evidence/audio-one-player/`. It extends
[R-0076](R-0076-first-race-audio.md) from the first DRAGSTER race to the one-player game, under
[D-0010](../decisions/D-0010-frame-anchored-sound-commands.md).

## Method

Each schedule is captured twice from a cold start (`captures.sh`): `NAME-disp1` watches the first
61 dispatcher call sites (`JSL $82:8035`) in banks 80-83 and the enqueue entry `$82:8000`;
`NAME-disp2` the other 13, the eleven `JSL $82:807E` sites that name each sound session's score,
and `$82:8000` again. `derive_cues2.py` turns the pair into one cue list (frame, enqueue word,
dispatch site, session); `front_end_runner --sound-cues` writes native's, and `compare_cues.py`
compares them line for line. `strip_events.py` keeps only the CPU rows of a capture used for cues
(a raw capture is about 2 GB per 10,000 frames); full captures are kept only for the conditional
check and the measurement.

## Verified observations

1. **Race songs.** `$83:CA08-CBC8` increments the cartridge counter `$77:10B1` modulo 6 on each
   race's sound load and plays resource 64, 62, 63, 64, 65 or 66; counter 4 also sets the music
   gain to 255 (`$83:CB6A`), the others 127. Every race and stunt event loads the same tables
   (resource 54) and sample table (`$83:FC75`). `six-quits` plays all six on DRAGSTER.
2. **The race's loading follows its song.** The race sound session (`$83:CA72`) uploads the
   driver, the tables, the song and the samples in one piece of CPU work, and the race's first
   update follows in the frame after the session's polls, or two frames after when the polls run
   late in their frame (DUELLER's song 65). The upload's length differs by song (2,457 to 2,629
   bytes of score) and by about 70,000 master clocks between sessions of one song, so the frames
   are measured, not derived: every track the menus reach, six races each from NOW PLAYING
   (`track_schedule.py`, `loading.py`): see "Race loading" below.
3. **Race effects.** Besides R-0076's, a race starts effect 18 when a rider settles into mud
   (`$81:8999-89A3`, `$0F45` clear) and effect 19 on the first update after mud lets go
   (`$81:8690-86A9`, `$0F47`); effect 27 on a corkscrew's first ejection (`$81:87C2-87D9`, `$0F49`
   not negative); effect 23 on a long flight's flat landing (`$81:942B-9447`: surface mode `$0F23`
   clear, the landing angle `$2A` from -30 to 29, `$0FBF` at least 120, before `$81:945B`'s pose
   test) and on an A turn's braking contact (`$82:A3D7-A3F4`, which also halves velocity); effect
   26 on each whole second from 0:05.0 to 0:00.0 of a stunt event's clock (`$81:C8BB-C8ED`) and on
   each second of a race clock in 9:50-9:59, its 9:59.9 saturation included (`$81:C78A-C79D`);
   effect 31 at each HUNTER effect's start, after its announcement and HUD message
   (`$83:CF1C-D0E4`). The mud, landing and stunt-clock sounds appear on `six-quits-t3`, `lap-won`
   and `bowl-lose`; the corkscrew, the count-up clock's and HUNTER's are static readings with no
   captured case.
4. **The stunt result's tally.** Each pass that raises a count plays the move's sound (volume
   127, effect 3; `$80:F756-F75D`) unless pad 1 has one of its twelve buttons down (`$80:B6D3`);
   each column total plays the choice's (63, 4; `$80:F7A7`). The first pass reads `$72` as the
   race left it: `$83:987D` puts back the work RAM `$83:9894` saved at NOW PLAYING's choice, so
   `$72` holds the choice's press and the first pass is silent (a WRAM access log of `bowl-lose`
   shows the write at `$83:988A`, frame 4145).
5. **The medal award's sound set.** `$83:A614` loads driver 50, tables 52, score 58 and the samples
   of `$83:FBF5`, then gains 255 and 79, the music start and five dispatcher calls
   (`$83:A644-A66B`). The award's medal enqueues volume 127, effect 1 (`$83:A286`) as `$77:10CB`
   leaves 0x26 and 0x30 (`$83:B02C-B03A`); the command reaches the driver, but the effect's
   priority (5) loses every voice to the award's music, so it never sounds, in the original as
   in native (review of PR #52). Leaving it, `$83:A721` (from `$83:B119`) loads the title set again and
   enqueues the music start and gains 255 and 127 (`$83:A75E-A770`).
6. **The gold endings' sound set.** `$83:A507` loads driver 51, tables 55, score 60 and the samples
   of `$83:FD35`, then as the award. Driver 51 differs from 50 in five bytes: it has 29 effects
   (SPC `$04CE` `CMP #$1D`, against 32), their flags at `$1760` (`$04D4`, `$0504`, `$0513`) and
   pointer high bytes at `$1743` (`$0523`), against `$1766` and `$1746`. An effect at or past the
   count takes SPC `$04C7`'s path (`JMP $14B3`), which native does not recover.
7. **Upload over the title set.** An upload writes only the bytes it transfers, so the sound
   processor's RAM holds each session's tables and score over the title set's bytes. Every race,
   award and ending session follows the title set (the menus' or the race return's). The award's
   304 bytes of tables reach only the first ten bytes of driver 50's effect tables
   (`$1726-$172F`, effects 0-9's pointer low bytes); the effects' high bytes and flags are the
   menus' (native reading pointer `$1767` for the medal's effect 1 showed it).
8. **Each session in its frame.** The award's and the endings' sessions start in their fade's
   blank frame, which `$83:A4E9` leaves without a frame wait (`$83:A4FE-A506`), and their way back's
   title session likewise. No frame waits while a session uploads; the screen's loads after the
   upload wait only in the frame before the reset's and from the frame before the fade in's
   (`fifth-win`, `forced-silver`, `hopper-gold`).
9. **The endings' sounds.** The endings call 28 helpers at `$83:A286-A4D0`, one an effect 1-28
   (volume 127 for 1, 13 and 17-28, 63 for 2 and 3, 79 otherwise). The shared walk (`$83:B79C`
   and RUNNER's copy at `$83:C80C`) steps with effect 18 on its pose 0 and 19 on pose 11
   (`$83:B7B1-B7BE`, `$83:C825-C832`). SHUFFLER: effect 12 on the turtle's tiles but the first
   (`$83:B2E9-B2EB`), the wheel's walk sounds (`$83:B32B-B338`), effects 1 and 21 as the red uni
   lands (`$83:B374-B377`), 13 as the tongue pulls (`$83:B436`). WALKER: 3 at walk step 0x60
   (`$83:B60C-B615`), 2 at the hit (`$83:B63F`), 4 at fall step 0x10 (`$83:B695-B69E`), 5 at roll
   counts 0x0D and 0x1C (`$83:B74E-B763`). BOUNDER: 7 on each rising step from 0x21
   (`$83:B8FC`), 8 and 9 at the bump (`$83:B918-B91B`), 10 as the ball goes (`$83:BA10`), 7 at the
   bound (`$83:BA6C`), 7 and 11 at the second bound (`$83:BB2A-BB2D`). JUMPER: 14 as the elephant
   lands (`$83:BCFC`). SPRINTER: 17 as the wobble starts (`$83:BFD2`), 15 on each cartwheel's
   first pose (`$83:C07C-C081`), 2 at the knock (`$83:C0B3-C0B5`). HOPPER: 16 as the flame starts
   (`$83:C302`), 20 as the burn starts (`$83:C3BB`). CRAWLER: 26 as the flash starts (`$83:C5DC`;
   `$83:C5C7`'s test of step 0x60 branches to its own next instruction). RUNNER: 24 as the weight
   lands (`$83:C86E`). `all-gold` reaches all eight.
10. **Score control 87.** SPC `$0F34-$0F39` stores 0xFF at `$0231+X`, which `$0D5C-$0D68` tests
    and clears: the next note reads its duration inline even under a fixed duration. The race songs
    63-66 use it.
11. **HUNTER's ending.** Each newspaper page's reveal plays the forward slide's sound (79, 2) just
    before its tables are copied (`$80:E2D5`).

## Race loading

`six-quits` (DRAGSTER) and `six-quits-tN` (track N; `track_schedule.py`: MIKE's level written
3 after the cold format so every tour opens on both sides) each run six races from NOW PLAYING
at 1300 + 1500k or 1700k, quitting each through the pause menu. `loading.py` reads, for every race,
the frames from NOW PLAYING's choice (`D choice`, the fade's last frame) to the session's first FF
request, the start cue's lead (`E 2 15`, `$82:D84A`, before the request), the frames from the
request to the race's first update (`D early`) and the request's master clocks after its frame's
boundary. Every track gives one offset and one lead over its six races. Each upload cell is one race:
song 64 (counters 0 and 3) gives different lengths on tracks 19 and 25, so the table is by
counter, not by song; the request's clock varies by at most 1,772 clocks between one track's races
(`loading-tables.json`). Tracks 0-4 keep the first checkpoint's anchors (DRAGSTER's is R-0076's).

| Track | Offset | Lead | Upload by counter 0-5 | Request clock (median, range) |
|---:|---:|---:|---|---|
| 0 | 42 | 6 | 79 80 79 79 81 79 | 169,144 (168,798-170,034) |
| 1 | 90 | 6 | 79 80 79 79 81 79 | 124,219 (123,882-125,080) |
| 2 | 49 | 6 | 79 80 79 79 80 79 | 69,947 (69,762-70,252) |
| 3 | 117 | 6 | 79 80 79 79 81 79 | 94,109 (93,876-95,330) |
| 4 | 97 | 5 | 80 81 80 80 81 79 | 306,008 (305,738-306,968) |
| 5 | 90 | 5 | 80 81 80 80 81 79 | 404,106 (403,864-405,234) |
| 6 | 88 | 6 | 79 80 79 79 80 79 | 54,754 (54,548-56,248) |
| 7 | 50 | 6 | 79 80 79 79 80 79 | 110,668 (110,270-111,688) |
| 8 | 110 | 6 | 80 81 79 80 81 79 | 224,075 (224,016-225,368) |
| 9 | 73 | 6 | 80 81 79 80 81 79 | 245,818 (245,506-246,754) |
| 10 | 131 | 6 | 79 80 79 79 80 79 | 76,076 (75,898-77,340) |
| 11 | 81 | 5 | 80 81 80 80 81 79 | 270,156 (269,814-271,060) |
| 12 | 57 | 6 | 79 80 79 79 80 79 | 69,311 (69,166-70,234) |
| 13 | 90 | 5 | 80 81 80 80 81 79 | 349,876 (349,492-351,078) |
| 14 | 55 | 6 | 80 80 79 80 81 79 | 218,874 (218,584-220,018) |
| 15 | 102 | 5 | 80 81 80 80 81 79 | 392,136 (392,056-393,326) |
| 16 | 96 | 6 | 79 80 79 79 80 79 | 38,807 (38,496-39,638) |
| 17 | 44 | 6 | 79 80 79 79 80 79 | 28,452 (28,246-30,018) |
| 18 | 85 | 6 | 80 80 79 80 81 79 | 203,698 (203,478-205,240) |
| 19 | 94 | 6 | 79 80 79 80 81 79 | 140,423 (140,238-141,138) |
| 20 | 73 | 6 | 80 81 79 80 81 79 | 249,260 (249,068-250,550) |
| 21 | 81 | 6 | 79 80 79 79 80 78 | 8,936 (8,658-9,974) |
| 22 | 44 | 5 | 80 81 80 80 81 79 | 288,078 (287,762-289,468) |
| 23 | 84 | 5 | 80 81 80 80 81 79 | 380,421 (379,958-381,570) |
| 24 | 80 | 5 | 80 81 80 80 81 79 | 269,302 (268,792-270,548) |
| 25 | 93 | 6 | 80 80 79 79 81 79 | 157,932 (157,552-159,018) |
| 26 | 83 | 6 | 79 80 79 79 81 79 | 93,327 (93,070-94,834) |
| 27 | 58 | 5 | 80 81 80 80 81 79 | 318,300 (317,898-319,454) |
| 28 | 94 | 5 | 80 81 80 80 81 79 | 379,582 (379,254-380,574) |
| 29 | 68 | 6 | 79 80 79 79 80 78 | 1,142 (1,014-2,230) |
| 30 | 89 | 6 | 79 80 79 79 80 79 | 46,304 (45,894-47,268) |
| 31 | 95 | 5 | 80 81 80 80 81 79 | 371,964 (371,808-372,964) |
| 32 | 48 | 6 | 79 80 79 79 80 79 | 34,262 (33,948-34,930) |
| 33 | 87 | 6 | 79 80 79 79 80 79 | 90,689 (90,286-91,922) |
| 34 | 71 | 6 | 79 80 79 79 80 79 | 69,184 (69,040-70,610) |
| 35 | 110 | 6 | 80 81 79 80 81 79 | 240,970 (240,648-241,510) |
| 36 | 66 | 5 | 80 81 80 80 81 79 | 305,180 (304,930-306,252) |
| 37 | 47 | 6 | 79 80 79 79 80 79 | 125,412 (125,178-126,734) |
| 38 | 82 | 5 | 80 81 80 80 81 79 | 296,727 (296,666-297,964) |
| 39 | 94 | 5 | 80 81 80 80 81 79 | 348,187 (347,878-349,182) |
| 40 | 142 | 5 | 80 81 80 80 81 79 | 264,025 (263,576-264,828) |
| 41 | 69 | 6 | 80 80 79 80 81 79 | 215,139 (214,922-216,686) |
| 42 | 77 | 5 | 80 81 80 80 81 79 | 335,340 (335,092-336,678) |
| 43 | 91 | 6 | 80 80 79 80 81 79 | 226,994 (226,632-227,718) |
| 44 | 71 | 6 | 80 81 79 80 81 79 | 225,821 (225,262-226,540) |

The upload itself varies with the sound processor's state when the request arrives: on track 35
with song 62 the session took 33,819,146 master clocks in `six-quits-t35` and 33,771,132 in
`all-gold`, so the polls ended 14,572 clocks into the 80th frame after the request in one and
32,548 clocks before it in the other; the race started a frame earlier in `all-gold` (80 frames
against the table's 81). Of `all-gold`'s 25 races it is the only one off the table. Native's front
end does not run the sound processor, so its race start follows the table; the laboratory runner
takes such a race's start from the capture (`--race-initialization`, `race_inits.py`), which moves
the session's upload, not its request.

## Native producers

The race engine reports, in program order, the effects of observation 3 (`race_sound::effect`):
mud's entry and exit from the special tiles' update (`SpecialTileUpdate::sound_effect`, the
counters' return), the corkscrew's first ejection, the long flight's landing from
`resolve_vertical_contact`'s return, an A turn's brake from `update_reflection_transition`'s
return, the stunt clock's and the race clock's warnings (`update_stunt_clock`, `timer_warning`)
and HUNTER's effect starts. The stunt result's passes and totals play the menus' sounds; NOW
PLAYING keeps its choice's pad word (`NowPlaying::choice_pads`). The award (`tour_award_frame`)
and the endings (`tour_ending_frame`) report their sessions in their blank frames, and the way
back its title session; `award_frame_waits`, `tour_ending_frame_waits` and
`hunter_ending_queue_waits` report the original's waits. The endings' scripts call
`play_screen_sound` (the `$83:A286-A4D0` helpers) and the shared `walk` its footsteps;
BOUNDER's bump and ball loops build the walk's pose without them (`walk_pose`). The boot after a
soft reset reports its session on its frame 28 (`AudioSessionLoad::reset_boot`).

The audio side loads each session at its declared anchor (D-0010): the race's by track
(`race_load_anchors`), the award's 2,342, an ending's 2,476, the way back's 1,278 and the reset
boot's 371,820 master clocks after their frames' boundaries (medians of 24 awards, 3 endings and
27 title sessions; the reset boot's from `all-gold`'s one reset). After the samples the award and
the endings enqueue gains 255 and 79 and the music start and poll five times; the way back
enqueues the music start and gains 255 and 127. The sets are composed as observation 7 says.

## Native cues

`cues.sh` runs `front_end_runner --sound-cues` on a schedule's inputs and record writes and
compares with the cues derived from its two captures (frames 620 on). Equal line for line on 60
schedules: `two-dragster`, `six-quits`, `two-races`, `cont-loss`, `lap-won`, `bowl-lose`,
`bowl-quit`, `fifth-win`, `forced-silver`, `hill-win`, `hill-complete`, `hopper-gold`,
`locked-gold`, `hunter-t41-right` and `hunter-t44-right` (two tags each: effect 31 after the late
dispatch), `six-quits-t1` to `six-quits-t44` (every track, six races each) and `all-gold`
(58,179 lines through 25 completions, eight gold endings, HUNTER's ending, the soft reset and the
title after it; with one race's start, track 35's, and the reset delay of 2 frames taken from the
capture, as the laboratory runner takes them). R-0076's seven schedules stay equal; their
derived cues name a race load `race`, which the native log now names by song.

## Measured agreement (D-0010)

`measure.sh` plays each schedule's native cues through `race_audio_runner` from power-on (no
original clock or event as input) and compares the commands reaching the driver and the raw PCM
with the capture. PCM is identical up to the first anchored command (pair 480,386). Level of each
20 ms window with signal (`pcm_metrics.py`):

| Schedule | Commands in frame | Identical pairs | Windows | Median level difference | 90th / 99th percentile | Within 1 dB |
|---|---|---|---|---|---|---|
| `six-quits` | 258 of 259 | 1,599,144 of 5,702,149 | 7,479 | 0.22 dB | 1.02 / 2.38 dB | 90% |
| `fifth-win` | 249 of 250 | 1,006,299 of 4,036,945 | 5,815 | 0.36 dB | 1.73 / 3.71 dB | 76% |
| `hopper-gold` | 181 of 186 | 1,608,891 of 3,651,888 | 4,240 | 0.36 dB | 2.49 / 21.79 dB | 74% |
| `bowl-lose` | 173 of 173 | 896,961 of 3,460,301 | 5,089 | 0.22 dB | 0.96 / 2.07 dB | 91% |
| `lap-won` | 171 of 171 | 925,318 of 5,381,791 | 8,044 | 0.47 dB | 2.02 / 3.94 dB | 71% |
| `locked-gold` | 464 of 476 | 3,603,199 of 10,443,406 | 11,697 | 0.47 dB | 2.10 / 17.46 dB | 71% |
| `six-quits-t35` | 271 of 271 | 1,831,412 of 7,368,000 | 9,675 | 0.13 dB | 0.54 / 1.51 dB | 97% |
| `six-quits-t41` | 257 of 257 | 1,528,093 of 7,368,009 | 9,923 | 0.17 dB | 0.72 / 1.85 dB | 95% |
| `hunter-t41-right` | 182 of 182 | 732,518 of 3,459,677 | 5,062 | 0.11 dB | 0.59 / 1.33 dB | 97% |
| `all-gold` | 1,600 of 1,627 | 7,858,704 of 27,413,906 | 31,458 | 0.43 dB | 2.04 / 16.24 dB | 73% |

Over the 44 `six-quits-tN` runs 11,496 of 11,526 commands reach the driver in the original's
frame; their median level differences are 0.13-0.34 dB. Commands off their frame are those queued
at a session's end or right after it: the session's upload ends earlier or later than the
original's with the running driver's response to the request (the ending's in `hopper-gold`
ended 75,700 clocks early, so its music start reached the driver in the session's own frame wait,
six frames before the original's), and the menus' restore frame after the award waits from the
generic frame-wait anchor, earlier in its frame than the original (`fifth-win`'s last title
command a frame early). The wide 99th percentiles of the award and ending schedules are those
windows. `compare_anchored.py` takes a word store whose bytes are 46 clocks apart (DRAM refresh
between them) as one command.

## Continuation

`continuation/run2.sh` saves the cued playback after a frame (48 kHz, 257-pair drains), restores
it in a fresh process and requires the concatenated PCM and events and the final state to equal
the uninterrupted run's: 26 of 26 saves on `hopper-gold` (inside the race load, the race, the
title reloads, both awards' sessions and animations, the ending's session and script, and the
menus) and 11 of 11 on `bowl-lose` (the stunt event and its tally), with R-0076's 23.

## Conditional driver and DSP

`cond.sh` feeds a full capture's CPU port writes at their SMP ticks to the native IPL, driver and
DSP from zero (`audio_driver_runner`, `ipl` mode), each session with its set as the product
composes it (`sets/`; an ending's tables with driver 51's layout, `@51`), and compares every
ordered DSP-register and SMP-port write and every raw stereo pair:

| Capture | Sessions | Ordered writes | Raw stereo pairs | Result |
|---|---|---:|---:|---|
| `full-six-quits` | boot, six races (songs 62-66, 64 twice), six title reloads | 1,863,797 | 5,702,149 | equal |
| `full-hopper-gold` | boot, three races, two awards, an ending, six title reloads | 1,383,923 | 3,651,888 | equal |
| `full-all-gold` | boot, 25 races, 16 awards, 8 endings, title reloads, HUNTER's ending, the reset boot | 10,521,739 | 27,413,906 | equal; native writes one more after the capture's end |
| `full-lap-won` | boot, a three-lap race, the title reload | 1,336,353 | 5,381,791 | equal |
| `review/full-fifth-win` (the reviewer's) | boot, a race, a title reload, an award with 61 medal commands, the way back | 1,176,515 | 4,036,945 | equal |

Score controls 87, 98, 99, 9A and A4 (observation 10 and below) were found this way: the race
songs 63-66 use 87; HOPPER's ending's noise uses 98-9A; the lap race uses A4. Control A4
(`$1103-$113F`) calls one of a table's targets chosen by the driver's random routine (`$13DA`):
it skips to the choice's word (CLRC drops ASL's carry), reads it, skips the table's rest (that
ASL's carry is added), pushes the return pointer (`$1301`) and jumps. Control 98/99 (SPC `$1052-$1061`) sets/clears
the voice's bit of `$D6`, which the music pass writes to DSP register 3D (`$06C0-$06C3`) and a
voice's reset clears (`$08B4-$08B9`); control 9A (`$1064-$1073`) writes the noise clock with
echo writes off into FLG from the score.

## Limits

- The product's PCM after the first anchored command is measured, not exact (D-0010).
- A race's first update follows the measured (track, song) table; the session's upload length
  varies with the sound processor's state, so the original's race can start a frame earlier or
  later (one of `all-gold`'s 25 races). The local and league modes keep their earlier measured
  loading for tracks 0-4 and do not track the song counter.
- Static readings with no captured case: a corkscrew's ejection (effect 27), the race clock's
  9:5x warning, the cheat newspaper page's reveal, and R-0076's slow-checkpoint clear and BRONSEN's
  praise voices. Effects at or past a driver's count take an unrecovered path, which native
  refuses.
- The anchors of the award's, the endings', the title reloads' and the reset boot's sessions are
  medians of 24, 3, 27 and 1 sessions.
- 2P, VS, league, OPTIONS and the attract demo stop the cued producer.
