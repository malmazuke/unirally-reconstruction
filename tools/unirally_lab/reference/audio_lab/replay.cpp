// Laboratory diagnostic only. Never linked into the product or used as content.
// The include path selects the pinned LGPL-2.1-or-later SPC_DSP sources; their
// notices remain in the original source checkout. This file supplies no CPU.
#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
struct { struct { struct { bool cubic = false; } dsp; } hacks; } configuration;
#include "SPC_DSP.cpp"

using Bytes = std::vector<uint8_t>;
static Bytes readFile(const char* path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) throw std::runtime_error("cannot open input");
    return Bytes(std::istreambuf_iterator<char>(stream), {});
}
static uint64_t little(const uint8_t* bytes, size_t size) {
    uint64_t result = 0;
    for (size_t i = 0; i < size; ++i) result |= uint64_t(bytes[i]) << (8 * i);
    return result;
}
static void writeFile(const char* path, const void* bytes, size_t size) {
    std::ofstream stream(path, std::ios::binary);
    if (!stream || !stream.write(static_cast<const char*>(bytes), size))
        throw std::runtime_error("cannot write output");
}
int main(int argc, char** argv) {
    try {
        if (argc != 5) throw std::runtime_error("replay INITIAL_RAM EVENTS OUT_PCM OUT_RAM");
        auto ram = readFile(argv[1]), events = readFile(argv[2]);
        if (ram.size() != 65536 || events.size() % 42) throw std::runtime_error("input size");
        SPC_DSP dsp;
        dsp.init(ram.data(), ram.data());
        dsp.reset();
        std::array<short, 8192> buffer{};
        dsp.set_output(buffer.data(), buffer.size());
        Bytes pcm;
        uint64_t clocks = 0, sequence = 0;
        int pending = 0, consumed = 0;
        for (size_t offset = 0; offset < events.size(); offset += 42) {
            const auto* event = events.data() + offset;
            const auto recordedClock = little(event + 16, 8);
            const auto address = little(event + 36, 2), value = little(event + 38, 2);
            const auto kind = event[40];
            if (little(event, 8) != sequence++ || recordedClock != clocks)
                throw std::runtime_error("event order or DSP clock mismatch at " + std::to_string(sequence - 1));
            switch (kind) {
            case 1:
                if (consumed != pending || (address != 1 && address != 32))
                    throw std::runtime_error("DSP run batch/count");
                dsp.run(int(address)); clocks += address;
                pending = dsp.sample_count(); consumed = 0;
                break;
            case 2:
                if (consumed + 2 > pending || uint16_t(buffer[consumed]) != address ||
                    uint16_t(buffer[consumed + 1]) != value)
                    throw std::runtime_error("PCM mismatch at pair " + std::to_string(pcm.size() / 4));
                pcm.insert(pcm.end(), event + 36, event + 40);
                consumed += 2;
                if (consumed == pending) { dsp.set_output(buffer.data(), buffer.size()); pending = consumed = 0; }
                break;
            case 3: dsp.write(int(address), int(value)); break;
            case 4: ram.at(address) = uint8_t(value); break;
            case 5: case 6: case 7: case 8: case 9: case 10: case 11: case 12: case 13: case 14: case 15: break;
            default: throw std::runtime_error("unknown event kind");
            }
        }
        if (consumed != pending) throw std::runtime_error("undelivered samples");
        writeFile(argv[3], pcm.data(), pcm.size());
        writeFile(argv[4], ram.data(), ram.size());
        std::cout << "events=" << sequence << " dsp_clocks=" << clocks << " stereo_pairs=" << pcm.size() / 4 << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
