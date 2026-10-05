# R-0083 - The riders' look as race state, and riders under the ink

Status: implemented on `task/split-riders-under-ink`
([SPLIT-RIDERS-UNDER-INK](../../tasks/SPLIT-RIDERS-UNDER-INK.md)), 6 October 2026. PAL ROM SHA-256
`a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`, audited bsnes core
`7d5aa1e656b9171524d01b1b22917197d8121cb4`.

This record closes most of what [R-0082](R-0082-split-captions.md) left: the race-time pictures
of two-human split races that still differed in the riders.

Sources: the captures named in R-0082, plus the SPLIT-CAPTIONS review's `mike1` and `p2pause` (main
`local/evidence/split-captions/review/`). The tools are in main
`local/evidence/split-riders-under-ink/`:
- `look_compare.py` compares both riders' look words against the original's work RAM, frame by
  frame, through the front-end runner's new `--look-timeline`;
- `split_compare.py --idle-latch` adds `$0D5B`/`$0D5D` to the compared race words.

Tags: **[L]** listing only, **[C]** confirmed in a capture.

## What the original does

### The look clears an idle latch

The look step (`$82:836D-$82:8926`, called last in the race loop at `$83:CDA6`) drives each rider's
seat and head. When a rider's scripted glance sequence ends, it also clears that rider's idle-cycle
latch: `$82:857F` the player's `$0D5B`, `$82:87AD` the opponent's `$0D5D` [L, R-0036].

[R-0036](R-0036-zoom-zoo-rider-objects.md) showed the clear cannot happen in a one-player race,
where only an idling opponent starts sequences and it never idles long enough mid-race. A human
rider 1 who idles reaches it.

In `vs-idle` (pad 2 idle) rider 1's first sequence starts on 2170 and ends on 2720. That update
clears `$0D5D`. The idle routine re-latches it on 2810, and only then does the second sequence
start. It ends on 3178 and clears the latch again. [C]

### Ink over a rider behind BG1

The split HUD's ink is BG3 priority, in front of BG1's priority tiles too. Its colour math takes
the sub screen, which is the objects alone (`$212D` = 0x10, R-0082). So a rider pixel hidden behind
a BG1 priority tile on the main screen still adds or subtracts its colour under the ink. [C: the
pictures below; register values from R-0082]

## Native

- **The look in race state.** `ZoomZooState::look` replaces `ClassicRaceHistoryTracker`'s own copy.
  - The race update runs `advance_rider_look` after the late sound dispatch and before HUNTER's
    update (`$83:CDA6`).
  - The end of a scripted glance clears that rider's `idle_pose.cycle_latched`.
  - The look tables move into the race content (`presentation.rider.look-tables.v1`), and
    `rider_look.cpp` into the engine library.
  - The picture history draws each update's overlays from the previous state's look: the look
    before that update's step, as before.
- **Serialization.** Two-view states carry the look, and so does the one-view demo, which shares
  the split trailer.
  - The trailer gains a 46-byte look block: ten words a rider, then each head point's presence, x
    and y. The two-view sizes become 830 and 882, the one-view demo's 1,004. Every two-view state
    layout changes size, and the VS and lower-view suffixes keep their order.
  - A league wrapper carries the block before its last byte.
  - The reader refuses a look the step cannot write (`rider_look_state_valid`): heads, targets or
    a sequence target past the 48 head frames, a looking-back flag over 1, a glance timer neither
    resting (-64 to -1) nor below the rider's limit, a sequence number of six or more, an odd
    sequence cursor or end, an end outside the sequence bytes, or a cursor past its end.
  - The one-player race layouts are unchanged. A league wrapper grows by the block whatever its
    race, so a one-player league state goes from 852 to 898 bytes. A restored one-player race
    state starts its look empty, so its overlays rebuild over the next updates, as the picture
    history's did before. R-0036's argument keeps its race words exact.
  - The split demo runner's restore probe, which counts its controller words from the state's
    end, steps over the look block.
- **The ink.** `draw_race_riders` skips a rider pixel behind a BG1 priority tile only where no ink
  covers it.
- **The runner.** `front_end_runner --look-timeline FILE` writes both riders' look words after
  each two-pad race update.

## Evidence

- **Race words and the look.** Twenty captures were compared with `split_compare.py
  --opponent-queue --idle-latch` and `look_compare.py`: R-0079's eleven, R-0081's six, `zzap`,
  `mike1` and `p2pause` (`words-look-527454d.txt`). All have 0 differences on every race frame.
  On main, `vs-idle`'s rider 1 latch differs on 230 frames from 2720.
- **Restores** are byte-identical to the end:
  - `vs-idle` at 2700, 2800 and 4400, across the latch clear;
  - `league-idle` at 9000, a league wrapper;
  - `zzap` at 3000, `twop-plain` at 2290, `league` at 7400 and `p2pause` at 2420.
- **Pictures**, with R-0082's `measure.py` against main ec05ea7 (`measure-527454d/`):

  | Capture | Equal pictures, main | Equal pictures, candidate |
  | --- | ---: | ---: |
  | `vs-idle` | 350 | 796 (every race-time picture) |
  | `league-idle` | 0 of 3 | 3 of 3 |
  | `leaguefin` | 120 | 121 of 121 |
  | `zz2p` | 1,014 | 1,017 |
  | `zzap` | 6,064 | 6,073 of 6,074 |
  | `mike1` | 2,695 | 2,700 of 2,700 |
  | `p2pause` | 2,892 | 2,900 of 2,900 |

  No picture is worse.
- **Tests.** `dragster_race_tests` covers the new state sizes and the look block's refusals.
  `race_pairing_tests` covers the latch clear at a glance's end, and that the latch stays set
  while the glance runs.
- **The review's withheld `idle2`**, a 2P ZOOM ZOO race with pad 2 idle throughout and a pause in
  one of rider 1's glances: words and look are equal on all 5,973 race frames, through nine
  latch clears. 5,974 of 5,974 pictures are equal (main 2,320). Rider 0's clear is never
  reached, as R-0036 argues.

## Not covered

- **One picture.** `zzap` 6317: in the lower view, rider 0's seat shows 22 pixels further right in
  native.
  - Its overlay frame changes every frame there, and the upper view's copy of the same object is
    exact. Only this one lower-view picture differs, which is not a constant lag.
  - Likely the original drops an object tile that straddles the split line (line 112) from the
    lower view, where native clips the composed object by pixel. Not established.
- **A restored one-player state** starts its look empty. This is the one-player layouts' choice,
  and nothing moves while R-0036's argument holds.
