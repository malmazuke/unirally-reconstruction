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
and the main menu. 1P leads to the rider menu PICK YOUR UNI (R-0055), where Y or X goes back to
the main menu; choosing MIKE starts DRAGSTER, the race the one-player screens' defaults lead
to (the tour and track screens are not native yet). 2P, VS, LEAGUE, OPTIONS, the demo and the
other riders are not native yet: choosing one, or leaving the menu idle for the demo, shows a
notice and returns to the main menu. `--track dragster|zoom-zoo|NN` starts directly in that
race instead.

DRAGSTER plays on the shared race engine (R-0038), whose jump, brake,
reversal, trick and finish tables only the two-track pack carries. With the
25-entry DRAGSTER pack the launcher uses a valid
`local/classic-pal-crawler-tracks-v17.pack` beside it without
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
The current recovered slice consumes controller port 0 only.

DRAGSTER follows the original controls: B jumps, Y brakes, Left rides and
turns back, A, X, L and R act in the air, and Start pauses (RESUME or RESTART
RACE). Opposing directions held together on a keyboard are dropped, as a SNES
pad cannot report them. Start on the stable result screen races again.

Audio is intentionally not implemented in M3. Both tracks are drawn by the
same renderer from the shared race state and their own track content: every
packed rider pose is composed from the original's pose tables (R-0036); a pose
outside the packed tables holds that rider's last drawn pose and is counted as
a fallback frame. Presentation never changes canonical simulation state. See
`docs/research/R-0016-minimal-frontend.md` for the exact scheduler, input,
display, dependency and first-launch contracts.
