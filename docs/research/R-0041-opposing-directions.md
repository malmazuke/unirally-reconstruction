# R-0041 - opposing directions and the controller port

Status: **verified finding; native implementation in
[ZOOM-ZOO-OPPOSING-INPUT](../../tasks/ZOOM-ZOO-OPPOSING-INPUT.md)**.

ROM SHA-256 `a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`;
audited bsnes core `e59bf88d4fc922c9fe3b5438e65ff3a6909d24e1628f0f87141c8de17699a91b`.

## Question

[R-0038](R-0038-dragster-ordinary-controls.md) found, from one fuzzed DRAGSTER
timeline that held Up with Down, that "the reference core's gamepad reports
`up & !down` and `left & !right`", and made DRAGSTER's runner and app drop such
pairs before the engine saw them. It recorded that "ZOOM ZOO's accepted input
path is unchanged", and
[NEXT_SESSION](../../tasks/NEXT_SESSION.md) kept the open item: hidden runs with
Left+Right exit 0, but no original ZOOM ZOO timeline had been captured with
opposing directions, so nothing established what the original does or whether
native agrees over a race.

What does the original publish when both directions of one axis are asked for,
and does native match it from the countdown to the result?

## The mechanism

**A rocker cannot close both contacts.** A SNES pad's D-pad is one rocker per
axis, and the controller shift register is read one button per clock. The
audited core states the constraint in the hardware's own terms
(`sfc/controller/gamepad/gamepad.cpp`, with the comment "D-pad physically
prevents up+down and left+right from being pressed at the same time"):

| Shift position | Published |
| --- | --- |
| 4 | `up & !down` |
| 5 | `down & !up` |
| 6 | `left & !right` |
| 7 | `right & !left` |

The auto-joypad read lands in `$4218`/`$4219`, which the game copies to `$0311`
and `$0313`, and derives `$0315` (vertical) and `$0319` (horizontal) from
(`src/core/input_timer.hpp`). So a pair reaches neither the game's controller
words nor anything downstream: on the original it is not "both directions", it
is *no* direction.

## Measurements

All three read the ROM through the audited core on the authenticated M4-15
cold-start scenario. Scripts and reports are under
`local/evidence/zoom-zoo-opposing-input/zoom-zoo-opposing-input/`.

**1. The original's memory is identical to a released pad's**
(`dpad_probe.py`, `dpad-probe.json`). Five runs share the primary timeline and
differ only over updates 1650-2400; the per-frame whole-WRAM digests from 1376
are compared against the released-pad run:

| Held over the window | WRAM equal to a released pad | First difference |
| --- | --- | --- |
| nothing | yes | - |
| Left+Right | yes | - |
| Up+Down | yes | - |
| Left | no | 1650 |
| the primary's own steering | no | 1650 |

**2. The published words themselves** (`port_words_probe.py`,
`port-words-probe.json`), over updates 1652-1750, listing every distinct value
each word takes:

| Held | `$0311` | `$0313` | `$0315` vertical | `$0319` horizontal |
| --- | --- | --- | --- | --- |
| nothing | 0 | 0 | 1 (neutral) | 1 (neutral) |
| Left+Right | 0 | 0 | 1 | 1 |
| Up+Down | 0 | 0 | 1 | 1 |
| Left | 0 | 2 | 1 | 0 (left) |
| Down | 0 | 4 | 2 (down) | 1 |

**3. Frozen complete races** (`zoom_zoo_playable freeze`). Two of the three new
cases hold opposing directions over a window whose released-pad counterpart is
already an accepted contract, and the original's 742-byte row stream is
identical to that accepted one although the delivered timeline differs:

| Case | Opposing input | Rows SHA-256 | Same as |
| --- | --- | --- | --- |
| opposing-ride | Left+Right held for updates 1650-2649 | `205d1705...` | the accepted idle late-start case |
| opposing-edges | Left+Right over the countdown 1377-1649, then Left+Right and Up+Down over the whole result screen 6725-7600 | `b4a34af7...` | the accepted M4-16 primary |
| opposing-axes | Left+Right and Up+Down together, updates 1650-2049 | `3d101ea6...` (its own race, player lost) | - |

## What native did, and where the rule belongs

Before this work, `update_zoom_zoo` received whatever the caller passed, and only
DRAGSTER's call sites dropped opposing pairs. A bounded native probe over the
same window (`native_dpad_probe.py`, `native-dpad-probe.json`) showed ZOOM ZOO
diverging from a released pad at the first update of the window for both axes,
while DRAGSTER stayed identical. A keyboard, an analog stick and the fuzz
generator can all ask for both; the original's port cannot deliver them, so the
behaviour native produced for them was not recovered from anything.

The rule is a property of the controller port, not of one track, so the shared
race engine now applies it once, for both tracks, after the historical
recovered-domain guard (which still reads the requested buttons, so the M4-12 to
M4-15 continuation domain is unchanged). The runner, the app and the fuzz runner
pass on what the device reports.

## Domain and limits

- This establishes what the *port* publishes, not what the game's code would do
  with both bits set. That state is unreachable on the console and through the
  audited core, so it remains unrecovered, and native must not invent it.
- The evidence is PAL, one-player, on the CRAWLER scenario's two tracks. The
  three frozen cases hold opposing directions over the countdown, a 1,000-update
  riding window, a 400-update both-axes window and the entire result screen
  including Race Again; they do not cover every update of a race.
- The pause menu's vertical navigation is covered by the ROM-free engine tests
  (Up+Down leaves the selection where it is, Down alone moves it), not by a
  captured original: the accepted pause originals press one direction at a time.
