#include "frontend.hpp"
#include "movement.hpp"
#include "presentation.hpp"
#include "save_file.hpp"
#include "zoom_zoo_movement.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <vector>

namespace {
void require(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}
}

// FRONT-END-1P-SETUP: 1P races natively where a race scenario has the race;
// RACE-RIDERS-OPPONENTS: whoever rides it, against whichever opponent;
// STUNT-RESULT: the stunt events too.
void one_player_race_rule() {
  auto state = unirally::start_front_end();
  state.mode_chosen = true;
  state.mode = unirally::FrontEndMode::one_player;
  state.rider_menu.rider = 0;          // MIKE
  state.now_playing.opponent = 0x11;   // BRONSEN
  state.tour_menu.track = 13;          // FLAT FUN
  require(unirally::app::native_menu_race(state), "MIKE against BRONSEN races");
  state.tour_menu.track = 2;           // BOWL, a stunt event: its result is native (R-0067)
  require(unirally::app::native_menu_race(state), "a stunt event races");
  state.tour_menu.track = 13;
  state.rider_menu.rider = 1;          // ANDREW
  require(unirally::app::native_menu_race(state), "another rider races");
  state.rider_menu.rider = 0;
  state.now_playing.opponent = 0x12;   // SILVIA
  require(unirally::app::native_menu_race(state), "SILVIA races");
  state.now_playing.opponent = 0x13;   // GOLDWYN
  require(unirally::app::native_menu_race(state), "GOLDWYN races");
  state.now_playing.opponent = 0x11;
  state.mode = unirally::FrontEndMode::two_player;
  state.tour_menu.track = 1; // DRAGSTER
  require(unirally::app::native_menu_race(state), "2P DRAGSTER races");
  state.mode = unirally::FrontEndMode::versus;
  require(unirally::app::native_menu_race(state), "VS DRAGSTER races");
  state.mode_chosen = false;
  require(!unirally::app::native_menu_race(state), "menu choice precedes race");
}

// STUNT-EVENT-RACE and STUNT-RESULT: --track starts the races a scenario has but no stunt event,
// whose result is the menus' (R-0067), not the --track path's Classic result screen; the menus
// start every one of them, the stunt events too.
void track_option_rule() {
  using unirally::ClassicRaceTrack;
  auto state = unirally::start_front_end();
  state.mode_chosen = true;
  state.mode = unirally::FrontEndMode::one_player;
  state.now_playing.opponent = 0x11;
  for (const std::uint8_t stunt : std::array<std::uint8_t, 9>{2, 7, 12, 17, 22, 27, 32, 37, 42}) {
    require(!unirally::app::native_race_track(ClassicRaceTrack{stunt}), "--track refuses a stunt event");
    state.tour_menu.track = stunt;
    require(unirally::app::native_menu_race(state), "the menus race a stunt event");
  }
  for (const std::uint8_t race : std::array<std::uint8_t, 6>{0, 1, 13, 36, 41, 44})
    require(unirally::app::native_race_track(ClassicRaceTrack{race}), "a race track runs");
  require(!unirally::app::native_race_track(ClassicRaceTrack{45}), "no track 45");
}

// APP-LEAGUE-RACE (R-0106): a scripted race reads front_end_runner's frames.
// With native audio the app waits the race's loading first, and those updates
// read the frames before the initialization frame; the race's first update
// reads the frame after it either way.
void scripted_race_frames_rule() {
  unirally::app::ScriptedRaceFrames frames;
  // podium-audio's league race: chosen at front-end frame 2708, 120 loading
  // frames.
  frames.start(2827, 120);
  require(frames.next() == 2708,
          "the first loading frame follows NOW PLAYING's last");
  for (unsigned loading = 1; loading < 120; ++loading)
    (void)frames.next();
  require(frames.next() == 2828,
          "the race's first update reads the frame after its initialization");
  require(frames.next() == 2829, "each update reads the next frame");
  frames.start(2827, 0); // no loading waited (no native audio)
  require(frames.next() == 2828,
          "without the wait the race's first update comes at once");
  // An idle demo's race, chosen at front-end frame 900 (R-0110): its first
  // update is front_end_runner's frame 900, as its exit frame counts it.
  frames.start(900 - 1, 0);
  require(frames.next() == 900,
          "the demo's first update is the front end's frame");
}

