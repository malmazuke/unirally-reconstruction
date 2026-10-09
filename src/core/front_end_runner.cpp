// FRONT-END-MAIN-MENU laboratory runner: the native front end from power-on, frame by frame.
//
// usage: front_end_runner --content-pack PACK --frames N [--inputs FILE] [--picture FRAME OUT.ppm]...
//                         [--records FRAME OUT.bin]... [--cartridge-in IMAGE.bin]
//                         [--race-initialization FRAME]...
//                         [--reset-upload-delay FRAMES]...
//                         [--record-write FRAME OFFSET BYTE]...
//
// FILE rows are "frame pad1 pad2" (hex controller words, as `$4218`/`$421A` read them); a frame
// without a row has both pads released. Each frame prints one line: the frame, the arrow (spin,
// x, target x, y, target y), the menu (idle, selection, latch), the palette cycle (delay, phase),
// the OAM buffer ($0A00, 544 bytes) and CGRAM, in hex, for comparison with a capture's work RAM.
// `--records` writes the cartridge RAM after that frame as native keeps it (8 KiB, `$77:0000`,
// cartridge_ram.hpp); `--cartridge-in` is the image power-on finds (SAVE-FILES).
// `--record-write` sets one record byte at its cartridge RAM offset (hex): a name
// `$77:000C-016B`, done track `$77:1075-10A6` or medal `$77:069C-073B`.
// Writes happen after FRAME, as in the bounded reference interventions (R-0065).
#include "cartridge_ram.hpp"
#include "content_pack.hpp"
#include "front_end.hpp"
#include "presentation.hpp"
#include "race_camera.hpp"
#include "race_sound.hpp"
#include "zoom_zoo_movement.hpp"
#include "zoom_zoo_pack.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <optional>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace {

struct Options {
    std::filesystem::path pack, inputs;
    std::filesystem::path race_timeline, look_timeline, sound_cues;
    std::uint32_t frames{};
    std::map<std::uint32_t, std::filesystem::path> pictures, records, vram;
    // The original's race initialization frames, in race order, from a capture: the loading time
    // varies by a frame with the sound program's handshake (R-0039, R-0058).
    std::vector<std::uint32_t> race_initializations;
    // After each soft reset in turn, how much later the sound program's upload ends than at
    // power-on (HUNTER-ENDING); native's own is 3.
    std::vector<std::uint32_t> reset_upload_delays;
    // Idle demo cycles (1 = the first) whose title loads a frame sooner in a capture (R-0087).
    std::vector<std::uint32_t> short_demo_titles;
    std::optional<std::uint32_t> human_after;
    std::optional<std::uint32_t> restore_check;
    std::filesystem::path cartridge_in; // a saved cartridge RAM image the boot finds
    // Record bytes written after a frame: the frame, the cartridge RAM offset and the byte.
    std::vector<std::tuple<std::uint32_t, std::uint32_t, std::uint8_t>> record_writes;
};

// The record byte native keeps for a cartridge RAM offset.
std::uint8_t& record_byte(unirally::OnePlayerRecords& records, std::uint32_t offset) {
    if (offset >= 0x000c && offset < 0x000c + records.rider_names.size())
        return records.rider_names[offset - 0x000c];
    if (offset >= 0x1075 && offset < 0x1075 + records.tracks_done.size())
        return records.tracks_done[offset - 0x1075];
    if (offset >= 0x069c && offset < 0x069c + records.medals.size())
        return records.medals[offset - 0x069c];
    if (offset >= 0x10d3 && offset < 0x10d3 + records.tour_levels.size())
        return records.tour_levels[offset - 0x10d3];
    if (offset >= 0x0678 && offset < 0x067e) return records.league_pair_cursor[offset - 0x0678];
    if (offset >= 0x067e && offset < 0x0684) return records.league_tracks[offset - 0x067e];
    if (offset >= 0x05e8 && offset < 0x0618) {
        const auto index = offset - 0x05e8;
        return records.league_pairings[index / 8][index % 8];
    }
    constexpr std::array<std::uint32_t, 3> holders{0x0550, 0x0582, 0x05b4};
    for (std::size_t rank = 0; rank < holders.size(); ++rank)
        if (offset >= holders[rank] && offset < holders[rank] + 50)
            return records.record_holders[rank][offset - holders[rank]];
    throw std::invalid_argument("--record-write: unsupported record offset");
}

