# Minimal desktop frontend

Build SDL 3.4.10 from its pinned source archive inside an isolated build tree:

```sh
python3 tools/project.py build --preset app-debug
```

First launch exact-gates a user-supplied supported PAL ROM and atomically writes
the local ignored Classic pack:

```sh
python3 tools/project.py frontend run --rom /path/to/Unirally.sfc
```

Later launches need only the validated pack at
`local/classic-crawler-dragster.pack`:

```sh
python3 tools/project.py frontend run
```

The app starts at power-on, as the original does (R-0054): the Nintendo screen, the title
and the main menu. 1P runs the one-player setup (R-0055, R-0056): PICK YOUR UNI, PICK TOUR, PICK TRACK and NOW
PLAYING, where Race starts the chosen race: any rider against the opponent NOW PLAYING chose
(R-0061), a stunt event included (R-0066). After a race the menus take over when the race's
result load begins: the result screen (R-0057; a lap race's graph, R-0058; a stunt event's trick
tally, R-0067), a press, then PICK TRACK again with the records and the done tracks updated. A tour's completion (its fifth win, or
pad 1 holding exactly Select + X + R as the result is left) shows the medal award and returns to
PICK TOUR (R-0059); a gold medal plays the tour's ending and PICK TOUR then reveals any tours it
opens (R-0062) and HUNTER's plays its pages and credits, then resets (R-0064). In a race started from the menus the
pause menu's second choice ends the race as the original's QUIT does: during the start countdown
it goes back to NOW PLAYING, after it the result shows QUIT and the race counts as lost (R-0060).
A race started with `--track` still restarts from it. The first idle demo runs a two-view
ZOOM ZOO race, then the next idle demo runs one-view track 3; both return to the main menu
(R-0069, R-0070). 2P and VS use two human controllers through their races and
result menus (R-0071). The main menu's Left+A+L+R code opens the WIPE RAM menu, whose
Select+Y+A answer puts the cartridge RAM (and the save file) back to a cold start's (R-0092; with
a pack older than v36 the code shows a notice). LEAGUE and OPTIONS still show a notice and return to the main menu
as it first appeared, so the rider and tour chosen last start over (the original keeps them);
the records are kept. `--track dragster|zoom-zoo|NN` starts directly in that
race instead (not a stunt event, whose result only the menus show).

DRAGSTER plays on the shared race engine (R-0038), whose jump, brake,
reversal, trick and finish tables only the two-track pack carries. With the
25-entry DRAGSTER pack the launcher uses a valid
`local/classic-pal-crawler-tracks-v28.pack` beside it without
opening the ROM, or extracts one when `--rom` is given; without either it
reports a missing prerequisite. The app itself refuses the DRAGSTER-only pack
before gameplay starts.

Pass `--pack PATH` to use another pack location. An existing corrupt or
incompatible pack is rejected and never silently replaced. The application
reports SDL/window/renderer failures as failed launches. A `--report` path
must not alias the ROM, pack, extraction rules or frontend executable; such a
collision exits before reading, extracting, launching or writing the report.
All frontend path operands are resolved once with the operating system's
symlink-aware semantics and those exact paths are used for both the preflight
and later opens. Python does not expand a quoted leading `~`: leave it unquoted
for shell expansion or pass an absolute path.

Keyboard controls are arrows, Z=B, X=Y, A=A, S=X, Q=L, W=R, Enter=Start and
Backspace=Select. Standard gamepad buttons follow the equivalent SNES layout.
The menus and one-player races use port 0; either port can interrupt an idle demo.

DRAGSTER follows the original controls: B jumps, Y brakes, Left rides and
turns back, A, X, L and R act in the air, and Start pauses (RESUME or RESTART
RACE; in a race from the menus RESTART RACE is the original's QUIT, see above). Opposing directions held together on a keyboard are dropped, as a SNES
pad cannot report them. Start on the stable result screen races again.

Audio is intentionally not implemented in M3. Both tracks are drawn by the
same renderer from the shared race state and their own track content: every
packed rider pose is composed from the original's pose tables (R-0036); a pose
outside the packed tables holds that rider's last drawn pose and is counted as
a fallback frame. Presentation never changes canonical simulation state. See
`docs/research/R-0016-minimal-frontend.md` for the exact scheduler, input,
display, dependency and first-launch contracts.


The opt-in option `--native-title-menu-audio` requires a v31 tracks pack and
starts from power-on. Native title/menu music and navigation effects continue
through the HUNTER pages and credits to a warm title/menu restart. Other menu
exits stop this bounded audio producer. The default app remains silent; R-0075
records the exact differential domain and accepted physical controller/listening
check. That live run also reports 265,848 underrun pairs; continuous sound
after every menu exit remains outside the bounded outcome.

Native audio plays in either build. A sound-set upload runs the native IPL and driver at about
16x real time in `app-release` and 6.5x in `app-debug` (AUDIO-UPLOAD-SPEED; 1.2x and 0.8x
before, when the sound processor yielded by exception). Output is a host latency policy, not
original timing: output pair n plays the producer's clock at n / rate seconds, and once it runs
more than four PAL frames behind the frame the game submits, the oldest queued pairs are dropped
until it is two behind (`native_audio_dropped_late_pairs`). The report gives the first drop (the
boot's) and every later one apart, each with its peak level (`native_audio_first_drop_*`,
`native_audio_later_drop*`; peak 0 is silence). A scripted HILL CLIMB run in `app-debug` drops
only the boot's silence.
