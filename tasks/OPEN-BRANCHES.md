# OPEN-BRANCHES - the reachable branches no capture of the original has run

- Status: **queued** 8 November 2026 by [COVERAGE-CENSUS](COVERAGE-CENSUS.md) (R-0109, queue item 4).
- Milestone: M4. Review tier: **1** (race and front-end state).

R-0109's code whose reachability is open and the branches the records list as never captured: the
HUD's fields of 10 and more (R-0043), the `$77:0749 = $FF` path (R-0058), `$83:9B49`'s arm and
`$83:9C47`, the rider sprite-tile queue's last slots (`$82:BFC9-C53C`, `$82:CFF3-D194`), the loop
descriptor facing away (R-0108), a stunt score of 1000 (R-0107), track 13's cell past its flag table
(R-0107), 2P and VS menus on the locked tours' tracks, a race tie, NEXT TRACK's wrap after track 44
(R-0095), demo cycles past the twentieth title (R-0096), and the never-captured sounds (corkscrew
ejection, the 9:5x warning, the cheat page reveal, the slow checkpoint, BRONSEN's praise, R-0077).
Outcome: for each, a route of real inputs captured on the original and compared with native, or a
recorded argument that it is unreachable; native changed only where it differs.
