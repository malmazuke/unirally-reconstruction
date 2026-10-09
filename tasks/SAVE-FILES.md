# SAVE-FILES - the cartridge RAM saved between app runs

## Assignment

- Status: **in progress**, claimed 9 October 2026 (08:15 UTC) on main `3191228`, in the session the
  user asked to keep working until weekly usage reaches 50% (8% at claim). Queued the same day by
  [COVERAGE-GAPS](COVERAGE-GAPS.md) (R-0089, queue item 1).
- Worker: Claude Code desktop app, Claude Opus 5.5 (`claude-opus-5-5`); coordinator, primary and
  integrator. Branch `task/save-files` in `.worktrees/save-files`.
- Task provider: Anthropic.
- Milestone: M4 (original game coverage).
- Review tier: **1** (D-0008: it changes the state the game boots with and the bytes it keeps).
- Dependencies: OPTIONS (R-0072), LEAGUE (R-0073), FRONT-END-1P-CONTINUATION (R-0057).

## Why

The app never writes cartridge RAM. Records, player names, leagues, medals and unlocked tours are
lost when it quits, so the native game cannot be played as the original is: over many sessions.
Native keeps the cartridge RAM as fields (`OnePlayerRecords` in `src/core/front_end.hpp`, with
their `$77:xxxx` addresses), but not as the original's 8 KB image: the record checksum
`$77:054E`, the statistics checksum `$77:02B0` and the score history `$77:0618` are not all kept,
and several bytes have only provisional meanings (R-0072, R-0067).

## Outcome

- One native image of the cartridge RAM (`$77:0000-1FFF`), equal to the original's at every point
  the original writes it, in the tested domain: built from native state and read back into it.
- The app loads it at boot and writes it to disk when the original writes cartridge RAM; with no
  file it starts as a cold start does now. A file the original wrote (an emulator's `.srm` of
  this ROM) loads too.
- The original's boot with populated cartridge RAM (its checksum test and what it does with a bad
  one) captured and native, since every capture so far starts from an empty or preloaded image.

## Evidence to gather

- Captures of the boot with saved cartridge RAM (a state origin, or `cartridge_ram_writes`
  before the boot reads it), with good and bad checksums.
- Whole-image comparisons at the save points of the existing corpus (OPTIONS, LEAGUE, the
  one-player continuation, HUNTER's ending), against `memory.sram` or the capture's SRAM rows.
- Round trips: every accepted front-end manifest's final state saved, reloaded, and played on to
  the same rows.

## Not in scope

The WIPE RAM menu (queue item 3, which builds on this image) and any change to what the game
stores.

## Checkpoint - 9 October 2026 11:30 UTC

- **Measured first.** Native's lab image (`front_end_runner --records`) against the original's over
  the R-0089 corpus: 458 bytes differed somewhere. A perturbation of each range in saved images
  found which a later boot reads (R-0090). The boot keeps an image that starts with the signature
  `$83:8000` and wipes any other.
- **Native.** `src/core/cartridge_ram.{hpp,cpp}`: `cartridge_image` writes every field native keeps
  at its address over `FrontEndState::cartridge` (the bytes native keeps no field for, as loaded
  or as the cold start left them); `cold_start_cartridge` runs the audio model's default writes
  (byte-equal to the original's image after frame 429); `insert_cartridge` sets the image power-on
  finds. `check_records` keeps a signed image's records (`records_kept`) and the boot skips the
  wipe's three frames, as after a soft reset. The runner takes `--cartridge-in`; `--records`
  writes the whole image. The app takes `--save-file` (and `frontend run --save-file`), loads it
  at power-on, hands it to the audio model too and writes it whenever it changes.
- **Results.** Every record field equal at every compared frame in 130 of 236 corpus runs; the
  rest are write timing, the `$0618` overlap and other modes' gaps (league, two-human restart, VS
  mode 2). Warm boots from three of the original's saves show the same screens; pictures differ
  around the sound handshakes (14,809 of 18,750 every-5th pictures equal), a known class.

## Review candidate

- Records: [R-0090](../docs/research/R-0090-save-files.md).
- Gates: `local/evidence/save-files/gates.sh` against main `3191228`'s binaries
  (`base-3191228/`). The cold path must not move: every frozen gate, sweep and capture as on main.
- Review tier 1: it changes the boot's state and timing when an image is found, and the app writes
  files.

## Review - round 1 (returned)

- [Review](https://github.com/malmazuke/unirally-reconstruction/pull/67#pullrequestreview-5468330240)
  on `8702bf3`: return. Blockers: (1) a session that ended before boot frame 404 saved blank
  records (native read the image only at the check); (2) the title code entered with a save, and
  a save made with the cheat on, differed from the original (the same cause). Should-fix: (3)
  `$10AD` = 1 in a one-player save colours the menu arrow; native drew the menu's colours.
  Advisories: the twelve corpus manifests that write cartridge RAM ran without the writes; the
  runner's message for a missing `--cartridge-in`; the test's place and format, literal sizes.
- Fixed: `insert_cartridge` reads the image's fields at power-on; the check keeps or wipes them
  and clears `$10AD-10AE`; the image writes `$10AD` from `one_player`. The review's failing
  cases now match on every picture (`m149`, `m043` with the title code; the cheat save with three
  scripts; `xad`; `m025`), and a run ended at 100, 402 or 403 frames leaves the save unchanged.
  The advisories are done; the gate run on `8702bf3` was stopped at the differential gates.

## Review - round 2 (approved)

- [Review](https://github.com/malmazuke/unirally-reconstruction/pull/67#pullrequestreview-5468474061)
  on `4ed4a71`: approve. Every round-1 finding verified fixed (early quits at 1-410 updates on four
  saves, the title code and cheat saves on every picture, the arrow's colours), the cold path
  unchanged on 8 manifests, five unsigned or garbage images still wiped as the original wipes
  them, and the app round trip exact. Two advisories, recorded in R-0090: `$0742` bit 1 is cleared
  at power-on, not frame 24; the audio model's warm power-on does not read the cheat flag and
  `$10AD` (queued to MODE-AUDIO).

## Gates - head `4ed4a71` (`local/evidence/save-files/gates-4ed4a71.out`)

- Four presets build, ctest 41 of 41 each (ASan presets unavailable on this host; Linux CI covers
  them); synthetic suite, v1 contracts, eight hidden app runs and the front end passed. Fuzz as on
  main (40 seeds, recorded non-pass).
- Eleven differential gates passed with main's row digests; race equivalence sweep 432 runs, 0
  differences; the front-end sweep 179 of 179 manifests equal main's runner; every pause, split,
  two-pad, race-end, look and restart comparison as recorded; the original's track sweeps give
  main's rows; R-0076's cue schedules identical; the idle demo lap 34 of 34 exact, 963 pictures.
- SAVE-FILES: the cold image at frame 429 equals the original's; the image sweep as R-0090; the
  warm sweep 14,809 of 18,750 pictures, end images agreeing on the record bytes in 20 of 21 (the
  VS race's personal best, R-0090); the app with no save file and with the all-gold save writes
  the runner's image.
- Tooling tests 554 passed; native symbols current. Two functions were over 80 lines
  (`front_end_content`, the runner's `main`): split in the follow-up commit, which moves lines into
  two helpers and changes no behaviour; its gates are rerun below.
