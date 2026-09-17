# R-0035 — Native ZOOM ZOO initialization and playable recovery

Status: **incomplete, unaccepted M4-16 experiment**. The accepted product remains
DRAGSTER; accepted ZOOM ZOO evidence remains M4-15's seeded laboratory domain.
This record consolidates source observations and implementation decisions, not
complete gameplay or presentation acceptance. See [task](../../tasks/M4-16.md)
and [independent review](../../tasks/M4-16-review.md).

## Identity and tested domain

PAL ROM SHA256 `a1105819d48c04d680c8292bbfa9abbce05224f1bc231afd66af43b7e0a1fd4e`;
private audited bsnes core SHA256
`e59bf88d4fc922c9fe3b5438e65ff3a6909d24e1628f0f87141c8de17699a91b`.
One-player MIKE/BRONSEN CRAWLER/ZOOM ZOO, three laps, fresh scenario defaults.
Original pairs cold-start through frame7600; all WRAM/SRAM images and controller
timeline are authenticated before native evaluation. Native starts at end1376
with static content and controllers only. Primary input is inherited from the
M4-15 manifest through6724, then neutral. Experimental overlays are retained in
tracked `tests/manifests/native/zoom-zoo-playable-*.case.json` and freeze files.

Current experimental `URZZ000B` is742 bytes: the prior730-byte inventory plus
four original pause bytes and eight semantic suspended-update clock bytes.
Historical accepted URZZ0001/2/3 remain readable; unaccepted intermediate formats
are retained as evidence, not compatibility promises. Packv5 has50 static entries,
including the625-byte combination table frozen before native tuning.

## Recovered producers and decisions

| Domain | Original source and verified observation | Native decision / remaining limit |
| --- | --- | --- |
| Clean scenario | $82D7C6–D7FA clears runtime; $82D89D–D904 derives positions/cameras from track header; $82DB25–DB7F fills lap/total sentinels; $82DB96–DBBD sets laps and cap448 | Derive from static header and explicit fresh scenario. No end1649 seed. Initial OAM X101 is explicit $82D76D–D76F. |
| Countdown | Fade $0FF1 grows to30; countdown waits until fade5. Start boosts $1261/1263 initialize384; countdown/control ordering clears or consumes them | Preserve original integer ordering at130/100/70 and timer below68. Initial fade draws prior update's value ($80883F–8849). |
| Manual turn | $82A35B–A49E controls A reflection; $82A49F–A5F9 changes L/R rate under A | Tested A, Up, Down and Select overlays complete. Up/Down/Select have no extra gameplay effect in these tested sequences; not a universal inertness claim. |
| Announcements | $829995–9A49 charge latches; $81C598–C5C8 enqueues; $81BEA8–BEF1 idle display; $81C0CE–C18A consumes weights/boost; $81C02A–C054 cooldown; $83CDBC–CE43 tutorial | Native player queue, hints and reached event1/17 reward feedback. Event/class arrays are static. Other compound reward producers remain incomplete. |
| Roll | $829398–9714 X entry, signed step, held/return poses and reflection; $829C98–9CA7 held landing reward | Full X, release/landing and re-press sequences recovered. Bounce charge/active/prior-step continuations remain unsupported and rejected at restore. |
| Roll content | $0080E8 pose table128 bytes; $008168 direction64; $82D7A4 learned weights26; $82DB87–DB94 copies weights | Authenticated static pack entries, never runtime state. $054B/054D mirrors support count ($818E06/$818F5A), not a bounce phase. |
| Camera | Original HDMA frame1382–6724 uses prior camera minus initial origin8944/1232; BG2 shifts wrapped word right once | Match all5343 observed mappings. Initial black frames do not prove visible alignment. |
| Result | $83904A–90F0 publishes graph extrema; $80F88D publishes totals. Primary load6725, visible6833, stable6839 | Archive final race when original WRAM becomes graphics; independently project graph min/max and totals from SRAM. Semantic load counter1–115 is explicitly an abstraction. |
| Restart | Original Start after result advances tour to STUNT, not ZOOM ZOO | Standalone Race Again resets the same clean scenario, including fresh persistent defaults. Full fresh-process restart comparison; no tour-continuation claim. |

