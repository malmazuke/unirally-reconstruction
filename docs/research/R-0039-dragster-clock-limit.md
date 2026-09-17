# R-0039 - The 10:00 race clock limit on DRAGSTER

Status: implementation candidate on `task/dragster-clock-limit`, not yet
independently reviewed or integrated. Task record:
[DRAGSTER-CLOCK-LIMIT](../../tasks/DRAGSTER-CLOCK-LIMIT.md). It continues
[R-0038](R-0038-dragster-ordinary-controls.md) (DRAGSTER on the shared race
engine) and the ZOOM ZOO clock-limit recovery recorded in
[M4-16](../../tasks/M4-16.md#idle-latch-clock-limit-and-time-out-integrated).

## Identity and tested domain

PAL ROM SHA-256 `a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`;
audited bsnes core SHA-256
`e59bf88d4fc922c9fe3b5438e65ff3a6909d24e1628f0f87141c8de17699a91b`; two-track
pack v7 `b75539a0...`. One-player MIKE/BRONSEN CRAWLER/DRAGSTER, one lap, fresh
scenario, on R-0038's accepted menu path
(`tests/manifests/replay/race-crawler-dragster-3000.json`, Start presses only,
all before frame 1206), race initialization at the end of frame 1328.

One case, `dragster-clock-limit-idle` (frames 1328-32200): the player rides
R-0038's primary inputs to 1999 (a countdown Y brake, Right, held and tapped B
jumps, short and long X rolls, L, and A+R in the air) and then presents nothing
at all for the remaining 30,201 frames. It is the only DRAGSTER case that
reaches the clock limit; the seven accepted DRAGSTER originals are all shorter
than 90 seconds.

## What the original does at 10:00

Captured twice with identical per-frame WRAM, cartridge RAM and video digests
(WRAM `52602f4a...`, SRAM `6d8782df...`, video `3b8bb965...`) and frozen before
any native change.

- **The clock holds and both riders are finished, frame 31534.** `$81:C73E-C75B`
  is the same routine ZOOM ZOO uses. The player is finished with
  `laps_remaining` 1 - one lap short, because it never crossed the line after
  the start - while the opponent finished ordinarily at 3214 with 0:33.58. The
  HUD clock reads `9:59:9` and stays there.
- **The player keeps the start-line crossing digits.** `time_digits` stay
  `[0,0,0,7,0]` (0:00.70, the initial crossing), so they do not describe the
  race; `total_times[0]` stays the 60000 no-time sentinel and every
  `lap_times[0][*]` slot stays 60000. The opponent keeps `lap_times[1][0]`
  3358 and `total_times[1]` 3358.
- **The HUD keeps racing furniture and adds LOSER.** `RACE` stays top-left, the
  held `9:59:9` top-right, and the opponent's finish time `0:33:58` stays low in
  the centre. `LOSER` appears centred while the player takes the loser pose
  under the large white window shape (original frames 31600, 32016-32200).
- **Result loading starts at 31775** (the ordinary 240-update finish delay) and
  the loser result screen settles at 32016, `result_updates` 242, the accepted
  DRAGSTER loser stable count. Mode 0 publishes no lap-graph extrema, as
  R-0038 recorded; `published_totals` are `{60000, 3358}`.
- **The result screen writes NO TIME for the player.** The map is the accepted
  DRAGSTER result - `DRAGSTER` / `COMPLETE`, `PLAYER     TIME`, then four rows -
  with `MIKE` carrying ` NO TIME` in the same eight columns the three `SOMEONE`
  rows use (original frame 32100).

### How DRAGSTER's time-out differs from ZOOM ZOO's

ZOOM ZOO's HUD keeps a lap counter (`2/3`) and its result screen is the
authored two-rider screen with totals, best laps and a lap graph, where the
timed-out player reads `NO TIME` and the opponent its real total. DRAGSTER's
result lists only the player and three empty `SOMEONE` rows with a single TIME
column, so the whole visible difference from an ordinary DRAGSTER result is
that one row. The loser stable count is DRAGSTER's own 242, not ZOOM ZOO's.
Both tracks hold 9:59.9, finish both riders, keep the 60000 sentinel and show
LOSER.

### Result loading is two updates late, as on ZOOM ZOO

