#include "audio_driver.hpp"
#include <array>
#include <stdexcept>

namespace unirally {
namespace {
constexpr std::uint16_t sample_directory = 0xff00, sample_bytes_begin = 0x3000;
constexpr std::uint16_t sample_fraction_table = 0xfe80, sample_transpose_table = 0xfec0;
constexpr unsigned sample_slots = 64;
constexpr std::array<std::array<std::uint8_t, 2>, 29> initial_dsp_registers{
    {{0x6c, 243}, {0x0c, 0}, {0x1c, 0}, {0x2c, 0}, {0x3c, 0},   {0x5c, 255}, {0x2d, 0}, {0x3d, 0},
     {0x4d, 0},   {0x7d, 0}, {0x6d, 3}, {0x0d, 0}, {0x5d, 255}, {0x0f, 127}, {0x1f, 0}, {0x2f, 0},
     {0x3f, 0},   {0x4f, 0}, {0x5f, 0}, {0x6f, 0}, {0x7f, 0},   {0x05, 0},   {0x15, 0}, {0x25, 0},
     {0x35, 0},   {0x45, 0}, {0x55, 0}, {0x65, 0}, {0x75, 0}}};
std::uint8_t byte(unsigned value) {
    return static_cast<std::uint8_t>(value);
}
std::uint16_t word(unsigned value) {
    return static_cast<std::uint16_t>(value);
}
bool negative_difference(std::uint8_t expected, std::uint8_t actual) {
    return (byte(unsigned(expected) - actual) & 128) != 0;
}
}
void TitleMenuAudioDriver::write_ram(unsigned ticks, std::uint16_t address, std::uint8_t value) {
    advance(ticks);
    bus_->write_ram(ticks_, address, value);
}
// 137D-138E: identified register/value initialization table, followed by its
// negative sentinel. The table contains hardware constants, not instructions.
void TitleMenuAudioDriver::initialize_dsp() {
    advance(4);
    for (const auto& pair : initial_dsp_registers) {
        write_dsp(10 + 4 + 8 + 10 + 8, pair[0], pair[1]);
        advance(4 + 8);
    }
    advance(10 + 8 + 10);
}
// 08CA-0914. Clear the two voice pages and direct-page state. Preserve the
// original five-byte trailing clear and its single count initialization.
void TitleMenuAudioDriver::initialize_voice_ram() {
    advance(8);
    for (unsigned index = 0; index < 240; ++index) {
        write_ram(12, word(index), 0);
        write_ram(12, word(0x100 + index), 0);
        advance(4 + 4 + (index == 239 ? 4U : 8U));
    }
    advance(4);
    write_ram(12, 0x170, 1);
    advance(4 + 4 + 4);
    advance(8);
    std::uint8_t offset = 0;
    do {
        write_ram(12, word(0x200U + offset), 0);
        write_ram(12, word(0x300U + offset), 0);
        offset = byte(unsigned(offset) - 1U);
        advance(4 + (offset ? 8U : 4U));
    } while (offset);
    advance(40 + 4);
    for (unsigned index = 0; index < 5; ++index) {
        advance(4);
        write_ram(14, word(0x300 + index), 0);
        advance(4 + 8 + 8 + 6 + 4 + (index == 4 ? 4U : 8U));
    }
    advance(8 + 6 + 4 + 4 + 10);
}
// 154D-1556. Each byte is read only after the incoming phase equals the
// expected byte. Acknowledgment precedes incrementing that wrapping phase.
std::uint8_t TitleMenuAudioDriver::read_upload_byte(std::uint8_t& phase) {
    advance(16);
    while (true) {
        const auto received = read_port(2);
        advance(received == phase ? 4U : 8U);
        if (received == phase) break;
    }
    const auto value = read_port(3);
    write_port(2, phase);
    phase = byte(phase + 1U);
    advance(4 + 10);
    return value;
}
// 151D-154C. Byte blocks end on two negative phase differences. The low-byte
// destination cursor wraps separately, and only then increments its high byte.
void TitleMenuAudioDriver::load_sample_bytes(std::uint8_t& phase, std::uint16_t& cursor) {
    advance(16 + 4);
    std::uint8_t offset = 0;
    while (true) {
        const auto received = read_port(2);
        if (received == phase) {
            advance(4);
            const auto value = read_port(3);
            write_port(2, phase);
            write_ram(12, word(cursor + offset), value);
            phase = byte(phase + 1U);
            offset = byte(offset + 1U);
            advance(4 + 4 + (offset ? 8U : 4U));
            if (!offset) {
                advance(10 + 8);
                cursor = word(cursor + 256U);
            }
            continue;
        }
        advance(8);
        const bool first = negative_difference(phase, read_port(2));
        advance(first ? 4U : 8U);
        if (!first) continue;
        const bool second = negative_difference(phase, read_port(2));
        advance(second ? 4U : 8U);
        if (!second) continue;
        phase = byte(phase + 1U);
        advance(4);
        write_port(2, phase);
        phase = byte(phase + 1U);
        advance(4 + 4 + 4 + 8 + 10);
        const unsigned low = (cursor & 255U) + offset;
        cursor = word(cursor + offset);
        advance(low > 255 ? 4U + 10 : 8U);
        advance(10);
        return;
    }
}
// 14BD-151C. Build the 64-entry sample directory and receive selected BRR
// blocks. Fraction and transpose are data, copied independently of the DSP.
void TitleMenuAudioDriver::load_samples() {
    advance(36);
    std::uint8_t phase = 129;
    std::uint16_t cursor = sample_bytes_begin;
    for (unsigned slot = 0; slot < sample_slots; ++slot) {
        const auto sample = read_upload_byte(phase);
        write_port(1, sample);
        advance(4 + (sample == 255 ? 8U : 4U));
        if (sample != 255) {
            const auto low = read_upload_byte(phase);
            const unsigned sum = unsigned(low) + (cursor & 255U);
            advance(4 + 8);
            write_ram(12, word(sample_directory + slot * 4 + 2), byte(sum));
            advance(8);
            const auto high = read_upload_byte(phase);
            advance(8 + 8);
            write_ram(12, word(sample_directory + slot * 4 + 3),
                      byte(unsigned(high) + (cursor >> 8) + (sum >> 8)));
            advance(8);
            write_ram(12, word(sample_directory + slot * 4), byte(cursor));
            advance(8);
            write_ram(12, word(sample_directory + slot * 4 + 1), byte(cursor >> 8));
            advance(8 + 4 + 4 + 4 + 4);
            const auto fraction = read_upload_byte(phase);
            write_ram(12, word(sample_fraction_table + slot), fraction);
            const auto transpose = read_upload_byte(phase);
            write_ram(12, word(sample_transpose_table + slot), transpose);
            load_sample_bytes(phase, cursor);
            advance(8);
        }
        advance(16);
        write_port(0, byte((slot + 1U) * 4));
        advance(4 + (slot == sample_slots - 1 ? 4U : 8U));
    }
    while (true) {
        advance(2);
        const bool ready = read_port(2) == 128;
        advance(2);
        advance(ready ? 4U : 8U);
        if (ready) break;
    }
    advance(2);
    write_port(2, 128);
    advance(10);
}
// 0400-0448. Native driver initialization after the IPL's final transfer.
// The final-transfer tick is still a declared laboratory condition.
void TitleMenuAudioDriver::boot() {
    advance(16 + 10);
    timers_.write_control(48);
    bus_->clear_ports(ticks_, 0);
    bus_->clear_ports(ticks_, 2);
    advance(16);
    initialize_dsp();
    advance(16);
    initialize_voice_ram();
    advance(70 + 4);
    write_port(2, 128);
    while (true) {
        const bool ready = read_port(2) == 128;
        advance(ready ? 4U : 8U);
        if (ready) break;
    }
    advance(4);
    write_port(0, 0);
    write_port(1, 0);
    advance(16);
    load_samples();
    advance(16);
    configure_timers();
    advance(16 + 10 + 10 + 4 + 10 + 10);
    write_dsp(20, 0x6c, 51);
    advance(4 + 16 + score_.start_music_timed(0));
    advance(10);
}
} // namespace unirally
