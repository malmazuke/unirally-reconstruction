#include "audio_cpu_scene.hpp"

namespace unirally {
namespace {
using Clock = AudioCpuWorkClock;
using Cartridge = std::array<std::uint8_t, 8192>;
void store_word(Cartridge& cartridge, unsigned address, std::uint16_t value) {
    cartridge[address] = static_cast<std::uint8_t>(value);
    cartridge[address + 1] = static_cast<std::uint8_t>(value >> 8);
}
std::uint16_t read_word(std::span<const std::uint8_t, 1158> data, unsigned address) {
    return static_cast<std::uint16_t>(data[address] | unsigned(data[address + 1]) << 8);
}
// $80:96FD; R-0075. The hardware divide path reads quotient and remainder after five NOPs.
void divide_track_number(Clock& c) {
    c.call_far();
    c.call_local();
    c.change_widths();
    c.store_port(2);
    c.store_port();
    for (unsigned nop = 0; nop < 5; ++nop) c.update_register();
    c.read_direct();
    c.change_widths();
    c.read_port(2);
    c.read_port(2);
    c.return_local();
    c.return_far();
}
// $83:9983; R-0075. The cold cleared track number is zero; store its type and divide by5.
void initialize_cold_track_work(Clock& c, Cartridge& cartridge,
                                std::span<const std::uint8_t, 50> track_types) {
    c.call_far();
    c.call_local();
    c.save_register(2);
    c.save_register();
    c.change_widths();
    c.save_register(2);
    c.read_direct(2);
    c.load_constant(2);
    c.store_ram(2, true);
    store_word(cartridge, 0x074a, 0);
    c.update_register();
    c.read_rom(2, true, true);
    c.load_constant(2);
    c.store_ram(2, true);
    store_word(cartridge, 0x0744, track_types[0]);
    c.load_constant(2);
    divide_track_number(c);
    c.store_direct(2);
    c.update_register();
    c.change_widths();
    c.store_ram(1, true);
    cartridge[0x074c] = 0;
    c.load_constant();
    c.branch(true);
    c.store_ram(1, true);
    cartridge[0x074b] = 0;
    c.change_widths();
    c.restore_register(2);
    c.restore_register();
    c.restore_register(2);
    c.return_local();
    c.return_far();
}
// $83:92CC; $83:92FA; $83:9328; R-0075. Identified name/member table copies.
void copy_default_table(Clock& c, Cartridge& cartridge, std::span<const std::uint8_t, 1158> data,
                        unsigned source, unsigned destination, unsigned count, unsigned width) {
    c.call_far();
    c.save_register();
    c.change_widths();
    c.load_constant(2);
    c.load_constant(2);
    for (unsigned entry = 0; entry < count; ++entry) {
        c.read_rom(width, true, true);
        c.store_ram(width, true, true);
        for (unsigned byte = 0; byte < width; ++byte)
            cartridge[destination + entry * width + byte] = data[source + entry * width + byte];
        for (unsigned step = 0; step < width + 1; ++step) c.update_register();
        c.branch(entry + 1 < count);
    }
    c.restore_register();
    c.return_far();
}
// $83:92E3; $83:9311; R-0075. Zero the statistics and league score words.
void clear_default_words(Clock& c, Cartridge& cartridge, unsigned destination, unsigned count) {
    c.call_far();
    c.save_register();
    c.change_widths();
    for (unsigned value = 0; value < 3; ++value) c.load_constant(2);
    for (unsigned word = 0; word < count; ++word) {
        c.store_ram(2, true, true);
        store_word(cartridge, destination + word * 2, 0);
        for (unsigned step = 0; step < 3; ++step) c.update_register();
        c.branch(word + 1 < count);
    }
    c.restore_register();
    c.return_far();
}
// $83:936E; R-0075. Four races and one stunt per tour; the first-place defaults are data.
void initialize_record_times(Clock& c, Cartridge& cartridge,
                             std::span<const std::uint8_t, 1158> data) {
    c.call_far();
    c.save_register();
    c.change_widths();
    c.load_constant(2);
    c.load_constant(2);
    constexpr std::array<unsigned, 4> race_offsets{0, 2, 6, 8};
    for (unsigned tour = 0; tour < 10; ++tour) {
        const auto offset = tour * 10;
        for (const auto race : race_offsets) {
            c.read_rom(2, true, true);
            c.store_ram(2, true, true);
            store_word(cartridge, 0x0422 + offset + race, read_word(data, 0x02cc + offset + race));
        }
        c.load_constant(2);
        for (const auto base : {0x0486U, 0x04eaU}) {
            for (const auto race : race_offsets) {
                c.store_ram(2, true, true);
                store_word(cartridge, base + offset + race, 0xea60);
            }
        }
        c.read_rom(2, true, true);
        c.store_ram(2, true, true);
        store_word(cartridge, 0x0426 + offset, read_word(data, 0x02d0 + offset));
        c.load_constant(2);
        for (const auto base : {0x048aU, 0x04eeU}) {
            c.store_ram(2, true, true);
            store_word(cartridge, base + offset, 0);
        }
        c.update_register();
        c.update_register();
        c.load_constant(2);
        c.update_register();
        c.update_register();
        c.branch(tour < 9);
    }
    c.restore_register();
    c.return_far();
}
// $83:93D8; R-0075. The original's default holder is numeric index16.
void initialize_record_holders(Clock& c, Cartridge& cartridge) {
    c.call_far();
    c.save_register();
    c.change_widths();
    c.load_constant(2);
    c.load_constant(2);
    for (unsigned track = 0; track < 50; ++track) {
        c.load_constant();
        for (const auto base : {0x0550U, 0x0582U, 0x05b4U}) {
            c.store_ram(1, true, true);
            cartridge[base + track] = 16;
        }
        c.update_register();
        c.update_register();
        c.branch(track < 49);
    }
    c.restore_register();
    c.return_far();
}
// $83:9486; R-0075. Copy ten settings words, then clear the hint word.
void initialize_settings(Clock& c, Cartridge& cartridge, std::span<const std::uint8_t, 1158> data) {
    c.call_far();
    c.save_register();
    c.change_widths();
    c.load_constant(2);
    c.load_constant(2);
    for (unsigned word = 0; word < 10; ++word) {
        c.read_rom(2, true, true);
        c.store_ram(2, true, true);
        store_word(cartridge, 0x073e + 2 * word, read_word(data, 0x0472 + 2 * word));
        for (unsigned step = 0; step < 3; ++step) c.update_register();
        c.branch(word < 9);
    }
    c.load_constant(2);
    c.store_ram(2, true);
    store_word(cartridge, 0x1116, 0);
    c.restore_register();
    c.return_far();
}
// $83:9340; R-0075. Every rider has four 9:59.99 race bests and a zero stunt best per tour.
void initialize_rider_bests(Clock& c, Cartridge& cartridge) {
    c.call_far();
    c.save_register();
    c.change_widths();
    c.load_constant(2);
    c.load_constant(2);
    for (unsigned group = 0; group < 160; ++group) {
        c.load_constant(2);
        for (const auto offset : {0U, 2U, 6U, 8U}) {
            c.store_ram(2, true, true);
            store_word(cartridge, 0x0829 + group * 10 + offset, 0xea5f);
        }
        c.load_constant(2);
        c.store_ram(2, true, true);
        store_word(cartridge, 0x082d + group * 10, 0);
        c.update_register();
        c.update_register();
        c.load_constant(2);
        c.update_register();
        c.update_register();
        c.branch(group < 159);
    }
    c.restore_register();
    c.return_far();
}
// $83:94A5; R-0075. Write the cartridge signature and four initial option words.
void write_cartridge_header(Clock& c, Cartridge& cartridge,
                            std::span<const std::uint8_t, 1158> data) {
    c.call_far();
    c.save_register();
    c.change_widths();
    for (unsigned table = 0; table < 2; ++table) {
        c.load_constant(2);
        c.load_constant(2);
        const auto count = table == 0 ? 6U : 4U;
        const auto source = table == 0 ? 0U : 0x047cU;
        const auto destination = table == 0 ? 0U : 0x111cU;
        for (unsigned word = 0; word < count; ++word) {
            c.read_rom(2, true, true);
            c.store_ram(2, true, true);
            store_word(cartridge, destination + 2 * word, read_word(data, source + 2 * word));
            for (unsigned step = 0; step < 3; ++step) c.update_register();
            c.branch(word + 1 < count);
        }
    }
    c.restore_register();
    c.return_far();
}
// $83:90F4; R-0075. Each sum discards the previous carry, preserving uint16 wrapping.
void write_record_checksums(Clock& c, Cartridge& cartridge) {
    c.call_far();
    c.call_local();
    c.save_register();
    c.change_widths();
    constexpr std::array<std::array<unsigned, 3>, 9> groups{{{0x000c, 176, 0x016c},
                                                             {0x0230, 64, 0x02b0},
                                                             {0x016e, 96, 0x022e},
                                                             {0x02c0, 176, 0x0420},
                                                             {0x02b2, 6, 0x02be},
                                                             {0x0422, 150, 0x054e},
                                                             {0x0550, 75, 0x05e6},
                                                             {0x05e8, 170, 0x073c},
                                                             {0x0829, 800, 0x0e69}}};
    for (const auto& group : groups) {
        for (unsigned value = 0; value < 3; ++value) c.load_constant(2);
        std::uint16_t sum = 0;
        for (unsigned word = 0; word < group[1]; ++word) {
            c.update_register();
            c.read_ram(2, true, true);
            const auto address = group[0] + 2 * word;
            const auto value = cartridge[address] | unsigned(cartridge[address + 1]) << 8;
            sum = static_cast<std::uint16_t>(sum + value);
            for (unsigned step = 0; step < 3; ++step) c.update_register();
            c.branch(word + 1 < group[1]);
        }
        c.store_ram(2, true);
        store_word(cartridge, group[2], sum);
    }
    c.restore_register();
    c.return_local();
    c.return_far();
}
} // namespace
// $80:8C80; R-0075. Static defaults through8CA9, before the league pairing reset.
void native_audio_menu_first_record_defaults(Clock& c, Cartridge& cartridge,
                                             std::span<const std::uint8_t, 1158> defaults,
                                             std::span<const std::uint8_t, 50> track_types) {
    initialize_cold_track_work(c, cartridge, track_types);
    copy_default_table(c, cartridge, defaults, 0x000c, 0x000c, 352, 1);
    clear_default_words(c, cartridge, 0x0230, 64);
    copy_default_table(c, cartridge, defaults, 0x015e, 0x016e, 192, 1);
    clear_default_words(c, cartridge, 0x02c0, 176);
    copy_default_table(c, cartridge, defaults, 0x0222, 0x02b2, 6, 2);
    initialize_record_times(c, cartridge, defaults);
    initialize_record_holders(c, cartridge);
    c.read_ram(2, true);
    c.update_register();
    c.store_ram(2, true);
    const auto word =
        static_cast<std::uint16_t>(cartridge[0x074c] | unsigned(cartridge[0x074d]) << 8);
    store_word(cartridge, 0x074c, static_cast<std::uint16_t>(word - 1));
}
// $83:8B23; R-0075. Clear menu session words and the pending tour-level reveals.
void native_audio_reset_menu_selection(Clock& c, AudioCpuSceneWorkState& scene,
                                       Cartridge& cartridge) {
    c.call_far();
    c.save_register();
    c.change_widths();
    for (unsigned value = 0; value < 3; ++value) c.load_constant(2);
    for (unsigned word = 0; word < 16; ++word) {
        c.store_ram(2, true, true);
        store_word(cartridge, 0x0400 + word * 2, 0);
        for (unsigned step = 0; step < 3; ++step) c.update_register();
        c.branch(word < 15);
    }
    c.load_constant(2);
    c.store_ram(2, true);
    store_word(cartridge, 0x10ad, 0);
    scene.menu_mode = 0;
    c.change_widths();
    c.load_constant(2);
    c.load_constant(2);
    for (unsigned rider = 0; rider < 16; ++rider) {
        c.store_ram(1, true, true);
        cartridge[0x10fd + rider] = 0;
        c.update_register();
        c.update_register();
        c.branch(rider < 15);
    }
    c.restore_register();
    c.return_far();
}
// $80:8CB6; R-0075. Finish cold defaults, restore the outer caller, then reset the menu session.
void native_audio_finish_menu_records(Clock& c, AudioCpuSceneWorkState& scene, Cartridge& cartridge,
                                      std::span<const std::uint8_t, 1158> defaults) {
    initialize_settings(c, cartridge, defaults);
    initialize_rider_bests(c, cartridge);
    write_cartridge_header(c, cartridge, defaults);
    write_record_checksums(c, cartridge);
    c.load_constant(2);
    c.restore_register();
    c.return_local();
    native_audio_reset_menu_selection(c, scene, cartridge);
}
} // namespace unirally
