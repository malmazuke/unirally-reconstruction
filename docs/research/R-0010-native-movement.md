# R-0010 — Native movement prerequisite investigation

- Status: prerequisite investigation accepted through M2-01A and M2-01; final
  native movement agreement is recorded in the task handoffs and
  [R-0011](R-0011-m2-acceptance.md).
- Task: [M2-01](../../tasks/M2-01.md), dispatched base `a9f86e590e4be0d76369a975ece2d556883aa51f`.
- ROM: PAL Unirally, SHA-256 `a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`; unchanged bsnes commit `7d5aa1e656b9171524d01b1b22917197d8121cb4`, patch `a719f5ffe2222dad4c1ab04336633319ad85004f74e32fc14893a058be333885`, Strict serialization.
- Domain: primary Crawler/DRAGSTER race; end-of-frame samples, first full native update intended at 1534 from initial observation 1533. Reference baseline is unchanged. Native sampling and progress components exist; autonomous movement is not implemented.

## Coordinator-approved scope amendment (12 September 2026)

The required player speed depends on opponent-derived state. The coordinator
explicitly includes the minimum track-grid gather/progress transition and
opponent motion that causally feed player speed. Unrelated opponent features
remain excluded. Required player projections and frozen outputs stay unchanged;
no expected per-frame state or dynamic table read may drive a native update.
Investigate this prerequisite for at most 45 minutes before reassessment. A
substantial unresolved format may become a separate prerequisite with standalone
evidence, rather than a partial gameplay acceptance.

## Verified observations

1. The entry range in M1 mostly copies player state into shared scratch and back.
   Position is copied `$0415 → $A5` at `$82:8AB1–8AB4` and back at
   `$82:8DB7–8DB9`; speed `$04BB → $0FA9` at `$82:8B97–8B9A` and back at
   `$82:8E97–8E9A`. Actual arithmetic is in callees dispatched by
   `$82:8C3A–8C87`, not the final field stores.
2. **Opponent progress affects required player speed.** At primary frame 1632,
   `$82:A7BA` reads opponent counter `$0FCF=6`; `$82:A7BE` subtracts the player
   counter `$0FCD=5`. `$82:A7CE` writes player adjustment `$0343` from 1 to 2.
   The positive-speed clamp `$82:A7E3–A817` combines zero `$11D7` with that 2,
   halves it and adds `$11D3=448`, then writes 449 to `$0FA9`. Earlier in the
   frame `$82:A9EC` wrote 472 (448+24). The published speed is 449. At frame
   1631, the adjustment is 1 and the cap is 448. The first nonzero adjustment
   in the observed window is at 1587; its first effect on the required speed
   is at 1632. These are primary observations, not withheld-case results.
3. The counter dependency extends through track traversal: `$82:91DD` publishes
   the opponent `$0FCF` from scratch `$0FB9`; `$82:9805` increments `$0FB9`
   after comparing a transition encoded in bits 10–12 of `$0FB5` with the ROM
   transition table at `$80:84CB`. `$81:8BC6` fills `$0FB5` from sampled track
   words, which `$81:8B66` gathers from `$7F:800F,X` into `$0260,Y`.
   The spatial gather is now reproduced (findings 4–6); autonomous opponent motion remains under investigation.
   This is a concrete dependency beyond a player-only isolated update.
4. **The pose/table concern is resolved for the sampling component.** The
   preliminary sprite-only interpretation was wrong: `$81:8DB6` calls
   `$81:9E1B` immediately before `$81:8DB9` calls the sampler `$81:8B75` (and
   the opponent follows the same pair at `$81:8F0A/8F0D`). The bank $21 records
   produce collision sample positions. An eight-byte record is indexed by
   the pose index `$0F85 * 8` (three `ASL`s at `$81:9E20-9E22`); its first four bytes are two x/y pairs, bytes
   four/five are an origin, and its last word selects eight additional pairs
   in the byte template region starting `$20:BC9F`. Template byte offset is
   `u16(byteswap16(selector) << 4)` (`XBA` and four `ASL`s, `$81:9E6F-9E76`), **not just
   the high selector byte times 16**. Each pair adds the origin modulo 256. When `$0F51 != 0`, every x
   becomes `u8(47-x)`, then every x gets `+8` modulo 256. The source operand
   offsets for 47 and 8 are `0x009F14` and `0x009F5D`.
