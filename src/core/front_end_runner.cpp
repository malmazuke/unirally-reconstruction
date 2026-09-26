// FRONT-END-MAIN-MENU laboratory runner: the native front end from power-on, frame by frame.
//
// usage: front_end_runner --content-pack PACK --frames N [--inputs FILE] [--picture FRAME OUT.ppm]...
//                         [--records FRAME OUT.bin]... [--race-initialization FRAME]...
//
// FILE rows are "frame pad1 pad2" (hex controller words, as `$4218`/`$421A` read them); a frame
// without a row has both pads released. Each frame prints one line: the frame, the arrow (spin,
// x, target x, y, target y), the menu (idle, selection, latch), the palette cycle (delay, phase),
// the OAM buffer ($0A00, 544 bytes) and CGRAM, in hex, for comparison with a capture's work RAM.
// `--records` writes the one-player records after that frame as the original keeps them in
// cartridge RAM (8 KiB, `$77:0000`), the words native does not keep left 0.
#include "content_pack.hpp"
#include "front_end.hpp"
#include "zoom_zoo_movement.hpp"
#include "zoom_zoo_pack.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

struct Options {
    std::filesystem::path pack, inputs;
    std::uint32_t frames{};
    std::map<std::uint32_t, std::filesystem::path> pictures, records;
    // The original's race initialization frames, in race order, from a capture: the loading time
    // varies by a frame with the sound program's handshake (R-0039, R-0058).
    std::vector<std::uint32_t> race_initializations;
};

Options parse_options(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        const std::string option = argv[i];
        const auto value = [&] {
            if (i + 1 >= argc)
                throw std::invalid_argument("front-end runner option needs a value: " + option);
            return std::string(argv[++i]);
        };
        if (option == "--content-pack")
            options.pack = value();
        else if (option == "--frames")
            options.frames = static_cast<std::uint32_t>(std::stoul(value()));
        else if (option == "--inputs")
            options.inputs = value();
        else if (option == "--picture") {
            const auto frame = static_cast<std::uint32_t>(std::stoul(value()));
            options.pictures[frame] = value();
        } else if (option == "--race-initialization") {
            options.race_initializations.push_back(static_cast<std::uint32_t>(std::stoul(value())));
        } else if (option == "--records") {
            const auto frame = static_cast<std::uint32_t>(std::stoul(value()));
            options.records[frame] = value();
        } else
            throw std::invalid_argument("unknown front-end runner option: " + option);
    }
    if (options.pack.empty() || options.frames == 0)
        throw std::invalid_argument("usage: front_end_runner --content-pack PACK --frames N "
                                    "[--inputs FILE] [--picture FRAME OUT.ppm]... "
                                    "[--records FRAME OUT.bin]... "
                                    "[--race-initialization FRAME]...");
    return options;
}

std::map<std::uint32_t, unirally::FrontEndPads> read_inputs(const std::filesystem::path& path) {
    std::map<std::uint32_t, unirally::FrontEndPads> rows;
    if (path.empty()) return rows;
    std::ifstream in(path);
    if (!in) throw std::runtime_error("cannot open front-end inputs");
    std::uint32_t frame{};
    std::string one, two;
    while (in >> frame >> one >> two)
        rows[frame] = {static_cast<std::uint16_t>(std::stoul(one, nullptr, 16)),
                       static_cast<std::uint16_t>(std::stoul(two, nullptr, 16))};
    return rows;
}

void write_ppm(const std::filesystem::path& path, const unirally::RgbFrame& frame) {
    std::ofstream out(path, std::ios::binary);
    out << "P6\n256 224\n255\n";
    out.write(reinterpret_cast<const char*>(frame.pixels.data()),
              static_cast<std::streamsize>(frame.pixels.size()));
    if (!out) throw std::runtime_error("cannot write front-end picture");
}

