# R-0084 - A two-human race's restart, and the race counter's finish poses

Status: implemented on `task/two-human-restart`
([TWO-HUMAN-RESTART](../../tasks/TWO-HUMAN-RESTART.md)), 6 October 2026; accepted (tier 1,
PR #60). PAL ROM SHA-256
`a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`, audited bsnes core
`7d5aa1e656b9171524d01b1b22917197d8121cb4`.

This record follows up [R-0079](R-0079-two-pad-pause.md)'s "Not covered": what the original does
after QUIT in a two-human race's countdown restarts it. It corrects R-0079 on where the restart
re-enters NOW PLAYING.

Sources: four captures in main `local/evidence/two-human-restart/`, every frame's work RAM, with
pictures every third frame from NOW PLAYING on:
- `vs-restart`, VS on DRAGSTER, pad 2 QUIT in the countdown (2140), the restarted race, its
  result, VS CHAMPIONS and PICK CHALLENGER; 8,400 frames.
- `twop-restart`, the same in 2P, through the result, the continuation and NEXT TRACK's third
  race; 8,400 frames.
- `vs-control`, `vs-restart`'s VS session with no restart; 7,800 frames.
- `league-restart`, the league pair of R-0079's `league-restart`, restarted in the countdown,
  then the restarted race and its result; 11,800 frames.

A listing reader's report (`listing-report.md`) and the tools are beside them: `runs.py` lists a
capture's differing pictures as runs. Tags: **[L]** listing only, **[C]** confirmed in a capture.

## What the original does

### The restart returns to its own handler

`$80:88DD` tests pad 1's total `$77:0769`, then pad 2's `$77:07D3`, for 0xEA62, the restart's
mark. With either, it clears the text buffer, puts the logo up (`$80:F53F`), sends the blank text
(`$80:93A5`), fades (`$80:9869`) and returns Z=0. It writes nothing else. [L; C]

Each mode's handler calls it after the race and loops back to its own NOW PLAYING call on Z=0:
- 1P `$80:BC59` → `$80:BC36`;
- 2P `$80:BD80` → `$80:BD5D`;
- VS `$80:C00D` → `$80:BFE0`.

[C] In `vs-restart` the stack's return address is 0xBFE2 at the first NOW PLAYING (1702) and again
after the restart (2252, r + 112). In `twop-restart` it is 0xBD5F. Neither capture runs any code in
`$80:BC36-BC5C`. So the mode `$77:10AD`, the pairing and VS's flags (`$77:0742`, `$77:0750`) are
unchanged, and the next race is the same two-human race in the same mode. R-0079 named the
one-player handler's site, which was wrong.

### NOW PLAYING's counts are 2P's only

`$80:B297-B2B9` prints the stream `$80:B50A` (`FE 19 06 FD B2 00 FE 19 0A FD B4 00`) only when
`$77:10AD` = 2 (2P). It prints `$77:10A9` in row 6 and `$77:10AB` in row 10, as full numbers
through `FD`. VS (`$77:10AD` = 3) never prints counts, on the first visit or after a restart. 2P's
second rider choice zeroes both words (`$80:BD1C-BD23`). [L; C: `twop-restart` reads them at 1704,
2254 and 7302; `vs-restart` never does; the original VS NOW PLAYING pictures have no counts]

### The race counter picks the finish poses

The race setup increments the cartridge counter `$77:10B1` modulo 6 on every race, aborted races
included (`$83:CA08-CA18`; R-0076's `race_song_counter`). The first race after a cold start runs
with 1.

The finish blocks read it (rider 0's at `$83:E999-EA0A`, rider 1's at `$83:EB2B-EB9C`). Each takes
the pose table kind `(n & ~1) + 1` for the winner, or `+ 2` for the loser, and passes it to
`$82:894F`, which stores it in `$11E9,Y` and indexes the pointer table at `$17:C7C8`. So counter 0
or 1 gives tables 1 and 2, 2 or 3 gives 3 and 4, and 4 or 5 gives 5 and 6. The same selection
runs for every race kind and both riders, the computer opponent's included. [L]

[C] `$83:CA18` writes `$10B1` = 1 at 1899 and 2 at 2499: the aborted race counted. The restarted
race's loser takes kind 4 (from 4529) and its winner kind 3 (4559). In `vs-control` they take 2
and 1. In the pictures the restarted race's riders stay upright where a first race's lean and
fall.

This is not specific to the restart. Any race after the second since a cold start uses tables 3-6:
the 2P third race, or a one-player second race.

### After the restarted race

[C] Original against original (`vs-restart` f against `vs-control` f - 600): the same screens at
the same frames, the same cartridge writes (41 entries) with only the times and three checksums
different. The aborted race counts nothing but `$10B1`: no statistics, no wins, no records. The
restarted race's records are those of a race with no restart.

The league restart behaves the same way through the league pair's handler. [C: `league-restart`;
see Evidence]

## Native

- **The race counter.** `ClassicRaceScenario::race_counter` and `ZoomZooState::race_counter` hold
  `$77:10B1` after this race's count: the front end's `race_song`, set by `start_race` and the
  app's race scenario. From the menus a restart ends the race (0xEA62), and the next race's
  setup counts on through `race_song`, so the aborted race counts as in the original.
- **A restart on its own** (`restart_zoom_zoo`: the standalone runner, the app's result Enter,
  the fuzz runner) keeps the counter. It has no menus and no setup to count, and the frozen gates
  require such a restart to equal a fresh race. The first version counted here, and all eleven
  frozen gates failed their restart check (review, 68c3256).
- **The finish poses.** `finish_rider` takes kind `(counter & ~1)` + 1 or + 2. The state reader
  accepts kinds 1-6, each up to its table's last selector (48, 88, 32, 22, 16, 13 for tables 1-6;
  table 2's limit is lenient, as before).
- **Serialization.** Two-view states carry the counter byte after the look block: sizes 831 and
  883. A league wrapper carries it before its last byte. The reader refuses a counter of 6 or
  more, and a pose kind outside the counter's pair.
- **One-player layouts** are unchanged and carry no counter, as they carry no pairing: the
  restore that takes the menus' pairing takes their counter too (`deserialize_zoom_zoo`'s
  `race_counter`, refused at 6 or more). Without menus a one-player state reads with 1, so a
  restored later race that has not yet locked its finish poses would take tables 1 and 2.
