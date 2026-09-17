# R-0037 — DRAGSTER race palette cycle

Status: **verified finding; native implementation pending** (see
[DRAGSTER-PALETTE-CYCLE](../../tasks/DRAGSTER-PALETTE-CYCLE.md)).

ROM SHA-256 `a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`;
audited bsnes core `e59bf88d4fc922c9fe3b5438e65ff3a6909d24e1628f0f87141c8de17699a91b`.

## Question

The accepted DRAGSTER renderer (`build_race_cgram` in `src/core/presentation.cpp`)
chooses colours 96-111 and colour 0 from two fixed 32-byte arrays,
`racing_cycle` and `finish_cycle`, keyed on DRAGSTER pose indices `$08D5`
(opponent) and `$04FE` (player). R-0015 called them "a semantic race/late-finish
cycle". M4-16 found that in ZOOM ZOO the same colours are animated every video
frame by the race NMI routine `$82:D382-$82:D496` from seventeen 16-word ROM
tables at `$80:82AB` indexed by `$0B84` (R-0035 on the M4-16 branch, "Race
palette cycle"), and that the pose rule was an accidental flip between two of
its sixteen phases. Does DRAGSTER's race run the same routine?

## Evidence

Existing private DRAGSTER access captures, every one of which records the
routine's accesses (instruction PCs `$82:D384-$82:D493`):

| Capture (ignored `artifacts/`) | Scenario | Frames | Routine calls | First `$0B84` write |
| --- | --- | --- | --- | --- |
| `.worktrees/m1-04/.../final/race-ports` | race-crawler-dragster-3000 | 0-2999 | 1,666 over 1334-2999 | 1 at 1334 |
| `.worktrees/m1-04/.../final/access-window` | race-crawler-dragster-3000 | 1500-2100 | 601 (every frame) | 7 at 1500 |
| `.worktrees/m1-04/.../final/w1600` | race-crawler-dragster-3000 | 1595-1605 | 11 | 6 at 1595 |
| `.worktrees/m2-01a-speed/.../speed/primary` | race-crawler-dragster-3000-fields | 1534-2999 | 1,466 | 9 at 1534 |
| `.worktrees/m4-01-loser-result/.../m4-01-winner-result-capture` | dragster-12000-continuous-right | 3423-3679 | 32, ending 3454 | 10 at 3423 |
| `.worktrees/m4-01-loser-result/.../m4-01-loser-result-capture` | dragster-12000-release-3000-3299 | 3528-3800 | 32, ending 3559 | 3 at 3528 |

In the 3,000-frame capture the only writes are: race initialization
`$82:D7F5` clears `$0B84` and `$0B92` at frame 1243, `$82:D861` and `$82:DC0A`
set `$0B92 = 1` at 1243 and 1249, and `$82:D493` advances `$0B84` once per
frame from 1334 (1,666 writes in 1,666 frames). The only other writes,
`$80:D2CB/D2D1/D347` at frame 751, come before race initialization and are
overwritten by it; what that earlier screen is was not examined.

**So DRAGSTER runs the same routine.** It starts at frame 1334 and stops at
result loading (after 3454 in the winner capture, 3559 in the loser capture).
`$0B84` ends frame n at `(n - 1333) & 15`, which predicts every first write
above: 1334→1, 1500→7, 1534→9, 1595→6, 3423→10, 3528→3, across three input
scenarios. Frame n therefore draws table index `(n - 1334) & 15`, one phase
per video frame, the same rule as ZOOM ZOO with DRAGSTER's start frame.

## The fixed arrays are two phases of the table

Composing colours 96-111 and colour 0 from the ROM tables (file offset
`0x02AB`, 544 bytes):

- `racing_cycle` with colour 0 `$7FFF` is exactly **phase 10** (phase 2 has the
  same colours 96-111 but colour 0 `$7E73`).
- `finish_cycle` with colour 0 `$7DAD` is exactly **phase 7** (phase 15 has the
  same colours 96-111 but colour 0 `$001F`).

The accepted DRAGSTER presentation frames fall on exactly those phases:
1600, 2000 and 2400 on phase 10, and 3213 and 3453 on phase 7. The pose rule
happened to separate those frames, so the frozen presentation contract could
not expose it. Colours 96-111 repeat every eight phases (phase 2 equals 10, 7
equals 15), so the two fixed arrays can be right on at most two phases in
eight, and only when the pose rule happens to pick the matching one; on the
other six in eight the checkered line is drawn with the wrong phase.

## Original CGRAM on every frame

Original-only replays (no intervention) of the accepted DRAGSTER winner
scenario `race-crawler-dragster-12000-continuous-right-fields` (frames
1600-3470) and loser scenario `race-crawler-dragster-12000-release-3000-3299-fields`
(1600-3860) on the audited core, each run twice
(`artifacts/dragster-palette-cycle/cgram_probe.py`, ignored). CGRAM was read
from each frame's strict serialized state at offset 206063, located by the
unique match of DRAGSTER's phase-10 colours at frame 1600. Both pairs of runs
are identical.

| Scenario | Racing frames matching `(n-1334)&15` (colours 96-111 and colour 0) | Loading frames matching the frozen rule |
| --- | --- | --- |
| Winner | 1,854 of 1,854 (1600-3453) | 17 of 17 (3454-3470) |
| Loser | 1,959 of 1,959 (1600-3558) | 75 of 302: every frame 3559-3633 |

Result loading starts at 3454 (winner) and 3559 (loser), exactly native's
loading update 1 (225 and 242 updates before the published results at 3678
and 3800), and exactly the routine's last call in the access captures. From
that frame colours 96-111 hold that frame's phase and colour 0 is black
(`$0000`). From loser loading update 76 (3634) the original writes the result
screen's own palettes; those frames are outside this finding. Only 464 of the
winner's 1,871 frames show either of the two fixed arrays.

## Native implementation

`apply_dragster_palette_cycle` draws racing frame n with index `(n-1334)&15`,
and during result loading freezes colours 96-111 at the loading start frame's
index with colour 0 black. It is used by the DRAGSTER race background whenever
the loaded pack carries the tables (`presentation.zoom.race-palette-cycle.v1`,
the same ROM bytes; the two-track pack v7 has them). DRAGSTER v1 packs, which
do not, keep the accepted pose-keyed palette, so the accepted v1 contracts,
fixtures and historical gates are untouched. No new pack version is added:
the two-track pack already carries every DRAGSTER entry and the tables. The
result screen and rider palettes are unchanged.

Verification:
- Accepted v1 presentation checks with the v1 pack: winner
  36/697/279/445/653/962/961 and loser 1,073, unchanged.
- The same frozen cases rendered with pack v7 (cycle active) and scored with
  the checker's own comparison: identical counts, as the phases predict.
- `presentation_tests` pins the rule to the observations above (1333 untouched,
  1600 phase 10, 3453 phase 7, winner loading 3454 and 3528 frozen at phase 8
  with black colour 0, loser loading 3559 at phase 1). Shifting the start frame
  by one fails it.

Not established: a pixel sweep of native DRAGSTER renders at non-frozen frames
(the frozen presentation cases are the only native fixtures with original
scroll values), the palette after loser loading update 75, and pause (native
DRAGSTER has none).
