#pragma once
#include "movement.hpp"

namespace unirally {
struct ReflectionTransition {
    std::uint16_t step{}, end{}, pose_base{}, pose_override{}, completed{}, hold{};
    std::uint16_t drive_pose_enabled{}, air_turns{}, direction_latch{}, base_velocity_cap{};
    std::uint16_t brake_input{}, rotate_negative_input{}, rotate_positive_input{}, jump_input{};
    std::uint16_t wrong_direction_counter{};
};
struct SurfaceTransition {
    std::uint16_t mode{}, angle{}, tile_mode{}, leading_support{}, tile_pose{}, animation_delta{}, tile_pose_enabled{};
};
struct ZoomZooFinishPose {
    std::uint16_t selector{}, kind{}, locked{}, active{};
};
struct ZoomZooRaceRider {
    std::uint16_t laps_remaining{}, checkpoint{}, next_checkpoint{}, start_line_latch{};
    std::uint16_t checkpoint_display_countdown{}, finished{};
    std::array<std::uint16_t,5> time_digits{};
};
struct ZoomZooCamera {
    std::uint16_t x{}, y{}, velocity_x{}, velocity_y{}, lookahead{}, screen_xy{};
};
struct ZoomZooRaceState {
    ZoomZooCamera camera;
    std::array<ZoomZooFinishPose,2> finish_pose;
    std::array<std::uint8_t,20> checkpoint_seen{};
    std::array<ZoomZooRaceRider,2> riders;
    std::array<std::array<std::uint16_t,10>,2> lap_times;
    std::array<std::uint16_t,2> total_times{};
    std::uint16_t provisional_1225{}, provisional_1227{}, finish_delay{};
};
struct ZoomZooResult {
    std::uint16_t graph_minimum{}, graph_maximum{};
    std::array<std::uint16_t,2> published_totals{};
    bool operator==(const ZoomZooResult&) const = default;
};
struct ZoomZooPlayerAnnouncements {
    RewardQueueState queue;
    std::uint16_t hints_active{}, hint_updates{}, hint_group{}, empty_display{};
};
struct ZoomZooRoll {
    // $829398-9714. Word step is signed; all other values retain original bits.
    std::uint16_t input_latched{}, prior_orientation{}, prior_reflection{}, pose_base{};
    std::uint16_t step{}, held_updates{}, bounce_charge{}, completed_rolls{};
    std::uint16_t held_rotations{}, bounce_active{}, support_count_mirror{}, prior_step{};
};
struct ZoomZooPause {
    std::uint16_t selection{}, released{}; // $0EF3: 0/racing, 1/resume, -1/authored restart (original Retire); $0EF5.
    std::uint32_t suspended_updates{}, suspended_countdown_updates{}; // Semantic update clocks.
};
struct ZoomZooState {
    ZoomZooPause pause;
    std::array<ZoomZooRoll,2> rolls{};
    std::array<std::array<std::uint8_t,25>,2> learned_weights{}; // Events2–26; event1 remains in each queue.
    bool native_initialization{};
    ZoomZooResult result;
    ZoomZooPlayerAnnouncements player_announcements;
    std::array<std::uint16_t,2> charge_announced{}; // $0D53/$0D55, audio latch only.
    std::uint16_t fade_level{};
    std::uint16_t result_updates{};
    std::array<std::uint16_t,2> start_boost{};
    bool complete_race{};
    ZoomZooRaceState race;
    bool sustained{};
    std::array<SurfaceTransition,2> surface;
    MovementState movement;
    std::array<ReflectionTransition,2> reflection;
    std::uint8_t opponent_horizontal{};
    std::uint8_t opponent_retained_oam_x{};
};
struct ZoomZooContent {
    MovementContent movement;
    std::span<const std::uint8_t> slope_coefficients;
    std::span<const std::uint8_t> reflection_pose_table;
    std::span<const std::uint8_t> landing_matrices;
    std::span<const std::uint8_t> finish_poses;
    std::span<const std::uint8_t> roll_poses;
    std::span<const std::uint8_t> roll_directions;
    std::span<const std::uint8_t> reward_weights;
    std::span<const std::uint8_t> trick_combinations;
};
// $82:9715–979D: count active updates opposing the track direction, with
// original wrapped word comparisons at velocities -16 and +16 (1/32 units).
std::uint16_t next_wrong_direction_counter(std::uint16_t previous,
    std::uint16_t velocity_x,std::uint16_t marker,unsigned horizontal,bool native_rewards=false);
std::vector<std::uint8_t> serialize_zoom_zoo(const ZoomZooState& state);
ZoomZooState deserialize_zoom_zoo(std::span<const std::uint8_t> bytes);
// $82:D7C6-DBD6, authenticated track header and one-player three-lap scenario.
ZoomZooState classic_crawler_zoom_zoo_start(const ZoomZooContent& content);
// Race Again selects the same clean scenario after the stable result.
void restart_zoom_zoo(ZoomZooState& state,const ZoomZooContent& content);
// Validate content-dependent restore invariants before emitting or advancing a state.
void validate_zoom_zoo_content_state(const ZoomZooState& state,const ZoomZooContent& content);
// Historical continuation and native scenario share this update path.
void update_zoom_zoo(ZoomZooState& state,const ControllerButtons& buttons,
                     const ZoomZooContent& content);
} // namespace unirally
