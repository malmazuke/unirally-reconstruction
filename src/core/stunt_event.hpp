#pragma once
// The stunt event's own rules (race mode 2, R-0066): the clock that counts down to the end of
// the run and the finish it allows, the trick tallies, and the finish's caption by score.
// Internal to the race engine; the public interface is zoom_zoo_movement.hpp.

#include "zoom_zoo_movement.hpp"

#include <cstdint>

namespace unirally {

// The riders whose own update runs: both, unless the opponent is switched off. $81:8E53-8E5D
// (contact) and $82:8EAB-8EB5 (the rider update) skip the opponent while `$0DE1` (a second
// human) and the AI flag `$0C6D` are both clear: in a stunt event, whose setup clears the flag
// ($83:CBD8). Its finish and its announcement queue still run.
unsigned rider_passes(const ClassicRaceScenario& scenario, bool two_human = false);
// $81:C7CE-C867, the race clock of a stunt event: once `running` (the start countdown below 68,
// as a race's clock) it counts down a tenth every five updates. The tick after 0:00.0 stops it;
// from then on every update finishes each rider that can finish (see stunt_rider_can_finish).
// Each whole second from 0:05.0 to 0:00.0 sounds the clock's warning (`race_sound`).
void update_stunt_clock(ZoomZooState& state, bool running);
// $83:E7C1-E8DD, a stunt event's finish before the race's finish routine: each finished rider
// that stands on the ground settles (see StuntEvent::settled); once both have finished and
// stand, the tutorial hints end, and once both queues are also empty the finish display starts
// on the next update. Returns true when it runs this update: the finish poses and captions
// ($83:E8E0) and the count towards the result load.
bool update_stunt_finish(ZoomZooState& state);
// $81:C836-C861: a rider finishes at the clock's end once it is supported (fewer than two
// updates unsupported) and not falling fast (vertical velocity at least -256).
bool stunt_rider_can_finish(const RiderMovementState& rider);
// $81:C0FF-C116 and $81:C173-C184 (the player's; rider 1's $81:C24E-C2C3): a shown trick of
// reward class `trick_class` (the class table, physics.reward.rotation-class) counts in its
// family's column, and its column adds the `paid` points (the weight the reward added to the
// score, 0 for none).
void tally_stunt_trick(StuntTallies& tallies, std::uint8_t trick_class, std::uint8_t paid);
// $83:E940-E957 (the player; $83:EAD2-EAE7 the opponent): a rider's finish caption in a stunt
// event is `draw` for a score equal to the qualifying score, else `loser` when the 16-bit
// difference is negative and `winner` when it is not.
std::uint8_t stunt_finish_announcement(std::uint16_t score, std::uint16_t qualifying_score);

} // namespace unirally
