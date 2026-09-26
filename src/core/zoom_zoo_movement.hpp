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
    std::uint16_t mode{}, angle{}, tile_mode{}, leading_support{}, tile_pose{}, animation_delta{},
        tile_pose_enabled{};
};
// R-0047: per-rider words of the special tiles (mud, flag pair 14; corkscrew,
// pair 10), which no accepted DRAGSTER or ZOOM ZOO race reaches. The other
// tracks' state (URTRnn05) always carries them; DRAGSTER's and ZOOM ZOO's carry
// them only while one is live (URDG0004, URZZ000E), so their 742-byte states
// are unchanged. Words keep the original bit patterns; signed where noted.
struct SpecialTileRider {
    // $0BCB/$0BCD ($0F45): 4 on each update a mud tile holds the rider, then
    // counts down one per update.
    std::uint16_t mud_cooldown{};
    // $0D57/$0D59 ($0F47): 4 on mud; cleared on the first update the
    // cooldown has already run out (the original also queues sound 0x213).
    std::uint16_t mud_exit_pending{};
    // $0DF7/$0DF9 ($0F49), signed: 1 after an update the corkscrew held the
    // rider, -4 after an ejection counting up to 0, otherwise 0.
    std::uint16_t corkscrew_latch{};
    // $0DF3/$0DF5 ($0FA7): the corkscrew step, 1 to 0x31; 0xFFFF after an ejection.
    std::uint16_t corkscrew_step{};
    // $0DFF/$0E01 ($0F4B): gravity is suspended while set.
    std::uint16_t corkscrew_float{};
    // $0547/$0549: updates left with the drive, speed limit and gravity
    // routines suspended; 8 on each corkscrew step.
    std::uint16_t physics_hold{};
    // $0BE7/$0BE9 ($0F2F): updates left with the reflection transition
    // locked; counts down only while surface mode is clear.
    std::uint16_t reflection_lock{};
    // $1516/$151A bit 4 ($0FAF): the rider object at OBJ priority 3 instead of
    // 2, so no BG1 tile covers it; toggled through the corkscrew.
    std::uint16_t raised_priority{};
    // $0351/$0353 ($0F51 at entry): the loop's direction, 0 entering against
    // a mirrored descriptor facing right-to-left (reflected), 1 the other way.
    std::uint16_t loop_direction{};
    // $0355/$0357, signed: the loop step, 1 to 16 while the loop (flag pair
    // 26) carries the rider, 0xFFFE after a refused entry counting up to 0.
    std::uint16_t loop_step{};
    // $0359/$035B: 3 on each loop update, then one less per update; at 1 the
    // loop's pose and float end. It also holds off the jump ($82:A8D0).
    std::uint16_t loop_cooldown{};
    // $0D3D/$0D3F ($0F29): flag pair 8's counter, one more per update on the
    // tile and one less per update ($81:8594); at 8 velocity x is held to
    // +-0x20 instead of counting on.
    std::uint16_t slow_counter{};
    bool operator==(const SpecialTileRider&) const = default;
};
struct ZoomZooFinishPose {
    std::uint16_t selector{}, kind{}, locked{}, active{};
};
struct ZoomZooRaceRider {
    std::uint16_t laps_remaining{}, checkpoint{}, next_checkpoint{}, start_line_latch{};
    std::uint16_t checkpoint_display_countdown{}, finished{};
    std::array<std::uint16_t, 5> time_digits{};
};
struct ZoomZooCamera {
    std::uint16_t x{}, y{}, velocity_x{}, velocity_y{}, lookahead{}, screen_xy{};
};
// A lap slot or total not run holds 60000 (0xEA60), as the cartridge's records do; the
// result screens show it as NO TIME.
constexpr std::uint16_t no_time = 60000;

// A slot's first-seen flag keeps bit 7 set until a rider first crosses the slot.
inline bool slot_not_yet_crossed(std::uint8_t first_seen_flag) {
    return (first_seen_flag & 0x80U) != 0;
}

