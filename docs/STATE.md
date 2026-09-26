# Project state

Updated 26 September 2026 (RACE-RIDERS-OPPONENTS): **every rider races, against every opponent
the one-player menus choose.**
- The rider changes no physics. It chooses its voices, its tutorial hints (the records keep which
  riders' hints have ended), its sprite colours and the HUD's ink colour.
- The opponent sets the computer's skill: SILVIA (after a bronze) and GOLDWYN (after a silver)
  catch up faster and launch their tricks by their own rules.
- Against seven captures of the original's races every update matches. Their pictures match apart
  from the off-screen rider arrow, which native has never drawn, and two one-frame residues on one
  track ([RACE-OFFSCREEN-ARROW](../tasks/RACE-OFFSCREEN-ARROW.md)). The menus around ANDREW's, SILVIA's
  and GOLDWYN's races match on every frame: 7,957 pictures and the records
  ([R-0061](research/R-0061-riders-and-opponents.md)).
- The caption's blank now reaches the screen a picture later, as in the original.
- Pack profile **v21** adds two tables (`content pack --rules
  tests/manifests/content/classic-crawler-tracks-pack.json --out
  local/classic-pal-crawler-tracks-v21.pack`).

Updated 26 September 2026 (RACE-PAUSE-EXITS): **the race's pause menu ends a one-player race as
the original does.**
- In a race started from the menus, its second choice (the original's QUIT) goes back to NOW
  PLAYING during the start countdown, and after it shows the result with QUIT and counts a loss.
- Leaving a result where no record was placed is a frame shorter, as in the original.
- Against three captures of the original (a quit, a restart and a lap race's quit), every frame
  matches: 529 pictures, the menus' state and the records ([R-0060](research/R-0060-pause-exits.md)).

Updated 26 September 2026 (FRONT-END-TOUR-END): **a tour's completion shows the medal award,
as the original does.**
- The medal falls onto the rider on a podium until a press, then the menus come back, the unlock
  rule runs, and PICK TOUR returns with the new medal; its choice, or Y, goes on to PICK TRACK.
- A completion comes from a tour's fifth win, or at once when pad 1 holds exactly Select + X + R
  as the result is left (the original's forced completion).
- Against captures of the original (a bronze and a silver award, and PICK TOUR's exits), every
  frame matches: 0 differing pixels in 4,161 pictures, and equal menu state, OAM buffer
  and text map ([R-0059](research/R-0059-tour-completion.md)). The SNES screen now draws mode 2.
- Not yet: the gold medal's endings and the reveal of newly opened tours
  ([FRONT-END-ENDINGS](../tasks/FRONT-END-ENDINGS.md)).
- Pack profile **v20** adds the award's content (`content pack --rules
  tests/manifests/content/classic-crawler-tracks-pack.json --out
  local/classic-pal-crawler-tracks-v20.pack`).

Updated 26 September 2026 (FRONT-END-LAP-RESULT): **after a lap race too, 1P goes on as the
original does.**
- A lap race's result is the menus' graph of every lap: twenty dots fly to their laps' times,
  beside each rider's total and best lap and the track's record line. A press on pad 1 leaves it.
- The best laps become the personal bests and the track's records, as the original's cartridge
  RAM has them. So every race the app starts (MIKE against BRONSEN, not the stunt event) now
  comes back to PICK TRACK.
- Known difference: the race's pause menu restarts inside the race and has no quit, where the
  original goes back to NOW PLAYING or scores the quit ([RACE-PAUSE-EXITS](../tasks/RACE-PAUSE-EXITS.md)).
- Against three captures of the original (a won and a lost ZOOM ZOO, and a second race with a
  record), every frame matches from each race's return to the end: 0 differing pixels in 4,104
  pictures, and equal menu state, OAM buffer and text map ([R-0058](research/R-0058-lap-result.md)).
- Pack profile **v19** adds the lap result's text (`content pack --rules
  tests/manifests/content/classic-crawler-tracks-pack.json --out
  local/classic-pal-crawler-tracks-v19.pack`).
- Next: the tour's end ([FRONT-END-TOUR-END](../tasks/FRONT-END-TOUR-END.md)).

Updated 26 September 2026 (FRONT-END-1P-CONTINUATION): **after a one-run race, 1P goes on as
the original does.**
- The menus take over on the frame the native race's result load begins: the menus' reload, the
  result screen with the track's records, the waits for a release and a press, then PICK TRACK
  again on the next track after a win.
- The statistics, the top three records, the personal bests, the done tracks and the loss flag
  are updated as the original's cartridge RAM has them. There is no game over in the original.
- Against a won and a lost DRAGSTER, every frame matches from the race's return to the next race:
  0 differing pixels in 2,577 pictures, equal menu state, OAM buffer and text map, and equal
  records ([R-0057](research/R-0057-one-run-result.md)).
- Pack profile **v18** adds the result's stream and icons (`content pack --rules
  tests/manifests/content/classic-crawler-tracks-pack.json --out
  local/classic-pal-crawler-tracks-v18.pack`).
- Next: the lap result ([FRONT-END-LAP-RESULT](../tasks/FRONT-END-LAP-RESULT.md)), then the
  tour's end ([FRONT-END-TOUR-END](../tasks/FRONT-END-TOUR-END.md)).

Updated 26 September 2026 (FRONT-END-1P-SETUP): **1P runs the original's four setup screens
to the race.**
- PICK YOUR UNI, PICK TOUR, PICK TRACK and NOW PLAYING slide in and out as in the original, with
  every back path, and NOW PLAYING's Race starts the chosen race.
- The race scenarios are MIKE's against BRONSEN. Another rider or a stunt event shows a notice.
- Against nine captures of the original, every frame matches: 0 differing pixels in 6,166
  pictures, and equal menu state, OAM buffer and text map. The captures cover moves, locked
  tours, the medal line, laps and stunt texts, and Exit.
- The SNES screen now applies colours written during the picture (the rider menu's HDMA palette
  split), and the text printer prints track names, numbers and objects
  ([R-0055](research/R-0055-rider-menu.md), [R-0056](research/R-0056-tour-track-now-playing.md)).
- Pack profile **v17** adds the four screens' content; live play needs it (`content pack --rules
  tests/manifests/content/classic-crawler-tracks-pack.json --out
  local/classic-pal-crawler-tracks-v17.pack`).
- Next: [FRONT-END-1P-CONTINUATION](../tasks/FRONT-END-1P-CONTINUATION.md). See
  [FRONT-END-1P-SETUP](../tasks/FRONT-END-1P-SETUP.md).

Updated 26 September 2026 (FRONT-END-MAIN-MENU): **the app starts at power-on, as the original
does.**
- It shows the Nintendo screen, the title and the main menu (1P, 2P, VS, LEAGUE, OPTIONS), with
  the arrow, its easing and the palette cycle. 1P starts DRAGSTER; the other choices and the idle
  demo show a notice and return to the menu.
- Against three captures of the original, every frame matches: 0 differing pixels, and equal
  menu state and OAM buffer.
- The front end's pieces serve every later screen: a general SNES screen renderer (as the
  reference emulator's PPU draws), the game's text printer, and a frame model of the boot
  ([R-0054](research/R-0054-boot-title-main-menu.md)). Pack profile **v15** adds the front
  end's content; live play needs it (`content pack --rules
  tests/manifests/content/classic-crawler-tracks-pack.json --out
  local/classic-pal-crawler-tracks-v15.pack`).
- Next: [FRONT-END-1P-SETUP](../tasks/FRONT-END-1P-SETUP.md). See
  [FRONT-END-MAIN-MENU](../tasks/FRONT-END-MAIN-MENU.md).

Updated 25 September 2026 (COVERAGE-ROADMAP): **the path from the race engine to the whole game
is mapped and queued.**
- All game code runs in banks `$80-$83`: 636 routines, 219 cited by native code, almost all in
  the race. The main loop runs the main menu, then a game mode: 1P, 2P, VS, LEAGUE, OPTIONS, or,
  after 480 idle frames, the demo (a split-screen race of two computer riders).
- New captures of each mode confirm the table. 136 routines (17,788 bytes) run in none of the
  captures yet.
- The queue makes the native app a complete one-player game first: the main menu, the 1P setup
  screens, the continuation after a race. Then the result icons, the split-screen race, the
  demo, 2P and VS, OPTIONS and LEAGUE, the stunt events, and audio (after a decision).
- Next: [FRONT-END-MAIN-MENU](../tasks/FRONT-END-MAIN-MENU.md). See
  [COVERAGE-ROADMAP](../tasks/COVERAGE-ROADMAP.md).

Updated 25 September 2026 (RESULT-TITLE-GLYPHS): **DOWN+UP and BOO! start in the app, and every
result title follows the original's text printer.**
- The original prints the title through its general text printer. Letters and digits get the
  big font. Any other byte of a track's name gets a small glyph one tile wide: `+`, `!`, `'`,
  and `_` as the small space ([R-0053](research/R-0053-result-title-printer.md)).
- The titles of both tracks match the original's shapes on every captured result frame, and
  match to the pixel on frames in native's palette phase.
- A winner's result now also draws while the opponent is still riding, as the original does.
- The user's stated goal (25 September 2026) is the whole game natively, menus and all. Next is
  a coverage roadmap that inventories the systems not yet recovered and queues their tasks. See
  [RESULT-TITLE-GLYPHS](../tasks/RESULT-TITLE-GLYPHS.md).

Updated 25 September 2026 (NATIVE-READABILITY part 3, task accepted): **no function in
`src/core` exceeds 80 lines, and the presentation reads by concern.**
- `presentation.cpp` is split into the SNES picture's parts, the result screen, the legacy
  DRAGSTER picture, the window and palette timeline, and the HUD. The split is a verified pure
  move of 114 units. `render_classic_race` reads as the picture's layers in order.
- The HUD's text queue, the result map, the rider look and the three runners' `main` are small
  named steps.
- Every address the code cites resolves to a function, member or constant.
- Pictures and behaviour are unchanged: see the task record for the gates, the equivalence
  sweep's pictures, 1,592 HUNTER frames and 243 legacy DRAGSTER frames.
- Next: [RESULT-TITLE-GLYPHS](../tasks/RESULT-TITLE-GLYPHS.md). See
  [NATIVE-READABILITY](../tasks/NATIVE-READABILITY.md).

Updated 25 September 2026 (NATIVE-READABILITY part 2): **the race engine reads by game system,
and no simulation function exceeds 80 lines.**
- `movement.cpp` is split into one file per system, as a verified pure move: 82 of 82 functions
  are unchanged. The systems are the race update, rider motion and pose, the AI, the
  announcements, the X trick, the special tiles, the HUNTER effects, progress, camera, setup
  and state IO.
- Each system was then rewritten as small named steps with named constants. The reward queue
  the user flagged now states its `$81:C238` quirk once (`takes_reward_path`), and the
  announcement events are named by their captions.
- Behaviour is unchanged:
  - every gate passes;
  - the native equivalence sweep is identical over 1.9 million updates, 1,047 restarts from
    saved states and 2,052 pictures;
  - 3,000 damaged states get the same result and refusal on both sides;
  - the 48 HUNTER captures are unchanged.
- All 351 ROM addresses the code cites have a record, and no citation was lost. Part 3
  (presentation and runners, tier 2) is next. See [NATIVE-READABILITY](../tasks/NATIVE-READABILITY.md).

Updated 25 September 2026 (NATIVE-READABILITY part 1): **the recovered C++ now has measurable
style rules, one format, and an index from every cited original address to the native code.**
- `src/core/README.md` "How this code is written": nine rules (game-meaning names, named
  constants, functions of at most 80 lines, intent-first comments with one evidence line, `$`
  only for addresses, ROM quirks in named helpers, guards through one helper, the index, the
  format). The reviewer checklist checks them.
- `src/core` is formatted with `src/core/.clang-format`. The lab-release object code is
  identical, and every gate, 1.23 million updates of native-against-native equivalence and
  both recompare sweeps are unchanged.
- `coverage native-symbols --lookup '$81:C238'` names the native function citing an address,
  with its records. The static map names native symbols beside 216 of its 636 routines.
- 22 functions still exceed 80 lines. Next is part 2 (tier 1): the simulation, starting with
  the reward queue. See [NATIVE-READABILITY](../tasks/NATIVE-READABILITY.md).

Updated 25 September 2026 (NATIVE-READABILITY recorded): **next is a readability pass on the
recovered native code, at the user's request, before RESULT-TITLE-GLYPHS.**
- The user asked whether the reward-queue update in `src/core/movement.cpp` is readable. It is
  not: it reads as an annotated translation of `$81:C238`. D-0003's readability rule was never
  measured, and 15 `src/core` functions exceed 80 lines at `47708b4`.
- [NATIVE-READABILITY](../tasks/NATIVE-READABILITY.md) refactors the recovered code under the
  frozen gates with no behaviour or format change, and adds an address-to-native-symbol index
  so reverse engineering stays a lookup ([D-0003 update](decisions/D-0003-human-readable-native-code.md)).
- A game-systems architecture stays deferred until recovery reaches beyond the race engine.

Updated 25 September 2026 (HUNTER-EFFECTS): **all 36 race tracks now match the original over
their whole compared windows on the sweep captures** (one review input on TWO LOOPS still differs:
[ROLLING-CONTACT](../tasks/ROLLING-CONTACT.md)).
- The HUNTER tour's tag effects are native ([R-0052](research/R-0052-hunter-effects.md)): when
  the hunter touches the player, one of eight timed effects starts, announced at the front of
  the queue.
  - The effects are barf mode, hedgehog speed, power bounce, screen flip, invisible track,
    slow motion, wobble mode and control reversed.
  - Effects 1 and 5 skip whole race updates; effect 2 changes the player's landings; effect 7
    reverses the controls.
  - The others change the picture. All eight are drawn, matching the original's pictures to
    the pixel apart from the declared off-screen arrow.
- The HUNTER opponent is character 20: its own palette and voices.
- Other tracks' states are now `URTRnn06` (916 bytes), and pack profile **v14** adds the effects'
  blink table and the opponent palette. Live play needs pack v14 (`content pack --rules
  tests/manifests/content/classic-crawler-tracks-pack.json --out
  local/classic-pal-crawler-tracks-v14.pack`, then rebuild app-debug).
- Next: [NATIVE-READABILITY](../tasks/NATIVE-READABILITY.md) (user request, entry above), then
  [RESULT-TITLE-GLYPHS](../tasks/RESULT-TITLE-GLYPHS.md) (DOWN+UP and BOO! cannot start in
  the app), then the stunt events.

Updated 25 September 2026 (TILE-PAIRS-8-12-26): **35 of the 36 race tracks now match the
original over their whole compared windows.**
- Native now reproduces the loop (tile flag pair 26), in movement and at the loop's top in
  contact, plus pairs 8, 12 and 28 ([R-0051](research/R-0051-loop-and-tile-pairs.md)).
- LAST ONE, JUMPOVER, DOWN+UP and HIGHROAD, which stopped, match to the end, released and
  with Right held. On DOWN+UP the player rides the loop.
- The loop's words and pair 8's counter are new state, so other tracks' states are now
  `URTRnn05` (854 bytes). No frozen gate moved.
- The one difference left is TWO LOOPS from update 1,252. It is the HUNTER tour's tag effects
  (`$83:CEC9`): when the riders touch, one of eight timed effects starts with its own
  announcement. Next: [HUNTER-EFFECTS](../tasks/HUNTER-EFFECTS.md). Pair 4 stays guarded
  (never reached).

**Live play needs pack v13** (`classic.pal.crawler.tracks.v13`: v12 plus the loop's x steps).
Build it with `content pack --rules tests/manifests/content/classic-crawler-tracks-pack.json
--out local/classic-pal-crawler-tracks-v13.pack`, then rebuild app-debug.

Updated 24 September 2026 (LOCKED-TOURS): **all 45 tracks are now reachable in the laboratory,
and 15 of the 20 locked race tracks match the original.**
- The five locked tours (JUMPER, BOUNDER, RUNNER, SPRINTER, HUNTER) open with a preloaded
  cartridge RAM (`track_reference capture --unlock-tours`; the product never runs original
  code).
- Pack profile **v12** carries their tracks, and each has a native scenario.
- The HUNTER tour runs at AI level 3. Every tour's opponent can turn around on steep slopes
  (`$0C73`), which is new state: other tracks' states are `URTRnn04`.
- Four locked tracks stop at tile pairs 8, 12 or 26, and TWO LOOPS differs at update 1,252
  ([R-0050](research/R-0050-locked-tours.md)).

Live play needs pack v12 (`content pack --rules tests/manifests/content/classic-crawler-tracks-pack.json
--out local/classic-pal-crawler-tracks-v12.pack`, then rebuild app-debug).

Updated 24 September 2026 (RACE-FINISH-BREADTH): **new tracks' finishes and one-run results
match the original.** The finish slowdown skips every third update by a counter that starts
at race setup (`$0304`), not by the absolute frame. Native used the frame, and so was wrong
after the first finish on 9 of the 14 new tracks. Captures on boundaries 0, 1 and 2 (mod 3)
are now exact through both finishes. WARIO PAINT and DRAGRACE (lost), FLAT FUN (won) and the
lap race MEGAJUMP (lost) are exact through the result load to the stable result
([R-0049](research/R-0049-race-finish-breadth.md)). Next: [LOCKED-TOURS](../tasks/LOCKED-TOURS.md), the
25 tracks in the five tours a cold start does not offer.

Updated 24 September 2026 (RACE-GUARDS): **no cold-start race stops at a native guard any
more.** All 16 race tracks match the original over their compared windows. Longer captures
to frame 4,400 of INFINITY, HAIRPIN HILL, MONSTER and HYBRID, released and with Right held,
match to their ends or to a finish. Every track runs 4,000 held-input updates clean in the
app ([R-0048](research/R-0048-race-guards.md)). Two recoveries did it:
- The checkpoint-seen flags are 80 bytes, not 20, so five- and seven-lap races index past the
  twentieth. Other tracks' states are now `URTRnn03` (836 bytes); DRAGSTER and ZOOM ZOO are
  unchanged.
- An inverted AI marker leaves the opponent with every input released and its direction
  neutral, as the port-2 reader sets it before the AI runs.

Next: [RACE-FINISH-BREADTH](../tasks/RACE-FINISH-BREADTH.md). No new track's finish or result
load is compared yet.

Updated 24 September 2026 (SPECIAL-TILE-RESPONSE): **the special tiles no longer stop any
captured race.** The original dispatches a rider's selected tile by flag pair in vertical
contact and again in movement; native now reproduces the lift (flag 25), mud (14), the
corkscrew (10) and the jump-driven tile (16), with every routine their state reaches
([R-0047](research/R-0047-special-tiles.md)). With the release capture:
- SWITCHER, MEGAJUMP, DRAGRACE, PINGPONG and SHORT CUT are exact over their whole windows;
  **13 of the 16 compared races now match the original to the end**.
- CROCK and WARIO PAINT are exact over 3,004 and 3,011 updates.
- Held-input captures put the player on mud and through a whole corkscrew, and match.
- The corkscrew raises the rider's sprite priority: 16 pictures through it match to the pixel.

Other tracks' states are now `URTRnn02` (776 bytes). DRAGSTER and ZOOM ZOO keep their
742-byte layouts, and take the same 776-byte extension (`URDG0002`, `URZZ000C`) only while a
special-tile word is live; no frozen gate moved. **Live play needs pack v11**
(`classic.pal.crawler.tracks.v11`: v10 plus the corkscrew heights). Rebuild it with `content
pack --rules tests/manifests/content/classic-crawler-tracks-pack.json --out
local/classic-pal-crawler-tracks-v11.pack`, then rebuild `app-debug`. Still stopping: MONSTER
and HYBRID at `inverted AI marker is unrecovered`, INFINITY and HAIRPIN HILL at `race checkpoint
index invalid`. Both are [RACE-GUARDS](../tasks/RACE-GUARDS.md), ready next. Tile flag pairs 4,
8, 12, 26 and 28 stay guarded.

Updated 23 September 2026 (TRACK-BREADTH accepted): **TRACK-BREADTH is accepted for the 20
tracks a cold start reaches, and [SPECIAL-TILE-RESPONSE](../tasks/SPECIAL-TILE-RESPONSE.md) is
ready.** After part 3, the user played EAST and LOOPER live and found two faults, both now
fixed:
- LOOPER's track picture did not wrap past the playfield's right edge (#13, R-0046
  observation 17).
- One-run tracks' result screens showed DRAGSTER's name (#14, observation 18). EAST's and FLAT
  FUN's results now match the original's title exactly.

The follow-ups, each its own task, are listed in TRACK-BREADTH's handoff. Weekly usage was 96%
when this was written; the next session starts after the reset on 24 September 08:00Z unless the
user overrides again.

Updated 23 September 2026 (TRACK-BREADTH part 3): **all 16 race tracks a cold
start reaches now start natively, chosen by id** (`frontend run --track NN`), from
pack profile **v10** (`local/classic-pal-crawler-tracks-v10.pack`; a v9 pack is
refused, rebuild with `content pack --rules
tests/manifests/content/classic-crawler-tracks-pack.json`). Each uses its own race
mode, lap count and content from the ROM. LOOPER, FLAT FUN, HYBRID, WARIO PAINT, CROCK and EAST
match the original exactly over about 1,500 released-controller updates, as DRAGSTER
and ZOOM ZOO do; the sampler's playfield edges were recovered on the way. Their pictures
show ZOOM ZOO's residue on consecutive frames of the countdown and GO window (updates
190-300, after the review found and fixed a parity fault on odd-boundary tracks) and at
sampled race frames beyond. Native still stops at a guard on an unrecovered branch on
seven of the other eight (PINGPONG first diverges, at update 1,068), and some exact tracks
reach one later in a race: CROCK at update 1,731 and WARIO PAINT at 1,719 even with the
controller released, HYBRID at 2,053 (`inverted AI marker is unrecovered`). So a new track
can still abort in play; EAST and LOOPER ran 4,000 held-input updates without one. No
new-track finish or result is compared yet ([R-0046](research/R-0046-track-breadth-matrix.md) observations 12-16).

Updated 23 September 2026 (TRACK-BREADTH part 2): **the 20 tracks a cold start
reaches (tours CRAWLER, SHUFFLER, WALKER, HOPPER) are captured through the
original menu and compared with native; four new tracks match exactly** (FLAT
FUN, WARIO PAINT, CROCK, EAST) over about 1,500 updates with the controller
released, beside DRAGSTER and ZOOM ZOO. Seven more match until a native guard on
an unrecovered branch, PINGPONG diverges in `opponent.response_b` after 1,068
updates, two differ only in their lap count (5 and 7), and four are stunt events,
a separate solo mode. The static playfield arms `$80`, `$20` and `$10` are now
observed. See [R-0046](research/R-0046-track-breadth-matrix.md); the task stays in
progress with native selection by track id next.

Updated 23 September 2026 (TRACK-BREADTH part 1): **every track in the ROM is
located and unpacked, and each is started natively; the per-track matrix is in
[R-0046](research/R-0046-track-breadth-matrix.md).** Track *i* is asset `$C2 +
i` in the loader's directory at `$82:B332`; all 45 streams decode, and
`content rnc-inventory` writes the tracked manifest
`tests/manifests/content/track-streams.json`. A general tile producer
reproduces both accepted tracks' per-track pack entries byte for byte, and
`track_geometry` takes four more playfield shapes read from the listing. With
the controller released for 1,200 updates, 16 tracks complete, 28 stop on one of
two named unrecovered branches and one (LITTLE DIPPER) is refused for its
shape. No new track is
compared with the original yet. The task stays in progress
([TRACK-BREADTH](../tasks/TRACK-BREADTH.md)); part 1 was merged as a checkpoint
because the session started at 90% weekly usage on the user's override.

Updated 23 September 2026 (PR-WORKFLOW): **changes now reach `main` only
through pull requests.** Hosted CI runs on pull requests, `main` requires its
checks on an up-to-date branch and accepts merge commits only, and pull request
descriptions follow `.github/pull_request_template.md`. See
[PR-WORKFLOW](../tasks/PR-WORKFLOW.md) and the source-control section of
[the workflow](AGENT_WORKFLOW.md#source-control-and-integration).

Updated 23 September 2026 (integration): **STATIC-CODE-MAP is reviewed and
integrated** (tier 2, approved with should-fix items, all applied; claimed on
the user's explicit override of the reset boundary at 89% weekly usage).
`python3 tools/project.py coverage disassemble` writes an ignored listing of
banks `$80`-`$83`, and `coverage static-map` writes the tracked
[docs/map/static/](map/static/code-banks.md). Every recorded site decodes at
its recorded length, and every byte has one class: 41,778 observed, 36,321
inferred, 92 data, **52,881 unknown (40.4%)**
([R-0045](research/R-0045-static-code-map.md)). That is above D-0008's
one-quarter trigger, so no more heuristics were added; the unknown regions
are targets for dynamic capture. Reproducing the tracked files needs the four
raw coverage captures behind the tracked maps, not only the ROM. Next:
[TRACK-BREADTH](../tasks/TRACK-BREADTH.md), now ready, after the weekly reset on
24 September 08:00Z or on a user override. Acceptance is conditional on the
final-tip CI in `artifacts/static-code-map-integration/closeout.json` in the
main checkout.

Updated 22 September 2026 (integration): **CLASSIC-SPLIT-TIME is reviewed
and integrated**; acceptance is conditional on the final-tip CI and remote
verification recorded in the ignored closeout
`artifacts/classic-split-time-integration/closeout.json` in the main checkout.
The two centred HUD fields are now drawn as the original draws them all race
long, not only at the finish ([R-0044](research/R-0044-classic-split-time.md)):
each rider's crossing time after a lap and at the finish, and at a checkpoint
the other rider has already passed the signed split against the first rider
through, `+M:SS:t` for the player and, by the original's own constant, always
`-` for the opponent; the first rider through a slot draws nothing, and the
cells blank 118 updates after each crossing unless the rider has finished. The
request is made after the lap routine and before the clock ticks, so the split
reads the previous update's clock (509 of 509 splits across 77 captures' WRAM,
against 448 for the current one). Native's engine already kept everything but
the first rider's stored clock, which is presentation history in
`ClassicRaceHudClock`. Measured: **0 differing pixels in both centred bands on
every frame of every set** - the primary's 274 kept frames (whole picture
18,601 -> 2,374, the rest the off-screen arrow), the brake race's 274,
trick-long's 192, 79 plus 60 consecutive primary frames across every kind of
transition, the lap-change, crossing and finish consecutive sets, and 34
consecutive DRAGSTER frames across its checkpoint, blank and finish. No pack
or state change; review tier 2 under D-0008: approved at the first round, report `a7cfc6b` on tag `archive/review/classic-split-time`, five should-fix items applied at `e225bfd`, with the reviewer's own withheld consecutive recaptures on five more originals of both tracks at 0 in both bands. Found on the way:
this host's ASan runtime now hangs before `main` (macOS 27.0, Xcode 26.1.1),
so the two sanitizer presets are unavailable locally and the Linux CI job is
their evidence.

Updated 22 September 2026: **no task is active; CLASSIC-SPLIT-TIME is ready.**
The session that came to dispatch it (Claude Fable 5.1, same provider) sampled
weekly usage at 79% against D-0004's 80% reserve floor, one point short of the
rule's stop, and prepared the task record instead of starting it:
[CLASSIC-SPLIT-TIME](../tasks/CLASSIC-SPLIT-TIME.md) recovers the original's
signed split time, which occupies the two centred HUD cells during the race
(R-0043 measured `-0:00:1` and a signed `0:01:3`; native draws those cells only
at the finish). Claim it after the weekly reset on 24 September 08:00Z or on an
explicit user override. CLASSIC-RACE-HUD's acceptance conditions are verified in
its closeout (final-tip run 35517040155 green on both platforms, remote
matching), and its record now says so.

Updated 22 September 2026 (later): **CLASSIC-SPLIT-TIME is in progress in
`.worktrees/classic-split-time` (claimed about 10:45Z on the user's instruction,
its own session owns its row and closeout); after it, STATIC-CODE-MAP, then
TRACK-BREADTH, under [D-0008](decisions/D-0008-static-map-track-breadth-review-tiers.md).**
The user asked how much of the game is reconstructed and why progress is slow.
Measured against the ROM: the union of the four coverage maps executes 41,778
bytes, 2.0% of the ROM and 32% of the code banks `$80`-`$83`; the records cite
64% of that observed code; two tracks are playable out of **45 RNC streams**
found contiguous in banks `$98`-`$9F` (DRAGSTER first, ZOOM ZOO second). By
features it is on the order of an eighth of the game. The causes recorded in
D-0008 are dynamic-only discovery with no static disassembly, review cost that
does not scale with risk, and tasks scoped to the smallest gain. The user
rejected running original code inside the product for any screen (goal, legal
risk, modding) and adopted the other three remedies: a static annotated code map
of the code banks seeded by the dynamic maps ([STATIC-CODE-MAP](../tasks/STATIC-CODE-MAP.md),
tier 2, ready), a per-track matrix of the whole track set through the shared
engine ([TRACK-BREADTH](../tasks/TRACK-BREADTH.md), planned behind it), and
review tiers by risk (now in AGENTS.md and the workflow). Weekly usage was 84%
at 11:20Z, above the reserve, so this session prepared records only and did no
other work; claim STATIC-CODE-MAP when CLASSIC-SPLIT-TIME is integrated, after
the reset on 24 September 08:00Z or on an explicit user override.

Breadth lines (D-0008): **tracks matched against the original: 2 of 45
inventoried streams accepted by frozen gates; 8 of 45 exact over about 1,500
released-controller updates in the TRACK-BREADTH laboratory comparison, 16 of 45
selectable natively by id (part 3)** (20 of
45 reachable from a cold start and captured, 16 of them races; 45 of 45 decode
and derive their tile content). **Code banks by class (STATIC-CODE-MAP, R-0045): of
131,072 bytes, 41,778 observed, 36,321 inferred, 92 data, 52,881 unknown
(40.4%, above D-0008's one-quarter trigger; the unknown regions are
dynamic-capture targets).**

Updated 20 September 2026: **CLASSIC-RACE-HUD is reviewed and integrated**;
acceptance is conditional on the final-tip CI and remote verification recorded
in the ignored closeout `artifacts/classic-race-hud-integration/closeout.json`
in the main checkout. The shared renderer now draws the original's in-race HUD
instead of an authored bar: the left field (the lap count on a tour race, the
word `race` otherwise, `finish` once the player's laps run out), the corner
clock with the original's tenths, and the two centred finish times, all on the
caption's own BG3 layer, in the caption's font sheet and colour, composed under
the riders with the measured red add and under the channel-6 window members
([R-0043](research/R-0043-classic-race-hud.md)). **No pack change**: every
glyph was already in `presentation.classic.font.v1`, so the profile stays
`classic.pal.crawler.two-tracks.v9`.

Measured against the originals, the HUD rows and the rows the authored bar used
to cover differ by **0 pixels on every frame on which the original is drawing a
race**: 274 kept frames of the M4-16 primary, 274 of the loser race, 192 of
trick-long, the 24 frames of the 9:59.9 time-out hold, DRAGSTER's twelve kept
race frames, and six sets of consecutive frames covering every transition the
change recovers. DRAGSTER 1400-3453 and its whole finish transition are
pixel-identical over the whole picture. Against a build of the base commit the
primary's whole-picture mismatch falls from 938,565 pixels to 18,601, and what
remains is two declared omissions: the off-screen rider arrow, and the
original's **signed split time**, which occupies the two centred cells during
the race and is a separate unrecovered mechanism - this task draws those cells
only at the finish, where they match exactly.

The recovery took **five review rounds, four of them returned**, and the record
keeps why. The HUD itself was right from the first candidate; every return was
about the original's redraw queue, which writes at most one field per update
with the left field first. In order: three fields drawn one picture late
because the queue's update numbers were applied to the drawn state; the
opponent's crossing also holding the clock; DRAGSTER holding on no crossing at
all, because its `$053F` branch enters the clock handler without clearing
`$0D17`; and the finish sequence placed at fixed offsets, which reproduce the
queue only while nothing competes for it. Each was found by a case the evidence
then in hand could not see, and the fourth round confirmed the final rule from
the ROM rather than from the record. Native now follows the queue itself.
Review records: [CLASSIC-RACE-HUD-review](https://github.com/malmazuke/unirally-reconstruction/blob/archive/review/classic-race-hud/tasks/CLASSIC-RACE-HUD-review.md)
on tag `archive/review/classic-race-hud`, and its rounds 2 to 5 on tags
`archive/review/classic-race-hud-2` to `-5` ([round 5, the approval](https://github.com/malmazuke/unirally-reconstruction/blob/archive/review/classic-race-hud-5/tasks/CLASSIC-RACE-HUD-review5.md)).

Updated 20 September 2026: **CI-FAST-PATH is reviewed and integrated**;
acceptance is conditional on the final-tip CI and remote verification recorded
in the ignored closeout `artifacts/ci-fast-path-integration/closeout.json` in
the main checkout. The hosted `synthetic.yml` now classifies each push on a
full-history checkout with the base revision's copy of
`.github/scripts/classify_changes.py`: a push that changes only Markdown under
`docs/` or `tasks/`, root Markdown or `.env.example` (renames, symlinks and
`docs/map/` excluded), whose base commit has a successful completed run of the
workflow, skips every build and test step and finishes green in about 23 s;
everything else takes the full path, now about 3.3 min because the Python
tooling tests run once per job. Measured on the task's own pushes: four
fast-path runs of 23 to 26 s and two full runs of 3.3 to 3.6 min, against 3
to 5 min for every push before. Review: one returned round (three required
corrections: renames, data under `docs/`, and the green-tip claim after a
cancelled run, now a base-run gate) and a confirming re-review with residuals
applied; [CI-FAST-PATH-review](../tasks/CI-FAST-PATH-review.md). No task is
dispatched next.

Updated 20 September 2026: **JEV-JUDGMENT-HARNESS is reviewed and
integrated**; acceptance is conditional on the final-tip CI and remote
verification recorded in the ignored closeout
`artifacts/jev-judgment-harness-integration/closeout.json` in the main
checkout. It adds `python3 tools/project.py judge ping|ask|evidence-lint`: a
standard-library client for TypeSafe's Jev that records every call (request,
response, versioned model, usage, elapsed time, state hash) as an artifact,
with the rule in [D-0007](decisions/D-0007-advisory-jev-judgments.md) that
every answer-derived check is optional and never a gate, that only tracked
records and authored state are sent, and that the key stays in the ignored
`.env`. The synthetic CI has no key and is unchanged. The evidence-lint
sweep over all 46 research records (37 s, 237k input tokens) raised 31
optional flags, one of which reads a real inconsistency in R-0025's status
line; reading those record by record is follow-up work. Review: one returned
round (two required corrections, four should-fix) and a confirming re-review;
[JEV-JUDGMENT-HARNESS-review](../tasks/JEV-JUDGMENT-HARNESS-review.md). The
next task is [CI-FAST-PATH](../tasks/CI-FAST-PATH.md), chosen by the user:
a docs-only fast path that still yields a green run for a records-only tip,
and one Python tooling-test run per CI job.

Updated 17 September 2026: **M4-16 is reviewed and integrated**; acceptance is
conditional on the final-tip CI and remote verification recorded in the ignored
closeout `artifacts/m4-16-integration/closeout.json` in the main checkout
(moved there by REPO-LOCAL-STATE-CLEANUP). If that file is absent, recover the
integration commit with `git log --first-parent main -- tasks/M4-16.md` and its
run with `gh run list --workflow synthetic.yml --commit <commit>`. M4 as a
milestone is **not** accepted and no milestone tag is due. No M4-17.

M4-16 delivers playable native ZOOM ZOO in the desktop app: native
initialization from the authenticated pack through countdown, a three-lap race,
the original's 10:00 time limit, finish, result loading, the result screen and
Race Again, with keyboard and gamepad. Pack `classic.pal.crawler.two-tracks.v9`
(57 entries; v5-v8 are refused) is extracted from the user's ROM and reproduces
byte for byte;
serialized state is `URZZ000B` (742 bytes). Native play does not execute the
original CPU. The work moved from GPT Astra to Claude Opus 5 under
[D-0004](decisions/D-0004-model-and-usage-budget.md) when those credits ran out,
with fresh Opus 5 independent reviewers.

Evidence, on the reviewed implementation `75626f8` and the integration commit:
exact original agreement over the primary start-to-result gate (6,225 states,
757 fresh-process restores, full restart), six complete cases covering both
outcomes, an idle late-start case (801 restores), seven reward and eight trick
probes, the M4-15 race matrix and the M4-12-M4-14 and DRAGSTER historical
matrix; four-preset tests; bootstrap, pack-only, wrong-ROM and bad-pack gates;
denied-execution autonomy; live keyboard play (earlier candidates) and a live
gamepad playtest with rider art (0 pose fallbacks) through a completed race,
result and restart; and frozen visual scenes with rider art, HUD, fade and
the start-line palette cycle matching the original. Review record:
[M4-16-review](../tasks/M4-16-review.md); task record: [M4-16](../tasks/M4-16.md);
research: [R-0035](research/R-0035-zoom-zoo-playable-recovery.md),
[R-0036](research/R-0036-zoom-zoo-rider-objects.md).

DRAGSTER ordinary controls (found in live play on 18 September 2026 local
time: Left or B while riding aborted and Y was ignored): recovered on
`task/dragster-ordinary-controls`, reviewed (`f427098`) and integrated; its
acceptance is conditional on the final-tip CI and the user's live playtest.
DRAGSTER runs the shared race engine from native initialization with its own
track content ([R-0038](research/R-0038-dragster-ordinary-controls.md)); seven
frozen originals (win, loss, tie, reversal, pause, random input) match every
742-byte state with fresh-process restores, and randomized ordinary-input
fuzzing over complete races finds no abort. Shared-engine corrections found on
the way (countdown A/X release, charge latch, roll bounce drive, zero-step roll
completion, the hold/rotation restore bound) keep the ZOOM ZOO gates. The legacy
DRAGSTER path and its historical gates are unchanged; live play needs the
two-track pack. Live play by the user is still due.

DRAGSTER's 10:00 race limit: candidate on `task/dragster-clock-limit`, not yet
reviewed ([R-0039](research/R-0039-dragster-clock-limit.md)). The shared engine
already applied `$81:C73E-C75B` for DRAGSTER, and native equals one original
idle timeline on all 742 declared bytes for frames 1328-31881, including the
9:59.9 hold, both riders finishing at 31534 with the 60000 no-time total and
the start of result loading. What changed is the result screen: it refused to
draw a timed-out result and now writes `MIKE ... NO TIME`, as the original
does. Its result loading is two updates late behind the SPC700 reset handshake,
as ZOOM ZOO's time-out already was, so the case is a declared incomplete
inventory rather than an acceptance freeze.

Declared omissions: audio; the original's decorative objects and captions
(start arrow and ring, hints, on-screen stunt names, opponent finish time,
WINNER caption, off-screen arrows) except both tracks' countdown, GO and
winner windows, recovered in R-0040 and ZOOM-ZOO-WINDOW-EFFECTS, and except the
on-screen stunt names, which the user chose as the next task and
[CLASSIC-STUNT-NAMES](../tasks/CLASSIC-STUNT-NAMES.md) now owns; **the in-race
HUD and both finish times are no longer omitted** - CLASSIC-RACE-HUD draws them
from the original's own fields (R-0043), leaving the off-screen rider arrow and
the signed split time those cells hold during the race; the result pixel
style;
the two-update later result load after a time-out (audio handshake timing).
Other tracks, riders, modes, menus and multiplayer remain outside the product.

Follow-ups from M4-16 findings. [DRAGSTER-PALETTE-CYCLE](../tasks/DRAGSTER-PALETTE-CYCLE.md)
is reviewed and integrated (acceptance conditional on its final-tip CI, closeout
`artifacts/dragster-palette-cycle-integration/closeout.json` in the main
checkout): DRAGSTER's race runs the same NMI palette
cycle ([R-0037](research/R-0037-dragster-race-palette-cycle.md)), visible as colour
0 in the GO and winner windows, which now take the cycled colour 0 when the pack
carries the tables (the two-track pack), matching the original where their
shapes agree; DRAGSTER v1 packs keep the accepted colours.
[DRAGSTER-WINDOW-EFFECTS](../tasks/DRAGSTER-WINDOW-EFFECTS.md) then recovered
when those windows appear and what shape each frame uses
([R-0040](research/R-0040-dragster-window-effects.md)): the original picks one
of a 25-table channel-6 family at `$15:8000` every frame, the countdown driver
`$83:E59C` from `$11C5` and the winner driver `$83:EA19` cycling members 7-24
from the winning rider's finish, and the vblank setup `$80:868E-$80:8699`
publishes the previous frame's choice. Native now draws the countdown digits,
their transitions, the alternating GO letters and the cycling winner banner
with no rider pose-pair gate, and its member equals the original's own pointer
on all 1,922 frames from 1533 to 3454 of a captured race. The family enters the
two-track pack additively as `presentation.effect.classic.window-tables.v1`;
the profile was then `classic.pal.crawler.two-tracks.v8` with 56 entries and
DRAGSTER v1 packs keep the accepted pose-keyed placement, so the accepted v1
contracts and the historical matrix are unchanged. That work is reviewed and
integrated (acceptance conditional on its final-tip CI, closeout
`artifacts/dragster-window-integration/closeout.json` in the main
checkout). The opponent-won banner beyond its
120-update counter is recovered by CLASSIC-PRESENTATION-UNIFICATION below. See
[NEXT_SESSION](../tasks/NEXT_SESSION.md).

[CLASSIC-PRESENTATION-UNIFICATION](../tasks/CLASSIC-PRESENTATION-UNIFICATION.md)
is reviewed and integrated (implementation `f8645d7`, rebased onto `main` as `e59744e`; review `a7f26c4`);
acceptance conditional on the final-tip CI and remote verification recorded in
the ignored closeout `artifacts/unification-integration/closeout.json` in the
main checkout. Presentation now scales the way
the simulation does: one renderer (`render_classic_race`) draws both tracks
from the 742-byte shared race state with track content selected by track from
the pack; the DRAGSTER-only renderer, its held rider art, its approximate fade
and its narrowing call path are gone from live play, and the accepted M3 v1
renderer remains only behind the frozen v1 contracts. The frozen DRAGSTER
contract race turned out to be the continuous-Right replay, whose inputs are
on record, so the shared engine reaches all seven frozen frames within their
thresholds; against the release-3213 originals the rectangle mismatch on the
132 window-effects frames falls from 757,274 to 680,807; the ten M4-16 ZOOM
ZOO scenes are pixel-identical to before; hidden runs of both tracks report 0
rider-pose fallback frames. The opponent-won winner banner is recovered over
its full length (presentation history plus a finish-time derivation, equal to
the original's `$80:868E` pointer on 2,226 of 2,226 frames of the opponent-won
capture). The shared engine reads its eight track-independent tables under
the neutral `physics.*` names. The launcher selects packs by the profile
recorded inside them, never by file name or track, refuses a typed pack of
another profile with the remedy, and reports a stale build before launch; the
four launcher defects from the 18 September playtest are closed.

[DRAGSTER-WINDOW-PAUSE](../tasks/DRAGSTER-WINDOW-PAUSE.md), from the user's
first play on that build, is reviewed and integrated (approved at `1d37c6a`,
report `067dbfe`, after one returned review; the re-review's items applied on
top); acceptance conditional on the final-tip CI in the ignored closeout
`artifacts/window-pause-integration/closeout.json` in the main checkout. The
countdown and winner windows are now
followed update by update the way the original's drivers keep `$11FD`
(`ClassicWindowPointer`): a pause disables the window and freezes the
countdown and the banner, and each rider's finish arms its own banner driver
with a 360-update life, the earliest-armed live driver owning the pointer.
Measured against seven originals captured with the user's ROM (two pauses,
an odd-length pause, two one-frame-apart finishes, the three accepted races),
native equals the original's pointer on every frame. R-0040's 360-frame
bound and "only the winner's driver runs" are corrected there.

[REPO-LOCAL-STATE-CLEANUP](../tasks/REPO-LOCAL-STATE-CLEANUP.md) is done on
`task/repo-local-state-cleanup` (18 September 2026 UTC), reviewed and
integrated; acceptance conditional on its final-tip CI in the ignored closeout
`artifacts/repo-local-state-cleanup-integration/closeout.json` in the main
checkout. Local state now has one home per kind: every worktree's `artifacts/`
moved intact to `local/evidence/<worktree>/` in the main checkout (115 GB,
including the DRAGSTER originals and the M4-16, M4-15 and idle captures the
gates read), the seven closeouts that lived in worktrees moved to
`artifacts/<task>-integration/` beside the earlier ones, and every recorded
gate script was repointed. All 93 registered worktrees and one unregistered
clone were removed, and 105 local branches were deleted after a patch-id audit
(nine with commits not represented on `main`, all review reports, were on
`origin` first, eight of them newly pushed; the two review reports the records cite but `main` lacked,
CLASSIC-PRESENTATION-UNIFICATION's and DRAGSTER-WINDOW-PAUSE's, are now on
`main`). Deleted: verified duplicate captures (7.6 GB), the M4-15 reviewer's
uncited exploration captures (11 GB), build output, caches and run-report
scratch. The repository went from 146 GB to about 117 GB; the DRAGSTER and
M4-16 differential gates pass from the new location. AGENTS.md now carries the
retention rule so a closing task leaves nothing behind in its worktree.

[ZOOM-ZOO-WINDOW-EFFECTS](../tasks/ZOOM-ZOO-WINDOW-EFFECTS.md) (18-19
September 2026 UTC, `task/zoom-zoo-window-effects`) binds the channel-6
window family for ZOOM ZOO, so its countdown digits, GO and winner banner now
draw from the original's own per-frame selection like DRAGSTER's. Read from
the existing M4-16 originals' WRAM, ZOOM ZOO's `$11FD` runs the same members
on the same drivers; the one difference is the countdown transition member,
`5 + $1229`, which race setup latches from the player's start reflection
(member 6 on DRAGSTER, 5 on ZOOM ZOO; derived from the track header). Two
shared corrections came with it: Start held after a pause resume keeps the
window off and the look still (the predicate now reads the engine's
suspended-update clock), and every window member composes after both riders
(the countdown members were drawn under them), which also fixes 75 DRAGSTER
frames where a rider sits under the digits or GO. Native's published
member equals the original's pointer on every frame of seven ZOOM ZOO
originals and the four DRAGSTER pause originals; every pixel the change
touches on the frames with original pictures matches the original. Reviewed
(one returned review, whose blocking finding corrected the compose rule, then
approval at `3e2d30d`, report `255a2c0`) and integrated by fast-forward;
acceptance conditional on the final-tip CI in the ignored closeout
`artifacts/zoom-zoo-window-integration/closeout.json` in the main checkout.

[ZOOM-ZOO-OPPOSING-INPUT](../tasks/ZOOM-ZOO-OPPOSING-INPUT.md) (19 September
2026 UTC, `task/zoom-zoo-opposing-input`) closes the last open follow-up: what
both tracks do when a player asks for both directions of one axis. A SNES pad's
rocker cannot close both contacts and the controller port publishes
`left & !right` and `up & !down`, so on the original such a pair is not "both
directions" but no direction: over a bounded window its whole WRAM is
byte-identical to a released pad's, and `$0311`/`$0313`/`$0315`/`$0319` read
exactly neutral ([R-0041](research/R-0041-opposing-directions.md)). DRAGSTER
already dropped opposing pairs at its call sites; ZOOM ZOO passed them to the
engine and diverged from the first update of such a window, so a keyboard or an
analog stick could drive its race with an input no rocker pad can deliver. The
shared race engine now applies the rocker once, for both tracks, after the
historical recovered-domain guard, which still reads the requested buttons, so
the M4-12 to M4-15 continuation domain and the legacy `update_movement` path are
unchanged. Three ZOOM ZOO originals hold opposing directions over complete
races - Left+Right for a 1,000-update riding window, both axes together, and the
countdown plus the entire result screen including Race Again - and native
matches every 742-byte row and every restore (801, 781 and 757). Two of them
reproduce accepted contracts byte for byte although their delivered timelines
differ: the idle late-start case (`205d1705...`) and the M4-16 primary
(`b4a34af7...`). The accepted M4-16 and DRAGSTER gates keep their recorded restore
counts. Reviewed (approve, then confirm on re-review, no blocking finding in
either round) and integrated by fast-forward; acceptance is conditional on the
final-tip CI and remote verification recorded in the ignored closeout
`artifacts/zoom-zoo-opposing-integration/closeout.json` in the main checkout.
What this game's own branches would do with both bits set stays
unrecovered: a standard rocker pad, through the audited core, cannot present it.
The accepted M3 `update_movement` path keeps the contradictory-direction
precedence recovered in `sample_controller` and is unchanged; the shared engine
never presents that precedence with a pair.

[CLASSIC-STUNT-NAMES](../tasks/CLASSIC-STUNT-NAMES.md) (19-20 September 2026)
draws the original's on-screen captions: the stunt names, the hint sentences and
the winner, draw and loser lines. They are the on-screen half of the reward
queue the engine already runs
([R-0042](research/R-0042-stunt-name-captions.md)): the event under the player's
read cursor indexes sixteen ASCII bytes in the table at `$17:C9F4`, each
character draws as two 8x16 font tiles `$10` apart on BG3 at word `$1800`, and
the queue blanks the caption two ways - a sentence ends by publishing sixteen
spaces, and `$81:BEA8-BEF1` blanks the display when the queue runs dry. The
renderer needs no new state; the pack gains one additive entry, the caption
table, under profile `classic.pal.crawler.two-tracks.v9`, and the font sheet was
already in it. The caption composes over the track, under the channel-6 window
members, and under the riders, where a sprite over a glyph shows
`red = min(31, sprite_red + 13)` - the caption's own attribute colour added
rather than either layer replacing the other. Every caption band measured
matches the original exactly: 274 kept frames of the M4-16 primary at worst
mismatch 0, the DRAGSTER and ZOOM ZOO frames, and eight frames naming a landed
`roll` and `twist`. Approved after three returned rounds, which caught a compose
order the primary had explained away as a declared omission, a throw on three
voice entries that would have ended a race, and an unsound gate shortcut the
primary had added; acceptance is conditional on the final-tip CI and remote
verification in the ignored closeout
`artifacts/classic-stunt-names-integration/closeout.json` in the main checkout.
Declared limits: no capture displays a voice entry (72-87), every measured blend
is on ZOOM ZOO, and the configuration selecting between the caption's two colour
indices is unrecovered.

## Accepted product and evidence

- Milestones M0–M3 are accepted; `m3` remains the latest milestone tag. The
  playable product is the identified PAL one-player CRAWLER/DRAGSTER slice
  through stable winner/loser results. Native play does not execute the original
  CPU. [M3 acceptance](research/R-0017-m3-acceptance.md) and
  [M4-01](../tasks/M4-01.md) define presentation limits.
- Exact user-ROM extraction produces the local 25-entry Classic pack. Audio,
  full rider art, other playable tracks/modes/riders, menus/progression and
  multiplayer remain outside the product. ROM/content/captures remain ignored.
- PAL ROM SHA-256 is
  `a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`;
  the private locator is `local/rom-location.txt`.
- M4-00 through M4-15 are individually accepted; **M4 is not accepted**.
  M4-12 added autonomous native ZOOM ZOO continuation for both riders from
  end-1649 through 1849: 200 updates, exact 395-byte state, Right/neutral input,
  one seed and authenticated static content. [R-0030](research/R-0030-zoom-zoo-native-trial.md).
- M4-13 adds B jump, actual player support loss, landing and recovery in the
  same horizon. Primary B1681–1695 lands at 1745 and matches 104 subsequent
  updates. Two fresh independent cases and two corrected regressions match all
  bytes and restore before/after each full player landing. Captured dynamic
  native inputs remaining: zero. This is still not full-track support and has
  no production ZOOM ZOO frontend/presentation dispatch.
- Fresh Sol/medium [review](../tasks/M4-13-review.md) approved corrected code
  `13f80ff` after two findings in one correction round. App-debug and app-sanitize
  each passed 400 checks with no skips; all M4-12, DRAGSTER, content/replay and
  presentation gates passed. The merged build repeated primary and independent
  cases/restores. Hosted integration CI `34757217687` passed both platforms.
  Hosted Linux is synthetic coverage, not private Linux differential execution.

## M4-14 capability and integration

[R-0033](research/R-0033-sustained-traversal.md) records native continuous Right
from authentic end-1649 through 3299: 1,650 updates / 33 PAL seconds, both riders,
423-byte state (`URZZ0002`), no captured dynamic inputs or original CPU fallback.
Recovery at 2185 leaves 1,114 updates. This is local resumed progress followed
by continued traversal; the rider revisits the same section. It does not prove
monotonic advance, obstacle clearance or full-race completion.

Recovered behavior includes inverted/horizontal probes, steep contact and
landing, tile-selected mode/angle lifecycle, pose/control consumers, and leading
landing reward bookkeeping. The seven additive words per rider preserve old
395-byte `URZZ0001` expectations. Full reference hashes, static inputs and guard
identities are frozen; corrected continuous instruction audit authenticates
30,355,912 instructions and closes the reached future-state inventory.

Fresh Sol/medium [review](../tasks/M4-14-review.md) approved corrected candidate
`e730aaa`. Review found an extra conversion on a full 28-unit landing, missing
binary restore validation and an unclosed audit record. All are resolved; the
failed case remains a regression and a fresh untuned replacement passes 96
restores. A separate material late variation also passes. Primary and cases
retain the complete horizon and frozen recovery requirement.

App-debug and app-sanitize each passed 403 checks, no skips; all accepted
M4-12/M4-13 differential cases/restores and DRAGSTER/content/replay/presentation
gates pass. Denied-ROM/repository execution and negative controls establish the
bounded native runtime's autonomy. M4 remains incomplete; ZOOM ZOO frontend,
presentation/audio, full-track support and private Linux differential execution
are not claimed.

Primary Astra/medium started 13:09 UTC, shared weekly usage 26%; independent
approval around 13:54, usage 35%. These are account-wide observations, not a
controlled model comparison. No reset, purchase or provider switch occurred.
The [task](../tasks/M4-14.md) records the 45-minute reassessment and integration
commands. Final accepted integration is `38c72e8`, 55.61 minutes, shared usage 26% to 37%,
with green exact-tip CI and synchronized private main, recorded in the ignored closeout.
If absent, inspect `38c72e8` in git and `gh run view 34761303975`. Do not create
a second documentation-only CI cycle to transcribe that result.

M4-13 remains accepted at `b288396`; its final closeout took 42.81 minutes and
eight shared usage points (15% to 23%). Historical evidence remains in R-0032
and its task/review. For any future task, retain D-0004/D-0006 budget and automatic
review practices; M4-16 has its own exception and preparation; no M4-17 dispatch is authorized.

## Where to look

- [Task registry](../tasks/README.md), [M4-14](../tasks/M4-14.md) and
  [R-0033](research/R-0033-sustained-traversal.md): actual commits, commands,
  resources, review and next reference experiment.
- [Build and validation](BUILD_AND_VALIDATION.md): implemented CLI and private
  fixture boundaries; [native source guide](../src/core/README.md): source map.
- [Project plan](PROJECT_PLAN.md): longer-term M4–M6 scope; M5/M6 have not started.
- [Workflow](AGENT_WORKFLOW.md): ownership, review and required private-origin sync.

## M4-15 race completion

[R-0034](research/R-0034-zoom-zoo-race-completion.md) and the
[review](../tasks/M4-15-review.md) describe complete native race simulation
from authentic end-1649 through 6724, preserving the original opponent.
Both riders finish (6484/6488); stored times are 9802/9810 centiseconds.
The primary, three corrected regressions and two fresh withheld variations
match every 565-byte state and fresh restores. Debug/sanitizer each pass 405
synthetic checks, and all 28 accepted M4-12–14 differential/restore runs plus
DRAGSTER/content/replay/presentation gates pass. Denied repository/ROM access
and negative controls pass. Exact merged validation and remote/CI passed for `8bc2e71`; closeout evidence is
in ignored `artifacts/m4-15-integration/closeout.json`. Final time was 95.14 minutes,
shared weekly usage 38% to 55%; no reset or purchase. Recover missing evidence
with git and `gh run view 34785933369`.

Recovered coupled behavior includes direction control, checkpoint/lap/time
publication, camera visibility feedback, finish collision poses and continuation.
Review corrected throttle and jump early returns and malformed restore checks.
The finite first-finish announcement-queue invariant is authenticated before
native case evaluation; arbitrary player-queue restores are not covered.
No native race-start initialization, ZOOM ZOO frontend, rendering/audio,
subsequent result-screen loading or universal input coverage is claimed.
Hosted Linux synthetic CI does not establish private Linux differential coverage.
M4 remains incomplete and no milestone tag is due.
