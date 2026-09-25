#pragma once

#include "flat_contact.hpp"
#include "input_timer.hpp"
#include "speed_limits.hpp"
#include "track_progress.hpp"

#include <array>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace unirally {

// Semantic continuation state for the recovered Dragster domain. Every word is
// an original 16-bit bit pattern; signed interpretation belongs to the update
// that consumes it. R-0011-motion records the source addresses and ordering.
struct JumpState {
    std::uint16_t pending{}, impulse_phase{}, baseline{}, previous_input{};
};
struct PoseState {
    std::uint16_t orientation{}, reflected_orientation{}, animation_phase{};
    std::uint16_t animation_increment{}, previous_x{}, previous_y{};
    std::uint16_t displacement_remainder{}, target_orientation{}, pose_index{};
    std::array<std::uint16_t, 3> displacement_history{};
    std::uint16_t rolling_level{}, alternate_animation_phase{};
    bool rolling{}, reflected{};
};
// $82:A0B7-$82:A236. Names describe the observed recurrence without assigning
// an unverified gameplay meaning to the oscillator's control words.
struct IdlePoseState {
    std::uint16_t active{}, wobble_offset{}, bias{}, velocity{};
    std::uint16_t previous_bias{}, direction_adjustment{}, cycle_latched{};
    std::uint16_t cycle_counter{}, orientation_reference{};
};
struct QuarterTurnState {
    std::uint16_t previous_quadrant{}, forward_turns{}, reverse_turns{};
    std::uint16_t forward_quarters{}, reverse_quarters{};
    bool initialized{}, reflected_at_start{};
};
struct RiderMovementState {
    ContactMotion motion{};
    RiderContactState contact{};
    SpeedModifiers speed{};
    TrackProgress progress{};
    JumpState jump{};
    PoseState pose{};
    IdlePoseState idle_pose{};
    QuarterTurnState quarter_turn{};
    std::uint16_t residue_x{}, residue_y{}, throttle{}, previous_brake{};
    std::uint16_t launch_override{}, small_motion_counter{};
};
struct OpponentContinuationState {
    std::uint16_t impulse_countdown{}, trick_selector{}, suppression_counter{};
};
struct RewardQueueState {
    std::array<std::uint8_t, 32> entries{};
    std::uint8_t read_cursor{}, write_cursor{};
    std::uint16_t cooldown{}, feature_total{};
    std::uint8_t event_one_weight{};
};
enum class RacePhase : std::uint8_t {
    Racing = 0,
    FinishDelay = 1,
    ResultLoading = 2,
    ResultScreen = 3
};
enum class RaceOutcome : std::uint8_t { Pending = 0, PlayerWon = 1, PlayerLost = 2 };
struct RaceFinishState {
    std::array<bool, 2> rider_finished{};
    std::array<std::uint16_t, 2> finish_time_centiseconds{};
    std::array<std::array<std::uint16_t, 5>, 2> finish_time_digits{};
    std::array<std::uint16_t, 2> finish_animation_countdown{};
    // $11E7 table selector for the opponent's $17:C7D6 finish-pose cycle.
    // Value 48 names the negative-sentinel entry; that call resets it to zero.
    std::uint16_t opponent_finish_pose_selector{};
    std::uint16_t player_finish_delay{}, result_loading_updates{};
    RacePhase phase{RacePhase::Racing};
    RaceOutcome outcome{RaceOutcome::Pending};
};
struct MovementState {
    std::uint32_t frame{};
    ControllerSample player_input{};
    std::array<RiderMovementState, 2> riders{}; // player, opponent
    RaceTimerDigits timer{};
    OpponentContinuationState opponent_ai{};
    RewardQueueState rewards{};
    std::uint16_t countdown{};
    std::uint8_t contact_phase{}, progress_phase{}, animation_counter{}, update_counter{};
    RaceFinishState finish{};
};

struct MovementContent {
    SamplingContent sampling{};
    FlatContactContent flat_contact{};
    std::span<const std::uint8_t> progress_transitions;
    std::span<const std::uint8_t> pose_slopes;
    std::span<const std::uint8_t> displacement_table;
    std::span<const std::uint8_t> idle_pose_table;
    std::span<const std::uint8_t> rotation_reward;
    std::span<const std::uint8_t> rotation_class;
    SpeedDecayContent speed_decay{};
};

inline constexpr std::array<std::uint8_t, 8> movement_state_magic{'U', 'R', 'M', 'V',
                                                                  '0', '0', '0', '1'};
inline constexpr std::array<std::uint8_t, 8> movement_state_magic_v2{'U', 'R', 'M', 'V',
                                                                     '0', '0', '0', '2'};
inline constexpr std::array<std::uint8_t, 8> movement_state_magic_v3{'U', 'R', 'M', 'V',
                                                                     '0', '0', '0', '3'};

// $83:E90D-$83:E932 non-crossing finish-display velocity adjustment.
// Input/output are signed 16-bit bit patterns in 1/32 units per update.
std::uint16_t finish_speed_toward_zero(std::uint16_t velocity);

// Fixed-order little-endian encoding. It contains semantic continuation fields
// only: no WRAM image, CPU registers, frame-indexed events or captured calls.
std::vector<std::uint8_t> serialize_movement_state(const MovementState& state);
MovementState deserialize_movement_state(std::span<const std::uint8_t> bytes);

// Public, capture-free semantic start for the accepted PAL CRAWLER/DRAGSTER
// slice. See docs/content/classic-pack-v1.md for its frozen identity.
MovementState classic_crawler_dragster_start();

// Advance one PAL game update in the recovered CRAWLER/DRAGSTER domain. This
// semantic path closes the frozen CRAWLER/DRAGSTER gameplay and finish/result
// continuation. No reference row or frame-indexed event enters it.
void update_movement(MovementState& state, const ControllerButtons& player_buttons,
                     const MovementContent& content);

} // namespace unirally
