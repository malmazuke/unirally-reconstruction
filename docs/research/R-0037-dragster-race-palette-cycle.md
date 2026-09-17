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
frame from 1334 (1,666 writes in 1,666 frames). The earlier `$80:D2CB/D2D1/D347`
writes at 751 belong to the pre-race screen.

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
not expose it. Every other race frame draws one of those two phases instead of
its own, so the checkered line is wrong on roughly fourteen frames in sixteen.

## Consequences for native

The frame-driven cycle reproduces the accepted CGRAM exactly at all five frozen
race frames, so the winner contract's race counts cannot regress from the
palette. It needs the ROM tables in DRAGSTER's content. The accepted DRAGSTER
pack `classic.pal.crawler.dragster.v1` does not hold them, so this needs an
additive DRAGSTER content version with explicit compatibility behaviour. The
M4-16 two-track pack v7 already carries them as
`presentation.zoom.race-palette-cycle.v1`.

Not established here: DRAGSTER pause behaviour (native DRAGSTER has no pause),
and whether colour 0 is visible anywhere in DRAGSTER.
