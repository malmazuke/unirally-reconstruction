# ZOOM-ZOO-OPPOSING-INPUT - independent review

- Candidate: `b477a0d` (code and cases `7b4ab4a`, documentation on top), base `main` at `8acee91`.
- Reviewing model: Claude Opus 5, fresh session, no inherited conversation with the primary.
- Checkout: `.worktrees/zoom-zoo-opposing-review`, branch `review/zoom-zoo-opposing-input`, at the exact
  candidate. Every command below ran from that directory against a build made there. Nothing on
  `task/zoom-zoo-opposing-input`, in the primary's worktree or in the main checkout was modified.
- Verdict: **approve**. No blocking finding. The mechanism, the three frozen cases and every accepted
  contract reproduce from my own build, and a withheld case of my own - built from an accepted race the
  primary did not use - reproduces its contract byte for byte. Five should-fix and five advisory items
  below; the first should-fix must be corrected before or in the integration commit.

## What I ran

```sh
PATH="$PWD/local/toolchain/ninja-1.13.2-darwin-arm64:$PATH" python3 tools/project.py build --preset app-debug
bash artifacts/review/review-gates.sh      # recaptures + 9 differential gates
bash artifacts/review/review-gates-b.sh    # 3 more DRAGSTER contracts, 3 probes, suites, v1, hidden, fuzz
```

Reviewer artifacts live in this checkout's ignored `artifacts/review/`: the two gate scripts and
`gates-b.log`, `gates/` (12 differential reports and their logs plus `validation-ledger.jsonl`),
`originals/` (11 captures of my own), `review-pause-opposing-saturated.case.json`,
`review-updown-3240.case.json`, `review-released-3240.case.json`, `pause-opposing.freeze.json`, and
the three probe scripts copied from the primary's worktree and re-run here (`ROOT` resolves to this
checkout). No reviewer input points into another worktree except the read-only copies of those probe
scripts and the primary's `native-dpad-probe.json`, used only for finding S2.

## 1. Reproductions, item by item

Every differential report below pins `source_commit b477a0d3c3d45a3a569abb23e5e38ca2c44b075d` and
`source_diff_sha256 e3b0c442...` (the SHA-256 of an empty diff), so the tree was clean throughout.

### 1.1 The three frozen cases, against originals I captured myself

I did not use the primary's 5.1 GB of captures for the gate. I recaptured all six from the ROM through
the audited core with the tracked case files, then compared against the tracked contracts. The compare
enforces `original_sha256`, `rows_sha256` and `timeline_sha256` against the frozen contract, so this
also independently reproduces the freeze itself.

| Case | my capture a/b identical | contract inventory | native compare | restores | primary recorded |
| --- | --- | --- | --- | --- | --- |
| opposing-ride (horizon 8100) | yes (`6e7ee4ec…` / `ad7bbb89…`) | matched | `passed` | 801 | 801 |
| opposing-axes (7600) | yes (`cd086d35…` / `1e17d445…`) | matched | `passed` | 781 | 781 |
| opposing-edges (7600) | yes (`59d9ac1d…` / `2a3f4764…`) | matched | `passed` | 757 | 757 |

Reproduced. Rows `205d1705…`, `3d101ea6…`, `b4a34af7…` as the record states.

### 1.2 The two equality claims, checked offline from tracked files alone

`zoom-zoo-playable-opposing-ride-v11.freeze.json` has `rows_sha256 205d1705b679…`, identical to the
accepted `zoom-zoo-playable-idle-late-start-v11.freeze.json`, with different `timeline_sha256`
(`3f6a61c6…` vs `7be9a5b5…`). `opposing-edges` has `b4a34af722b2…`, identical to the accepted
`zoom-zoo-playable-primary-v11.freeze.json`, with `7cb03075…` vs `f3ab533d…`. Reproduced.

### 1.3 The probes

| Probe | result | matches the record |
| --- | --- | --- |
| `dpad_probe.py` | neutral, Left+Right, Up+Down all `equal_to_neutral=True` over 1,025 frames; Left and the primary's own steering differ first at 1650 | yes |
| `port_words_probe.py` | released / Left+Right / Up+Down each take exactly `$0311=0`, `$0313=0`, `$0315=1`, `$0319=1`; Left `0/2/1/0`; Down `0/4/2/1` | yes, table for table |
| `native_dpad_probe.py` at `b477a0d` | both tracks `equal_to_neutral=True` for both pairs, `False` at 1650 for Left alone | see S2 |

