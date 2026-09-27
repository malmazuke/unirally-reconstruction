# R-0065 - a tour completed by its fifth win

Status: recovered and compared on `task/fifth-win-completion` (FIFTH-WIN-COMPLETION), 27 September
2026. PAL ROM `a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`, the pinned bsnes
core. Follows R-0059 (the forced completion and the award) and R-0062 (the endings).

Every completion captured before this task was forced: pad 1 holding exactly Select + X + R as a
result is left. The ordinary way is a fifth won race on a tour. This record captures one and
compares native with it.

## The scoring

On the result's exit (`$83:879A`, its pad read), `$83:87B6` reads pad 1. When it is not exactly
Select + X + R, `$83:87BB` branches to `$83:87C0`, which runs the race's win test through a jump
table (`$83:88F7`, by race mode). For a one-run race and a lap race alike the test is
`$83:88D3-88DD`: the player's total must be strictly under the opponent's, so a tie is a loss. A
stunt event's is `$83:88E1`. A win sets Z at `$83:88F1`. Then:

- `$83:87F5-87FB` marks the track done: `$77:1075` + (`$CE` & 0x3F) = 1 (`$83:9EC8`).
- `$83:8801-8813` adds the tour's five done bytes, from `$77:1075` + 5 x `$D0` (`$83:9ED5`), in 8
  bits, a carry cleared before each add, and compares the sum with 5. At 5 or more it branches to
  `$83:881B`. That is the forced completion's path (R-0059): the five done bytes cleared
  (`$83:895B`, the store at `$83:8967`), the medal raised, the award.
- Below 5, `$83:8815` runs the checksums (`$83:90F4`) and the scoring ends.

The done flag set at `$83:87FB` and its clear at `$83:8967` fall in the same frame. The end-of-frame
cartridge RAM never shows the fifth flag.

From the scoring on, the fifth win is the forced completion one frame later. The award, the way
back, the unlock rule, the checksums and PICK TOUR are identical. Only the work RAM word `$5D` (the
win test's address, kept until PICK TOUR's printer overwrites it) and the pad words differ; native
keeps neither.

## Why a capture from power-on needs a cartridge RAM write

- **Five real wins are out of reach.** Each tour's third track is a stunt event (R-0046). A fifth
  win needs a won stunt event, which native cannot run yet (STUNT-EVENTS).
- **A power-on preload does not survive.** Choosing a rider on PICK YOUR UNI clears all fifty done
  bytes and sets `tries` to 3 (`$80:BBD6-BBE5`). Done bytes preloaded at power-on, or written before
  the choice, read 0 afterwards. Native clears them the same way (R-0057).
- **The route taken** is cont-win's DRAGSTER win (R-0058), with CRAWLER's other four done bytes
  (`$77:1076-1079`) set to 1 after frame 2000, during the race. Nothing reads them before the
  scoring. The work RAM is byte for byte the forced-choice capture's up to frame 3798; the inputs
  first differ at 3799 (the review found the first difference there on full work RAM digests).

A replay manifest can now carry such writes (`cartridge_ram_writes`: after a frame, an offset in
cartridge RAM and a byte). The reference worker makes each write after the frame's sample and
before a state saved after that frame. A manifest without writes derives the same script as before.
The front-end runner replays them with `--record-write FRAME OFFSET BYTE`, for the done bytes and
the medals. The injected write is not a CPU store, so an access log shows the bytes read with no
store before them, and the runner's `--records` dump at a write's own frame comes before it.

## Native

Native's scoring (`race_result.cpp`, `score_race`) and completion (`award.cpp`, `complete_tour`)
needed no change for this path. One rule was made exact: native tested that each of the five done
bytes was nonzero, and now adds them in 8 bits and compares the sum with 5, as `$83:8805-8813` does.
The two agree for the 0 and 1 the game writes (`$83:87FB`, `$83:8967`, `$80:BBDC`); they differ
only for cartridge RAM the game did not write. A ROM-free test covers a done byte of 2.

Not changed: the original marks the done track at `$CE` & 0x3F but sums the tour at 5 x `$D0`.
Native takes both from the raced track, which is the same while the track lies in the chosen tour,
as it always does from the menus.

## Evidence

In `local/evidence/fifth-win-completion/`: the research (`decode/fifth-win.md`), the manifest
`fifth-win.json` (6,301 frames), its capture through the project's tools (`project/`: coverage
with the pictures from 3790 to 6300, and the work RAM series from `access capture`), and this
task's copies of `compare.py` and `sram.py`, which pass the manifest's writes to the runner.

The two captures of the manifest agree: the research's own script and the project's tools give the
same work RAM `$0000-$1FFF` on all 6,301 frames and the same 2,511 pictures.

| Comparison | Frames compared | Pictures equal | Differences |
| --- | --- | --- | --- |
| fifth-win, power-on to 6300 | 4,055 | 2,511 of 2,511 | none |
| fifth-win's records (1000, 3803, 3804, 3900, 5600, 6300) | 6 | - | none |
| fifth-win without the writes (main's runner) | from 3805 | - | native opens PICK TRACK, the original the award |

The coverage capture executed `$83:87C0-87D4`, `$83:87F5-8813`, the win test's `$83:88D3-88DD`
and `$83:88F1-88F3` (the forced captures skip them), then the completion from `$83:881B`. The loss
path is not in it. `static-map` reads only the tracked
maps, none of them a front-end capture, so its listing still shows the scoring as unobserved.

## Not covered

- A fifth win on a lap race (the same code and win test, not captured) or on a stunt event
  (`$83:88E1`, stunt points).
- A fifth win that raises a medal to gold (an ending), or one on HUNTER. From `$83:881B` these are
  R-0062's and R-0064's paths.
- PICK TRACK with four done tracks written before it (its markers and cursor search): here the
  write is made during the race.
- A tie, which is a loss by the listing (`$83:88DD`).
