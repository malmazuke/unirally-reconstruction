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
  const MovementState &movement;
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
RiderFrameSelection rider_frame_for_pose(std::uint16_t pose_index,
                                         bool reflected);
// Exact $81:B3CF/$81:B3D3 decoded-track gather: $000F+X, then X += stride.
std::vector<std::uint16_t> gather_dragster_bg1(std::span<const std::uint8_t>,
                                               std::uint16_t source_x,
                                               std::uint16_t stride_bytes,
                                               std::size_t word_count);
// Rejected diagnostic interpretation retained for its bounded gather test.
std::array<std::uint16_t, 30 * 16>
    expand_dragster_bg1(std::span<const std::uint8_t>);
// Stateless form of the observed rolling $81:B270 map construction. Scroll
// units are pixels; each selector covers a 64 by 64 pixel metatile.
std::array<std::uint16_t, 32 * 32>
build_dragster_bg1_map(std::span<const std::uint8_t>, std::int16_t scroll_x,
                       std::int16_t scroll_y);
// Semantic 32-by-32 result map recovered at $80:C431. The returned words are
// in row-major order and retain the original tile attributes. R-0019 freezes
// the player-loss time glyphs independently of the whole-frame visual gate.
std::array<std::uint16_t, 32 * 32>
build_dragster_result_map(const MovementState &,
                          std::span<const std::uint8_t> result_assets);
struct RgbFrame {
  static constexpr std::size_t width = 256, height = 224;
  std::array<std::uint8_t, width * height * 3> pixels{};
};
// Exact 15-bit SNES colour operations used by the frozen mode-3 result case.
// `palette_group` is the three-bit tilemap palette field.
std::uint16_t snes_direct_colour(std::uint8_t palette_colour,
                                 std::uint8_t palette_group);
std::uint16_t snes_add_colour(std::uint16_t main_colour,
                              std::uint16_t sub_colour, bool halve);
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
void apply_zoom_zoo_palette_cycle(std::array<std::uint8_t,512>& cgram,std::span<const std::uint8_t> tables,
                                  std::uint32_t frame);
// DRAGSTER runs the same cycle from frame 1334 (R-0037): racing frame n draws
// index (n-1334)&15. The routine runs for the last time on loading update 1, so
// colours 96-111 then hold that frame's index and colour 0 is black.
void apply_dragster_palette_cycle(std::array<std::uint8_t,512>& cgram,std::span<const std::uint8_t> tables,
                                  const MovementState& state);
// The same cycle for the shared race state of either track. `setup_frame` is
// the first frame the race vblank publishes: the scenario's initialization
// frame plus 6 (1334 for DRAGSTER, 1382 for ZOOM ZOO). Result loading freezes
// the phase and blackens colour 0 as above.
void apply_classic_race_palette_cycle(std::array<std::uint8_t,512>& cgram,std::span<const std::uint8_t> tables,
                                      const ZoomZooState& state,std::uint32_t setup_frame);
// R-0040: the original composes the countdown/GO and winner-banner shapes from
// one channel-6 window HDMA table per frame. The countdown driver $83:E59C
// selects it from $11C5, the winner driver $83:EA19 cycles indices 7-24 from
// the winning rider's finish, and the vblank setup $80:868E-$80:8699 publishes
// the selection made by the previous frame's logic. Returns the index into the
// 25-table family for the frame `state` draws, or nothing when channel 6 is
// disabled that frame. Indices 0-6 compose before the riders, 7-24 after them.
std::optional<unsigned> dragster_window_table_index(const MovementState& state);
// The 898 scanline bytes of one member of the family.
std::span<const std::uint8_t> dragster_window_table(
    std::span<const std::uint8_t> tables, unsigned index);
// The same selection from the shared race state. The winner driver counts from
// the player's finish through `race.finish_delay`; when the opponent won, its
// finish frame is presentation history (`ClassicRaceHistory`) or, once the
// player has also finished, is recovered from the two finish times by
// `classic_opponent_finish_frame`.
std::optional<unsigned> classic_window_table_index(const ZoomZooState& state,std::uint32_t setup_frame,
                                                   std::optional<std::uint32_t> opponent_finish_frame);
