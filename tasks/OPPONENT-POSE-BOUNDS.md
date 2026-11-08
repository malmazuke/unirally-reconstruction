# OPPONENT-POSE-BOUNDS - a computer opponent's colour maths read past its table

- Status: **queued** 9 November 2026 by [SAVE-IMAGE-EXACT](SAVE-IMAGE-EXACT.md) (review round 1).
- Milestone: M4. Review tier: **1** (race presentation content for a scenario the menus reach).

A libc++-hardened debug build (`_LIBCPP_HARDENING_MODE_DEBUG`) aborts on the corpus route `c175`
(`hunter-effects`, R-0109): `presentation.cpp:243`
`colour_math.subspan(colour_math_size * scenario.pairing.opponent, colour_math_size)` with the
computer opponent `0x14` (ANTI-UNI), called from `classic_race_presentation_content` from the
runner's `start_race`. The release build reads past the table without a check; a Linux debug run of
that route would abort. The code and the scenario are main's (SAVE-IMAGE-EXACT did not change them).
Outcome: the original's colour maths for opponents past the table recovered from the listing and
used, or the read bounded as the original's is, with a test on opponent `0x14` and the route's
pictures unchanged.