5. **Spatial gather reconstructed.** `$81:8A2A–8B74`: x/y divided by 64 select
   four neighbouring coarse-map words at decoded offsets `0x000F +
   2*(row*width+column)`, next column, next row, and both. Each word indexes a
   32-byte block at decoded offset `0x800F`; the ten points choose a quadrant
   using the sign bit of 8-bit subtraction against `64-(coordinate&63)`.
   Fine-cell offset is `(((point.x+x)&0x30)/4 + ((point.y+y)&0x30))/2`, added
   to the block base. Intermediate word arithmetic wraps at 16 bits. Coarse
   width is `$04F5=1024`, stride `$04F7=2048` in every primary series record
   1533–2999. Negative-y and the last-column alternate branch are not covered.
6. **Native isolated sampler equals all observed words.**
   `src/core/track_sampling.cpp`, using static extracted content and captured
   incoming x/y, pose index and reflection flag, matches 29,320 words from
   2,932 original calls (both riders; primary frames 1534–2999). Incoming x/y
   are sampled at `$81:8D97/8D9C` for the player and `$81:8EF1/8EF6` for the
   opponent, before collision correction; end-of-frame y can differ and
   cannot substitute. Each call's ten outputs are captured at `$81:8B6A`,
   with Y counting 18,16,...,0. This proves an isolated dependency, not an
   autonomous native update. The first native implementation matched without
   changing any reference value. The first harness attempt rejected the
   capture's extra `frames.count` metadata before invoking native code;
   correcting that schema check yielded agreement.

## Reproduction and evidence

Baseline commands, all exit 0, under `artifacts/m2-01/baseline/`: `doctor`,
`bootstrap`, `build --preset lab-debug`, `test --suite synthetic` (196 checks),
`reference build`, and `replay compare --manifest tests/manifests/replay/race-crawler-dragster-3000-fields.json`.
The administrative checkpoint occurred during the long chained command: its
synthetic report records the source transition. No implementation or baseline
changed; a stable-source rerun is required before submission.

Reference freeze: three cases in `tests/manifests/native/`, each compared in two
fresh processes under `artifacts/m2-01/freeze/<case>/`; each reports all original
ranges, final state and A/V identical. Primary sample digest is
`72f618f2f7e3416eac64d38f882471b1a9ebee25018ed76143360a92cdd99b1f`.
Withheld series are sealed for implementation; only their identity and comparator
outcomes were inspected. Their compact projected series are generated by
`tools/unirally_lab/native/freeze_reference.py`, which computes no native field.
Regeneration instructions are in the native manifest README.

Capture 1, unchanged sample digest and final state, exit 0:

```sh
python3 tools/project.py access capture --manifest tests/manifests/replay/race-crawler-dragster-3000-fields.json --out artifacts/m2-01/movement-access --from-frame 1533 --to-frame 1700 --watch-address 0x0000A5 --watch-address 0x000F5F --watch-address 0x000F73 --watch-address 0x000FA9 --watch-pc 0x819E42 --watch-pc 0x819E54 --watch-pc 0x83F2F1 --watch-pc 0x83F2FB --wram-series-range 0 0x2200 --timeout 120 --report artifacts/m2-01/movement-access/report.json
```

Access SHA-256 `8027f268b22e5b1d…`; 3,061,594 instructions, 1,523,897
accesses over frames 1533–1700; 3,847 unresolved accesses, zero unresolved
stores. Series SHA-256 `b54da2c8bd5d6cdd561752d669c8d04be898b2f711d715a75719407f044d9a51`,
3,000 records of 0x2200 bytes, offset 0, no header. Read each record's words
little-endian. This series contains primary state only.

Capture 2 isolates the speed dependency (unchanged sample digest, exit 0):

```sh
python3 tools/project.py access capture --manifest tests/manifests/replay/race-crawler-dragster-3000-fields.json --out artifacts/m2-01/speed-dependency --from-frame 1580 --to-frame 1642 --watch-address 0x000FCD --watch-address 0x000FCF --watch-address 0x000FB5 --watch-address 0x000FB7 --watch-address 0x000FB9 --watch-address 0x000343 --watch-address 0x000FA9 --watch-address 0x0011D3 --watch-address 0x0011D7 --watch-address 0x0011F1 --watch-pc 0x82A7BA --watch-pc 0x82A7BE --watch-pc 0x82A7F8 --watch-pc 0x82A817 --watch-pc 0x829805 --timeout 120 --report artifacts/m2-01/speed-dependency/report.json
```

