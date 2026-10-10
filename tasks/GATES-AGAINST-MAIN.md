# GATES-AGAINST-MAIN - check changes against accepted native code

## Assignment

- Status: **queued** 11 October 2026 by [EVIDENCE-RETENTION](EVIDENCE-RETENTION.md), at the
  user's request. Unclaimed.
- Milestone: M4 (original game coverage); project infrastructure.
- Review tier: **1** (it changes differential gates and reference baselines, D-0008).
- Dependencies: [D-0011](../docs/decisions/D-0011-evidence-retention.md).

## Why

- **What the gates do today.** The gates compare each candidate with stored captures of the
  original: the eleven frozen race gates, the front-end and audio cue comparisons. The folders
  they read hold about 122 GB.
- **Why that may be more than needed.** Once a part is accepted, native was proven equal to the
  original on those inputs. A candidate equal to accepted `main` on the same inputs is therefore
  equal to the original on them, for the same compared fields.
- **What the project already does.** The race sweep and front-end sweep already compare against
  `main`'s binary.
- **The user's question (11 October 2026).** Why keep the original's captures once a part is
  migrated?

## Outcome

- **The design.** A decision record proposes which gates compare against `main`'s native output
  rerun from its commit, and which must stay against the original:
  - changes meant to alter behaviour;
  - parts not yet native;
  - known residual differences;
  - the first acceptance of a capability.
  It also says how a task regenerates the original's capture from its manifest when it needs one.
- **The proof.** The switched gates catch a deliberately introduced regression in each gated
  system (race state, pictures, cues), and the remaining original captures are listed with the
  reason each stays.
- **The payoff.** With the decision adopted, the original captures the switched gates no longer
  read can go under D-0011, keeping their recipes.

## Boundaries

- Acceptance of new behaviour still compares with the original.
- No gate is weakened silently. Each switched gate's compared fields and frame ranges are the
  same as before.
