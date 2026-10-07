# PLAYER-HINT-VOICES - Does a player voice from 150 end the tutorial hints?

## Assignment

- Status: accepted (tier 3, research only), by pull request #59
  (https://github.com/malmazuke/unirally-reconstruction/pull/59).
- Milestone: M4 breadth.
- Coordinator: this session (primary and integrator), assigned by the user on 6 October 2026
  from SPLIT-CAPTIONS' listing report (main `local/evidence/split-captions/listing-report.md`,
  "What ends rider 1's hints").
- Task provider: Anthropic. Worker/session/runtime/model: Claude Code desktop, Claude Opus 5.5.
- Tier: **3** (D-0008). The result is a research record and record updates; no `src/core`, pack,
  gate or baseline change. Had the capture shown a divergence, the fix would have been tier 1 (race
  state) and this record would have been reclassified.
- Base commit: main `ec05ea7`.
- Branch and worktree: `claude/zealous-mestorf-1bd239` in
  `.claude/worktrees/zealous-mestorf-1bd239`.
- Owned paths: `docs/research/R-0085-player-hints-and-voices.md`, this record, the R-0061
  "not recovered" item, `tasks/README.md`.

## Outcome and boundaries

The question: can a one-player race queue a combination voice from 150 (riders 10-15) while the
tutorial hints run? If so, the original's bit-7 test (`$81:C5B0`) would end the hints where
native's `event < wrong_way` does not. The work: capture such a race, check `$12E3` and
`$0CA5`, and fix native if they differ.

Result: no divergence. A voice is always queued after its landing's trick event (1-21), and that
event ends the hints. Native already equals the original. See
[R-0085](../docs/research/R-0085-player-hints-and-voices.md). Out of scope: changing
`queue_player_announcement` to the bit-7 form for readability alone (tier 1 for no behavior
change).

## Evidence and attempts

| Attempt | Hypothesis | Experiment | Observation | Next decision |
| --- | --- | --- | --- | --- |
| 1 | A voice can be the first ending event | Listing `$82:9B69-9D97`, `$81:C598-C5ED`, combination table entry 0 | Trick events (1-21) are always queued before the voices of the same landing. Entry 0 is `$FE` | Confirm in a capture |
| 2 | Inputs exist for a first-landing combination | Native probe of the player's queue (`search/probe.cpp`). Fixed jumps found none; a random search over 41 tracks (`search/rsearch.py`, 1,200 runs) found 6 | Track 23, rider 12: Right from 1500, X+R 2018-2135 lands 3, 18, 171, 171 at 2143 | Capture the original at +55 frames |
| 3 | The original ends the hints on the roll, not the voice | `captures.sh` (`combo-a`, `combo-b`); `access capture` of `combo.json` watching `$81:C5B0`, `C5B4`, `C5B9` | 2198: `$12E3` 1 to 0, `$0CA5` zeroed; `C5B4` runs 4 times, `C5B9` once, with Y = 9 (event 3) | No change |
| 4 | Native equals the original | `track_reference explore --scenario auto` on `combo-a` | 1,360 of 1,360 updates exact, queue, cursors, `$0CA5` and `$12E3` included | Record |

All under main `local/evidence/player-hint-voices/`: `captures.sh`, `combo-a`, `combo-b` (full
work RAM per frame to 2800), `combo.json` with `make_manifest.py`, `access-combo` (work RAM
`$0000-$1FFF` per frame and the watched registers), `explore-combo-a.json`, `read_queue.py`, and
`search/` (the probe, the search and its hits).

## Handoff

- Head: this branch's records commit on `ec05ea7`; no uncommitted state.
- Commands:
  - `zsh local/evidence/player-hint-voices/captures.sh` printed two identical digests.
  - `python3 tools/project.py access capture --manifest .../combo.json --out .../access-combo
    --from-frame 2190 --to-frame 2200 --wram-series-range 0x0000 0x2000 --wram-series-every 1
    --watch-pc 0x81C5B0 --watch-pc 0x81C5B4 --watch-pc 0x81C5B9` gave `status=passed`.
  - `track_reference explore` gave 1,360 exact updates and no divergence.
- Skipped: the race sweep and the eleven differential gates. No source changed, so they would
  compare main with itself.
- Next: none for this question. R-0061's other riders' voices are covered by the listing.