// The frame on which the opponent finished, from a state in which both riders
// have finished: `finish_centiseconds` advances two per frame plus the frame
// parity, so total[0]-total[1] = 2(fa-fb)+(fa&1)-(fb&1) has one solution.
// Nothing before the player finishes (a restored state then shows no
// opponent-won banner; the live tracker does) and nothing for a 10:00 time-out
// (the 60000 sentinel is not a finish time).
std::optional<std::uint32_t> classic_opponent_finish_frame(const ZoomZooState& state);
// The legacy finish and result phases (RaceFinishState) derived from the shared
// race state, for the recovered mode-0 result screen (R-0012, R-0019).
// Presentation only, never gameplay.
RaceFinishState classic_finish_view(const ZoomZooState& race);
// Lap shown for a laps_remaining value in a `laps`-lap race: the extra initial
// count is the start-line crossing, and the last lap holds. Three laps show
// 4,3,2,1,0 as 0,1,2,3,3.
unsigned classic_hud_lap(unsigned laps_remaining,unsigned laps);
// Authored HUD text. Callers pass the state from BEFORE the update being
// drawn: the original's lap glyph changes one frame after $0EFB at every
// crossing of the frozen primary timeline (1675, 3208, 4840, 6484). The rider
// objects show that same earlier update (R-0036) and the BG scroll its camera.
// Once the player has finished, the original replaces the lap with FINISH,
// drops the running clock and shows the finish time with WINNER or LOSER.
struct ClassicRaceHud {
  std::string lap, clock, finish_time, caption;
  bool countdown_caption{}; // READY/GO: authored stand-ins for the countdown windows.
};
ClassicRaceHud classic_race_hud(const ZoomZooState& previous_update);
// Presentation-only $0D45/$0D47 upper-body overlay frames. The original
// derives them from look state the serialized race does not carry (R-0036),
// so a caller without that history draws the pose frames alone.
struct ZoomZooRiderOverlays {
  std::array<std::optional<std::uint16_t>,2> pose{};
  bool operator==(const ZoomZooRiderOverlays&) const = default;
};
// Presentation history the serialized race does not carry: the rider look
// overlays of the update on screen, and the frame on which the opponent
// finished (the winner banner's origin when the opponent won, R-0040).
struct ClassicRaceHistory {
  ZoomZooRiderOverlays overlays{};
  std::optional<std::uint32_t> opponent_finish_frame{};
};
// Follows the rider look animation across consecutive race updates of either
// track, beginning at the native race initialization, and keeps the history of
// the update currently on screen. Reset it whenever the race state is replaced.
class ClassicRaceHistoryTracker {
public:
  void reset();
  // Call once for every simulation update, with the state before and after it.
  void observe_update(const ZoomZooState& previous,const ZoomZooState& updated,
                      const ClassicContentPack& pack);
  // History for the update that produced the `previous_update` being drawn.
  ClassicRaceHistory on_screen() const {return {on_screen_,opponent_finish_frame_};}
  const RiderLookState& look() const {return look_;}
private:
  RiderLookState look_{};
  ZoomZooRiderOverlays latest_{}, on_screen_{};
  std::optional<std::uint32_t> opponent_finish_frame_{};
};
// The content one track's race is drawn from, selected by track from the pack.
// Every span is pack content; the scenario and geometry come from the engine.
struct ClassicRacePresentationContent {
  ClassicRaceScenario scenario{};
  TrackGeometry geometry{};
  std::string_view track_name; // Authored result screen only.
  std::span<const std::uint8_t> track, bg1_tiles, bg2_tiles, bg2_map, palette;
  // Race NMI palette tables ($80:82AB), one ROM table for both tracks.
  std::span<const std::uint8_t> race_palette_cycle;
  // Channel-6 window HDMA family (R-0040). Empty when the track's per-frame
  // selection is not recovered or the pack does not carry the family; the
  // countdown and winner windows are then omitted.
  std::span<const std::uint8_t> window_tables;
  RiderObjectContent riders;
  // Recovered mode-0 result screen (R-0012, R-0019). Empty spans for a track
  // whose result is the authored tour screen.
  std::span<const std::uint8_t> result_assets, result_base_vram, result_palette, result_palette_tail;
};
ClassicRacePresentationContent classic_race_presentation_content(const ClassicContentPack& pack,ClassicRaceTrack track);
// One renderer for both tracks. previous_update is the state before the update
// being drawn. The original picture shows the HUD and both rider objects from
// that update (the BG scroll is derived from this state's prior camera);
// without one they are drawn from state itself, one update ahead.
// Throws std::invalid_argument for a rider pose outside the packed tables.
RgbFrame render_classic_race(const ZoomZooState& state,const ClassicRacePresentationContent& content,
                             const ZoomZooState* previous_update=nullptr,
                             const ClassicRaceHistory* history=nullptr);
// Authored standalone pause menu over a race picture (M4-16): the picture is
// halved, then RESUME / RESTART RACE is drawn with the selection marker.
void draw_race_pause_menu(RgbFrame& frame,std::uint16_t selection,std::array<std::uint8_t,3> panel,
                          std::array<std::uint8_t,3> ink);
// The accepted DRAGSTER v1 presentation entries (M3-02, M4-01); the race
// palette cycle and window family are optional (DRAGSTER v1 packs keep the
// accepted pose-keyed colours and windows). Only the frozen v1 contracts and
// their runners draw through this content; live play uses the renderer above.
PresentationContent dragster_presentation_content(const ClassicContentPack& pack);
RgbFrame render_dragster_headless(const PresentationSample &,
                                  const PresentationContent &);
// Presentation-only rider atlas override. Gameplay state still controls the
// scene palette, window effects, positions, camera and HUD.
RgbFrame
render_dragster_headless_with_rider_art(const PresentationSample &,
                                        const PresentationContent &,
                                        const std::array<RiderArtPose, 2> &);
} // namespace unirally