Frame 1632 sequence numbers: opponent-progress read 1457; player-progress
subtraction 1459; adjustment store 1466; speed-clamp store 1484; speed loaded
for position integration 1526; speed publication load 2147. The same capture
shows the prior frame and first adjustment at 1587. Watch values on arithmetic
reads may be null by the existing access contract; the loaded register and
prior stores resolve the expression, without treating null as a value.

Local-only disassembly was read from ROM and retained under
`artifacts/m2-01/`. To recover observed modes and offsets:

```sh
python3 tools/project.py coverage capture --manifest tests/manifests/replay/race-crawler-dragster-3000-fields.json --out artifacts/m2-01/coverage --timeout 120 --report artifacts/m2-01/coverage/report.json
python3 tools/project.py coverage map --coverage artifacts/m2-01/coverage/coverage.json --out artifacts/m2-01/coverage/map.json --detail artifacts/m2-01/coverage/detail.json --report artifacts/m2-01/coverage/map-report.json
```

Both exit 0; 49,465,866 instructions; the sample digest is unchanged. No ROM
bytes, full disassembly, extracted content or emulator states are tracked.

## Not established

No autonomous native movement simulation, native movement comparison or native
withheld-case agreement exists yet. Sampling and progress are recovered as
isolated components, with explicit 15-byte progress serialization and sampler
sanitizer evidence. The full riding dependency closure, collision response and
opponent input/motion remain unfinished. Timer digits and input axes are not
implemented. Frozen observations do not satisfy gameplay acceptance.

## Sampling checkpoint

Native source is readable C++20 with explicit byte/word wrapping and checked
content bounds. Static content manifest `tests/manifests/native/movement-sampling.content.json`
extracts the existing track data plus bank $21's first 32 KiB (ROM offset
`0x108000`) and the available template-region tail from `$20:BC9F` through
`$20:FFFF` (ROM `0x103C9F`, 17,249 bytes). Extracting the available tail is a
bounds choice, not a claim that every byte is one template. Observed primary
selectors include offsets 0,16,8560,8576,8592,8608,9632,9680; a first proposed
4 KiB extent was insufficient and was expanded **before the probe**. Pose
indices fit the extracted bank in this domain. The identity-checked content
stays ignored; authored tests contain no original tables.

```sh
python3 tools/project.py access capture --manifest tests/manifests/replay/race-crawler-dragster-3000-fields.json --out artifacts/m2-01/sampling-access --from-frame 1534 --to-frame 2999 --watch-address 0x0000A5 --watch-address 0x0000A7 --watch-address 0x000F85 --watch-address 0x000F51 --watch-pc 0x818B6A --watch-pc 0x819E1D --watch-pc 0x818A2A --timeout 180 --report artifacts/m2-01/sampling-access/report.json
python3 tools/project.py content decode --manifest tests/manifests/native/movement-sampling.content.json --out artifacts/m2-01/content-expanded
python3 -m tools.unirally_lab.native.probe_sampling --access artifacts/m2-01/sampling-access/access.json --content-manifest tests/manifests/native/movement-sampling.content.json --content artifacts/m2-01/content-expanded --probe build/lab-debug/tests/native/sampling_probe --coarse-width 1024 --report artifacts/m2-01/sampling-probe-3.json
```

All exit 0. Capture access SHA-256 `bebe75c349332a55…`, 27,090,754 instructions,
13,466,478 accesses, 28,223 unresolved accesses and zero unresolved stores;
original sample digest unchanged. Native pose/grid/bounds C++ tests 3/3 and
reference-freeze/capture-input Python tests 7/7 pass. These are not the full
M2-01 native movement acceptance tests. The final stable-source suite and sanitizer results below supersede this
checkpoint; withheld native tests remain unrun.

Independent coordinator evidence: separately reproduced the speed chain at
1631/1632 (`artifacts/m2-01-coordinator/speed-dependency/access.json` in root,
SHA-256 `3e362d4b48180cb0f3f12d684b86c8d4d9e6f336509ab12aac20d4a224d5474d`).
It also independently projected all 1,467 primary rows from both its pre-freeze
captures, matching the primary expected-file SHA-256
`5b6b2f6f2d513d1ef2b230f21b0774229bdbc6a207ff4a530245e8c7051fb640` without
using the freeze utility. No withheld output was inspected in either check.