- **NOW PLAYING.** The counts print in 2P only, from `records.player_wins` and `opponent_wins`
  through `FD`. 2P's second choice zeroes them.
- **The restart** already re-entered NOW PLAYING in its own mode at r + 112. Only the comments
  naming `$80:BC36-BC45` change.

## Evidence

Gates on `551a777` (`gates-551a777.out` beside the captures):
- **Race words and the look** equal the original's on every race frame of the four captures:
  `vs-restart` 2,331, `twop-restart` 2,855, `league-restart` 2,506 and `vs-control` 2,168. Every
  earlier split capture stays equal too.
- **Restores** are byte-identical to the end: `vs-restart` at 2700 and 4600, `twop-restart` at
  3200 and 4600, `league-restart` at 8600 and 9800 (a league wrapper; the restarted race runs with
  counter 2).
- **Pictures.** The restarted races' finishes (`vs-restart` and `twop-restart` 4530-4743,
  `league-restart` 9732-9870) now equal the original's. Everything that still differs differs
  the same on main's binary:
  - NOW PLAYING's arrows and icons;
  - the result icons and the slides after the result, the same in `vs-control`;
  - the league result's icons and one transition (11301-11358).
- **Front-end sweep against main.** 121 of 179 manifests are equal (`rows-551a777.txt`):
  - In 57 of the 58 others the two-view, league and one-view demo state rows differ by the
    counter byte.
  - In seven league manifests a later race's finish also takes other poses. The original's
    work RAM there has the counter at 3 and kinds 3 and 4 (`organic-stunt-tricks` 22100), or 3 and
    3 (`organic-three-events` 21700). Native now takes 3 where main took 1.
  - In `goldwyn` the only change is the picture at 13000, the finish of the session's third
    race (counter 3, tables 3 and 4). The manifest keeps no original frames; the review's capture
    of it shows `$10B1` = 3 and kinds 3/4.
  - Against the originals' frames (`fe-pictures-551a777.txt`) 6 pictures are better and none
    worse.
- **Unchanged:** the eleven frozen gates, the race sweep (432 runs, 0 differences), the cues and
  the pause menu's pictures.
- **Tests.**
  - `front_end_tests`: the restart's NOW PLAYING in 2P and VS, and 2P's second choice.
  - `dragster_race_tests`: the counter's winner and loser kinds, its byte in the split trailer and
    the league wrapper, and the refusals (6 or more, a pose outside the counter's pair).
  - `race_pairing_tests`: a restart on its own keeps the counter; the menus' restore takes theirs.

## Not covered

- NOW PLAYING's arrow and icon animation, R-0071's known residual, in 2P and VS alike.
- The pre-existing front-end gaps after the result, the same in the control: the result's icons,
  the result → VS CHAMPIONS and → PICK CHALLENGER slides, the 2P result → continuation slide.
- **Tables 5 and 6** (counter 4 or 5, a session's fifth and sixth races) are read from the
  listing only. Their bounds are checked against the pack, but no capture shows a 5/6 pose.
- **The attract demo** keeps counter 1. The original's demo skips the count (`$83:CA08`) but
  reads the cartridge's `$10B1`, so a demo shown after two or more races would take tables 3-6
  if a demo rider finished. No capture shows a demo rider finishing.
- **A one-player state's poses** are not checked against the caller's counter. The two-view
  reader refuses a locked pose outside its counter's pair; a one-player state with counter 1 and
  kinds 3/4 reads. No product path restores a one-player race.
- **SAME TRACK after a 2P result** (the review's capture): the original starts the DRAGSTER race
  one frame earlier than native's loading predicts (7127 against 7128). Not this task's change.
- The legacy DRAGSTER movement engine (`movement.cpp`, the v1 contracts' first races) keeps table
  1 for its opponent. No product race runs it.
