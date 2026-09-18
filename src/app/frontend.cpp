#include "frontend.hpp"
#include "content_pack.hpp"
#include "rider_object.hpp"
#include "zoom_zoo_movement.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace unirally::app {

PalScheduler::PalScheduler(std::uint64_t now_ns)
    : next_deadline_ns_(now_ns + pal_update_ns) {}

std::uint32_t PalScheduler::updates_due(std::uint64_t now_ns) {
  if (paused_ || now_ns < next_deadline_ns_)
    return 0;
  const std::uint64_t due = (now_ns - next_deadline_ns_) / pal_update_ns + 1;
  if (due > maximum_catch_up_updates) {
    next_deadline_ns_ = now_ns + pal_update_ns;
    return maximum_catch_up_updates;
  }
  next_deadline_ns_ += due * pal_update_ns;
  return static_cast<std::uint32_t>(due);
}

void PalScheduler::pause(std::uint64_t now_ns) {
  (void)now_ns;
  paused_ = true;
}

void PalScheduler::resume(std::uint64_t now_ns) {
  paused_ = false;
  next_deadline_ns_ = now_ns + pal_update_ns;
}

std::uint16_t button_mask(LogicalButton button) {
  return static_cast<std::uint16_t>(1U << static_cast<unsigned>(button));
}

LogicalButton keyboard_mapping(KeyboardKey key) {
  return static_cast<LogicalButton>(key);
}

LogicalButton gamepad_mapping(GamepadButton button) {
  return static_cast<LogicalButton>(button);
}

ControllerButtons controller_buttons(std::uint16_t mask) {
  ControllerButtons buttons{};
  buttons.b = (mask & button_mask(LogicalButton::B)) != 0;
  buttons.y = (mask & button_mask(LogicalButton::Y)) != 0;
  buttons.select = (mask & button_mask(LogicalButton::Select)) != 0;
  buttons.start = (mask & button_mask(LogicalButton::Start)) != 0;
  buttons.up = (mask & button_mask(LogicalButton::Up)) != 0;
  buttons.down = (mask & button_mask(LogicalButton::Down)) != 0;
  buttons.left = (mask & button_mask(LogicalButton::Left)) != 0;
  buttons.right = (mask & button_mask(LogicalButton::Right)) != 0;
  buttons.a = (mask & button_mask(LogicalButton::A)) != 0;
  buttons.x = (mask & button_mask(LogicalButton::X)) != 0;
  buttons.left_shoulder =
      (mask & button_mask(LogicalButton::LeftShoulder)) != 0;
  buttons.right_shoulder =
      (mask & button_mask(LogicalButton::RightShoulder)) != 0;
  return buttons;
}

void InputState::keyboard(KeyboardKey key, bool pressed) {
  const auto mask = button_mask(keyboard_mapping(key));
  keyboard_mask_ = pressed ? static_cast<std::uint16_t>(keyboard_mask_ | mask)
                           : static_cast<std::uint16_t>(keyboard_mask_ & ~mask);
}

void InputState::gamepad(std::uint8_t port, GamepadButton button,
                         bool pressed) {
  if (port >= gamepad_masks_.size())
    throw std::out_of_range("gamepad port must be 0 or 1");
  const auto mask = button_mask(gamepad_mapping(button));
  auto &state = gamepad_masks_[port];
  state = pressed ? static_cast<std::uint16_t>(state | mask)
                  : static_cast<std::uint16_t>(state & ~mask);
}

void InputState::disconnect(std::uint8_t port) {
  if (port >= gamepad_masks_.size())
    throw std::out_of_range("gamepad port must be 0 or 1");
  gamepad_masks_[port] = 0;
}

void InputState::clear() {
  keyboard_mask_ = 0;
  gamepad_masks_.fill(0);
}

std::array<std::uint16_t, 2> InputState::snapshot() const {
  return {static_cast<std::uint16_t>(keyboard_mask_ | gamepad_masks_[0]),
          gamepad_masks_[1]};
}

Viewport integer_viewport(int output_width, int output_height) {
  if (output_width <= 0 || output_height <= 0)
    throw std::invalid_argument("display output size must be positive");
  constexpr int source_width = 256, source_height = 224;
  const int scale = std::max(
      1, std::min(output_width / source_width, output_height / source_height));
  const int width = source_width * scale;
  const int height = source_height * scale;
  return {(output_width - width) / 2, (output_height - height) / 2, width,
          height, scale};
}

bool is_recovered_pose_pair(const MovementState &state) {
  const auto player = state.riders[0].pose.pose_index;
  const auto opponent = state.riders[1].pose.pose_index;
  if (!state.riders[0].pose.reflected || !state.riders[1].pose.reflected)
    return false;
  return (player == 0x04f9 && opponent == 0x0263) ||
         (player == 0x0855 && opponent == 0x0895) ||
         (player == 0x0895 && opponent == 0x0895) ||
         (player == 0x0855 && opponent == 0x08d5) ||
         (player == 0x04fe && opponent == 0x037c);
}

LiveFrame LivePresentation::render(const MovementState &state,
                                   const PresentationPosition &position,
                                   const PresentationContent &content) {
  const bool fallback = !is_recovered_pose_pair(state);
  if (!fallback) {
    for (std::size_t rider = 0; rider < 2; ++rider) {
      recovered_pair_.pose_indices[rider] = state.riders[rider].pose.pose_index;
      recovered_pair_.reflected[rider] = state.riders[rider].pose.reflected;
    }
  }
  const PresentationSample sample{state, position.camera_x, position.bg1_x,
                                  position.bg1_y, position.bg2_x,
                                  position.bg2_y};
  if (!fallback)
    return {render_dragster_headless(sample, content), false};
  const std::array<RiderArtPose, 2> rider_art{
      RiderArtPose{recovered_pair_.pose_indices[0],
                   recovered_pair_.reflected[0]},
      RiderArtPose{recovered_pair_.pose_indices[1],
                   recovered_pair_.reflected[1]}};
  return {render_dragster_headless_with_rider_art(sample, content, rider_art),
          true};
}

void LivePresentation::observe_update(const ZoomZooState& previous,const ZoomZooState& updated,
                                      const ClassicContentPack& pack) {
  history_.observe_update(previous,updated,pack);
}

LiveFrame LivePresentation::render_race(const ZoomZooState& state,const ZoomZooState& previous_update,
                                        const ClassicRacePresentationContent& content) {
  auto history=history_.on_screen();
  // The authored tour result draws no riders; every other picture does.
  if(state.result_updates && content.result_base_vram.empty())
    return {render_classic_race(state,content,&previous_update,&history),false};
  auto drawn=previous_update;
  bool fallback=false;
  for(std::size_t rider=0;rider<2;++rider) {
    auto& pose=drawn.movement.riders[rider].pose.pose_index;
    try {
      (void)compose_rider_object(content.riders,pose,history.overlays.pose[rider],RiderRowClip::none);
      drawn_pose_[rider]=pose;
    } catch(const std::invalid_argument&) {
      if(!drawn_pose_[rider])throw;
      pose=*drawn_pose_[rider];
      history.overlays.pose[rider].reset();
      fallback=true;
    }
  }
  return {render_classic_race(state,content,&drawn,&history),fallback};
}

} // namespace unirally::app
