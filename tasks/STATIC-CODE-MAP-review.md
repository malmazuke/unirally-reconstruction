# STATIC-CODE-MAP - independent review (tier 2, one round)

- Candidate: `d287845` on `task/static-code-map` (base `main` `bdfd82e`).
- Reviewer: fresh Anthropic subagent, Claude Opus 5.5 (`claude-opus-5-5`), Claude Code
  desktop, 23 September 2026, isolated worktree `.worktrees/review-static-code-map` on
  branch `review/static-code-map` at the candidate. The author's worktree and `main` were
  not modified.
- Tier: 2 (laboratory tooling). The diff touches `tools/unirally_lab/coverage/`,
  `tests/tooling/`, `docs/map/static/`, research/task/state records and the command
  inventory only; nothing under `src/core`, packs, gates or baselines. No escalation.

## Verdict

**Approve with should-fix items.** Every required check reproduces: the tracked outputs
regenerate byte for byte on the reviewer's ROM path, the four inferred routines walked
against the ROM match the listing instruction for instruction and read as real code, the
site agreement holds under an independent decoder, the partition sums to 131,072 and the
tracked JSON carries no bytes or mnemonics. One correctness bug in the jump-table walk
(S1) leaves a 15-entry table in bank `$81` unwalked; fixing it moves about 1.2 KiB from
unknown to inferred but does not change the D-0008 conclusion (still above a quarter
unknown). The other items are record accuracy and test coverage.

## Commands run and digests

ROM: the path in the main checkout's `local/rom-location.txt`, passed explicitly with
`--rom`; SHA-256 `a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`.

Raw coverage inputs (sha256 computed by the reviewer) equal each tracked map's
`coverage.sha256`:

| Raw coverage | SHA-256 | Tracked map |
| --- | --- | --- |
| `local/evidence/m1-01/m1-01/cap1/coverage.json` | `1e6c0274...db8ca6` | `boot-start-600` |
| `local/evidence/m1-01/m1-01/race-cap1/coverage.json` | `a0752207...bf813d1` | `race-crawler-dragster-3000` |
| `local/evidence/m3-00-finish-evidence/m3-00-coverage/capture/coverage.json` | `7654ac20...839337e` | `race-crawler-dragster-12000-continuous-right-fields` |
| `local/evidence/m4-02-second-track/m4-02/coverage/capture-1/coverage.json` | `476de96f...ccd41d` | `race-crawler-zoom-zoo-3300` |

1. `python3 tools/project.py coverage static-map --out <tmp>/a/code-banks.map.json --summary <tmp>/a/code-banks.md --labels <tmp>/a/labels.json --rom <ROM> --coverage <the four files>`:
   `status=passed elapsed=16.336s`; checks `observed_ranges_tile` (2020 ranges, 0 without a
   tiling, 42700 shared instructions checked, 0 disagreements), `every_site_decodes` (53062
   sites, 0 disagreements), `classes_partition_the_banks` (observed 41778, inferred 35134,
   data 61, unknown 54099; 41.3%), `map_written` (185586 bytes) all passed.
   Digests equal the committed files:
   `code-banks.map.json` `bb35bb0b9a68f1b1d37af741e8f6b539c5f4c98b8e5f6112df504a0b82e13d36`,
   `code-banks.md` `91b237f1dc5e737aa93e11241a8a308daebc30ed3302e31b70b52611ef5387de`,
   `labels.json` `55312c4326e596afe175a4ccb00b015f31a278595d31fd1f7397a563ff5d001d`.
2. The same into `<tmp>/b`: `status=passed`; `cmp` identical to `<tmp>/a` for all three files
   (separate processes, so also a different Python hash seed).
