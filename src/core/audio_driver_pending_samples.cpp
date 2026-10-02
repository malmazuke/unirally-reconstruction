#include "audio_driver.hpp"
#include <stdexcept>

namespace unirally {
namespace {
std::uint8_t byte(unsigned value) {
    return static_cast<std::uint8_t>(value);
}
std::uint16_t word(unsigned value) {
    return static_cast<std::uint16_t>(value);
}
bool negative(std::uint8_t expected, std::uint8_t actual) {
    return (byte(unsigned(expected) - actual) & 128) != 0;
}
}
// R-0075, 14BD-14F6. Only identified sample metadata enters native state.
void TitleMenuAudioDriver::plan_sample_phase() {
    auto& state = continuation_;
    const auto value = state.port_reads[3];
    const auto directory = word(0xff00U + unsigned(state.slot) * 4);
    switch (state.phase) {
    case AudioDriverPhase::slot_received:
        state.sample = value;
        write_port(1, value);
        advance(4 + (value == 255 ? 8U : 4U));
        if (value == 255)
            finish_plan(AudioDriverPhase::slot_finished);
        else
            plan_descriptor(AudioDriverPhase::loop_low_received);
        break;
    case AudioDriverPhase::loop_low_received:
        state.loop_sum = word(unsigned(value) + (state.sample_cursor & 255U));
        advance(12);
        write_ram(12, word(directory + 2U), byte(state.loop_sum));
        advance(8);
        plan_descriptor(AudioDriverPhase::loop_high_received);
        break;
    case AudioDriverPhase::loop_high_received:
        advance(16);
        write_ram(12, word(directory + 3U),
                  byte(unsigned(value) + (state.sample_cursor >> 8) + (state.loop_sum >> 8)));
        advance(8);
        write_ram(12, directory, byte(state.sample_cursor));
        advance(8);
        write_ram(12, word(directory + 1U), byte(state.sample_cursor >> 8));
        advance(24);
        plan_descriptor(AudioDriverPhase::fraction_received);
        break;
    case AudioDriverPhase::fraction_received:
        write_ram(12, word(0xfe80U + state.slot), value);
        pitch_.sample_fraction.at(state.slot) = value;
        plan_descriptor(AudioDriverPhase::transpose_received);
        break;
    case AudioDriverPhase::transpose_received:
        write_ram(12, word(0xfec0U + state.slot), value);
        pitch_.sample_transpose.at(state.slot) = value;
        finish_plan(AudioDriverPhase::bulk_begin);
        break;
    case AudioDriverPhase::slot_finished:
        advance(16);
        write_port(0, byte((unsigned(state.slot) + 1) * 4));
        advance(4 + (state.slot == 63 ? 4U : 8U));
        ++state.slot;
        if (state.slot == 64)
            finish_plan(AudioDriverPhase::samples_ready);
        else
            plan_descriptor(AudioDriverPhase::slot_received);
        break;
    default: throw std::logic_error("invalid native sample phase");
    }
}
// R-0075, 151D-154C. A negative two-read difference terminates a block.
// The byte cursor and page carry retain separate wrapping integer semantics.
void TitleMenuAudioDriver::plan_bulk_phase() {
    auto& state = continuation_;
    const auto received = state.port_reads[2];
    switch (state.phase) {
    case AudioDriverPhase::bulk_begin:
        advance(20);
        state.sample_offset = 0;
        plan_read_port(2);
        finish_plan(AudioDriverPhase::bulk_compare);
        break;
    case AudioDriverPhase::bulk_compare:
        if (received == state.upload_phase) {
            advance(4);
            plan_read_port(3);
            write_port(2, state.upload_phase);
            finish_plan(AudioDriverPhase::bulk_byte_received);
        } else {
            advance(8);
            plan_read_port(2);
            finish_plan(AudioDriverPhase::bulk_negative_first);
        }
        break;
    case AudioDriverPhase::bulk_byte_received:
        write_ram(12, word(state.sample_cursor + state.sample_offset), state.port_reads[3]);
        ++state.upload_phase;
        ++state.sample_offset;
        advance(8 + (state.sample_offset ? 8U : 4U));
        if (!state.sample_offset) {
            advance(18);
            state.sample_cursor = word(state.sample_cursor + 256U);
        }
        plan_read_port(2);
        finish_plan(AudioDriverPhase::bulk_compare);
        break;
    case AudioDriverPhase::bulk_negative_first:
    case AudioDriverPhase::bulk_negative_second: {
        const bool is_negative = negative(state.upload_phase, received);
        advance(is_negative ? 4U : 8U);
        if (!is_negative) {
            plan_read_port(2);
            finish_plan(AudioDriverPhase::bulk_compare);
        } else if (state.phase == AudioDriverPhase::bulk_negative_first) {
            plan_read_port(2);
            finish_plan(AudioDriverPhase::bulk_negative_second);
        } else {
            ++state.upload_phase;
            advance(4);
            write_port(2, state.upload_phase);
            ++state.upload_phase;
            advance(30);
            const auto low = (state.sample_cursor & 255U) + state.sample_offset;
            state.sample_cursor = word(state.sample_cursor + state.sample_offset);
            advance(low > 255 ? 14U : 8U);
            advance(10);
            finish_plan(AudioDriverPhase::bulk_after);
        }
        break;
    }
    case AudioDriverPhase::bulk_after:
        advance(8);
        finish_plan(AudioDriverPhase::slot_finished);
        break;
    default: throw std::logic_error("invalid native sample transfer phase");
    }
}
} // namespace unirally
