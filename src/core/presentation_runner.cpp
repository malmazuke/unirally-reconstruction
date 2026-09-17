#include "content_pack.hpp"
#include "movement.hpp"
#include "presentation.hpp"
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
std::vector<std::uint8_t> read(const std::filesystem::path &p) {
  std::ifstream in(p, std::ios::binary);
  if (!in)
    throw std::runtime_error("cannot open presentation state");
  return {std::istreambuf_iterator<char>(in), {}};
}
template <typename Integer>
Integer parse_integer(const char *text, const char *name) {
  Integer value{};
  const std::string_view input(text);
  const auto parsed = std::from_chars(input.data(), input.data() + input.size(),
                                      value);
  if (parsed.ec != std::errc{} || parsed.ptr != input.data() + input.size())
    throw std::invalid_argument(std::string("invalid ") + name);
  return value;
}
} // namespace
int main(int argc, char **argv) try {
  std::filesystem::path pack, state_path, out_path;
  std::int32_t camera{};
  std::int16_t sx{}, sy{}, bg2x{}, bg2y{};
  for (int i = 1; i < argc; i += 2) {
    if (i + 1 >= argc)
      throw std::invalid_argument("presentation runner requires option values");
    const std::string o = argv[i];
    if (o == "--content-pack")
      pack = argv[i + 1];
    else if (o == "--state")
      state_path = argv[i + 1];
    else if (o == "--out")
      out_path = argv[i + 1];
    else if (o == "--camera-x")
      camera = parse_integer<std::int32_t>(argv[i + 1], "camera-x");
    else if (o == "--bg1-scroll-x")
      sx = parse_integer<std::int16_t>(argv[i + 1], "bg1-scroll-x");
    else if (o == "--bg1-scroll-y")
      sy = parse_integer<std::int16_t>(argv[i + 1], "bg1-scroll-y");
    else if (o == "--bg2-scroll-x")
      bg2x = parse_integer<std::int16_t>(argv[i + 1], "bg2-scroll-x");
    else if (o == "--bg2-scroll-y")
      bg2y = parse_integer<std::int16_t>(argv[i + 1], "bg2-scroll-y");
    else
      throw std::invalid_argument("unknown presentation runner option: " + o);
  }
  if (pack.empty() || state_path.empty() || out_path.empty())
    throw std::invalid_argument(
        "presentation runner requires --content-pack, --state and --out");
  unirally::ClassicContentPack content(pack);
  const auto state = unirally::deserialize_movement_state(read(state_path));
  const unirally::PresentationContent assets{
      content.entry("physics.track.dragster.data"),
      content.entry("presentation.track.dragster.bg1-tiles.v1"),
      content.entry("presentation.track.dragster.bg2-tiles.v1"),
      content.entry("presentation.track.dragster.bg2-map.v1"),
      content.entry("presentation.classic.palette.v1"),
      content.entry("presentation.classic.font.v1"),
      content.entry("presentation.rider.mike.race-tiles.v1"),
      content.entry("presentation.result.classic.font-layout.v1"),
      content.entry("presentation.effect.go-window.v1"),
      content.entry("presentation.effect.winner-window.v1"),
      content.entry("presentation.result.classic.base-vram.v1"),
      content.entry("presentation.result.classic.palette.v1"),
      content.entry("presentation.result.classic.palette-tail.v1"),
      content.optional_entry("presentation.zoom.race-palette-cycle.v1")};
  const auto frame = unirally::render_dragster_headless(
      {state, camera, sx, sy, bg2x, bg2y},
      assets);
  std::ofstream out(out_path, std::ios::binary | std::ios::trunc);
  if (!out)
    throw std::runtime_error("cannot create presentation output");
  out << "P6\n256 224\n255\n";
  out.write(reinterpret_cast<const char *>(frame.pixels.data()),
            static_cast<std::streamsize>(frame.pixels.size()));
  if (!out)
    throw std::runtime_error("cannot write presentation output");
  return 0;
} catch (const std::exception &e) {
  std::cerr << e.what() << '\n';
  return 1;
}
