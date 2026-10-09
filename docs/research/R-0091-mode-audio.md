# R-0091 - Native audio in 2P, VS, league and OPTIONS

Status: research result and native change ([MODE-AUDIO](../../tasks/MODE-AUDIO.md)), 10 October
2026, on main `a54341a`. PAL ROM SHA-256
`a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`; bsnes `7d5aa1e6` with
R-0075's audio observation core v5, Strict synchronization. It extends
[R-0077](R-0077-one-player-audio.md) under
[D-0010](../decisions/D-0010-frame-anchored-sound-commands.md). Evidence in main
`local/evidence/mode-audio/`.

Tags: **[L]** listing only, **[C]** confirmed in a capture.

## Method

R-0077's, with its scripts copied: each schedule is captured twice from a cold start watching
every dispatcher call site (`captures.sh`), the cue list derived (`derive_cues2.py`) and compared
with `front_end_runner --sound-cues` frame by frame (`cues.sh`, `diff_cues.py`). Two more captures
watch every `JSL $82:8000` site (`captures_enq.sh`, `enqueues.py`) to name each sound's helper.
Six schedules from corpus manifests:

| Schedule | Manifest | Frames |
|---|---|---:|
| `options-rename` | `options/rename-g-finish` | 2,350 |
| `options-league` | `options/define-league-confirm` | 2,100 |
| `twop-next` | `two-player-vs/mode-1-next-race-entry` | 7,800 |
| `vs-champions` | `two-player-vs/mode-2-champions-next` | 7,600 |
| `league-two` | `league/organic-two-race` | 12,000 |
| `demo-cold` | `idle-demo-cycles/cold`, first 10,000 frames | 10,000 |

## Findings

- **The menus' sounds.** Every OPTIONS, league and two-player menu sound is one of the helpers at
  `$80:B10F-B18C` (result, select, the two slides, a move), or `$80:B0FA`, a move's cue from its
  own helper played where a choice is refused (an empty name, a league of fewer than two or more
  than eight riders). Native called none of them outside the one-player screens. Now: [L, C]
  - OPTIONS, RECORDS and the league slots: a move (`$80:B9B1` Up, `$80:BA13` Down), the slot's
    choice (`$80:9F15`).
  - The keyboard: each direction (`$80:A2BA`, `$80:A2F9`, `$80:A326`, `$80:A34A`), a character
    typed (`$80:A449`, before the length test), a deletion (`$80:A420`), OK (`$80:A495`), and
    the refusal of an empty name (`$80:A493`, `$80:A416-A41A`).
  - The league's members: marking or clearing a rider (result), a refused count on every frame
    Start is held.
  - The league's awards: the back slide on their third frame (`$80:8B28`) and the result
    (`$80:B051`).
  - Picks: the select sound for a rider's choice (`$80:BBB8`, `$80:BCDB`, `$80:BD02`,
    `$80:BF8A`, `$80:C0BD`, VS's first pick even when backing out, `$80:BF5C`), none in DEFINE
    PLAYER and RENAME (`$80:C13D`, `$80:D45B`).
  - The second player's menu (`$80:CBC3`, from `$80:BCEF` and `$80:BF79`) calls neither slide
    helper: native's slide there is now silent.
  - The two-player continuation: a move and NEXT TRACK's select (`$80:AEAD`).
- **The split race's finish fade** has its own copy at `$83:E7F2-E7FD`; native reports it as its
  own site (`finish2`), anchored at 26,966 master clocks after its frame's boundary, the median
  of `twop-next`'s 61 calls (25,450-33,920). [C]
- **Cues.** `options-rename` (1,765 lines) and `options-league` (1,496) are equal line for line.
  `twop-next` differs on 2 frames of 10,259 lines, `vs-champions` on 4 of 9,421, `league-two` on
  2 of 13,834: each a slide's sound a frame or more from the original's because the slide itself
  starts there, R-0071's, R-0084's and R-0073's recorded picture residuals (the continuation's
  slide a frame late, VS CHAMPIONS' back slide 7 frames early, PICK CHALLENGER's slide, the league
  table after the awards). The one-player schedules of R-0077 stay equal. [C]
- **Measured agreement** (`measure.sh`, a third capture of the schedule with DSP rows): [C]

| Schedule | Commands in frame | Identical pairs | Windows | Median level difference | 90th / 99th percentile | Within 1 dB |
|---|---|---|---|---|---|---|
| `options-rename` | 47 of 47 | 853,934 of 1,505,495 | 2,249 | 0.00 dB | 0.07 / 0.58 dB | 99.6% |
| `twop-next` | 120 of 123 (3 a frame off) | 1,286,680 of 4,997,367 | 7,340 | 0.13 dB | 0.69 / 2.44 dB | 95.7% |

- **The app** keeps its cued producer running past any main menu choice but the idle demo, and
  stops it for a race with no measured loading, as before (R-0077: local and league races on
  tracks 0-4).
- **The idle demo is silent in native.** The original reloads the title set at each demo's title
  (`demo-cold`: frames 957, 3353, 4004, ...), plays a slide, and runs the race's dispatches and
  effects with that set; native's demo reports no cues at all (5,712 differing frames). Queued as
  DEMO-AUDIO.

## Not covered

- The demo's sound (above).
- The slide residuals' sounds keep their slides' frames.
- Read from the listing while placing the sounds, not native: the keyboard's Start goes straight
  to OK (`$80:B916`) and held directions repeat; the league member picker takes A as well as
  Start (`$80:B8C4`) and marks with B, Y or X; VS refuses the champion as challenger
  (`$80:C0B1-C0BB`, with the refusal's sound); the awards take a press only after `$80:B051`;
  and the original's second-player menu does not slide at all. Queued with CARTRIDGE-OPTION-BITS'
  menu work (R-0089 item 7) as MENU-INPUT.
- SAVE-FILES' advisory: the audio model's warm power-on title work (R-0090) is still open.
- PCM measured on two schedules only.
