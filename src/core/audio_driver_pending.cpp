#include "audio_driver.hpp"
#include <algorithm>
#include <stdexcept>

namespace unirally {
namespace {
bool valid_operation(const AudioDriverPendingIo& operation) {
    switch (operation.kind) {
    case AudioDriverIoKind::read_port:
    case AudioDriverIoKind::write_port: return operation.address < 4;
    case AudioDriverIoKind::write_dsp: return operation.address < 128;
    case AudioDriverIoKind::write_ram: return true;
    case AudioDriverIoKind::read_timer:
    case AudioDriverIoKind::timer_target: return operation.address < 3;
    case AudioDriverIoKind::timer_control: return operation.address == 0;
    case AudioDriverIoKind::clear_ports: return operation.address == 0 || operation.address == 2;
    case AudioDriverIoKind::continue_phase:
        return operation.address <= static_cast<unsigned>(AudioDriverPhase::exited);
    }
    return false;
}
}
void TitleMenuAudioDriver::write_control(std::uint8_t value) {
    if (planning_)
        queue_io(AudioDriverIoKind::timer_control, 0, value);
    else
        timers_.write_control(value);
}
void TitleMenuAudioDriver::write_target(std::uint8_t timer, std::uint8_t value) {
    if (planning_)
        queue_io(AudioDriverIoKind::timer_target, timer, value);
    else
        timers_.write_target(timer, value);
}
void TitleMenuAudioDriver::clear_ports(std::uint8_t first_port) {
    if (planning_)
        queue_io(AudioDriverIoKind::clear_ports, first_port);
    else
        bus_->clear_ports(ticks_, first_port);
}
void TitleMenuAudioDriver::queue_io(AudioDriverIoKind kind, std::uint16_t address,
                                    std::uint8_t value) {
    constexpr std::size_t maximum_pending_operations = 2048;
    if (!planning_ || continuation_.operations.size() >= maximum_pending_operations)
        throw std::logic_error("invalid native audio work plan");
    continuation_.operations.push_back({planned_ticks_, kind, address, value});
}
void TitleMenuAudioDriver::finish_plan(AudioDriverPhase phase) {
    queue_io(AudioDriverIoKind::continue_phase, static_cast<std::uint16_t>(phase));
}
void TitleMenuAudioDriver::plan_read_port(std::uint8_t port) {
    advance(5);
    queue_io(AudioDriverIoKind::read_port, port);
    advance(1);
}
void TitleMenuAudioDriver::plan_read_timer(std::uint8_t timer) {
    queue_io(AudioDriverIoKind::read_timer, timer);
}
void TitleMenuAudioDriver::execute_pending_io(const AudioDriverPendingIo& operation) {
    const auto byte_address = static_cast<std::uint8_t>(operation.address);
    switch (operation.kind) {
    case AudioDriverIoKind::read_port:
        continuation_.port_reads.at(byte_address) = bus_->read_port(ticks_, byte_address);
        break;
    case AudioDriverIoKind::write_port:
        bus_->write_port(ticks_, byte_address, operation.value);
        break;
    case AudioDriverIoKind::write_dsp:
        bus_->write_dsp(ticks_, byte_address, operation.value);
        break;
    case AudioDriverIoKind::write_ram:
        bus_->write_ram(ticks_, operation.address, operation.value);
        break;
    case AudioDriverIoKind::read_timer:
        continuation_.timer_reads.at(byte_address) = timers_.read_output(byte_address);
        break;
    case AudioDriverIoKind::timer_target:
        timers_.write_target(byte_address, operation.value);
        break;
    case AudioDriverIoKind::timer_control: timers_.write_control(operation.value); break;
    case AudioDriverIoKind::clear_ports: bus_->clear_ports(ticks_, byte_address); break;
    case AudioDriverIoKind::continue_phase:
        continuation_.phase = static_cast<AudioDriverPhase>(operation.address);
        if (continuation_.phase == AudioDriverPhase::exited) stopped_for_ipl_ = true;
        break;
    }
}
// A force-sync visit occurs after the physical clock step and before timers.
// Retain that small pending timer step when the CPU takes control.
void TitleMenuAudioDriver::advance_pending_clock(std::uint64_t target) {
    if (continuation_.deferred_timer_step) {
        timers_.advance_to(ticks_);
        continuation_.deferred_timer_step = false;
    }
    const auto quantum = bus_->clock_sync_step();
    if (!quantum) {
        ticks_ = target;
        timers_.advance_to(ticks_);
        return;
    }
    if (quantum > 2) throw std::logic_error("invalid SMP synchronization quantum");
    while (ticks_ < target) {
        ticks_ += std::min<std::uint64_t>(quantum, target - ticks_);
        continuation_.deferred_timer_step = true;
        bus_->advance_clock(ticks_);
        timers_.advance_to(ticks_);
        continuation_.deferred_timer_step = false;
    }
}
// A CPU yield can interrupt a port access before it completes. The operation
// stays pending, while physical SMP/timer clocks have already reached that tick.
void TitleMenuAudioDriver::run_pending_until(std::uint64_t exclusive_ticks) {
    if (exclusive_ticks < ticks_) throw std::invalid_argument("driver clock moves backwards");
    while (!stopped_for_ipl_ && ticks_ < exclusive_ticks) {
        if (continuation_.next_operation == continuation_.operations.size()) {
            continuation_.operations.clear();
            continuation_.next_operation = 0;
            plan_phase();
        }
        const auto& operation = continuation_.operations.at(continuation_.next_operation);
        if (operation.ticks >= exclusive_ticks) break;
        advance_pending_clock(operation.ticks);
        execute_pending_io(operation);
        ++continuation_.next_operation;
    }
}
AudioDriverSnapshot TitleMenuAudioDriver::snapshot() const {
    if (!resumable_ || planning_) throw std::logic_error("driver has no stable pending state");
    return {score_.state(),   timers_.state(),        ticks_,
            command_phase_,   music_counter_,         effect_counter_,
            update_counter_,  master_volume_,         master_volume_rate_,
            stopped_for_ipl_, pitch_.sample_fraction, pitch_.sample_transpose,
            continuation_};
}
void TitleMenuAudioDriver::restore(const AudioDriverSnapshot& state) {
    const auto& pending = state.continuation;
    if (!resumable_ || planning_
        || (state.timers.ticks > state.ticks
            || state.ticks - state.timers.ticks > (pending.deferred_timer_step ? 2U : 0U))
        || pending.next_operation > pending.operations.size()
        || pending.phase > AudioDriverPhase::exited
        || pending.poll_return > AudioDriverPhase::exited
        || pending.descriptor_return > AudioDriverPhase::exited || pending.voice > 7
        || pending.slot > 64 || (state.command_phase & 63) || pending.operations.size() > 2048)
        throw std::invalid_argument("invalid driver pending state");
    auto previous = state.ticks;
    for (std::size_t i = pending.next_operation; i < pending.operations.size(); ++i) {
        const auto& operation = pending.operations[i];
        if (operation.ticks < previous || !valid_operation(operation))
            throw std::invalid_argument("invalid driver pending operation");
        previous = operation.ticks;
    }
    score_.restore(state.score);
    timers_.restore(state.timers);
    ticks_ = state.ticks;
    command_phase_ = state.command_phase;
    music_counter_ = state.music_counter;
    effect_counter_ = state.effect_counter;
    update_counter_ = state.update_counter;
    master_volume_ = state.master_volume;
    master_volume_rate_ = state.master_volume_rate;
    pitch_.sample_fraction = state.sample_fraction;
    pitch_.sample_transpose = state.sample_transpose;
    stopped_for_ipl_ = state.stopped_for_ipl;
    continuation_ = pending;
}
void TitleMenuAudioDriver::plan_phase() {
    planning_ = true;
    planned_ticks_ = ticks_;
    const auto phase = continuation_.phase;
    try {
        if (phase <= AudioDriverPhase::boot_finish)
            plan_boot_phase();
        else if (phase <= AudioDriverPhase::effect_counter)
            plan_main_phase();
        else if (phase <= AudioDriverPhase::update_end)
            plan_update_phase();
        else
            plan_stop_phase();
    } catch (...) {
        planning_ = false;
        throw;
    }
    planning_ = false;
    if (continuation_.operations.empty()) throw std::logic_error("empty native audio work plan");
}
} // namespace unirally
