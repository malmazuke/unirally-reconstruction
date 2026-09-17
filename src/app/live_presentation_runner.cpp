#include "content_pack.hpp"
#include "frontend.hpp"
#include "movement.hpp"

#include <charconv>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
std::vector<std::uint8_t> read(const std::filesystem::path &path) {
  std::ifstream input(path, std::ios::binary);
  if (!input)
    throw std::runtime_error("cannot open live presentation state");
  return {std::istreambuf_iterator<char>(input), {}};
}

template <typename Integer>
Integer parse_integer(const char *text, const char *name) {
  Integer value{};
  const std::string_view input(text);
  const auto parsed =
      std::from_chars(input.data(), input.data() + input.size(), value);
  if (parsed.ec != std::errc{} || parsed.ptr != input.data() + input.size())
    throw std::invalid_argument(std::string("invalid ") + name);
  return value;
}
} // namespace

int main(int argc, char **argv) try {
  std::filesystem::path pack_path, state_path, out_path;
  unirally::app::PresentationPosition position{};
  for (int index = 1; index < argc; index += 2) {
    if (index + 1 >= argc)
      throw std::invalid_argument(
          "live presentation runner requires option values");
    const std::string option = argv[index];
    if (option == "--content-pack")
      pack_path = argv[index + 1];
    else if (option == "--state")
      state_path = argv[index + 1];
    else if (option == "--out")
      out_path = argv[index + 1];
    else if (option == "--camera-x")
      position.camera_x =
          parse_integer<std::int32_t>(argv[index + 1], "camera-x");
    else if (option == "--bg1-scroll-x")
      position.bg1_x =
          parse_integer<std::int16_t>(argv[index + 1], "bg1-scroll-x");
    else if (option == "--bg1-scroll-y")
      position.bg1_y =
          parse_integer<std::int16_t>(argv[index + 1], "bg1-scroll-y");
    else if (option == "--bg2-scroll-x")
      position.bg2_x =
          parse_integer<std::int16_t>(argv[index + 1], "bg2-scroll-x");
    else if (option == "--bg2-scroll-y")
      position.bg2_y =
          parse_integer<std::int16_t>(argv[index + 1], "bg2-scroll-y");
    else
      throw std::invalid_argument("unknown live presentation option: " +
                                  option);
  }
  if (pack_path.empty() || state_path.empty() || out_path.empty())
    throw std::invalid_argument(
        "live presentation runner requires --content-pack, --state and --out");

  unirally::ClassicContentPack pack(pack_path);
  const auto state = unirally::deserialize_movement_state(read(state_path));
  const auto before = unirally::serialize_movement_state(state);
  const unirally::PresentationContent content{
      pack.entry("physics.track.dragster.data"),
      pack.entry("presentation.track.dragster.bg1-tiles.v1"),
      pack.entry("presentation.track.dragster.bg2-tiles.v1"),
      pack.entry("presentation.track.dragster.bg2-map.v1"),
      pack.entry("presentation.classic.palette.v1"),
      pack.entry("presentation.classic.font.v1"),
      pack.entry("presentation.rider.mike.race-tiles.v1"),
      pack.entry("presentation.result.classic.font-layout.v1"),
      pack.entry("presentation.effect.go-window.v1"),
      pack.entry("presentation.effect.winner-window.v1"),
      pack.entry("presentation.result.classic.base-vram.v1"),
      pack.entry("presentation.result.classic.palette.v1"),
      pack.entry("presentation.result.classic.palette-tail.v1"),
      pack.optional_entry("presentation.zoom.race-palette-cycle.v1"),
      pack.optional_entry("presentation.effect.classic.window-tables.v1")};

  unirally::app::LivePresentation fresh;
  const auto first = fresh.render(state, position, content);
  unirally::app::LivePresentation reused;
  (void)reused.render(state, position, content);
  const auto second = reused.render(state, position, content);
  if (first.frame.pixels != second.frame.pixels ||
      first.used_pose_fallback != second.used_pose_fallback)
    throw std::runtime_error(
        "live presentation history changed the stable result");
  if (unirally::serialize_movement_state(state) != before)
    throw std::runtime_error("live presentation changed canonical state");
  if (state.finish.outcome == unirally::RaceOutcome::PlayerLost) {
    auto prior_counter = state;
    if (prior_counter.finish.result_loading_updates == 0)
      throw std::runtime_error("loser result has no prior loading counter");
    --prior_counter.finish.result_loading_updates;
    bool rejected = false;
    try {
      (void)fresh.render(prior_counter, position, content);
    } catch (const std::invalid_argument &) {
      rejected = true;
    }
    if (!rejected)
      throw std::runtime_error(
          "live presentation accepted a contradictory loser boundary");
  }

  std::ofstream output(out_path, std::ios::binary | std::ios::trunc);
  if (!output)
    throw std::runtime_error("cannot create live presentation output");
  output << "P6\n256 224\n255\n";
  output.write(reinterpret_cast<const char *>(first.frame.pixels.data()),
               static_cast<std::streamsize>(first.frame.pixels.size()));
  if (!output)
    throw std::runtime_error("cannot write live presentation output");
  return 0;
} catch (const std::exception &error) {
  std::cerr << error.what() << '\n';
  return 1;
}
