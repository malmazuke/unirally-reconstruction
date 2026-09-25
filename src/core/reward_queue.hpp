#pragma once
// The announcement queues: trick rewards and their speed boosts, voices and captions.
// Internal to the race engine; the public interface is zoom_zoo_movement.hpp.

#include "zoom_zoo_movement.hpp"

namespace unirally {

void update_reward_queue(MovementState& state, unsigned event_one, const MovementContent& content,
                         std::span<std::uint8_t> learned_weights);
void enqueue_zoom_opponent(MovementState& state, unsigned event);
void enqueue_zoom_player(ZoomZooState& state, unsigned event);
void update_zoom_landing_rewards(ZoomZooState& state, unsigned index,
                                 const ZoomZooContent& content);
void consume_zoom_player(ZoomZooState& state, const MovementContent& content,
                         std::span<const std::uint8_t> captions);
void push_front_zoom_player(ZoomZooState& state, unsigned event);
void update_zoom_hints(ZoomZooState& state);

} // namespace unirally
