#pragma once

#include "front_end.hpp"
#include "input_timer.hpp"
#include "presentation.hpp"
#include "zoom_zoo_movement.hpp"

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

struct RiderPosePair {
  std::array<std::uint16_t, 2> pose_indices{};
  std::array<bool, 2> reflected{};
};

bool is_recovered_pose_pair(const MovementState &state);

struct LiveFrame {
  RgbFrame frame;
  bool used_pose_fallback{};
};

// Live race pictures for either track.
class LivePresentation {
public:
  // The accepted M3 DRAGSTER v1 renderer, kept for the frozen v1 contracts:
  // only the pose indices/reflection consumed by the bounded M3-02 atlas are
  // held when a pair is unrecovered. The app does not draw through it.
  LiveFrame render(const MovementState &state,
                   const PresentationPosition &position,
                   const PresentationContent &content);
  // Both tracks draw every packed pose (R-0036). Call observe_update once per
  // simulation update so the rider look overlays and the opponent's finish
  // frame (R-0040) follow the race.
  void observe_update(const ZoomZooState& previous,const ZoomZooState& updated,
                      const ClassicContentPack& pack);
  // A rider pose outside the packed tables fails closed in the renderer; the
  // live frame then holds that rider's last drawn pose and reports the frame
  // as a fallback instead of ending the session.
  LiveFrame render_race(const ZoomZooState& state,const ZoomZooState& previous_update,
                        const ClassicRacePresentationContent& content);
  RiderPosePair last_recovered_pose_pair() const { return recovered_pair_; }

private:
  RiderPosePair recovered_pair_{{0x04f9, 0x0263}, {true, true}};
  ClassicRaceHistoryTracker history_{};
  std::array<std::optional<std::uint16_t>, 2> drawn_pose_{};
};

// The app's front end: power-on to the main menu (R-0054), 1P's setup screens
// to the race (R-0055, R-0056), and after a one-run race its result and PICK
// TRACK again (R-0057). Until the other modes are native, choosing 2P, VS,
// LEAGUE, OPTIONS, reaching the demo, or a 1P race the race scenarios do not
// have (another rider than MIKE, a stunt event) shows a short notice and
// returns to the main menu as it first appeared, the records kept.
class FrontEndSession {
public:
  explicit FrontEndSession(const ClassicContentPack &pack);
  // One PAL frame from the two ports' masks (`button_mask` bits). True once a
  // race with a native scenario is chosen: `race_track()`.
  bool update(const std::array<std::uint16_t, 2> &ports);
  // The same with the pads as the SNES reads them (`$4218`, `$421A`), as the
  // laboratory's input scripts give them.
  bool update(FrontEndPads pads);
  // The front end's own frame since power-on (notices do not count).
  std::uint32_t front_end_frame() const { return state_.frame; }
  RgbFrame frame() const;
  std::uint32_t frames() const { return frames_; }
  std::uint32_t notices() const { return notices_; }
  std::uint32_t returns_to_menu() const { return returns_; }
  std::uint32_t races() const { return races_; }
  // The front end's screen now, and the rider's medal on the tour chosen last.
  unsigned screen() const { return static_cast<unsigned>(state_.screen); }
  unsigned tour_medal() const {
    return state_.records.medals[state_.tour_menu.tour * 16U + state_.rider_menu.rider];
  }
  ClassicRaceTrack race_track() const {
    return ClassicRaceTrack{state_.tour_menu.track};
  }
  // The race is over for the menus (`update_race_for_menus`): its result load
  // has begun, or its pause menu quit or restarted it. The front end takes over
  // with its times.
  void return_from_race(const ZoomZooState &race, const RaceTimes &times);

private:
  FrontEndContent content_;
  FrontEndState state_ = start_front_end();
  std::optional<FrontEndState> main_menu_;
  std::uint32_t notice_frames_{}, frames_{}, notices_{}, returns_{}, races_{};
  FrontEndMode notice_mode_{};
};

// 1P's race is native when a race scenario has it (R-0046, R-0050): MIKE
// against BRONSEN, on a track with a scenario (not a stunt event).
bool native_one_player_race(const FrontEndState &state);

// A port's mask as the SNES reads the pad (`$4218`: B in bit 15 ... R in bit
// 4).
std::uint16_t snes_pad_word(std::uint16_t mask);

} // namespace unirally::app