The source map is not a complete reached-producer closure. Result audit includes
non-ROM PCs and unresolved reads; zero unresolved stores must not be described
as zero residual accesses. Finish camera/lap/AI domains inherit R-0034 evidence.
Uncertain fields keep provisional names. Native arithmetic retains wrapped words,
signed steps, arithmetic shifts and original per-rider scheduling.

## Reproducible evidence

Private root: `.worktrees/m4-16-playable-zoom-zoo/artifacts/m4-16` relative to
main. `boundary-a/b`, `brake-a/b`, `start-brake-a/b`, `stop-brake-a/b`,
`trick-left-a/b`, `trick-long-a/b`, `ordinary-controls/{a,up,down,x}-a/b`,
`held-controls/{held-x,select,resume-x}-a/b` retain independent fresh originals.
Each reference has its timeline, memory hashes and video identity. Do not commit
these memory files, screenshots, packs or ROMs.

`initialization-audit` covers1207–1650, `result-audit`6725–7200,
`trick-long-audit`1718–1785, `ordinary-controls/a-audit`1681–1740,
`ordinary-controls/x-audit`1690–1760 and both `held-controls/*-audit`1700–1760.
Their `authentication.json` records exact capture commands and6394 authenticated
WRAM frames. Access reports retain residual reads and original hashes.

From the task checkout, repeat the immutable primary candidate gate:

```sh
python3 -m tools.unirally_lab.native.zoom_zoo_playable compare --reference artifacts/m4-16/boundary-a --repeat artifacts/m4-16/boundary-b --contract tests/manifests/native/zoom-zoo-playable-primary-v11.freeze.json --binary build/app-debug/src/core/zoom_zoo_runner --pack local/classic-crawler-two-tracks-v5.pack --out artifacts/m4-16/FRESH-primary-v11.json
```

Use fresh output names and a clean source/binary for the entire gate. Original
recovery uses `zoom_zoo_playable_reference --core ... --case ... --out FRESH`.
The ordinary `freeze` and `compare` commands require both finishes and200 stable
result updates. The re-press original fails that requirement (no player finish
by7600); its separate incomplete inventory is recovery evidence only.

At clean4f8aaad the632-byte primary passed479 restores and full restart;
debug/sanitize each405 checks, all28 historical M4-12–14 differential/restores,
DRAGSTER win/loss/restores/presentation, content and original replay passed.
At cleanedec610 independent full X680-byte gate passed1782 restores/restart.
These are historical candidate-specific passes, not final V10 or M4-16 acceptance.
Candidate9f7f3b4 and later review results are recorded in NEXT_SESSION.
Independent V10 review found a held duration7/rotations6 forged restore that
changed event17 weight after re-press. Correction uses positive duration <=
accumulated rotations before16-bit wrap, not equality: return/new roll retains
prior rotation counts. Elapsed-update bounds reject counters that could not yet
have accumulated. Negative return-phase relationships remain less constrained;
finite restore checks are not a complete reachability proof.

## Product gaps and next experiments

Still required: bounce and compound rewards, Start behavior, wrong-direction
counter boundary, complete producer closure, clean isolated bootstrap/extraction,
wrong-ROM/incomplete-pack/denied-access gates, latest historical M4-15 matrix,
untuned latest variations, representative frozen visual checks, and actual live
complete race/result/restart with independent reviewer input exercise.

Prototype rendering uses original track/background, prior camera, native HUD and
result values. Rider objects are now composed from the original pose frames,
reflection flip, OAM projection and look overlays ([R-0036](R-0036-zoom-zoo-rider-objects.md));
the final independent review (`9559d0b`) found rider facing and anchors
pixel-identical to the original, including untuned timelines. Audio is excluded
by task scope. Current app exit diagnostics still print generic movement race
status for ZOOM ZOO; its separate result line is more informative. A gamepad was
exercised live by the user (see the task record).

Earlier actual-window CUA taps produced2 down/up pairs but zero nonzero50Hz
updates. The inspected app stayed neutral, never finished/restarted, and its
source changed during that run. `live-inspection.json` says launch passed;
that is not live acceptance. No frozen-mask demonstration may replace live input.
The built-in desktop API cannot hold a key. A private PID-restricted CGEvent
helper is prepared but has never run; explicit permission was requested because
the desktop tool prohibits that fallback without user authorization. No answer
was received at this checkpoint. Do not infer authorization from elapsed time.