struct ZoomZooRaceState {
    ZoomZooCamera camera;
    std::array<ZoomZooFinishPose, 2> finish_pose;
    // $114D-$119C: 80 first-seen flags, laps remaining * 4 + checkpoint
    // ($81:CD25-CD2E fills them with 0xFF; the first crossing of a slot clears its flag, see
    // slot_not_yet_crossed). The shared 742-byte layout holds the
    // first 20, all a race of up to four laps reaches; the other tracks' layout
    // (URTRnn05) holds all 80 (R-0048).
    std::array<std::uint8_t, 80> checkpoint_seen{};
    std::array<ZoomZooRaceRider, 2> riders;
    std::array<std::array<std::uint16_t, 10>, 2> lap_times;
    std::array<std::uint16_t, 2> total_times{};
    std::uint16_t provisional_1225{}, provisional_1227{}, finish_delay{};
};
struct ZoomZooResult {
    std::uint16_t graph_minimum{}, graph_maximum{};
    std::array<std::uint16_t, 2> published_totals{};
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
    std::uint16_t selection{},
        released{}; // $0EF3: 0/racing, 1/resume, -1/authored restart (original Retire); $0EF5.
    std::uint32_t suspended_updates{}, suspended_countdown_updates{}; // Semantic update clocks.
};
// The race engine was first recovered on ZOOM ZOO, hence the ZoomZoo names.
// DRAGSTER runs the same original routines with its own track content and
// scenario (DRAGSTER-ORDINARY-CONTROLS, R-0038), and so do the other race
// tracks with a recovered scenario (TRACK-BREADTH, R-0046).
//
// A track is its index in the ROM's track set: SRAM `$77:074A`, from which the
// race loader unpacks asset `0xC2 + index` (`$82:E140-E152`). DRAGSTER is track
// 0 and ZOOM ZOO track 1.
struct ClassicRaceTrack {
    std::uint8_t index{1};
    static const ClassicRaceTrack Dragster, ZoomZoo;
    friend constexpr bool operator==(ClassicRaceTrack, ClassicRaceTrack) = default;
};
inline constexpr ClassicRaceTrack ClassicRaceTrack::Dragster{0};
inline constexpr ClassicRaceTrack ClassicRaceTrack::ZoomZoo{1};
// Opponent characters (`$77:0749`): BRONSEN, then SILVIA and GOLDWYN once the rider holds a
// bronze or a silver on the tour (NOW PLAYING, `$80:B31F-B346`), and ANTI-UNI on HUNTER's.
namespace opponent {
inline constexpr std::uint8_t bronsen = 17, silvia = 18, goldwyn = 19, anti_uni = 20;
} // namespace opponent
// The riders of a one-player race: the player's character `$77:0748` (PICK YOUR UNI's 0-15, MIKE
// 0) and the opponent's `$77:0749`. The rider changes no race physics; it chooses the player's
// voices, tutorial bit, sprite palette and ink colour (R-0061).
struct RacePairing {
    std::uint8_t rider{};
    std::uint8_t opponent{opponent::bronsen};
    friend constexpr bool operator==(RacePairing, RacePairing) = default;
};
inline constexpr unsigned rider_characters = 16;
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
    // Race mode `$77:074B`: 1 for a lap race (ZOOM ZOO), 0 for a one-run race
    // (DRAGSTER); 2, the stunt event, has no native scenario.
    // Besides the laps above it selects the speed-limiter progress adjustment
    // bound `$1281` (72 or 96, see race_adjustment_limit), the final-lap
    // announcement ($81:81AE) and the result screen: mode 1 publishes the lap
    // graph extrema at load 106 ($83:904A-90F0); the mode-0 screen publishes none.
    bool tour_race{};
    // `$131F`: the HUNTER tour, whose race runs the tag effects ($83:CEC9, R-0052).
    bool hunter_tour{};
    // The rider and opponent: MIKE against BRONSEN (ANTI-UNI on the HUNTER tour) unless the
    // menus chose others.
    RacePairing pairing{};
    // `$12E3` at the start ($82:D94C-D96F): the player's tutorial hints run unless its rider's
    // bit is set in the cartridge RAM's `$77:1116`, which a race sets once its hints end.
    bool tutorial_hints{true};
};
// The opponent's tier, which `$83:CC0B-CC7C` sets at the race's setup from the opponent and the
// track: the AI level `$1275` (opponent - 16: BRONSEN 1, SILVIA 2, GOLDWYN 3), the catch-up term
// `$1283` the opponent's speed cap gains while the player leads (0; SILVIA the track's catch-up
// byte + 0x20, GOLDWYN + 0x40) and the progress adjustment bound `$1281` (0x60 for a one-run race,
// 0x48 for a lap race, less the catch-up unless that goes below zero). The HUNTER tour
// ($131F nonzero) sets level 3, 0x40 and 0x60 directly (LOCKED-TOURS).
struct OpponentTier {
    std::uint16_t ai_level{1};
    std::uint16_t catch_up{};
    std::uint16_t adjustment_limit{0x60};
    friend constexpr bool operator==(const OpponentTier&, const OpponentTier&) = default;
};
// `catch_up_by_track` is the `$83:C8B3` table (race.opponent-catch-up); only SILVIA and
// GOLDWYN read it, so BRONSEN's and HUNTER's tiers need none.
OpponentTier opponent_tier(const ClassicRaceScenario& scenario,
                           std::span<const std::uint8_t> catch_up_by_track);
// The scenario of a race track with MIKE against its usual opponent; throws for a track
// without a recovered one.
ClassicRaceScenario classic_race_scenario(ClassicRaceTrack track);
// The same with the menus' pairing; throws for a pairing the one-player menus cannot choose
// (a rider past 15, an opponent other than BRONSEN, SILVIA and GOLDWYN, or on HUNTER's tracks
// other than ANTI-UNI).
ClassicRaceScenario classic_race_scenario(ClassicRaceTrack track, RacePairing pairing,
                                          bool tutorial_hints = true);
bool classic_race_has_scenario(ClassicRaceTrack track);
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
// Whether a rider starts the race reflected: the parity of its start y word in
// the track header (bytes 5-6 for the player, 9-10 for the opponent; the x
// words precede each), as `classic_race_start` sets `pose.reflected`
// (`$0BA7`/`$0BA9`). Race setup also latches the player's word into `$1229`
// for the countdown windows.
bool classic_race_start_reflected(std::span<const std::uint8_t> decoded_track, unsigned rider);

// The HUNTER tour's eight effects, by their index in HunterEffects::effect (R-0052).
namespace hunter_effect {
inline constexpr unsigned barf_mode = 0;        // BG2's axes swap
inline constexpr unsigned hedgehog_speed = 1;   // growing, then shrinking, freezes
inline constexpr unsigned power_bounce = 2;     // the player lands with matrix 0
inline constexpr unsigned screen_flip = 3;      // the picture upside down
inline constexpr unsigned invisible_track = 4;  // BG1 off
inline constexpr unsigned slow_motion = 5;      // three updates in four skipped
inline constexpr unsigned wobble_mode = 6;      // mosaic
inline constexpr unsigned control_reversed = 7; // left and right, the rotations, Y and A
inline constexpr unsigned count = 8;
} // namespace hunter_effect
// R-0052: the HUNTER tour's tag effects ($83:CEC9). When the riders' boxes
// overlap, the player's x picks one of eight effects, each announced at the
// front of the player's queue and most timed over 500 updates. Words keep the
// original bit patterns.
struct HunterEffects {
    std::uint16_t latched{}; // $1325: the progress counts have once differed by 2 or more
    std::uint16_t active{};  // $1323: an effect is running
    // $1327-$1335 (effect k at $1327 + 2k): 1 on the update the tag picks it,
    // 2 while it runs.
    std::array<std::uint16_t, 8> effect{};
    // The 500-update timers: effect 0 $0557, 2 $12B7, 3 $7E:2052, 4 $055B,
    // 5 $12B5, 6 $0561, 7 $12CD; effect 1 has none (index 1 stays 0).
    std::array<std::uint16_t, 8> timer{};
    // Effect 1's freeze pulses, bytes: $1285 updates left to skip, $1287 set
    // once the pulse has grown to 20, $1289 the pulse length.
    std::uint16_t pulse{}, pulse_shrinking{}, pulse_length{};
    std::uint16_t blink{};       // $7E:2054: effect 3's HDMA picture is on
    std::uint16_t wave_phase{};  // $7E:26BE: effect 3's alternating table
    std::uint16_t hide_track{};  // $055D: effect 4 takes BG1 off the main screen
    std::uint16_t mosaic{};      // $055F: effect 6's mosaic is on
    std::uint16_t skip_update{}; // $128B: the next update's race routines are skipped
    std::uint16_t message{};     // $12AF: the HUD message event to show, 0 once shown
    // $11C1: an announcement was shown since the queue last ran dry. The
    // original keeps it on every track; only the HUNTER tour reads it.
    std::uint16_t shown{};
    // $128D-$129C: the event whose caption the HUD message buffer holds (0
    // blank); the empty announcement row shows it ($81:BF32-BFB7).
    std::uint16_t hud_event{};
    // $0EA7-$0EB6: the caption row on screen, as the smallest event whose
    // caption has its text (0 blank). On the HUNTER tour a front-of-queue
    // announcement overwrites the queue slot the row was drawn from, so the
    // row is carried rather than read back from the queue (R-0052).
    std::uint16_t caption{};
    // $0563 (byte): the race NMI counts it while effect 6 runs ($80:8821-8835);
    // its low three bits are the mosaic size of the next picture.
    std::uint16_t mosaic_counter{};
    bool operator==(const HunterEffects&) const = default;
};
struct ZoomZooState {
    ClassicRaceTrack track{ClassicRaceTrack::ZoomZoo}; // Serialized as the state magic.
    ZoomZooPause pause;
    std::array<ZoomZooRoll, 2> rolls{};
    std::array<std::array<std::uint8_t, 25>, 2>
        learned_weights{}; // Events2–26; event1 remains in each queue.
    bool native_initialization{};
    ZoomZooResult result;
    ZoomZooPlayerAnnouncements player_announcements;
    std::array<std::uint16_t, 2> charge_announced{}; // $0D53/$0D55, audio latch only.
    std::uint16_t fade_level{};
    std::uint16_t result_updates{};
    std::array<std::uint16_t, 2> start_boost{};
    bool complete_race{};
    ZoomZooRaceState race;
    bool sustained{};
    std::array<SurfaceTransition, 2> surface;
    MovementState movement;
    std::array<ReflectionTransition, 2> reflection;
    std::uint8_t opponent_horizontal{};
    std::uint8_t opponent_retained_oam_x{};
    std::array<SpecialTileRider, 2> special_tiles{};
    // $0E7B, one word for both riders: clear when the latest drive routine
    // ($82:98CF) took the small-displacement path, so the pose and idle
    // routines follow throttle rather than velocity. A rider whose drive is
    // suspended (physics_hold) reads the other rider's value. It is serialized
    // with the special-tile words; while none is live no drive is suspended,
    // so each rider rewrites it before reading it.
    std::uint16_t drive_target_latch{};
    // $0C73: updates left of the opponent's turnaround on a steep slope
    // ($83:E0C5-E111, LOCKED-TOURS); serialized with the special-tile words.
    std::uint16_t opponent_turnaround{};
    HunterEffects hunter;
    // Not serialized: the menus' riders and the opponent's tier set from them at the race's
    // setup. A deserialized race is MIKE's against the track's usual opponent.
    RacePairing pairing{};
    OpponentTier opponent_tier{};
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
    // $00:8088 and $00:80B8, 48 signed bytes each: the corkscrew's y step by
    // step, the second for a rider whose rolling flag is set (R-0047).
    std::span<const std::uint8_t> corkscrew_heights;
    // $81:834C, 17 signed words: the loop's x step by loop step (R-0051).
    std::span<const std::uint8_t> loop_offsets;
    // $83:D3BC, 64 bytes: the HUNTER effects' blink pattern for their first
    // and last 50 (60) updates (R-0052).
    std::span<const std::uint8_t> hunter_blink;
    // presentation.classic.captions.v1 (`$17:CA04`), sixteen characters per
    // event 1-255: the HUNTER caption row's identity is its text.
    std::span<const std::uint8_t> captions;
    // $83:C8B3, a byte per track: SILVIA's and GOLDWYN's catch-up (R-0061); empty in packs
    // before profile v21, whose races are BRONSEN's.
    std::span<const std::uint8_t> opponent_catch_up;
};
// $82:9715–979D: count active updates opposing the track direction, with
// original wrapped word comparisons at velocities -16 and +16 (1/32 units).
std::uint16_t next_wrong_direction_counter(std::uint16_t previous, std::uint16_t velocity_x,
                                           std::uint16_t marker, unsigned horizontal,
                                           bool native_rewards = false);
// R-0047: the special tiles' parts of one rider's movement update, in the
// order update_zoom_zoo runs them. Words the tiles set for the rest of one
// update only:
struct SpecialTileUpdate {
    std::uint16_t drive_step{};   // $0F3B: replaces the drive routines' 24 (mud 4, pair 12 1)
    std::uint16_t mud_velocity{}; // $0F3F: the pose target's velocity source
    std::uint16_t contact_skip{}; // $0F5B/$0DFB: skips this update's vertical contact
    bool corkscrew_stepped{};     // $81:8949 stored 1 at $0EA3 (the player's rolling flag)
    std::uint16_t slow_tile{}; // $0F2D: flag pair 8 ran (the slope nudge and pose follow velocity)
    std::uint16_t crank_brake{}; // $0FB1: flag pair 12 ran (the brake path skips the idle step)
};
// $81:8690-86FE: the counters' part of the reset before the tile dispatch.
void update_special_tile_counters(SpecialTileRider& tiles, ReflectionTransition& transition,
                                  std::uint8_t selected_high);
// $81:871C-875B: the boost tile (flag pair 2), pushing by 0x80 plus `extra`.
void apply_boost_tile(RiderMovementState& rider, SurfaceTransition& surface, std::uint16_t extra);
// $81:8999-89F6: mud (flag pair 14).
void update_mud_tile(RiderMovementState& rider, SpecialTileRider& tiles, SurfaceTransition& surface,
                     SpecialTileUpdate& special);
