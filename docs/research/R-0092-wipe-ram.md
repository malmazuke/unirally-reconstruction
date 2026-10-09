# R-0092 - The main menu's hidden WIPE RAM menu and factory reset

Status: research result and native change ([WIPE-RAM](../../tasks/WIPE-RAM.md)), 10 October 2026,
on main `33f3645`. PAL ROM SHA-256
`a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`, reference core bsnes
`7d5aa1e6`. Evidence in main `local/evidence/wipe-ram/`.

Tags: **[L]** read from the ROM's bytes, **[C]** confirmed in a capture.

## Domain

Eight cold-start captures with every picture from frame 400 and per-frame work RAM (`captures.sh`):
`confirm`, `cancel`, `main-at-once`, `pad2` (B on the menu, X to cancel, Y back, from pad 2),
`start-held` (a one-frame press does not dismiss), `confirm-long` (Select+Y+A held 150 frames),
`twice` (the code entered repeatedly) and `partial` (Y, A and Select+Y alone, then pad 2 wipes);
the same eight as sound schedules (MODE-AUDIO's tools in `audio/`); and warm boots from three of
SAVE-FILES' saved images through the wipe (`warm/`).

## Findings

- **The menu.** Left+A+L+R on either pad (`$02B0`, R-0054) leads to `$80:A9B4`: `$AC` = 1, `$9D` = 1,
  the entries "WIPE RAM" and "MAIN MENU" (`$80:A9FE`, arrow columns `$80:AA19`) printed by
  `$80:EF4D-EF88` (last entry first, `$80:EF5D`, `$80:EF73`) on the code's frame and slid in under
  the logo from the next. The loop is the shared `$80:B93C-BA69`: Y or X first, then B, Start or A,
  then Down or Select, then Up (the tests at `$80:B983`, `$80:B988`, `$80:B992`, `$80:B997`;
  their branches at `$80:B99C`, `$80:B9A9`, `$80:B9D9`); each pass that stays turns
  the done-track markers (`$80:EB23`, `$77:10A7`). MAIN MENU (`$80:A9CD`) returns to the main loop
  (`$80:A9EB-A9F7`). [L, C]
- **The warning** (`$80:AA1B-AA62`): the logo raised, the text at `$80:AAF9` printed ("- WARNING -
  THIS OPTION WILL RESET YOUR GAME PAK'S MEMORY TO ITS FACTORY DEFAULT ... YOU WILL LOSE ALL YOUR
  RECORDS ETC. ... SELECT+Y+A TO RESET") and slid in (`$80:AA35`). Each frame (`$80:AA38`,
  `$80:AA42`) any button of mask `$9F70` on either pad that is not exactly `$6080` (Select+Y+A)
  cancels, the cancel tested first, so Select+Y+A+B cancels. [L, C]
- **The reset** (`$80:AA64-AABA`): `$83:FB41` clears the 8 KiB, `$CA`, `$CC`, `$CE` and `$D0` are
  cleared, and the cold start's defaults run (`$80:8C74`, `$80:8C4E-8CCA`, `$83:9983-99C1` with
  `$83:9987`; the 8-bit DEC and INC of `$77:074C` at `$80:AA96`/`$80:AAA3`, stored at
  `$80:AA97`/`$80:AAA4`, change nothing). The image
  right after equals `cold_start_cartridge` byte for byte, from a cold start and from the three
  saved images. It spans the press frame and two more without a frame wait; "INITIALISING BATTERY
  RAM" (`$80:AB9E`) prints on the second and shows a frame later, with the logo down (`$0742`
  bit 1 cleared). The three frames are measured, not derived. [L, C]
- **The answers' waits** (`$80:C24C`, then `$80:C206`) need a release and then a press on two
  frames running. A cancel (`$80:AAD7`) prints `$80:ABB9`, lowers the logo and waits the same way.
  Both return to `$80:A9E7`, which goes back to `$80:A9B8` with A = `$80` in `$AC`: the menu slides
  in backwards after an answer. [L, C]
- **Native** (`src/core/wipe_ram.cpp`): the menu, the warning, both answers and the reset, with
  the menus' sounds; the boot's wipe and the menu's share `wipe_cartridge()`. Pack profile v36 adds
  the menu text, its arrow columns and the messages (`front-end.wipe-ram-*`). [C]
- **Results.** Every compared frame equal in all eight captures (arrow, palette cycle, logo, slide,
  decorations, `$9B`, `$8F` and the idle count, the OAM buffer, the text map, every picture:
  12,349 frames). Their sound cues equal line for line. The warm boots' pictures equal on every
  frame; the image after the wipe equals the original's, and at each run's end only `$10A7` differs
  (R-0090's limit). The app's save file after a hidden confirm run equals the runner's image. [C]

## Not covered

- `$10A7` is neither written to nor read from the image (R-0090): after a menu visit a native save
  keeps the boot's value, and a warm boot's hidden marker tiles start from 0.
- The reset clears `$CA` with `$CC`, `$CE` and `$D0`; native keeps no field for `$CA`, and no
  capture shows an effect.
- `$0742` bits 9 and 10 (which pad the menus read) are not modelled; native reads both, as its
  main menu does.
- The app's audio hand-off on the code is compared through the runner's cues, not live.