// The records at their cartridge RAM addresses (front_end.hpp's OnePlayerRecords).
void write_records(const std::filesystem::path& path, const unirally::OnePlayerRecords& records) {
    std::array<std::uint8_t, 0x2000> image{};
    const auto put_word = [&](std::size_t at, std::uint16_t value) {
        image[at] = static_cast<std::uint8_t>(value);
        image[at + 1] = static_cast<std::uint8_t>(value >> 8U);
    };
    for (std::size_t k = 0; k < records.tour_levels.size(); ++k)
        image[0x10d3 + k] = records.tour_levels[k];
    for (std::size_t k = 0; k < records.medals.size(); ++k) image[0x069c + k] = records.medals[k];
    for (std::size_t k = 0; k < records.tracks_done.size(); ++k)
        image[0x1075 + k] = records.tracks_done[k];
    for (std::size_t k = 0; k < records.best.size(); ++k) put_word(0x0829 + 2 * k, records.best[k]);
    constexpr std::array<std::size_t, 3> times{0x0422, 0x0486, 0x04ea},
        holders{0x0550, 0x0582, 0x05b4};
    for (std::size_t place = 0; place < 3; ++place)
        for (std::size_t track = 0; track < 50; ++track) {
            put_word(times[place] + 2 * track, records.record_times[place][track]);
            image[holders[place] + track] = records.record_holders[place][track];
        }
    for (std::size_t rider = 0; rider < records.statistics.size(); ++rider)
        for (std::size_t k = 0; k < 4; ++k)
            put_word(0x0230 + 8 * rider + 2 * k, records.statistics[rider][k]);
    put_word(0x0742, records.race_lost ? 0x1000 : 0);
    put_word(0x10a9, records.player_wins);
    put_word(0x10ab, records.opponent_wins);
    put_word(0x1073, records.tries);
    std::ofstream out(path, std::ios::binary);
    out.write(reinterpret_cast<const char*>(image.data()),
              static_cast<std::streamsize>(image.size()));
    if (!out) throw std::runtime_error("cannot write front-end records");
}

template <std::size_t N>
std::string hex(const std::array<std::uint8_t, N>& bytes) {
    static constexpr char digits[] = "0123456789abcdef";
    std::string out;
    out.reserve(N * 2);
    for (const auto byte : bytes) {
        out.push_back(digits[byte >> 4U]);
        out.push_back(digits[byte & 15U]);
    }
    return out;
}

// $008F as the original holds it.
unsigned latch_byte(const unirally::MenuLatches& latches) {
    return (latches.moved ? 1U : 0U) | (latches.up ? 8U : 0U) | (latches.down ? 4U : 0U);
}

// The state after the main menu (R-0055), as `key=value` fields.
void print_screens(const unirally::FrontEndState& state) {
    const auto& slide = state.slide;
    const auto& d = state.decorations;
    const auto& r = state.rider_menu;
    std::cout << "screen=" << unsigned(state.screen) << " script=" << state.script_frame
              << " logo=" << unsigned(state.logo.offset) << " countdown=" << int(slide.countdown)
              << " speed=" << unsigned(slide.speed) << " scroll=" << slide.scroll
              << " hidden=" << slide.hidden_half << " shown=" << slide.shown_half
              << " delay=" << unsigned(d.delay) << " pair=" << unsigned(d.pair_step)
              << " cycle=" << unsigned(d.pair_cycle) << " trio=" << unsigned(d.trio_step)
              << " wave_delay=" << unsigned(d.wave_delay) << " wave=" << hex(d.wave)
              << " sway=" << unsigned(d.sway) << " rider=" << unsigned(r.rider)
              << " row=" << unsigned(r.row) << " up=" << state.latches.up
              << " down=" << state.latches.down << " tour=" << unsigned(state.tour_menu.tour)
              << " cursor=" << unsigned(state.tour_menu.cursor)
              << " track=" << unsigned(state.tour_menu.track)
              << " track_cursor=" << unsigned(state.track_menu.cursor)
              << " opponent=" << unsigned(state.now_playing.opponent)
              << " holder=" << unsigned(state.now_playing.record_holder) << " intro=" << r.intro
              << " idle_step=" << unsigned(r.idle_step) << " text=";
    std::array<std::uint8_t, 2048> text{};
    for (std::size_t k = 0; k < state.text.words.size(); ++k) {
        text[k * 2] = static_cast<std::uint8_t>(state.text.words[k]);
        text[k * 2 + 1] = static_cast<std::uint8_t>(state.text.words[k] >> 8U);
    }
    std::cout << hex(text);
}

// The pads as the race engine takes them.
unirally::ControllerButtons race_buttons(std::uint16_t word) {
    unirally::ControllerButtons buttons{};
    buttons.b = word & 0x8000U;
    buttons.y = word & 0x4000U;
    buttons.select = word & 0x2000U;
    buttons.start = word & 0x1000U;
    buttons.up = word & 0x0800U;
    buttons.down = word & 0x0400U;
    buttons.left = word & 0x0200U;
    buttons.right = word & 0x0100U;
    buttons.a = word & 0x0080U;
    buttons.x = word & 0x0040U;
    buttons.left_shoulder = word & 0x0020U;
    buttons.right_shoulder = word & 0x0010U;
    return buttons;
}

