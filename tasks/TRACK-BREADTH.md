# TRACK-BREADTH - every track through the shared engine, measured against the original

## Assignment

- Status: ready (prepared 22 September 2026 UTC under
  [D-0008](../docs/decisions/D-0008-static-map-track-breadth-review-tiers.md);
  [STATIC-CODE-MAP](STATIC-CODE-MAP.md) integrated 23 September 2026: read
  `docs/map/static/code-banks.md` and the ignored listing from `coverage disassemble` before
  designing a capture). Claim after the weekly reset on 2026-09-24T08:00Z (89% weekly at
  STATIC-CODE-MAP's integration) or on an explicit user override
- Milestone: M4 breadth (remaining tracks); the matrix, not any one track, is the outcome
- Coordinator: the preparing session (Claude Fable 5.1, Claude Code desktop, 22 September
  2026 UTC); the claiming session is coordinator, primary and integrator once it claims this record
- Task provider (fixed for all children; record any user-initiated platform change): Anthropic
- Worker/session/runtime/model: to be recorded at claim
- Actual model/reasoning effort, routing rationale and frontier escalation question (if any):
  the claiming session's model at default effort. Review tier under D-0008: **tier 2** for
  the inventory, extraction and pack profile; **tier 1** for any change to `src/core`
  arithmetic, the 742-byte state or the gates, which the primary must call out in the
  candidate so the reviewer applies the full D-0006 process to that part.
- Provider quota window/baseline timestamp, used/remaining or unknown, reserve and session
  allowance (D-0004): at preparation, weekly all-models 84% at 2026-09-22T11:20Z; sample
  fresh at claim. The matrix is expected to span sessions; checkpoint each track's row.
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

- Native capability delivered / still missing: to be filled per row.
- Frozen exact-match interval, field set and reference/seed identity: per track, in R-0046.
- Dynamic captured inputs still consumed (must be zero for autonomy): the `pre_race_matrix`
  entry and any other capture-derived entry, per track; the target is zero.
- Relevant branches/transitions exercised, including independent variations: race start,
  countdown, riding, per track.
- First divergence and cheapest next discriminating experiment: per row.
- Trial-wide usage baseline/current, reserve, reset authorization/outcome or none: to be sampled.

## Evidence and attempts

| Attempt | Hypothesis | Experiment | Observation | Next decision |
| --- | --- | --- | --- | --- |
| 0 (preparation) | The ROM's track set is a contiguous run of RNC streams | Header scan above | 45 streams in `$98`-`$9F`; DRAGSTER and ZOOM ZOO are the first two | Inventory command first; directory from the static map |

## Handoff

- Current base/head commit and uncommitted state: not claimed.
- Verified findings: the inventory observation above (preparation, not independently checked).
- Current hypothesis and failed approaches: none yet.
- Commands executed, outcomes and report hashes: none yet.
- Unavailable/skipped checks: none yet.
- Exact next experiment/command: at claim, implement `content rnc-inventory`, decode all 45
  streams, and compare the first two against the pack's recorded digests before anything else.
- Remaining dependencies: STATIC-CODE-MAP for the directory and loader reading.
- Runtime needs (network, build time, fixtures, memory): ROM, the pinned core for captures,
  an app-debug build (about 25 s), the gate scripts (minutes per track).
- Aggregate parent/child time, provider usage before/after (or unknown), other-account-work caveat: to be recorded.
- Accepted outcome, review/fix rounds and next routing decision: to be recorded.

## Review and integration

- Reviewer and independent reproduction/withheld-case results:
- Required changes or acceptance rationale:
- Exact merge candidate and required-check results:
- Integrated commit and evidence location:
- Remote synchronization: pushed ref(s), verified local/remote commit IDs, or exact push failure:
- Scope still unverified:
