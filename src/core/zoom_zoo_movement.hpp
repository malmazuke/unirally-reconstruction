#pragma once
#include "audio_cue.hpp"
#include "movement.hpp"
#include "rider_look.hpp"

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
// $83:EA69/$83:EBD7: the finished flag of a rider a VS race finishes when the other rider's
// banner ends; its laps, times and digits stay as they were (R-0081).
constexpr std::uint16_t forced_finish = 0xffff;

// A slot's first-seen flag keeps bit 7 set until a rider first crosses the slot.
inline bool slot_not_yet_crossed(std::uint8_t first_seen_flag) {
    return (first_seen_flag & 0x80U) != 0;
}

// A finished rider's banner driver ($0F03/$0F07 and $0F05/$0F09, $83:EA11 and $83:EBA3): the
// banner's member, 0 until its first step, then 7..24, and its life in updates.
struct ZoomZooBannerDriver {
    std::uint16_t index{}, life{};
    bool operator==(const ZoomZooBannerDriver&) const = default;
};
// Its members and life ($83:EA19-EA5B): the index cycles 7..24; the life is set to 360 while the
// index is 0 and counted down in the same run, so a stored life is at most 359.
constexpr std::uint16_t first_banner = 7, last_banner = 24, banner_life_updates = 360;
struct ZoomZooRaceState {
    ZoomZooCamera camera;
    // $041F/$0423, $04FB/$04FF, $0555: the second rider's camera only while
    // $0DE1 selects two 112-line viewports (R-0069). The one-player state format
    // does not carry these unused words.
    ZoomZooCamera second_camera;
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
    // Followed only in a VS race, where the end of one rider's banner finishes the other; the
    // picture keeps its own copy in every race (`ClassicWindowPointer`).
    std::array<ZoomZooBannerDriver, 2> banners{};
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
// Rider 1's tutorial hints in a two-human race or the split demo (R-0082): `$12E5` is set at the
// race's setup when rider 1's own bit is clear in `$77:1116` ($82:D930-D96F); every 300 updates
// `$83:CE43-CEAF` queues its next group of four into rider 1's queue (`$12ED` counts from 0,
// `$12F1` is the group); its first scoring event ends them ($81:C5D5-C5E1).
struct ZoomZooOpponentHints {
    bool active{};
    std::uint16_t updates{}, group{};
    bool operator==(const ZoomZooOpponentHints&) const = default;
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
    // $83:F6B4-F6F3: the menu sits in the lower view (VRAM 0x1A68) because pad 2's Start opened
    // it without pad 1's. Two-pad races only, and only while the menu is open.
    bool lower_view{};
};
// $83:E254-E55B: the idle demo's two computer riders. The four per-rider words
// are $1377/$1379, $137B/$137D, $137F/$1381 and $1383/$1385; elapsed is $1387.
struct DemoControllers {
    std::array<std::uint16_t, 2> trick_bits{}, rotation_window{}, turnaround{}, airborne_rotation{};
    std::uint16_t elapsed{};
    bool exit_requested{}; // $12B3
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
    // (DRAGSTER); 2, the stunt event, sets `stunt_event` instead.
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
    // `$77:10B1` once this race's setup has counted it ($83:CA08-CA18, modulo 6): every race's
    // sound load counts, an aborted one's too, and a cold start's first race has 1. Its even half
    // picks the finish poses' pair of tables (R-0084).
    std::uint8_t race_counter{1};
    // `$12E3` at the start ($82:D94C-D96F): the player's tutorial hints run unless its rider's
    // bit is set in the cartridge RAM's `$77:1116`, which a race sets once its hints end.
    bool tutorial_hints{true};
    // `$12E5` at the start: rider 1's, by its own bit, in a two-human race (R-0082).
    bool opponent_tutorial_hints{};
    // Race mode 2, a stunt event (`$83:99AD-99B8`: place 2 of every tour; R-0066): a solo run
    // against the header's clock for points, the opponent switched off (StuntEvent).
    bool stunt_event{};
    // The rider's best medal on the tour, `$77:069C` & 3 (0 none, 1 bronze, 2 silver, 3 gold):
    // it picks a stunt event's qualifying score ($83:9EEB). A race ignores it.
    std::uint8_t best_medal{};
    // `$12D1`, NEON: track 42 in one-player play (R-0068). Its picture is its own scenery (14)
    // lit by the BG1 palette under the player; its race routines otherwise run as a stunt
    // event's (the presentation reads `player_contact_palette`).
    bool neon_lighting{};
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
// The local modes pair two distinct human riders; unlike the one-player factory, this
// leaves the opponent's AI tier at zero (R-0071, DRAGSTER capture).
ClassicRaceScenario classic_local_race_scenario(ClassicRaceTrack track, RacePairing pairing,
                                                bool tutorial_hints = true,
                                                bool opponent_tutorial_hints = false);
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
    // `$0FF7`, set only by the narrowest playfield (16 columns of 1,024 rows, 65,536 units
    // tall): the sampler takes every y as a row, a negative one included (see sample_track).
    bool whole_height{};
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
// R-0066: a stunt event's own words. The tallies are `$77:076B-07BA`, a row of four columns
// (x1-x4: one to four of a kind) per family of tricks; each column counts the tricks of that
// family and count shown, and the points they paid. `$81:C0FF-C116` counts a trick before it
// reads the trick's weight, so a trick whose weight has not grown (a short tabletop) counts
// with no points.
struct TrickTally {
    std::uint8_t shown{};   // one byte; the column's second byte stays 0
    std::uint16_t points{}; // the weights paid, added as $77:07BB is
    bool operator==(const TrickTally&) const = default;
};
namespace trick_family {
// The families by row: roll (events 1-4), flip (5-8), twist (9-12), z flip (18-21) and "mega"
// (16 head bounce in x1, 17 tabletop in x2).
inline constexpr unsigned roll = 0, flip = 1, twist = 2, z_flip = 3, mega = 4, count = 5;
inline constexpr unsigned columns = 4;
} // namespace trick_family
struct StuntEvent {
    // $77:0753: the score to reach, set at the race's setup ($80:99ED) from the tour and the
    // rider's best medal on it (`front-end.qualifying-scores`).
    std::uint16_t qualifying_score{};
    // $0BF5: 1 once the clock, counting down from the track header's time, has run out
    // ($81:C830-C867); from then on each update tests whether each rider can finish.
    std::uint16_t clock_stopped{};
    // $0FE9: 1 once both riders have finished and stand on the ground and both announcement
    // queues are empty ($83:E898-E8DD); only then do the finish poses and captions run and the
    // finish display count towards the result.
    std::uint16_t finish_display{};
    // $12DF/$12E1: 1 once a finished rider has stood on the ground ($83:E85C-E867,
    // $83:E87A-E885). From then on its vertical velocity is held at 128 each update and its
    // controls are released ($82:AA7D-AAA1, $82:AB62-AB67).
    std::array<std::uint16_t, 2> settled{};
    std::array<std::array<TrickTally, trick_family::columns>, trick_family::count> tallies{};
    bool operator==(const StuntEvent&) const = default;
};
// League bonuses read twenty wrapping byte counts and two wipeout words; R-0073.
struct LeagueRaceStatistics {
    bool enabled{};
    std::array<std::array<std::uint8_t, 20>, 2> tricks{};
    std::array<std::uint16_t, 2> wipeouts{};
    std::array<std::uint16_t, 20> opponent_points{};
    bool operator==(const LeagueRaceStatistics&) const = default;
};
struct ZoomZooState {
    ClassicRaceTrack track{ClassicRaceTrack::ZoomZoo}; // Serialized as the state magic.
    bool split_screen{}; // $0DE1; separate native demo/two-player state format pending.
    bool demo_ai{};      // $7E:212C; controls both riders in a split demo.
    bool versus{};       // $77:0750 bit 2: a VS race from the menus (R-0081).
    DemoControllers demo;
    ZoomZooPause pause;
    std::array<ZoomZooRoll, 2> rolls{};
    std::array<std::array<std::uint8_t, 25>, 2>
        learned_weights{}; // Events2–26; event1 remains in each queue.
    bool native_initialization{};
    ZoomZooResult result;
    ZoomZooPlayerAnnouncements player_announcements;
    ZoomZooOpponentHints opponent_hints;
    // The riders' look (R-0036): `$83:CDA6` runs it last in each race update, and the end of a
    // scripted glance clears the rider's idle latch (`$82:857F`/`$82:87AD`, R-0083).
    RiderLookState look;
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
    // Only in a stunt event, whose state (URTRnn07) appends it; zero in a race.
    StuntEvent stunt;
    LeagueRaceStatistics league_statistics;
    // Not serialized: the menus' riders and the opponent's tier set from them at the race's
    // setup. A deserialized race is MIKE's against the track's usual opponent.
    RacePairing pairing{};
    OpponentTier opponent_tier{};
    // `$77:10B1` (ClassicRaceScenario::race_counter). Two-view states and league wrappers carry
    // it; a deserialized one-player race has a cold start's first race's 1.
    std::uint8_t race_counter{1};
    // Not serialized, and read only by the picture: `$12CF`, the lowest BG1 palette (a cell
    // word's bits 10-12) among the ten cells the player's latest contact sampled
    // ($81:8B75-8BB3), which NEON's lighting follows (R-0068). The setup leaves 0.
    std::uint8_t player_contact_palette{};
    // Not serialized: this update's sound queue work in program order (R-0076). Each update
    // starts it empty; the caller hands it to the audio side.
    AudioCueList sound_cues;
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
    // front-end.qualifying-scores (`$83:A218`), a word per tour and medal level: the stunt
    // events' qualifying scores; empty in packs before profile v17, which hold no stunt event.
    std::span<const std::uint8_t> qualifying_scores;
    // audio.announcement-voices (`$81:C441`), a byte per 8-bit announcement event: its voice
    // (R-0076); empty in packs before profile v32, whose races are silent.
    std::span<const std::uint8_t> announcement_voices;
    // presentation.rider.look-tables.v1: the riders' look (R-0036), whose scripted glance clears
    // a rider's idle latch when it ends; empty for loose content, which runs no look.
    std::span<const std::uint8_t> look_tables;
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
    // The tile's sound effect this update, 0 for none: mud's entry ($81:89A3) or the
    // corkscrew's first ejection ($81:87D9) (`race_sound`).
    std::uint8_t sound_effect{};
};
// Whether the update that left `tiles` skipped the rider's contact with the track, the
// transient `contact_skip` above, from the words it leaves: a corkscrew step sets the physics
// hold to 8 after the reset has counted it down, and a loop step other than the entry and the
// top sets the loop's cooldown to 3 and moves its step on to 2-16. For the presentation, which
// sees only the states between updates.
bool special_tiles_skipped_contact(const SpecialTileRider& tiles);
// $81:8690-86FE: the counters' part of the reset before the tile dispatch. Returns whether
// mud let the rider go, the update the original sounds its exit ($81:86A9).
bool update_special_tile_counters(SpecialTileRider& tiles, ReflectionTransition& transition,
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
// `06`; the 742-byte layout followed by the special-tile words with $0E7B and
// $0C73 (52 bytes), the last 60 checkpoint-seen flags and the HUNTER effects (916 bytes).
// A stunt event's is `07`: the same 916 bytes and its StuntEvent (90 bytes).
std::array<std::uint8_t, 8> classic_race_state_magic(ClassicRaceTrack track);
// A race: the player finished first (or with the opponent). A stunt event: the score reached
// the qualifying score, as the scoring's win test `$83:88E1` counts it.
bool classic_race_player_won(const ZoomZooState& state);
// $83:9EEB: a stunt event's qualifying score for the tour of `track` (five tracks a tour) and the
// rider's best medal on it, silver and gold alike; `table` is front-end.qualifying-scores.
std::uint16_t stunt_qualifying_score(std::span<const std::uint8_t> table, ClassicRaceTrack track,
                                     std::uint8_t best_medal);
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
void update_zoom_zoo(ZoomZooState& state, const ControllerButtons& first_port,
                     const ControllerButtons& second_port, const ZoomZooContent& content);
} // namespace unirally
