#include "audio_timers.hpp"
#include <iostream>
#include <stdexcept>

static void require(bool result, const char* message) {
    if (!result) throw std::runtime_error(message);
}
int main() {
    try {
        unirally::AudioTimers clock;
        clock.write_target(2, 1); clock.write_control(4);
        clock.advance_to(31); require(clock.read_output(2) == 0, "timer pulsed before a falling edge");
        clock.advance_to(32); require(clock.read_output(2) == 1, "timer missed a falling edge");
        require(clock.read_output(2) == 0, "timer read did not clear output");
        clock.advance_to(544); require(clock.read_output(2) == 0, "four-bit timer output did not wrap");
        clock.write_target(2, 0); clock.advance_to(544 + 8191);
        require(clock.read_output(2) == 0, "zero target acted before 256 falling edges");
        clock.advance_to(544 + 8192); require(clock.read_output(2) == 1, "zero target never pulsed");
        clock.write_target(1, 2); clock.write_control(6);
        clock.advance_to(10003);
        unirally::AudioTimers restored; restored.restore(clock.state());
        for (const auto tick : {10017U, 10433U, 12567U, 18765U}) {
            clock.advance_to(tick); restored.advance_to(tick);
            for (std::uint8_t index = 0; index < 3; ++index)
                require(clock.read_output(index) == restored.read_output(index), "restored timer output differs");
            require(clock.state() == restored.state(), "restored divider phase differs");
        }
        clock.write_target(2, 1); clock.advance_to(18784);
        clock.write_control(6); require(clock.read_output(2) == 0, "timer-2 CONTROL behavior differs");
        auto invalid = clock.state(); invalid.timers[2].divider = 16;
        bool rejected = false;
        try { restored.restore(invalid); } catch (const std::invalid_argument&) { rejected = true; }
        require(rejected, "invalid divider state accepted");
        std::cout << "native timer edges, wrap, zero target and continuation pass\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