### 1.4 Accepted contracts, from my own build

| Gate | status | restores | previously recorded |
| --- | --- | --- | --- |
| M4-16 primary (`boundary-a/b`) | passed | 757 | 757 |
| M4-16 idle late start (`late-start-a/b`) | passed | 801 | 801 |
| DRAGSTER primary | passed | 379 | 379 |
| DRAGSTER random-1 | passed | 567 | 567 |
| DRAGSTER reversal | passed | 327 | 327 |
| DRAGSTER random-3 (mine, see A3) | passed | 493 | 493 |
| DRAGSTER regression-landing-held-roll (mine) | passed | 181 | 181 |
| DRAGSTER regression-countdown-actions-tie (mine) | passed | 179 | 179 |

### 1.5 Suites

`build` and `ctest` on `lab-debug`, `lab-release`, `lab-sanitize`, `app-sanitize`, `app-debug`:
`status=passed` and **23/23** on each. Synthetic suite on `lab-debug` `status=passed` (48.7 s). Both v1
presentation contracts `status=passed` (winner's seven visual checks 0.135-1.678% against 2/3/15%
limits; loser 1.871% against 15%). Six hidden 4,000-update app runs (`dragster` and `zoom-zoo` at masks
128, 192 and 48) `rc=0` with 0 rider-pose fallback frames. `dragster_fuzz_runner` 40 seeds: 382,535
updates, 79 completed races, 1,242 pause restarts, 7,696 renders, **0 aborts**.

Not reproduced, because it does not exist yet: hosted CI on the final tip. Acceptance stays conditional
on it, as the task record says.

## 2. The mechanism, checked from the core and the ROM rather than the probes

I read `local/emulators/bsnes/bsnes/sfc/controller/gamepad/gamepad.cpp` myself. `Gamepad::data()`
returns `up & !down`, `down & !up`, `left & !right`, `right & !left` at shift positions 4-7, under the
comment that the D-pad physically prevents both. That is the whole mechanism, and it is in the
emulator, not in the ROM.

