#include "audio_driver.hpp"
#include <algorithm>
#include <stdexcept>

namespace unirally {
namespace {
std::uint8_t byte(unsigned value) {
    return static_cast<std::uint8_t>(value);
}
}
void TitleMenuAudioDriver::plan_poll(AudioDriverPhase return_phase) {
    continuation_.poll_return = return_phase;
    advance(16);
    plan_read_port(0);
    finish_plan(AudioDriverPhase::poll_bulk_check);
}
// R-0075, 0572-0596, 05B8/0606. Decisions use completed hardware reads.
void TitleMenuAudioDriver::plan_poll_phase() {
    auto& state = continuation_;
    switch (state.phase) {
    case AudioDriverPhase::poll_bulk_check:
        if (state.port_reads[0]) {
            advance(4);
            plan_read_port(0);
            finish_plan(AudioDriverPhase::poll_bulk_confirm);
        } else {
            advance(22);
            plan_read_port(2);
            finish_plan(AudioDriverPhase::poll_header_first);
        }
        break;
    case AudioDriverPhase::poll_bulk_confirm:
        if (state.port_reads[0] != 255)
            throw std::runtime_error("bulk transfer outside recovered driver loop");
        advance(8);
        write_port(0, 255);
        advance(14);
        finish_plan(AudioDriverPhase::stop_begin);
        break;
    case AudioDriverPhase::poll_header_first:
        state.first_header = state.port_reads[2];
        plan_read_port(2);
        finish_plan(AudioDriverPhase::poll_header_compare);
        break;
    case AudioDriverPhase::poll_header_compare: {
        if (state.first_header != state.port_reads[2]) {
            advance(8);
            plan_read_port(2);
            finish_plan(AudioDriverPhase::poll_header_first);
            break;
        }
        advance(14);
        const auto phase = byte(state.first_header & 192);
        if (phase == command_phase_) {
            advance(22);
            finish_plan(state.poll_return);
            break;
        }
        advance(16);
        command_phase_ = phase;
        plan_read_port(2);
        finish_plan(AudioDriverPhase::poll_header_captured);
        break;
    }
    case AudioDriverPhase::poll_header_captured:
        state.command_header = state.port_reads[2];
        advance(44);
        plan_read_port(3);
        write_port(2, command_phase_);
        advance(14);
        finish_plan(AudioDriverPhase::execute_command);
        break;
    case AudioDriverPhase::execute_command:
        execute_command(byte(state.command_header & 63), state.port_reads[3]);
        finish_plan(state.poll_return);
        break;
    default: throw std::logic_error("invalid native command poll phase");
    }
}
// R-0075, 0448-0486: separate byte accumulators and physical timers.
void TitleMenuAudioDriver::plan_main_phase() {
    auto& state = continuation_;
    switch (state.phase) {
    case AudioDriverPhase::iteration:
        advance(16);
        plan_poll(AudioDriverPhase::music_timer);
        break;
    case AudioDriverPhase::music_timer:
        advance(18);
        plan_read_timer(2);
        finish_plan(AudioDriverPhase::music_counter);
        break;
    case AudioDriverPhase::music_counter:
        music_counter_ = byte(music_counter_ + state.timer_reads[2]);
        advance(14);
        if (music_counter_ < 4) {
            advance(8);
            finish_plan(AudioDriverPhase::effect_timer);
            break;
        }
        advance(50);
        music_counter_ = byte(std::min(music_counter_, std::uint8_t{6}) - 4U);
        state.effects = false;
        finish_plan(AudioDriverPhase::update_begin);
        break;
    case AudioDriverPhase::effect_timer:
        advance(16);
        plan_read_timer(1);
        finish_plan(AudioDriverPhase::effect_counter);
        break;
    case AudioDriverPhase::effect_counter:
        effect_counter_ = byte(effect_counter_ + state.timer_reads[1]);
        advance(12);
        if (effect_counter_ < 4) {
            advance(8);
            finish_plan(AudioDriverPhase::iteration);
            break;
        }
        advance(56);
        effect_counter_ = byte(std::min(effect_counter_, std::uint8_t{6}) - 4U);
        state.effects = true;
        finish_plan(AudioDriverPhase::update_begin);
        break;
    default: plan_poll_phase();
    }
}
// R-0075, 067E-074A, 0915-0992 and 1312-137C. Native score calculations
// produce bounded hardware work, and each command poll resumes its own pass.
void TitleMenuAudioDriver::plan_update_phase() {
    auto& state = continuation_;
    switch (state.phase) {
    case AudioDriverPhase::update_begin:
        advance(14);
        update_counter_ = byte(update_counter_ + 1U);
        write_port(3, update_counter_);
        advance(6 + (state.effects ? 8U : 4U));
        if (!state.effects) {
            advance(32);
            write_dsp(20, 0x4c, score_.take_key_on_pending());
            advance(10);
            write_dsp(20, 0x3d, 0);
        }
        state.voice = 0;
        finish_plan(AudioDriverPhase::score_voice);
        break;
    case AudioDriverPhase::score_voice: {
        advance(20);
        const auto work = score_.update_voice_timed(state.voice, state.effects, update_counter_);
        unsigned elapsed = 0;
        for (const auto& target : work.timer2_writes) {
            advance(target.ticks - elapsed);
            write_target(2, target.value);
            elapsed = target.ticks;
        }
        advance(work.ticks - elapsed);
        if (work.polls_commands)
            plan_poll(AudioDriverPhase::score_voice_next);
        else
            finish_plan(AudioDriverPhase::score_voice_next);
        break;
    }
    case AudioDriverPhase::score_voice_next:
        ++state.voice;
        if (state.voice < 8) {
            finish_plan(AudioDriverPhase::score_voice);
            break;
        }
        state.voice = 0;
        write_dsp(34, 0x5c, score_.take_key_off_pending());
        advance(10);
        write_dsp(24, 0x0c, master_volume_);
        write_dsp(18, 0x1c, master_volume_);
        finish_plan(AudioDriverPhase::output_voice);
        break;
    case AudioDriverPhase::output_voice: plan_output_voice(); break;
    case AudioDriverPhase::output_voice_next:
        ++state.voice;
        if (state.voice < 8) {
            finish_plan(AudioDriverPhase::output_voice);
            break;
        }
        state.voice = 0;
        write_dsp(20, 0x5c, 0);
        advance(10);
        finish_plan(AudioDriverPhase::update_end);
        break;
    case AudioDriverPhase::update_end:
        if (state.effects) {
            advance(14);
            finish_plan(AudioDriverPhase::iteration);
        } else
            finish_plan(AudioDriverPhase::effect_timer);
        break;
    default: throw std::logic_error("invalid native voice update phase");
    }
}
// R-0075, 1312-137C: six register writes precede the per-voice command poll.
void TitleMenuAudioDriver::plan_output_voice() {
    const auto& state = continuation_;
    advance(24);
    const auto work = score_.voice_register_work(state.voice);
    const auto values = score_.voice_register_values(state.voice);
    constexpr std::array<std::uint8_t, 6> registers{0, 1, 2, 3, 4, 7};
    unsigned elapsed = 0;
    if (work.writes_registers)
        for (unsigned index = 0; index < registers.size(); ++index) {
            write_dsp(work.write_ticks[index] - elapsed,
                      byte(unsigned(state.voice) * 16 + registers[index]), values[index]);
            elapsed = work.write_ticks[index];
        }
    advance(work.ticks_to_poll - elapsed);
    plan_poll(AudioDriverPhase::output_voice_next);
}
// R-0075, 0487-04B9: nested polls all unwind into one native stop phase.
void TitleMenuAudioDriver::plan_stop_phase() {
    auto& state = continuation_;
    switch (state.phase) {
    case AudioDriverPhase::stop_begin:
        advance(6);
        plan_read_timer(2);
        advance(6);
        state.stop_volume = master_volume_;
        finish_plan(AudioDriverPhase::stop_wait);
        break;
    case AudioDriverPhase::stop_wait:
        advance(6);
        plan_read_timer(2);
        finish_plan(AudioDriverPhase::stop_pulse);
        break;
    case AudioDriverPhase::stop_pulse:
        if (!state.timer_reads[2]) {
            advance(8);
            finish_plan(AudioDriverPhase::stop_wait);
            break;
        }
        advance(20);
        state.stop_volume = state.stop_volume < 8 ? 0 : byte(state.stop_volume - 8U);
        write_dsp(18, 0x0c, state.stop_volume);
        write_dsp(18, 0x1c, state.stop_volume);
        advance(4 + (state.stop_volume ? 8U : 4U));
        finish_plan(state.stop_volume ? AudioDriverPhase::stop_wait
                                      : AudioDriverPhase::stop_finish);
        break;
    case AudioDriverPhase::stop_finish:
        write_port(2, 0);
        write_dsp(22, 0x5c, 255);
        advance(12);
        write_control(176);
        clear_ports(0);
        clear_ports(2);
        write_dsp(20, 0x6c, 224);
        advance(6);
        finish_plan(AudioDriverPhase::exited);
        break;
    default: throw std::logic_error("invalid native stop phase");
    }
}
} // namespace unirally
