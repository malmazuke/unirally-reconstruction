# AUDIO-ONE-PLAYER - native audio through the whole one-player game

## Assignment

- Status: validated candidate `e023ba8` (3 October 2026), in tier-1 review on its pull
  request. Claimed 2 October 2026 05:42 UTC (15:42 Sydney) after verifying PR #51
  merged as `df0bf14`, main equal to `origin/main` and main's
  `artifacts/audio-first-race-integration/closeout.json`
  (`accepted_integrated_and_cleaned`, next ready task AUDIO-ONE-PLAYER).
- Sessions: Claude Fable 5.1 to the first checkpoint (`c4b4cef`); Claude Opus 5.5
  (`claude-opus-5-5`) from there to the candidate. Children and reviewer: fresh Claude
  subagents (D-0004, Anthropic-started task).
- Primary/coordinator: Claude Code, Claude Fable 5.1 (`claude-fable-5-1`). This task
  starts on Anthropic, so under D-0004 its children and its independent reviewer are
  fresh Claude subagents, not Sol.
- Quota at claim: Claude weekly all-models 16% (14% at this session's start), five-hour
  54%. D-0004 checkpoint at a 20-point increase (36%); the user's standing rule allows
  continuing to 80% weekly, with the last 20% reserved for review and recovery.
- Branch/worktree: `task/audio-one-player`, `.worktrees/audio-one-player`, from main
  `df0bf14` (pack v32, the three presets built, ctest not yet rerun here).
- Milestone: M4 (original game coverage); not the complete M4 gate.
- Tier: 1. New sound producers, song selection state, content extraction and
  scene continuation require the full D-0006 review process.
- Provider/model/budget: under D-0004 the claiming session's provider owns the
  task, its children and its independent reviewer. Record usage at claim and
  every checkpoint; no reset, spending or provider change.
- Dependencies: AUDIO-FIRST-RACE, D-0009, D-0010, R-0075, R-0076; the accepted
  native one-player front end (setup, tours, results, awards, endings, HUNTER)
  and the stunt events.
- Owned scope: native sound producers in the race engine and front end, the
  audio transport's anchors for new dispatch sites, affected sound sets and
  pack rules, SDL audio scene ownership, laboratory tooling, tests and records.
- Evidence/log homes: main `local/evidence/audio-one-player/` and
  `artifacts/audio-one-player-integration/`.

## Outcome and rationale

From native power-on, play continuous native audio through every one-player
path the native game already plays: all tours' races and their songs, lap
races, the four stunt events, results, awards, tour endings and the HUNTER
ending, with Race Again, pause/restart/quit and the return to the menus.
AUDIO-FIRST-RACE stops the producer after the first race because later races
play other songs (`$83:CA08-CBC8` selects resources 64, 62, 63, 64, 65, 66 from
the cartridge counter `$77:10B1`) and other tracks may use other sample sets.

Use the D-0010 pattern: native producers with exact command content, order and
frame; declared frame-anchored dispatch clocks; the recovered driver, score,
samples and DSP. No original CPU/SPC execution, captured events or
prerecorded audio in the product.

## First experiment

1. Verify PR #51's merge and closeout and claim an isolated checkout.
2. Read the static listing for the song and sample-set selection, the stunt
   and lap race sound sites and the result/award/ending producers; cite them.
3. Capture a cold one-player schedule through two consecutive races with
   dispatch/enqueue watches (AUDIO-FIRST-RACE's `capture.py`,
   `derive_cues.py`), then extend the producers until native cues match.
4. Prove the driver exact for each new song and sample set conditionally on
   the original's port writes before measuring anchored agreement.

## Checkpoint, 2 October 2026 06:45 UTC (usage limit; source uncommitted work committed as is)

Verified so far (private captures and scripts in main `local/evidence/audio-one-player/`;
`captures.sh NAME` makes NAME-disp1/2 with every dispatcher site plus the eleven `JSL $82:807E`
score sites, `derive_cues2.py` names each session's set and song, `captures_enq.sh` watches the
112 enqueue sites, `enqueues.py` lists them; `strip_events.py` keeps only CPU rows, as 40
captures filled the disk once):

- Static: six race songs by the cartridge counter `$77:10B1` (resources 64, 62, 63, 64, 65, 66;
  counter 4 also sets music gain 255, `$83:CB6A`); every race and stunt event loads the same
  tables (54) and sample table (`$83:FC75`); the award's set is 50/52/58 + `$83:FBF5`
  (`$83:A614`), the gold endings' 51/55/60 + `$83:FD35` (`$83:A507`, driver 51 differs from 50
  in five bytes: effect bound 0x1D and table bases), and `$83:A721` reloads the title set.
