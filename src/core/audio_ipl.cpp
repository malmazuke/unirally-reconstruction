#include "audio_ipl.hpp"
#include <stdexcept>

namespace unirally {
namespace {
std::uint8_t byte(unsigned value) {
    return static_cast<std::uint8_t>(value);
}
}
AudioIplHandshake::AudioIplHandshake(AudioDriverBus& bus, std::uint64_t entry_ticks) : bus_(&bus) {
    state_.ticks = entry_ticks;
    state_.next_access_ticks = entry_ticks + 20;
}
void AudioIplHandshake::schedule(AudioIplPhase phase, unsigned ticks) {
    state_.phase = phase;
    state_.next_access_ticks = state_.ticks + ticks;
}
void AudioIplHandshake::run_until(std::uint64_t exclusive_ticks) {
    if (exclusive_ticks < state_.ticks) throw std::invalid_argument("IPL clock moves backwards");
    while (!driver_ready() && state_.next_access_ticks < exclusive_ticks) {
        state_.ticks = state_.next_access_ticks;
        step();
    }
}
void AudioIplHandshake::restore(const AudioIplState& state) {
    if (state.next_access_ticks < state.ticks || state.clear_index > 239
        || state.phase > AudioIplPhase::driver_ready
        || (state.phase == AudioIplPhase::clear_page && !state.clear_index))
        throw std::invalid_argument("invalid IPL pending state");
    state_ = state;
}
// Hardware IPL FFC0-FFCE clears EF..01, then announces AA/BB. The last
// clear uses a four-tick branch; port stores retain their dummy reads.
void AudioIplHandshake::clear_page() {
    bus_->write_ram(state_.ticks, state_.clear_index, 0);
    --state_.clear_index;
    schedule(state_.clear_index ? AudioIplPhase::clear_page : AudioIplPhase::ready_first_read,
             state_.clear_index ? 20U : 15U);
}
void AudioIplHandshake::ready_ports() {
    switch (state_.phase) {
    case AudioIplPhase::ready_first_read:
        bus_->read_port(state_.ticks, 0);
        schedule(AudioIplPhase::ready_first_write, 3);
        break;
    case AudioIplPhase::ready_first_write:
        bus_->write_port(state_.ticks, 0, 170);
        schedule(AudioIplPhase::ready_second_read, 7);
        break;
    case AudioIplPhase::ready_second_read:
        bus_->read_port(state_.ticks, 1);
        schedule(AudioIplPhase::ready_second_write, 3);
        break;
    case AudioIplPhase::ready_second_write:
        bus_->write_port(state_.ticks, 1, 187);
        schedule(AudioIplPhase::wait_request, 7);
        break;
    case AudioIplPhase::wait_request: {
        const bool requested = bus_->read_port(state_.ticks, 0) == 204;
        schedule(requested ? AudioIplPhase::destination_low : AudioIplPhase::wait_request,
                 requested ? 20U : 18U);
        break;
    }
    default: throw std::logic_error("invalid IPL ready phase");
    }
}
// FFEF-FFF9 reads two words, acknowledges the request and selects transfer
// or destination entry. The two port accesses of each word are four ticks apart.
void AudioIplHandshake::receive_header() {
    switch (state_.phase) {
    case AudioIplPhase::destination_low:
        state_.destination = bus_->read_port(state_.ticks, 2);
        schedule(AudioIplPhase::destination_high, 4);
        break;
    case AudioIplPhase::destination_high:
        state_.destination = static_cast<std::uint16_t>(
            state_.destination | (unsigned(bus_->read_port(state_.ticks, 3)) << 8));
        schedule(AudioIplPhase::header, 16);
        break;
    case AudioIplPhase::header:
        state_.header = bus_->read_port(state_.ticks, 0);
        schedule(AudioIplPhase::transfer_mode, 4);
        break;
    case AudioIplPhase::transfer_mode:
        state_.transfer_mode = bus_->read_port(state_.ticks, 1);
        schedule(AudioIplPhase::header_dummy_read, 6);
        break;
    case AudioIplPhase::header_dummy_read:
        bus_->read_port(state_.ticks, 0);
        schedule(AudioIplPhase::acknowledge_header, 3);
        break;
    case AudioIplPhase::acknowledge_header:
        bus_->write_port(state_.ticks, 0, state_.header);
        schedule(state_.transfer_mode ? AudioIplPhase::wait_first_byte
                                      : AudioIplPhase::enter_destination,
                 state_.transfer_mode ? 21U : 24U);
        break;
    default: throw std::logic_error("invalid IPL header phase");
    }
}
// FFDA/FFEB use the sign bit of wrapping Y-port subtraction, including the
// second comparison before interpreting a negative difference as an end marker.
void AudioIplHandshake::compare_byte(bool end_check) {
    const auto value = bus_->read_port(state_.ticks, 0);
    const auto difference = byte(unsigned(state_.byte_index) - value);
    const bool negative = (difference & 128) != 0;
    if (end_check)
        schedule(negative ? AudioIplPhase::destination_low : AudioIplPhase::compare_byte,
                 negative ? 10U : 14U);
    else if (!difference)
        schedule(AudioIplPhase::read_byte, 10);
    else
        schedule(negative ? AudioIplPhase::compare_end : AudioIplPhase::compare_byte,
                 negative ? 18U : 22U);
}
void AudioIplHandshake::receive_byte() {
    switch (state_.phase) {
    case AudioIplPhase::wait_first_byte:
        state_.byte_index = bus_->read_port(state_.ticks, 0);
        schedule(state_.byte_index ? AudioIplPhase::wait_first_byte : AudioIplPhase::compare_byte,
                 state_.byte_index ? 14U : 10U);
        break;
    case AudioIplPhase::read_byte:
        state_.transfer_byte = bus_->read_port(state_.ticks, 1);
        schedule(AudioIplPhase::byte_dummy_read, 6);
        break;
    case AudioIplPhase::byte_dummy_read:
        bus_->read_port(state_.ticks, 0);
        schedule(AudioIplPhase::acknowledge_byte, 3);
        break;
    case AudioIplPhase::acknowledge_byte:
        bus_->write_port(state_.ticks, 0, state_.byte_index);
        schedule(AudioIplPhase::store_byte, 14);
        break;
    default: throw std::logic_error("invalid IPL byte phase");
    }
}
void AudioIplHandshake::store_byte() {
    const auto address = static_cast<std::uint16_t>(state_.destination + state_.byte_index);
    // Uploaded driver code is not needed by native logic or this DSP-only RAM.
    // Only the separately identified score data is retained from IPL transfers.
    if ((address >= 0x1600 && address < 0x1600 + 621)
        || (address >= 0x1d00 && address < 0x1d00 + 2200))
        bus_->write_ram(state_.ticks, address, state_.transfer_byte);
    state_.byte_index = byte(state_.byte_index + 1U);
    if (state_.byte_index) {
        schedule(AudioIplPhase::compare_byte, 17);
        return;
    }
    state_.destination = static_cast<std::uint16_t>(state_.destination + 256U);
    const bool negative = (state_.destination & 0x8000) != 0;
    schedule(negative ? AudioIplPhase::compare_end : AudioIplPhase::compare_byte,
             negative ? 25U : 29U);
}
void AudioIplHandshake::enter_destination() {
    if (state_.destination == 0xffc0) {
        state_.clear_index = 239;
        schedule(AudioIplPhase::clear_page, 20);
    } else if (state_.destination == 0x0400)
        schedule(AudioIplPhase::driver_ready, 0);
    else
        throw std::runtime_error("IPL destination outside identified title/menu protocol");
}
void AudioIplHandshake::step() {
    const auto phase = state_.phase;
    if (phase == AudioIplPhase::clear_page)
        clear_page();
    else if (phase <= AudioIplPhase::wait_request)
        ready_ports();
    else if (phase <= AudioIplPhase::acknowledge_header)
        receive_header();
    else if (phase == AudioIplPhase::compare_byte || phase == AudioIplPhase::compare_end)
        compare_byte(phase == AudioIplPhase::compare_end);
    else if (phase == AudioIplPhase::store_byte)
        store_byte();
    else if (phase == AudioIplPhase::enter_destination)
        enter_destination();
    else
        receive_byte();
}
} // namespace unirally
