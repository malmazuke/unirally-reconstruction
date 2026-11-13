#include "cartridge_ram.hpp"
#include "front_end_screens.hpp"

#include "audio_cpu_clock.hpp"
#include "audio_cpu_scene.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <stdexcept>

namespace unirally {
namespace {

// `$83:8000`: the signature, the default records and the option words (pack entry
// `audio.cartridge-defaults`); one track kind per track (`audio.track-types`).
constexpr std::size_t cartridge_defaults_bytes = 1158, track_count = 50;

// One pass over every field native keeps, at its cartridge RAM address (front_end.hpp's
// OnePlayerRecords). `Codec` either stores the fields into the image or loads them from it, so
// the two directions cannot disagree.
// One byte of a word field: its low byte at an even distance, else its high byte.
void store_word_byte(std::uint16_t& word, std::size_t distance, std::uint8_t byte) {
    const unsigned shift = static_cast<unsigned>(distance % 2) * 8U;
    word = static_cast<std::uint16_t>((word & ~(0xffU << shift)) | unsigned{byte} << shift);
}

template <class Codec>
void transfer_records(Codec& at, OnePlayerRecords& records) {
    for (std::size_t k = 0; k < records.rider_names.size(); ++k)
        at.byte(0x000c + k, records.rider_names[k]);
    for (std::size_t k = 0; k < records.league_names.size(); ++k)
        at.byte(0x016e + k, records.league_names[k]);
    for (std::size_t rider = 0; rider < records.statistics.size(); ++rider)
        for (std::size_t k = 0; k < 4; ++k)
            at.word(0x0230 + 8 * rider + 2 * k, records.statistics[rider][k]);
    for (std::size_t slot = 0; slot < records.league_members.size(); ++slot) {
        at.word(0x02b2 + 2 * slot, records.league_members[slot]);
        for (std::size_t row = 0; row < records.league_scores[slot].size(); ++row) {
            at.word(0x02c0 + 32 * slot + 4 * row, records.league_played[slot][row]);
            at.word(0x02c0 + 32 * slot + 4 * row + 2, records.league_scores[slot][row]);
            at.byte(0x05e8 + 8 * slot + row, records.league_pairings[slot][row]);
            at.word(0x0618 + 16 * slot + 2 * row, records.league_event_totals[slot][row]);
        }
        at.byte(0x0678 + slot, records.league_pair_cursor[slot]);
        at.byte(0x067e + slot, records.league_tracks[slot]);
        at.word(0x0684 + 4 * slot, records.league_best[slot]);
        at.word(0x0686 + 4 * slot, records.league_best_holder[slot]);
    }
    at.word(0x02be, records.league_members_checksum);
    constexpr std::array<std::size_t, 3> times{0x0422, 0x0486, 0x04ea};
    constexpr std::array<std::size_t, 3> holders{0x0550, 0x0582, 0x05b4};
    for (std::size_t place = 0; place < times.size(); ++place)
        for (std::size_t track = 0; track < records.record_times[place].size(); ++track) {
            at.word(times[place] + 2 * track, records.record_times[place][track]);
            at.byte(holders[place] + track, records.record_holders[place][track]);
        }
    for (std::size_t k = 0; k < records.medals.size(); ++k) at.byte(0x069c + k, records.medals[k]);
    at.bit(0x0742, 0x0002, records.league_naming);
    at.bit(0x0742, 0x0004, records.versus_chosen);
    at.bit(0x0742, 0x0008, records.second_human);
    at.bit(0x0742, 0x0020, records.group_table_shown);
    at.bit(0x0742, 0x0100, records.result_objects_parked);
    at.bit(0x0742, 0x1000, records.race_lost);
    at.bit(0x0742, 0x2000, records.league_cycle_complete);
    for (std::size_t k = 0; k < records.best.size(); ++k) at.word(0x0829 + 2 * k, records.best[k]);
    at.word(0x1073, records.tries);
    for (std::size_t k = 0; k < records.tracks_done.size(); ++k)
        at.byte(0x1075 + k, records.tracks_done[k]);
    for (std::size_t rider = 0; rider < records.versus_races.size(); ++rider) {
        at.word(0x0380 + 4 * rider, records.versus_races[rider]);
        at.word(0x0382 + 4 * rider, records.versus_wins[rider]);
        at.word(0x0400 + 2 * rider, records.versus_today[rider]);
    }
    at.word(0x10f9, records.versus_percentage_races);
    at.word(0x10fb, records.versus_percentage_wins);
    at.word(0x10a9, records.player_wins);
    at.word(0x10ab, records.opponent_wins);
    at.byte(0x10b1, records.race_song_counter);
    at.byte(0x10c8, records.demo_track);
    at.flag(0x10d0, records.cheat);
    for (std::size_t k = 0; k < records.tour_levels.size(); ++k)
        at.byte(0x10d3 + k, records.tour_levels[k]);
    for (std::size_t k = 0; k < records.levels_before_cheat.size(); ++k)
        at.byte(0x10e3 + k, records.levels_before_cheat[k]);
    at.byte(0x1115, records.demo_split);
    at.word(0x1116, records.tutorial_bits);
    at.byte(0x111a, records.hunter_races);
}

struct Store {
    CartridgeImage& image;
    void byte(std::size_t at, std::uint8_t value) { image.at(at) = value; }
    void word(std::size_t at, std::uint16_t value) {
        byte(at, static_cast<std::uint8_t>(value));
        byte(at + 1, static_cast<std::uint8_t>(value >> 8U));
    }
    // One bit of a word whose other bits native does not keep.
    void bit(std::size_t at, std::uint16_t mask, bool set) {
        auto value = static_cast<std::uint16_t>(image.at(at) | image.at(at + 1) << 8U);
        value = static_cast<std::uint16_t>(set ? value | mask : value & ~mask);
        word(at, value);
    }
    // A flag native keeps as a bool: a byte that already reads as the flag stays as it is (an
    // image's `$FF` before the boot's check), else 1 or 0.
    void flag(std::size_t at, bool set) {
        if ((image.at(at) != 0) != set) byte(at, set ? 1 : 0);
    }
};

struct Load {
    const CartridgeImage& image;
    void byte(std::size_t at, std::uint8_t& value) const { value = image.at(at); }
    void word(std::size_t at, std::uint16_t& value) const {
        value = static_cast<std::uint16_t>(image.at(at) | image.at(at + 1) << 8U);
    }
    void bit(std::size_t at, std::uint16_t mask, bool& set) const {
        set = ((image.at(at) | image.at(at + 1) << 8U) & mask) != 0;
    }
    void flag(std::size_t at, bool& set) const { set = image.at(at) != 0; }
};

// One byte stored at a cartridge RAM offset, into whichever field native keeps there (one bit of a
// word field takes the byte's bit; a flag, whether the byte is nonzero).
struct Patch {
    std::size_t offset;
    std::uint8_t value;
    void byte(std::size_t at, std::uint8_t& field) const {
        if (at == offset) field = value;
    }
    void word(std::size_t at, std::uint16_t& field) const {
        if (offset == at || offset == at + 1) store_word_byte(field, offset - at, value);
    }
    void bit(std::size_t at, std::uint16_t mask, bool& set) const {
        if (offset != at && offset != at + 1) return;
        const unsigned shift = offset == at ? 0U : 8U;
        if (((unsigned{mask} >> shift) & 0xffU) != 0)
            set = ((unsigned{value} << shift) & mask) != 0;
    }
    void flag(std::size_t at, bool& set) const {
        if (at == offset) set = value != 0;
    }
};

} // namespace

CartridgeImage cold_start_cartridge(const FrontEndContent& content) {
    if (content.cartridge_defaults.size() != cartridge_defaults_bytes
        || content.track_types.size() != track_count)
        throw std::invalid_argument("the pack has no cartridge defaults");
    // The audio work model performs the original's default writes (R-0075); its clock is
    // discarded here.
    CartridgeImage image{};
    AudioCpuWorkClock clock;
    AudioCpuSceneWorkState scene;
    const std::span<const std::uint8_t, cartridge_defaults_bytes> defaults{
        content.cartridge_defaults.data(), cartridge_defaults_bytes};
    native_audio_menu_first_record_defaults(
        clock, image, defaults,
        std::span<const std::uint8_t, track_count>{content.track_types.data(), track_count});
    native_audio_menu_league_defaults(clock, image);
    native_audio_finish_menu_records(clock, scene, image, defaults);
    return image;
}

bool has_signature(const CartridgeImage& image, const FrontEndContent& content) {
    constexpr std::size_t signature_bytes = 12;
    return content.cartridge_defaults.size() >= signature_bytes
        && std::equal(image.begin(), image.begin() + signature_bytes,
                      content.cartridge_defaults.begin());
}

namespace {
// The sum of `words` little-endian words from `first`, wrapping (`$83:90F4`).
std::uint16_t checksum(const CartridgeImage& image, std::size_t first, std::size_t words) {
    std::uint16_t sum = 0;
    for (std::size_t k = 0; k < words; ++k)
        sum = static_cast<std::uint16_t>(sum + image.at(first + 2 * k)
                                         + (image.at(first + 2 * k + 1) << 8U));
    return sum;
}

// A records checksum (`$83:90F4`) at `at`, rewritten only when its records differ from `kept`'s.
void store_records_checksum(CartridgeImage& image, const CartridgeImage& kept, std::size_t first,
                            std::size_t words, std::size_t at) {
    const auto summed = std::span(image).subspan(first, 2 * words);
    if (std::equal(summed.begin(), summed.end(), kept.begin() + static_cast<std::ptrdiff_t>(first)))
        return;
    const auto sum = checksum(image, first, words);
    image.at(at) = static_cast<std::uint8_t>(sum);
    image.at(at + 1) = static_cast<std::uint8_t>(sum >> 8U);
}
} // namespace

CartridgeImage cartridge_image(const FrontEndState& state) {
    auto image = state.cartridge;
    auto records = state.records;
    Store store{image};
    transfer_records(store, records);
    // The names' checksums, which the original rewrites with the names (`$83:90F4`); `$83:89EF`
    // reads the riders'. While the names are as loaded or as the wipe wrote them, so is the
    // checksum (an image's `$FF` before the boot's check, R-0111). Its other checksums (`$02B0`,
    // `$0420`, `$054E`, `$05E6`, `$073C`, `$0E69`) are not read and keep what the boot left
    // (R-0090): following their records moved more write frames away from the original's than it
    // brought to it, since the original sums them only at the end of each records update (R-0111).
    store_records_checksum(image, state.cartridge, 0x000c, 176, 0x016c);
    store_records_checksum(image, state.cartridge, 0x016e, 96, 0x022e);
    // `$77:0742` bit 1 is the logo held up as well as the league keyboard's (`$80:F51B`,
    // `$80:F53F`, `$80:F52B`); every boot clears it (`$80:A0A5`) before the logo hook reads it.
    store.bit(0x0742, 0x0002, records.league_naming || state.logo.raised);
    // `$77:0742` bits 9 and 10, the menus' pad gate, live in cartridge RAM too. A later boot does
    // not read them: the main menu opens both pads (`$80:ABC8`) before any gated test (R-0105).
    store.bit(0x0742, 0x0200, state.pad_one_ignored);
    store.bit(0x0742, 0x0400, state.pad_two_ignored);
    if (state.demo_option_byte) image[0x0742] = *state.demo_option_byte;
    // `$77:10FD + $CA`: the pending reveal is kept per rider (`$83:9F8F`); native keeps the
    // current rider's (R-0062, R-0111).
    image.at(pending_reveals + (state.options_rider & 0x0fU)) = state.records.pending_reveal;
    // `$10AD`, the mode, is stored in the image where the original stores it (store_saved_mode).
    return image;
}

OnePlayerRecords records_from_cartridge(const CartridgeImage& image) {
    OnePlayerRecords records;
    const Load load{image};
    transfer_records(load, records);
    return records;
}

CartridgeImage power_on_cartridge() {
    CartridgeImage image;
    image.fill(0xff);
    return image;
}

void insert_cartridge(FrontEndState& state, std::span<const std::uint8_t> image) {
    if (image.size() != state.cartridge.size())
        throw std::invalid_argument("a cartridge RAM image is 8192 bytes");
    if (state.frame != 0) throw std::logic_error("the cartridge goes in before power-on");
    std::copy(image.begin(), image.end(), state.cartridge.begin());
    // The original reads cartridge RAM from power-on: the title's code and cheat flag before the
    // boot's check (`$80:F55F`), `$10AD` for the arrow's colours (`$83:91FB`). The check then
    // keeps these fields or wipes them (front_end.cpp's check_records).
    state.records = records_from_cartridge(state.cartridge);
    state.records.league_naming = false; // `$80:A0A5` clears `$0742` bit 1 on every boot
    state.records.pending_reveal = state.cartridge[pending_reveals + (state.options_rider & 0x0fU)];
    const Load load_gate{state.cartridge};
    load_gate.bit(0x0742, 0x0200, state.pad_one_ignored); // the pad gate, as saved
    load_gate.bit(0x0742, 0x0400, state.pad_two_ignored);
    state.one_player = state.cartridge[one_player_mode] == 1;
    // `$77:10A7`, the done-track markers' step, goes on from the saved word (R-0100), whatever it
    // holds: a save made during a gold ending leaves the ending's counter there (0xFFF4 to 0x0020,
    // R-0117), which the first turns count down (`$80:EB28-EB32`, `turn_markers`). Only a cold
    // start's wipe resets it.
    state.track_menu.marker_step = front_end_screens::saved_marker_step(state);
}

void store_race_mode_from_option_byte(FrontEndState& state) {
    constexpr std::size_t option_word = 0x0742, race_mode = 0x0750;
    state.cartridge.at(race_mode) = cartridge_image(state).at(option_word);
}

void store_record_byte(FrontEndState& state, std::size_t offset, std::uint8_t byte) {
    // The reference core mirrors the 8 KiB through banks `$77` and `$78`, so an offset past
    // `$77:1FFF` lands on its mirror (review 2: cursors poked to 0xFF put `$80:F88D`'s totals at
    // `$0616`, a cursor word of 0x1000 at `$0618`).
    constexpr std::size_t mirror = 0x1fff;
    offset &= mirror;
    const Patch patch{offset, byte};
    transfer_records(patch, state.records);
    state.cartridge.at(offset) = byte;
}

void store_record_word(FrontEndState& state, std::size_t offset, std::uint16_t value) {
    store_record_byte(state, offset, static_cast<std::uint8_t>(value));
    store_record_byte(state, offset + 1, static_cast<std::uint8_t>(value >> 8U));
}

void add_race_cartridge_increments(FrontEndState& state, std::span<const std::uint16_t> offsets) {
    constexpr std::size_t best_times = 0x0829;
    auto& best = state.records.best;
    for (const auto offset : offsets) {
        if (offset < best_times || offset >= best_times + 2 * best.size())
            throw std::invalid_argument(
                "a race's cartridge RAM increment lies outside the records");
        auto& word = best[(offset - best_times) / 2];
        const unsigned shift = (offset - best_times) % 2 * 8U;
        const unsigned value = word;
        const unsigned byte = ((value >> shift) + 1U) & 0xffU;
        word = static_cast<std::uint16_t>((value & ~(0xffU << shift)) | (byte << shift));
    }
}

} // namespace unirally
