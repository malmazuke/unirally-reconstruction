#include "content_pack.hpp"
#include "presentation.hpp"
#include "zoom_zoo_pack.hpp"
#include "zoom_zoo_movement.hpp"
#include <charconv>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>
namespace {
unirally::ZoomZooState load_state(const char* path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::invalid_argument("cannot read state");
    const std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(input), {}};
    return unirally::deserialize_zoom_zoo(bytes);
}
unsigned parse_unsigned(std::string_view text, const char* name) {
    unsigned value{};
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value);
    if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size())
        throw std::invalid_argument(std::string("invalid ") + name);
    return value;
}
// RACE-RIDERS-OPPONENTS: a timeline of another rider's or opponent's race (zoom_zoo_runner
// --rider/--opponent/--tutorial-hints) names its pairing, which the states do not carry.
struct Pairing {
    unirally::RacePairing riders;
    bool tutorial_hints = true;
    std::span<const std::uint8_t> opponent_catch_up;
};
// STUNT-HUD: the player's contact palette by frame (zoom_zoo_runner --contact-palettes), which
// NEON's picture follows and the timeline's states do not carry (R-0068).
using ContactPalettes = std::map<unsigned, std::uint8_t>;

ContactPalettes read_contact_palettes(const char* path) {
    std::ifstream input(path);
    if (!input) throw std::invalid_argument("cannot read the contact palettes");
    ContactPalettes palettes;
    unsigned frame{}, palette{};
    while (input >> frame >> palette) {
        if (palette > 7) throw std::invalid_argument("a contact palette is 0 to 7");
        palettes[frame] = static_cast<std::uint8_t>(palette);
    }
    if (!input.eof()) throw std::invalid_argument("malformed contact palettes");
    return palettes;
}

unirally::ZoomZooState parse_timeline_row(const std::string& line, unsigned& frame,
                                          const std::optional<Pairing>& pairing = {},
                                          const ContactPalettes& palettes = {}) {
    const auto space = line.find(' ');
    if (space == std::string::npos) throw std::invalid_argument("timeline row lacks a state");
    frame = parse_unsigned(std::string_view(line).substr(0, space), "timeline frame");
    const std::string_view hex = std::string_view(line).substr(space + 1);
    if (hex.size() % 2) throw std::invalid_argument("timeline state has odd hex length");
    std::vector<std::uint8_t> bytes(hex.size() / 2);
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        const auto parsed =
            std::from_chars(hex.data() + 2 * i, hex.data() + 2 * i + 2, bytes[i], 16);
        if (parsed.ec != std::errc{} || parsed.ptr != hex.data() + 2 * i + 2)
            throw std::invalid_argument("timeline state is not hexadecimal");
    }
    auto state =
        pairing ? unirally::deserialize_zoom_zoo(bytes, pairing->riders, pairing->tutorial_hints,
                                                 pairing->opponent_catch_up)
                : unirally::deserialize_zoom_zoo(bytes);
    if (state.movement.frame != frame)
        throw std::invalid_argument("timeline row frame differs from its state");
    if (const auto palette = palettes.find(frame); palette != palettes.end())
        state.player_contact_palette = palette->second;
    return state;
}
void write_frame(const char* path, const unirally::RgbFrame& frame) {
    std::ofstream out(path, std::ios::binary);
    out << "P6\n256 224\n255\n";
    out.write(reinterpret_cast<const char*>(frame.pixels.data()),
              static_cast<std::streamsize>(frame.pixels.size()));
    if (!out) throw std::runtime_error("cannot write rendered frame");
}
// Index-level check (R-0040): for every row of a native timeline, the channel-6 window member
// the renderer selects, with the tracked history and without it (as a single restored state
// would), or "-" when channel 6 is disabled for that frame.
int print_window_indices(const char* pack_path, const char* timeline) {
    unirally::ClassicContentPack pack(pack_path);
    std::ifstream input(timeline);
    if (!input) throw std::invalid_argument("cannot read timeline");
    unirally::ClassicRaceHistoryTracker history;
    std::string line;
    unsigned frame{};
    if (!std::getline(input, line)) throw std::invalid_argument("timeline is empty");
    auto previous = parse_timeline_row(line, frame);
    const auto setup = unirally::classic_race_scenario(previous.track).initialization_frame + 6U;
    const auto transition =
        unirally::classic_race_presentation_content(pack, previous.track).window_transition_member;
    // Three columns after the frame: the member the tracker published for
    // the picture (as the app draws), the frame-based selection given the
    // tracked finish frame, and the frame-based selection alone.
    const auto print = [&](const unirally::ZoomZooState& state) {
        const auto on_screen = history.on_screen();
        const auto published = on_screen.window_published ? on_screen.window_table : std::nullopt;
        const auto tracked = unirally::classic_window_table_index(
            state, setup, on_screen.opponent_finish_frame, transition);
        const auto alone =
            unirally::classic_window_table_index(state, setup, std::nullopt, transition);
        const auto show = [](std::optional<unsigned> v) {
            return v ? std::to_string(*v) : std::string("-");
        };
        std::cout << state.movement.frame << ' ' << show(published) << ' ' << show(tracked) << ' '
                  << show(alone) << '\n';
    };
    print(previous);
    while (std::getline(input, line)) {
        auto state = parse_timeline_row(line, frame);
        history.observe_update(previous, state, pack);
        print(state);
        previous = std::move(state);
    }
    return 0;
}