void set_record_byte(unirally::OnePlayerRecords& records, std::uint32_t offset, std::uint8_t byte) {
    const auto write_word = [&](std::uint16_t& word, std::uint32_t base) {
        if (offset == base)
            word = static_cast<std::uint16_t>((word & 0xff00U) | byte);
        else
            word =
                static_cast<std::uint16_t>((word & 0x00ffU) | (static_cast<unsigned>(byte) << 8U));
    };
    if (offset >= 0x0230 && offset < 0x02b0) {
        const auto index = offset - 0x0230;
        write_word(records.statistics[index / 8][(index % 8) / 2], offset & ~1U);
        return;
    }
    if (offset >= 0x02c0 && offset < 0x02c0 + 32 * records.league_scores.size()) {
        const auto index = offset - 0x02c0;
        auto& value = index % 4 >= 2 ? records.league_scores[index / 32][(index % 32) / 4]
                                     : records.league_played[index / 32][(index % 32) / 4];
        write_word(value, offset & ~1U);
        return;
    }
    if (offset >= 0x0829 && offset < 0x0829 + records.best.size() * 2) {
        const auto index = offset - 0x0829;
        write_word(records.best[index / 2], 0x0829 + (index & ~1U));
        return;
    }
    constexpr std::array<std::uint32_t, 3> times{0x0422, 0x0486, 0x04ea};
    for (std::size_t rank = 0; rank < times.size(); ++rank)
        if (offset >= times[rank] && offset < times[rank] + 100) {
            const auto index = offset - times[rank];
            write_word(records.record_times[rank][index / 2], times[rank] + (index & ~1U));
            return;
        }
    record_byte(records, offset) = byte;
}

// A whole unsigned number in the base, at most the limit; refuses a sign, trailing text or overflow.
std::uint32_t parse_number(const std::string& text, int base, std::uint32_t limit,
                           const std::string& what) {
    std::size_t end = 0;
    unsigned long value = 0;
    try {
        if (!text.empty() && text.front() != '-' && text.front() != '+')
            value = std::stoul(text, &end, base);
    } catch (const std::exception&) {
        end = 0;
    }
    if (end == 0 || end != text.size() || value > limit)
        throw std::invalid_argument("--record-write: bad " + what + ": " + text);
    return static_cast<std::uint32_t>(value);
}

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
        else if (option == "--race-timeline")
            options.race_timeline = value();
        else if (option == "--look-timeline")
            options.look_timeline = value();
        else if (option == "--sound-cues")
            options.sound_cues = value();
        else if (option == "--picture") {
            const auto frame = static_cast<std::uint32_t>(std::stoul(value()));
            options.pictures[frame] = value();
        } else if (option == "--race-initialization") {
            options.race_initializations.push_back(static_cast<std::uint32_t>(std::stoul(value())));
        } else if (option == "--short-demo-title") {
            options.short_demo_titles.push_back(static_cast<std::uint32_t>(std::stoul(value())));
        } else if (option == "--reset-upload-delay") {
            options.reset_upload_delays.push_back(static_cast<std::uint32_t>(std::stoul(value())));
        } else if (option == "--record-write") {
            const auto frame = parse_number(value(), 10, UINT32_MAX - 1, "frame");
            const auto offset = parse_number(value(), 16, 0x1fff, "offset");
            const auto byte = static_cast<std::uint8_t>(parse_number(value(), 16, 0xff, "byte"));
            unirally::OnePlayerRecords check;
            set_record_byte(check, offset, byte); // refuses an offset native keeps no record for
            options.record_writes.emplace_back(frame, offset, byte);
        } else if (option == "--cartridge-in") {
            options.cartridge_in = value();
        } else if (option == "--records") {
            const auto frame = static_cast<std::uint32_t>(std::stoul(value()));
            options.records[frame] = value();
        } else if (option == "--vram") {
            const auto frame = static_cast<std::uint32_t>(std::stoul(value()));
            options.vram[frame] = value();
        } else if (option == "--human-after") {
            options.human_after = static_cast<std::uint32_t>(std::stoul(value()));
        } else if (option == "--restore-check") {
            options.restore_check = static_cast<std::uint32_t>(std::stoul(value()));
        } else
            throw std::invalid_argument("unknown front-end runner option: " + option);
    }
    if (options.pack.empty() || options.frames == 0)
        throw std::invalid_argument("usage: front_end_runner --content-pack PACK --frames N "
                                    "[--inputs FILE] [--picture FRAME OUT.ppm]... "
                                    "[--records FRAME OUT.bin]... [--cartridge-in IMAGE.bin] "
                                    "[--race-initialization FRAME]... "
                                    "[--human-after FRAME] "
                                    "[--restore-check FRAME] "
                                    "[--reset-upload-delay FRAMES]... "
                                    "[--record-write FRAME OFFSET BYTE]...");
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

