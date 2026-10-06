#include "audio_ipl.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
struct Event {
    char kind;
    std::uint64_t ticks;
    std::uint16_t address;
    std::uint8_t value;
    bool operator==(const Event&) const = default;
};
// Independent synthetic sender: wait forty ticks after each acknowledgment,
// transfer 258 bytes across a counter/page wrap, then request driver entry.
struct SenderBus final : unirally::AudioDriverBus {
    std::array<std::uint8_t, 4> ports{};
    std::vector<Event> events;
    std::uint64_t pending_tick = 0;
    unsigned bytes_sent = 0;
    // AUDIO-UPLOAD-SPEED: the CPU takes control before every port access once, by return.
    bool yield_every_port = false;
    mutable bool yielded = false;
    bool yield_due(std::uint64_t) const override {
        if (!yield_every_port) return false;
        yielded = !yielded;
        return yielded;
    }
    enum class SenderPhase { waiting_ready, header, data, finish, done } phase{};
    void update(std::uint64_t ticks) {
        if (!pending_tick || ticks < pending_tick) return;
        pending_tick = 0;
        if (phase == SenderPhase::header)
            ports = {204, 1, 0, 22};
        else if (phase == SenderPhase::data)
            ports = {static_cast<std::uint8_t>(bytes_sent),
                     static_cast<std::uint8_t>(bytes_sent * 7U + 3U), 0, 22};
        else if (phase == SenderPhase::finish)
            ports = {4, 0, 0, 4};
    }
    std::uint8_t read_port(std::uint64_t ticks, std::uint8_t port) override {
        update(ticks);
        const auto value = ports.at(port);
        events.push_back({'R', ticks, port, value});
        return value;
    }
    void write_port(std::uint64_t ticks, std::uint8_t port, std::uint8_t value) override {
        events.push_back({'P', ticks, port, value});
        if (phase == SenderPhase::waiting_ready && port == 1 && value == 187)
            phase = SenderPhase::header;
        else if (phase == SenderPhase::header && port == 0 && value == 204)
            phase = SenderPhase::data;
        else if (phase == SenderPhase::data && port == 0) {
            if (++bytes_sent == 258) phase = SenderPhase::finish;
        } else if (phase == SenderPhase::finish && port == 0 && value == 4)
            phase = SenderPhase::done;
        else
            return;
        pending_tick = ticks + 40;
    }
    void write_ram(std::uint64_t ticks, std::uint16_t address, std::uint8_t value) override {
        events.push_back({'N', ticks, address, value});
    }
    void write_dsp(std::uint64_t, std::uint8_t, std::uint8_t) override {
        throw std::runtime_error("IPL unexpectedly wrote DSP");
    }
    void clear_ports(std::uint64_t, std::uint8_t) override {
        throw std::runtime_error("IPL unexpectedly cleared IO");
    }
};
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
struct Snapshot {
    unirally::AudioIplState ipl;
    SenderBus bus;
};
void check_phase_continuations() {
    SenderBus sender;
    unirally::AudioIplHandshake original(sender);
    std::array<bool, 21> seen{};
    std::vector<Snapshot> snapshots;
    for (std::uint64_t tick = 1; tick < 100000 && !original.driver_ready(); ++tick) {
        const auto phase = static_cast<std::size_t>(original.state().phase);
        if (!seen.at(phase)) {
            snapshots.push_back({original.state(), sender});
            seen.at(phase) = true;
        }
        original.run_until(tick);
        require(original.state().ticks < tick, "IPL crossed an exclusive horizon");
    }
    require(original.driver_ready(), "synthetic upload did not complete");
    require(sender.bytes_sent == 258, "sender counter differs");
    const auto data_writes =
        std::count_if(sender.events.begin(), sender.events.end(),
                      [](const auto& e) { return e.kind == 'N' && e.address >= 0x1600; });
    require(data_writes == 258, "IPL page-wrap data writes differ");
    for (const auto& snapshot : snapshots) {
        auto resumed_sender = snapshot.bus;
        unirally::AudioIplHandshake resumed(resumed_sender);
        resumed.restore(snapshot.ipl);
        resumed.run_until(100000);
        require(resumed.driver_ready(), "resumed upload did not complete");
        require(resumed.state().ticks == original.state().ticks, "resumed entry clock differs");
        require(resumed_sender.events == sender.events, "resumed bus events differ");
    }
    require(snapshots.size() >= 19, "too few pending phases exercised");
    // A yield by return leaves the access pending at its tick; the next run makes it.
    SenderBus yielding_sender;
    yielding_sender.yield_every_port = true;
    unirally::AudioIplHandshake yielding(yielding_sender);
    unsigned returns = 0;
    while (!yielding.driver_ready() && returns < 100000) {
        yielding.run_until(100000);
        if (!yielding.driver_ready()) {
            ++returns;
            require(yielding.state().ticks == yielding.state().next_access_ticks,
                    "a yielded access is not pending at its tick");
        }
    }
    require(yielding.driver_ready() && returns > 500, "too few yields by return");
    require(yielding_sender.events == sender.events, "a yield by return duplicated or lost IO");
    require(yielding.state().ticks == original.state().ticks, "yielded entry clock differs");
    auto invalid = original.state();
    invalid.next_access_ticks = invalid.ticks - 1;
    bool rejected = false;
    try {
        original.restore(invalid);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected, "backwards pending clock was accepted");
}
}
int main() {
    try {
        check_phase_continuations();
        std::cout << "IPL page wrap and pending-phase continuations passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
