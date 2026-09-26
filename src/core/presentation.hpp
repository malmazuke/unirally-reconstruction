#pragma once
#include "movement.hpp"
#include "rider_look.hpp"
#include "rider_object.hpp"
#include "zoom_zoo_movement.hpp"
#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>
namespace unirally {
struct PresentationSample {
    const MovementState& movement;
    std::int32_t camera_x{};
    std::int16_t bg1_scroll_x{}, bg1_scroll_y{}, bg2_scroll_x{}, bg2_scroll_y{};
};
enum class RiderFrameId : std::uint8_t {
    LeanForward,
    CoastForward,
    RollingForward,
    RollingReflected,
    FinishForward,
    FinishReflected,
    SettledForward,
    SettledReflected
};
struct RiderFrameSelection {
    RiderFrameId id;
    std::string_view logical_id;
    bool reflected;
};
struct RiderArtPose {
    std::uint16_t pose_index{};
    bool reflected{};
};
RiderFrameSelection rider_frame_for_pose(std::uint16_t pose_index, bool reflected);
// Exact $81:B3CF/$81:B3D3 decoded-track gather: $000F+X, then X += stride.
std::vector<std::uint16_t> gather_dragster_bg1(std::span<const std::uint8_t>,
                                               std::uint16_t source_x, std::uint16_t stride_bytes,
                                               std::size_t word_count);
// Rejected diagnostic interpretation retained for its bounded gather test.
std::array<std::uint16_t, 30 * 16> expand_dragster_bg1(std::span<const std::uint8_t>);
// Stateless form of the observed rolling $81:B270 map construction. Scroll
// units are pixels; each selector covers a 64 by 64 pixel metatile.
std::array<std::uint16_t, 32 * 32>
build_dragster_bg1_map(std::span<const std::uint8_t>, std::int16_t scroll_x, std::int16_t scroll_y);
// Semantic 32-by-32 result map recovered at $80:C431. The returned words are
// in row-major order and retain the original tile attributes. R-0019 freezes
// the player-loss time glyphs independently of the whole-frame visual gate.
std::array<std::uint16_t, 32 * 32>
build_dragster_result_map(const MovementState&, std::span<const std::uint8_t> result_assets);
struct RgbFrame {
    static constexpr std::size_t width = 256, height = 224;
    std::array<std::uint8_t, width * height * 3> pixels{};
};
// Exact 15-bit SNES colour operations used by the frozen mode-3 result case.
// `palette_group` is the three-bit tilemap palette field.
std::uint16_t snes_direct_colour(std::uint8_t palette_colour, std::uint8_t palette_group);
std::uint16_t snes_add_colour(std::uint16_t main_colour, std::uint16_t sub_colour, bool halve);
struct PresentationContent {
    std::span<const std::uint8_t> track, bg1_tiles, bg2_tiles, bg2_map;
    std::span<const std::uint8_t> palette, font, rider_tiles, result_assets;
    std::span<const std::uint8_t> go_window, winner_window;
    std::span<const std::uint8_t> result_base_vram, result_palette;
    std::span<const std::uint8_t> result_palette_tail;
    // Race NMI palette tables ($80:82AB). Empty for DRAGSTER v1 packs, which keep
    // the accepted pose-keyed palette; the two-track pack carries them.
    std::span<const std::uint8_t> race_palette_cycle;
    // Channel-6 window HDMA family $15:8000-$15:D7CA: 25 tables of 899 bytes
    // (898 scanline bytes then the run terminator). Empty for DRAGSTER v1 packs,
    // which keep the accepted pose-keyed GO and winner windows (R-0040).
    std::span<const std::uint8_t> window_tables;
};
class ClassicContentPack;
// Race palette cycle ($82:D382-D496): the table index a frame draws, if the
// cycle has started, and its application to colours 96-111 and 0.
std::optional<unsigned> zoom_zoo_palette_cycle_index(std::uint32_t frame);
void apply_zoom_zoo_palette_cycle(std::array<std::uint8_t, 512>& cgram,
                                  std::span<const std::uint8_t> tables, std::uint32_t frame);
// DRAGSTER runs the same cycle from frame 1334 (R-0037): racing frame n draws
// index (n-1334)&15. The routine runs for the last time on loading update 1, so
// colours 96-111 then hold that frame's index and colour 0 is black.
void apply_dragster_palette_cycle(std::array<std::uint8_t, 512>& cgram,
                                  std::span<const std::uint8_t> tables, const MovementState& state);
// The same cycle for the shared race state of either track. `setup_frame` is
// the first frame the race vblank publishes: the scenario's initialization
// frame plus 6 (1334 for DRAGSTER, 1382 for ZOOM ZOO). Result loading freezes
// the phase and blackens colour 0 as above.
void apply_classic_race_palette_cycle(std::array<std::uint8_t, 512>& cgram,
                                      std::span<const std::uint8_t> tables,
                                      const ZoomZooState& state, std::uint32_t setup_frame);
// R-0040: the original composes the countdown/GO and winner-banner shapes from
// one channel-6 window HDMA table per frame. The countdown driver $83:E59C
// selects it from $11C5, the winner driver $83:EA19 cycles indices 7-24 from
// the winning rider's finish, and the vblank setup $80:868E-$80:8699 publishes
// the selection made by the previous frame's logic. Returns the index into the
// 25-table family for the frame `state` draws, or nothing when channel 6 is
// disabled that frame. Indices 0-6 compose before the riders, 7-24 after them.
std::optional<unsigned> dragster_window_table_index(const MovementState& state);
// The 898 scanline bytes of one member of the family.
std::span<const std::uint8_t> dragster_window_table(std::span<const std::uint8_t> tables,
                                                    unsigned index);
// The same selection from the shared race state for a single restored state
// without history: the first finisher's driver runs 360 updates from its first
// odd update, then the other rider's driver, if armed, from the later of that
// stop and the update after its finish. The player's finish frame comes from
// `race.finish_delay`; the opponent's is presentation history
// (`ClassicRaceHistory`) or, once both have finished, is recovered from the
// two finish times by `classic_opponent_finish_frame`. Exact when no pause
// diverted a race update since the first finish; `ClassicWindowPointer` is.
std::optional<unsigned>
classic_window_table_index(const ZoomZooState& state, std::uint32_t setup_frame,
                           std::optional<std::uint32_t> opponent_finish_frame,
                           unsigned transition_member);
// The frame on which the opponent finished, from a state in which both riders
// have finished: `finish_centiseconds` advances two per frame plus the frame
// parity, so total[0]-total[1] = 2(fa-fb)+(fa&1)-(fb&1) has one solution.
// Nothing before the player finishes (a restored state then shows no
// opponent-won banner; the live tracker does) and nothing for a 10:00 time-out
// (the 60000 sentinel is not a finish time). Either finish order.
std::optional<std::uint32_t> classic_opponent_finish_frame(const ZoomZooState& state);
// The preceding update's race fade $0FF1 (0..30) that the NMI writes INIDISP
// from: the previous update's level when that update is earlier than `state`;
// otherwise the accepted frame formula (one per update from the scenario's
// initialization frame), because $0FF1 holds at 30 and a saturated state
// cannot say whether the preceding level was 29 or 30.
unsigned classic_race_prior_fade(const ZoomZooState& state, const ZoomZooState* previous_update,
                                 const ClassicRaceScenario& scenario);
// The legacy finish and result phases (RaceFinishState) derived from the shared
// race state, for the recovered mode-0 result screen (R-0012, R-0019).
// Presentation only, never gameplay.
RaceFinishState classic_finish_view(const ZoomZooState& race);
// Lap shown for a laps_remaining value in a `laps`-lap race: the extra initial
// count is the start-line crossing, and the last lap holds. Three laps show
// 4,3,2,1,0 as 0,1,2,3,3.
unsigned classic_hud_lap(unsigned laps_remaining, unsigned laps);
// R-0043: the text of the original's in-race HUD fields, as its own BG3
// tilemap holds them. Callers pass the state from BEFORE the update being
// drawn: the original's lap glyph changes one frame after $0EFB at every
// crossing of the frozen primary timeline (1675, 3208, 4840, 6484). The rider
// objects show that same earlier update (R-0036) and the BG scroll its camera.
// An empty field is one the original is not showing.
// The left field alone, which the redraw queue gives priority over the clock:
// `$81:EB8E-$81:EC5E` writes `finish` from column 1 once the player's laps run
// out, the lap count from column 2 on a tour race, and `race` from column 2
// otherwise. An update that changes it is an update on which the clock is not
// republished (R-0043).
struct ClassicHudField {
    std::string text;
    unsigned column{};
};
ClassicHudField classic_hud_left_field(const ZoomZooState& published,
                                       const ClassicRaceScenario& scenario);
struct ClassicHudText {
    std::string left;
    unsigned left_column{};
    // `$81:ED5C-$81:EDD9` at columns 24-29, blanked by `$81:ECCF` at the finish.
    std::string clock;
    // The two centred seven-cell fields at columns 13-19 of rows 5-6 (player)
    // and 20-21 (opponent): each rider's crossing time in `M:SS:th` after a
    // lap or the finish (`$81:EE89-$81:EF49`, `$81:F0E6-$81:F1A0`) and its
    // signed split against the first rider through the same checkpoint in
    // `+M:SS:t` (`$81:EF5E-$81:F030`, `$81:F1B5-$81:F288`; R-0044).
    std::string player_cells, opponent_cells;
};
// RACE-OFFSCREEN-ARROW: the one-player race's direction arrow. It shows while
// the player is behind, points the way the player's track marker runs, and its
// length in chevrons (0 to 3) tells how far behind. The side arrows sit in rows
// 14-15, right at columns 26-28 and left at columns 5-7; the up arrow at
// columns 15-16 of rows 4-6 and the down arrow at rows 24-26, a chevron a row.
struct ClassicRaceArrow {
    enum class Direction : std::uint8_t { Right, Left, Up, Down };
    Direction direction{};
    unsigned chevrons{};
    // The up arrow only: its rows 5-6 hold the player's centred cells instead,
    // which the NMI does not draw over and which overwrite it.
    bool middle_rows_covered{};
    bool operator==(const ClassicRaceArrow&) const = default;
};
// What the original's HUD cells are holding: the clock digits the queue last
// wrote, and whether it has reached each of the three fields the finish sets
// going. `finish`'s own cells need no flag because the left field is derived
// from the state directly and the queue writes it first.
struct ClassicHudPublished {
    std::optional<std::string> clock{};
    bool clock_blanked{};
    // What the two centred fields hold, nothing while blank.
    std::optional<std::string> player_cells{}, opponent_cells{};
    // The direction arrow the NMI last drew, nothing while none shows.
    std::optional<ClassicRaceArrow> arrow{};
    // The caption table entry (1-255) the caption cells show, 0 while blank.
    unsigned caption_event{};
    bool operator==(const ClassicHudPublished&) const = default;
};
// $81:E9AC-$81:E9DB: the arrow's length for a lead of `lead` transitions and
// the NMI counter `race_nmis` (race NMIs since the fade reached 5; its bits 3-4
// are the step `$1255`, which moves every 8 frames): one chevron under a lead
// of 10, then 2, 1, 0, 2 as the step runs 0-3 under a lead of 20, then 3, 2, 1, 0.
unsigned classic_arrow_chevrons(std::uint16_t lead, unsigned race_nmis);
// The arrow the NMI draws from the update that produced `updated` ($82:9822
// computes its word, the NMI its length): nothing unless the player's
// transition count is behind the opponent's and outside its pair (the two
// counts halved differ), the player has not finished and the player's last
// transition was not rejected. The direction is the player's track marker as it stood before the
// update's contact (`previous`).
std::optional<ClassicRaceArrow> classic_race_arrow(const ZoomZooState& previous,
                                                   const ZoomZooState& updated, unsigned race_nmis);
// R-0044: the centred fields' content. A crossing publishes the crossing
// clock digits `$0E43,y..` (native `time_digits`) as `M:SS:th`. A checkpoint
// the other rider has already passed publishes the difference between the
// clock as this update read it and the time the first rider through stored,
// digit by digit with borrows ($81:C94F-CA36): non-negative as `+M:SS:t`,
// negative through the ten's complement the original takes (`5-T`, `9-s`,
// `10-t` with ten shown as zero, the minute as its one's complement) as
// `-M:SS:t`. Clock and stored digits are minutes, tens, seconds, tenths.
// The opponent's field shows the minus glyph whatever the sign ($81:F1C0
// reads the constant $80:8220 where the player's writer reads `$11BB`).
std::string classic_hud_crossing_text(const std::array<std::uint16_t, 5>& time_digits);
std::array<std::uint8_t, 4> classic_hud_clock_digits(const RaceTimerDigits& timer);
std::string classic_hud_split_text(const std::array<std::uint8_t, 4>& clock,
                                   const std::array<std::uint8_t, 4>& stored);
// One centred field's pending request, as `$0349`/`$034B` hold it: nothing,
// a positive draw of `text`, or a negative blank.
struct ClassicHudCellRequest {
    enum class Kind { None, Draw, Blank } kind{};
    std::string text;
};
// `published` is what the original's cells are holding, followed update by
// update by `ClassicRaceHudClock`. Without it the clock digits are derived from
// `previous_update` itself and the finish fields are placed at fixed offsets
// from the finish, which is what the original does only while nothing else
// wants the queue.
ClassicHudText classic_race_hud_text(const ZoomZooState& previous_update,
                                     const ClassicRaceScenario& scenario,
                                     std::optional<std::uint32_t> opponent_finish_frame,
                                     const std::optional<ClassicHudPublished>& published = {});
// The clock cells hold what the redraw queue last wrote. The queue rewrites at
// most one field per update, the left field goes first, and an update that
// **writes** it spends that update ($81:EC5E and $81:EB98 both end at
// $81:ECBC, which clears `$0D17` and returns through $81:F357), so the clock
// digits stand for one more picture. Two paths write: a tour race whose flag
// `$0D17` is set - either rider's lap counter sets it, the opponent's without
// changing what the field shows - and the player's laps reaching zero, which
// writes `finish` on either track. A dirty flag alone is not enough: when
// `$053F` is set, $81:EB93 branches to $81:EB9B, whose JMP enters the clock
// handler, and nothing clears the flag, so a sprint holds on no crossing. Measured on
// compound-reverse 3212 (player), ordinary-controls/down-a 3207-3208 (both
// riders) and DRAGSTER's regression-landing-held-roll-a 1599, which the
// original does not hold.
// The redraw queue itself, followed update by update. Each update the original
// services the **first** pending field and returns, so a field behind a busy
// one waits: the left field ($0D17, set only by $81:818D, the lap-counter
// decrement, for either rider), then the clock ($034D, negative to blank it),
// then the player's finish time ($0349), then the opponent's. Placing the
// finish sequence at fixed offsets from the finish instead reproduces the
// queue only while nothing competes for it, and slides by an update when the
// opponent's counter steps just after the player finishes - `$0D17` is set
// again while `$0EFB` is already zero, so `finish` is written a second time
// and everything behind it waits (review 4 B1, measured on two DRAGSTER races
// at pictures 3215-3217).
// The centred fields (R-0044) are requested by `$81:C910-CB23`, which runs
// after the lap routine and before the clock ticks, on the update a rider's
// display countdown `$0FFF,y` is set to 120 (every crossing but the initial
// one, recognised here by the next-checkpoint step with a nonzero countdown)
// and, negatively, on the update it reaches 2. A lap or finish crossing
// draws the crossing digits; a checkpoint draws the split against the first
// rider through that slot of that lap, or, for the first rider, stores that
// clock in the shared slot and clears its own request ($81:CA38-CA61: it
// draws nothing, and the opponent's countdown is cut to 2 so its cells are
// blanked at once). The slot times are presentation history the serialized
// race does not carry, so a queue that did not observe the first crossing of
// a slot leaves the cells as they are. A finished rider's blank is skipped
// without spending the update ($81:EDE9, $81:F040).
// The one-player NMI ($81:E8C9) redraws the direction arrow before it services
// the queue, and only after an update whose progress phase `$0302` is set: every
// other picture keeps the arrow of the picture before. The caption is the
// queue's last task (`$0EE7`, $81:F30C), after the opponent's cells (the stunt
// event's field, which comes between, has no native scenario), so a new caption
// waits a picture behind each field still pending, usually the clock's tenth
// (RACE-OFFSCREEN-ARROW).
class ClassicRaceHudClock {
public:
    void reset() {
        latest_ = {};
        on_screen_ = {};
        pending_ = {};
        slot_times_ = {};
        race_nmis_ = 0;
        caption_buffer_ = 0;
        consumed_since_blank_ = false;
    }
    void observe_update(const ZoomZooState& previous, const ZoomZooState& updated);
    // The cells as the picture drawn from the earlier of the two states last
    // observed shows them, so this lags one update exactly as the rider overlays
    // do: picture N is drawn from the state at N-1 and shows what the queue
    // wrote on update N, which it derived from the state at N-1.
    const ClassicHudPublished& published() const { return on_screen_; }

private:
    struct Pending {
        bool left{}, clock_blank{}, caption{};
        std::array<ClassicHudCellRequest, 2> cells{};
    };
    ClassicHudPublished latest_{}, on_screen_{};
    Pending pending_{};
    // `$1255`/`$1257`: the race NMIs counted since the fade reached 5, pauses
    // included (the NMI runs through them).
    unsigned race_nmis_{};
    // `$0EA7`: the caption text the update last wrote for the NMI to upload,
    // as its caption table entry (0 blank), and `$11C1`: whether an event has
    // been consumed since the last blank was written.
    unsigned caption_buffer_{};
    bool consumed_since_blank_{};
    // The clock the first rider through each slot stored, minutes, tens,
    // seconds, tenths. Indexed like `checkpoint_seen`, laps remaining * 4 +
    // checkpoint; the original keeps four bytes a slot at `$100D` + 16 * laps
    // remaining + 4 * checkpoint: 80 slots, `$100D-$114C` (R-0048).
    std::array<std::optional<std::array<std::uint8_t, 4>>, 80> slot_times_{};
    // The two halves of observe_update: what this update asks the HUD queue for, and the
    // one field the queue services.
    void request_fields(const ZoomZooState& previous, const ZoomZooState& updated);
    ClassicHudCellRequest crossing_cell(const ZoomZooState& previous, std::size_t rider,
                                        const ZoomZooState& updated);
    void service_one_field(const ZoomZooState& updated);
    void redraw_arrow(const ZoomZooState& previous, const ZoomZooState& updated);
    void request_caption(const ZoomZooState& previous, const ZoomZooState& updated);
};
// Presentation-only $0D45/$0D47 upper-body overlay frames. The original
// derives them from look state the serialized race does not carry (R-0036),
// so a caller without that history draws the pose frames alone.
struct ZoomZooRiderOverlays {
    std::array<std::optional<std::uint16_t>, 2> pose{};
    bool operator==(const ZoomZooRiderOverlays&) const = default;
};
// Presentation history the serialized race does not carry: the rider look
// overlays of the update on screen, the frame on which the opponent finished
// (the winner banner's origin when the opponent won, R-0040), and the
// channel-6 window member the race vblank has published for the picture on
// screen, followed update by update as the original's drivers choose it
// (`window_published` is false for a caller without that history).
struct ClassicRaceHistory {
    ZoomZooRiderOverlays overlays{};
    std::optional<std::uint32_t> opponent_finish_frame{};
    bool window_published{};
    std::optional<unsigned> window_table{};
    ClassicHudPublished published_hud{};
    // R-0052: HUNTER effect 0 as the BG scroll routine of the update on screen
    // read it ($81:AE90, before that update's own effect routine ran).
    bool hunter_barf{};
    // R-0052: HUNTER effect 3's blink at the end of the update before the one on
    // screen. The riders' OAM vertical-flip bit outlives the blink by an update:
    // the NMI resets the attributes ($80:876C) only after its OAM transfer.
    bool hunter_flip_prior{};
};
// The countdown's transition member for a track: 5 + `$1229`, which
// $83:CC05-CC08 latches at race initialization from the player's reflection
// word `$0BA7`, itself set from the track header (`classic_race_start`). It
// stays fixed for the race however the player turns. Member 6 (DRAGSTER,
// reflected start) and member 5 (ZOOM ZOO) are different shapes, not mirrors.
unsigned classic_window_transition_member(std::span<const std::uint8_t> decoded_track);
// The countdown driver's selection on one race update ($83:E59C, R-0040),
// from the countdown word `$11C5` as the update read it (before its own
// decrement), the update's `$0300` parity, which is the parity of the frames
// since the initialization boundary (the frame parity on the even boundaries of
// DRAGSTER and ZOOM ZOO; the clock ticks through a pause, the drivers do not
// run through one), and the
// track's transition member above. Nothing once the word is zero.
std::optional<unsigned> classic_countdown_window(std::uint16_t countdown_before, bool parity_set,
                                                 unsigned transition_member);
// The channel-6 window pointer `$11FD` as the original keeps it (R-0040,
// DRAGSTER-WINDOW-PAUSE), followed update by update from the shared race
// state alone. Each rider's finish arms that rider's banner driver
// ($0F03/$0F07 and $0F05/$0F09); a driver first runs on the next race update
// the pause menu does not divert, and the earliest-armed live driver owns the
// pointer. Per run: with its index still zero the life is set to 360; a life
// of zero stops the driver for good (the next armed one runs in the same
// update); otherwise the life counts down and, on an odd frame, the index
// steps 8..24 then 7..24; the request is member 7 for index zero. Without a
// banner the countdown driver selects. A diverted update publishes nothing.
class ClassicWindowPointer {
public:
    // Reset it whenever the race state is replaced (a restart); one instance
    // follows one race from its initialization.
    void reset();
    // Call once for every simulation update, with the state before and after it
    // and the track's countdown transition member.
    void observe_update(const ZoomZooState& previous, const ZoomZooState& updated,
                        unsigned transition_member);
    // The member the vblank published for the picture of the latest observed
    // state: nothing while the channel is disabled.
    std::optional<unsigned> published() const { return published_; }
    bool observed() const { return observed_; }

private:
    struct Driver {
        bool dead{};
        unsigned index{}, life{};
    };
    std::array<Driver, 2> drivers_{};
    std::array<std::size_t, 2> order_{}, pending_{};
    std::size_t ordered_{}, pending_count_{};
    std::optional<unsigned> chosen_{}, published_{};
    bool observed_{};
};
// Follows the rider look animation across consecutive race updates of either
// track, beginning at the native race initialization, and keeps the history of
// the update currently on screen. Reset it whenever the race state is replaced.
class ClassicRaceHistoryTracker {
public:
    void reset();
    // Call once for every simulation update, with the state before and after it.
    void observe_update(const ZoomZooState& previous, const ZoomZooState& updated,
                        const ClassicContentPack& pack);
    // History for the update that produced the `previous_update` being drawn.
    ClassicRaceHistory on_screen() const {
        return {on_screen_,           opponent_finish_frame_, window_.observed(),
                window_.published(),  clock_.published(),     on_screen_barf_,
                on_screen_flip_prior_};
    }
    const RiderLookState& look() const { return look_; }

private:
    RiderLookState look_{};
    ZoomZooRiderOverlays latest_{}, on_screen_{};
    bool latest_barf_{}, on_screen_barf_{}, latest_flip_prior_{}, on_screen_flip_prior_{};
    std::optional<std::uint32_t> opponent_finish_frame_{};
    ClassicWindowPointer window_{};
    ClassicRaceHudClock clock_{};
    // The track's countdown transition member, a constant of the race read
    // from the pack on the first update after a reset.
    std::optional<unsigned> transition_member_{};
};
// The content one track's race is drawn from, selected by track from the pack.
// Every span is pack content; the scenario and geometry come from the engine.
struct ClassicRacePresentationContent {
    ClassicRaceScenario scenario{};
    TrackGeometry geometry{};
    std::string track_name; // Authored result screen only.
    std::span<const std::uint8_t> track, bg1_tiles, bg2_tiles, bg2_map, palette;
    // Race NMI palette tables ($80:82AB), one ROM table for both tracks.
    std::span<const std::uint8_t> race_palette_cycle;
    // Channel-6 window HDMA family (R-0040), one ROM family for both tracks:
    // the same drivers select the countdown digits, GO and the winner banner
    // on ZOOM ZOO as on DRAGSTER (ZOOM-ZOO-WINDOW-EFFECTS). Empty only when the
    // pack does not carry the family; the windows are then omitted.
    std::span<const std::uint8_t> window_tables;
    // R-0042: the caption table, sixteen ASCII bytes per reward event, entries 1
    // to 255 of `$17:C9F4`. The v9 profile requires it, so this span is never
    // empty in a validated pack; the renderer's emptiness check is a span
    // contract for a profile that ever makes it optional.
    std::span<const std::uint8_t> captions;
    // The 2bpp 128-tile sheet the captions are drawn with, already in the pack.
    std::span<const std::uint8_t> caption_font;
    // The countdown's transition member, 5 + `$1229`, which race initialization
    // latches from the player's start reflection ($83:CC05-CC08): 6 on
    // DRAGSTER, 5 on ZOOM ZOO. Derived from the track header like the engine's
    // start state (`classic_window_transition_member`).
    unsigned window_transition_member{};
    RiderObjectContent riders;
    // Recovered mode-0 result screen (R-0012, R-0019). Empty spans for a track
    // whose result is the authored tour screen.
    std::span<const std::uint8_t> result_assets, result_base_vram, result_palette,
        result_palette_tail;
    // The result title's name bytes for a one-run track beyond DRAGSTER (its
    // name-table entry with the 0xFF); empty for the two accepted tracks.
    std::span<const std::uint8_t> result_track_name;
    // The riders' sprite palettes, asset 6 + character: OBJ palette 3 the player's
    // ($82:DD90-DD9A) and 4 the opponent's ($82:DDB0-DDBC; R-0052, R-0061).
    std::span<const std::uint8_t> rider_palette, opponent_palette;
    // The player's four bytes of `$82:D4DC`: COLDATA's writes and CGADSUB, the colour math
    // HDMA channel 5 applies to the BG3 ink ($82:D57F-D5FB, R-0061).
    std::span<const std::uint8_t> rider_colour_math;
};
// $82:D57F-D5FB: HDMA channel 5 writes the rider's COLDATA bytes (bits 5-7 choose red, green
// and blue, bits 0-4 the intensity) and CGADSUB (bit 7 subtracts). With CGWSEL 0x02 and BG3
// enabled the race's ink is CGRAM 27 (`ink`) plus, or minus, that fixed colour, clamped per
// channel (R-0061): MIKE's (13,0,0) + (15,0,0), TONY's black.
std::uint16_t classic_race_ink(std::uint16_t ink, std::span<const std::uint8_t> colour_math);
// The content of MIKE's race against the track's usual opponent.
ClassicRacePresentationContent classic_race_presentation_content(const ClassicContentPack& pack,
                                                                 ClassicRaceTrack track);
// The content of the race `scenario` sets up (its track and pairing).
ClassicRacePresentationContent
classic_race_presentation_content(const ClassicContentPack& pack,
                                  const ClassicRaceScenario& scenario);
// One renderer for both tracks. previous_update is the state before the update
// being drawn. The original picture shows the HUD and both rider objects from
// that update (the BG scroll is derived from this state's prior camera);
// without one they are drawn from state itself, one update ahead.
// Throws std::invalid_argument for a rider pose outside the packed tables.
// R-0042: the top tile of a caption glyph, or nothing for a space. Every byte
// of the caption table is a space, `!`, `"`, `-` or a lowercase letter; any
// other byte is outside the recovered domain and throws. R-0043 adds the HUD's
// own characters, which come from the same sheet: the digits at 0x01-0x0a,
// `:` at `$45` and `/` at `$4e`, as the original's character table `$80:81F4`
// indexes them.
std::optional<unsigned> classic_caption_tile(char glyph);

// R-0042: the sixteen bytes the caption shows for a published state, or nothing
// when the queue has blanked the display ($81:BEA8-BEF1's `empty_display`) or
// has published no event yet. Without the HUD queue's history this is the
// state's own text, which the original shows a picture late whenever the queue
// was busy (`ClassicRaceHudClock`).
std::optional<std::span<const std::uint8_t>>
classic_caption_entry(const ZoomZooState& published, std::span<const std::uint8_t> captions);
// The sixteen bytes of caption table entry `event`, or nothing for 0 (blank)
// or a table of the wrong size.
std::optional<std::span<const std::uint8_t>>
classic_caption_text(unsigned event, std::span<const std::uint8_t> captions);

RgbFrame render_classic_race(const ZoomZooState& state,
                             const ClassicRacePresentationContent& content,
                             const ZoomZooState* previous_update = nullptr,
                             const ClassicRaceHistory* history = nullptr);
// Authored standalone pause menu over a race picture (M4-16): the picture is
// halved, then RESUME / RESTART RACE is drawn with the selection marker.
void draw_race_pause_menu(RgbFrame& frame, std::uint16_t selection,
                          std::array<std::uint8_t, 3> panel, std::array<std::uint8_t, 3> ink);
// The accepted DRAGSTER v1 presentation entries (M3-02, M4-01); the race
// palette cycle and window family are optional (DRAGSTER v1 packs keep the
// accepted pose-keyed colours and windows). Only the frozen v1 contracts and
// their runners draw through this content; live play uses the renderer above.
PresentationContent dragster_presentation_content(const ClassicContentPack& pack);
RgbFrame render_dragster_headless(const PresentationSample&, const PresentationContent&);
// Presentation-only rider atlas override. Gameplay state still controls the
// scene palette, window effects, positions, camera and HUD.
RgbFrame render_dragster_headless_with_rider_art(const PresentationSample&,
                                                 const PresentationContent&,
                                                 const std::array<RiderArtPose, 2>&);
} // namespace unirally
