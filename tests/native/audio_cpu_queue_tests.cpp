#include "audio_cpu_queue.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
class Receiver final : public unirally::AudioCpuWorkObserver {
public:
    std::uint8_t acknowledgment = 128, header = 0;
    std::uint64_t header_clock = 0;
    std::vector<std::pair<std::uint8_t, std::uint8_t>> received;
    void scanline(std::uint64_t, std::uint64_t) override {}
    std::uint8_t read_audio_port(std::uint64_t, std::uint8_t port) override {
        if (port != 2) throw std::runtime_error("queue read used wrong port");
        return acknowledgment;
    }
    void write_audio_port(std::uint64_t clock, std::uint8_t port, std::uint8_t value) override {
        if (port == 2) {
            header = value;
            header_clock = clock;
        } else if (port == 3) {
            if (clock - header_clock != 6 && clock - header_clock != 46)
                throw std::runtime_error("header/parameter bus ordering differs");
            received.emplace_back(header, value);
            acknowledgment = header & 192;
        } else
            throw std::runtime_error("queue write used wrong port");
    }
};
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
}
int main() {
    try {
        Receiver receiver;
        unirally::AudioCpuWorkClock clock(&receiver);
        unirally::AudioCpuQueueState state;
        for (unsigned i = 1; i <= 15; ++i)
            require(unirally::native_audio_enqueue(clock, state, static_cast<std::uint8_t>(i),
                                                   static_cast<std::uint8_t>(200 + i)),
                    "available queue slot rejected");
        const auto full = state;
        require(!unirally::native_audio_enqueue(clock, state, 31, 7), "full queue accepted cue");
        require(state == full, "full queue changed owned state");
        receiver.acknowledgment = 0;
        require(!unirally::native_audio_poll_queue(clock, state), "busy receiver accepted cue");
        require(state == full && receiver.received.empty(), "busy queue consumed cue");
        receiver.acknowledgment = 128;
        for (unsigned i = 1; i <= 15; ++i) {
            require(unirally::native_audio_poll_queue(clock, state), "ready cue not sent");
            const auto [header, parameter] = receiver.received.back();
            require((header & 63) == i && parameter == 200 + i, "queue lost FIFO values");
            require((header & 192) == ((i & 1) ? 64 : 128), "phase did not alternate");
        }
        const auto empty = state;
        require(!unirally::native_audio_poll_queue(clock, state), "empty queue emitted cue");
        require(state == empty, "empty poll changed ring state");
        require(unirally::native_audio_enqueue(clock, state, 3, 255), "wrapped slot rejected");
        require(unirally::native_audio_poll_queue(clock, state), "wrapped cue not sent");
        require(receiver.received.back() == std::pair<std::uint8_t, std::uint8_t>{131, 255},
                "wrapped cue or phase differs");
        std::cout << "full, busy, FIFO, wrap and header ordering pass\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