- Captured on the original: `two-dragster`, `six-quits` (DRAGSTER x6), `six-quits-t1..t4`
  (ZOOM ZOO, CRAWLER STUNT, DUELLER, GOING UP x6 each), `two-races` (cont-win), `cont-loss`,
  `lap-won`, `hill-win`, `bowl-lose`, `bowl-quit`, `hill-complete`, `fifth-win`,
  `forced-silver`, `hopper-gold`, `locked-gold`, `all-gold` (its disp2 shows `unknown`
  sessions from frame 34329: check its integrity before trusting it).
- The race's loading depends on the song: the sound session uploads driver, tables, song and
  samples in one piece of CPU work and the race's first update follows in the frame after the
  upload ends, so the first update is FF + 79..81 frames by track and song (thirty sessions,
  `race_sound_upload_frames` table; the earlier constants 127/197/175 for tracks 2-4 embedded
  songs 64/65/66). The sound request follows the choice frame by 42/90/49/117/97 frames; the
  start's countdown cue runs 6 frames before it (5 on GOING UP). Native now plays all six
  songs on all five tracks with these timings.
- Native cues equal the original's line for line on `two-dragster`, `six-quits`, `two-races`,
  `cont-loss`, `six-quits-t1` and `six-quits-t4` (loads named by song). The stunt event's
  countdown has three beeps (270, 220, 160), not four.
- Remaining differences: effect 18 at `$81:89A3` (mud tile, `$0F45` clear; DUELLER), effect 23
  at `$81:9447` (a contact sound: |x| < 30, `$0FBF` >= 120, `$0F23` clear; ZOOM ZOO), effect
  26 at `$81:C8ED` (stunt scoring, `$0E21` < 6; BOWL), the stunt result's tally sounds
  (`$80:B12B/B132` 8 63 + 2 4 every 14 frames, `$80:B17F/B186` 8 127 + 2 3 every 8), the
  award, ending and `title-award` sessions (no native producers yet), tracks beyond 0-4 (no
  loading measured: HILL CLIMB 22 needs a `six-quits-t22` schedule), and the anchored
  measurement/continuation/gates on the new code.
- Source state at this commit: front end song counter and loading tables; audio side with
  per-song sets, per-track load anchors and the loud gain; pack profile v33 (songs 2-5) built as
  `local/classic-pal-crawler-tracks-v33.pack`; the three presets build and ctest passes 41/41;
  `docs/map/static/native-symbols.json` not regenerated; measure.sh and continuation/run.sh
  still name the v32 pack; no R-0077 record yet; no review.
- Next: regenerate native-symbols; write R-0077 from the notes above; add the three race
  effects and the stunt result sounds; capture the enqueue sites for the award/ending screens
  and recover their producers and sets (pack v34); then measurement, continuation, gates,
  review and the PR.

## Checkpoint, 3 October 2026 (Claude Opus 5.5 session, source `ef995c5` + uncommitted records)