// Power-on, with the cartridge RAM `--cartridge-in` gives (SAVE-FILES).
unirally::FrontEndState start_state(const Options& options) {
    auto state = unirally::start_front_end();
    if (options.cartridge_in.empty()) return state;
    std::ifstream in(options.cartridge_in, std::ios::binary);
    if (!in)
        throw std::runtime_error("cannot read --cartridge-in " + options.cartridge_in.string());
    const std::vector<std::uint8_t> image((std::istreambuf_iterator<char>(in)),
                                          std::istreambuf_iterator<char>());
    unirally::insert_cartridge(state, image);
    return state;
}

// The cartridge RAM as native keeps it (cartridge_ram.hpp).
void write_records(const std::filesystem::path& path, const unirally::FrontEndState& state) {
    const auto image = unirally::cartridge_image(state);
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
    std::optional<unirally::ClassicRacePresentationContent> presentation;
    unirally::ClassicRaceHistoryTracker history;
    unirally::ZoomZooState state{};
    std::uint32_t initialization_frame{};
    std::uint32_t loading_initialization{}; // the frame the track's loading gives, if measured
    // An idle demo restored from its own serialized state at `--restore-check`, run alongside.
    std::optional<unirally::ZoomZooState> restored_demo;
};

// Why a race is not run (the run stops there): its loading time on this path is not known. Empty
// when it starts. A given initialization frame (`initialization`, nonzero) takes the place of the
// loading's. The rider and opponent change no loading time (R-0061).
std::string start_race(RaceBetweenMenus& race, const unirally::ClassicContentPack& pack,
                       const unirally::FrontEndState& front_end, std::uint32_t initialization) {
    if (front_end.mode == unirally::FrontEndMode::demo) {
        const bool split = front_end.demo_split_race;
        auto scenario =
            unirally::classic_race_scenario(unirally::ClassicRaceTrack{front_end.tour_menu.track});
        scenario.pairing = {front_end.rider_menu.rider, front_end.now_playing.opponent};
        // The demo's setup skips the race count (`$83:C9F6-CA05`): `$77:10B1` as it stands.
        scenario.race_counter = front_end.records.race_song_counter;
        // Labelled by its absolute frame: a demo state's clock gives its start (R-0087).
        scenario.initialization_frame = front_end.frame - 1U;
        race.content = unirally::classic_race_content(pack, scenario.track);
        race.state = unirally::classic_race_start(*race.content, scenario);
        if (split)
            unirally::initialize_split_cameras(race.state);
        else
            unirally::initialize_second_camera(race.state);
        race.state.demo_ai = true;
        // Rider 1's hints run in the demo, but never for MIKE (rider 0, R-0082, R-0087).
        race.state.opponent_hints.active = race.state.pairing.opponent != 0;
        if (split) race.state.opponent_tier.ai_level = 0;
        race.presentation = unirally::classic_race_presentation_content(pack, scenario);
        race.presentation->rider_names = front_end.records.rider_names;
        race.initialization_frame = initialization ? initialization : front_end.frame - 1U;
        race.loading_initialization = front_end.frame - 1U;
        race.history = {};
        return {};
    }
    const bool local =
        front_end.mode == unirally::FrontEndMode::two_player
        || front_end.mode == unirally::FrontEndMode::versus
        || (front_end.mode == unirally::FrontEndMode::league && front_end.second_rider < 16);
    auto scenario =
        local ? unirally::classic_local_race_scenario(
                    unirally::ClassicRaceTrack{front_end.tour_menu.track},
                    {front_end.rider_menu.rider, front_end.second_rider},
                    ((front_end.records.tutorial_bits >> front_end.rider_menu.rider) & 1U) == 0,
                    ((front_end.records.tutorial_bits >> front_end.second_rider) & 1U) == 0)
              : unirally::one_player_race_scenario(front_end);
    scenario.race_counter = front_end.race_song; // `$77:10B1` after this race's count (R-0084)
    const auto loading_frames = unirally::race_loading_frames(front_end);
    if (loading_frames == 0 && initialization == 0) return "its loading time is not known";
    race.content = unirally::classic_race_content(pack, scenario.track);
    race.state = unirally::classic_race_start(*race.content, scenario);
    race.state.league_statistics.enabled = front_end.mode == unirally::FrontEndMode::league;
    race.state.versus = front_end.mode == unirally::FrontEndMode::versus;
    if (local) unirally::initialize_split_cameras(race.state);
    race.presentation = unirally::classic_race_presentation_content(pack, scenario);
    race.presentation->rider_names = front_end.records.rider_names;
    race.history = {};
    race.loading_initialization = loading_frames ? front_end.frame - 1 + loading_frames : 0;
    // The race keeps its scenario's frame label, which its own clocks count from; the runner
    // lines its first update up with the menus' frame after `initialization_frame`.
    race.initialization_frame = initialization != 0 ? initialization : race.loading_initialization;
    return {};
}

