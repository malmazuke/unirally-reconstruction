#include "audio_cpu_scene.hpp"

namespace unirally {
namespace {
using Clock = AudioCpuWorkClock;
bool decrement(Clock& c, std::uint8_t& counter, std::uint8_t last, bool direct, bool accumulator) {
    if (accumulator) {
        c.read_ram();
        c.update_register();
    } else if (direct) {
        c.modify_direct_byte();
    } else {
        c.modify_ram_byte();
    }
    counter = static_cast<std::uint8_t>(counter - 1U);
    const bool wrapped = (counter & 128) != 0;
    c.branch(!wrapped);
    if (wrapped) {
        c.load_constant();
        counter = last;
        if (!accumulator) {
            if (direct)
                c.store_direct();
            else
                c.store_ram();
        }
    }
    if (accumulator) c.store_ram();
    return wrapped;
}
void pair_tiles(Clock& c) {
    for (unsigned pair = 0; pair < 2; ++pair) {
        c.change_widths();
        c.read_ram(2);
        c.load_constant(2);
        c.update_register();
        c.change_widths();
        c.read_rom(1, true, true);
        c.store_ram();
        c.store_ram();
    }
}
void wave_step(Clock& c, AudioCpuDecorationWorkState& state) {
    c.load_constant(2);
    c.load_constant(2);
    for (unsigned i = 0; i < 8; ++i) {
        auto& counter = state.wave[7 - i];
        c.read_ram(1, false, true);
        c.exchange_accumulator_bytes();
        c.load_constant();
        c.exchange_accumulator_bytes();
        c.update_register();
        counter = static_cast<std::uint8_t>(counter - 1U);
        const bool wrapped = (counter & 128) != 0;
        c.branch(!wrapped);
        if (wrapped) {
            c.load_constant();
            counter = 19;
        }
        c.store_ram(1, false, true);
        c.save_register(2);
        c.update_register();
        c.read_rom(1, true, true);
        c.restore_register(2);
        c.store_ram(1, false, true);
        c.update_register();
        c.update_register();
        c.update_register();
        c.update_register();
        c.update_register();
        c.branch(i < 7);
    }
    c.change_widths();
    decrement(c, state.sway, 19, false, true);
    c.update_register();
    for (unsigned i = 0; i < 4; ++i) {
        c.load_constant();
        c.update_register();
        c.read_rom(1, true, true);
        c.store_ram();
    }
}
} // namespace
// $83:9A1E-9AF8; R-0075 static listing, R-0055 counters/initial wave.
void native_audio_step_decorations(Clock& c, AudioCpuDecorationWorkState& state) {
    c.save_register();
    c.save_register(2);
    c.save_register();
    c.change_widths();
    if (decrement(c, state.delay, 2, true, false)) {
        if (decrement(c, state.pair_step, 7, false, false))
            decrement(c, state.pair_cycle, 7, false, false);
        pair_tiles(c);
        decrement(c, state.trio_step, 7, false, true);
        c.change_widths();
        c.load_constant(2);
        c.update_register();
        c.change_widths();
        for (unsigned i = 0; i < 3; ++i) {
            c.read_rom(1, true, true);
            c.store_ram();
        }
    }
    if (decrement(c, state.wave_delay, 1, true, false)) wave_step(c, state);
    c.restore_register();
    c.restore_register(2);
    c.restore_register();
    c.return_far();
}
} // namespace unirally
