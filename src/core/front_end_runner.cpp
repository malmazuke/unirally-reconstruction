// FRONT-END-MAIN-MENU laboratory runner: the native front end from power-on, frame by frame.
//
// usage: front_end_runner --content-pack PACK --frames N [--inputs FILE] [--picture FRAME OUT.ppm]...
//                         [--records FRAME OUT.bin]... [--race-initialization FRAME]...
//                         [--reset-upload-delay FRAMES]...
//                         [--record-write FRAME OFFSET BYTE]...
//
// FILE rows are "frame pad1 pad2" (hex controller words, as `$4218`/`$421A` read them); a frame
// without a row has both pads released. Each frame prints one line: the frame, the arrow (spin,
// x, target x, y, target y), the menu (idle, selection, latch), the palette cycle (delay, phase),
// the OAM buffer ($0A00, 544 bytes) and CGRAM, in hex, for comparison with a capture's work RAM.
// `--records` writes the one-player records after that frame as the original keeps them in
// cartridge RAM (8 KiB, `$77:0000`), the words native does not keep left 0.
// `--record-write` sets one record byte at its cartridge RAM offset (hex): a name
// `$77:000C-016B`, done track `$77:1075-10A6` or medal `$77:069C-073B`.
// Writes happen after FRAME, as in the bounded reference interventions (R-0065).
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
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace {

struct Options {
    std::filesystem::path pack, inputs;
    std::filesystem::path race_timeline, sound_cues;
    std::uint32_t frames{};
    std::map<std::uint32_t, std::filesystem::path> pictures, records, vram;
    // The original's race initialization frames, in race order, from a capture: the loading time
    // varies by a frame with the sound program's handshake (R-0039, R-0058).
    std::vector<std::uint32_t> race_initializations;
    // After each soft reset in turn, how much later the sound program's upload ends than at
    // power-on (HUNTER-ENDING); native's own is 3.
    std::vector<std::uint32_t> reset_upload_delays;
    std::optional<std::uint32_t> human_after;
    std::optional<std::uint32_t> restore_check;
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
        else if (option == "--sound-cues")
            options.sound_cues = value();
        else if (option == "--picture") {
            const auto frame = static_cast<std::uint32_t>(std::stoul(value()));
            options.pictures[frame] = value();
        } else if (option == "--race-initialization") {
            options.race_initializations.push_back(static_cast<std::uint32_t>(std::stoul(value())));
        } else if (option == "--reset-upload-delay") {
            options.reset_upload_delays.push_back(static_cast<std::uint32_t>(std::stoul(value())));
        } else if (option == "--record-write") {
            const auto frame = parse_number(value(), 10, UINT32_MAX - 1, "frame");
            const auto offset = parse_number(value(), 16, 0x1fff, "offset");
            const auto byte = static_cast<std::uint8_t>(parse_number(value(), 16, 0xff, "byte"));
            unirally::OnePlayerRecords check;
            set_record_byte(check, offset, byte); // refuses an offset native keeps no record for
            options.record_writes.emplace_back(frame, offset, byte);
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
                                    "[--records FRAME OUT.bin]... "
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

// The records at their cartridge RAM addresses (front_end.hpp's OnePlayerRecords).
void write_records(const std::filesystem::path& path, const unirally::OnePlayerRecords& records) {
    std::array<std::uint8_t, 0x2000> image{};
    const auto put_word = [&](std::size_t at, std::uint16_t value) {
        image[at] = static_cast<std::uint8_t>(value);
        image[at + 1] = static_cast<std::uint8_t>(value >> 8U);
    };
    std::copy(records.rider_names.begin(), records.rider_names.end(), image.begin() + 0x000c);
    std::uint16_t name_checksum = 0;
    for (std::size_t at = 0; at < records.rider_names.size(); at += 2)
        name_checksum = static_cast<std::uint16_t>(
            name_checksum + records.rider_names[at]
            + (static_cast<std::uint16_t>(records.rider_names[at + 1]) << 8U));
    put_word(0x016c, name_checksum);
    std::copy(records.league_names.begin(), records.league_names.end(), image.begin() + 0x016e);
    std::uint16_t league_checksum = 0;
    for (std::size_t at = 0; at < records.league_names.size(); at += 2)
        league_checksum = static_cast<std::uint16_t>(
            league_checksum + records.league_names[at]
            + (static_cast<std::uint16_t>(records.league_names[at + 1]) << 8U));
    put_word(0x022e, league_checksum);
    for (std::size_t slot = 0; slot < records.league_members.size(); ++slot)
        put_word(0x02b2 + 2 * slot, records.league_members[slot]);
    put_word(0x02be, records.active_league_members);
    for (std::size_t slot = 0; slot < records.league_scores.size(); ++slot)
        for (std::size_t row = 0; row < records.league_scores[slot].size(); ++row) {
            put_word(0x02c0 + 32 * slot + 4 * row, records.league_played[slot][row]);
            put_word(0x02c0 + 32 * slot + 4 * row + 2, records.league_scores[slot][row]);
            image[0x05e8 + 8 * slot + row] = records.league_pairings[slot][row];
            put_word(0x0618 + 16 * slot + 2 * row, records.league_event_totals[slot][row]);
        }
    for (std::size_t slot = 0; slot < records.league_members.size(); ++slot) {
        image[0x0678 + slot] = records.league_pair_cursor[slot];
        image[0x067e + slot] = records.league_tracks[slot];
        put_word(0x0684 + 4 * slot, records.league_best[slot]);
        put_word(0x0686 + 4 * slot, records.league_best_holder[slot]);
    }
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
    put_word(0x0742, static_cast<std::uint16_t>((records.race_lost ? 0x1000 : 0)
                                                | (records.league_naming ? 0x0002 : 0)
                                                | (records.league_cycle_complete ? 0x2000 : 0)));
    put_word(0x10a9, records.player_wins);
    put_word(0x10ab, records.opponent_wins);
    put_word(0x1073, records.tries);
    put_word(0x1116, records.tutorial_bits);
    image[0x10fd] = records.pending_reveal;
    image[0x10d0] = records.cheat ? 1 : 0;
    for (std::size_t k = 0; k < records.levels_before_cheat.size(); ++k)
        image[0x10e3 + k] = records.levels_before_cheat[k];
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
};

// Why a race is not run (the run stops there): its loading time on this path is not known. Empty
// when it starts. A given initialization frame (`initialization`, nonzero) takes the place of the
// loading's. The rider and opponent change no loading time (R-0061).
std::string start_race(RaceBetweenMenus& race, const unirally::ClassicContentPack& pack,
                       const unirally::FrontEndState& front_end, std::uint32_t initialization) {
    if (front_end.mode == unirally::FrontEndMode::demo) {
        const bool split = front_end.demo_cycles == 0;
        auto scenario =
            unirally::classic_race_scenario(unirally::ClassicRaceTrack{front_end.tour_menu.track});
        scenario.pairing = {front_end.rider_menu.rider, front_end.now_playing.opponent};
        if (!split) scenario.initialization_frame = front_end.frame - 1U;
        race.content = unirally::classic_race_content(pack, scenario.track);
        race.state = unirally::classic_race_start(*race.content, scenario);
        if (split)
            unirally::initialize_split_cameras(race.state);
        else
            unirally::initialize_second_camera(race.state);
        race.state.demo_ai = race.state.demo.opponent_hints_active = true;
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
    const auto scenario =
        local ? unirally::classic_local_race_scenario(
                    unirally::ClassicRaceTrack{front_end.tour_menu.track},
                    {front_end.rider_menu.rider, front_end.second_rider},
                    ((front_end.records.tutorial_bits >> front_end.rider_menu.rider) & 1U) == 0)
              : unirally::one_player_race_scenario(front_end);
    const auto loading_frames = unirally::race_loading_frames(front_end);
    if (loading_frames == 0 && initialization == 0) return "its loading time is not known";
    race.content = unirally::classic_race_content(pack, scenario.track);
    race.state = unirally::classic_race_start(*race.content, scenario);
    race.state.league_statistics.enabled = front_end.mode == unirally::FrontEndMode::league;
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

void update_demo_race(const Options& options, const unirally::ClassicContentPack& pack,
                      unirally::FrontEndState& front_end, RaceBetweenMenus& race,
                      std::uint32_t frame, unirally::FrontEndPads pads, std::size_t& races) {
    if (options.human_after && frame >= *options.human_after) race.state.demo_ai = false;
    const auto previous = race.state;
    unirally::update_zoom_zoo(race.state, race_buttons(pads.one), race_buttons(pads.two),
                              *race.content);
    race.history.observe_update(previous, race.state, pack);
    if (const auto picture = options.pictures.find(frame); picture != options.pictures.end()) {
        const auto shown = race.history.on_screen();
        write_ppm(picture->second,
                  unirally::render_classic_race(race.state, *race.presentation, &previous, &shown));
    }
    if (race.state.demo.exit_requested) {
        unirally::return_from_demo(front_end, frame, race.state.demo.elapsed);
        race.content.reset();
        race.presentation.reset();
        ++races;
    }
    print_state(frame, front_end);
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
    if (saved.size() != 784 || saved[7] != 'H')
        throw std::runtime_error("local DRAGSTER save is not layout H");
    restored = unirally::deserialize_zoom_zoo(saved);
    if (unirally::serialize_zoom_zoo(*restored) != saved)
        throw std::runtime_error("local DRAGSTER save did not round-trip");
    auto invalid = saved;
    invalid.back() = 1; // The last trailer byte is demo_ai, zero for local riders.
    bool refused = false;
    try {
        (void)unirally::deserialize_zoom_zoo(invalid);
    } catch (const std::invalid_argument&) {
        refused = true;
    }
    if (!refused) throw std::runtime_error("local save accepted demo AI");
    auto extended = race_state;
    extended.special_tiles[0].mud_cooldown = 1;
    const auto extended_saved = unirally::serialize_zoom_zoo(extended);
    if (extended_saved.size() != 836 || extended_saved[7] != 'I'
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
        constexpr std::array<const char*, 9> sites{"wait",      "early",      "late",
                                                   "countdown", "finish",     "choice",
                                                   "pause",     "pause-fade", "pause-continue"};
        for (const auto& cue : cues) switch (cue.kind) {
            case unirally::AudioCueKind::enqueue:
                out_ << frame << " E " << unsigned(cue.command) << ' ' << unsigned(cue.parameter)
                     << '\n';
                break;
            case unirally::AudioCueKind::dispatch:
                out_ << frame << " D " << sites[static_cast<unsigned>(cue.site)] << '\n';
                break;
            case unirally::AudioCueKind::load:
                rotating_ = {};
                out_ << frame << " L ";
                if (cue.load == unirally::AudioSessionLoad::race)
                    out_ << "race-" << unsigned(unirally::race_song_resource(cue.parameter))
                         << " t" << unsigned(cue.command);
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
        write_records(at->second, state.records);
    if (const auto at = options.vram.find(frame); at != options.vram.end()) {
        std::ofstream out(at->second, std::ios::binary);
        out.write(reinterpret_cast<const char*>(state.video.vram.data()),
                  static_cast<std::streamsize>(state.video.vram.size()));
    }
}

void update_local_race(const Options& options, const unirally::ClassicContentPack& pack,
                       const unirally::FrontEndContent& content, unirally::FrontEndState& front_end,
                       RaceBetweenMenus& race, std::ofstream& race_timeline,
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
        timing.upload_frames = static_cast<std::uint32_t>(
            static_cast<std::int64_t>(timing.upload_frames) + race.initialization_frame
            - race.loading_initialization);
    sound_cues.write(frame, unirally::race_sound::loading(race.initialization_frame - frame, timing,
                                                          track.index, state.race_song));
    if (const auto picture = options.pictures.find(frame); picture != options.pictures.end())
        write_ppm(picture->second, unirally::RgbFrame{});
    if (const auto records = options.records.find(frame); records != options.records.end())
        write_records(records->second, state.records);
}

} // namespace

int main(int argc, char** argv) try {
    const auto options = parse_options(argc, argv);
    std::ofstream race_timeline;
    if (!options.race_timeline.empty()) {
        race_timeline.open(options.race_timeline);
        if (!race_timeline) throw std::runtime_error("cannot create race timeline");
    }
    const unirally::ClassicContentPack pack(options.pack);
    const auto content = unirally::front_end_content(pack);
    const auto inputs = read_inputs(options.inputs);
    auto state = unirally::start_front_end();
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
            update_local_race(options, pack, content, state, race, race_timeline, restored_local,
                              races, frame, pads);
            sound_cues.write(frame, race.state.sound_cues);
        } else if (state.mode_chosen) {
            break;
        } else {
            unirally::update_front_end(state, content, pads);
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
                write_records(at->second, state.records);
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
