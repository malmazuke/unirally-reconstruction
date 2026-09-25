// FRONT-END-MAIN-MENU laboratory runner: the native front end from power-on, frame by frame.
//
// usage: front_end_runner --content-pack PACK --frames N [--inputs FILE] [--picture FRAME OUT.ppm]...
//
// FILE rows are "frame pad1 pad2" (hex controller words, as `$4218`/`$421A` read them); a frame
// without a row has both pads released. Each frame prints one line: the frame, the arrow (spin,
// x, target x, y, target y), the menu (idle, selection, latch), the palette cycle (delay, phase),
// the OAM buffer ($0A00, 544 bytes) and CGRAM, in hex, for comparison with a capture's work RAM.
#include "content_pack.hpp"
#include "front_end.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

struct Options {
    std::filesystem::path pack, inputs;
    std::uint32_t frames{};
    std::map<std::uint32_t, std::filesystem::path> pictures;
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
        } else
            throw std::invalid_argument("unknown front-end runner option: " + option);
    }
    if (options.pack.empty() || options.frames == 0)
        throw std::invalid_argument("usage: front_end_runner --content-pack PACK --frames N "
                                    "[--inputs FILE] [--picture FRAME OUT.ppm]...");
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

} // namespace

int main(int argc, char** argv) try {
    const auto options = parse_options(argc, argv);
    const unirally::ClassicContentPack pack(options.pack);
    const auto content = unirally::front_end_content(pack);
    const auto inputs = read_inputs(options.inputs);
    auto state = unirally::start_front_end();
    for (std::uint32_t frame = 0; frame < options.frames && !state.mode_chosen; ++frame) {
        const auto row = inputs.find(frame);
        unirally::update_front_end(state, content,
                                   row == inputs.end() ? unirally::FrontEndPads{} : row->second);
        const auto& a = state.arrow;
        std::cout << frame << ' ' << unsigned(a.spin) << ' ' << a.x << ' ' << a.target_x << ' '
                  << a.y << ' ' << a.target_y << ' ' << state.menu.idle << ' '
                  << unsigned(state.menu.selection) << ' ' << latch_byte(state.latches) << ' '
                  << int(state.cycle.delay) << ' ' << int(state.cycle.phase) << ' '
                  << hex(state.oam_buffer) << ' ' << hex(state.video.cgram) << ' ';
        print_screens(state);
        std::cout << '\n';
        if (const auto picture = options.pictures.find(frame); picture != options.pictures.end())
            write_ppm(picture->second, unirally::render_front_end(state));
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
