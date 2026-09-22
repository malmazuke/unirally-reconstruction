# D-0008 - Static code map, track breadth and review tiers

- Status: accepted
- Date and owner: 22 September 2026, user direction recorded by the coordinator
  (Claude Fable 5.1 session, Claude Code desktop)
- Related milestone/tasks: M4 breadth; [STATIC-CODE-MAP](../../tasks/STATIC-CODE-MAP.md),
  [TRACK-BREADTH](../../tasks/TRACK-BREADTH.md); [CLASSIC-SPLIT-TIME](../../tasks/CLASSIC-SPLIT-TIME.md)
  stays ready and is reordered behind them
- Evidence records: [R-0006](../research/R-0006-observed-code-map.md) (the dynamic maps and
  their rule that nothing is inferred statically), [R-0018](../research/R-0018-m4-feature-inventory.md)
  (the feature matrix), [R-0021](../research/R-0021-zoom-zoo-content-contract.md) (the RNC
  decoder and the two tracks' differing widths), the tracked maps under `docs/map/`, and the
  measurements below, taken on 22 September 2026 against ROM SHA-256 `a1105819d48c04d6...`

## Problem

The user asked how much of the game is reconstructed, whether there is a faster method, and
why the current one is slow. Measured on 22 September 2026, twelve days and 741 commits in:

| Measure | Value |
| --- | --- |
| ROM | 2,097,152 bytes, LoROM, 64 banks of 32 KiB |
| Banks that profile as 65816 code (JSR/RTS density, entropy) | `$80`-`$83`, the first 128 KiB |
| Union of executed bytes over the four tracked coverage maps | 41,778 = 2.0% of the ROM, 32% of the code banks |
| Observed code inside a run that a research or task record cites by address | 26,894 bytes, 64% of the observed code (644 distinct cited addresses) |
| ROM bytes the two-track pack draws on | 1,144,488 = 54.6%, of which banks `$A0`-`$BF` (1 MiB) are the raw rider sprite atlas |
| RNC method-1 streams in the ROM (`RNC\x01` headers) | 45, contiguous in banks `$98`-`$9F` (file offsets `0xC0000`-`0xFBB4C`); DRAGSTER is the first, ZOOM ZOO the second |
| Tracks playable natively | 2 |

So roughly a third of the code has run under observation, about a sixth is documented, two of
what look like 45 tracks are recovered, and no menu, mode, rider, tour or audio is. By features
the reconstruction is on the order of an eighth of the game.

Why it is slow, from the records rather than from impression:

1. **Every routine is discovered dynamically.** The maps say it: only executed instructions
   classify bytes; nothing is inferred statically. There is no disassembly of the code banks in
   the repository, so every task rediscovers the routines around its topic through captures
   and watch sets (M4-05 through M4-11 were one mechanic each).
2. **Process cost does not scale with risk.** CLASSIC-RACE-HUD took five review rounds, four
   returned, all about the original's one-field-per-update redraw queue, which a static read of
   the dispatcher shows in one sitting. Presentation and content tasks carry the same
   preregistration, review and closeout load as arithmetic tasks.
3. **Tasks are scoped to the smallest evidenced gain** and never bet on the engine
   generalizing, so breadth grows one label at a time.

## Options and experiment

1. **Matching decompilation** (write C, compile, diff against the ROM), the method that makes
   N64 and GameCube projects fast. Not applicable: the game is hand-written 65816 assembly
   with no compiler output to match.
2. **Library signature matching.** Little value; SNES titles share almost no library code.
   The RNC compressor is already identified (R-0008, R-0021); the sound driver may be a
   recognizable one, which is the only place this could pay.
3. **A hybrid that executes the original code in an embedded 65816 interpreter for
   non-gameplay screens** (menus, tours, options) while native owns the race. **Rejected by
   the user on 22 September 2026**, and not to be re-proposed: it strays from the goal of a
   native reconstruction, it creates legal risk by executing and effectively distributing the
   original program (and it breaks D-0005's separation between a clean program and a locally
   generated content pack), and it prevents extending those features for modding and new
   gameplay.
4. **One static, annotated code map of the code banks, seeded by the dynamic maps.** Adopted.
5. **Track breadth by betting on the shared engine and extracting every track.** Adopted.
6. **Review tiers by risk.** Adopted.

## Decision and consequences

### Static code map (STATIC-CODE-MAP)

- Produce a static disassembly of banks `$80`-`$83` once, seeded from the union of the tracked
  coverage maps: their sites carry the processor mode (M and X widths) that makes 65816 static
  disassembly otherwise ambiguous, their entry points and static references seed recursive
  descent, and the header vectors anchor it. Gaps are swept with modes propagated from
  REP/SEP along fall-through and branch edges, and each byte ends classified as observed code,
  inferred code, referenced data or unknown.
- **What is tracked** follows map schema 1's rule: no ROM bytes, opcode bytes or mnemonics.
  The tracked artifacts are a per-byte classification and routine table with entry kinds,
  callers and cited-by records, and a label file (address, name, comment, source record). The
  readable listing is regenerated from the user's ROM into ignored `artifacts/` by a tracked
  command, and agents read it locally.
- **Rule change.** A task may cite the listing as the source of a routine's reading, and
  should read it before designing a capture. A gameplay claim still needs the dynamic
  evidence AGENTS.md requires: the listing generates hypotheses and tells a capture where to
  look; it is not acceptance evidence, and the coverage maps remain the ground truth for
  what executes.

### Track breadth (TRACK-BREADTH)

- The 45 RNC streams are the candidate track set; the static map is expected to find the
  directory that indexes them. The bet is that the race engine is track-agnostic and the
  remaining tracks are content, which CLASSIC-PRESENTATION-UNIFICATION already established for
  the second track ("engine content through `zoom_zoo_content` with the track's own
  overrides, presentation content through `classic_race_presentation_content(pack, track)`,
  no new renderer or loader").
- The experiment is a matrix, not a track: decode every stream, extract an all-tracks pack,
  load and run each track natively, capture a short original reference per track through the
  menu, and compare. Tracks that match are done; each divergence names the missing mechanism
  and becomes a task with its address range already known. This replaces label-only breadth
  in R-0018 with measured breadth.
- Per-track entries that today come from a capture of the original (the
  `pre_race_matrix` landing-response entry) are dynamic inputs; a track whose producers are
  not recovered loads but is not accepted, and the matrix says so.

### Review tiers

Classify every task at claim, record the tier and the reason in the task record, and let a
reviewer escalate a tier when the diff touches something the classification did not admit.

| Tier | Scope | Process |
| --- | --- | --- |
| 1: arithmetic and state | Anything that changes simulation state, integer arithmetic, ordering or timing in `src/core`, the 742-byte state, pack rules or format, differential gates or reference baselines | Unchanged D-0006 process: preregistered inventory, fresh isolated independent reviewer with withheld cases, returned rounds until approval, consolidated closeout |
| 2: presentation, content and tooling | Presentation on recovered layers, extraction with existing decoders, laboratory tooling and CI, the static map itself | One independent review round by a fresh subagent against the reviewer checklist, on the exact candidate; no preregistration; the frozen differential gates and pixel sweeps are the evidence; a returned finding gets one re-review |
| 3: records and regenerated maps | Task, decision, state and research records; maps regenerated by tracked tools with unchanged inputs | No independent review; the docs-only CI fast path; the next session's read is the review |

D-0004's budget rules and the 20% reserve are unchanged. D-0006's automatic review applies in
full to tier 1, as one round to tier 2 and not at all to tier 3.

### Ordering and budget

STATIC-CODE-MAP first (tier 2 tooling, the enabler), TRACK-BREADTH next (tier 2 extraction
with tier 1 engine changes), CLASSIC-SPLIT-TIME after them or whenever the static map makes
it a short read. All of them start after the weekly reset on 24 September 2026 08:00Z or on
an explicit user override: this session sampled weekly all-models usage at **84%** at
11:20Z, above the reserve floor, and prepared records only.

### Progress measure

`docs/STATE.md` carries two breadth lines from now on, updated when their inputs change:
tracks matched against the original out of the inventoried streams, and code-bank bytes by
class (observed, inferred, data, unknown) out of 131,072. The project plan's rule stands:
neither is the headline metric, but both are what the user asked for and both are measurable.

## Revisit trigger

- The static decoder leaves more than a quarter of the code banks unknown after the gap
  sweep: fall back to targeted dynamic captures for the unknown regions rather than expanding
  the static heuristics.
- More than a third of the inventoried tracks diverge from the original in engine arithmetic
  rather than in content: the generalization bet failed; return to per-mechanism tasks, but
  ordered by the divergence matrix.
- A tier 2 task ships a state-affecting defect that tier 1 review would have caught: tighten
  the tier boundary and record the case here.
- The user's rejection of option 3 is a standing constraint, not a revisit trigger.
