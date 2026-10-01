#include "audio_cpu_scene.hpp"
#include <bit>
#include <stdexcept>
#include <utility>

namespace unirally {
namespace {
using Clock = AudioCpuWorkClock;
using Cartridge = std::array<std::uint8_t, 8192>;
struct RankingRow {
    std::uint16_t index = 0, score = 0;
};
std::uint16_t read_word(const Cartridge& cartridge, unsigned address) {
    return static_cast<std::uint16_t>(cartridge[address] | unsigned(cartridge[address + 1]) << 8);
}
void write_word(Cartridge& cartridge, unsigned address, std::uint16_t value) {
    cartridge[address] = static_cast<std::uint8_t>(value);
    cartridge[address + 1] = static_cast<std::uint8_t>(value >> 8);
}
void copy_work_bytes(Clock& c, unsigned count) {
    c.call_far();
    for (unsigned byte = 0; byte < count; ++byte) c.move_ram_byte();
    c.return_far();
}
// $83:963C; $83:969F; R-0075. Expand the membership bits into eight rider slots.
std::array<std::uint8_t, 8> read_members(Clock& c, Cartridge& cartridge, unsigned league) {
    c.call_far();
    c.save_register();
    c.change_widths();
    c.save_register(2);
    c.save_register(2);
    c.save_register();
    c.load_constant();
    c.load_constant(2);
    std::array<std::uint8_t, 8> members;
    members.fill(16);
    for (unsigned slot = 0; slot < 8; ++slot) {
        c.store_ram(1, false, true);
        c.update_register();
        c.branch(slot < 7);
    }
    c.restore_register();
    c.change_widths();
    c.load_constant(2);
    c.update_register();
    c.update_register();
    c.read_ram(2, true, true);
    const auto mask = read_word(cartridge, 0x02b2 + 2 * league);
    if (std::popcount(mask) > 8)
        throw std::invalid_argument("league membership exceeds eight riders");
    c.load_constant(2);
    c.load_constant(2);
    c.store_direct(2);
    unsigned count = 0;
    for (unsigned rider = 0; rider < 16; ++rider) {
        c.update_register();
        const bool selected = (mask & (0x8000U >> rider)) != 0;
        c.branch(!selected);
        if (selected) {
            c.change_widths();
            c.save_register();
            c.update_register();
            c.store_ram(1, false, true);
            if (count < members.size()) members[count] = static_cast<std::uint8_t>(rider);
            ++count;
            c.update_register();
            c.restore_register();
            c.change_widths();
        }
        c.update_register();
        c.load_constant(2);
        c.branch(rider < 15);
    }
    c.call_local();
    c.change_widths();
    c.load_constant(2);
    for (unsigned slot = 0; slot < 8; ++slot) {
        const auto index = 7 - slot;
        c.read_ram(1, false, true);
        c.update_register();
        c.update_register();
        c.store_ram(1, true, true);
        cartridge[0x10b6 + members[index]] = static_cast<std::uint8_t>(index);
        c.update_register();
        c.branch(slot < 7);
    }
    c.return_local();
    c.restore_register(2);
    c.restore_register(2);
    c.restore_register();
    c.return_far();
    return members;
}
// $80:EFD4; R-0075. Unsigned selection sort replaces the minimum on equal words.
void sort_ranking(Clock& c, std::array<RankingRow, 8>& rows) {
    c.call_local();
    c.save_register(2);
    c.save_register(2);
    c.save_register();
    c.change_widths();
    c.save_register(2);
    c.load_constant(2);
    c.store_direct(2);
    for (unsigned remaining = 8; remaining > 0; --remaining) {
        c.load_constant(2);
        c.load_constant(2);
        c.store_direct(2);
        c.store_direct(2);
        std::uint16_t minimum = 0x7fff;
        unsigned minimum_index = 0;
        for (unsigned index = 0; index < remaining; ++index) {
            c.read_ram(2, false, true);
            c.read_direct(2);
            const bool equal = rows[index].score == minimum;
            c.branch(equal);
            if (!equal) c.branch(rows[index].score >= minimum);
            if (rows[index].score <= minimum) {
                c.store_direct(2);
                c.store_direct(2);
                minimum = rows[index].score;
                minimum_index = index;
            }
            for (unsigned step = 0; step < 5; ++step) c.update_register();
            c.branch(index + 1 < remaining);
        }
        c.read_direct(2);
        for (unsigned step = 0; step < 3; ++step) c.update_register();
        c.read_direct(2);
        for (unsigned field = 0; field < 2; ++field) {
            c.read_ram(2, false, true);
            c.save_register(2);
            c.read_ram(2, false, true);
            c.store_ram(2, false, true);
            c.restore_register(2);
            c.store_ram(2, false, true);
        }
        std::swap(rows[minimum_index], rows[remaining - 1]);
        c.modify_direct_word();
        c.read_direct(2);
        c.branch(remaining > 1);
    }
    c.restore_register(2);
    c.restore_register();
    c.restore_register(2);
    c.restore_register(2);
    c.return_local();
}
// $83:96BE; R-0075. Map sorted row indices back to rider bytes, then copy eight bytes.
void reorder_members(Clock& c, std::array<std::uint8_t, 8>& members,
                     const std::array<RankingRow, 8>& rows) {
    c.call_far();
    c.save_register(2);
    c.save_register(2);
    c.save_register(2);
    c.save_register();
    c.change_widths();
    c.load_constant(2);
    c.load_constant(2);
    auto previous = members;
    for (unsigned slot = 0; slot < 8; ++slot) {
        c.read_ram(1, false, true);
        c.change_widths();
        c.load_constant(2);
        c.save_register(2);
        c.update_register();
        c.change_widths();
        c.read_ram(1, false, true);
        c.store_ram(1, false, true);
        members[7 - slot] = previous[rows[7 - slot].index];
        c.restore_register(2);
        for (unsigned step = 0; step < 5; ++step) c.update_register();
        c.branch(slot < 7);
    }
    c.load_constant();
    c.store_ram();
    c.store_ram();
    c.change_widths();
    for (unsigned value = 0; value < 3; ++value) c.load_constant(2);
    copy_work_bytes(c, 8);
    c.restore_register();
    c.restore_register(2);
    c.restore_register(2);
    c.restore_register(2);
    c.return_far();
}
// $80:AD79; R-0075. The packed score scratch keeps row identities alongside score words.
std::array<RankingRow, 8> initialize_ranking(Clock& c, const Cartridge& cartridge,
                                             unsigned league) {
    c.change_widths();
    c.read_direct(2);
    for (unsigned step = 0; step < 6; ++step) c.update_register();
    c.load_constant(2);
    c.store_direct(2);
    c.update_register();
    c.change_widths();
    c.load_constant();
    c.store_ram();
    c.load_constant(2);
    c.store_ram();
    c.change_widths();
    c.load_constant(2);
    copy_work_bytes(c, 32);
    for (unsigned value = 0; value < 3; ++value) c.load_constant(2);
    std::array<RankingRow, 8> rows;
    for (unsigned row = 0; row < 8; ++row) {
        rows[row] = {static_cast<std::uint16_t>(row),
                     read_word(cartridge, 0x02c2 + league * 32 + row * 4)};
        c.store_ram(2, false, true);
        for (unsigned step = 0; step < 6; ++step) c.update_register();
        c.branch(row < 7);
    }
    return rows;
}
// $80:AD94; R-0075. Write pairing riders and the initial best score/holder.
void save_pairing(Clock& c, Cartridge& cartridge, unsigned league,
                  const std::array<std::uint8_t, 8>& members) {
    c.read_direct(2);
    for (unsigned step = 0; step < 4; ++step) c.update_register();
    c.load_constant(2);
    c.update_register();
    c.change_widths();
    c.load_constant(2);
    for (unsigned slot = 0; slot < 8; ++slot) {
        c.read_ram(1, false, true);
        c.store_ram(1, true, true);
        cartridge[0x05e8 + league * 8 + (7 - slot)] = members[7 - slot];
        c.update_register();
        c.update_register();
        c.branch(slot < 7);
    }
    c.change_widths();
    c.read_direct(2);
    c.load_constant(2);
    for (unsigned step = 0; step < 3; ++step) c.update_register();
    c.read_ram(2, true);
    const auto track = read_word(cartridge, 0x074c) & 7;
    c.load_constant(2);
    c.load_constant(2);
    c.branch(track == 0);
    if (track != 0) {
        c.load_constant(2);
        c.branch(track == 3);
    }
    c.load_constant(2);
    const auto best = track == 0 || track == 3 ? 0xea62 : 0;
    if (best == 0) c.branch(true);
    c.store_ram(2, true, true);
    write_word(cartridge, 0x0684 + league * 4, static_cast<std::uint16_t>(best));
    c.read_direct(2);
    c.load_constant(2);
    c.store_ram(2, true, true);
    write_word(cartridge, 0x0686 + league * 4, members[0]);
}
// $80:AD43; R-0075. Sort one league and restore the original caller's registers.
void initialize_league(Clock& c, Cartridge& cartridge, unsigned league) {
    c.call_far();
    c.call_local();
    for (unsigned reg = 0; reg < 3; ++reg) c.save_register(2);
    c.save_register();
    c.change_widths();
    c.read_direct();
    auto members = read_members(c, cartridge, league);
    auto rows = initialize_ranking(c, cartridge, league);
    sort_ranking(c, rows);
    reorder_members(c, members, rows);
    save_pairing(c, cartridge, league, members);
    c.restore_register();
    for (unsigned reg = 0; reg < 3; ++reg) c.restore_register(2);
    c.return_local();
    c.return_far();
}
void clear_byte_rows(Clock& c, Cartridge& cartridge, unsigned destination, unsigned count,
                     bool initial_width_change = true) {
    if (initial_width_change) c.change_widths();
    c.load_constant();
    c.load_constant(2);
    c.load_constant(2);
    for (unsigned byte = 0; byte < count; ++byte) {
        c.store_ram(1, true, true);
        cartridge[destination + byte] = 0;
        c.update_register();
        c.update_register();
        c.branch(byte + 1 < count);
    }
}
} // namespace
// $83:93F5; R-0075. Six native league resets, medals, HUNTER defaults and tour levels.
void native_audio_menu_league_defaults(Clock& c, Cartridge& cartridge) {
    c.call_far();
    c.save_register();
    c.change_widths();
    c.load_constant();
    c.load_constant(2);
    c.load_constant(2);
    for (unsigned byte = 0; byte < 48; ++byte) {
        c.store_ram(1, true, true);
        cartridge[0x0618 + byte] = 20;
        c.update_register();
        c.update_register();
        c.branch(byte < 47);
    }
    c.change_widths();
    c.load_constant(2);
    for (unsigned remaining = 6; remaining > 0; --remaining) {
        c.store_direct(2);
        initialize_league(c, cartridge, remaining - 1);
        c.update_register();
        c.branch(remaining > 1);
    }
    for (unsigned value = 0; value < 3; ++value) c.load_constant(2);
    for (unsigned word = 0; word < 48; ++word) {
        c.store_ram(2, true, true);
        write_word(cartridge, 0x0618 + 2 * word, 0);
        for (unsigned step = 0; step < 3; ++step) c.update_register();
        c.branch(word < 47);
    }
    clear_byte_rows(c, cartridge, 0x0678, 6);
    clear_byte_rows(c, cartridge, 0x067e, 6, false);
    clear_byte_rows(c, cartridge, 0x069c, 160);
    c.change_widths();
    c.load_constant(2);
    c.load_constant(2);
    for (unsigned rider = 0; rider < 16; ++rider) {
        c.load_constant();
        c.store_ram(1, true, true);
        cartridge[0x071c + rider] = 2;
        c.update_register();
        c.update_register();
        c.branch(rider < 15);
    }
    c.change_widths();
    c.load_constant();
    c.load_constant(2);
    c.load_constant(2);
    for (unsigned rider = 0; rider < 16; ++rider) {
        for (const auto base : {0x10d3U, 0x10e3U}) {
            c.store_ram(1, true, true);
            cartridge[base + rider] = 0;
        }
        c.update_register();
        c.update_register();
        c.branch(rider < 15);
    }
    c.restore_register();
    c.return_far();
    c.read_ram(2, true);
    c.update_register();
    c.store_ram(2, true);
    write_word(cartridge, 0x074c, static_cast<std::uint16_t>(read_word(cartridge, 0x074c) + 1));
}
} // namespace unirally
