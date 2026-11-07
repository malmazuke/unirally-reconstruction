# SAVE-IMAGE-EXACT - the saved image equal to the original's at every save point

- Status: **queued** 8 November 2026 by [COVERAGE-CENSUS](COVERAGE-CENSUS.md) (R-0109, queue item 2).
- Milestone: M4. Review tier: **1** (cartridge RAM state and its write timing).

SAVE-FILES (R-0090) keeps the whole image, but bytes native keeps no field for stay as loaded or as
the cold start left them: `$074C-074F`, `$10AD`'s values other than 1, `$10A7` written back as
loaded, `$0750`'s race words, the league totals a 2P or VS result writes (`$0618 + 16 x slot`,
R-0095), and the write timing of `$10B1` and `$1115-1117`; a save taken during a race lacks what the
race writes until it returns (R-0090, R-0095, R-0100, R-0105). Outcome: native's image equals the
original's byte for byte at every frame the original writes cartridge RAM, over the corpus's warm and
cold routes (the ones R-0109 lists), or each remaining byte is recorded with why it cannot matter.