3. `python3 tools/project.py coverage disassemble --out <tmp>/dis --rom <ROM> --coverage <the four files>`:
   `status=passed elapsed=16.683s`, same check lines. Listing digests: `agreement.json`
   `29c8e8cc...eb415696` and `bank-82.lst`/`bank-83.lst` equal the author's
   `listing-digests.txt`; `bank-80.lst`, `bank-81.lst` and `static-map.json` differ from it
   only by the labels `unk_808000` and `loc_819FCA`, which the records gained after the author
   generated the listing (see S3).
4. Fallback without `--coverage`: `status=passed`, `coverage_available` and
   `every_site_decodes` skipped (optional), observed 36823, inferred 29567, data 60, unknown
   64622 (49.3%), as R-0045 says. With one `--coverage` of four: `coverage_available` failed,
   `status=failed`, as designed.
5. `python3 -m unittest tests/tooling/test_static_map.py tests/tooling/test_coverage.py`:
   43 tests, OK. (The full synthetic suite was not run; it needs a native build.)

## Independent checks

### Agreement (check 3)

A reviewer script with its own 65816 length table (written from the opcode matrix, not
from `opcodes.py`; it agrees with `opcodes.py` on all 256 opcodes in every reachable mode,
differing only in the 24 unreachable emulation-mode states with M or X clear) read the
`sites` of all four raw files: 53,062 ROM sites in the code span (2,503 + 15,908 + 17,461
+ 17,190), 17,569 distinct offsets, 12 emulation-mode sites (all with M and X set), 0
offsets with two lengths, 0 sites starting inside another site, 0 crossing a bank end. The
union of site extents is 41,778 bytes, and the set of sites equals the tool's `observed`
instructions exactly (17,569, every length equal). The claim holds; see S2 for what it
does and does not test.

### Four inferred routines walked against the ROM (check 2)

A reviewer script decoded each routine from the ROM by recursive descent (fall-through,
branches, BRL, `JMP abs`; REP/SEP tracked; PLP ends the known mode) from the routine's
recorded entry mode, independently of `static_map.py`, and compared every instruction start
and length inside the routine's extent with the tool's listing.

| Routine (class inferred) | Entry mode | Reached by | Instructions (reviewer / tool) | Differences | Reading |
| --- | --- | --- | --- | --- | --- |
| `$80:98B3`-`$80:997F` (205 bytes) | NmX | JSR from `$80:BC61` | 87 / 88 | none; the tool's extra one is the final RTS after PLP, which the reviewer's walker leaves unmoded (RTS is one byte in every mode) | PHP; LDA #; STA dp; REP #$20; long loads and stores to `$77:07xx`; JSL `$83:9A1E`; a counted loop over X/Y (`LDX #$0013`, `STX $76` ... `DEC $76`, `BMI`, `BRL` back); SEP/JSR/REP; PLP; RTS. Plausible code; mode flips at `$80:98EC`/`$80:98F1` match |
| `$81:D936`-`$81:DA4E` (281 bytes) | NMX | JMP from `$81:D885`, `$81:D8B3` | 104 / 104 | none; all 281 bytes covered | 16-bit compare/subtract chains on `$0FCx`/`$12xx` work RAM, BIT #imm flag tests, JMPs to `$81:DB10`/`$81:DA4F`. Plausible |
| `$82:AEF5`-`$82:B059` (357 bytes) | NMX | JMP x4 from `$82:AE6F` ... | 149 / 149 | none; all 357 bytes covered | SEP #$20 then 8-bit flag work on `$15xx` and `$7E:20A3`, REP #$20 16-bit compares. Plausible; see S6 for the mode joins it contains |
| `$83:D581`-`$83:DAF2` (1394 bytes) | NMX | JSR from `$83:D345` | 690 / 690 | none; all 1394 bytes covered | REP #$30; decrement of `$7E:26BE`; a long unrolled run of `INC A; INC A; STA $7E:24xx`. Plausible |

Every mode transition the listing shows (REP/SEP at `$80:98B8`, `$80:98C9`, `$80:98EC`,
`$80:996C`, `$82:AEF5`, `$82:AF11`, `$83:D581`) matches the reviewer's propagation.

