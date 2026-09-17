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
// The race engine was first recovered on ZOOM ZOO, hence the ZoomZoo names.
// DRAGSTER runs the same original routines with its own track content and
// scenario (DRAGSTER-ORDINARY-CONTROLS, R-0038).
enum class ClassicRaceTrack : std::uint8_t { ZoomZoo=0, Dragster=1 };
struct ClassicRaceScenario {
    ClassicRaceTrack track{};
    // Original frame number at the race initialization boundary: the end of
    // the frame before fade `$0FF1` first advances. It labels native updates
    // so they align with original captures; menu timing sets it, not physics.
    std::uint32_t initialization_frame{};
    // One-player race length `$77:0744`; `$82:DBA6-DBB2` stores laps + 1 in
    // `$0EFB/$0EFD`, the extra count being the initial start-line crossing.
    std::uint16_t laps{};
    // Result-loading updates until the result screen is stable:
    // player won, player lost.
    std::uint16_t stable_result_won{}, stable_result_lost{};
    // Race mode `$77:074B`: 1 for the ZOOM ZOO tour race, 0 for DRAGSTER.
    // Besides the laps above it selects the speed-limiter progress adjustment
    // bound `$1281` (72 or 96, see race_adjustment_limit), the final-lap
    // announcement ($81:81AE) and the result screen: mode 1 publishes the lap
    // graph extrema at load 106 ($83:904A-90F0); the mode-0 screen publishes none.
    bool tour_race{};
};
// $83:CC59-CC7C: 0x48 (mode 1) or 0x60 (mode 0) minus `$1283`, which is zero
// on every authenticated frame of both tracks' references (guarded).
std::uint16_t race_adjustment_limit(const ClassicRaceScenario& scenario);
ClassicRaceScenario classic_race_scenario(ClassicRaceTrack track);
// $81:A304-A51B: decoded track byte 13 selects one of the fixed playfields of
// 16,384 64-unit coarse cells. Zero selects 1,024 columns (DRAGSTER) and 0x40
// selects 256 (ZOOM ZOO); x wraps with `$0D4F`, columns * 64 - 1. The same arm
// sets the camera and visibility scale: world x is shifted left by `$03F1`
// before comparison with the follow window `$03F3/$03F5` ($81:9FB0-A05D) and
// the visible span `$0425/$0427` ($82:AD0F-AD28).
struct TrackGeometry {
    std::uint16_t coarse_columns{}, position_mask{};
    unsigned screen_shift{};
    std::int16_t follow_window_low{}, follow_window_high{};
    std::int16_t visible_left{}, visible_right{};
};
TrackGeometry track_geometry(std::span<const std::uint8_t> decoded_track);

struct ZoomZooState {
    ClassicRaceTrack track{ClassicRaceTrack::ZoomZoo}; // Serialized as the state magic.
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
// The same initializer for the one-player, one-lap CRAWLER/DRAGSTER race.
ZoomZooState classic_crawler_dragster_race_start(const ZoomZooContent& content);
ZoomZooState classic_race_start(const ZoomZooContent& content,const ClassicRaceScenario& scenario);
// Identity of a DRAGSTER race state on the shared engine; same 742-byte layout as URZZ000B.
inline constexpr std::array<std::uint8_t,8> dragster_race_state_magic{'U','R','D','G','0','0','0','1'};
bool classic_race_player_won(const ZoomZooState& state);
// Result-loading update at which the result screen is stable (restart allowed).
std::uint16_t stable_result_updates(const ZoomZooState& state);
// Race Again selects the same clean scenario after the stable result.
void restart_zoom_zoo(ZoomZooState& state,const ZoomZooContent& content);
// Validate content-dependent restore invariants before emitting or advancing a state.
void validate_zoom_zoo_content_state(const ZoomZooState& state,const ZoomZooContent& content);
// Historical continuation and native scenario share this update path.
void update_zoom_zoo(ZoomZooState& state,const ControllerButtons& buttons,
                     const ZoomZooContent& content);
} // namespace unirally
