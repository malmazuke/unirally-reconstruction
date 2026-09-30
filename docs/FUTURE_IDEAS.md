# Future feature ideas

This file preserves ideas for after the full native reproduction of the
supported original ROM. Entries are deferred product directions, not scheduled
work, implementation specifications or claims about available features. They
do not change the [project plan](PROJECT_PLAN.md), the current roadmap or the
[next-session assignment](../tasks/NEXT_SESSION.md). Revisit an idea when the
user chooses to take it forward; documenting it does not authorize agents to
dispatch its implementation.

## Personal ghosts and asynchronous ghost racing

Recorded on 30 September 2026 from the user's discussion with the coordinator.
The user supports the direction below and explicitly defers proving unaided
human play. The full original-game reproduction remains the immediate goal.

### Player experience

- Save a race locally and race against a ghost of the player's own previous run.
- Share runs through online leaderboards and download other players' ghosts.
- Display multiple downloaded ghosts together, with roughly 20 other racers on
  the track as an initial aspiration. This is not a measured performance limit.
- Play against downloaded recordings locally. Live synchronization among the
  racers is outside this idea's initial scope.

The proposed ghosts are visual opponents that cannot collide with riders or
change gameplay state. Adding or removing a displayed ghost must not affect
the player's race or time. Fixed race conditions still need to account for
anything in the underlying game that affects a run, including an original
opponent if the chosen mode retains one. A future implementation task must
define that mode explicitly within the project's Classic/Extended boundaries.

### Replay verification for leaderboards

The proposed trust boundary is a hosted verification service running the
approved native simulation. Public source code is compatible with this model:
players control their clients, while the service controls the computation that
admits results to its leaderboard.

1. Record controller inputs at each documented simulation update, together with
   track identity, rules version and approved race settings.
2. Upload the input recording. Treat a client-reported time as a claim.
3. Start a fresh race on the service using its approved initial state, track
   data, rules and any required RNG state. Do not accept client-supplied
   arbitrary states, content or memory writes.
4. Replay inputs through the native simulation and calculate the actual finish
   and time using its preserved integer arithmetic, ordering and timing.
5. Publish a result only if the run completes under the approved conditions.
   Retain the replay and, if useful, generate position and animation data for
   ghost display from the verified run.

This would reject invented times and altered movement that cannot reproduce
under approved conditions. A future task must establish deterministic replay
across the supported environments and define allowed inputs, completion rules,
version compatibility and bounded upload/verification resources. Hashes identify
rules, content and recordings; they do not establish legitimacy by themselves.

Competitive replays need a restricted format. The existing
[laboratory replay formats](BUILD_AND_VALIDATION.md#rom-and-replay-identity)
support research setup and cartridge RAM writes; those capabilities must not
become accepted ranked input. The verifier would use native code, without
executing original ROM code inside the product or service.

### Storage and playback

Personal ghosts can remain local with no account or service. For shared
leaderboards, a proposed database holds accounts, verified times, track/rules
identities and replay references; file or object storage holds compressed
replays and any generated ghost trajectories. The client downloads selected
ghosts before racing and caches them for local playback.

Keep incompatible rule or gameplay-content revisions in separate leaderboard
categories. Selecting a host, storage provider, account system, retention policy
or operating budget is deferred. No service, deployment or spending is
authorized by this note.

### Human play verification is deferred

A legal input sequence can be produced through slow motion, save states,
editing, a bot or copying another replay. Independent replay verification
establishes that the run follows the approved game rules; it does not prove
unaided human play or authorship. The user considers that a later problem,
not a requirement for the first ghost or leaderboard feature.

Live input submission with server-controlled deadlines, suspicious-input
analysis and supervised or video-supported records are possible later topics.
They are not commitments, and live submission alone would not eliminate bots
or prepared input playback.

### Prior art and a possible sequence

Trackmania's Nations/United Forever replay investigation distinguishes physics
validation from slow-motion cheating; the community's Competition Patch adds
checks aimed at input injection and game-speed manipulation. See the
[first-hand investigation](https://donadigo.com/tmx1) and
[patch description](https://donadigo.com/tmcp). DDNet publishes ranks from its
[official servers](https://ddnet.org/) and has a
[moderation policy for cheat clients and bots](https://github.com/ddnet/ddnet-rules/blob/master/Moderation%20Procedure.md).
These are examples to study, not guarantees for this project.

A possible implementation sequence, when explicitly taken up, is personal
ghosts, then shared replay-verified leaderboards and multiple-ghost playback,
then stronger competitive verification if needed. The detailed replay schema,
UI, rendering performance and hosting design remain open.
