# R-0077 - native audio through the one-player game

Status: in progress for [AUDIO-ONE-PLAYER](../../tasks/AUDIO-ONE-PLAYER.md), 3 October 2026.
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
   (`$83:A644-A66B`). The award's medal plays volume 127, effect 1 (`$83:A286`) as `$77:10CB`
   leaves 0x26 and 0x30 (`$83:B02C-B03A`). Leaving it, `$83:A721` loads the title set again and
   enqueues the music start and gains 255 and 127 (`$83:A75E-A770`).
6. **The gold endings' sound set.** `$83:A507` loads driver 51, tables 55, score 60 and the samples
   of `$83:FD35`, then as the award. Driver 51 differs from 50 in five bytes: it has 29 effects
   (SPC `$04CE` `CMP #$1D`, against 32), their flags at `$1760` (`$04D4`, `$0504`, `$0513`) and
   pointer high bytes at `$1743` (`$0523`), against `$1766` and `$1746`. An effect at or past the
   count takes SPC `$04C7`'s path (`JMP $14B3`), which native does not recover.
7. **Upload over the title set.** An upload writes only the bytes it transfers, so the sound
   processor's RAM holds each session's tables and score over the title set's bytes. Every race,
   award and ending session follows the title set (the menus' or the race return's). The award's
   304 bytes of tables stop short of driver 50's effect tables (`$1726` on), so its effects are the
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

[Table pending: per track, the frames from the choice to the session's first FF request, the
start cue's lead, the race's first update by song counter, and the request's clock.]

## Native producers

[Pending.]

## Measured agreement (D-0010)

[Pending.]

## Conditional driver and DSP

[Pending.]
