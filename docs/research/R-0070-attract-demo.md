# R-0070 - second idle demo cycle

Status: candidate evidence for [ATTRACT-DEMO](../../tasks/ATTRACT-DEMO.md). The tested domain
is a PAL cold power-on with Start held on frames 300-305, both pads then released through
frame 6999. Two separate runs press port 1 A at frame 5000 or port 2 B at frame 5200. The
claim covers this second cycle through its main-menu return, not later demo cycles or the
2P/VS menu paths.

## Identities and method

- ROM SHA-256 `a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`;
  pinned bsnes libretro SHA-256 `e59bf88d4fc922c9fe3b5438e65ff3a6909d24e1628f0f87141c8de17699a91b`.
  Captures use Strict serialization and a fresh core with blank persistent memory.
- The first-cycle [R-0069](R-0069-split-screen-race.md) capture's whole-WRAM, SRAM and
  video hashes for frames 1300-6999 repeat in a fresh cold run at
  `local/evidence/attract-demo/reference-repeat/` on all 5,700 frames. A separate fresh
  whole-memory run authenticated the 565-byte second-race projection against those
  hashes on all 1,901 frames 4523-6423 (`authentication.json`). The frozen projection
  is `second-race-reference.txt`, SHA-256
  `1deef31035f1ea697eb32ade6f6631a3a301241a1ab249f336556f4ff49f661b`.
- Evidence lives in the main checkout at `local/evidence/attract-demo/`, including the
  capture scripts, inputs, projections, bounded access traces, pictures, comparisons and
  save/continuation outputs. The worktree links to that home; it stores no evidence.

## Verified observations

1. The first demo returns and the idle counter reaches zero at frame 3946. Mode 5 begins
   at 3947. At 4399 SRAM holds track 3 and race type 0. The split flag `$0DE1` clears at
   4406; at 4407 SRAM rider/opponent become 6/1 and option word `$77:0750` is `0xC202`.
   The race initializes at frame 4523, with `$7E:212C=1`. `$1387` reaches `0x076B` at
   6422 and `$12B3` requests exit at 6423. The menu idle state resumes at 6541.
2. The static code map `artifacts/static-map/bank-83.lst` reads `$83:CD50-CD61` as a call
   to `$83:E254` demo controls followed by `$83:E082` opponent AI when `$0DE1=0`.
   `second-entry-access` observes both calls on frame 4524. Its instruction-level
   writes show the demo routine writing horizontal 2 to `$0319`, then AI writing 2 to
   `$031B`, before `$83:E794/E79F` publishes the input words. This ordering explains
   why the two-view path cannot simply be reused for the one-view race.
3. The static map's `$81:A23C-A2D5` is the second camera follow. In the one-view demo,
   its x/y remain `0x0530/0x0120` and its OAM word `0x2065`, while velocity and
   lookahead advance. The original whole-WRAM trace and native 42-byte trailer agree
   on all 1,901 frames. `contact-access` dynamically observes `$83:E3E3` copy the
   alternating `$0300` contact phase to `$0331` for a descending rider off the surface
   on frames 4766-4770. Modeling that removes six one-byte differences at frames
   4767, 4769, 4849, 4851, 4853 and 4855.
4. The native `URTR0308` one-view demo is 958 bytes: the ordinary 916-byte track-3
   race state plus the existing 42-byte demo trailer. Bytes 12-564 of its state and
   the trailer's corresponding original WRAM/SRAM fields agree on every frame
   4523-6423. The magic identifies the new layout; the frame label is absolute
   here. Native restore at frames 4523, 5000, 6300 and 6420 reproduces the
   uninterrupted serialization through 6423 on 1,901, 1,424, 124 and 4 rows.
5. The second title holds full brightness one picture longer before its fade than the
   first title. `second-title-fade` confirms the repeat's original hashes at
   4399-4406. Native RGB equals the original on 22 retained pictures spanning
   3946-6999, including the transition, race, timer exit and menu return
   (`picture-report.json`). These are samples, not all pictures in the interval.
6. Two fresh independent control runs exit at the pressed frame: port 1 A at 5000
   and port 2 B at 5200. Their full-WRAM/SRAM/video hashes agree with the cold
   baseline on every race frame before the press. Native projected race bytes
   12-564 agree on all 478 and 678 original rows through each exit; the frozen
   original projections have SHA-256
   `0794dfd54b80d4236f46301e477eef799f4f59687cc4b6d7abb2c80c540b6128`
   and `e49c44ec05be14201d53e1d2fcacb93f4f1104585e80f9e92492da204f4cc776`.
   The interrupted return stays black two extra frames compared with the timer
   return. Its `$00C8/$00C9` palette cycle starts one picture earlier relative to
   the delayed return script: at frame 5104 the original is 5/3 and a native
   hook started at the timer path's point was 6/3. After modeling those measured
   timings, 53 dense pictures on the port 1 return at 5085-5121 and 5185-5200,
   and seven port 2 pictures at 5199, 5200, 5201, 5230, 5309, 5318 and 5400,
   have zero differing pixels. The source of the original's two extra blank
   frames remains a hypothesis; the observed frame schedule is the implemented
   domain.

## Reproduction and limits

From the main checkout, `local/evidence/attract-demo/authenticate_second.py` verifies
the full-memory race projection, `picture_compare.py` regenerates the 22 normal-path
pictures, and `capture_interrupt.py` repeats either control run. Use
`front_end_runner --content-pack local/classic-pal-crawler-tracks-v26.pack --frames 7000
--inputs local/evidence/split-screen-race/title-start-300-305.inputs --race-timeline
OUT.txt` for the native timeline. The private ROM, content pack and screenshots stay
ignored. The original and native state formats are compared only on established
fields: bytes 12-564 and the 42 demo/camera fields, not unrelated whole-WRAM
locations. Audio, arbitrary input during the demo, later cycles and other menu
modes have not been established by these captures.
