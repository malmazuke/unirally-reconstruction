// A bounded native demo race from the accepted ZOOM ZOO initializer. Picture
// labels are the PAL cold-start frames, 72 after the historical race clock.
#include "content_pack.hpp"
#include "presentation.hpp"
#include "race_camera.hpp"
#include "zoom_zoo_movement.hpp"
#include "zoom_zoo_pack.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
constexpr unsigned frame_offset = 72;
void write_picture(const std::filesystem::path& path, const unirally::RgbFrame& frame) {
    std::ofstream out(path, std::ios::binary);
    if (!out) throw std::runtime_error("cannot create split demo picture");
    out << "P6\n256 224\n255\n";
    out.write(reinterpret_cast<const char*>(frame.pixels.data()),
              static_cast<std::streamsize>(frame.pixels.size()));
    if (!out) throw std::runtime_error("cannot write split demo picture");
}

void check_split_save_words(const std::vector<std::uint8_t>& saved) {
    const auto restored = unirally::deserialize_zoom_zoo(saved);
    if (unirally::serialize_zoom_zoo(restored) != saved)
        throw std::runtime_error("split state did not round-trip");
    // The trailer has camera words, then four pairs of controller words, and ends with the
    // riders' 46-byte look block (R-0083). An all-ones value is impossible for every controller
    // word in either layout.
    constexpr unsigned look_block = 46;
    for (const unsigned from_end : {30U, 28U, 26U, 24U, 22U, 20U, 18U, 16U}) {
        auto impossible = saved;
        impossible[impossible.size() - look_block - from_end] = 0xff;
        impossible[impossible.size() - look_block - from_end + 1] = 0xff;
        bool refused = false;
        try {
            (void)unirally::deserialize_zoom_zoo(impossible);
        } catch (const std::invalid_argument&) {
            refused = true;
        }
        if (!refused) throw std::runtime_error("split state accepted impossible controller word");
    }
    for (const unsigned from_end : {26U, 24U}) {
        auto impossible = saved;
        impossible[impossible.size() - look_block - from_end] = 49;
        impossible[impossible.size() - look_block - from_end + 1] = 0;
        bool refused = false;
        try {
            (void)unirally::deserialize_zoom_zoo(impossible);
        } catch (const std::invalid_argument&) {
            refused = true;
        }
        if (!refused) throw std::runtime_error("split state accepted impossible rotation window");
    }
}
} // namespace

int main(int argc, char** argv) try {
    std::filesystem::path pack_path;
    std::map<unsigned, std::filesystem::path> pictures;
    std::optional<unsigned> restore_frame;
    for (int i = 1; i < argc; ++i) {
        const std::string option = argv[i];
        if (option == "--content-pack" && i + 1 < argc)
            pack_path = argv[++i];
        else if (option == "--picture" && i + 2 < argc) {
            const unsigned frame = static_cast<unsigned>(std::stoul(argv[++i]));
            pictures[frame] = argv[++i];
        } else if (option == "--restore-check" && i + 1 < argc) {
            restore_frame = static_cast<unsigned>(std::stoul(argv[++i]));
        } else
            throw std::invalid_argument("split_demo_runner: bad option");
    }
    if (pack_path.empty() || pictures.empty() || pictures.begin()->first <= 1448)
        throw std::invalid_argument("split_demo_runner needs a pack and pictures after 1448");
    unirally::ClassicContentPack pack(pack_path);
    auto scenario = unirally::classic_race_scenario(unirally::ClassicRaceTrack::ZoomZoo);
    const auto content = unirally::classic_race_content(pack, scenario.track);
    auto state = unirally::classic_race_start(content, scenario);
    unirally::initialize_split_cameras(state);
    state.demo_ai = state.opponent_hints.active = true;
    state.pairing = {4, 14}; // $77:0748/$77:0749 at the frame-1448 boundary.
    state.opponent_tier.ai_level = 0;
    scenario.pairing = state.pairing;
    const auto picture_content = unirally::classic_race_presentation_content(pack, scenario);
    unirally::ClassicRaceHistoryTracker history;
    std::optional<unirally::ZoomZooState> restored;
    const unsigned last = pictures.rbegin()->first - frame_offset;
    while (state.movement.frame < last) {
        auto previous = state;
        unirally::update_zoom_zoo(state, {}, content);
        const auto original_frame = state.movement.frame + frame_offset;
        if (restore_frame == original_frame) {
            const auto saved = unirally::serialize_zoom_zoo(state);
            if (saved[7] != 'F') throw std::runtime_error("expected demo split layout F");
            check_split_save_words(saved);
            auto extended = state;
            extended.special_tiles[0].mud_cooldown = 1;
            const auto extended_saved = unirally::serialize_zoom_zoo(extended);
            if (extended_saved[7] != 'G')
                throw std::runtime_error("expected extended demo split layout G");
            check_split_save_words(extended_saved);
            restored = unirally::deserialize_zoom_zoo(saved);
        } else if (restored) {
            unirally::update_zoom_zoo(*restored, {}, content);
            if (unirally::serialize_zoom_zoo(*restored) != unirally::serialize_zoom_zoo(state))
                throw std::runtime_error("split restored continuation diverged at "
                                         + std::to_string(original_frame));
        }
        history.observe_update(previous, state, pack);
        if (const auto picture = pictures.find(original_frame); picture != pictures.end()) {
            const auto on_screen = history.on_screen();
            write_picture(picture->second, unirally::render_classic_race(state, picture_content,
                                                                         &previous, &on_screen));
        }
    }
    return 0;
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
}