## Progress recurrence and component review fixes

`track_progress.cpp` implements `$82:979A–981D`. The next tag is
`(marker_word & 0x1C00) >> 9`. A zero previous tag or unchanged tag causes no
counter step. Otherwise lookup the previous tag's byte offset in the four
ordered tables at `$80:84CB`, `$80:84DB`, `$80:84EB`, `$80:84FB`: matching the
new tag changes the u16 counter by +1,+2,-1,-2, respectively. A negative table
word stops lookup and rejects; reaching the fifth table cannot accept because
both of its paths reject, so the native implementation directly rejects there.
A rejection preserves the old tag. Success remembers the new tag and clears
rejection. The static 80-byte table region is extracted through
`movement-progress.content.json` from ROM file offset `0x0004CB`.

Marker observation scans gathered samples from index 9 to 0, retaining the last
word whose low ten bits are zero and whose bits 10–12 are neither zero nor
all set (`$81:8BB5–8BC6`). The marker persists if no sample supplies one.

**Subframe schedule:** `$83:CCB8–CCBE` computes `u8(1 - previous_phase)` into
`$0302`; the player dispatch `$82:8C4E–8C6B` advances progress on phase 1 and
the opponent dispatch on phase 0. A first hypothesis advancing both each frame
produced 159 mismatches, first opponent at 1577; reading the phase branch fixed
that error. This is not absolute frame parity: `ProgressUpdateState.phase` is
initialized once from `$0302` at frame 1533, persisted and serialized, then
updated by subtraction. Both marker observations still run every frame.

The native recurrence on native sampled words matches all four fields
(marker/tag/count/rejection) for both riders on 1534–2999: 11,728 values.
Initial fields at frame 1533 are zero for both riders; initial phase is 1.
`ProgressBytes` is exactly 15 explicit bytes: little-endian marker, tag, count,
one rejection byte, repeated per rider, then one phase byte. Invalid flag/phase
values and incorrect length are rejected. The probe round-trips this encoding
after every frame; it is a component continuation check, not M2-02 acceptance.

```sh
python3 tools/project.py content decode --manifest tests/manifests/native/movement-progress.content.json --out artifacts/m2-01/progress-content
python3 -m tools.unirally_lab.native.probe_sampling --access artifacts/m2-01/sampling-access/access.json --content-manifest tests/manifests/native/movement-sampling.content.json --content artifacts/m2-01/content-expanded --probe build/lab-debug/tests/native/sampling_probe --coarse-width 1024 --report artifacts/m2-01/sampling-probe-reviewed.json
python3 -m tools.unirally_lab.native.probe_progress --sampling-output artifacts/m2-01/sampling-probe-reviewed.native.txt --sampling-report artifacts/m2-01/sampling-probe-reviewed.json --series artifacts/m2-01/movement-access/wram-series.bin --series-access artifacts/m2-01/movement-access/access.json --content-manifest tests/manifests/native/movement-progress.content.json --content artifacts/m2-01/progress-content/progress-transitions.bin --probe build/lab-debug/tests/native/progress_probe --report artifacts/m2-01/progress-probe-reviewed.json
```

Every incoming sample position/pose is still captured. Progress itself evolves
from one seed; this does not remove the oracle dependency from the *whole*
movement experiment, so no movement agreement is claimed.

Independent component review of `611c396`: reviewer reproduced the capture
digest and all 29,320 words in debug/sanitizer builds, plus 206/206 checks. CI
run 34654922992 passed Linux (including sanitizer stage) and macOS on that
checkpoint. Its findings led to: an authored wrapped-sign test (point x=192,
boundary=64, BMI selects the left quadrant); execution/protocol errors mapped
to exit 1 before attempting output comparison; strict primary reference identity
checks (core/patch/options/serialization, ROM, manifest, script, completeness,
nontruncated watches); and corrected stale status prose. New tests reject changed
identity, missing output and malformed output. No reference expectation changed.
The sampler's original check after passing the suite used the misspelled preset
`lab-sanitized` and returned exit 3; the corrected `lab-sanitize` built and ran
all sampler checks without diagnostics. The task handoff lists final rechecks.

