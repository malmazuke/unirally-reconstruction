#pragma once
// The announcement queues: trick rewards and their speed boosts, voices and captions.
// Internal to the race engine; the public interface is zoom_zoo_movement.hpp.

#include "zoom_zoo_movement.hpp"

namespace unirally {

// The opponent's queue for one update: queue `published_event` (0 for none), show the
// next announcement when its turn comes, and reward it.
void update_opponent_announcements(MovementState& state, unsigned published_event,
                                   const MovementContent& content,
                                   std::span<std::uint8_t> learned_weights);
void queue_opponent_announcement(MovementState& state, unsigned event);
void queue_player_announcement(ZoomZooState& state, unsigned event);
// Announce the tricks of `rider`'s landing (0 the player, 1 the opponent).
void announce_landing_tricks(ZoomZooState& state, unsigned rider, const ZoomZooContent& content);
void show_next_player_announcement(ZoomZooState& state, const MovementContent& content,
                                   std::span<const std::uint8_t> captions);
void push_front_player_announcement(ZoomZooState& state, unsigned event);
void update_tutorial_hints(ZoomZooState& state);

} // namespace unirally
