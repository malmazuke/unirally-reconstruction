#include "audio_driver.hpp"
#include <algorithm>
#include <stdexcept>

namespace unirally {
namespace {
constexpr std::uint8_t counter_threshold = 4, counter_clip = 6;
constexpr std::uint8_t normal_master_volume = 127, music_timer_target = 133,
                       effect_timer_target = 20;
std::uint8_t byte(unsigned value) {
    return static_cast<std::uint8_t>(value);
}
}
TitleMenuAudioDriver::TitleMenuAudioDriver(const TitleMenuAudioData& data,
                                           const AudioPitchData& pitch, AudioDriverBus& bus,
                                           const AudioTimersState& timers,
                                           std::uint64_t entry_ticks, bool driver_boot)
    : score_(data, &pitch), bus_(&bus), ticks_(entry_ticks) {
    if (timers.ticks > entry_ticks) throw std::invalid_argument("timer phase follows driver entry");
    timers_.restore(timers);
    timers_.advance_to(entry_ticks);
    if (driver_boot)
        boot();
    else
        score_.start_music(0);
}
void TitleMenuAudioDriver::advance(unsigned ticks) {
    ticks_ += ticks;
    timers_.advance_to(ticks_);
}
std::uint8_t TitleMenuAudioDriver::read_port(std::uint8_t port) {
    advance(5);
    const auto value = bus_->read_port(ticks_, port);
    advance(1);
    return value;
}
void TitleMenuAudioDriver::write_port(std::uint8_t port, std::uint8_t value) {
    advance(5);
    bus_->read_port(ticks_, port); // A direct-page store includes the original dummy read.
    advance(3);
    bus_->write_port(ticks_, port, value);
}
void TitleMenuAudioDriver::write_dsp(unsigned ticks, std::uint8_t reg, std::uint8_t value) {
    advance(ticks);
    bus_->write_dsp(ticks_, reg, value);
}
// 04BA-04C6. Music changes reconfigure both timers, preserving physical phase.
void TitleMenuAudioDriver::configure_timers() {
    advance(10);
    timers_.write_control(0);
    advance(10);
    timers_.write_target(2, music_timer_target);
    advance(10);
    timers_.write_target(1, effect_timer_target);
    advance(10);
    timers_.write_control(6);
    advance(10);
}
// 0626-063A, 066E-0673. Unrecovered commands fail at the domain boundary.
void TitleMenuAudioDriver::execute_command(std::uint8_t command, std::uint8_t parameter) {
    switch (command) {
    case 1:
        advance(24);
        advance(score_.start_music_timed(byte(parameter - 1U)));
        master_volume_ = normal_master_volume;
        advance(16);
        configure_timers();
        advance(14);
        break;
    case 2:
        advance(16 + score_.start_effect_timed(parameter));
        master_volume_ = normal_master_volume;
        advance(34);
        break;
    case 7:
    case 8:
        score_.set_volume_gain(command == 8, parameter);
        advance(18);
        break;
    default: throw std::runtime_error("command outside recovered title/menu driver");
    }
}
// 0572-0596, 05B8/0606. Poll twice for a stable header, then acknowledge its
// two-bit phase before executing the command. CPU port reads split 1+1 ticks.
void TitleMenuAudioDriver::poll_commands() {
    advance(16);
    if (read_port(0)) throw std::runtime_error("bulk upload/stop remains outside driver loop");
    advance(8 + 4 + 10);
    std::uint8_t first = 0, second = 0;
    do {
        first = read_port(2);
        second = read_port(2);
        advance(first == second ? 4U : 8U);
    } while (first != second);
    const auto phase = byte(first & 192);
    advance(4 + 6);
    if (phase == command_phase_) {
        advance(8 + 4 + 10);
        return;
    }
    advance(4 + 4 + 8);
    command_phase_ = phase;
    const auto header = read_port(2);
    advance(4 + 4 + 10 + 8 + 10 + 8);
    const auto parameter = read_port(3);
    write_port(2, phase);
    advance(4 + 10);
    execute_command(byte(header & 63), parameter);
}
// 0915-0992. Apply tempo writes at their native work offsets, then poll.
void TitleMenuAudioDriver::update_score_voice(std::uint8_t voice, bool effects) {
    advance(20);
    const auto work = score_.update_voice_timed(voice, effects, update_counter_);
    unsigned elapsed = 0;
    for (const auto& target : work.timer2_writes) {
        advance(target.ticks - elapsed);
        timers_.write_target(2, target.value);
        elapsed = target.ticks;
    }
    advance(work.ticks - elapsed);
    if (work.polls_commands) poll_commands();
}
// 1312-137C. Output all six registers in order; commands can arrive between
// voices, but not during one voice's volume/pitch calculation.
void TitleMenuAudioDriver::output_voice(std::uint8_t voice) {
    advance(24);
    const auto work = score_.voice_register_work(voice);
    const auto values = score_.voice_register_values(voice);
    constexpr std::array<std::uint8_t, 6> registers{0, 1, 2, 3, 4, 7};
    unsigned elapsed = 0;
    if (work.writes_registers)
        for (unsigned index = 0; index < registers.size(); ++index) {
            write_dsp(work.write_ticks[index] - elapsed,
                      byte(unsigned(voice) * 16 + registers[index]), values[index]);
            elapsed = work.write_ticks[index];
        }
    advance(work.ticks_to_poll - elapsed);
    poll_commands();
}
// 067E-074A. Both update modes share the counter and output pass. The bounded
// title/menu domain has no global fade/ramp or noise command.
void TitleMenuAudioDriver::update_voices(bool effects) {
    advance(8 + 6);
    update_counter_ = byte(update_counter_ + 1U);
    write_port(3, update_counter_);
    advance(6 + (effects ? 8U : 4U));
    if (!effects) {
        advance(6 + 8 + 10 + 8);
        write_dsp(20, 0x4c, score_.take_key_on_pending());
        advance(10);
        write_dsp(20, 0x3d, 0);
    }
    for (std::uint8_t voice = 0; voice < 8; ++voice) update_score_voice(voice, effects);
    write_dsp(34, 0x5c, score_.take_key_off_pending());
    advance(10);
    write_dsp(24, 0x0c, master_volume_);
    write_dsp(18, 0x1c, master_volume_);
    for (std::uint8_t voice = 0; voice < 8; ++voice) output_voice(voice);
    write_dsp(20, 0x5c, 0);
    advance(10);
}
// 0448-0486. Unsigned byte accumulators clip at six before subtracting four.
// The effect counter is independent of the music tempo target.
void TitleMenuAudioDriver::iteration() {
    advance(16);
    poll_commands();
    advance(18);
    music_counter_ = byte(music_counter_ + timers_.read_output(2));
    advance(10 + 4);
    if (music_counter_ < counter_threshold)
        advance(8);
    else {
        advance(4 + 4 + 8 + 4 + 4 + 10 + 16);
        music_counter_ = byte(std::min(music_counter_, counter_clip) - counter_threshold);
        update_voices(false);
    }
    advance(16);
    effect_counter_ = byte(effect_counter_ + timers_.read_output(1));
    advance(8 + 4);
    if (effect_counter_ < counter_threshold)
        advance(8);
    else {
        advance(4 + 4 + 8 + 4 + 4 + 8 + 8 + 16);
        effect_counter_ = byte(std::min(effect_counter_, counter_clip) - counter_threshold);
        update_voices(true);
        advance(8 + 6);
    }
}
void TitleMenuAudioDriver::run_until(std::uint64_t ticks) {
    while (ticks_ < ticks) iteration();
}
} // namespace unirally
