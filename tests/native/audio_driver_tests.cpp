#include "audio_driver.hpp"
#include <algorithm>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace {
struct CpuYield {};
struct Event {
    char kind; std::uint64_t ticks; std::uint16_t address; std::uint8_t value;
    bool operator==(const Event&) const = default;
};
struct Bus final : unirally::AudioDriverBus {
    std::vector<Event> events;
    bool interrupt_reads = false, resume_read = false, interrupt_clock = false;
    // AUDIO-UPLOAD-SPEED: yields by return, before every port access once and at one clock visit.
    bool return_yields = false, return_clock = false;
    mutable bool yielded = false;
    bool yield_due(std::uint64_t) const override {
        if (!return_yields) return false;
        yielded = !yielded;
        return yielded;
    }
    unsigned clock_sync_step() const override { return interrupt_clock || return_clock ? 2U : 0U; }
    bool advance_clock(std::uint64_t ticks) override {
        if (interrupt_clock && ticks >= 54322) { interrupt_clock = false; throw CpuYield{}; }
        if (return_clock && ticks >= 54322) { return_clock = false; return true; }
        return false;
    }
    std::uint8_t read_port(std::uint64_t ticks, std::uint8_t port) override {
        if (interrupt_reads && !resume_read) { resume_read = true; throw CpuYield{}; }
        resume_read = false;
        const std::uint8_t value = port == 2 ? 128 : (port == 0 && ticks >= 100000 ? 255 : 0);
        events.push_back({'R', ticks, port, value}); return value;
    }
    void write_port(std::uint64_t t, std::uint8_t p, std::uint8_t v) override {
        events.push_back({'P', t, p, v});
    }
    void write_dsp(std::uint64_t t, std::uint8_t r, std::uint8_t v) override {
        events.push_back({'D', t, r, v});
    }
    void clear_ports(std::uint64_t t, std::uint8_t p) override {
        events.push_back({'C', t, p, 0});
    }
    void write_ram(std::uint64_t t, std::uint16_t a, std::uint8_t v) override {
        events.push_back({'N', t, a, v});
    }
};
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
unirally::AudioTimersState initial_timers() {
    unirally::AudioTimers timers;
    timers.write_target(2, 133); timers.write_target(1, 20); timers.write_control(6);
    return timers.state();
}
void check_continuations() {
    const unirally::AudioSoundSet data{std::vector<std::uint8_t>(621),
                                           std::vector<std::uint8_t>(2200)};
    const unirally::AudioPitchData pitch;
    Bus continuous_bus, interrupted_bus; interrupted_bus.interrupt_reads = true; interrupted_bus.interrupt_clock = true;
    unirally::TitleMenuAudioDriver continuous(data, pitch, continuous_bus, initial_timers(), 0,
                                              false, true);
    continuous.run_until(300000);
    require(continuous.returned_to_ipl(), "synthetic stop did not finish");
    auto interrupted = std::make_unique<unirally::TitleMenuAudioDriver>(
        data, pitch, interrupted_bus, initial_timers(), 0, false, true);
    unsigned suspensions = 0; bool saw_deferred_timer = false;
    while (!interrupted->returned_to_ipl()) {
        try { interrupted->run_until(300000); }
        catch (const CpuYield&) {
            const auto snapshot = interrupted->snapshot(); ++suspensions;
            if (snapshot.continuation.deferred_timer_step) {
                saw_deferred_timer = true;
                require(snapshot.timers.ticks < snapshot.ticks
                        && snapshot.ticks - snapshot.timers.ticks <= 2,
                        "force yield did not retain its deferred timer step");
            }
            interrupted = std::make_unique<unirally::TitleMenuAudioDriver>(
                data, pitch, interrupted_bus, initial_timers(), 0, false, true);
            interrupted->restore(snapshot);
        }
    }
    require(saw_deferred_timer, "deferred physical clock step was not tested");
    require(suspensions > 100, "too few CPU-yielding accesses");
    require(interrupted_bus.events == continuous_bus.events, "CPU yield duplicated or lost IO");
    const auto expected = continuous.snapshot(), actual = interrupted->snapshot();
    require(expected.score == actual.score && expected.timers == actual.timers
            && expected.ticks == actual.ticks, "restored score or timer phase differs");
    auto invalid = actual; invalid.continuation.operations.push_back(
        {actual.ticks, unirally::AudioDriverIoKind::read_port, 4, 0});
    bool rejected = false;
    try { interrupted->restore(invalid); }
    catch (const std::invalid_argument&) { rejected = true; }
    require(rejected, "invalid pending port was accepted");
    require(std::any_of(continuous_bus.events.begin(), continuous_bus.events.end(),
            [](const auto& e) { return e.kind == 'D' && e.address == 0x6c && e.value == 224; }),
            "stop did not disable the DSP");
    // The same yields by return: run_until returns with the access pending, and the next call
    // makes it; a clock visit's yield keeps its deferred timer step.
    Bus returning_bus; returning_bus.return_yields = true; returning_bus.return_clock = true;
    unirally::TitleMenuAudioDriver returning(data, pitch, returning_bus, initial_timers(), 0,
                                             false, true);
    unsigned returns = 0; bool saw_returned_timer = false;
    while (!returning.returned_to_ipl() && returns < 100000) {
        returning.run_until(300000);
        if (returning.returned_to_ipl()) break;
        ++returns;
        if (returning.snapshot().continuation.deferred_timer_step) saw_returned_timer = true;
    }
    require(returning.returned_to_ipl() && returns > 100, "too few yields by return");
    require(saw_returned_timer, "a clock visit's yield by return was not tested");
    require(returning_bus.events == continuous_bus.events, "a yield by return duplicated or lost IO");
    const auto returned = returning.snapshot();
    require(expected.score == returned.score && expected.timers == returned.timers
            && expected.ticks == returned.ticks, "a yield by return changed score or timer phase");
}
}
int main() {
    try { check_continuations(); std::cout << "driver CPU-yield/restore/stop passed\n"; return 0; }
    catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