## Remaining dependency boundary

After gathering samples, `$81:8DBC` calls `$81:8F98` (opponent analog likewise).
The caller publishes corrected position at `$81:8E17/8E1C`, horizontal/vertical
velocity at `$81:8E22/8E28`, and contact/airborne-like state `$054B` from `$0F33`
(the latter meaning remains provisional). Original player y before collision
can differ from its end-of-frame y (861 vs 859 at frame 1600), so the response
cannot be omitted. The opponent has additional nonzero `$0F33` states and
velocity changes which later affect the player's speed through progress.
Inventory this post-gather response's incoming fields, outputs and actual
branches before implementation. Following it comes pose/orientation selection
in `$83:EF54–F0F7` and `$83:ED7B–EF53`, which supplies the next pose-indexed
collision points. No new static table format is presently a blocking unknown;
the remaining risk is the coupled movement/contact state closure.


## Coordinator reassessment and M2-01A handoff

The coordinator stopped expansion at this boundary on 12 September 2026:
required player riding speed depends on opponent jumps/contact/orientation that
M1 explicitly left unvalidated. M2-01 is blocked on **M2-01A**, a separate
research prerequisite owned by the coordinator. This is not a new undecoded
bank-table format: sampling and transition tables now have narrow documented
interpretations. The unresolved mechanism is the coupled contact/pose/opponent
motion contract. No native contact update was added, no required player field
was removed, and no native withheld case was run.

The smallest next closure is the actual post-gather response and the next pose
it selects, including the opponent jump state that changes that response:

- `$81:8B75` preprocesses raw sample words and the per-tile table into ten-word
  scratch arrays `$0230–0243`, `$0290–02A3`, `$02C0–02D3` before `$81:8F98`.
  The native spatial sampler deliberately ends before this preprocessing.
- The conservative access inventory for PCs `$81:8F98–982B` contains 105
  distinct `(address,width)` spans, 54 read-only within that range. These include
  scratch and stack locations; they are **not** 105 semantic state fields and
  do not include callees outside the range (for example `$81:982C`). The ignored
  artifact is `artifacts/m2-01/collision-response-access-inventory.json`, derived
  from `sampling-access/access.json`. To regenerate the inventory, filter its
  `accesses` rows by that inclusive PC range, group by `(address,width)`, and
  collect kind/read/write PCs using the record's named `access_fields` and
  `kind_names`; preserve its source hash and conservative scope.
- In all 2,932 primary riding calls, `$81:980C` writes y scratch `$A7`.
  `$81:970B` writes vertical velocity `$0FAB=0` and `$81:97E1` writes horizontal
  velocity `$0FA9` in 2,889 calls. Normal-response ROM read sites `$81:96AC/B0`
  each read `$00:822B/824B` 2,889 times. The velocity-matrix writers
  `$81:95FB/9601` do not execute in riding frames 1534–2999, even though the
  full-run coverage includes them before riding. Do not infer riding coverage
  merely from the full-run map.
- `$81:9248` writes `$0F33` 14 times across riding frames 1580–1751;
  `$81:94CA` clears it once at 1617. These nonzero states and the next
  pose/orientation prevent an always-grounded opponent approximation.

### Focused primary capture and exact next experiment

Run from the task checkout, with the existing private ROM and pinned core.
This command regenerates all watches used in the focused investigation; no
withheld fixture is involved:

```python
import subprocess
addresses = [0xA5, 0xA7, 0x333, 0xF1F, 0xF33, 0xF51, 0xF85,
             0xF91, 0xF93, 0xFA9, 0xFAB, 0x12D1]
addresses += list(range(0x230, 0x244))
addresses += list(range(0x290, 0x2A4))
addresses += list(range(0x2C0, 0x2D4))
pcs = [0x818F98, 0x81982B, 0x819248, 0x8194CA, 0x81970B,
       0x81980C, 0x83E122, 0x83E21F, 0x82A914, 0x82A921,
       0x82A92D, 0x82A935]
cmd = ['python3', 'tools/project.py', 'access', 'capture', '--manifest',
       'tests/manifests/replay/race-crawler-dragster-3000-fields.json',
       '--out', 'artifacts/m2-01/contact-contract', '--from-frame', '1576',
       '--to-frame', '1618', '--timeout', '120', '--report',
       'artifacts/m2-01/contact-contract/report.json']
for address in addresses:
    cmd += ['--watch-address', hex(address)]
for pc in pcs:
    cmd += ['--watch-pc', hex(pc)]
subprocess.run(cmd, check=True)
```

