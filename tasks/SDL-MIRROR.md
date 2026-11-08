# SDL-MIRROR - the app builds when GitHub's release downloads fail

## Assignment

- Status: **claimed** 8 November 2026 09:25 UTC on main `68b1c9b4`, by the coordinator, to unblock
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

- A future SDL version bump must update `mirror_urls` with the version; if the mirror is missing
  or differs, `URL_HASH` fails the build rather than accepting another archive.
- The lab presets do not fetch SDL; the toolchain bootstrap's own downloads (CMake, Ninja) are
  cached by CI and not changed here.

## Handoff

Not reviewed yet.