// One front-end frame, first marking a capture's idle-demo title that loaded a frame sooner
// (`--short-demo-title`, R-0087).
void update_front_end(const Options& options, unirally::FrontEndState& state,
                      const unirally::FrontEndContent& content, unirally::FrontEndPads pads) {
    const auto& titles = options.short_demo_titles;
    // The first title has no hold to shorten (R-0088).
    if (state.screen == unirally::FrontEndScreen::demo_title && state.demo_cycles != 0
        && state.script_frame < unirally::demo_title_hold_frame
        && std::find(titles.begin(), titles.end(), state.demo_cycles + 1U) != titles.end())
        state.demo_title_short = true;
    unirally::update_front_end(state, content, pads);
}

void update_demo_race(const Options& options, const unirally::ClassicContentPack& pack,
                      unirally::FrontEndState& front_end, RaceBetweenMenus& race,
                      std::uint32_t frame, unirally::FrontEndPads pads, std::size_t& races) {
    if (options.human_after && frame >= *options.human_after) race.state.demo_ai = false;
    const auto previous = race.state;
    unirally::update_zoom_zoo(race.state, race_buttons(pads.one), race_buttons(pads.two),
                              *race.content);
    // A restore check: the state saved on its frame reads back to the same bytes, and the
    // restored race continues in step with the running one until the demo ends.
    if (race.restored_demo) {
        unirally::update_zoom_zoo(*race.restored_demo, race_buttons(pads.one),
                                  race_buttons(pads.two), *race.content);
        if (unirally::serialize_zoom_zoo(*race.restored_demo)
            != unirally::serialize_zoom_zoo(race.state))
            throw std::runtime_error("demo save continuation diverged at " + std::to_string(frame));
    } else if (options.restore_check == frame) {
        const auto saved = unirally::serialize_zoom_zoo(race.state);
        race.restored_demo = unirally::deserialize_zoom_zoo(saved);
        if (unirally::serialize_zoom_zoo(*race.restored_demo) != saved)
            throw std::runtime_error("demo save failed round-trip");
    }
    race.history.observe_update(previous, race.state, pack);
    if (const auto picture = options.pictures.find(frame); picture != options.pictures.end()) {
        const auto shown = race.history.on_screen();
        write_ppm(picture->second,
                  unirally::render_classic_race(race.state, *race.presentation, &previous, &shown));
    }
    if (race.state.demo.exit_requested) {
        if (race.restored_demo)
            std::cerr << "demo restore continued to the exit at " << frame << '\n';
        race.restored_demo.reset();
        unirally::return_from_demo(front_end, frame, race.state.demo.elapsed);
        race.content.reset();
        race.presentation.reset();
        ++races;
    }
    print_state(frame, front_end);
}

