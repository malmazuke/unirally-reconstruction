# TRACK-BREADTH - every track through the shared engine, measured against the original

## Assignment

- Status: in progress. Part 1 (inventory, per-track producers, playfield shapes, native idle
  matrix) is reviewed and integrated by pull request #9. Part 2 (references for the 20 tracks
  a cold start reaches and the match column) is reviewed and integrated by pull request #10.
  Part 3 (native selection of the cold-start race tracks by id, pack profile v10; tier 1) is
  reviewed and integrated by pull request #11. The live play the acceptance table asks for was
  done by the user after the merge, on EAST and LOOPER with a gamepad rather than the keyboard
  (attempt 30); everything else in the table is met for the 20 reachable tracks (see Review and
  integration). Claimed 23 September 2026 at about 01:05Z on the user's explicit override
  of the reset boundary ("you may work past the 80% reserve until the task is complete or the
  weekly limit is reached"). Prepared 22 September 2026 under
  [D-0008](../docs/decisions/D-0008-static-map-track-breadth-review-tiers.md).
- Milestone: M4 breadth (remaining tracks); the matrix, not any one track, is the outcome
- Coordinator: the preparing session (Claude Fable 5.1, Claude Code desktop, 22 September
  2026 UTC); the claiming session is coordinator, primary and integrator once it claims this record
- Task provider (fixed for all children; record any user-initiated platform change): Anthropic
- Worker/session/runtime/model: Claude Opus 5.5 (`claude-opus-5-5`), Claude Code desktop, one
  session from 01:05Z on 23 September 2026; coordinator, primary and integrator
- Actual model/reasoning effort, routing rationale and frontier escalation question (if any):
  the claiming session's model at default effort. Review tier under D-0008: **tier 2** for
  the inventory, extraction and pack profile; **tier 1** for any change to `src/core`
  arithmetic, the 742-byte state or the gates, which the primary must call out in the
  candidate so the reviewer applies the full D-0006 process to that part.
- Provider quota window/baseline timestamp, used/remaining or unknown, reserve and session
  allowance (D-0004): at preparation, weekly all-models 84% at 2026-09-22T11:20Z. At claim,
  2026-09-23T01:07Z: weekly all-models **90%**, five-hour 10%, resetting 2026-09-24T08:00Z;
  the reserve floor is crossed on the user's instruction. Part 1 as a separately merged
  checkpoint is the response to that: the weekly limit can end the session, and each part
  keeps its own review. The matrix is expected to span sessions; checkpoint each track's row.
- Reviewer (primary automatically spawns fresh model/effort, isolated checkout; no user
  trigger): a fresh Anthropic subagent in a separate checkout at the candidate. Withheld
  case: at least one track the primary did not name as matching, re-captured and compared by
  the reviewer.
- Dependencies and evidence of acceptance: the v9 pack and its rules
  (`tests/manifests/content/classic-crawler-two-tracks-pack.json`), the RNC decoder
  (`tools/unirally_lab/content/rnc.py`, R-0008, R-0021), the shared engine and renderer
  (CLASSIC-PRESENTATION-UNIFICATION: engine content through `zoom_zoo_content` with the
  track's overrides, presentation through `classic_race_presentation_content(pack, track)`),
  the reference menu path (R-0006 finding 8) and `recapture.py`; STATIC-CODE-MAP for the
  track directory and loader reading.
- Base commit: the `main` tip at claim.
- Branch and isolated worktree: `task/track-breadth` in `.worktrees/track-breadth`.
- Owned paths and shared interfaces: `tools/unirally_lab/content/` (inventory and
  extraction), a new pack rules file and profile constant (`classic.pal.all-tracks.v10` or
  as bumped), `src/core/content_pack.cpp` and the loaders for track selection by id,
  `src/app/` for `--track <id>`, tests under `tests/`, replay manifests per track under
  `tests/manifests/replay/`, `docs/research/R-0046-track-breadth-matrix.md`, this record,
  `docs/STATE.md`, `tasks/README.md`, `tasks/NEXT_SESSION.md`. The 742-byte state is
  read-only unless a divergence proves a missing field, which is a tier 1 change.
- Claim/lease/heartbeat/checkpoint location: the matrix in R-0046 is the checkpoint; local
  artifacts under `artifacts/track-breadth/<track>/`, moved to `local/evidence/track-breadth/`
  at integration.
- Session time limit, concurrency allocation and actual spend authorization if relevant: one
  active worker plus the review subagent; 45-minute reassessment intervals; no monetary spend.

## Outcome and boundaries

A tracked matrix with one row per inventoried track stream, each row carrying measured
values for: decodes, extracts into the pack, loads in the engine, runs an idle race for N
updates without abort, has a reference capture, matches the original's 742-byte state for M
updates from race start, and if not, the first divergence and the mechanism it names. Plus
the tooling and pack profile that make every row reproducible, native selection of any
track by id, and no new renderer or loader.

Boundaries: one player, MIKE, controller 1, the Race event, whatever tour and opponent the
original assigns to each track (record them from the NOW PLAYING screen per track; do not
assume Crawler). No menus, riders, modes, multiplayer, audio or per-track presentation
beyond what the shared renderer already draws from content. A track whose per-track
captured entries (see below) have no recovered producer loads but is not accepted, and its
row says so. Recovering a divergence's mechanism is a follow-up task named by the row, not
this task, unless it is small and the primary records why it stayed inside.

## Inputs and prerequisites

**RNC stream inventory (preparation observation, 22 September 2026, ROM SHA-256
`a1105819d48c04d6...`).** Scanning the ROM for the `RNC\x01` header gives exactly 45 method-1
streams, contiguous in fast banks `$98`-`$9F` (slow `$18`-`$1F`), file offsets `0xC0000` to
`0xFBB4C`, unpacked sizes 33,815 to 65,354 bytes, packed 369 to 13,508. The first is
DRAGSTER (`$18:8000`, 33,815 unpacked, the pack's `physics.track.dragster.data`) and the
second ZOOM ZOO (`$18:8183`, 50,665, `zoom.track-data`, whose consumed source ends at
`$18:9B4A` exactly where the third stream begins). Regenerate with:

```python
import re, struct
rom = open(open("local/rom-location.txt").read().strip(), "rb").read()
for m in re.finditer(rb"RNC\x01", rom):
    o = m.start(); unp, pk = struct.unpack(">II", rom[o+4:o+12])
    print(f"{o:06x} ${0x80+(o>>15):02X}:{0x8000+(o&0x7FFF):04X} unpacked {unp} packed {pk}")
```

Whether all 45 are tracks, and which tour and index each belongs to, is what the track
directory in the code map will say; the count is consistent with the shipped game's track
count from general knowledge, which is not evidence. The first tracked deliverable is a
`content rnc-inventory` command and a tracked manifest of the streams (bank, address,
packed and unpacked sizes, digests), carrying no bytes.

**Per-track entries that come from captures.** The v9 rules carry 57 entries: 54 raw, 2
RNC, and 1 `pre_race_matrix` (`zoom.landing-response-matrices`, captured from the original
over frames 1394-1512 of the M4-16 primary). Several `zoom.*` raw entries were located
through ZOOM ZOO's loader rather than through a general directory. For a new track every
entry needs either a producer read from the ROM by track id, or a per-track capture, and
the matrix records which. The two widths already known (DRAGSTER 256, ZOOM ZOO 512,
R-0021) mean the decoder must be driven by the track's own header, not by the two cases.

**Reference capture per track.** R-0006 finding 8 gives the menu path with every choice a
default; a track needs cursor movement on PICK TOUR and PICK TRACK. Determine the inputs
from frame images as that finding did, confirm the NOW PLAYING text (rider, opponent,
track name) per track, freeze a replay manifest per track, and capture consecutive
originals across race start; the sampled-frames rule from CLASSIC-RACE-HUD applies.

## Acceptance

| Criterion | Command or experiment | Expected result | Required artifact |
| --- | --- | --- | --- |
| Inventory | `python3 tools/project.py content rnc-inventory --report ...` | 45 streams with the digests of their unpacked bytes; byte-identical on two runs | tracked stream manifest, report |
| All-tracks pack | `content pack --rules <all-tracks rules>` then `content pack-inspect` | Every stream decodes and extracts; the pack validates; v9 tests and gates unchanged | pack digest, report |
| Loads and runs | For each track, a headless idle race of N updates (N declared before the run, at least a countdown and 600 updates of riding) | Rows: loads yes/no, abort or first fault | `artifacts/track-breadth/<track>/run.json` |
| Reference | One frozen replay manifest per track through the original menu, two fresh processes identical | NOW PLAYING text, tour, opponent recorded per track | manifests, R-0046 |
| Match | Differential comparison of the 742-byte state from race start for M updates (M declared) | Rows: exact updates, first divergence address and field, named mechanism | R-0046 matrix, comparison reports |
| Native selection | `frontend run --track <id>` for at least two tracks beyond the current pair | Starts, draws, plays with keyboard | live report |
| Declared | The matrix names every track that loads but is not accepted and why | No row is blank | R-0046 |
| Breadth line | `docs/STATE.md` | "tracks matched / inventoried" updated | STATE |
| Review | Tier 2 one round, tier 1 full process for any engine change | Reviewer's withheld track re-captured and compared | review record |

## Capability and coverage checkpoint

- Native capability delivered / still missing: every track's content is located and
  derivable from the ROM (inventory, tile producer); the engine takes four more playfield shapes;
  16 of 45 tracks complete 1,200 idle updates natively. Missing: native selection by track id in
  the pack and app, each track's own scenario, the two guarded branches, the `$04` shape.
- Frozen exact-match interval, field set and reference/seed identity: none new yet (DRAGSTER and
  ZOOM ZOO keep theirs).
- Dynamic captured inputs still consumed (must be zero for autonomy): the landing-response
  matrices, as before; the per-track tile content now has a ROM producer.
- Relevant branches/transitions exercised, including independent variations: race start,
  countdown and 930 updates of riding with the controller released, native only.
- First divergence and cheapest next discriminating experiment: R-0046, "Next experiments".
- Trial-wide usage baseline/current, reserve, reset authorization/outcome or none: 90% weekly at
  claim, user override of the reserve; no reset.

## Evidence and attempts

| Attempt | Hypothesis | Experiment | Observation | Next decision |
| --- | --- | --- | --- | --- |
| 0 (preparation) | The ROM's track set is a contiguous run of RNC streams | Header scan above | 45 streams in `$98`-`$9F`; DRAGSTER and ZOOM ZOO are the first two | Inventory command first; directory from the static map |
| 1 (01:10Z) | The loader selects the stream by track index | Listing of `$82:E12B-E152`; the asset directory at `$82:B332` dumped for `$B8`-`$FF` | Track *i* is asset `$C2 + i` (index from SRAM `$77:074A`); `$C2`-`$EE` point in order at the 45 streams, the only compressed run | Decode all 45 through the directory |
| 2 (01:12Z) | Every stream decodes with the existing port | `rnc.decompress` on all 45 (0.6 s) | All decode to their header length and consume exactly their directory length; 0 and 1 equal the pack's digests | Header fields |
| 3 (01:14Z) | The header's byte 13 is the playfield shape | Headers of all 45; listing of `$81:A304-A51B`, the four unknown arms decoded by hand | Six values; byte 13 = columns / 4; seven arms, each 16,384 cells; `$04` also sets `$0FF7` | Generalize the tile producer |
| 4 (01:20Z) | The R-0008 tile-set rule is general | `tracks.tile_content` against the pack | All 8 per-track entries of both accepted tracks byte for byte | Run every track natively |
| 5 (01:30Z) | The engine runs other tracks with their own content | Loose `--content-dir` runs | Refused: the loose path lacks the trick tables | Lab-only `--track-override` on a pack |
| 6 (01:35Z) | Override is neutral | ZOOM ZOO's own bytes through `--track-override`, 1,200 idle updates | Identical to the plain pack run | Matrix |
| 7 (01:40Z) | - | Idle matrix, N = 1,200 declared before the run | 13 complete, 13 special-tile guard, 1 edge guard, 18 refused by shape in their first update | Add the four non-`$04` arms |
| 8 (01:45Z) | The static arms let the refused tracks run | Same matrix after `track_geometry` gains `$80/$20/$10/$08` | 16 complete, 19 special-tile, 9 edge, 1 refused in its first update (track 37) | Commit, gates, review |
| 9 (01:18-01:50Z) | DRAGSTER and ZOOM ZOO cannot move | `artifacts/track-breadth/gates.sh` on code commit `1f554fb`: three presets and ctest, synthetic, both v1 contracts, hidden runs, fuzz, and **all eleven differential gates run** (the change reaches the gate binary, so `gate_identity` could not cite them) | ctest 23/23 on lab-debug, lab-release and app-debug; synthetic passed; v1 winner and loser passed; hidden DRAGSTER and ZOOM ZOO 0 fallback frames; fuzz 40 seeds, 79 races, 0 aborts; all eleven gates passed with the same row digests and restore counts as the `6e0fad6` run. `lab-sanitize` and `app-sanitize` unavailable on this host | Records, pull request, review |
| 11 (02:20Z, part 2) | A generic menu path reaches any Crawler track | `track_reference capture` for ZOOM ZOO, compared with the M4-16 idle original | Byte-identical WRAM and SRAM over frames 1207-2649 | Boundary rule |
| 12 (02:25Z) | The boundary is the first frame with countdown 270 | Same capture | Wrong: 1,291; the countdown reads 270 for 85 frames. The M4-16 rule (the frame before `$0FF1` first advances) gives 1,328 and 1,376 | Compare |
| 13 (02:30Z) | Native matches the accepted tracks from the found boundary | `explore` on DRAGSTER (DRAGSTER scenario) and ZOOM ZOO | Exact, 1,322 and 1,274 rows to horizon frame 2,650 | New tracks |
| 14 (02:35Z) | Crawler 2-4 are ordinary races | NOW PLAYING pictures | BOWL is a stunt event; SWITCHER one run; MONSTER 3 laps. SWITCHER exact 384 and MONSTER exact 361 updates, then the special-tile guard; MONSTER's memory holds the `$20` arm's constants | Other tours |
| 15 (02:40Z) | Other tours are locked on a cold start | PICK TOUR picture, one Down | Four tours offered (CRAWLER, SHUFFLER, WALKER, HOPPER); SHUFFLER lists tracks 10-14 | Sweep all 20 |
| 16 (02:45-03:00Z) | - | `sweep` over the 20, each captured twice | First run compared the menu position instead of `$77:074A` on rows 1-3 (bug, fixed, run discarded as `sweep-position-bug`). Second run: all repeats identical; indices 10 x row + position; 6 exact over about 1,500 updates, 7 exact until a native guard, PINGPONG 1,068 then `opponent.response_b`, 2 lap-count only, 4 stunt | R-0046 part 2 |
| 17 (03:05Z) | The landing matrices vary by track | WRAM `$0572-$0B59` at each boundary against the pack entry | Identical on all 20 | Records |
| 18 (03:10Z, part 3) | The scenery comes from the track index | Listing of `$82:DC20-DD84`, asset directory `$70`-`$B5` | s = track mod 14; BG2 tiles `$70+s`, map `$82+s`, palette row `$93+s`, class block by `$82:DC12+s`; the generated pieces equal both accepted tracks' v9 entries | Pack v10 |
| 19 (03:20Z) | The v9 per-track entries come from general producers | `tracks.track_pack_entries`/`scenery_pack_entries` for tracks 0 and 1 against the v9 rules | All 14 source lists and digests equal | Generate v10 (147 entries) |
| 20 (03:40Z) | A track index replaces the two-track enum | `ClassicRaceTrack{index}`, scenario table, `URTR<NN>01`, `classic_race_content`, per-track presentation, runner and app | Three presets build, ctest 23/23, new native checks pass | Per-track comparison |
| 21 (03:50Z) | Each track's own scenario matches | `track_reference recompare --per-track` on the part 2 captures, v10 pack, no override | FLAT FUN, WARIO PAINT, CROCK, EAST exact; HAIRPIN HILL 302 rows exact to its guard; INFINITY 379 rows then `race checkpoint index invalid`; the rest unchanged | Pictures |
| 22 (03:55Z) | New tracks draw as the original | Native frames against original frames, six race frames each | 0-36 differing pixels on FLAT FUN, WARIO PAINT, CROCK; 0-72 on the ZOOM ZOO baseline | Gates |
| 23 (04:05Z) | New tracks play under input | Gate script's hidden app runs, 4,000 updates with a held button, tracks 13 and 30 | Both abort at `unrecovered coarse-grid edge branch` | Recover the edge inside this part (small, and live play needs it) |
| 24 (04:15Z) | The listing's edge paths are the original's | `$81:8A2C-8A3B` (negative y to cell 0, 0) and `$81:8A60-8A99` (last column wraps to column 0), then recompare | LOOPER and HYBRID (opponent in the last column) exact to the end, 1,484 and 1,515; DRAGRACE (opponent y `$FFFB`) 1,353, 900 past its stop | Gates, review |
| 25 (03:07-03:47Z) | Nothing accepted moves | `artifacts/track-breadth-3/gates.sh` on `fa62939`, v10 pack: presets and ctest, synthetic, v1 contracts, hidden app runs (DRAGSTER, ZOOM ZOO, 13, 30), fuzz, all eleven differential gates, `rnc-inventory --expect` | ctest 23/23 x3; synthetic, v1 winner and loser passed; hidden DRAGSTER, ZOOM ZOO and FLAT FUN 0 fallback frames over 4,000 held-input updates; **WARIO PAINT aborts at the special-tile guard under held input** (declared, the next follow-up); fuzz 0 aborts; all eleven gates passed with the same row digests and restore counts as `1f554fb`; inventory passed. Sanitizers unavailable on this host | Records, review |
| 26 (04:30Z, review round 1) | - | Tier 1 review of `775e7ba` returned: R1 the GO letters swap on odd-boundary tracks (the window drivers took `$0300`'s parity from the absolute frame); R2 the records overclaimed | Confirmed; the review's six sampled frames had missed the window | Fix R1 at the three parity sites, correct the records |
| 27 (04:45Z) | `$0300` counts from the boundary | Parity from `frame - initialization frame` in the countdown driver, the restored-state selection and the opponent-finish inference; 111 consecutive frames of updates 190-300 on CROCK, LOOPER, EAST, FLAT FUN, ZOOM ZOO | All five show ZOOM ZOO's profile (36 or 0 pixels, 470 at update 272 on all); recompare unchanged | Gates, re-review |
| 28 (05:15Z, re-review) | - | Tier 1 re-review of `5bab77e`: R1 confirmed fixed on MONSTER, PINGPONG, HAIRPIN HILL, SHORT CUT and DRAGSTER (the old code reproduced the fault), parity logic correct on every path, gates complete; **returned** R3: the suggested acceptance play named CROCK, which aborts at update 1,731 with the controller released | Records only: EAST and LOOPER named instead; the later stops (CROCK 1,731, WARIO PAINT 1,719, HYBRID 2,053 on `inverted AI marker is unrecovered`) and the update-272 residue recorded | Third round |
| 29 (05:40Z, rounds 3-4) | - | Round 3 (fresh Opus 5.5) on `93c9a01`: R3 fixed, EAST and LOOPER confirmed clean over 4,000 held-input updates, the stops confirmed, `gate_identity` passed; **returned** R4 (HYBRID's guard on no follow-up list). Round 4 on `ccef821`: **approved**, three minor advisories applied before merge | Merge |
| 30 (06:55Z, after the merge) | New tracks play live | The user ran `frontend run --track 33` and `--track 10` (app-debug at `a9e80f2`, v10 pack) with an Xbox Series X controller on port 0; the main checkout's app build was stale at first and was rebuilt | EAST: 4,736 updates, one finished race, stable result reached, Race Again from it, 0 rider-pose fallback frames, no guard stop; LOOPER: 4,330 updates, the same outcome. Gamepad only (no mapped keys); buttons A, B, X, Start, shoulders and the D-pad | Live criterion met with the gamepad. The user reported two faults: (1) the result screen of both shows DRAGSTER's name; a one-run race reuses DRAGSTER's result assets, including its captured base VRAM (R-0046 observation 13's result-by-mode hypothesis), so the result screen's track name is not recovered; (2) on LOOPER, near the end of a race, the track seemed to vanish below the rider at moments; cause unknown (a first suspect is BG1 row streaming on the 64 x 256 playfield, where the original's map fetch reads `$0D4D`, which native does not model); no frame of a late LOOPER race has been compared |
| 31 (07:20Z, user report) | LOOPER's vanishing track is a missing BG1 wrap | Held-Right capture of LOOPER (native exact on all 1,284 rows), native render at frame 2,617 against the original, then the fix (BG1 world x masked with the playfield mask, `$81:AD05`) and 200 consecutive frames | Before: 1,718 px missing in columns 176-255; after: 0-108 px (the declared rider arrow) on 198 frames, 549 on two where native shows a hint caption the original does not | Tier 2 pull request #13, approved at the first round (withheld HYBRID: up to 3,486 px before, 0 after); advisories applied (the caption timing repeats every 60 updates and is open; `explore` honours a held input) |
| 10 (01:55Z) | The name table follows the track index | Relative-text search, then the table at `$83:9FFA` | 45 names in lowercase ASCII, then five `unavailable` and nine tour names; names 0 and 1 agree with the verified indices | R-0046 observation 5 (static) |

## Handoff

- Current base/head commit and uncommitted state: part 1 is merged (see the pull request named
  in `artifacts/track-breadth-part1-integration/closeout.json` in the main checkout); the task
  continues from the `main` tip in a new `task/track-breadth-2` branch and worktree.
- Verified findings: [R-0046](../docs/research/R-0046-track-breadth-matrix.md) observations
  1-4. The tracked manifest is `tests/manifests/content/track-streams.json`.
- Current hypothesis and failed approaches: nine tours of five tracks, index = 5 x tour +
  position (R-0046, unverified). A loose `--content-dir` cannot start a complete race (no trick
  tables), which is why the lab override sits on a pack.
- Commands executed, outcomes and report hashes: `content rnc-inventory` twice (identical,
  `--expect` passes); `content track-idle-matrix --updates 1200` twice (identical rows);
  `artifacts/track-breadth/gates.sh` on the code commit (see Review and integration).
- Unavailable/skipped checks: `lab-sanitize` and `app-sanitize` (the host's ASan runtime hangs
  before `main` since the macOS 27 update; recorded unavailable, the hosted Linux job covers
  them). No reference capture of a new track yet.
- Gate addition from the part 1 review: run `content rnc-inventory --expect
  tests/manifests/content/track-streams.json` in every gate run of this task (CI has no ROM,
  so nothing hosted checks the manifest's values), and the runner's option check
  (`zoom_zoo_runner.cpp:38` accepts any four options) is tightened with the next change that
  already reruns the differential gates.
- Part 2: `tools/unirally_lab/native/track_reference.py` (lab only) and its tests (the menu
  path only; no ROM-free test covers the boundary finder or the comparison); the sweep
  and captures are in `local/evidence/track-breadth/track-breadth-2/` after integration.
- Part 3: native selection by id is in (R-0046 observations 12-15). Evidence in
  `local/evidence/track-breadth/track-breadth-3/` after integration (recompare, pictures,
  gates). The sampler edges were recovered inside this part (small; live play of the new
  tracks aborted without them). Next: the special-tile response, HYBRID's `inverted AI
  marker is unrecovered` guard (update 2,053), INFINITY's checkpoint guard, PINGPONG's
  `opponent.response_b`, then the locked tours and a captured finish and result
  on a new track (the result timing and assets follow the race mode as a hypothesis).
- Former next experiment (after part 2, done in part 3): make the race scenario data (mode, laps,
  initialization frame per track, from R-0046 observations 7-8), add the reachable tracks'
  per-track entries to a new pack profile and select a track by id in the runner and the app;
  then re-run `track_reference sweep` on the native scenario per track. The part 1 plan below
  is done.
- Former next experiment (part 1, done in part 2): capture track 2 (Crawler, Down twice on PICK TRACK from the
  ZOOM ZOO manifest's path) with the controller released and consecutive frames across race
  start, record NOW PLAYING, and write a generic 742-byte projection so the match column can
  be measured; then track 3 and 4. In parallel, a watch capture on track 2 names the tile flag
  behind its special-tile stop at update 7.
- Remaining dependencies: none outside the project. Tours other than Crawler may be locked on a
  cold start; if so, record how the original unlocks them before choosing a capture method.
- Runtime needs (network, build time, fixtures, memory): ROM, the pinned core for captures, a
  lab-debug build (seconds), about 25 minutes for the gate script.
- Aggregate parent/child time, provider usage before/after (or unknown), other-account-work
  caveat: part 1 ran from 01:05Z; usage at claim 90% weekly, the rest in the closeout.
- Accepted outcome, review/fix rounds and next routing decision: part 1 checkpoint; the task
  is not accepted until the reference and match columns are filled.

## Review and integration

Part 1 (checkpoint; the task is not accepted):

- Reviewer and independent reproduction/withheld-case results: a fresh Claude Opus 5.5
  subagent in the detached checkout `.worktrees/track-breadth-review` at `bf27e45`, tier 1 for
  the geometry arms and the runner option, tier 2 for the rest. It decoded `$81:A304-A51B` from
  the ROM by hand and matched every constant of the four new arms; scanned the ROM and the
  directory itself; decoded withheld tracks 8, 17, 30 and 40 against the manifest; reproduced
  the inventory, the matrix row for row, the override neutrality for ZOOM ZOO and DRAGSTER;
  re-ran `dragster-random-1` and `opposing-axes` with the same digests and restore counts; 25
  unit tests and ctest 23/23. **Approved**, no required findings, seven advisories
  ([review](https://github.com/malmazuke/unirally-reconstruction/pull/9#pullrequestreview-5286077171)).
- Required changes or acceptance rationale: none required. Advisories 1, 2, 4, 5, 6 and 7 are
  applied in the records (the `$0FF7` persistence limit in R-0046, fault positions as updates
  completed, the STATE wording, the manifest description, the static/verified split, the
  `--expect` check in the gate script and the handoff). Advisory 3 (the runner accepts any four
  options) is deferred to the next change that reruns the differential gates, because fixing it
  alone changes the gate binary; it is in the handoff. The CI fix `6a329f5` (the ROM-gated unit
  test removed, since the synthetic suite counts a skip as a failure) and the advisory commit
  touch no file the gate binary links; `gate_identity` against `gates-1f554fb` proves it.
- Exact merge candidate and required-check results: the pull request head; `changes`, `lab
  (ubuntu-24.04)` and `lab (macos-15)` green on it before merging (run IDs in the closeout).
- Integrated commit and evidence location: the merge commit of
  [#9](https://github.com/malmazuke/unirally-reconstruction/pull/9);
  `artifacts/track-breadth-part1-integration/closeout.json` and
  `local/evidence/track-breadth/` in the main checkout.
- Remote synchronization: through the pull request; local `main` fast-forwarded and compared
  with `origin/main` after the merge (closeout).
- Scope still unverified: every new track against the original; the four static arms under
  capture; the tour hypothesis; the `$0FF7` persistence.

Part 2 (checkpoint; the task is not accepted):

- Reviewer: a fresh Claude Opus 5.5 subagent in the detached checkout
  `.worktrees/track-breadth-2-review` at `933b68c`, tier 2 (laboratory tooling and records).
  It captured PINGPONG and FLAT FUN (withheld) and ZOOM ZOO itself, and reproduced their rows
  and pictures; checked the boundary rule on the M4-16 idle original and the byte-identity over
  frames 1207-2649; compared `original_rows` with the accepted `original()`; checked every
  R-0046 count against `sweep.json`, the masked lap comparison and the landing matrices; unit
  tests and the synthetic suite (484 checks). **Approved**, no required findings, six
  advisories ([review](https://github.com/malmazuke/unirally-reconstruction/pull/10#pullrequestreview-5286304245)).
- Changes in response (`a775f5d`): the guard departures inside exact windows (opponent X on
  HYBRID from row 348, A on SHORT CUT from row 953) and HAIRPIN HILL's native stop are
  recorded; rows versus updates defined; the player A/X/Start timeline check restored (all 16
  comparisons unchanged); the capture's menu position named as such. Declined: a ROM-free test
  of the comparison logic (it needs emulator memory; the gap stays open in the handoff).
- Merge candidate and checks: the pull request head, with `changes` and both `lab` jobs green
  on it; `gate_identity --since 1f554fb` passes (no gate-binary input changed).
- Integrated commit and evidence: the merge commit of
  [#10](https://github.com/malmazuke/unirally-reconstruction/pull/10);
  `artifacts/track-breadth-part2-integration/closeout.json`, and the sweep and captures in
  `local/evidence/track-breadth/track-breadth-2/`, in the main checkout.
- Scope still unverified: riding inputs, finishes and results on the new tracks; the five tours
  not offered on a cold start; stunt events.

Part 3 (checkpoint; the task is not accepted):

- Reviewer: a fresh Claude Opus 5.5 subagent in the detached checkout
  `.worktrees/track-breadth-3-review` at `775e7ba`, tier 1. It extracted pack v10 itself
  (identical; v9's 57 entries unchanged; the compiled table equals the rules), derived
  sceneries 5, 6, 9 and 13 from the listing (withheld), decoded `$81:8A2A-8AC7` and matched
  the new sampler, reproduced the recompare row for row and each edge case at the former
  stops, round-tripped a CROCK state, checked the 14 scenarios against the sweep, re-ran
  `opposing-axes` and `dragster-random-1` (same digests), picture-checked LOOPER, CROCK and
  EAST itself (withheld), and ran the unit tests, ctest and the synthetic suite.
  **Returned** with two required findings and eight advisories
  ([review](https://github.com/malmazuke/unirally-reconstruction/pull/11#pullrequestreview-5286795353)).
- Changes in response: R1 (the GO letters swapped on the six odd-boundary tracks; `$0300`
  counts from race start) fixed at the three parity sites and verified on 111 consecutive
  frames of five tracks; R2 (overclaims in STATE, R-0046 observation 15, stale matrix rows)
  corrected. Advisories: A2 (runner options) tightened; A3, A4, A5 and A7 corrected in code
  comments and R-0046; A1 (the finish slowdown's absolute frame modulo 3) recorded as an
  open question for a new-track finish capture; A6 (compare the frame label too) declined,
  since the labels already agree on all 16; A8 answered by this entry. Re-review requested
  on the new head.
- Re-review of `5bab77e` (fresh Claude Opus 5.5): R1 fixed and verified independently on
  five tracks, with the old code reproducing the fault; **returned** R3 (records named CROCK
  for the acceptance play, which aborts at update 1,731) and five advisories (A9-A13). All
  addressed in records, comments and the gate script; no gate-binary input changed
  (`gate_identity` against `gates-5bab77e`). Round 3 of `93c9a01` confirmed R3 and returned
  R4 (HYBRID's guard on no follow-up list), fixed in `ccef821`; round 4 **approved**
  `ccef821` with three minor advisories on wording, applied before the merge.
- Merge candidate and checks: the pull request head after re-review, with `changes` and both
  `lab` jobs green on it and the gate script run on it.
- Integrated commit and evidence: the merge commit of
  [#11](https://github.com/malmazuke/unirally-reconstruction/pull/11);
  `artifacts/track-breadth-part3-integration/closeout.json` and
  `local/evidence/track-breadth/track-breadth-3/` in the main checkout.
- Scope still unverified: a new track's finish, winner banner and result screen; the special-
  tile response, HYBRID's inverted-AI-marker guard and INFINITY's checkpoint guard (play can
  abort there, including after the exact windows: CROCK at update 1,731, WARIO PAINT at
  1,719, HYBRID at 2,053, all with the controller released); the five locked tours.