### Partition and clean outputs (check 4)

`41,778 + 35,134 + 61 + 54,099 = 131,072`, and the per-bank rows sum to the same (the test
also asserts it). A walk of every key and string value in `code-banks.map.json` and
`labels.json` finds only addresses (`$bb:hhhh`), record paths, scenario ids, digests, mode
names, class and reason vocabulary and map-schema edge kinds (JSR, JSL, JMP, JML): no opcode
bytes, no operand values, no mnemonics. The test in `tests/tooling/test_static_map.py` tests
what it claims for the six behaviours the acceptance table lists; S8 lists what it does not
cover.

### Cited addresses

The reviewer's own scan of the four globs finds 667 distinct matches, 643 of them at `$8000`
or above in the code span, equal to `labels.json`'s 643 entries.

## Findings

**S1 (should-fix, correctness): the jump-table walk ends at a zero first entry.**
`tools/unirally_lab/coverage/static_map.py:455` (`if target < 0x8000: break`). The dispatch
`$81:82F0 JSR ($82F5,X)` is guarded at `$81:82E6` (`BNE`; X = 0 goes to `JMP $8313`
instead), so entry 0 is a `$0000` placeholder. The walk stops there and records the table
with 0 entries (`jump_tables` in `code-banks.map.json`), so its 14 non-zero entries (13
distinct targets in bank `$81`, among them `$81:8316`, which is currently only a gap-sweep
candidate and stays unknown) are never seeded. The table ends exactly at the observed
`$81:8313`, 15 entries. Reproduction: the same analysis with one change, skipping a zero
entry at index 0 instead of breaking, gives 15 entries and observed 41,778 (unchanged),
inferred 36,321 (+1,187), data 90 (+29), unknown 52,883 (40.3% instead of 41.3%), with no new
rejections. Fix, add a synthetic test (a table whose first entry is zero), and regenerate the
tracked outputs and the figures in R-0045, STATE and the task record. The D-0008 trigger
conclusion stands either way.

**S2 (should-fix, record accuracy): with `--coverage`, the per-site check is close to
tautological.** Sites are placed at `instruction_length(rom[pc], mode)`
(`static_map.py:172`, `:228-237`) and `check_sites` recomputes the same expression
(`static_map.py:591-592`). It can only fail when two sites overlap or one address has two
lengths, which is a real but weaker property than R-0045 observation 1 and the handoff state
("decode at the same address with the same length under their recorded mode"). The
independent evidence is that mutual consistency (confirmed above) plus the tiling cross-check
(42,700 shared instructions from the tracked maps alone, 0 disagreements). Say so in R-0045
observation 1 and the handoff.

