# D-0003 — Human-readable native code from the first implementation

- Status: accepted project policy (not a gameplay finding)
- Date and owner: 12 September 2026, Codex coordinator, following the user's maintainability/community question
- Related milestone/tasks: M2-01 and later native implementations
- Evidence records: existing architecture in PROJECT_PLAN.md; no new gameplay claim

## Problem

M2-01 introduces the first recovered game source. A mechanically translated routine can preserve behavior yet become difficult for humans to understand or extend. A later wholesale readability rewrite would mix structural changes with newly discovered mechanics.

## Options and experiment

Defer all readability work, build an extensible framework now, or require readable small implementations now and grow abstractions from measured needs. Choose the third: it supports review immediately without designing a mod API around unknown mechanics. This is an implementation policy decision, not an experimentally verified game property.

## Decision and consequences

Use descriptive names for established concepts; label unknowns by neutral names and provenance. Document units, fixed-point scales, widths, wrapping, input phase and update order. Prefer small functions with explicit state, input and content dependencies, with research links and relevant source addresses for unusual behavior. Keep a compact guide beside the first native implementation explaining its flow, supported domain, evidence and commands.

Keep exact Classic behavior as the constraint on every readability change. Isolate processor-shaped helpers when needed for exact arithmetic; a research translation is permitted but must not silently become the public architecture. Avoid per-instruction comments that merely repeat C++ operations. Review both semantic correctness and whether a human can trace the implementation to its evidence.

Use frozen differential cases to support incremental refactoring. Defer mod APIs, plugin frameworks and speculative abstractions until real requirements exist; preserve the current simulation/content/frontend and Classic/Extended boundaries. No change to publication, licensing or service authority.

## Revisit trigger

Repeated native routines reveal common structure, a measured dependency makes the current boundary misleading, or M3/M5 supplies a concrete extension use case. Revisit those boundaries with tests, without removing the readability requirement.

## Update 25 September 2026 - the rule was not enforced; make it measurable

The user asked whether the reward-queue update in `src/core/movement.cpp` (at `47708b4`) is
readable, and whether to refactor now or after full ROM coverage. It is not: it reads as an
annotated translation of `$81:C238`, with bare literals, verification guards between gameplay
branches and comments that explain the ROM match rather than the game. The reviewer checklist
named readability, but no acceptance criterion measured it, so fifteen `src/core` functions
grew past 80 lines and the translation became the public code.

Decision: a bounded readability pass on the recovered code now
([NATIVE-READABILITY](../../tasks/NATIVE-READABILITY.md)), under the frozen gates and with no
behaviour or format change, then measurable rules (function size, named constants, intent
before evidence, an address-to-native-symbol index) checked on every later task. Waiting for
full coverage was rejected: the recovered code is still a minority of the game, each task
copies the engine it extends, and a later wholesale rewrite is the outcome this decision was
written to prevent. A game-systems architecture stays deferred until the recovered code shows
the game's structure beyond the race engine; revisit it when recovery reaches the frontend and
menus.

The index keeps reverse engineering of the unassessed code as easy as before: once the C++ no
longer follows the ROM routine by routine, the index is how an address found in new
disassembly leads to the native code that already implements it.
