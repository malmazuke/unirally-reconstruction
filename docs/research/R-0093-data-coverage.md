# R-0093 - Which data the original reads

Status: research result and tooling change ([DATA-COVERAGE](../../tasks/DATA-COVERAGE.md)), 10 October
2026, on main `6b6790a` (captures run on `33f3645`'s tools plus this task's two commits; WIPE-RAM
changed only the content tools). PAL ROM SHA-256
`a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`, reference core bsnes
`7d5aa1e6` (patched, `tools/locks/emulators.json`). Pack figures are against profile v36's rules.
Evidence in main `local/evidence/data-coverage/`.

Tags: **[L]** listing only, **[C]** confirmed in a capture, **[R]** stated by an accepted record.

## Domain

- **The corpus** is R-0089's: every distinct cold-start replay manifest behind the accepted tasks.
  236 runs, 1,666,993 frames. Manifest 004 (`audio-decision`'s deliberately invalid one) is left
  out, as in R-0089. Runs 103-106 fail only expectation checks stored in their manifests (the
  sample digest; 105 also the final-state hash). They are still cold-start executions of the
  original with their inputs, so their reads are counted (`followup.sh`).
- **Not in the corpus**: captures that preload cartridge RAM before power-on (the track sweeps,
  locked tours and HUNTER-EFFECTS' held runs). A replay manifest cannot express them, so the
  locked tours' tracks and scenery are under-counted here, as in R-0089.
- **What "read" means.** A ROM byte counts as read when a CPU instruction loads it as data, a DMA
  channel copies it from the A bus to the B bus, or HDMA reads it as a table or indirect data.
  Instruction fetches are not data reads. Banks `$84-$BF` are the 60 data banks.

## Method

- `access capture --resolve-rmw` (new, this task) for each manifest. Then `extract.py` reduces the
  record to three things:
  - the CPU's ROM-read bitmap and per-pc ranges;
  - each ROM-sourced A-to-B DMA, read byte by byte with fixed and decrement modes;
  - each HDMA table in ROM, walked direct or indirect.
- **Why `--resolve-rmw` was needed.** The sound upload at `$82:809C-80E3` reads `LDA [$63]` and
  advances the pointer with a 16-bit `INC $63` (`$82:8151`) [L, C].
  - Without the flag, an RMW's written value is unknown, so every later read through `[$63]`
    stays unresolved: 12,179 accesses in the 470-frame run 052 alone.
  - With it, the derivation computes the INC and the run has 0 unresolved accesses. Over the
    whole corpus there are 0 unresolved accesses [C].
- `coverage capture` for each manifest, then `coverage merge` (new, this task) to one corpus
  coverage file. `coverage map` of that gives the tracked map `docs/map/data-coverage-corpus.map.json`,
  and `coverage static-map` regenerates the static map with five raw captures.
- `aggregate.py` unions the runs and attributes bytes to banks, the two directories, pack entries
  and RNC streams. `track_assets.py` unpacks each track with the `$81:B8E2` port and lists its
  scenery assets.

## Findings

### How much of the ROM is read

- **Whole ROM:** the corpus reads 1,562,774 of 2,097,152 bytes (74.5%) [C]. By route, these sets
  overlap:
  - CPU: 904,803;
  - DMA: 678,967;
  - HDMA: 22,748.
- **Code banks `$80-$83`:** 10,983 bytes are read as data (tables).
- **Data banks `$84-$BF`:** 1,551,791 of 1,966,080 bytes read (78.9%). Unread padding is 91,883
  bytes (runs of 32 or more `$00`/`$FF`). Unread other bytes are 322,406 (16.4%).
- **What holds the unread non-padding bytes** [C]:

| Bytes | Held by |
| ---: | --- |
| 138,841 | packed rider presentation (sprite tiles and pose frames no captured rider or pose uses) |
| 91,557 | packed track streams (tracks the corpus does not load) |
| 36,922 | no directory entry and no pack entry (mostly bank `$A2`, below) |
| 27,387 | unpacked asset-directory entries (below) |
| 15,548 | packed rider collision poses |
| 8,239 | packed scenery |
| 3,912 | other packed entries (captions 2,875; front-end assets 544; ZOOM ZOO 476; track physics 17) |

- **Banks with heavy reads:**
  - Banks `$A7-$BC` are read almost entirely by DMA. The callers at `$82:B907`-`$82:BE0E` are the
    rider sprite-tile uploads [C].
  - Banks `$84-$93` are read by one to eight CPU routines each: the sound and asset loaders [C].
  - Bank `$95` is read only as HDMA tables (22,475 bytes) [C].

### The two directories

- **Asset directory.** It has 239 five-byte entries `$00-$EE` at `$82:B332` (bank|flag, address,
  length) [L]. It ends where a second directory begins: `$82:B7DD` holds the track scenery
  directory, 41 entries `$00-$28` in banks `$96-$97`. `$82:E17A-E181` installs `$82:B7DD` as `[$4F]`
  before a track's scenery is loaded [L, C].
- **Scenery lists.** Each track stream, unpacked to `$7F:0000`, holds a `$FF`-terminated list of
  scenery indices at the offset in its word `$000B`. `$82:E19F-E1BB` loads them one at a time
  through `$82:B2AC` [L, C].
  - 36 of the 41 scenery entries are read.
  - `$1B` and `$1C` are named only by track 44, which is locked.
  - `$00`, `$21` and `$23` are named by no track list [C].
- **Flagged entries are the tracks.** The 45 entries carrying the `$80` flag (`$C2-$EE`) are the
  45 track streams.
  - The track loader (`$82:E140-E152`: `ADC #$C2` to the track number from `$77:074A`) sends them
    through `$82:B2DD` to `$81:B8E2`, which the corpus executes [L, C].
  - `$81:BB7A`, the unpacker R-0089 counted as never run, is its VRAM twin. It has the same
    18-byte header skip, writes to video memory, and its only caller is `$82:B219`, the compressed
    branch of the VRAM copier `$82:B1DB` [L].
  - No call site asks that copier for a flagged entry. The listing regenerated from this corpus
    has 69 call sites: 67 pass constants up to `$C0` (the credits picture at `$83:FB10`/`FB1A`),
    and 2 compute an index (`$82:DC57`, `$82:DC65`) [L].
  - The computed index is a value mod 14 plus `$70` or `$82`, or 14 itself when bit 3 of
    `$77:0750` is clear (`$82:DC2E-DC3D`, which also sets `$12D1`). So it stays within `$70-$7E`
    and `$82-$90` [L].
  - Entries `$7E` and `$90` are the packed but never-read `scenery.14` backdrop. Which option
    selects it belongs to CARTRIDGE-OPTION-BITS.
  - So R-0089's "second graphics format" is the track format, and `$81:BB7A` looks unreachable in
    the shipped game. It is not a missing feature.
- **Which tracks load.** The corpus loads 25 of the 45 track streams in full: 0-6, 8-11, 13-16,
  18-20, 22, 23, 25, 30, 35, 40, 41. The unpacker reads every byte except the CRC and length
  fields of the header [C].
  - Nine more streams show exactly two bytes read, each at its first offset. Each of these starts
    where a fully read stream ends: the unpacker's word read-ahead (`$81:BB60`) runs two bytes past
    its stream [C].
  - The remaining 11 streams are untouched: tracks 27-29, 32-34, 37-39, 43 and 44, the locked
    tours. The pack has all 45 [R].

### What the original reads and native does not extract

- **Total:** pack v36 covers 1,816,601 ROM bytes, and the corpus reads 1,540,814 of them. In the
  data banks, 19,147 bytes are read but not packed [C].
- **The demo's sound set (7,356 bytes of samples).** Samples 02, 23 and 34 lie exactly between
  packed samples:

| Sample | Source | Bytes |
| --- | --- | ---: |
| 02 | `$90:8C41` | 2,425 |
| 23 | `$92:9504` | 2,020 |
| 34 | `$92:DC80` | 2,911 |

  - The sound loaders `$82:82D4`/`$82:82F0` read them, together with the block at `$93:F712`
    (2,899 bytes).
  - They load only in the 9-11 runs that reach an idle demo (attract-demo, idle-demo-cycles, and
    league or VS runs left idle) [C].
  - This is content [DEMO-AUDIO](../../tasks/DEMO-AUDIO.md) will need.
- **The rest of the sound uploads (11,603 bytes).** These are the unpacked parts of the uploads
  that `$82:809C-80BA` streams to the sound processor. The upload headers sit at `$93:C6B0`,
  `$93:EA9A` and `$94:8265` (review). The bytes lie between the packed pitch values, tables and
  scores:

| Source | Bytes | Read by |
| --- | ---: | --- |
| `$93:C6B2` | 4,107 | every run |
| `$93:D773` | 154 | every run |
| `$93:D80F` | 4,443 | every run (5 in full) |
| `$93:F712` | 2,899 | 10 runs: the nine idle-demo runs plus 053, which reaches the demo's start |

  - D-0009 reimplements the sound driver natively, so the driver's own bytes are not extracted
    by design [R].
  - The table does not separate driver code from data the native driver may still need. That
    question belongs to DEMO-AUDIO for `$93:F712`, and to D-0009's audits for the rest.
- **The rest (188 bytes):** single words and 3-byte pieces in `$84` and `$97`, read by the asset
  copier `$82:B296` and table walks [C].
- **Unread and unpacked asset entries** [C]:
  - **Credits-cheat picture:** `$BF` (`$8F:BAC0`, 13,920 bytes), `$C0` (`$8F:F120`, 1,792) and
    `$C1` (`$8F:F820`, 32). `$83:FB06-FB1A` loads them, R-0089's credits name cheat [L].
    CREDITS-NAME's pack v37 (#70) adds them.
  - **No constant call site:**
    - `$49` (`$86:9338`, 7,040), `$4B` (`$86:C2B8`, 2,400), `$51` (`$86:EE78`, 1,920);
    - `$47` (`$85:82F8`, 256), `$4F` (`$86:DEF8`, 2,048);
    - eight 32-byte palettes `$B7-$BE` (`$8F:B9C0-BAA0`) and `$03`, `$04`, `$3D`.
    - The loaders with computed indices take rider and member palettes (`$06`+rider,
      `$1F`/`$54`+member, `$26`+value). Whether any of these entries is reached that way is
      open [L].
- **Bank `$A2`, 32,768 bytes, is never read and in no entry.**
  - Its 8-byte records match the layout of bank `$A1`'s collision poses.
  - `$81:9E1B-9E42` indexes `[$21:8000 + pose*8]`, carrying into the next bank for poses 4096 and
    above [L]. The corpus reaches pose `$0AEB` at most [C].
  - Native packs only `$A1` (`physics.rider.collision-poses`). `$A2` is either the table's
    unreached continuation or unused; open.
- **Two smaller unread, unpacked regions:** `$94:957B` (2,532 bytes) and `$94:9147` (532) hold a
  command-like stream; `$97:D9F4` (896) is padded text [C].

### The static map from the corpus

- **Reclassification:** with the corpus map as a fifth tracked map, the static map's classes move
  [C]:

| Class | Before | After |
| --- | ---: | ---: |
| Observed | 41,778 | 86,837 |
| Inferred | 36,321 | 12,737 |
| Data | 92 | 74 |
| Unknown | 52,881 (40.4%) | 31,424 (24.0%) |

- **Agreement checks:**
  - The unknown share is now below D-0008's one-quarter trigger.
  - 95,618 sites over the five captures decode at one length, with 0 disagreements.
  - All 2,612 tracked ranges tile.
- **Routines:** the map now holds 759 routines (was 636). On main `ef8e440` (with CREDITS-NAME)
  native code cites 537 of them, covering 69,761 routine bytes.
- **Map size:** the corpus map is 1,149,327 bytes. M1-01 keeps a tracked map under about 1 MiB.
  - A corpus reaching most of the code banks passes 1 MiB however it is split (half the corpus
    gave 1,091,108 bytes).
  - So `coverage map` allows 2 MiB for a map of merged coverage only.

## Limits

- **HDMA tables in work RAM:** 3,321,994 HDMA channel activations used a table in work RAM, which
  `extract.py` cannot walk without per-frame work RAM. ROM data their indirect entries point to
  is not counted.
- **Discarded resolution rounds (review finding 4), measured:** the access derivation recorded a
  round's ROM reads before it knew whether the round would be discarded for a conflict.
  [ACCESS-ROUND-READS](../../tasks/ACCESS-ROUND-READS.md) records only the kept round and
  re-measured the corpus: no frame of the 1,666,993 needed a second round, every one of the 236
  reduced runs is byte-identical to this record's, and `aggregate.py` gives the same totals byte
  for byte. The figures above stand.
- **Coarse attribution:** per-pc attribution uses each pc's minimum and maximum address, so a
  routine that reads two far-apart tables is listed against everything between them. The read
  bitmaps themselves are exact.
- **Corpus scope:** the locked tours, cartridge-RAM states and listing-only branches are outside
  the corpus. "Never read" means never read by these 236 runs.

## Reproduction

Since this task the static map has five tracked maps, so `coverage static-map` and
`coverage disassemble` need five raw captures: STATIC-CODE-MAP's four and
`local/evidence/data-coverage/corpus-coverage.json`.

From the data-coverage checkout, with `local/` linked to the main checkout's, under
`local/locks/heavy-run.sh`:

```
local/evidence/data-coverage/captures.sh 10        # about 2 hours on 10 jobs
local/evidence/data-coverage/followup.sh           # runs 103-106 and any missing coverage
python3 local/evidence/data-coverage/track_assets.py <ROM> local/evidence/data-coverage/track_assets.json
python3 local/evidence/data-coverage/aggregate.py <ROM> tests/manifests/content/classic-crawler-tracks-pack.json local/evidence/data-coverage/aggregate.json
ls local/evidence/data-coverage/cov/*/coverage.json | sort > local/evidence/data-coverage/corpus-coverage.list
python3 tools/project.py coverage merge --coverage-list local/evidence/data-coverage/corpus-coverage.list --scenario data-coverage-corpus --out local/evidence/data-coverage/corpus-coverage.json
python3 tools/project.py coverage map --coverage local/evidence/data-coverage/corpus-coverage.json --out docs/map/data-coverage-corpus.map.json --summary docs/map/data-coverage-corpus.md
python3 tools/project.py coverage static-map --out docs/map/static/code-banks.map.json --summary docs/map/static/code-banks.md --labels docs/map/static/labels.json \
  --coverage <the four raw captures of tasks/STATIC-CODE-MAP.md, each after its own --coverage> --coverage local/evidence/data-coverage/corpus-coverage.json
```