M4-16 recorded that ZOOM ZOO's time-out result waits longer in the SPC700 reset
handshake `$82:8088-809A`. DRAGSTER's does the same, by the same two updates:
the mode-0 totals (`$80:F88D-F8A5`) publish at 31884 instead of the ordinary
load update 108 (31882), and the first new result picture is 31885 instead of
31883. Every other DRAGSTER case publishes on time. Audio hardware timing is
outside the contract, so this case is frozen as a declared incomplete
inventory, `tests/manifests/native/dragster-clock-limit-idle-incomplete.inventory.json`,
whose exact race and loading prefix is frames 1328-31881.

## What native did, before and after

**The gameplay engine needed no change.** DRAGSTER runs `update_zoom_zoo`
(R-0038), whose clock-limit arm is gated on `native_initialization`, not on the
track, so it already applied `$81:C73E-C75B`. Native from a fresh initialization
equals the original on all 742 declared bytes for 30,554 consecutive updates,
frames 1328-31881, including the finish at 31534, the 240-update finish delay
and the start of result loading. Across the whole 30,873-update capture exactly
two rows differ, 31882 and 31883, and only in `result.player_total` and
`result.opponent_total`: native publishes `{60000, 3358}` on the ordinary
update while the original waits the two SPC700 updates described above. No
guard fired; no restore differed.

**The result screen did need a change.** The accepted DRAGSTER result
composition required every finished rider's five crossing digits to agree with
its total, which a timed-out player cannot satisfy: 0:00.70 against the 60000
sentinel. The app therefore threw `unsupported Classic result composition`
where the original draws the result. Native now admits that one sentinel and
writes ` NO TIME` in the player row, the same 60000 convention the empty
`SOMEONE` rows and the ZOOM ZOO result already use. An opponent with a
sentinel total has no original evidence and stays rejected, as does a player
total below the sentinel that disagrees with its digits.

## Reproduction

```sh
python3 -m tools.unirally_lab.native.dragster_playable_reference --core local/emulators/bsnes/bsnes/out/bsnes_libretro.dylib --case tests/manifests/native/dragster-clock-limit-idle.case.json --horizon 32200 --out artifacts/FRESH-idle-a --frame-image 31534 --frame-image 31600 --frame-image 32100
python3 -m tools.unirally_lab.native.dragster_playable_reference --core local/emulators/bsnes/bsnes/out/bsnes_libretro.dylib --case tests/manifests/native/dragster-clock-limit-idle.case.json --horizon 32200 --out artifacts/FRESH-idle-b
python3 -m tools.unirally_lab.native.dragster_playable inventory --reference artifacts/FRESH-idle-a --repeat artifacts/FRESH-idle-b --out artifacts/FRESH-idle.inventory.json
python3 -m tools.unirally_lab.native.dragster_playable compare --prefix --reference artifacts/FRESH-idle-a --repeat artifacts/FRESH-idle-b --contract tests/manifests/native/dragster-clock-limit-idle-incomplete.inventory.json --binary build/app-debug/src/core/zoom_zoo_runner --pack local/classic-crawler-two-tracks-v7.pack --out artifacts/FRESH-idle-gate.json
```

Each capture takes about ten minutes and writes about 4 GB of raw WRAM and
cartridge RAM; keep both under ignored `artifacts/`. `freeze` deliberately
refuses this case, because its result load timing is not recovered; `inventory`
records it instead, and `compare --prefix` gates the exact prefix with a
declared bounded restore set (36 boundaries: the initialization edge, every
controller change while the player still rides, the opponent's finish, eight
samples of the 29,000-update rest, the clock limit, the finish delay and the
loading and prefix edges). The ordinary per-controller-change restore set is
not used here because each restore replays the whole 30,000-update tail.

## Not established

- The two-update SPC700 result-loading wait itself. It is observed on both
  tracks' time-outs and on no other case, and audio is excluded from the
  contract.
- DRAGSTER's `LOSER` caption, `RACE` label, held `9:59:9` clock, opponent
  finish time and loser window shape are original observations only. Native's
  accepted DRAGSTER renderer draws none of them; it draws the four abstract
  timer bars and holds the last recovered rider art (R-0038's presentation
  limits, and the window timing follow-up in `tasks/NEXT_SESSION.md`). The
  clock-limit work did not change the DRAGSTER renderer apart from the result
  row.
- Whether any DRAGSTER input other than idling can reach the limit. Only this
  timeline was captured.