Pack v4 has49 entries and is experimental. Existing v2/v3 files at older local
paths are intentionally not overwritten and will be rejected by current code.
The landing-matrix extraction helper is tied to the audited macOS core identity;
clean Linux private extraction has not been established. Hosted synthetic CI
cannot substitute for it. M4 remains incomplete; no milestone tag is due.

## Latest user-requested handover

Latest behaviorca4602597960426629ad743f92f788e84c01a175 recovers the frozen
late-roll bounce: `$829636-9711` retains charge160, applies wrapped complement
bounce velocity, and shares ordinary completion/landing ordering. Late-roll
originals finish6468/6488, stable6823; all742 bytes match6225 native observations.
`late-roll-audit` authenticates6394 WRAM frames; residual3835 reads/zero stores
remain explicitly reported. Full latest restores and independent bounce review
are still due. Focused debug/sanitizer builds/tests and five complete diagnostic
cases pass; see NEXT_SESSION for exact evidence and known failed invocations.

Additional recovered source boundaries: NMI `$808642-865B` delays controller
publication through prior fade4; `$818721-875B` skips horizontal boost under
leading support; `$8191F4-920C` clears leading support on auxiliary boundary return.
Constant-direction/long-roll diagnostics match but do not finish and never count
as playable acceptance. The wrong-way counter179 intervention is an explicitly
artificial mechanics experiment, frozen inb83d7f3, separate from native-start gates.

Independent landing/persistent reviews are integrated; the durable
[persistent-input ledger](M4-16-persistent-input-ledger.md) distinguishes fresh
scenario defaults, current-race result inputs, presentation-only loads and
excluded tour continuity.

## Generic opponent reward consumption

The high-priority missing non-event-one opponent consumer is recovered at
`aeb62e0`. `$81C219-C2C9` is the player consumer `$81C0CE-C18A` with the
opponent addresses and one deliberate difference: it adds the **whole** reward
word to vertical boost (`$81C2A5-C2AD`) where the player halves it (`$81C169`).
Class table `$81C50A` has 72 entries and gives a real class only to events 1-12
and 16-21; reward words are `$81C493`. `$82DB87-DB94` copies one 26-byte
`$82D7A4` template into both learned banks, so the opponent event-one weight is
static content, not a constant.

The domain test `$81C238` is `CMP #$48` followed by `BMI`, which tests bit 7 of
the 8-bit difference rather than comparing signed values. The reward path is
taken for events **0-71 and again for 200-255**, and 72-199 take the voice path.
The fixed BRONSEN voices 200-215 therefore reach the reward path, leaving both
the class table and the 26-byte bank; the original exits on the zero weight at
`$7E21C9-$7E21D8`. The cartridge class counter it still bumps is outside the
recovered inventory here exactly as the player's `$77076B` is.

An earlier revision of this record called that test a signed comparison and the
implementation spelled it `int8(event) < 72`. That is wrong for events 128-199,
which a signed test diverts to the reward path but the hardware sends to the
voice path. Independent review caught it and settled it on the original rather
than on paper: injecting event 150 changes no cartridge class counter, while 205
increments `$770801`, so 150 really is on the voice side. Event 150 is also an
original-only control here — the native refuses to restore it at all, because
no producer emits it — so the corrected predicate has source and original-side
evidence but no native differential coverage for 128-199.

`$7E21C9-$7E21D8` was likewise asserted to be guarded when it was not. It is
zero on all **343,482** authenticated frames of all **53** reference captures,
and those sixteen bytes are now actual entries in
`zoom-zoo-race-guards.reference.json`, so a future capture that reaches a
nonzero value fails before native evaluation instead of being silently
mismodelled. The opponent hint latch `$12E5`, which the enqueue `$81C5D5-C5E3`
would otherwise clear, was already guarded at zero. Treat the out-of-bank exit
as a guarded bounded observation, not as a claim about arbitrary opponent state.

