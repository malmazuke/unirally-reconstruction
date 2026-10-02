#pragma once
#include "audio_driver.hpp"

namespace unirally {
// Named hardware-upload phases, not an SPC instruction reader. R-0075.
// All clocks are absolute default SMP ticks. A pending access can be saved
// before it runs; driver entry is computed by the handshake, never supplied.
enum class AudioIplPhase : std::uint8_t {
    clear_page,
    ready_first_read,
    ready_first_write,
    ready_second_read,
    ready_second_write,
    wait_request,
    destination_low,
    destination_high,
    header,
    transfer_mode,
    header_dummy_read,
    acknowledge_header,
    wait_first_byte,
    compare_byte,
    compare_end,
    read_byte,
    byte_dummy_read,
    acknowledge_byte,
    store_byte,
    enter_destination,
    driver_ready
};
struct AudioIplState {
    std::uint64_t ticks = 0, next_access_ticks = 20;
    AudioIplPhase phase = AudioIplPhase::clear_page;
    std::uint16_t destination = 0;
    std::uint8_t clear_index = 239, byte_index = 0;
    std::uint8_t header = 0, transfer_mode = 0, transfer_byte = 0;
};

class AudioIplHandshake {
public:
    explicit AudioIplHandshake(AudioDriverBus& bus, std::uint64_t entry_ticks = 0);
    void run_until(std::uint64_t exclusive_ticks);
    const AudioIplState& state() const { return state_; }
    void restore(const AudioIplState& state);
    bool driver_ready() const { return state_.phase == AudioIplPhase::driver_ready; }
    // The current sound set's identified table/score lengths, retained in DSP
    // RAM from the transfers to 1600 and 1D00. Defaults: the title/menu set.
    void retain_sound_set(std::uint16_t table_bytes, std::uint16_t score_bytes);

private:
    AudioDriverBus* bus_;
    AudioIplState state_;
    std::uint16_t retained_table_bytes_ = 621, retained_score_bytes_ = 2200;
    void schedule(AudioIplPhase phase, unsigned ticks);
    void step();
    void clear_page();
    void ready_ports();
    void receive_header();
    void receive_byte();
    void compare_byte(bool end_check);
    void store_byte();
    void enter_destination();
};
} // namespace unirally
