# DRAGSTER-WINDOW-PAUSE - independent review

Reviewer: fresh independent subagent (Claude Opus 5, 1M context), no inherited
conversation. Isolated checkout `.worktrees/window-pause-review`, branch
`review/dragster-window-pause`.

- Candidate reviewed: `e00159a` ("Start the banner driver on a second-finish
  update; bind test loops by reference"), two commits on `main` at `db042ef`.
  The first draft `5b21f38` was read as well; the coordinator moved the
  candidate mid-review and the banner-life rule is identical in both.
- The candidate was not modified. Three temporary mutations were applied to
  `src/core/presentation.cpp` for mutation testing and reverted; `git status`
  was clean afterwards.

**Verdict: return.** One blocking reproducible failure against a freshly
captured original, one should-fix off-by-one in the frame-based fallback that a
second freshly captured original shows, one should-fix test gap and one
should-fix evidence correction. Everything else the candidate claims was
reproduced and holds.

## What was reproduced

Toolchain: `PATH="$PWD/local/toolchain/ninja-1.13.2-darwin-arm64:$PATH"
python3 tools/project.py build --preset lab-debug` (clean `rm -rf build` first),
status=passed. ROM SHA-256 `a1105819...fd66af43b7e0a1fd4e`, core SHA-256
`e59bf88d...8de17699a91b` (both as recorded in R-0040).

| Check | Command | Result |
| --- | --- | --- |
| lab-debug suite | `ctest --test-dir build/lab-debug` | 23/23 passed |
| synthetic suite | `python3 tools/project.py test --suite synthetic --preset lab-debug` | status=passed, 48.5s |
| capture agreement | `verify_pause.py` copied to `artifacts/window-pause-review/verify_pause_review.py` (only `ROOT` and the two output paths changed, so nothing is written into the task worktree), run on the five captures | published 2,267 / 2,319 / 2,119 / 2,544 / 3,200, all 100%; frame-based-with-finish 1,934 / 1,907 / 2,119 / 2,544 / 3,200; frame-based-alone 1,934 / 2,186 / 2,119 / 2,184 / 2,839 - exactly the task's table |
| v1 winner contract | `project.py native presentation-check --manifest tests/manifests/presentation/classic-crawler-dragster-v1.json` with `local/classic-crawler-dragster.pack` | 36 / 697 / 279 / 445 / 653 / 962 / 961, all passed |
| v1 loser contract | `...-loser-v1.json` | 1,073, passed |
| legacy index on the release-3213 sweep | `artifacts/window-pause-review/sweep_index.cpp` built against the candidate's libraries, run on all 2,147 `state-*.bin` of `.worktrees/dragster-palette-review/artifacts/dragster-palette-review/sweep` | byte-identical to the frozen `native-window-index.txt` of DRAGSTER-WINDOW-EFFECTS after normalising its `-1` sentinel to `-`. `dragster_window_table_index` is unchanged on every reachable legacy state. |
| capture repeats | `shasum -a 256 memory.wram` on the a/b pairs | `orig-countdown-pause-a/b` and `orig-banner-pause-a/b` byte-identical, and their `reference.json` too |

The pointer alignment was checked without the candidate's script, reading
`memory.wram` directly (`(frame - 1159) * 131072 + 0x11FD`, member
`(value - 0x8000) / 899`, sentinel `$DB4E`). Frame n's picture is the
end-of-frame n-1 value, as claimed. On `orig-countdown-pause-a` the members are
6 at 1334-1354, 0 at 1355-1383, 6 at 1384-1400, **nothing 1401-1502**, 6 at
1503-1516, 1 at 1517-1545, 6 at 1546-1561, **nothing 1562-1602**, 6 at
1603-1617, 2 at 1618-1646, 6 at 1647-1677, then 3/4 alternating from 1678 with
3 on the even frames, and nothing from 1747.

Independently confirmed from the same series:

- **The pause disables the channel from the opening update through the resume
  update.** `$11C5` reads 203 at the end of 1399, 1400 and every frame to 1501,
  and 202 at the end of 1502: updates 1400-1501 neither decrement nor select,
  and the first free update 1502 is on screen at 1503. The second pause,
  updates 1561-1601, blanks 1562-1602. `zoom_zoo_update_was_paused` (previous or
  updated `pause.selection` non-zero) marks exactly that set.
- **The GO parity is the frame parity across a resume.** End-of-frame `$0300`
  equals `frame & 1` on all of 1340-1620, through both pauses; the two pauses
  are 102 updates (even) and 41 updates (odd), 143 in total, and the GO letters
  still show member 3 on even frames (driver frame odd), the same relation as
  the unpaused capture in R-0040.
- **The countdown word is the one the update read.** Picture 1503 is member 6
  for `$11C5` = 203 at the end of 1501; picture 1516 is 6 for 190 and 1517 is
  member 1 for 189; picture 1677 is 6 for 70 and 1678 is a GO member for 69.
  `previous.movement.countdown` is the right input.
- The engine reproduces both new originals byte for byte: 2,573 of 2,573 native
  rows identical on each, including every paused frame.

## Independent cases

Two originals were captured on this machine with the user's ROM and the audited
core (`dragster_playable_reference`, horizon 3900, each captured twice and
byte-identical; case JSONs and `reference.json` in
`artifacts/window-pause-review/`; the 360 MB WRAM dumps stayed outside the
repository and regenerate from the case JSON in about ten seconds).

1. `banner-pause-odd` - the task's `banner-pause` case with the resume moved
   from 3301 to 3300, so the menu diverts **61** updates instead of 62.
   Opponent 3214, player 3410, loading 3651.
2. `continuous-right` - Right held from 1329, no pause. Player 3213, opponent
   3214: the release-3213 shape the second commit is about.

## Findings

### A. Blocking - the banner's life is 360 driver updates, not 180 steps

`winner_window_life_steps=180` and

```cpp
if(banner_alive_ && input.parity_set) {
    if(banner_life_steps_>=winner_window_life_steps)banner_alive_=false;
    else {++banner_life_steps_;++banner_steps_;}
}
```

count the life in *odd* driver updates ("the 181st odd driver update disables
it"). The original's counters say otherwise. `$0F09`/`$0F07` (16-bit, little
endian at those offsets) are the life counters and they decrement **once per
driver update of either parity**:

- `random-1-a`, opponent 3214: `$0F09` = 359 at the end of 3215 and one less
  each update, 0 at the end of 3574; the last request is update 3574 (picture
  3575, member 7) and update 3575 makes none.
- `reversal-a`, opponent 3325, first driver update even: `$0F09` = 359 at the
  end of both 3326 and 3327 (while `$0F05` is still 0 the driver re-sets the
  life each update), then one less per update, 0 at the end of 3686; picture
  3687 is member 7 and 3688 is blank.
- `orig-banner-pause-a`: `$0F09` is frozen at 335 for updates 3240-3301 and
  resumes at 3302, so diverted updates do not consume the life either.

360 driver updates contain exactly 180 odd ones **only when the diverted
updates between them are even in number**. In `orig-banner-pause-odd-a` the
opponent's 360 driver updates are 3215 plus the 359 that decrement the counter,
skipping the 61 diverted updates 3240-3300, and **181** of those 360 fall on odd
frames. The two readings therefore separate, and the original follows the
counter, not the step count.

Reproduction:

```
python3 artifacts/window-pause-review/verify_pause_review.py <SCRATCH>/orig-banner-pause-odd-a
orig-banner-pause-odd-a: native rows 2573, original rows 2573, identical 2573;
  events {'finish_frames': [3410, 3214], 'loading_frame': 3651, ...}
  paused frames (native): 60 from 3240 to 3299
  published (history): 2303/2318 agree;
    first disagreements [(3637, 7, 8), (3638, 8, 9), (3639, 8, 9), (3640, 9, 10)];
    last [(3650, 14, 15), (3651, 14, 15)]
```

Fifteen frames wrong, every frame from 3637 to result loading, one member ahead.
What the original does there, from `$0F00-$0F0F`: update 3635 is the opponent
driver's 360th, it steps the index to 8 and requests it (picture 3636 = 8);
update 3636 finds `$0F09` = 0, stops for good, and the player's own driver
(finished at 3410, `$0F03`/`$0F07`) starts in the same update with `$0F07` =
`$0167` and index 0, so it requests member **7** (picture 3637 = 7) and 8 from
3638. The candidate keeps one continuous phase and publishes 8, 9, 10 ... .

This is the scenario the task exists for - the user pausing during a race - and
it is the live path: `ClassicRaceHistoryTracker` is what `src/app/frontend.hpp`
draws from. The 15-frame window here is bounded only by result loading starting
241 frames after the player's finish; with the banner expiring earlier relative
to that finish the wrong member persists for most of the banner.

The five accepted captures cannot see this: `countdown-pause` and `primary-a`
have no pause during a banner, `random-1-a` and `reversal-a` have none either,
and `orig-banner-pause-a`'s pause is 62 updates - even - so its 360 driver
updates do contain exactly 180 odd ones and both readings coincide. The one
capture that would have distinguished them differs from the accepted one by a
single frame of resume.

Two further claims fall with it, both stated in R-0040's new section, the task
record, `src/core/README.md` and the code comments:

- "A finish while the banner is alive restarts the life with no gap": it does
  not restart anything. In `orig-banner-pause-a` the player finishes at 3411 and
  `$0F09` keeps counting down (225, 224, 223 ...) with `$0F03`/`$0F07` still
  zero. The second rider's driver is armed and starts only on the update after
  the first driver stops.
- "a finish after expiry starts the driver from the phase it stopped at": the
  new driver starts at index 0, that is member 7. It only looks like a
  continuation because 360 driver updates is exactly 10 member cycles when no
  odd-length pause intervenes, so the old driver happens to die on member 7.
  `orig-banner-pause-odd-a` is the case where it does not, and the original
  visibly steps back from 8 to 7 at 3637.

R-0040's older claim that "Only the winner's driver runs" is also wrong:
`random-1-a` is an opponent-won race in which the player's own driver
(`$0F03`/`$0F07`) runs from 3637, and so is `orig-banner-pause-a`.

**A rule that does fit.** Modelling the two drivers separately, as the ROM does,
reproduces the original's `$11FD` on every scored frame of all seven captures
(`artifacts/window-pause-review/model.py`, driven by the same native rows the
runner uses):

```
countdown-pause: 2267/2267   banner-pause: 2319/2319   primary-a: 2119/2119
random-1-a: 2544/2544        reversal-a: 3200/3200
banner-pause-odd (independent): 2318/2318
continuous-right (independent): 2121/2121
```

The model is: each rider's finish arms that rider's driver, which first runs on
the following non-diverted race update; per driver, `if index == 0: life = 360`,
then `if life == 0` the driver stops for good and the next armed driver runs in
the same update, else `life -= 1`, and on an odd-frame update
`index = 8 if index == 0 else (7 if index == 24 else index + 1)`; the request is
`7 if index == 0 else index`. The countdown dispatch and the pause handling are
unchanged from the candidate.

### B. Should fix - `classic_window_table_index` shows the banner one frame early when the two finishes are one frame apart

`window_table_index_for` guards the driver's first update with

```cpp
if(*shown<first+1U)return std::nullopt;
...
if(driver==latest && !alive_at_latest)return std::nullopt;
```

The first bound admits `shown == first + 1`, whose driver frame is the first
finish update itself, and the second guard only catches it while
`latest == first`. With the two finishes one frame apart, `latest > first` makes
`alive_at_latest` true and the finish update is treated as a driver update.

On the independent `continuous-right` original (player 3213, opponent 3214) both
frame-based columns disagree with the original at frame 3214, which the original
leaves blank (`$11FD` is still the `$DB4E` sentinel at the end of 3213; the
player's driver starts on update 3214 and its member 7 appears at 3215):

```
python3 artifacts/window-pause-review/verify_pause_review.py <SCRATCH>/orig-continuous-right-a
  published (history): 2121/2121 agree
  frame-based with finish: 2120/2121 agree; first disagreements [(3214, None, 7)]
  frame-based alone:       2120/2121 agree; first disagreements [(3214, None, 7)]
```

Standalone repro without a capture, `artifacts/window-pause-review/probe.cpp`
(a restored `ZoomZooState`, `total_times` = `2f + (f & 1)` per rider,
`finish_delay = frame - player_finish`):

```
player 3213, opponent 3214: classic_window_table_index at 3214 = 7  (original: none)
                                                      at 3215 = 7, 3216 = 8, 3217 = 8  (correct)
opponent 3213, player 3214: index at 3214 = 7  (same one-frame error, mirrored)
```

`if(*shown<first+2U)return std::nullopt;` fixes both orders and leaves
`primary-a` (finishes three apart) and every sweep state alone; the legacy
`dragster_window_table_index` is unaffected because it passes the same frame as
first and latest, so the `driver==latest` guard already fires. This is inside
the "exact when no pause diverted a driver update after the first finish"
contract in `presentation.hpp`, so the contract is currently overstated.

### C. Should fix - the tracker itself has no test

No test constructs `ClassicRaceHistoryTracker`; the only other user is
`src/app/frontend.hpp`. Mutation results on the candidate:

| Mutation | `ctest --test-dir build/lab-debug` | capture agreement |
| --- | --- | --- |
| `winner_window_life_steps` 180 -> 179 | 1 test failed | random-1-a 2,302/2,544 |
| GO parity `parity_set?3U:4U` -> `4U:3U` | 1 test failed | countdown-pause 2,198/2,267 |
| delete `if(paused)chosen_.reset();` | **23/23 still passed** | countdown-pause 2,124/2,267, banner-pause 2,257/2,319 |

The task's central rule - a diverted update disables the channel - is guarded
only by an ignored-artifact script that no gate runs. A small synthetic test that
drives `observe_update` over a handful of hand-made updates (a paused stretch,
the publish delay from initialization + 6, the last publish on result-loading
update 1, a banner start and a banner expiry) would close it, and would also
have caught finding A if it encoded the counter rather than the step count.

### D. Should fix - evidence that states the wrong rule

R-0040's "Established later: the banner's life, and windows through a pause",
the task record's "Recovered" list, `src/core/README.md` and the comments around
`winner_window_life_steps` all assert the 180-step life, the restart-on-second-
finish and the phase-continues-after-expiry readings corrected in finding A.
They should be rewritten from the counters (`$0F03`/`$0F05` index,
`$0F07`/`$0F09` life) rather than from the member sequence, with the
`banner-pause-odd` capture as the case that separates the two readings, and
R-0040's earlier "Only the winner's driver runs" corrected.

### E. Advisory - readability and hosted CI

1. `const auto steps=alive_at_latest || driver<latest?odd_frames(...):odd_frames(...);`
   relies on `||` binding tighter than `?:` with no parentheses; three
   `return std::nullopt` guards with overlapping conditions precede it. The
   function is the hardest part of the change to follow and would read better as
   named predicates (D-0003).
2. `const auto driver=*shown-1U;` is computed before the `*shown<first+1U` bound
   check, so it wraps for a hand-made frame-0 state. It is only compared, so the
   result is defined, but the bound check belongs first.
3. The eight-line countdown dispatch ladder is duplicated verbatim in
   `window_table_index_for` and `classic_window_driver_selection`. One helper
   taking the countdown word and the parity would keep the two forms from
   drifting.
4. The candidate wrote `native-rows.txt` and `window-agreement.json` into
   `.worktrees/dragster-ordinary-controls/artifacts/.../originals/primary-a`,
   `random-1-a` and `reversal-a`, another task's accepted-evidence directories.
   They are ignored paths, but the script should write beside its own artifacts.
5. No GCC is installed on this machine, so the `-Werror=range-loop-construct`
   fix could not be checked locally; `-Wall -Wextra -Wpedantic -Wconversion
   -Wsign-conversion -Wshadow -Werror` pass under Apple clang on all five
   presets' worth of sources built here. The new tip needs its own Ubuntu CI run
   before integration; the two new range-for loops now bind by `const auto &`
   and the other new loops (`for(const auto& finish:{...})`,
   `for(const unsigned parity:{0U,1U})`) already do.

## Evidence

`artifacts/window-pause-review/` (ignored) holds the adapted verification
script, the per-capture `*-window-agreement.json` and `*-native-rows.txt` for
all seven captures, the two independent case JSONs and their a/b
`reference.json`, the corrected driver model `model.py`, the hand-made state
probe `probe.cpp`, and `sweep_index.cpp` with its 2,147-row output. The two new
WRAM captures were kept outside the repository and regenerate with
`python3 -m tools.unirally_lab.native.dragster_playable_reference --core CORE
--case artifacts/window-pause-review/<id>.case.json --horizon 3900 --out DIR`.

---

# Re-review at `1d37c6a`

Same reviewer and checkout. Candidate: `1d37c6a` ("Rename a test lambda's local
that shadowed an outer one on GCC"), three commits after the reviewed
`e00159a`: `fe0ddc7` (the two-driver `ClassicWindowPointer`), `e9838ea` (the
evidence corrections) and `1d37c6a` (a GCC `-Wshadow` fix in the new test, a
pure rename). The candidate was not modified; four temporary mutations were
applied for mutation testing and reverted, and the tree was clean afterwards.
Nothing was written to the task worktree.

**Verdict: approve**, with two should-fix items that are not reproducible
failures of the behaviour and one standing precondition (the Ubuntu CI run on
this exact tip).

## Findings A to E from the first round

| Finding | State |
| --- | --- |
| A (blocking) - the 180-step life | **Fixed.** `ClassicWindowPointer` is the two-driver model. |
| B (should fix) - the banner one frame early with finishes one frame apart | **Fixed.** |
| C (should fix) - no test drove the tracker | **Fixed.** |
| D (should fix) - evidence stating the wrong rule | **Fixed.** |
| E (advisory) - readability, stray artifacts, CI | **Fixed**, except that the hosted result on this tip is still pending. |

Each was checked, not taken on report.

### A - the pointer against the originals and against the review model

`verify_pause.py` (my copy, output redirected) on all seven captures with a
build of `1d37c6a`, published member against the original's `$11FD`:

| capture | scored frames | published | frame-based with finish | frame-based alone |
| --- | --- | --- | --- | --- |
| countdown-pause | 2,267 | **2,267** | 1,934 | 1,934 |
| banner-pause | 2,319 | **2,319** | 1,907 | 2,186 |
| banner-pause-odd | 2,318 | **2,318** (was 2,303) | 1,907 | 1,943 |
| continuous-right | 2,121 | **2,121** | 2,121 (was 2,120) | 2,121 (was 2,120) |
| primary-a | 2,119 | **2,119** | 2,119 | 2,119 |
| random-1-a | 2,544 | **2,544** | 2,544 | 2,184 |
| reversal-a | 3,200 | **3,200** | 3,200 | 2,839 |

The frame-based columns still disagree only through a pause and before the
player has finished in an opponent-won race, which is the declared domain.

Beyond the captures, `ClassicWindowPointer` was driven directly
(`artifacts/window-pause-review/ptr_probe.cpp` reads a scripted timeline of
`frame countdown pause finished0 finished1 result_updates`) and compared with
my own model of the drivers over randomly generated races - random pause
windows, both finish orders, both riders finishing on the same update,
one-finisher races and result loading:

```
python3 artifacts/window-pause-review/differential.py [seed]
200/200 synthetic races agree between the review model and ClassicWindowPointer   (seeds 20260918, 7, 99, 12345)
```

800 synthetic races, no disagreement. My model is the one derived independently
from `$0F03`/`$0F05`/`$0F07`/`$0F09` in the first round, so this is two
independent transcriptions of the counters agreeing, not a restatement.

The regenerated captures in the task's artifacts are byte-identical to mine
(`memory.wram` `08ac2cab5f981508` for banner-pause-odd, `7768eb5c1acce358` for
continuous-right), and both of mine repeated byte-identically.

### B - the fallback on hand-made states

`artifacts/window-pause-review/fallback_probe.cpp` builds restored
`ZoomZooState`s (finish times `2f + (f & 1)`, `finish_delay` saturating at 240,
`result_updates` from the loading frame as the engine sets them) and
`fallback_check.py` scores `classic_window_table_index` against the model over
ten configurations, every frame from before the first finish to past loading:

```
OK  continuous-right: player 3213, opponent 3214 (one frame apart): 248/248
OK  reversed: opponent 3213, player 3214: 247/247
OK  same update: both 3214: 247/247
OK  primary-a: player 3211, opponent 3214: 250/250
OK  random-1: opponent 3214, player 3636 (expiry then restart): 255/255
OK  reversal: opponent 3325, player 4292 (even first update): 249/249
OK  player only, opponent times out: player 3300: 261/261
OK  opponent 3214, player exactly at the expiry 3575: 256/256
OK  opponent 3214, player one before the expiry 3574: 257/257
OK  opponent 3214, player long after 4000: 261/261
total disagreements: 0
```

scored from the player's finish onward; the earlier frames of an opponent-won
race are the documented blind spot (a restored state cannot recover the
opponent's finish frame until the player has also finished), and the probe
reports them separately. The finish update itself now selects nothing in both
orders.

`dragster_window_table_index` is still byte-identical to the frozen
DRAGSTER-WINDOW-EFFECTS record on all 2,147 release-3213 sweep states
(`artifacts/window-pause-review/sweep-index-1d37c6a.txt`).

### C - the suite now guards the rule

Mutations of `src/core/presentation.cpp`, each built and run with
`ctest --test-dir build/lab-debug` and the capture comparison, then reverted:

| Mutation | ctest | captures |
| --- | --- | --- |
| `winner_window_life_updates` 360 -> 359 | **1 test failed** | banner-pause-odd 2,303/2,318 |
| drop `chosen_.reset()` on a diverted update | **1 test failed** | countdown-pause 2,124/2,267, banner-pause-odd 2,257/2,318 |
| set the life once instead of while the index is zero | **1 test failed** | (reversal is the case that shows it) |
| drop `if(updated.result_updates>1U)return;` | 23/23 | 7/7 unchanged |

The last is not a gap: with the publish gate already bounded by
`result_updates<=1`, running the drivers during later loading updates changes
no published member, only dead state.

### Gates re-run on this tip

| Gate | Result |
| --- | --- |
| ctest, lab-debug / lab-release / lab-sanitize / app-debug / app-sanitize | 23/23 each |
| `test --suite synthetic` (lab-debug) | passed, 43s |
| v1 winner contract, v1 pack | 36 / 697 / 279 / 445 / 653 / 962 / 961, passed |
| v1 loser contract | 1,073, passed |
| release-3213 sweep, legacy index | identical to the frozen record, 2,147 states |
| `dragster_fuzz_runner` (lab-release), 40 seeds | 382,535 updates, 79 races, 1,242 pause restarts, 7,696 renders, **0 aborts** - the recorded numbers exactly |
| hidden app runs, both tracks, 4,000 updates | rc=0, 0 rider-pose fallback frames each |

Not re-run here: the `stage_c` picture measurements, which need the
before-change binaries in another task's worktree. The index-level check on
`continuous-right`, the same finish shape as release-3213 (3213 and 3214),
agrees with the original on all 2,121 frames in all three columns, so the
picture gate has no room to move at those frames.

## Remaining findings

### F. Should fix - `order_[ordered_++]` is unbounded

```cpp
std::array<std::size_t,2> order_{}, pending_{};
std::size_t ordered_{}, pending_count_{};
...
for(std::size_t i=0;i<pending_count_;++i)order_[ordered_++]=pending_[i];
```

`ordered_` is never bounded and never cleared except by `reset()`. Two arms per
race fill `order_`; a third writes past it. A race is armed twice, so any
caller that keeps one pointer across a race restart overruns the array on the
next race. `artifacts/window-pause-review/restart_probe.cpp` drives a
heap-allocated pointer through N races without `reset()` and dumps its bytes
(`sizeof(ClassicWindowPointer)` is 96; `order_` at +24, `pending_` at +40,
`ordered_` at +56):

```
1 race:  order_ = [0, 1]  pending_ = [1, 0]  ordered_ = 2
2 races: order_ = [0, 1]  pending_ = [1, 1]  ordered_ = 4   <- order_[2], order_[3] wrote pending_
3 races: order_ = [1, 1]  pending_ = [1, 1]  ordered_ = 1   <- order_[4] wrote ordered_ itself
```

The writes are intra-object, so neither ASan nor UBSan reports them; the object
silently corrupts its own fields. It is not reachable through any current
caller: `sdl_main.cpp` and `dragster_fuzz_runner.cpp` both replace
`LivePresentation` whenever the frame rewinds, and the 1,242 pause restarts in
the fuzz gate go through that path. But `ClassicWindowPointer`'s own header
comment says only "Call once for every simulation update" and does not require
a reset, unlike `ClassicRaceHistoryTracker`'s ("Reset it whenever the race state
is replaced"), and the runner's `--window-index` mode does not require
consecutive frames, so a timeline containing a restart would reach it. A bound
on the write (and the same sentence about resetting in the new class's comment)
is one line; I would land it before integration rather than leave new
undefined behaviour in a public class.

### G. Should fix - the record's Gates table is from the pre-rewrite tree

`tasks/DRAGSTER-WINDOW-PAUSE.md` still attributes its Gates table to
`gates.sh` "on the candidate's tree" from before the two-driver rewrite: the
capture row reads "100% on all five captures after the fix" although the
Measurements table above it is now seven, and the hosted-CI row records only
the `-Werror=range-loop-construct` failure on `5b21f38`, not the `-Wshadow`
failure on `e9838ea` that `1d37c6a` exists to fix. The Mistakes section has the
first GCC failure but not the second. Every row except `stage_c` reproduces on
`1d37c6a` (above), so the fix is to say which commit each row was measured on
and to record the second Ubuntu failure, not to re-measure.

### H. Advisory - wrapping arithmetic for hand-made states

`first_odd+winner_window_life_updates-1U` and `(*shown-1U)&1U` in
`window_table_index_for` wrap for a state with a tiny frame number. Defined,
used only in comparisons, and unreachable for real states, the same class as
the note already carried at the end of R-0040.

### I. Advisory - hosted CI is the standing precondition

No GCC on this machine, so the `-Wshadow` fix could not be checked locally;
Apple clang with `-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion
-Wshadow -Werror` builds all five presets clean. Two consecutive Ubuntu-only
warning failures have now come out of this task, so the Ubuntu job on `1d37c6a`
itself must be green before integration, as the project's own closeout rule
requires.

## Evidence added in this round

In `artifacts/window-pause-review/`: `ptr_probe.cpp` and `differential.py` (the
randomised model-versus-implementation test), `fallback_probe.cpp` and
`fallback_check.py` (hand-made restored states), `restart_probe.cpp` (finding
F), `sweep-index-1d37c6a.txt`, and the seven captures' regenerated
`*-window-agreement.json` and `*-native-rows.txt` under `agreement/`.
