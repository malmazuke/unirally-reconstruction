#include "frontend.hpp"
#include "movement.hpp"
#include "presentation.hpp"
#include "zoom_zoo_movement.hpp"

#include <array>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace {
void require(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}
}

// FRONT-END-1P-SETUP: 1P races natively where a race scenario has the race.
void one_player_race_rule() {
  auto state = unirally::start_front_end();
  state.mode_chosen = true;
  state.mode = unirally::FrontEndMode::one_player;
  state.rider_menu.rider = 0;          // MIKE
  state.now_playing.opponent = 0x11;   // BRONSEN
  state.tour_menu.track = 13;          // FLAT FUN
  require(unirally::app::native_one_player_race(state), "MIKE against BRONSEN races");
  state.tour_menu.track = 2;           // BOWL, a stunt event
  require(!unirally::app::native_one_player_race(state), "a stunt event is refused");
  state.tour_menu.track = 13;
  state.rider_menu.rider = 1;          // ANDREW
  require(!unirally::app::native_one_player_race(state), "another rider is refused");
  state.rider_menu.rider = 0;
  state.now_playing.opponent = 0x12;   // SILVIA
  require(!unirally::app::native_one_player_race(state), "another opponent is refused");
  state.now_playing.opponent = 0x11;
  state.mode = unirally::FrontEndMode::two_player;
  require(!unirally::app::native_one_player_race(state), "2P is refused");
}

