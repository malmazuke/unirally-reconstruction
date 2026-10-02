# AUDIO-ONE-PLAYER - native audio through the whole one-player game

## Assignment

- Status: claimed 2 October 2026 05:42 UTC (15:42 Sydney) after verifying PR #51
  merged as `df0bf14`, main equal to `origin/main` and main's
  `artifacts/audio-first-race-integration/closeout.json`
  (`accepted_integrated_and_cleaned`, next ready task AUDIO-ONE-PLAYER).
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
