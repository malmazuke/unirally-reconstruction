# SAVE-IMAGE-REMAINDER - the saved image's differences R-0111 leaves without an argument

## Assignment

- Status: **claimed** 12 November 2026 06:45 UTC on main `5c7522ee`, under the user's instruction of
  12 November ("you can continue working on new tasks until you get to 90% weekly usage, or the reset
  happens (7PM tonight)"), after PICTURE-RESIDUALS #104. Queued 9 November 2026 by
  [SAVE-IMAGE-EXACT](SAVE-IMAGE-EXACT.md) (R-0111, review round 1). Claude Code desktop app, Claude
  Opus 5.5 (`claude-opus-5-5`): coordinator and integrator; a worker subagent captures and
  implements, a fresh subagent reviews. Run under
  [D-0013](../docs/decisions/D-0013-task-time-and-disk-budget.md): claim to merge against the
  6-hour target, new evidence under 1 GB. Usage at the claim: 5-hour 1%, weekly 41%.
- Branch `task/save-image-remainder` in `.worktrees/save-image-remainder`; evidence in main
  `local/evidence/save-image-remainder/`.
- Task provider: Anthropic.
- Milestone: M4. Review tier: **1** (race state that reaches a saved record, and cartridge RAM).

R-0111 leaves 14,380 of the original's write frames (from frame 406) differing outside the bytes no
later boot reads, on 207 of 916 routes; most are bounded timing shifts it argues, but some are values
a player sees or a later boot reads. First, the personal bests on four `hunter-effects` routes
(`c175`, `c177`, `c210`, `c220`): native's race adds to `$77:085F` (or `$085B`) at frames 1973 and
1991 through a computer opponent's voice class read past rider 1's tallies
(`race_update.cpp:852`, `increment_opponent_tally_byte`, R-0098), where the original writes nothing;
main does the same, so this is a race difference that lands in a saved record: find which class the
original takes there. Second, the endings' own `$77:10A7` stores (`$83:B649` on, 7 routes), which a
later boot reads as the markers' phase. Third, the rest of R-0111's Value list: the name bytes after
a terminator (RENAME's keyboard leftovers), `$0742` bit 10 in one fuzz run, `$1116` on three
track_reference runs, `$10B1` and `$074D` at the end of `league/organic-full-tour`, and the league
pair's swap (`$83:95C4`, `$83:95CA`, `$0748-0749` and the race words), each modelled or argued
unreachable or unread.

Outcome: each item native's image matches the original's at every write frame of the corpus
(`local/evidence/save-image-exact/check_image.sh`, its `expected.json` re-recorded), or is recorded
with why it cannot matter; the personal-best increments fixed in the race if the original takes
another class there.

## Handoff

Not started.