void print_state(std::uint32_t frame, const unirally::FrontEndState& state) {
    const auto& a = state.arrow;
    std::cout << frame << ' ' << unsigned(a.spin) << ' ' << a.x << ' ' << a.target_x << ' ' << a.y
              << ' ' << a.target_y << ' ' << state.menu.idle << ' '
              << unsigned(state.menu.selection) << ' ' << latch_byte(state.latches) << ' '
              << int(state.cycle.delay) << ' ' << int(state.cycle.phase) << ' '
              << hex(state.oam_buffer) << ' ' << hex(state.video.cgram) << ' ';
    print_screens(state);
    std::cout << '\n';
}

// A one-player race between the menus: the native race from its initialization frame, the
// front end resuming when the race's result load begins (R-0057). A race initializes its track's
// loading frames after NOW PLAYING's fade ends (`race_loading_frames`).
struct RaceBetweenMenus {
    std::optional<unirally::ZoomZooContent> content;
    unirally::ZoomZooState state{};
    std::uint32_t initialization_frame{};
    std::uint32_t loading_initialization{}; // the frame the track's loading gives, if measured
};

// Why a race is not run (the run stops there): its loading time on this path is not known, or its
// opponent is not one the race scenarios have. Empty when it starts. A given initialization frame
// (`initialization`, nonzero) takes the place of the loading's.
std::string start_race(RaceBetweenMenus& race, const unirally::ClassicContentPack& pack,
                       const unirally::FrontEndState& front_end, std::uint32_t initialization) {
    const unirally::ClassicRaceTrack track{front_end.tour_menu.track};
    const auto loading_frames = unirally::race_loading_frames(track);
    if (loading_frames == 0 && initialization == 0) return "its loading time is not known";
    // The race scenarios are MIKE's against BRONSEN (R-0046); another opponent is another race.
    constexpr std::uint8_t bronsen = 0x11;
    if (front_end.now_playing.opponent != bronsen)
        return "opponent " + std::to_string(front_end.now_playing.opponent) + " has no scenario";
    race.content = unirally::classic_race_content(pack, track);
    race.state =
        unirally::classic_race_start(*race.content, unirally::classic_race_scenario(track));
    race.loading_initialization = loading_frames ? front_end.frame - 1 + loading_frames : 0;
    race.initialization_frame = initialization != 0 ? initialization : race.loading_initialization;
    race.state.movement.frame = race.initialization_frame;
    return {};
}

} // namespace

int main(int argc, char** argv) try {
    const auto options = parse_options(argc, argv);
    const unirally::ClassicContentPack pack(options.pack);
    const auto content = unirally::front_end_content(pack);
    const auto inputs = read_inputs(options.inputs);
    auto state = unirally::start_front_end();
    RaceBetweenMenus race;
    std::size_t races = 0;
    for (std::uint32_t frame = 0; frame < options.frames; ++frame) {
        const auto row = inputs.find(frame);
        const auto pads = row == inputs.end() ? unirally::FrontEndPads{} : row->second;
        if (state.screen == unirally::FrontEndScreen::race) {
            const auto given = races < options.race_initializations.size()
                                 ? options.race_initializations[races]
                                 : 0;
            if (!race.content) {
                if (const auto why = start_race(race, pack, state, given); !why.empty()) {
                    std::cout << "race " << unsigned(state.tour_menu.track) << " not run: " << why
                              << '\n';
                    break;
                }
            }
            if (frame <= race.initialization_frame) continue;
            unirally::update_zoom_zoo(race.state, race_buttons(pads.one), *race.content);
            if (race.state.result_updates != 1) continue;
            const auto times = unirally::race_times(race.state);
            unirally::return_from_race(state, content, frame, times);
            std::cerr << "race returned at " << frame << "; totals " << times.player_total << '/'
                      << times.opponent_total << "; initialized at " << race.initialization_frame
                      << " (its loading gives " << race.loading_initialization << ")\n";
            race.content.reset();
            ++races;
        } else if (state.mode_chosen) {
            break;
        } else {
            unirally::update_front_end(state, content, pads);
        }
        print_state(frame, state);
        if (const auto picture = options.pictures.find(frame); picture != options.pictures.end())
            write_ppm(picture->second, unirally::render_front_end(state));
        if (const auto at = options.records.find(frame); at != options.records.end())
            write_records(at->second, state.records);
    }
    // A mode chosen; for 1P the race NOW PLAYING chose.
    if (state.mode_chosen)
        std::cout << "mode " << unsigned(state.mode) << " race " << unsigned(state.tour_menu.track)
                  << " rider " << unsigned(state.rider_menu.rider) << " opponent "
                  << unsigned(state.now_playing.opponent) << '\n';
    return 0;
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
}