// APP-PARITY (R-0110): a player's launch has audio and the default save; a
// laboratory launch has neither unless asked, so a scripted run never touches
// the player's save; a --track race has neither.
void launch_settings_rule() {
  using unirally::app::launch_settings;
  using unirally::app::LaunchRequest;
  const auto player = launch_settings(LaunchRequest{});
  require(player.audio && !player.audio_asked && player.default_save_file,
          "a player's launch has audio and the default save");
  LaunchRequest laboratory;
  laboratory.laboratory = true;
  const auto silent = launch_settings(laboratory);
  require(!silent.audio && !silent.default_save_file,
          "a laboratory launch is silent and keeps no save");
  laboratory.audio = true;
  laboratory.save_file_given = true;
  const auto asked = launch_settings(laboratory);
  require(asked.audio && asked.audio_asked && !asked.default_save_file,
          "a laboratory launch takes audio and a save file when asked");
  LaunchRequest opted_out;
  opted_out.audio = false;
  opted_out.no_save_file = true;
  const auto none = launch_settings(opted_out);
  require(!none.audio && !none.default_save_file,
          "--no-audio and --no-save-file opt a player out");
  LaunchRequest track;
  track.track = true;
  const auto race = launch_settings(track);
  require(!race.audio && !race.default_save_file, "a --track race has neither");
  const auto refused = [](LaunchRequest request) {
    try {
      (void)launch_settings(request);
    } catch (const std::invalid_argument &) {
      return true;
    }
    return false;
  };
  track.audio = true;
  require(refused(track), "audio needs the front end");
  LaunchRequest both;
  both.save_file_given = both.no_save_file = true;
  require(refused(both), "--save-file and --no-save-file contradict");
}

void default_save_file_rule() {
  using unirally::app::default_save_file;
  using unirally::app::SavePlatform;
  require(
      default_save_file(SavePlatform::macos, {"/Users/a", nullptr, nullptr}) ==
          "/Users/a/Library/Application Support/Unirally/unirally.sav",
      "macOS keeps it in Application Support");
  require(default_save_file(SavePlatform::other_unix,
                            {"/home/a", "/data", nullptr}) ==
              "/data/unirally/unirally.sav",
          "Linux keeps it in XDG_DATA_HOME");
  require(default_save_file(SavePlatform::other_unix,
                            {"/home/a", "rel", nullptr}) ==
              "/home/a/.local/share/unirally/unirally.sav",
          "a relative XDG_DATA_HOME is ignored");
  require(
      default_save_file(SavePlatform::other_unix, {"/home/a", "", nullptr}) ==
          "/home/a/.local/share/unirally/unirally.sav",
      "an empty XDG_DATA_HOME is ignored");
  require(default_save_file(SavePlatform::windows,
                            {nullptr, nullptr, "C:/AppData"}) ==
              std::filesystem::path("C:/AppData") / "Unirally" / "unirally.sav",
          "Windows keeps it in APPDATA");
  require(default_save_file(SavePlatform::macos, {}).empty(),
          "no home, no default save");
}

// APP-PARITY (R-0110): a laboratory script's frames go on through a notice and
// after it. notice_race.sh's route: the WIPE RAM code at front-end frame 500
// leaves the front end at 501 for the notice's 100 frames, then the main menu
// comes back at its first frame, 420.
void script_clock_rule() {
  unirally::app::ScriptClock clock;
  require(clock.frame(420, 0) == 420 && clock.offset() == 0,
          "before a notice the script is read at the front end's frame");
  require(clock.frame(501, 100) == 501, "a notice's first frame");
  require(clock.frame(501, 99) == 502, "a notice's frames count");
  require(clock.frame(501, 1) == 600, "a notice's last frame");
  clock.notice_ended(501, 420);
  require(clock.offset() == 181,
          "the notice and the main-menu frames gone back over");
  require(clock.frame(420, 0) == 601,
          "the main menu goes on from the notice's end");
  // A second notice at front-end frame 500 again (script frame 681).
  require(clock.frame(501, 100) == 682 && clock.frame(501, 1) == 781,
          "a second notice counts on");
  clock.notice_ended(501, 420);
  require(clock.frame(420, 0) == 782, "and the main menu goes on from its end");
}

int main() {
  script_clock_rule();
  launch_settings_rule();
  default_save_file_rule();
  one_player_race_rule();
  track_option_rule();
  scripted_race_frames_rule();
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