// The two riders' look words (R-0036) as the picture history holds them after a two-pad race's
// update: per rider $0D49 head, $1259 target, $1269, $126D, $1271, $0D5F, $0D63, $0D67, $125D and
// $0D6F, as hex words.
void write_look(std::ofstream& out, std::uint32_t frame, const unirally::RiderLookState& look) {
    if (!out.is_open()) return;
    out << frame;
    for (const auto& r : look.riders)
        for (const auto word :
             {r.head, r.target, r.looking_back, r.distance_step, r.glance_timer, r.sequence_cursor,
              r.sequence_end, r.sequence_delay, r.sequence_target, r.sequence_number})
            out << ' ' << std::hex << word << std::dec;
    out << '\n';
}

void write_race_state(std::ofstream& out, std::uint32_t frame,
                      const unirally::ZoomZooState& state) {
    if (!out.is_open()) return;
    out << frame << ' ';
    for (const auto byte : unirally::serialize_zoom_zoo(state)) {
        constexpr char hex[] = "0123456789abcdef";
        out << hex[byte >> 4U] << hex[byte & 15U];
    }
    out << '\n';
}

// Check both local save layouts, the AI guard and the frame after restoration.
void check_local_restore(const unirally::ZoomZooState& race_state,
                         std::optional<unirally::ZoomZooState>& restored) {
    if (race_state.league_statistics.enabled) {
        const auto saved = unirally::serialize_zoom_zoo(race_state);
        restored = unirally::deserialize_zoom_zoo(saved);
        if (unirally::serialize_zoom_zoo(*restored) != saved)
            throw std::runtime_error("league save failed round-trip");
        return;
    }
    const auto saved = unirally::serialize_zoom_zoo(race_state);
    // A VS race appends its banner drivers after the trailer (R-0081), pad 2's open pause menu
    // one byte after them (R-0079).
    const auto lower_view = race_state.pause.lower_view ? 1U : 0U;
    const auto versus = race_state.versus ? 8U : 0U;
    // DRAGSTER's two-pad layouts are H and I, ZOOM ZOO's F and G.
    const bool dragster = race_state.track == unirally::ClassicRaceTrack::Dragster;
    if (saved.size() != 831 + versus + lower_view || saved[7] != (dragster ? 'H' : 'F'))
        throw std::runtime_error("local save is not layout H or F");
    restored = unirally::deserialize_zoom_zoo(saved);
    if (unirally::serialize_zoom_zoo(*restored) != saved)
        throw std::runtime_error("local DRAGSTER save did not round-trip");
    const auto refused = [](std::span<const std::uint8_t> bytes) {
        try {
            (void)unirally::deserialize_zoom_zoo(bytes);
        } catch (const std::invalid_argument&) {
            return true;
        }
        return false;
    };
    // The split flag and demo_ai come before the trailer's look block (R-0083). A ZOOM ZOO
    // two-view state may be the split demo's (R-0069), so only DRAGSTER refuses demo AI; any
    // two-view state refuses a clear split flag.
    constexpr std::size_t look_block = 46 + 1; // the look, then the race counter (R-0084)
    const auto trailer_end = saved.size() - lower_view - versus;
    auto invalid = saved;
    invalid[trailer_end - look_block - 1] = 1;
    if (dragster && !refused(invalid)) throw std::runtime_error("local save accepted demo AI");
    invalid = saved;
    invalid[trailer_end - look_block - 2] = 0;
    if (!refused(invalid)) throw std::runtime_error("local save accepted a clear split flag");
    if (lower_view) {
        invalid = saved;
        invalid.back() = 2;
        if (!refused(invalid)) throw std::runtime_error("local save accepted a lower view of 2");
    }
    if (versus) {
        invalid = saved;
        invalid[trailer_end] = 6; // rider 0's banner index, first in the VS block: 0 or 7..24
        if (!refused(invalid)) throw std::runtime_error("local save accepted a banner index of 6");
    }
    auto extended = race_state;
    extended.special_tiles[0].mud_cooldown = 1;
    const auto extended_saved = unirally::serialize_zoom_zoo(extended);
    if (extended_saved.size() != 883 + versus + lower_view
        || extended_saved[7] != (dragster ? 'I' : 'G')
        || unirally::serialize_zoom_zoo(unirally::deserialize_zoom_zoo(extended_saved))
               != extended_saved)
        throw std::runtime_error("extended local DRAGSTER save failed round-trip");
}

