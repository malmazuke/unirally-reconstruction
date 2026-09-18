// Randomized ordinary-input fuzz for DRAGSTER on the shared race engine.
// It drives the same calls as the desktop app's update loop (physical D-pad,
// Start at the stable result restarts, pause menu restart) over many seeds of
// complete races and reports every exception as an abort. It also renders
// DRAGSTER pictures at phase changes and every 64th update, and checks the
// canonical state survives serialization after every update. Validation only.
#include "content_pack.hpp"
#include "frontend.hpp"
#include "zoom_zoo_pack.hpp"

#include <charconv>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
unsigned parse(std::string_view text, const char *name) {
  unsigned value{};
  const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value);
  if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size())
    throw std::invalid_argument(std::string("invalid ") + name);
  return value;
}

using unirally::app::LogicalButton;
std::uint16_t bit(LogicalButton button) { return unirally::app::button_mask(button); }

// Segments of held buttons, 1-90 updates each. Style 0 plays like a person:
// mostly Right, with jumps, brakes, Left reversals, A, X, L/R, Up/Down, Select
// and a rare Start pause. Style 1 adds keyboard-only opposing directions and
// frequent pause navigation, including RESTART RACE. Style 2 mashes every
// button every update for its first 1,500 updates, then plays as style 0.
class Player {
public:
  Player(std::uint32_t seed, unsigned style) : random_(seed), style_(style) {}
  std::uint16_t next() {
    ++updates_;
    if (style_ == 2 && updates_ <= 1500U)
      return static_cast<std::uint16_t>(draw(0x1000U));
    if (remaining_ == 0) {
      remaining_ = 1U + draw(90U);
      mask_ = 0;
      const bool pausing = style_ == 1;
      const auto roll = draw(100U);
      if (roll < 78U) mask_ |= bit(LogicalButton::Right);
      else if (roll < 92U) mask_ |= bit(LogicalButton::Left);
      if (pausing && draw(5U) == 0U) {
        mask_ |= bit(LogicalButton::Left);
        mask_ |= bit(LogicalButton::Right);
      }
      const std::array<std::pair<LogicalButton, unsigned>, 9> odds{{
          {LogicalButton::B, 25}, {LogicalButton::Y, 8}, {LogicalButton::A, 10}, {LogicalButton::X, 15},
          {LogicalButton::LeftShoulder, 12}, {LogicalButton::RightShoulder, 12}, {LogicalButton::Up, pausing ? 20U : 5U},
          {LogicalButton::Down, pausing ? 20U : 5U}, {LogicalButton::Select, 2}}};
      for (const auto &[button, percent] : odds)
        if (draw(100U) < percent) mask_ |= bit(button);
      if (draw(1000U) < (pausing ? 60U : 5U)) {
        mask_ |= bit(LogicalButton::Start);
        remaining_ = 1U + remaining_ % 6U;
      }
    }
    --remaining_;
    return mask_;
  }
private:
  // result_type is 64 bits wide on LP64 Linux; the bound keeps it in range.
  unsigned draw(unsigned bound) { return static_cast<unsigned>(random_() % bound); }
  std::mt19937 random_;
  unsigned style_{};
  unsigned remaining_{}, updates_{};
  std::uint16_t mask_{};
};
} // namespace