Evidence is `zoom_zoo_opponent_reward_probe`, an artificial original-only
intervention. It cold-starts the authenticated primary timeline, verifies every
pre-intervention frame, then performs exactly the enqueue the producer would
perform and records the original's answer. The natural primary reaches only
opponent events 1, 14, 15 and 39, so **nine** events were each captured twice,
identically, and frozen: 2, 8 and 21 for the full reward path, 17 for a counted
class with a zero weight, 13 for the leading class, 200 and 215 for the
out-of-table voice range, and 150 as the voice-path control the native refuses
to restore. Every capture is required to reach consumption — all do, at frame
1722 — and native continuation matches all 742 bytes across 9 observations for
each; a single flipped byte at the consumption frame is detected.

At the 742-byte projection the voice path and the out-of-table reward path are
both "consume, publish nothing", so the probes alone do not discriminate the two
domain predicates. The cartridge class counters are what separate them, and they
sit outside the projected inventory: event 150 moves none, event 205 increments
`$770801`, event 17 increments `$770819` (class 34) and event 18 increments
`$770805` (class 24). These probes are internal mechanics experiments and never
a seed-based playable acceptance fallback.

```sh
python3 -m tools.unirally_lab.native.zoom_zoo_opponent_reward_probe capture --reference artifacts/m4-16/boundary-a --core local/emulators/bsnes/bsnes/out/bsnes_libretro.dylib --event 2 --out artifacts/m4-16/opponent-reward-probe/FRESH
python3 -m tools.unirally_lab.native.zoom_zoo_opponent_reward_probe native --probe artifacts/m4-16/opponent-reward-probe/FRESH --reference artifacts/m4-16/boundary-a --binary build/app-debug/src/core/zoom_zoo_runner --pack local/classic-crawler-two-tracks-v5.pack
```

The legacy DRAGSTER/M4-12-15 caller keeps its accepted event-one domain and its
rejections unchanged; those formats serialize no learned-weight bank to model.

Standalone pause Down/Start is explicitly labelled RESTART RACE and uses shared
fresh initialization, not original Retire/tour emulation. Both input/art reset
paths are implemented; actual live evidence remains due. The earlier producer
table/gap lists above describe historical recovery boundaries where superseded
by this checkpoint. M4-16 remains unaccepted.

## Race palette cycle — `$82:D382-$82:D496`

Verified 17 September 2026 against the primary initialization capture
(`artifacts/m4-16/initialization-audit`, frames 1207-1650) and WRAM series.

- While `$0B92` is nonzero, the routine sets CGRAM address `$60` and writes
  colours 96-111 from sixteen 16-word ROM tables at `$80:82AB + 32k`, then
  colour 0 from `$80:84AB`, all indexed by `$0B84`, and advances
  `$0B84 = ($0B84 + 1) & 15` (`$82:D48D-D493`). The capture records 269 calls
  over frames 1382-1650, one per frame.
- `$0B84` ends frame n at `(n - 1381) & 15` from 1382 through the race, and
  keeps advancing while paused (pause-a 6000-6009), so it counts video frames,
  like native `movement.frame`. It stops at result loading (1 from 6725) and
  `$0B92` becomes 14 on the result screen.
- The cycle animates the checkered start/finish line: colour 97, for example,
  steps `$7FFF, $56B5, $2D6B, $0000` two frames each.

Native: frame n draws table index `(n - 1382) & 15`, applied to the CGRAM words
before brightness. Pack `classic.pal.crawler.two-tracks.v7` adds entry
`presentation.zoom.race-palette-cycle.v1` (file offset `0x02AB`, 544 bytes,
SHA-256 `a82bb272...14f7a57f`, rules SHA-256 `5920a130...69308b236`, 55
entries); two fresh extractions are byte-identical (`b75539a0...`). Across all
489 recaptured primary race frames available (1382-6720), the cycle changes
55,056 pixels in 91 frames relative to the previous renderer, and every one
equals the original; the previous renderer matched none of them.

The ZOOM ZOO renderer previously used DRAGSTER's pose-keyed late-finish
palette (`build_race_cgram`), which is true on 284 of 6,225 primary states and
only flipped the checker between two of the sixteen phases. DRAGSTER keeps its
accepted rule; whether DRAGSTER's race runs the same routine is not
investigated here.