On clean source `ac586beb2f32a7837e4cbc42335d6173f53f108e`, exit 0,
778,312 instructions and 388,664 accesses in frames 1576–1618; 1,038 unresolved
accesses, **zero unresolved stores**, complete nontruncated watches. Original
sample digest `72f618f2f7e3416e…` and final state remain unchanged.
Access SHA-256 `8ff540061a4aeb680de3f07b7b5ca740d16bc2e8eca06a57143e73dfac5809c7`;
report SHA-256 `bbd4ba8eada28a7d537cc95382046a4bbc67d6ad3c5c592a963655b9832b81c0`.
The 86 entries and 86 returns delimit two calls per frame; caller order and
scratch load/publication sites must identify the rider, not the shared scratch
address alone. Watch register entries are entry-time observations, not an
implicit snapshot of all scratch memory.

The chronology to preserve when studying the jump/contact contract:

| Writer and watched word | Exact focused observation | Interpretation limit |
| --- | --- | --- |
| `$83:E122 → $0333` | Writes 1 every frame 1577–1613; `$83:E21F` writes 0,1,0,1,0 on 1614–1618 | Opponent input generation; do not substitute player Right input |
| `$82:A914 → $0F91` | Writes 1 once, at 1578 | Initial jump-related latch, distinct from subsequent counter progression |
| `$82:A921 → $0F91` | Clears at 1580 | First later update; not the initial trigger |
| `$82:A92D → $0F93` | Writes 1 through 9 at 1580,1582,…,1596; `$82:A935` clears it in the same frame 1596 | Alternating update schedule; these frames are not nine new jump triggers |
| `$81:9248 → $0F33` | Writes 1 through 9 on every frame 1580–1588 | Separate contact-like state; semantic name still provisional |
| `$81:94CA → $0F33` | Clears at 1617 | Recontact/reset path to explain from instructions, not yet a validated landing model |

Next, correlate these writes by **sequence within each frame** with the two
`$81:8F98` entries/returns, and recover which preprocessed words determine each
branch. Trace the actual producer of `$0333`, the update-phase branch of
`$82:A914–A935`, and the downstream pose selection `$83:EF54–F0F7` and
`$83:ED7B–EF53`. Add watches for producers only when the current trace cannot
resolve their last writer. State the full input/output contract and initial
provenance before any native implementation. M2-01A must preregister a separate
research variation before its capture; the two existing frozen movement
withheld series remain sealed. Do not enlarge this into a broad AI engine.

## Final stable-source component checks

All checks below used clean, unchanged code commit
`ac586beb2f32a7837e4cbc42335d6173f53f108e`. The later handoff adds documentation and the reviewer's authored ordering regression; production source is unchanged. Commands execute from the task root;
ROM-dependent probes require the regenerated content/captures above.

```sh
python3 tools/project.py build --preset lab-debug
python3 tools/project.py test --suite synthetic --report artifacts/m2-01/progress-checkpoint-suite.json
python3 tools/project.py build --preset lab-sanitize --report artifacts/m2-01/progress-sanitize-build.json
local/toolchain/cmake-3.31.10-darwin-arm64/bin/ctest --test-dir build/lab-sanitize -R 'sampling_|progress_' --output-on-failure
python3 -m tools.unirally_lab.native.probe_sampling --access artifacts/m2-01/sampling-access/access.json --content-manifest tests/manifests/native/movement-sampling.content.json --content artifacts/m2-01/content-expanded --probe build/lab-sanitize/tests/native/sampling_probe --coarse-width 1024 --report artifacts/m2-01/sampling-reviewed-sanitize.json
python3 -m tools.unirally_lab.native.probe_progress --sampling-output artifacts/m2-01/sampling-reviewed-sanitize.native.txt --sampling-report artifacts/m2-01/sampling-reviewed-sanitize.json --series artifacts/m2-01/movement-access/wram-series.bin --series-access artifacts/m2-01/movement-access/access.json --content-manifest tests/manifests/native/movement-progress.content.json --content artifacts/m2-01/progress-content/progress-transitions.bin --probe build/lab-sanitize/tests/native/progress_probe --report artifacts/m2-01/progress-reviewed-sanitize.json
```