int main() {
  one_player_race_rule();
  using namespace unirally::app;
  PalScheduler scheduler(1'000);
  require(scheduler.updates_due(20'000'999) == 0, "no early update");
  require(scheduler.updates_due(20'001'000) == 1, "exact deadline");
  require(scheduler.updates_due(80'001'000) == 3, "retained catch-up");
  require(scheduler.updates_due(400'001'000) == 4, "bounded long stall");
  require(scheduler.next_deadline_ns() == 420'001'000, "stall debt dropped");
  scheduler.pause(401'000'000);
  require(scheduler.updates_due(900'000'000) == 0, "paused update");
  scheduler.resume(900'000'000);
  require(scheduler.updates_due(919'999'999) == 0, "resume is not early");
  require(scheduler.updates_due(920'000'000) == 1, "resume deadline");

  InputState input;
  input.keyboard(KeyboardKey::Z, true);
  input.keyboard(KeyboardKey::Right, true);
  input.gamepad(0, GamepadButton::North, true);
  input.gamepad(1, GamepadButton::LeftShoulder, true);
  auto masks = input.snapshot();
  require(masks[0] == (button_mask(LogicalButton::B) |
                       button_mask(LogicalButton::Right) |
                       button_mask(LogicalButton::X)),
          "simultaneous keyboard/gamepad port zero");
  require(masks[1] == button_mask(LogicalButton::LeftShoulder),
          "second gamepad owns port one");
  input.keyboard(KeyboardKey::Z, false);
  input.gamepad(0, GamepadButton::North, false);
  require(input.snapshot()[0] == button_mask(LogicalButton::Right),
          "release clears its source");
  input.disconnect(1);
  require(input.snapshot()[1] == 0, "disconnect clears port");
  input.clear();
  require(input.snapshot() == std::array<std::uint16_t, 2>{},
          "focus loss clears every source");

  for (unsigned bit = 0; bit < 12; ++bit) {
    const auto logical = static_cast<LogicalButton>(bit);
    require(button_mask(logical) == (1U << bit), "accepted SNES bit order");
    require(keyboard_mapping(static_cast<KeyboardKey>(bit)) == logical,
            "complete keyboard mapping");
    require(gamepad_mapping(static_cast<GamepadButton>(bit)) == logical,
            "complete gamepad mapping");
  }
  const auto buttons = controller_buttons(
      button_mask(LogicalButton::B) | button_mask(LogicalButton::Start) |
      button_mask(LogicalButton::Up) | button_mask(LogicalButton::A) |
      button_mask(LogicalButton::RightShoulder));
  require(buttons.b && buttons.start && buttons.up && buttons.a &&
              buttons.right_shoulder && !buttons.y && !buttons.down,
          "mask converts to semantic buttons");

  const auto three = integer_viewport(1000, 700);
  require(three.x == 116 && three.y == 14 && three.width == 768 &&
              three.height == 672 && three.scale == 3,
          "centered integer viewport");
  const auto minimum = integer_viewport(200, 100);
  require(minimum.scale == 1 && minimum.width == 256 &&
              minimum.height == 224,
          "sub-source output retains nearest 1x contract");
  bool rejected = false;
  try {
    (void)integer_viewport(0, 224);
  } catch (const std::invalid_argument &) {
    rejected = true;
  }
  require(rejected, "invalid display size rejected");

  // Two display-poll cadences consume the same update-indexed live input
  // script. Every input snapshot advances exactly one canonical update.
  unirally::MovementState initial{};
  initial.frame = 1533;
  initial.countdown = 69;
  initial.contact_phase = 1;
  initial.progress_phase = 1;
  initial.animation_counter = 13;
  initial.update_counter = 205;
  initial.rewards.write_cursor = 1;
  initial.rewards.cooldown = 2;
  initial.rewards.event_one_weight = 4;
  for (auto &rider : initial.riders) {
    rider.motion.x = 1088;
    rider.motion.y = 64;
    rider.pose.previous_x = 1088;
    rider.pose.previous_y = 64;
    rider.pose.reflected = true;
    rider.throttle = 432;
    rider.previous_brake = 1;
  }
  initial.riders[0].residue_x = 0xffff;
  const std::array<std::uint8_t, 9> speed_masks{
      0xff, 0x7f, 0x3f, 0x1f, 0x0f, 0x07, 0x03, 0x01, 0x00};
  const std::array<std::uint8_t, 18> decrements{
      4, 0, 4, 0, 4, 0, 4, 0, 4, 0, 4, 0, 4, 0, 4, 0, 4, 0};
  std::vector<std::uint8_t> track(33815), poses(32768), templates(17249);
  std::vector<std::uint8_t> transitions(80), columns(640), flags(20),
      slopes(128), displacement(512), idle_pose(64), reward(2),
      reward_class(1);
  displacement[28] = 196;
  const unirally::MovementContent movement_content{
      {track, poses, templates}, {columns, flags}, transitions, slopes,
      displacement, idle_pose, reward, reward_class,
      {speed_masks, decrements}};
  const std::array<std::uint16_t, 8> live_script{
      button_mask(LogicalButton::Right), button_mask(LogicalButton::Right),
      button_mask(LogicalButton::Right), 0, button_mask(LogicalButton::Right),
      button_mask(LogicalButton::Right), 0, button_mask(LogicalButton::Right)};
  const auto run = [&](const std::vector<std::uint64_t> &polls) {
    PalScheduler clock(0);
    auto state = initial;
    std::vector<std::vector<std::uint8_t>> states;
    std::size_t update{};
    for (const auto poll : polls) {
      const auto count = clock.updates_due(poll);
      for (std::uint32_t due_index = 0; due_index < count; ++due_index) {
        require(update < live_script.size(), "poll schedule overran script");
        const auto snapshot = live_script[update++];
        unirally::update_movement(state, controller_buttons(snapshot),
                                  movement_content);
        states.push_back(unirally::serialize_movement_state(state));
      }
    }
    require(update == live_script.size(), "poll schedule omitted updates");
    return states;
  };
  require(run({20'000'000, 40'000'000, 60'000'000, 80'000'000,
               100'000'000, 120'000'000, 140'000'000, 160'000'000}) ==
              run({40'000'000, 80'000'000, 120'000'000, 160'000'000}),
          "display cadence does not alter canonical sequence");

  // The accepted headless renderer consumes but cannot change gameplay state.
  auto presentation_state = initial;
  presentation_state.riders[0].pose.pose_index = 0x04f9;
  presentation_state.riders[1].pose.pose_index = 0x0263;
  std::vector<std::uint8_t> bg1(2560), bg2(992), bg2_map(8192),
      palette(352), font(2048), rider_tiles(3456), result_assets(5224),
      go, winner, result_vram(41536), result_palette(216),
      result_tail(128);
  const auto empty_window_table = [] {
    std::vector<std::uint8_t> table;
    for (const unsigned lines : {127U, 97U}) {
      table.push_back(static_cast<std::uint8_t>(0x80U | lines));
      for (unsigned line = 0; line < lines; ++line)
        table.insert(table.end(), {255, 0, 255, 0});
    }
    return table;
  };
  go = empty_window_table();
  winner = empty_window_table();
  for (std::size_t index = 0; index < palette.size(); ++index)
    palette[index] = static_cast<std::uint8_t>((index * 37U) & 0xffU);
  for (std::size_t index = 0; index < rider_tiles.size(); ++index)
    rider_tiles[index] =
        static_cast<std::uint8_t>(((index / 32U) * 37U + index) & 0xffU);
  const auto before = unirally::serialize_movement_state(presentation_state);
  const auto rendered = unirally::render_dragster_headless(
      {presentation_state, 0, -14, 208, -7, 104},
      {track, bg1, bg2, bg2_map, palette, font, rider_tiles, result_assets,
       go, winner, result_vram, result_palette, result_tail, {}, {}});
  require(rendered.pixels.size() == 256U * 224U * 3U,
          "accepted native-resolution frame displayed");
  require(unirally::serialize_movement_state(presentation_state) == before,
          "presentation leaves canonical state unchanged");

  LivePresentation live;
  auto unsupported_one = presentation_state;
  unsupported_one.riders[0].pose.pose_index = 0x04fa;
  unsupported_one.riders[1].pose.pose_index = 0x04fa;
  unsupported_one.timer.seconds = 1;
  const PresentationPosition first_position{100, 86, 208, 43, 104};
  const auto first_live = live.render(unsupported_one, first_position,
                                      {track, bg1, bg2, bg2_map, palette, font,
                                       rider_tiles, result_assets, go, winner,
                                       result_vram, result_palette, result_tail, {}, {}});
  require(first_live.used_pose_fallback, "unsupported pair uses rider fallback");
  const std::array<unirally::RiderArtPose, 2> opening_art{
      unirally::RiderArtPose{0x04f9, true},
      unirally::RiderArtPose{0x0263, true}};
  const auto direct_one = unirally::render_dragster_headless_with_rider_art(
      {unsupported_one, first_position.camera_x, first_position.bg1_x,
       first_position.bg1_y, first_position.bg2_x, first_position.bg2_y},
      {track, bg1, bg2, bg2_map, palette, font, rider_tiles, result_assets,
       go, winner, result_vram, result_palette, result_tail, {}, {}},
      opening_art);
  require(first_live.frame.pixels == direct_one.pixels,
          "fallback changes only rider art selection");

  auto unsupported_two = unsupported_one;
  unsupported_two.frame += 1;
  unsupported_two.riders[0].motion.x += 32;
  unsupported_two.riders[1].motion.x += 16;
  unsupported_two.timer.seconds = 2;
  const PresentationPosition second_position{132, 118, 208, 59, 104};
  const auto second_live = live.render(unsupported_two, second_position,
                                       {track, bg1, bg2, bg2_map, palette, font,
                                        rider_tiles, result_assets, go, winner,
                                        result_vram, result_palette, result_tail, {}, {}});
  require(second_live.used_pose_fallback, "later unsupported pair uses fallback");
  require(first_live.frame.pixels != second_live.frame.pixels,
          "unsupported gameplay/camera/timer states produce fresh frames");
  require(unirally::serialize_movement_state(unsupported_one) !=
              unirally::serialize_movement_state(unsupported_two),
          "regression states are distinct");
  require(live.last_recovered_pose_pair().pose_indices ==
              std::array<std::uint16_t, 2>{0x04f9, 0x0263},
          "rider art remains the recovered fallback pair");

  const auto active_window_table = [] {
    std::vector<std::uint8_t> table;
    for (const unsigned lines : {127U, 97U}) {
      table.push_back(static_cast<std::uint8_t>(0x80U | lines));
      for (unsigned line = 0; line < lines; ++line)
        table.insert(table.end(), {0, 255, 255, 0});
    }
    return table;
  };
  go = active_window_table();
  winner = active_window_table();
  const unirally::PresentationContent effect_content{
      track, bg1, bg2, bg2_map, palette, font, rider_tiles, result_assets,
      go, winner, result_vram, result_palette, result_tail, {}, {}};

  LivePresentation rolling_history;
  auto rolling = presentation_state;
  rolling.riders[0].pose.pose_index = 0x0855;
  rolling.riders[1].pose.pose_index = 0x0895;
  (void)rolling_history.render(rolling, first_position, effect_content);
  LivePresentation finish_history;
  auto finish = presentation_state;
  finish.riders[0].pose.pose_index = 0x04fe;
  finish.riders[1].pose.pose_index = 0x037c;
  (void)finish_history.render(finish, first_position, effect_content);

  auto same_unsupported = unsupported_one;
  same_unsupported.riders[0].motion.x = 0;
  same_unsupported.riders[1].motion.x = 0;
  const auto same_before = unirally::serialize_movement_state(same_unsupported);
  const PresentationPosition offscreen_position{0, -14, 208, -7, 104};
  const auto rolling_offscreen =
      rolling_history.render(same_unsupported, offscreen_position,
                             effect_content);
  const auto finish_offscreen =
      finish_history.render(same_unsupported, offscreen_position,
                            effect_content);
  require(rolling_offscreen.frame.pixels == finish_offscreen.frame.pixels,
          "off-screen fallback history cannot change scene effects");
  require(unirally::serialize_movement_state(same_unsupported) == same_before,
          "fallback histories leave canonical state unchanged");

  auto visible_unsupported = same_unsupported;
  visible_unsupported.riders[0].motion.x = 900;
  visible_unsupported.riders[1].motion.x = 1050;
  visible_unsupported.riders[0].motion.y = 752;
  visible_unsupported.riders[1].motion.y = 752;
  const auto visible_before =
      unirally::serialize_movement_state(visible_unsupported);
  const auto rolling_visible =
      rolling_history.render(visible_unsupported, offscreen_position,
                             effect_content);
  const auto finish_visible =
      finish_history.render(visible_unsupported, offscreen_position,
                            effect_content);
  require(rolling_visible.frame.pixels != finish_visible.frame.pixels,
          "visible rider art retains its recovered history");
  for (std::size_t pixel_index = 0;
       pixel_index < rolling_visible.frame.pixels.size(); pixel_index += 3) {
    const bool differs =
        rolling_visible.frame.pixels[pixel_index] !=
            finish_visible.frame.pixels[pixel_index] ||
        rolling_visible.frame.pixels[pixel_index + 1] !=
            finish_visible.frame.pixels[pixel_index + 1] ||
        rolling_visible.frame.pixels[pixel_index + 2] !=
            finish_visible.frame.pixels[pixel_index + 2];
    if (!differs)
      continue;
    const auto pixel = pixel_index / 3;
    const int x = static_cast<int>(pixel % unirally::RgbFrame::width);
    const int y = static_cast<int>(pixel / unirally::RgbFrame::width);
    const bool inside_player = x >= 68 && x < 132 && y >= 0 && y < 64;
    const bool inside_opponent = x >= 218 && x < 256 && y >= 0 && y < 64;
    require(inside_player || inside_opponent,
            "fallback history differences are confined to visible riders");
  }
  require(unirally::serialize_movement_state(visible_unsupported) ==
              visible_before,
          "visible fallback leaves canonical state unchanged");
}
