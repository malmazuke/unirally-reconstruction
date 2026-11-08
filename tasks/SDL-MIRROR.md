# SDL-MIRROR - the app builds when GitHub's release downloads fail

## Assignment

- Status: **accepted** 8 November 2026 (tier 2, #98). Claimed 8 November 2026 09:20 UTC on main `68b1c9b4`, by the coordinator, to unblock
  [APP-PARITY](APP-PARITY.md)'s hosted CI (#97). Claude Code desktop app, Claude Opus 5.5
  (`claude-opus-5-5`): coordinator and implementer; a fresh subagent reviews.
- Branch `task/sdl-mirror` in `.worktrees/sdl-mirror`.
- Task provider: Anthropic.
- Review tier: **2** (D-0008: build configuration and CI; no native code, no gameplay change).

## Why

On 8 November 2026 from about 08:40 UTC GitHub's release downloads answered HTTP 504 on and off,
for SDL's pinned archive (`release-3.4.10/SDL3-3.4.10.tar.gz`) and for other projects' release
assets too (CMake's), while `githubstatus.com` reported all systems operational. Every app preset
fetches SDL from that one URL at configure time, and the Linux CI job configures three of them
(app-debug, app-release, app-sanitize), so a job fails if any one fetch gets a 504:
- APP-PARITY's run 42463821792 failed four times, at a different preset's SDL fetch each time
  (app-sanitize twice, app-debug, app-release);
- a `workflow_dispatch` run on main (42465128357) failed the same way at app-debug.

Reproduced in an Ubuntu 24.04 container with CMake 3.31.10: `download-sdl3-populate.cmake` reports
`status_code: 22`, `The requested URL returned error: 504`; `curl` from macOS saw 504 on every try
at 09:10 and 09:21 UTC and 200 at 09:13.

## Outcome

- `tools/locks/frontend.json` lists `mirror_urls` for SDL: libsdl.org's copy of the same release
  archive (`https://www.libsdl.org/release/SDL3-3.4.10.tar.gz`, 15,606,216 bytes, SHA-256
  `12b34280...2785`, the lock's hash; checked with `curl` and `shasum` on 8 November 2026).
- `src/app/CMakeLists.txt` passes the lock's URL and then each mirror to `FetchContent_Declare`'s
  `URL`, which tries them in order; `URL_HASH` still checks the archive whichever URL served it,
  so the pinned source cannot change.

## Evidence

- Fallback: in an Ubuntu 24.04 container with CMake 3.31.10, a `FetchContent_Declare` whose first
  URL returns 404 and whose second is the mirror populates SDL's source with the lock's hash.
- A fresh `app-debug` configure and build on macOS (`status=passed`) passes both URLs to the
  subbuild; `ctest` 41/41.
- Hosted CI on the pull request exercises the Linux presets.

## Limits

- A future SDL version bump must update `mirror_urls` with the version. A stale or missing mirror
  goes unnoticed while GitHub answers; when GitHub fails, the hash check refuses a stale file (a
  missing one fails at download) and the build fails as it did before, never accepting another
  archive.
- `mirror_urls` is optional: a missing key or an empty list leaves the GitHub URL alone (review 1).
- The lab presets do not fetch SDL; the toolchain bootstrap's own downloads (CMake, Ninja) are
  cached by CI and not changed here.

## Review (tier 2, one round)

A fresh subagent in its own checkout at `b00f793b` (09:23-09:28 UTC): approve with should-fixes,
in `local/evidence/sdl-mirror/review-1/review.md` and on the pull request. It reproduced the
fallback with the real lock on macOS and in Ubuntu 24.04 while GitHub answered 504 (the subbuild
tried GitHub, then the mirror, then checked the hash), and against a local server on CMake 3.31.10
and 3.27.1: 504 then a good file populates; 504 then a wrong file fails the hash; URLs are tried in
order. Should-fixes, both handled in the next commit: an empty or missing `mirror_urls` failed
configure (now the GitHub URL alone), and the Limits overstated when a stale mirror is caught and
the claim time postdated the commit. Its note: the gate runner counts any change under
`tools/locks/` or a `CMakeLists.txt` as changing every binary, but none can differ (the lab presets
do not read the lock and the archive's hash is pinned), so no local gates were run; hosted CI
builds every preset.

## Handoff

- Commits: `b00f793b` (the fallback), then review 1's fixes and this record.
- Review 1's fixes, checked: `cmake -P` on the CMakeLists' URL lines with the real lock gives the
  GitHub URL then the mirror; with `mirror_urls` removed, and with it empty, the GitHub URL alone.
  App-debug reconfigures and builds (`status=passed`), `ctest` 41/41.
- Hosted CI on `c9372e62` (run 42467380403): `changes`, `lab (ubuntu-24.04)` (10 min 8 s, every app
  preset fetched SDL while GitHub was answering 504 on and off) and `lab (macos-15)` passed. No
  local gates: no binary can differ (review 1's note above).
- Time: claim 09:20 UTC to merge about 09:50 UTC, about 30 minutes.
- Next: rerun APP-PARITY's (#97) hosted CI against main with this change.