// AUDIO-FIRST-RACE: each frame's sound queue work, one cue a line ("FRAME E COMMAND PARAMETER",
// "FRAME D SITE", "FRAME L race-RESOURCE tTRACK|title|award|ending|title-award"), rotation cues
// resolved through the audio side's latches
// so that the lines compare with the original's enqueue and dispatch watches (R-0076).
class SoundCueLog {
public:
    explicit SoundCueLog(const std::filesystem::path& path) {
        if (path.empty()) return;
        out_.open(path);
        if (!out_) throw std::runtime_error("cannot create sound cue log");
    }
    void write(std::uint32_t frame, const unirally::AudioCueList& cues) {
        if (!out_.is_open()) return;
        for (const auto& cue : cues) switch (cue.kind) {
            case unirally::AudioCueKind::enqueue:
                out_ << frame << " E " << unsigned(cue.command) << ' ' << unsigned(cue.parameter)
                     << '\n';
                break;
            case unirally::AudioCueKind::dispatch:
                out_ << frame << " D " << unirally::audio_dispatch_site_name(cue.site) << '\n';
                break;
            case unirally::AudioCueKind::load:
                rotating_ = {};
                out_ << frame << " L ";
                if (cue.load == unirally::AudioSessionLoad::race)
                    out_ << "race-" << unsigned(unirally::race_song_resource(cue.parameter)) << " t"
                         << unsigned(cue.command);
                else
                    out_ << unirally::audio_session_name(cue.load);
                out_ << '\n';
                break;
            case unirally::AudioCueKind::rotation:
                if (rotating_[cue.command] == (cue.parameter != 0)) break;
                rotating_[cue.command] = cue.parameter != 0;
                out_ << frame << " E " << (cue.parameter ? 11 : 6) << ' ' << 42 + cue.command
                     << '\n'
                     << frame << " E 2 12\n";
                break;
            }
    }

private:
    std::ofstream out_;
    std::array<bool, 2> rotating_{};
};

void write_frame_outputs(const Options& options, std::uint32_t frame,
                         const unirally::FrontEndState& state) {
    print_state(frame, state);
    if (const auto picture = options.pictures.find(frame); picture != options.pictures.end())
        write_ppm(picture->second, unirally::render_front_end(state));
    if (const auto at = options.records.find(frame); at != options.records.end())
        write_records(at->second, state);
    if (const auto at = options.vram.find(frame); at != options.vram.end()) {
        std::ofstream out(at->second, std::ios::binary);
        out.write(reinterpret_cast<const char*>(state.video.vram.data()),
                  static_cast<std::streamsize>(state.video.vram.size()));
    }
}

void update_local_race(const Options& options, const unirally::ClassicContentPack& pack,
                       const unirally::FrontEndContent& content, unirally::FrontEndState& front_end,
                       RaceBetweenMenus& race, std::ofstream& race_timeline,
                       std::ofstream& look_timeline,
                       std::optional<unirally::ZoomZooState>& restored_local, std::size_t& races,
                       std::uint32_t frame, unirally::FrontEndPads pads) {
    const auto previous = race.state;
    const auto over = unirally::update_race_for_menus(race.state, race_buttons(pads.one),
                                                      race_buttons(pads.two), *race.content);
    const bool local =
        front_end.mode == unirally::FrontEndMode::two_player
        || front_end.mode == unirally::FrontEndMode::versus
        || (front_end.mode == unirally::FrontEndMode::league && front_end.second_rider < 16);
    if ((local || front_end.mode == unirally::FrontEndMode::league)
        && options.restore_check == frame) {
        check_local_restore(race.state, restored_local);
    } else if (restored_local) {
        (void)unirally::update_race_for_menus(*restored_local, race_buttons(pads.one),
                                              race_buttons(pads.two), *race.content);
        if (unirally::serialize_zoom_zoo(*restored_local)
            != unirally::serialize_zoom_zoo(race.state))
            throw std::runtime_error("local save continuation diverged at "
                                     + std::to_string(frame));
    }
    // A one-player race keeps its picture history only when pictures are asked for.
    if (local || !options.pictures.empty()) {
        race.history.observe_update(previous, race.state, pack);
        if (local) write_look(look_timeline, frame, race.state.look);
        const auto shown = race.history.on_screen();
        if (const auto picture = options.pictures.find(frame); picture != options.pictures.end())
            write_ppm(picture->second, unirally::render_classic_race(race.state, *race.presentation,
                                                                     &previous, &shown));
    }
    if (local) write_race_state(race_timeline, frame, race.state);
    if (!over) return;
    const auto& times = *over;
    unirally::return_from_race(front_end, content, frame, times);
    std::cerr << "race returned at " << frame << "; totals " << times.player_total << '/'
              << times.opponent_total << "; initialized at " << race.initialization_frame
              << " (its loading gives " << race.loading_initialization << ")\n";
    race.content.reset();
    ++races;
}