This matters for how the finding should be stated. The probes cannot be independent confirmation that
the console drops opposing directions: the core is the only path to the ROM here, and because its
gamepad model never publishes both bits, `dpad_probe`'s byte-identical WRAM is close to a
tautology. What the probes do establish, and it is worth having, is that nothing *else* in the emulated
machine leaks the raw request - no second input path, no auto-joypad quirk - over 1,025 frames and at
the four words the game derives. What the *console* does rests on the construction of a standard SNES
rocker pad, which this task asserts (through the core's comment) rather than measures.

So the claim I can sign is: **through the audited core, and on a standard rocker pad, a request for
both directions of one axis is indistinguishable from a released pad, in whole WRAM and at
`$0311`/`$0313`/`$0315`/`$0319`.** R-0041's limits section is close to this but overreaches in two
places, both in S4 and A1 below: it says the both-bits state is "unreachable on the console", when a
third-party or programmable pad closes both contacts and the port would then publish both; and it says
what the game would do with both bits "remains unrecovered", when `src/core/input_timer.cpp` already
carries a recovered answer.

## 3. My withheld case

The primary's three cases all add opposing pairs to frames that are already released. I wanted a case
that (a) the primary did not use, (b) reaches the pause menu's vertical navigation - the one branch
R-0041's limits declare is covered by ROM-free tests only - and (c) makes a falsifiable prediction
about the *original* rather than only about native.

**`review-pause-opposing-saturated`.** I took the accepted `zoom-zoo-playable-pause-shifted` case (a
complete won race with a countdown pause at 1450-1461, navigated Down at 1455 and Up at 1457) and added
the opposing pair for every axis that is neutral, on every frame from 1377 to 7600: **6,222 vertical
pairs and 1,141 horizontal pairs**, including the pause frames. Directions the accepted case actually
publishes were left alone, so the port publication should be unchanged.

- Offline, before touching the core, I proved the prediction: applying the rocker to my delivered
  timeline gives the accepted one exactly, both ports, all 7,601 frames; my timeline hash is
  `b0a1ff0e5bd7…` against the accepted `374274a0f90a…`, which is also the accepted contract's
  `timeline_sha256`, so my reconstruction of that case is the one behind the contract.
- Captured twice from the ROM: identical (`wram d5ec2451…`, `sram 442e1240…`). Frozen.
- **`rows_sha256 5d83cbdd33d89b0b64e629af9062d9c8f584ba7ae55d6c09489fbdd90b5ac3f7`, exactly the accepted
  `zoom-zoo-playable-pause-shifted-v11.freeze.json`**, with identical events (finish 6498/6501, loading
  6739, first visible 6847, stable 6853, `player_won`) and a different timeline. The original cannot
  tell my saturated pad from the accepted one, over a complete race, in the pause menu included.
- Native compare against my freeze: `status=passed`, **777 restores**, empty working diff. The compare
  feeds the native runner the *raw* timeline, opposing pairs and all, so this exercises the engine's new
  rocker directly rather than a pre-filtered stream.
- The capture also passed `original()`'s `zoom-zoo-race-guards` constant-domain check on every frame, so
  saturating the pad reached no new gameplay state that would need recovering first.

**Attempt 4, verified with a counterexample search of my own.** The record says an opposing window
spliced into the marker-guided riding script desynchronizes it and never finishes. I tried the stronger
form the primary did not: an `idle` window, which *does* shift the resume, held Up+Down for 1,200
updates from 3240 (`review-updown-3240`), horizon 8400, plus the same window released
(`review-released-3240`). Result: the two produce **identical** 7,025-row streams
(`c90bf4b41938dc3a…`) from different delivered timelines (`20b6ad84…` vs `8628b02a…`) - another fresh
equivalence measurement that depends on no accepted contract - and **neither finishes**
(`finish_frames [None, 6488]`). So attempt 4 holds, and holds even with the shift-resume: a mid-ride
window desynchronizes the script whatever is held in it, which is why the usable riding window is the
one at 1650. No counterexample found.

## 4. The placement, checked adversarially

- **Is the recovered-domain guard still equivalent?** For ZOOM ZOO, trivially: no ZOOM ZOO call site
  ever applied the rocker, so the guard sees exactly what it saw before. For DRAGSTER the guard now
  reads the pre-rocker request where it used to read the post-rocker buttons, which is a real change in
  principle. It is inert in practice: every clause except `(!state.complete_race && requested.left)` is
  gated on `!state.native_initialization`, and a DRAGSTER state always has
  `native_initialization` - `classic_race_start` sets it, `serialize_zoom_zoo` throws
  `"DRAGSTER race state requires native initialization"` otherwise, and `deserialize_zoom_zoo` routes
  only 742-byte `URDG0001` states to the Dragster track, which forces `bytes[7]=='B'` and hence
  `native_initialization`, which in turn forces `complete_race`. So all clauses are unreachable for
  DRAGSTER and the guard is behaviour-preserving. Confirmed empirically by six DRAGSTER gates, three of
  whose own timelines hold opposing pairs (below).
- **Does the M4-12 to M4-15 continuation domain move?** No for Up+Down (the guard rejects them there,
  as before) and no for Left+Right in the `!complete_race` seeds (rejected, as before). The only
  arithmetic change in that domain would be a `complete_race && !native_initialization` seed delivering
  Left+Right, and no tracked case or replay manifest does; the five-preset `ctest` and the synthetic
  suite, which carry those contracts, are unchanged at 23/23.
- **The legacy `update_movement` path.** Untouched, as claimed. It still feeds raw buttons to
  `sample_controller`, so on that path Up+Down resolves to Up by the original's branch precedence and
  Left+Right throws `"leftward movement is outside the recovered primary domain"`. Only
  `movement_runner` and `tests/app/frontend_contract_tests.cpp` reach it, and neither delivers an
  opposing pair, so nothing moves - but the two entry points now answer the same request
  differently, and no record says so (S4).
- **Serialization and restore.** `whole.player_input` is now the port publication rather than the raw
  request, which is the point of the change; the restore path re-enters the engine with the same raw
  timeline, so it re-applies the rocker and stays consistent. The 12 differential gates I ran replay
  6,800 restores in total, 777 of them across my saturated pause case.
- **Any caller that is not a physical pad?** No. `zoom_zoo_runner`, `sdl_main`,
  `dragster_fuzz_runner` and the two compare tools all deliver what a device asked for. In each of the
  three call sites the local `buttons` is used only for `.start` and for the engine call, so removing
  the filter changed nothing else. `with_physical_dpad` is idempotent, so a caller that still applied it
  would be harmless.
- **Would rejecting have been more faithful than dropping?** No. The original does not reject: it runs a
  complete race whose 742-byte rows are byte-identical to a released pad's, which my withheld case
  reproduces. Throwing would be a native invention that contradicts the measurement, and it would abort
  live play on an ordinary keyboard chord. Dropping is the only answer the evidence supports, and it
  matches what DRAGSTER's accepted call sites already did.
- **Is the rule in the right place?** Yes, on the evidence: it is a property of the controller port,
  both tracks share one engine, and the previous arrangement had already been forgotten once - that is
  exactly the defect this task fixes. The cost is that `update_zoom_zoo`'s input contract silently
  changed meaning for every caller, and the header was not updated to say so (S3).

## 5. Findings

### Blocking

None.

### Should-fix

- **S1. `tasks/NEXT_SESSION.md` preclaims this review and an integration that has not happened.** At the
  reviewed commit it reads "Every started task is reviewed, integrated on `main` and accepted conditional
  only on its recorded final-tip CI; ZOOM-ZOO-OPPOSING-INPUT's closeout is
  `artifacts/zoom-zoo-opposing-integration/closeout.json` in the main checkout", and "The last open
  follow-up below (opposing-direction input) is closed". Reproduce: `git show b477a0d:tasks/NEXT_SESSION.md
  | head -12`; then `ls "artifacts/zoom-zoo-opposing-integration/closeout.json"` in the main checkout -
  it does not exist. The same commit's `tasks/README.md` says "review: candidate `7b4ab4a`" and the task
  record says "pending independent review", so the candidate contradicts itself. For the previous task
  this wording was added only in the integration commit `4b76ee1`, after the approving review `c610066`;
  doing it in the candidate writes a self-approval into the coordinator record, which D-0006 forbids.
  Restore the "in review" wording, and add the integrated/closeout wording in the integration commit.
- **S2. `native-dpad-probe.json` is a post-fix run stamped with the pre-fix commit, so attempt 2 is not
  reproducible from the retained evidence.** The task record's attempt 2 says native ZOOM ZOO "diverges
  from neutral at 1650 for both Left+Right and Up+Down", but the artifact that is supposed to show it
  records `equal_to_neutral: true` for `zoom-zoo:left+right` and `zoom-zoo:up+down` with
  `"commit": "c2de73e489994870784f86cd2652de206e01fbac"`. That combination is impossible:
  `git show c2de73e:src/core/movement.cpp | grep -c with_physical_dpad` is 0 and
  `git show c2de73e:src/core/zoom_zoo_runner.cpp` applies the filter only when
  `state.track==ClassicRaceTrack::Dragster`, so at that commit ZOOM ZOO must differ from neutral at
  1650. I re-ran the same probe at `b477a0d` and got a `results` object identical to the retained one,
  differing only in the `commit` field - i.e. the artifact is a post-fix run recorded while `HEAD` was
  still the pre-fix commit. Re-run the probe on `c2de73e` and keep both reports, or relabel the existing
  one and say plainly that the pre-fix divergence is evidenced by the diff rather than by a probe. The
  overlapping attempt timestamps (3 at 03:15-03:20 spanning 4 at 03:15-03:17 and 5 at 03:17-03:20) make
  this chronology hard to audit and are worth tidying at the same time.
- **S3. The engine header still states the old input contract.** `src/core/zoom_zoo_movement.hpp:153`
  declares `update_zoom_zoo(ZoomZooState&, const ControllerButtons& buttons, const ZoomZooContent&)`
  under only "Historical continuation and native scenario share this update path". The header is an
  owned path of this task and is where a new caller looks, yet the one place the contract changed - the
  engine now expects what the *device* reports and applies the controller port's rocker itself - is
  documented in the three call sites and in the definition, not in the declaration. Rename the parameter
  to match the definition's `requested_buttons` and add the one-line contract.
- **S4. R-0041 says the both-bits behaviour is unrecovered; the tracked code says it is recovered.**
  R-0041's limits: "what the game's code would do with both bits set. That state is unreachable on the
  console and through the audited core, so it remains unrecovered, and native must not invent it."
  But `src/core/input_timer.cpp:25-27` has carried, since `510ba04`, "The original branches establish
  precedence for contradictory directions" with `vertical = up ? 0 : (down ? 2 : 1)` and
  `horizontal = left ? 0 : (right ? 2 : 1)` recovered from `$82:AAA4-AB59`. After this change that
  precedence is unreachable from the race engine but still live on `update_movement`. Say which it is,
  and record in R-0041's limits that the two entry points now answer the same request differently
  (engine: neither direction; `update_movement`: Up wins, and Left+Right throws on its horizontal
  domain guard).
- **S5. Tracked records point evidence at a directory that does not exist yet.** R-0041's Measurements
  section says "Scripts and reports are under `local/evidence/zoom-zoo-opposing-input/zoom-zoo-opposing-input/`",
  and `tasks/NEXT_SESSION.md` lists `…/originals` among the gate inputs. Reproduce:
  `ls "local/evidence/zoom-zoo-opposing-input"` in the main checkout - "No such file or directory". They
  are still in the task worktree until closeout, which the task record states but the two tracked
  records assert as present fact. Either qualify them ("after closeout") or move the evidence first.

### Advisory

- **A1. "The console cannot reach it" is wider than the evidence.** STATE.md and R-0041 both say so; what
  was established is the audited core's gamepad model plus the construction of a standard rocker pad.
  A third-party, turbo or programmable pad closes both contacts, and the port would then publish both.
  "A standard rocker pad, through the audited core" is the precise domain and costs one clause.
- **A2. The `idle` variation's new `buttons` are not constrained to be port-neutral.** The docstring and
  `docs/BUILD_AND_VALIDATION.md:458` both justify the feature by "the port publishes nothing for an
  opposing pair, so it is the same window", and the shift-resume (`primary[f-count]`) only makes sense
  for a window that is idle at the port - but `{"idle":{"from":1650,"frames":10,"buttons":["left"]}}`
  validates and silently produces a held-Left window with a resume shift that means nothing. One
  assertion that `with_physical_dpad(held)` is empty would keep the name honest.
- **A3. The DRAGSTER regression set that actually holds opposing pairs was not re-gated.** Counting
  opposing events in the tracked DRAGSTER cases: `regression-landing-held-roll` 117 frames,
  `random-1` 42, `random-3` 19, `regression-countdown-actions-tie` 10; `primary`, `random-2` and
  `reversal` none. The primary gated primary/random-1/reversal, so only 42 frames of DRAGSTER opposing
  input were re-checked, although these are exactly the contracts the placement change could break. I
  ran the other three: `passed` at 493, 181 and 179 restores, each equal to the count recorded in
  DRAGSTER-CLOCK-LIMIT. Worth adding to the task's evidence table rather than leaving to a reviewer.
- **A4. Naming inside `update_zoom_zoo`.** There are now three values a reader must keep apart: the
  parameter `requested_buttons` (what the device asked), the local `requested` (the same, but already
  zeroed while `fade_level<5` by the NMI publication rule), and `buttons` (what the port publishes).
  `requested` is the misleading one, since it is not purely the request. A word in the existing comment
  would do.
- **A5. The three new cases carry no `id`-to-contract note in the manifests.**
  `zoom-zoo-playable-opposing-ride.case.json` and `-axes` are single-line JSON while `-edges` is
  pretty-printed, and none of the three records which accepted contract its rows are expected to equal.
  That equality is the whole point of two of them and is currently only in R-0041's table.

## 6. What remains unverified

- Hosted CI on the final tip, which does not exist yet. Acceptance stays conditional on it.
- Whether a real SNES console with a non-rocker pad publishes both bits, and what the ROM then does.
  Not reachable with the inputs this project has; see section 2 and S4/A1 for how to state it.
- The pause menu's vertical navigation *with a single direction* under an opposing hold on the other
  axis - my withheld case saturates the neutral axis but keeps the accepted case's own Down/Up presses,
  so the interaction of a published direction with a dropped pair on the other axis at the same update
  is exercised only by the ROM-free tests.
- The primary's own six captures. I did not read them; I recaptured all six cases from the ROM instead,
  which is a stronger check of the contracts but says nothing about the files under
  `.worktrees/zoom-zoo-opposing-input/artifacts/`.
- Everything `docs/STATE.md` lists as a declared omission is out of scope and untouched here.

---

# Re-review at `024bf56`

- Correction commit: `024bf56` "Apply the independent review's findings", a single commit on
  `task/zoom-zoo-opposing-input` on top of the reviewed candidate `b477a0d`.
- Same reviewer session and checkout; this branch stays at `b477a0d` plus its review commits, so the
  corrected tree was exercised from a detached `024bf56` in this worktree and the branch was restored
  afterwards. Nothing on the implementation branch or in the primary's worktree was modified.
- Verdict: **confirm**. All ten findings are addressed. Nothing regressed on my own build. Four
  residual items below, one of them a small should-fix; none blocking, and none needs another round
  before integration as long as they are dispositioned in the integration commit.

The code delta is exactly what was described: a local rename, two comments, a header comment and
parameter rename, the held-set guard in the capture tool, and one case file compacted.
`git diff b477a0d..024bf56 -- src/ tools/ tests/` contains nothing else, and the only tests/manifests
change is `opposing-edges.case.json` reflowed onto one line.

## Verification on my own build at `024bf56`

Five presets built and `ctest` **23/23 on each**. Synthetic suite `status=passed` (44.7 s). Both v1
presentation contracts `status=passed`. Four hidden runs at masks 192 and 48 on both tracks `rc=0`
with 0 rider-pose fallback frames. Fuzz 40 seeds: 382,535 updates, 79 races, **0 aborts**.

Four differential gates, each pinning `source_commit 024bf56c…` and the empty-diff
`source_diff_sha256 e3b0c442…`:

| Gate | status | restores | unchanged from |
| --- | --- | --- | --- |
| my withheld `review-pause-opposing-saturated` | passed | 777 | my run at `b477a0d`, same `rows_sha256 5d83cbdd…` |
| opposing-ride (my own originals) | passed | 801 | the contract and my `b477a0d` run |
| M4-16 primary | passed | 757 | the recorded count |
| DRAGSTER regression-landing-held-roll (117 opposing frames) | passed | 181 | the count recorded in DRAGSTER-CLOCK-LIMIT |

I also read the primary's eleven gate reports at `024bf56`: all `passed` with empty diffs and the same
counts, and the previous run is archived under `gates-b477a0d/` rather than overwritten.

## Finding by finding

- **S1 - closed.** `tasks/NEXT_SESSION.md` at `024bf56` states the state at that commit: reviewed,
  corrections applied, integration and closeout the only work left, and the closeout path is no longer
  claimed. The task record's Status line and its new "Review and integration" section say the same.
- **S2 - closed, and reproduced independently.** The probe now takes the runner as `argv[1]`, the
  report name as `argv[2]` and a build description as `argv[3]`, and records the binary path, its
  SHA-256 and that description. I did not take the primary's "before" artifact on trust: I checked out
  `c2de73e` in this worktree, built `app-debug` there (runner SHA-256 `fa849a4a…`, distinct from the
  candidate's), and ran the corrected probe against it myself. Result, matching
  `native-dpad-probe-before.json` row for row:

  | Build I made | ZOOM ZOO Left+Right | ZOOM ZOO Up+Down | ZOOM ZOO Left | DRAGSTER pairs |
  | --- | --- | --- | --- | --- |
  | `c2de73e` | differs at 1650 | differs at 1650 | differs at 1650 | identical to released |
  | `024bf56` | identical | identical | differs at 1650 | identical |

  Attempt 2's divergence is now reproducible from the retained evidence, which is what the finding
  asked for. The misleading single-report artifact is gone rather than left alongside the new pair.
- **S3 - closed.** `src/core/zoom_zoo_movement.hpp` declares `requested_buttons` and says the update
  applies the rocker itself and that opposing directions reach the race as neither.
- **S4 - closed.** R-0041's new "The legacy path keeps its own answer, deliberately" states the
  precedence, that Up+Down resolves to Up and Left+Right to Left which `update_movement` then rejects,
  that only `movement_runner` and `tests/app/frontend_contract_tests.cpp` reach it, and why the
  precedence is not evidence about the port. The `src/core/movement.cpp:797` citation is exact - that
  is the `throw` line at `024bf56`. `docs/STATE.md` carries the short form.
- **S5 - closed.** R-0041 and `tasks/NEXT_SESSION.md` both now say the captures are in the task
  worktree until closeout.
- **A1 - closed in substance, one clause left (R1).** R-0041's limits now carry the caveat in the
  terms I asked for: the core's gamepad is the only path to the ROM and already drops the pairs, so
  measurement 1 is close to a tautology; what the measurements establish is that nothing else in the
  emulated machine leaks the raw request; and the hardware claim is the audited core's own, taken as
  given rather than measured on a console. That is the right statement.
- **A2 - closed, and I tested the guard myself.** The variation now removes each complete axis pair
  from the held set and refuses anything that survives. Checked at `024bf56` without the ROM:
  `[]`, `["left","right"]`, `["up","down"]` and `["up","down","left","right"]` are accepted;
  `["left"]`, `["b"]`, `["start"]`, `["up","down","b"]`, `["up","down","left"]` and
  `["right","up","down"]` are all refused. Every tracked case that parses through `case_timeline`
  still produces its contract's `timeline_sha256` exactly - `opposing-ride` `3f6a61c6…`, `opposing-axes`
  `7349d3f3…`, `opposing-edges` `7cb03075…`, the accepted `idle-late-start` `7be9a5b5…`,
  `pause-shifted` `374274a0…` and `idle-stop-timeout` `cb43ec60…` - so neither the guard nor the
  reflow of `opposing-edges.case.json` moved a parsed variation.
- **A3 - closed in evidence, thin in the record (R2).** The three contracts are gated: the primary's
  reports show `random-3` 493, `regression-landing-held-roll` 181 and
  `regression-countdown-actions-tie` 179 at `024bf56`, and I re-ran the densest one myself.
- **A4 - addressed, but the new name is inaccurate the other way (R3).**
- **A5 - closed, and the stated reason checks out.** The three case files are one compact line each
  with unchanged parsed variations (above), and the expected row equalities are in
  `docs/BUILD_AND_VALIDATION.md` and R-0041. I verified the justification rather than accepting it:
  `reference.json` does store the parsed `variation`, and `original_sha256` is the digest of that whole
  document, so adding an annotation key to a case would change the freeze - injecting one into my
  `ride-a` capture moves `original_sha256` from `fa6630e8b5f1f59f1c886721d4288c35…` (which is the
  tracked contract's value, reproduced by my own capture) to `b259d2abef1df966499f0a8c25e75270…`.

## Residual items

- **R1 (should-fix, one clause).** The A1 disposition says "unreachable on the console" was "reworded
  everywhere", but `docs/STATE.md:187` still reads "a keyboard or an analog stick could drive its race
  with an input the console cannot produce" - the same unmeasured hardware premise, in the summary a
  fresh agent reads first. Reproduce: `git grep -n "the console cannot" 024bf56 -- docs`. Reword to a
  standard pad, or point at R-0041's limits where the premise is now stated as taken-as-given.
- **R2 (advisory).** The task's acceptance table still lists the required gate set as "M4-16 primary
  and idle gates, DRAGSTER primary/random-1/reversal gates, v1 presentation contracts". The three
  added DRAGSTER contracts appear only in the A3 disposition row, so the row that defines what this
  task must gate does not name them. One edit puts the record where the evidence already is.
- **R3 (advisory, and my own A4 is partly to blame).** Renaming the local to `published` fixes the
  half I complained about and breaks the other half: the value named `published` is the request gated
  by the NMI publication window but *before* the rocker, while the value the controller port actually
  publishes is `buttons`, after it. The new comment says the guard "deliberately reads the publication
  before the rocker", which reads as a contradiction for the same reason. Physically the rocker comes
  first and the publication window second. Something like `gated_request`, with "the guard reads the
  request as the publication window gates it, before the rocker" would be accurate; it changes no
  behaviour either way, because `with_physical_dpad({}) == {}` makes the two orders agree on `buttons`.
- **R4 (advisory, cosmetic).** In `docs/BUILD_AND_VALIDATION.md` the inserted paragraph about the
  expected row equalities runs into the pre-existing sentence on one long line ("…matching its own
  contract. `ride` holds Left+Right for updates"). A line break restores the paragraph.
- **Housekeeping.** `.worktrees/opposing-before`, the throwaway detached checkout at `c2de73e` used for
  the S2 measurement, is still registered in `git worktree list`. AGENTS.md's retention rule expects a
  closing session to remove the worktrees it created; worth adding to the closeout along with the
  captures.

## Still unverified after the corrections

Unchanged from the first report: hosted CI on the final tip does not exist yet, so acceptance stays
conditional on it; what a non-rocker pad would make the ROM do is not reachable with this project's
inputs, and R-0041 now says so in those terms; and I verified the primary's `024bf56` gate reports by
reading them, having reproduced four of the eleven on my own build rather than all of them.
