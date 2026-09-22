# R-0045 - A static code map of banks $80-$83

Status: implemented on `task/static-code-map` (STATIC-CODE-MAP), 23 September
2026. PAL ROM `a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`.
Builds on [R-0006](R-0006-observed-code-map.md) (opcode lengths, the LoROM
rule, map schema 1) under
[D-0008](../decisions/D-0008-static-map-track-breadth-review-tiers.md). The
listing is a reading aid and a hypothesis generator. **It is not gameplay
evidence.**

## What it is

`coverage disassemble` writes an ignored listing of ROM offsets
`$00000-$1FFFF` (banks `$80`-`$83`, mirrored at `$00`-`$03`). It has one file
per bank plus `static-map.json` and `agreement.json`. `coverage static-map`
writes the tracked `docs/map/static/code-banks.map.json`, `code-banks.md` and
`labels.json`. These hold addresses, classes and counts only: no ROM bytes, no
opcode bytes, no mnemonics. `tests/tooling/test_static_map.py` enforces this.
Every byte gets exactly one class:

| Class | Meaning | Bytes |
| --- | --- | ---: |
| observed | executed in one of the four tracked maps | 41,778 |
| inferred | decoded by descent from observed code, the vectors or a jump table | 35,134 |
| data | named by a recorded static reference, a long operand of inferred code, or a jump-table walk | 61 |
| unknown | none of these | 54,099 (41.3%) |

The unknown share is above D-0008's one-quarter trigger. As that decision
directs, no further heuristics were added; the unknown regions are targets for
dynamic capture. Per-bank totals, the routine table (618 routines) and the
counts below are in [code-banks.md](../map/static/code-banks.md).

## Verified observations

1. **The observations and the decoder agree everywhere.** All 53,062 recorded
   sites in the four raw captures behind the tracked maps decode at the same
   address with the same length under their recorded mode, with 0
   disagreements. Separately, each of the 2,020 tracked ranges can be tiled
   with exactly its recorded instruction count, with every recorded entry
   point on a boundary. The 42,700 instructions that every tiling shares are
   all sites of that length (0 disagreements).
2. **The tracked maps alone do not fix every boundary.** 462 of the 2,020
   ranges admit more than one tiling under their recorded modes. Without the
   raw sites, 6,179 range-instructions stay unresolved, and the run that uses
   the tracked maps alone classifies 49.3% of the bytes as unknown instead of
   41.3%.
3. **REP/SEP propagation inside an observed range can exclude the truth.** An
   earlier version propagated modes through each range and used the result
   to choose among tilings. At `$81:9FCA` it found exactly one tiling, and that
   tiling was wrong (8-bit accumulator where the site ran 16-bit), because the
   range joins code entered in more than one mode. Tiling now uses the
   recorded modes without propagation and keeps only the instructions that
   every tiling shares.
4. **A plausibility-only gap sweep decodes tables as code.** Decoding
   unreferenced gaps in every native mode, and accepting those that decode
   alike and end in a return or jump, accepts pointer and value tables. Two
   examples: the ascending 16-bit pointers at `$83:8535` read as a run of `STA`
   and `STX` to direct page, and the bit table `$80:8000` (`01 02 04 08 ...`)
   read as `ORA`/`TSB`/`BPL`/`RTI`. Gap-sweep results are therefore listed as
   candidates (2,835 instructions, 6,385 bytes) whose bytes stay `unknown`,
   and they seed nothing.

## Method and named assumptions (decisions)

- **Observed boundaries from the raw coverage.** With `--coverage` (one raw
  `coverage.json` per tracked map, matched by the digest the map records),
  observed instructions are the recorded sites. Without it they are the
  shared tilings, and the per-site check is reported as skipped. This departs
  from the task record's "tracked maps only" wording, because of observation
  2. A fresh host regenerates the raw files with each map's
  `regeneration_command`.
- **Descent is transactional.** Each seed decodes its routine's internal flow
  (fall-through, branches, `JMP abs`) as one trial. The whole trial is
  rejected if it reaches `BRK`, `COP`, `STP` or `WDM`, an `RTI` outside an
  interrupt vector, a `JSR`/`JMP` to a program-bank address below `$8000`,
  another instruction's interior, data, or a bank end. `JSR`, `JSL` and `JML`
  targets are separate trials. Seeds are the unexecuted successors of observed
  code, the vector targets, and the entries of `JMP (abs,X)`/`JSR (abs,X)`
  tables in the program bank (walked while each entry is a ROM address that
  is not data or another instruction's interior).
- **Modes are sets, never guesses.** REP and SEP narrow the candidate modes;
  PLP, RTI and XCE widen them to all four native modes. An instruction whose
  length differs among the candidates ends that path as a recorded mode
  ambiguity.
- **Two assumptions are counted, not hidden:** a `JSR`/`JSL` returns to the
  next instruction, and in the mode it was called in (used 1,259 times).
  4,083 absolute operands of inferred code name an unknown data bank and are
  not classified.
- **Cited addresses.** `labels.json` lists the 643 distinct ROM code-bank
  addresses that the research, task, inventory and content records cite
  (`\$(8[0-3]|0[0-3]):hhhh` at `$8000` or above), with their position and
  class and the records that cite them. 28 of them fall in unknown bytes (two are this record's own table examples); the
  summary lists each with its records. Fifteen are in `$80:8000`-`$80:84EB`
  and three at `$82:833B`-`$82:835B`. No decoded operand names them. The
  likely reason (not checked record by record) is that the records reach them
  through indexed or data-bank operands, which this map does not resolve.
  `$80:FFC0` is the cartridge header.

## Hypotheses (not verified)

- The routine starts and extents in `code-banks.map.json` are static
  readings. `inferred` code has never been executed in a capture. Any
  gameplay claim that rests on it needs a dynamic trace, as D-0008 requires.
- Much of the unknown share is probably data: tables read through the data
  bank or an index. Resolving it would need the data bank at each site, which
  the access captures record and this map does not use.

## Regeneration

```sh
python3 tools/project.py coverage static-map --out docs/map/static/code-banks.map.json --summary docs/map/static/code-banks.md --labels docs/map/static/labels.json --coverage <raw coverage of each tracked map>
python3 tools/project.py coverage disassemble --out artifacts/static-map/ --coverage <the same files>
```

Both runs take about 17 s. Two fresh regenerations give byte-identical files.
The digests are in the task record.
