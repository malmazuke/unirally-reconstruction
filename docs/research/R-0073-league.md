# R-0073 - LEAGUE tournament investigation

## Identity and method

PAL ROM SHA-256 `a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`, pinned bsnes `7d5aa1e656b9171524d01b1b22917197d8121cb4`, patch `a719f5ffe2222dad4c1ab04336633319ad85004f74e32fc14893a058be333885`. Main `local/evidence/league/` contains JSON controller manifests and ignored probe.py retaining selected pictures, WRAM and SRAM. Coverage capture independently checks ROM/core identity. Commands run from the task checkout. See [LEAGUE](../../tasks/LEAGUE.md).

## Verified observations

1. organic-two-entry.json organically defines OPTIONS slot ONE as T with TONY/COLIN, returns to main, chooses LEAGUE at 5540 and ONE at 5750. Its coverage gate passes. Retained pictures show slots at 5600/5700, PICK TOUR at 5850, LEAGUE TABLE with zero played/points at 6200, NOW PLAYING COLIN against TONY on DRAGSTER at 6600 and the race at 7400. No native comparison exists.
2. organic-two-race.json holds both Right from 7100 through 10499, then acknowledges at 10600/11000/11400. DRAGSTER COMPLETE at 9700/10000 gives TONY 0:32.90 and COLIN 0:32.93. POINT AWARDS at 10700 shows TONY 10, COLIN 9 and MOST STUNT BONUS: COLIN. First-slot played words `$77:02C0/02C4` become 1 and points `$77:02C2/02C6` become 9/10. Pair list `$77:05E8-05EF` changes `07 09 10 10 10 10 10 10` to `09 07 10 10 10 10 10 10`; track byte `$77:067E` becomes 1. At 11100 standings reorder TONY first; at 11500 NOW PLAYING is COLIN against TONY over three laps on ZOOM ZOO. This is original-only evidence, not a scoring-formula or native-acceptance claim.

## Static hypotheses

Read `artifacts/static-map/bank-80.lst` first; the task's ignored dynamically seeded `local/evidence/league/listing/bank-80.lst` and bank-83.lst resolve executed handler widths. `$80:BDD4-BF48` reads saved slot/cursors; `$80:AD47` appears to sort score records into pairing order; `$80:AF54` advances played counters and tracks; `$83:98AB` has a sentinel rider branch; `$83:95CA` may swap participants/results after loss. These are listing-derived hypotheses pending dynamic tests. Existing native comment about adjacent row indices conflicts with the observed first-slot played counters.

## Failed approach and next experiment

organic-two-recon.json selects OPTIONS because Up after main-menu return wraps its reset selection 0 to 4. Corrected entry uses three Downs. Next: opposite outcome, odd membership, score-write timing and exit/resume, then native standings/setup comparisons. Captured state remains laboratory evidence only, never product input.