This session resumed the Fable 5.1 checkpoint on Claude Opus 5.5 (`claude-opus-5-5`); weekly
all-models usage 24% at its start, 27% here. Tools moved to main `local/evidence/audio-one-player/`:
`cues.sh NAME` (native cues vs the original's; `INITS=1` takes each race's start from the capture,
`NAME.reset-delays.txt` the reset delay), `track_schedule.py`, `track_batch.sh`, `loading.py`,
`loading_tables.py` (writes `loading-tables.json`), `loading_cpp.py`, `hunter_schedule.py`,
`race_inits.py`, `cond.sh CAPTURE SESSIONS...` (conditional driver/DSP check), `sessions.py`,
`upload_len.py`, `dis.py`/`lst.py` (65816 reading aids), `sets/` (each set as the product composes it).

Done and verified since the first checkpoint:
- Race effects 18/19 (mud), 23 (long flight's flat landing, A turn's braking contact), 26 (stunt
  clock 0:05-0:00, count-up 9:5x), 27 (corkscrew), 31 (HUNTER effect start); the stunt tally's
  pass/total sounds (first pass reads NOW PLAYING's choice pads restored by `$83:987D`).
- Award (driver 50/tables 52/score 58, `$83:FBF5`) and ending (driver 51, tables 55, score 60,
  `$83:FD35`) sessions, the way back's title reload, the award's medal sound, all eight endings'
  script sounds (helpers `$83:A286-A4D0`, walk footsteps), HUNTER's page reveal sound, its waits,
  the soft reset's boot session; PICK TOUR's reveal sound moved after its redraw.
- Pack profile v34 (award/ending sets, 15 new samples). Each set is its upload over the title
  set's bytes (the award's tables do not reach the effect tables). Driver 51's effect layout.
  Score controls 87 (inline duration), 98/99 (noise voices `$D6` to DSP 3D), 9A (noise clock to
  FLG); audio state URAU0006.
- Loading measured for all 45 tracks (six races each; `loading-tables.json`).
- Cue equality, line for line: two-dragster, six-quits, two-races, cont-loss, six-quits-t1..t44
  (all 45 tracks), lap-won, bowl-lose, bowl-quit, fifth-win, forced-silver, hill-win,
  hill-complete, hopper-gold, locked-gold, all-gold (58,179 lines; with one race's start and the
  reset delay 2 from the capture: the upload's length varies with the sound processor's state, so
  track 35 song 62 took 80 frames there and 81 in six-quits-t35).
- Conditional exactness (original port writes into native IPL/driver/DSP): full-six-quits
  (1,863,797 rows, 5,702,149 pairs), full-hopper-gold (1,383,923 / 3,651,888), full-all-gold
  (10,521,739 / 27,413,906; native adds one row after the capture's end).

Open: `hunter-t41-right`/`hunter-t44-right` captures (HUNTER tag effect 31; native tags twice in
each) were running; R-0077 is drafted in the worktree (sections pending); anchored measurement
(`measure.sh`, update to v34 and per-load tracks), fresh-process continuation at race/result/award/
ending loads, native-symbols and static-map regeneration, D-0010/STATE/README records, gates,
review, PR.

## Candidate `e023ba8`, 3 October 2026

Source candidate `e023ba8` (records follow it). [R-0077](../docs/research/R-0077-one-player-audio.md)
holds the observations, the loading table, the producers and the measured domain.

- Gates on `ea32eaa` (`local/evidence/audio-one-player/gates.sh`, output
  `gates-ea32eaa.out` and `gates-ea32eaa-tail.out`): lab-debug, lab-release and app-debug
  build, ctest 41/41 each; v1 winner and loser contracts pass; eight hidden app runs and the
  front end's Start-held run pass with no pose fallbacks; the eleven frozen race gates pass with
  6,023 restores and the same row digests as main; the race equivalence sweep against main
  `df0bf14` finds 0 differences in 2,387,105 updates (432 runs); AUDIO-TITLE-MENU's six frozen
  comparisons stay equal; R-0076's continuation 19 + 4 PASS. The DRAGSTER fuzz gate aborts 40 of
  40 as on main (recorded non-pass, own follow-up). The synthetic suite and the native-symbol
  check failed there on a stale index (regenerated in `e023ba8`); one function over 80 lines
  (split in `e023ba8`).
- Audio on the candidate: native cues equal on 60 schedules (R-0077 "Native cues") and R-0076's
  seven (its derived cues label race loads `race`); measured agreement (R-0077 table; 11,496 of
  11,526 commands in frame over the 44 track runs); conditional exactness on four full captures;
  continuation 26/26 (`hopper-gold`) and 11/11 (`bowl-lose`).
- Front-end equivalence against main: 149 of 174 manifests equal on `ea32eaa`. `e023ba8`
  restores the league and local modes' loading (the checkpoint had replaced it with the
  song-62 formula, which moved 7 league manifests); the remaining differences are one-player
  runs whose races now start at the measured song table's frame (`template`, `lap-record`,
  `lap-record-frames`, `forced-silver-bronsen`, `goldwyn`, `bowl-lose`, `bowl-press`,
  `bowl-quit`, whose quit then catches one more trick) or that now run past tracks main could not
  load (`all-gold`, `hopper-gold`, `locked-gold`, `reveal`, `shuffler-gold`, `walker-gold`,
  `hill-win`, `hill-complete`, `track-race`). Where this task captured the same inputs
  (`bowl-lose`, `bowl-quit`, `hill-win`, `hill-complete`, `hopper-gold`, `locked-gold`,
  `all-gold`), native cues equal the original's, race starts included; the others follow the
  same per-track table, measured on six races of every track.