// A frame of the race's content load: its sound load cues, and the requested outputs. The load
// is forced blank; retain requested pictures instead of silently omitting them from a
// fixed-frame differential capture (R-0073).
void write_loading_frame(const Options& options, const unirally::FrontEndState& state,
                         const RaceBetweenMenus& race, std::uint32_t frame,
                         SoundCueLog& sound_cues) {
    const unirally::ClassicRaceTrack track{state.tour_menu.track};
    auto timing = unirally::race_sound_load_timing(track, state.race_song);
    // A given initialization frame (a capture's) changes the session's upload, not its request:
    // the upload's length varies with the sound processor's state (R-0077).
    if (timing.upload_frames && race.loading_initialization)
        timing.upload_frames =
            static_cast<std::uint32_t>(static_cast<std::int64_t>(timing.upload_frames)
                                       + race.initialization_frame - race.loading_initialization);
    sound_cues.write(frame, unirally::race_sound::loading(race.initialization_frame - frame, timing,
                                                          track.index, state.race_song));
    if (const auto picture = options.pictures.find(frame); picture != options.pictures.end())
        write_ppm(picture->second, unirally::RgbFrame{});
    if (const auto records = options.records.find(frame); records != options.records.end())
        write_records(records->second, state);
}

} // namespace

int main(int argc, char** argv) try {
    const auto options = parse_options(argc, argv);
    std::ofstream race_timeline, look_timeline;
    if (!options.race_timeline.empty()) {
        race_timeline.open(options.race_timeline);
        if (!race_timeline) throw std::runtime_error("cannot create race timeline");
    }
    if (!options.look_timeline.empty()) {
        look_timeline.open(options.look_timeline);
        if (!look_timeline) throw std::runtime_error("cannot create look timeline");
    }
    const unirally::ClassicContentPack pack(options.pack);
    const auto content = unirally::front_end_content(pack);
    const auto inputs = read_inputs(options.inputs);
    auto state = start_state(options);
    RaceBetweenMenus race;
    std::optional<unirally::ZoomZooState> restored_local;
    std::size_t races = 0, resets = 0;
    SoundCueLog sound_cues(options.sound_cues);
    for (std::uint32_t frame = 0; frame < options.frames; ++frame) {
        for (const auto& [after, offset, byte] : options.record_writes)
            if (frame == after + 1) set_record_byte(state.records, offset, byte);
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
                if (state.mode == unirally::FrontEndMode::demo)
                    write_race_state(race_timeline, race.initialization_frame, race.state);
                restored_local.reset();
            }
            if (state.mode == unirally::FrontEndMode::demo) {
                if (frame <= race.initialization_frame) continue;
                update_demo_race(options, pack, state, race, frame, pads, races);
                write_race_state(race_timeline, frame, race.state);
                continue;
            }
            if (frame <= race.initialization_frame) {
                write_loading_frame(options, state, race, frame, sound_cues);
                continue;
            }
            update_local_race(options, pack, content, state, race, race_timeline, look_timeline,
                              restored_local, races, frame, pads);
            sound_cues.write(frame, race.state.sound_cues);
        } else if (state.mode_chosen) {
            break;
        } else {
            update_front_end(options, state, content, pads);
            sound_cues.write(frame, state.sound_cues);
            const bool reset = state.after_soft_reset && state.boot_start + 1 == state.frame;
            if (reset && resets < options.reset_upload_delays.size())
                state.reset_upload_delay = options.reset_upload_delays[resets];
            if (reset) ++resets;
        }
        if (state.screen != unirally::FrontEndScreen::race) {
            write_frame_outputs(options, frame, state);
        } else {
            if (frame <= race.initialization_frame)
                if (const auto at = options.pictures.find(frame); at != options.pictures.end())
                    write_ppm(at->second, unirally::RgbFrame{});
            if (const auto at = options.records.find(frame); at != options.records.end())
                write_records(at->second, state);
        }
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