int main(int argc, char **argv) try {
  std::filesystem::path pack_path, failure_cases;
  unsigned first_seed = 1, seeds = 100, maximum_updates = 20000, races_per_seed = 2;
  for (int index = 1; index + 1 < argc; index += 2) {
    const std::string_view option(argv[index]), value(argv[index + 1]);
    if (option == "--content-pack") pack_path = value;
    else if (option == "--first-seed") first_seed = parse(value, "first seed");
    else if (option == "--seeds") seeds = parse(value, "seed count");
    else if (option == "--max-updates") maximum_updates = parse(value, "update limit");
    else if (option == "--races") races_per_seed = parse(value, "race count");
    else if (option == "--failure-cases") failure_cases = value;
    else throw std::invalid_argument("usage: dragster_fuzz_runner --content-pack PATH [--first-seed N] [--seeds N] [--max-updates N] [--races N]");
  }
  if (pack_path.empty()) throw std::invalid_argument("--content-pack is required");
  const unirally::ClassicContentPack pack(pack_path);
  const auto content = unirally::dragster_race_content(pack);
  const auto presentation = unirally::classic_race_presentation_content(pack, unirally::ClassicRaceTrack::Dragster);
  unsigned aborts = 0, total_races = 0, total_updates = 0, total_renders = 0, pause_restarts = 0;
  for (unsigned seed = first_seed; seed < first_seed + seeds; ++seed) {
    const unsigned style = seed % 3U;
    Player player(seed, style);
    auto state = unirally::classic_crawler_dragster_race_start(content);
    unirally::app::LivePresentation live;
    auto previous = state; // The update before the one drawn (R-0036).
    unsigned races = 0, updates = 0, renders = 0;
    std::string failure;
    std::vector<std::uint16_t> since_start; // Masks applied since the current race began.
    auto previous_phase = unirally::classic_finish_view(state).phase;
    try {
      for (; updates < maximum_updates && races < races_per_seed; ++updates) {
        const bool stable = state.result_updates && state.result_updates == unirally::stable_result_updates(state);
        auto mask = player.next();
        // A player at the stable result soon presses Start for Race Again.
        if (stable && (updates & 15U) == 0U) mask = static_cast<std::uint16_t>(mask | bit(LogicalButton::Start));
        // Likewise a paused player soon confirms the highlighted pause entry.
        if (state.pause.selection && (updates & 31U) == 0U) mask = static_cast<std::uint16_t>(mask | bit(LogicalButton::Start));
        const auto buttons = unirally::with_physical_dpad(unirally::app::controller_buttons(mask));
        const auto result_before = state.result_updates;
        const auto frame_before = state.movement.frame;
        previous = state;
        if (stable && buttons.start) {
          unirally::restart_zoom_zoo(state, content);
          ++races;
          live = {};
          previous = state;
          since_start.clear();
        } else {
          since_start.push_back(mask);
          unirally::update_zoom_zoo(state, buttons, content);
          if (state.movement.frame < frame_before) { ++pause_restarts; live = {}; previous = state; since_start.clear(); }
          else live.observe_update(previous, state, pack);
        }
        const auto phase = unirally::classic_finish_view(state).phase;
        if (phase != previous_phase || updates % 64U == 0U ||
            (state.result_updates != result_before && state.result_updates >= 224)) {
          (void)live.render_race(state, previous, presentation);
          ++renders;
        }
        previous_phase = phase;
        {
          const auto bytes = unirally::serialize_zoom_zoo(state);
          if (unirally::serialize_zoom_zoo(unirally::deserialize_zoom_zoo(bytes)) != bytes)
            throw std::logic_error("canonical state does not survive serialization");
        }
      }
    } catch (const std::exception &error) {
      failure = error.what();
      for (const auto &roll : state.rolls)
        std::cout << "  roll step " << static_cast<std::int16_t>(roll.step) << " held " << static_cast<std::int16_t>(roll.held_updates)
                  << " rotations " << roll.held_rotations << " completed " << roll.completed_rolls << '\n';
      std::cout << "  frame " << state.movement.frame << " suspended " << state.pause.suspended_updates << '\n';
      if (!failure_cases.empty()) {
        // The failing race as an original-capture case: one change per update
        // from the first race update, in the capture tool's button names.
        static constexpr std::array<const char *, 12> names{{"b", "y", "select", "start", "up", "down", "left", "right", "a", "x", "l", "r"}};
        std::filesystem::create_directories(failure_cases);
        std::ofstream out(failure_cases / ("fuzz-seed-" + std::to_string(seed) + ".case.json"));
        out << "{\"id\": \"fuzz-seed-" << seed << "\", \"changes\": [";
        bool first = true;
        const auto initialization = unirally::classic_race_scenario(unirally::ClassicRaceTrack::Dragster).initialization_frame;
        for (std::size_t at = 0; at < since_start.size(); ++at) {
          if (!since_start[at]) continue;
          out << (first ? "\n" : ",\n") << "  {\"from\": " << initialization + 1U + at << ", \"to\": " << initialization + 1U + at << ", \"buttons\": [";
          bool first_button = true;
          for (unsigned button = 0; button < names.size(); ++button)
            if (since_start[at] & (1U << button)) { out << (first_button ? "" : ", ") << '"' << names[button] << '"'; first_button = false; }
          out << "]}";
          first = false;
        }
        out << "\n]}\n";
      }
    }
    total_races += races; total_updates += updates; total_renders += renders;
    std::cout << "seed " << seed << " style " << style << " updates " << updates << " races " << races
              << " frame " << state.movement.frame << " result " << state.result_updates;
    if (!failure.empty()) { ++aborts; std::cout << " ABORT " << failure; }
    std::cout << '\n';
  }
  std::cout << "summary seeds " << seeds << " updates " << total_updates << " completed races " << total_races
            << " pause restarts " << pause_restarts << " renders " << total_renders << " aborts " << aborts << '\n';
  return aborts ? 1 : 0;
} catch (const std::exception &error) {
  std::cerr << error.what() << '\n';
  return 2;
}