// A one-player state's layout does not carry the riders' look (R-0083), so a replay steps it
// itself, as the race update does, onto each row after the first; two-view states, the one-view
// demo's and league wrappers carry their own. The row's idle latch stays as recorded.
void replay_look(const unirally::ZoomZooState& previous, unirally::ZoomZooState& state,
                 const unirally::ZoomZooContent& engine) {
    if (state.split_screen || state.demo_ai || state.league_statistics.enabled) return;
    state.look = previous.look;
    if (state.result_updates || unirally::zoom_zoo_update_was_paused(previous, state)) return;
    auto stepped = state;
    unirally::advance_rider_look(stepped, engine);
    state.look = stepped.look;
}

// Replays consecutive native states from the timeline's first row so the rider look overlays
// (R-0036) and the opponent's finish frame (R-0040) follow the race, then draws FRAME.
int draw_timeline_frame(const char* pack_path, const char* timeline, const char* frame_text,
                        const char* out, std::optional<Pairing> pairing,
                        const ContactPalettes& palettes) {
    unirally::ClassicContentPack pack(pack_path);
    if (pairing) pairing->opponent_catch_up = pack.optional_entry("race.opponent-catch-up");
    const auto target = parse_unsigned(frame_text, "frame");
    std::ifstream input(timeline);
    if (!input) throw std::invalid_argument("cannot read timeline");
    unirally::ClassicRaceHistoryTracker history;
    std::string line;
    unsigned frame{};
    if (!std::getline(input, line)) throw std::invalid_argument("timeline is empty");
    auto previous = parse_timeline_row(line, frame, pairing, palettes);
    if (!previous.native_initialization
        || frame != unirally::classic_race_scenario(previous.track).initialization_frame)
        throw std::invalid_argument("timeline must begin at the native race initialization");
    if (target <= frame) throw std::invalid_argument("frame must follow the timeline's first row");
    auto content = unirally::classic_race_presentation_content(
        pack, pairing ? unirally::classic_race_scenario(previous.track, pairing->riders,
                                                        pairing->tutorial_hints)
                      : unirally::classic_race_scenario(previous.track));
    // These timelines are races on their own, whose pause menu restarts.
    content.pause_second_choice =
        unirally::ClassicRacePresentationContent::PauseSecondChoice::restart;
    const auto engine = unirally::classic_race_content(pack, previous.track);
    while (std::getline(input, line)) {
        const auto previous_frame = frame;
        auto state = parse_timeline_row(line, frame, pairing, palettes);
        if (frame != previous_frame + 1U)
            throw std::invalid_argument("timeline rows must be consecutive");
        replay_look(previous, state, engine);
        history.observe_update(previous, state, pack);
        if (frame == target) {
            const auto on_screen = history.on_screen();
            write_frame(out, unirally::render_classic_race(state, content, &previous, &on_screen));
            return 0;
        }
        previous = std::move(state);
    }
    throw std::invalid_argument("frame is beyond the timeline");
}

// One state, drawn as the app draws it.
int draw_state(const char* pack_path, const char* state_path, const char* out,
               const char* previous_path) {
    unirally::ClassicContentPack pack(pack_path);
    const auto state = load_state(state_path);
    // The HUD and riders show the previous update; without PREVIOUS_STATE
    // they are drawn one update ahead. A single state carries no rider look
    // history, so the upper-body overlays are omitted here, an opponent-won
    // banner is drawn only once both finish times are known, and the clock
    // digits are derived from the state rather than followed, which differs
    // from the original on the pictures after an update that wrote the left
    // field (R-0043). This debug form is the only caller without the
    // history: the app and the --timeline form above both keep the tracker.
    const auto previous = previous_path ? load_state(previous_path) : state;
    auto content = unirally::classic_race_presentation_content(pack, state.track);
    content.pause_second_choice =
        unirally::ClassicRacePresentationContent::PauseSecondChoice::restart;
    write_frame(out, unirally::render_classic_race(state, content, &previous));
    return 0;
}

// PACK --timeline TIMELINE FRAME OUT [--pairing RIDER OPPONENT HINTS] [--contact-palettes FILE]
int draw_timeline_frame_with_options(int argc, char** argv) {
    std::optional<Pairing> pairing;
    ContactPalettes palettes;
    for (int i = 6; i < argc;) {
        const std::string_view option = argv[i];
        if (option == "--pairing" && i + 3 < argc && !pairing) {
            pairing = Pairing{{static_cast<std::uint8_t>(parse_unsigned(argv[i + 1], "rider")),
                               static_cast<std::uint8_t>(parse_unsigned(argv[i + 2], "opponent"))},
                              parse_unsigned(argv[i + 3], "tutorial hints") != 0,
                              {}};
            i += 4;
        } else if (option == "--contact-palettes" && i + 1 < argc && palettes.empty()) {
            palettes = read_contact_palettes(argv[i + 1]);
            i += 2;
        } else {
            throw std::invalid_argument("unknown or repeated timeline option");
        }
    }
    return draw_timeline_frame(argv[1], argv[3], argv[4], argv[5], pairing, palettes);
}

} // namespace

int main(int argc, char** argv) try {
    // One picture of either track's shared race state, drawn as the app draws it.
    if (argc == 4 && std::string_view(argv[2]) == "--window-index")
        return print_window_indices(argv[1], argv[3]);
    if (argc >= 6 && std::string_view(argv[2]) == "--timeline")
        return draw_timeline_frame_with_options(argc, argv);
    if (argc != 4 && argc != 5)
        throw std::invalid_argument(
            "usage: classic_race_presentation_runner PACK STATE OUT.ppm [PREVIOUS_STATE]\n"
            "       classic_race_presentation_runner PACK --timeline NATIVE_TIMELINE FRAME "
            "OUT.ppm [--pairing RIDER OPPONENT TUTORIAL_HINTS] [--contact-palettes FILE]\n"
            "       classic_race_presentation_runner PACK --window-index NATIVE_TIMELINE");
    return draw_state(argv[1], argv[2], argv[3], argc == 5 ? argv[4] : nullptr);
} catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
}
