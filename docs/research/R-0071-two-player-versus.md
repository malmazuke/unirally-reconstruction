# R-0071 - Two-player and versus mode entry

Status: active investigation for [TWO-PLAYER-VS](../../tasks/TWO-PLAYER-VS.md), 29 September 2026. These are observations of the original, not a native accuracy claim.

## Identity and method

- PAL ROM SHA-256 `a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`; Strict bsnes lock commit `7d5aa1e656b9171524d01b1b22917197d8121cb4`, patch SHA-256 `a719f5ffe2222dad4c1ab04336633319ad85004f74e32fc14893a058be333885` (built core reports `e59bf88d4fc9`).
- Original input and full-WRAM/video hash evidence: `local/evidence/two-player-vs/` in the main checkout. The mode-1 and mode-2 `*-repeat/samples.json` runs reproduce their respective COVERAGE-ROADMAP captures on all 3,000 frames: zero differing whole-WRAM hashes and zero differing video hashes. Both use a cold start, Start at frame 300, one or two main-menu Down presses and Start at 620. The first rider is chosen with port 1 Start at frame 900.
- The pre-existing static listing `artifacts/static-map/bank-80.lst` has `$80:BCBF` (2P) and `$80:BF49` (VS) as unknown bytes. After the new port-2 selection captures, temporary maps under the task worktree's ignored `artifacts/two-player-vs/temp-maps/` and the regenerated ignored `static-mode/bank-80.lst` decode these as observed instructions. Across the two captures, 8,254 sites decode with zero address/length disagreements. This listing is the source for the routine reading below; the captures are the dynamic evidence.

## Reached selection path

Both modes show their first rider screen by frame 700 and a second rider screen by frame 1000. Mode 1 calls it PICK ANOTHER; mode 2 calls it PICK PLAYER TWO. At frame 900, both have selected rider 0 (MIKE, `$7E:017D=0`). The second screen starts with rider 0 at the arrow. Repeated Start on port 1, Start on port 2 without moving, or port 1 Down and Start leave the second screen in place through at least frame 1500.

Port 2 Down at frames 1120-1125 moves the second arrow to row 1. Port 2 Start at 1200 stores rider 2 (MARTIN) at `$7E:017F` on that frame and reaches PICK TOUR by frame 1250 in both modes. The same Down/Start sequence on port 1 does not leave the second rider screen. The exact tested domain is a cold start with the stated pad pulses and the first 1,600 frames; no general rule for every menu input is claimed.

The newly observed listing explains the repeated same-rider choice: 2P `$80:BCFB-BD00` and VS `$80:BF85-BF88` compare the second choice with `$017D` and branch back to the second-rider prompt when equal. The accepted choice is written to `$017F` at `$80:BD02-BD05` or `$80:BF8A-BF8D`. Both handlers then set `$77:0742` bit 3, copy it to `$77:0750` and enter the shared tour flow. VS also sets bit 2 at `$80:BF94-BF98`. The latter flag interpretation is still provisional until the reached race/result path is measured.

## Next experiment

The `mode-1-continuation` and `mode-2-continuation` cold captures extend the successful choice with port 1 Start on frames 1550, 1700 and 1850. Both show PICK TRACK at 1600, NOW PLAYING with MIKE and MARTIN at 1750, black from 1857-1994, and a split DRAGSTER race from 1995. Presses on frames 2000, 2200 and 2400 were also in the exploratory manifests; they may affect the race and are not a neutral baseline. Video hashes are equal between modes on every race frame 1995-3199, but their NOW PLAYING hashes differ on 118 frames 1739-1856 even though the retained frame-1750 pictures look alike at a glance. Those pictures need a pixel comparison and explanation before reuse.

Fresh access captures `mode-{1,2}-race-access` cover instructions 1850-2100 and retain all work RAM bytes `$0000-$21FF` at every frame through 3199. Their sample digests equal their coverage continuation runs. Both set split flag `$7E:0DE1=1` at frame 1899 and keep demo flag `$7E:212C=0`. The access reads show SRAM `$77:0742=0x0A` and `$77:0750=0xC208` in 2P, versus `$77:0742=0x0E` and `$77:0750=0xC20C` in VS. Pairing `$77:0748/$0749=0/2` is written at frame 1857 on both. By frame 2500, the two modes' work RAM differs on only six menu or saved-menu bytes in the captured range (`$00B4`, `$0191`, `$0193`, `$0194`, `$01FB`, `$01FC`); that is an observation of these inputs, not proof the modes share all later gameplay rules.

The bounded D-0004 architecture consultation found risks in native mode identity, port-2 delivery through the menu-race bridge, one-player scenario validation, finish/result rules and split DRAGSTER serialization. These are source-inspection hypotheses, not original-game findings. The first internal slice is both native menu paths through split DRAGSTER initialization and 300 asymmetric live updates, followed by complete result and return recovery. Do not accept the slice as the task outcome.

Next: make a neutral race capture with no post-entry Start, use distinct port-1 and port-2 controls, and compare both riders' work RAM projections. Measure natural finishes and result/return, including each finish order, before assuming the one-player result path applies.

## First finish and result paths

The `mode-1-both-right` and `mode-2-both-right` original captures remove the exploratory race-time Start pulses and hold Right on both ports from 2250-4500. Both riders naturally finish DRAGSTER: MARTIN (port 2) has 0:34.37 and MIKE (port 1) 0:34.41. Both modes show a result by frame 4400; their captured frame-4400 and frame-5000 pictures show the same names and times. The 2P-only `first-right` and `second-right` runs drive only the named rider; the other remains active and no result screen appears through frame 5499. This bounds the observed requirement that both riders finish before this result path, rather than proving every finish or timeout branch.

`mode-{1,2}-result-exit` repeat the both-right paths and press port 1 Start at 5200. At frame 5300, 2P shows a five-choice continuation screen: NEXT TRACK, SAME TRACK, SELECT TRACK, SELECT TOUR, QUIT. VS instead shows VS CHAMPIONS with the winner at the top and wins/today/percentage columns. Both captures run through frame 6499, still on those screens with no further pad input. Their later paths and persistence are separate recovery work, despite the shared setup and first race. These references have not yet been compared with native.
