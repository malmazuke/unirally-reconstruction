// Render one DRAGSTER shared-engine race state (URDG0001) as the desktop app
// draws it, for presentation comparison against original frame images.
#include "content_pack.hpp"
#include "frontend.hpp"
#include "zoom_zoo_movement.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

int main(int argc, char **argv) try {
  std::filesystem::path pack_path, state_path, out_path;
  for (int index = 1; index + 1 < argc; index += 2) {
    const std::string option = argv[index];
    if (option == "--content-pack") pack_path = argv[index + 1];
    else if (option == "--race-state") state_path = argv[index + 1];
    else if (option == "--out") out_path = argv[index + 1];
    else throw std::invalid_argument("unknown DRAGSTER picture option: " + option);
  }
  if (pack_path.empty() || state_path.empty() || out_path.empty())
    throw std::invalid_argument("usage: dragster_race_picture_runner --content-pack PATH --race-state PATH --out PPM");
  const unirally::ClassicContentPack pack(pack_path);
  std::ifstream input(state_path, std::ios::binary);
  const std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(input), {}};
  const auto race = unirally::deserialize_zoom_zoo(bytes);
  if (race.track != unirally::ClassicRaceTrack::Dragster)
    throw std::invalid_argument("not a DRAGSTER race state");
  const auto position = unirally::app::presentation_position(race.movement.riders[0].motion.x);
  unirally::app::LivePresentation live;
  const auto picture = live.render_dragster_race(race, position, unirally::app::dragster_presentation_content(pack));
  std::ofstream output(out_path, std::ios::binary | std::ios::trunc);
  output << "P6\n256 224\n255\n";
  output.write(reinterpret_cast<const char *>(picture.frame.pixels.data()),
               static_cast<std::streamsize>(picture.frame.pixels.size()));
  std::cout << (picture.used_pose_fallback ? "pose fallback\n" : "recovered poses\n");
  return output ? 0 : 1;
} catch (const std::exception &error) {
  std::cerr << error.what() << '\n';
  return 1;
}