// $81:87C2-894F: the corkscrew (flag pair 10); `heights` is zoom.corkscrew-heights.
void update_corkscrew_tile(RiderMovementState& rider, SpecialTileRider& tiles,
                           SurfaceTransition& surface, ReflectionTransition& transition,
                           SpecialTileUpdate& special, std::span<const std::uint8_t> heights);
// $81:85AB-8622: the loop's part of the reset, after the reflection lock.
void update_loop_cooldown(RiderMovementState& rider, SpecialTileRider& tiles,
                          ReflectionTransition& transition);
// $81:837E-84AB: the loop (flag pair 26); `offsets` is zoom.loop-offsets.
void update_loop_tile(RiderMovementState& rider, SpecialTileRider& tiles,
                      SurfaceTransition& surface, ReflectionTransition& transition,
                      SpecialTileUpdate& special, std::span<const std::uint8_t> offsets);
// $83:CEC9-D600: the HUNTER tag and its effects, at the end of every update,
// skipped ones included; `blink` is zoom.hunter-blink.
void update_hunter_effects(ZoomZooState& state, std::span<const std::uint8_t> blink);
std::vector<std::uint8_t> serialize_zoom_zoo(const ZoomZooState& state);
ZoomZooState deserialize_zoom_zoo(std::span<const std::uint8_t> bytes);
// A state of a race with another pairing than MIKE against the track's usual opponent, or
// whose rider's tutorial hints had ended before it started: the layouts carry neither, nor the
// opponent's tier, which SILVIA's and GOLDWYN's take from `opponent_catch_up`
// (race.opponent-catch-up).
ZoomZooState deserialize_zoom_zoo(std::span<const std::uint8_t> bytes, RacePairing pairing,
                                  bool tutorial_hints,
                                  std::span<const std::uint8_t> opponent_catch_up);
// $82:D7C6-DBD6, authenticated track header and one-player three-lap scenario.
ZoomZooState classic_crawler_zoom_zoo_start(const ZoomZooContent& content);
// The same initializer for the one-player, one-lap CRAWLER/DRAGSTER race.
ZoomZooState classic_crawler_dragster_race_start(const ZoomZooContent& content);
ZoomZooState classic_race_start(const ZoomZooContent& content, const ClassicRaceScenario& scenario);
// Identity of a DRAGSTER race state on the shared engine; same 742-byte layout as
// URZZ000B (URDG0004 and 794 bytes while a special-tile word is live, R-0047).
inline constexpr std::array<std::uint8_t, 8> dragster_race_state_magic{'U', 'R', 'D', 'G',
                                                                       '0', '0', '0', '1'};
// Identity of any other track's race state: `URTR`, the two-digit track index,
// `05`; the 742-byte layout followed by the special-tile words with $0E7B and
// $0C73 (52 bytes) and the last 60 checkpoint-seen flags (854 bytes).
std::array<std::uint8_t, 8> classic_race_state_magic(ClassicRaceTrack track);
bool classic_race_player_won(const ZoomZooState& state);
// Result-loading update at which the result screen is stable (restart allowed).
std::uint16_t stable_result_updates(const ZoomZooState& state);
// Race Again selects the same clean scenario after the stable result.
void restart_zoom_zoo(ZoomZooState& state, const ZoomZooContent& content);
// Validate content-dependent restore invariants before emitting or advancing a state.
void validate_zoom_zoo_content_state(const ZoomZooState& state, const ZoomZooContent& content);
// Historical continuation and native scenario share this update path.
// `requested_buttons` is what a device asked for, not what a controller port
// can publish: the update applies the D-pad rocker itself, so opposing
// directions on one axis reach the race as neither (R-0041).
void update_zoom_zoo(ZoomZooState& state, const ControllerButtons& requested_buttons,
                     const ZoomZooContent& content);
} // namespace unirally
