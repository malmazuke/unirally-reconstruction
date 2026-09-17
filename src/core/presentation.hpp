#pragma once
#include "movement.hpp"
#include "rider_look.hpp"
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
};
struct ZoomZooState;
class ClassicContentPack;
// Lap shown for a laps_remaining value: 4,3,2,1,0 display as 0/3,1/3,2/3,3/3,3/3.
unsigned zoom_zoo_hud_lap(unsigned laps_remaining);
// Authored ZOOM ZOO HUD text. Callers pass the state from BEFORE the update
// being drawn: the original's lap glyph changes one frame after $0EFB at every
// crossing of the frozen primary timeline (1675, 3208, 4840, 6484). The rider
// objects show that same earlier update (R-0036) and the BG scroll its camera.
// Once the player has finished, the
// original replaces the lap with FINISH, drops the running clock and shows the
// finish time with WINNER or LOSER.
struct ZoomZooHud {
  std::string lap, clock, finish_time, caption;
};
ZoomZooHud zoom_zoo_hud(const ZoomZooState& previous_update);
// Presentation-only $0D45/$0D47 upper-body overlay frames. The original
// derives them from look state the serialized race does not carry (R-0036),
// so a caller without that history draws the pose frames alone.
struct ZoomZooRiderOverlays {
  std::array<std::optional<std::uint16_t>,2> pose{};
  bool operator==(const ZoomZooRiderOverlays&) const = default;
};
// Follows the rider look animation across consecutive ZOOM ZOO race updates,
// beginning at the native race initialization, and keeps the overlays of the
// update currently on screen. Reset it whenever the race state is replaced.
class ZoomZooRiderLookTracker {
public:
  void reset();
  // Call once for every simulation update, with the state before and after it.
  void observe_update(const ZoomZooState& previous,const ZoomZooState& updated,
                      const ClassicContentPack& pack);
  // Overlays for the update that produced the `previous_update` being drawn.
  const ZoomZooRiderOverlays& on_screen() const {return on_screen_;}
  const RiderLookState& look() const {return look_;}
private:
  RiderLookState look_{};
  ZoomZooRiderOverlays latest_{}, on_screen_{};
};
// previous_update is the state before the update being drawn. The original
// picture shows the HUD and both rider objects from that update (the BG scroll
// is derived from this state's prior camera); without one they are drawn from
// state itself, one update ahead.
// Throws std::invalid_argument for a rider pose outside the packed tables.
RgbFrame render_zoom_zoo(const ZoomZooState&,const ClassicContentPack&,
                         const ZoomZooState* previous_update=nullptr,
                         const ZoomZooRiderOverlays* overlays=nullptr);
RgbFrame render_dragster_headless(const PresentationSample &,
                                  const PresentationContent &);
// Presentation-only rider atlas override. Gameplay state still controls the
// scene palette, window effects, positions, camera and HUD.
RgbFrame
render_dragster_headless_with_rider_art(const PresentationSample &,
                                        const PresentationContent &,
                                        const std::array<RiderArtPose, 2> &);
} // namespace unirally