- `e023ba8` re-run (`gates-e023ba8-synthetic.out`, `gates-e023ba8-tail.out`): see the PR.

## Review 1, 3 October 2026

Fresh Claude Opus 5.5 subagent in its own clone (comment review on PR #52 of `6ea3656`):
changes required, two blocking and four record findings. Reproduced: build and ctest, pack v34
byte-identical, eleven cue comparisons and `all-gold` (`INITS=1`), the `full-hopper-gold`
conditional check, seven continuation saves. Withheld: `w-restart-quit` and `w-tally-held` equal;
`w-demo-race` found finding 2. Its outputs are in main `local/evidence/audio-one-player/review/`.

1. Ubuntu CI: `audio_score_controls.cpp` control A4's `rest >> 7U` promoted to `int`
   (`-Wsign-conversion`). Fixed (`unsigned{rest}`).
2. The attract demo advanced the song counter (`choose_race_song` in `demo_title_frame`);
   `$83:C9F6-CA05` jumps to `$83:CBF2` with the demo flag, past `$83:CA08`. Fixed; `w-demo-race`
   now loads `race-62` at 4249 with cues equal from frame 4000 (the demo's own title reloads stay
   outside the cued domain).
3. Records: the award's tables reach `$1726-$172F`; the medal's effect never gets a voice. Fixed in
   R-0077 observations 5 and 7 and the content comment.
4. The reviewer's `full-fifth-win` (an award with 61 medal commands) is conditionally equal;
   added to R-0077's table.
5. STATE/NEXT_SESSION's level range: corrected to 0.13-0.34 dB for the 44 track runs.
6. The loading table's cells are single races, by counter (song 64 differs on tracks 19 and 25):
   stated in R-0077.

## Acceptance and closeout

- Native cues equal the original's on frozen schedules covering every song,
  each sample set, a lap race, a stunt event, an award and an ending.
- Conditional exact driver/DSP output for every new sound set; measured
  arrival error and PCM agreement for the product path (D-0010).
- Fresh-process partial-drain continuation at race, result and scene loads.
- Front-end and race "nothing moves" sweeps against main's binaries, the
  frozen race gates, app checks and hosted macOS/Ubuntu CI.
- Fresh isolated tier-1 review, PR with green checks, live listening with
  retained delivery counters, merge commit, synchronized main and cleanup.
