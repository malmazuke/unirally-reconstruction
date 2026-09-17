#pragma once

#include "input_timer.hpp"
#include "presentation.hpp"

#include <array>
#include <cstdint>
#include <optional>

namespace unirally::app {

inline constexpr std::uint64_t pal_update_ns = 20'000'000;
inline constexpr std::uint32_t maximum_catch_up_updates = 4;

class PalScheduler {
public:
  explicit PalScheduler(std::uint64_t now_ns);
  std::uint32_t updates_due(std::uint64_t now_ns);
  void pause(std::uint64_t now_ns);
  void resume(std::uint64_t now_ns);
  bool paused() const { return paused_; }
  std::uint64_t next_deadline_ns() const { return next_deadline_ns_; }

private:
  std::uint64_t next_deadline_ns_{};
  bool paused_{};
};

enum class LogicalButton : std::uint8_t {
  B, Y, Select, Start, Up, Down, Left, Right, A, X, LeftShoulder,
  RightShoulder
};

enum class KeyboardKey : std::uint8_t {
  Z, X, Backspace, Return, Up, Down, Left, Right, A, S, Q, W
};

enum class GamepadButton : std::uint8_t {
  South, West, Back, Start, DpadUp, DpadDown, DpadLeft, DpadRight,
  East, North, LeftShoulder, RightShoulder
};

std::uint16_t button_mask(LogicalButton button);
LogicalButton keyboard_mapping(KeyboardKey key);
LogicalButton gamepad_mapping(GamepadButton button);
ControllerButtons controller_buttons(std::uint16_t mask);

class InputState {
public:
  void keyboard(KeyboardKey key, bool pressed);
  void gamepad(std::uint8_t port, GamepadButton button, bool pressed);
  void disconnect(std::uint8_t port);
  void clear();
  std::array<std::uint16_t, 2> snapshot() const;
  std::uint16_t keyboard_mask() const { return keyboard_mask_; }
  std::uint16_t gamepad_mask(std::uint8_t port) const { return gamepad_masks_.at(port); }

private:
  std::uint16_t keyboard_mask_{};
  std::array<std::uint16_t, 2> gamepad_masks_{};
};

struct Viewport { int x{}, y{}, width{}, height{}, scale{}; };
Viewport integer_viewport(int output_width, int output_height);

struct PresentationPosition {
  std::int32_t camera_x{};
  std::int16_t bg1_x{}, bg1_y{}, bg2_x{}, bg2_y{};
};
PresentationPosition presentation_position(std::uint16_t player_x);

struct RiderPosePair {
  std::array<std::uint16_t, 2> pose_indices{};
  std::array<bool, 2> reflected{};
};

bool is_recovered_pose_pair(const MovementState &state);

// The accepted DRAGSTER presentation entries; the race palette cycle is
// optional (DRAGSTER v1 packs keep the accepted colours).
PresentationContent dragster_presentation_content(const ClassicContentPack &pack);

// DRAGSTER plays on the shared race engine (R-0038) but is drawn by the
// accepted M3 presentation, which reads the legacy finish and result phases.
// Derive those from the shared race state: presentation only, never gameplay.
MovementState dragster_presentation_state(const ZoomZooState &race);
// Picture brightness 0-15 from the preceding update's race fade ($80:883F).
unsigned race_picture_brightness(const ZoomZooState &race);

struct LiveFrame {
  RgbFrame frame;
  bool used_pose_fallback{};
};

// Render the current gameplay/camera/HUD. Only the pose indices/reflection
// consumed by the bounded M3-02 atlas are held when a pair is unrecovered.
class LivePresentation {
public:
  LiveFrame render(const MovementState &state,
                   const PresentationPosition &position,
                   const PresentationContent &content);
  // DRAGSTER on the shared race engine: the accepted scene, faded in from
  // black and overlaid by the pause menu while the race is paused.
  LiveFrame render_dragster_race(const ZoomZooState &race,
                                 const PresentationPosition &position,
                                 const PresentationContent &content);
  // ZOOM ZOO draws every packed pose (R-0036). Call observe_zoom_update once
  // per simulation update so the rider look overlays follow the race.
  void observe_zoom_update(const ZoomZooState& previous,const ZoomZooState& updated,
                           const ClassicContentPack& pack);
  // A rider pose outside the packed tables fails closed in the renderer; the
  // live frame then holds that rider's last drawn pose and reports the frame
  // as a fallback instead of ending the session.
  LiveFrame render_zoom(const ZoomZooState& state,const ZoomZooState& previous_update,
                        const ClassicContentPack& pack);
  RiderPosePair last_recovered_pose_pair() const { return recovered_pair_; }

private:
  RiderPosePair recovered_pair_{{0x04f9, 0x0263}, {true, true}};
  ZoomZooRiderLookTracker zoom_look_{};
  std::array<std::optional<std::uint16_t>, 2> zoom_drawn_pose_{};
};

} // namespace unirally::app