**S3 (should-fix, reproducibility): the tracked `labels.json` is a function of the records,
not only of the ROM and the coverage.** `cited_addresses` (`static_map.py:630-642`) scans
`tasks/*.md`, `docs/research/*.md`, `docs/inventory/*.md` and `docs/content/*.md` at run time.
Any later record that cites a new code-bank address makes the committed `labels.json` stale,
so D-0008's tier 3 "maps regenerated by tracked tools with unchanged inputs" never quite
applies to it. This has already happened once: the author's own listing digests went stale
when R-0045 gained `$80:8000` and `$81:9FCA`. It happens again with this review file:
regenerating on this branch with this file present gives 674 cited addresses instead of 643 (31 new, and 3 existing entries gain this file as a source record), and all three tracked files change digest (`labels.json` `4af239d292a44c58...`, `code-banks.map.json` `7d16effff71fc087...`, `code-banks.md` `b5ea93a678427844...`). Record the rule (regenerate `labels.json` at each integration that
changes a record's citations, and after this review lands), or fix the inventory as a
tracked input, or add a staleness check to the tooling tests.

**S4 (should-fix, records and fallback): "a fresh host needs the ROM and Python only" does
not hold for the tracked outputs.** The task record's Inputs and the handoff's runtime needs
say ROM and Python. Reproducing the committed files needs the four raw coverage files, which
live only in `local/evidence/` and need the pinned core build and `coverage capture` to
regenerate; without them the tools produce different tracked files (49.3% unknown). In that
fallback, 4,955 executed bytes inside the tracked ranges (41,778 - 36,823) are not classified
`observed`, although the class means "executed in a tracked map"; descent may then place
inferred instructions over executed bytes whose boundaries are unfixed. Correct the records,
and either keep every byte of a tracked range `observed` in the fallback or say in the
summary that the fallback undercounts it.

**S5 (nit, measured assumption): the call-return assumption can be measured and is false
about one time in nine.** Of 682 observed JSR/JSL/JSR (abs,X) sites whose return point also
executed, 80 (11.7%) return in a mode disjoint from the calling mode (e.g. `$80:8652` called
NMX, returns NmX). 44 inferred call sites call one of the 56 callees seen doing this, and their
fall-through is decoded in the calling mode (`static_map.py:306`, `:311`). A forward check from
each of those 44 under the callee's observed return mode finds 0 instruction-length
differences before the code resets M/X, so there is no boundary error today. Record the
failure rate next to the use count (1,259) in R-0045.

**S6 (nit, wording): modes at a join are the first arrival's, not a set.** `trial`
(`static_map.py:353-354`) skips an address already decoded, whatever modes the new path
brings. 251 flow edges arrive with modes absent from the target's recorded set (35 into
inferred code); none changes an instruction length before the modes re-converge (the code
issues REP/SEP right after), but the listing's mode column understates: `$82:AF30` is listed
NmX and is also reached in NMX by the `BRA` at `$82:AF28`. Either union the modes and re-check
lengths (recording an ambiguity on a difference) or soften "Modes are sets, never guesses" in
R-0045.

**S7 (nit): stale module docstring.** `static_map.py:7-12` says observed boundaries come from
tiling "propagated through REP/SEP" and that the gap sweep produces `inferred` code; the
implementation (and R-0045 observations 3 and 4) do neither.

**S8 (nit, tests): the jump-table walk, the cited-address scan, the several-tilings logic of
`_tile` (the fallback's core, R-0045 observation 2), trial rejection on a mid-instruction
entry or a bank end, and vector seeding have no synthetic test.** The walk is where S1 sits.
At least add tests for `walk_tables` (including a zero entry) and for a range with two
tilings keeping only the shared instructions.

**S9 (nit): a branch that leaves ROM and a fall-through past a bank end end the path
silently instead of rejecting the trial** (`static_map.py:310`, `:320-323`; `JSR`/`JMP` to
the same region are rejected by `implausible`). A scan of the decoded instructions finds 0 of
either today, so no current effect.

**S10 (nit, unrecorded deviation): explicit ranges are not labelled.** The task record's
Inputs says the cited ranges (`$82:A627--A6F7` style) seed `labels.json`; the regex labels the
start address only. Record it.

## Deviation and decisions

- **Raw coverage as input, with a fallback.** Justified: R-0045 observation 2 reproduces (the
  fallback's 49.3% against 41.3%), the inputs are digest-matched to the tracked maps and
  recorded in the tracked map, a partial set fails, and no coverage at all is reported as a
  skipped optional check. Accepted, subject to S4's record correction.
- **Gap-sweep decodings stay `unknown`.** Justified and conservative: both R-0045 examples are
  tables (`$80:8000` reads `01 02 04 08 ...`; `$83:8539`-`$83:8558` is ascending
  16-bit pointers), and the candidates seed nothing. It is also consistent with D-0008's
  revisit trigger. Accepted.
- **Determinism** holds (two separate processes, identical files), and the ROM-free tests pass.

## Scope not verified by the reviewer

- The full synthetic suite (native build) and the 444-test count.
- The review of `opcodes.py` itself (relied on R-0006's review, cross-checked by the
  reviewer's own length table as above).
