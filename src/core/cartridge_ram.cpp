#include "cartridge_ram.hpp"

#include "audio_cpu_clock.hpp"
#include "audio_cpu_scene.hpp"

#include <algorithm>
#include <stdexcept>

namespace unirally {
namespace {

// One pass over every field native keeps, at its cartridge RAM address (front_end.hpp's
// OnePlayerRecords). `Codec` either stores the fields into the image or loads them from it, so
// the two directions cannot disagree.
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
    at.word(0x02be, records.active_league_members);
    constexpr std::array<std::size_t, 3> times{0x0422, 0x0486, 0x04ea};
    constexpr std::array<std::size_t, 3> holders{0x0550, 0x0582, 0x05b4};
    for (std::size_t place = 0; place < times.size(); ++place)
        for (std::size_t track = 0; track < records.record_times[place].size(); ++track) {
            at.word(times[place] + 2 * track, records.record_times[place][track]);
            at.byte(holders[place] + track, records.record_holders[place][track]);
        }
    for (std::size_t k = 0; k < records.medals.size(); ++k) at.byte(0x069c + k, records.medals[k]);
    at.bit(0x0742, 0x0002, records.league_naming);
    at.bit(0x0742, 0x1000, records.race_lost);
    at.bit(0x0742, 0x2000, records.league_cycle_complete);
    for (std::size_t k = 0; k < records.best.size(); ++k) at.word(0x0829 + 2 * k, records.best[k]);
    at.word(0x1073, records.tries);
    for (std::size_t k = 0; k < records.tracks_done.size(); ++k)
        at.byte(0x1075 + k, records.tracks_done[k]);
    at.word(0x10a9, records.player_wins);
    at.word(0x10ab, records.opponent_wins);
    at.byte(0x10b1, records.race_song_counter);
    at.byte(0x10c8, records.demo_track);
    at.flag(0x10d0, records.cheat);
    for (std::size_t k = 0; k < records.tour_levels.size(); ++k)
        at.byte(0x10d3 + k, records.tour_levels[k]);
    for (std::size_t k = 0; k < records.levels_before_cheat.size(); ++k)
        at.byte(0x10e3 + k, records.levels_before_cheat[k]);
    at.byte(0x10fd, records.pending_reveal);
    at.byte(0x1115, records.demo_split);
    at.word(0x1116, records.tutorial_bits);
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
    void flag(std::size_t at, bool set) { byte(at, set ? 1 : 0); }
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

} // namespace

CartridgeImage cold_start_cartridge(const FrontEndContent& content) {
    if (content.cartridge_defaults.size() != 1158 || content.track_types.size() != 50)
        throw std::invalid_argument("the pack has no cartridge defaults");
    // The audio work model performs the original's default writes (R-0075); its clock is
    // discarded here.
    CartridgeImage image{};
    AudioCpuWorkClock clock;
    AudioCpuSceneWorkState scene;
    const std::span<const std::uint8_t, 1158> defaults{content.cartridge_defaults.data(), 1158};
    native_audio_menu_first_record_defaults(
        clock, image, defaults, std::span<const std::uint8_t, 50>{content.track_types.data(), 50});
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
} // namespace

CartridgeImage cartridge_image(const FrontEndState& state) {
    auto image = state.cartridge;
    auto records = state.records;
    Store store{image};
    transfer_records(store, records);
    // The names' checksums, which the original rewrites with the names (`$83:90F4`); `$83:89EF`
    // reads the riders'. Its other checksums (`$02B0`, `$0420`, `$054E`, `$05E6`, `$073C`,
    // `$0E69`) are not read and keep what the boot left (R-0090).
    store.word(0x016c, checksum(image, 0x000c, 176));
    store.word(0x022e, checksum(image, 0x016e, 96));
    return image;
}

OnePlayerRecords records_from_cartridge(const CartridgeImage& image) {
    OnePlayerRecords records;
    const Load load{image};
    transfer_records(load, records);
    return records;
}

void insert_cartridge(FrontEndState& state, std::span<const std::uint8_t> image) {
    if (image.size() != state.cartridge.size())
        throw std::invalid_argument("a cartridge RAM image is 8192 bytes");
    if (state.frame != 0) throw std::logic_error("the cartridge goes in before power-on");
    std::copy(image.begin(), image.end(), state.cartridge.begin());
}

} // namespace unirally