| Evidence under `artifacts/m2-01/` | Result | Report SHA-256 |
| --- | --- | --- |
| `progress-checkpoint-suite.json` | 211/211 checks, exit 0 | `0d75784a23433cd4acefe59501c06aa55d32d69510de4abeb43b5573838c52a1` |
| `progress-sanitize-build.json` | Build passes; four sampler/progress CTests pass without sanitizer diagnostics | `8886d47e165eff5846229e1368fed7bd1fb30d8ca419920e0b6822fe6f023187` |
| `sampling-reviewed-sanitize.json` | 29,320/29,320 sample words, exit 0 | `4cc22bb6b9aad04453acc5b6e48c5fa3697a03ba9b6d1697e34db95dd0e62ff8` |
| `progress-reviewed-sanitize.json` | 11,728/11,728 progress fields, exit 0 | `3a03aa3a55b63ec57c3859760923cc5f0badb1f67079f141c90affd3be4c8da8` |
| `sampling-final-debug.json` | Fresh debug process, same sample output | `fe150b79b50f9c9797d063ae91ca71ee2d7094c8cf39673aa8cb76d8996d3d4b` |
| `progress-final-debug.json` | Fresh debug process, same progress output | `64606dcc5981d46f08917282300043bb24007f362175af3f602d6b2580f0e5e1` |

The final debug probes use the same two probe commands with
`build/lab-debug`, report stems `sampling-final-debug`/`progress-final-debug`,
and the matching sampler output/report as progress input. Their native stdout
hashes equal the sanitizer runs: sampling
`2cfc5eb7a7bfb63c5a47d44f3abcebd55f876e85258f632ab5b91e249b31ee56`,
progress `acb7a65ca8306dcf99311aca6d2cd4e2ff9144e1a5c4ed06ad465a976e869bb8`.
These are isolated output hashes, **not** hashes of complete future-affecting
movement state. Full movement determinism, first-divergence reporting, primary
and withheld native agreement, and M2-02 continuation remain untested because
there is no autonomous movement update. No proposed `native compare` command
is registered or represented as implemented.


Independent follow-up review of `ac586beb` reproduced 211/211 checks and both
29,320-word sampling and 11,728-field progress probes in debug and sanitizer
builds, approving component integration with no blocking findings. The review
artifacts are under `.worktrees/m2-01-sampling-review/artifacts/progress-review/`
in the coordinator workspace; sampler report is its sibling
`sampling-review/REVIEW.md`. Neither withheld movement series was opened.
The reviewer showed that moving marker observation before progress advancement
survived the old authored test but diverged on primary frame 1576 (opponent
previous tag 2 versus reference 0). The final authored regression now supplies a
new marker in one frame and verifies it only advances the corresponding rider
on its next active phase, including a serialization round-trip. Its data is
synthetic, adapted from the reviewer's `progress-review/boundary.cpp`.


### Final authored-regression recheck

Clean checkpoint `131ecd03eccdd809f76bc094b8a1791578672e04` adds the ordering
regression and handoff documentation; production source is unchanged from
independently approved `ac586beb`. Final debug/sanitizer builds pass and the
full suite remains 211/211. Commands are the final-check commands above with
report paths `final-build-debug.json`, `final-build-sanitize.json` and
`final-suite.json`. The four sanitizer component CTests pass without diagnostics.
Their build/report SHA-256 values are respectively
`193cc8dc7b001c2cea7c358c500add95076512de9afb096de8589597091e3d0e`,
`c8cc429895eef8ecc3813b4c7722a8bcfd60698b88c318cdbbe5ac0b737b479e`,
and `0d1219ef3189f212f5c54d44ba1762a24493cfd986039b10f132dc43db2fca1f`.

The reviewer's ignored `mutant-order.cpp` (same implementation except observing
both new marker arrays before advancing the active rider) was compiled against
the promoted `tests/native/progress_tests.cpp`. It exits 1 with
`progress expectation failed`; the original passes. Command and observed exit
are recorded in `artifacts/m2-01/progress-order-mutation.json`, SHA-256
`dd7c21c59a3afc2a34a12bf2f0be2f650f15c10dc8e4a927a3a9228becd8af3e`.
This deliberate mutant failure is the expected regression outcome, not a
production test failure. The final evidence-only commit does not change code.
