#pragma once
// The announcement queues: trick rewards and their speed boosts, voices and captions.
// Internal to the race engine; the public interface is zoom_zoo_movement.hpp.

#include "zoom_zoo_movement.hpp"

namespace unirally {

// The opponent's queue for one update: queue `published_event` (0 for none), show the
// next announcement when its turn comes, and reward it. While rider 1's tutorial hints run
// (`hints_active`) an announcement holds as long as a hint.
void update_opponent_announcements(MovementState& state, unsigned published_event,
                                   bool hints_active, const MovementContent& content,
                                   std::span<std::uint8_t> learned_weights);
void queue_opponent_announcement(MovementState& state, unsigned event);
void queue_player_announcement(ZoomZooState& state, unsigned event);
// Announce the tricks of `rider`'s landing (0 the player, 1 the opponent).
void announce_landing_tricks(ZoomZooState& state, unsigned rider, const ZoomZooContent& content);
void show_next_player_announcement(ZoomZooState& state, const MovementContent& content,
                                   std::span<const std::uint8_t> captions);
void push_front_player_announcement(ZoomZooState& state, unsigned event);
// $83:CDBC-CEAF: both riders' tutorial hint groups, the player's first.
void update_tutorial_hints(ZoomZooState& state);
// $81:8709-8718: each rider's pass lowers both queues' cooldowns by 1, not below 0; a stunt
// event runs one pass (rider_passes), a race two. A race restored from a capture has no player
// queue to lower.
void lower_announcement_cooldowns(ZoomZooState& state, const ClassicRaceScenario& scenario);

} // namespace unirally
