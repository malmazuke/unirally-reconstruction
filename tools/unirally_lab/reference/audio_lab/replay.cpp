// Laboratory diagnostic only. Never linked into the product or used as content.
// Uses the separately linked LGPL DSP-only hardware adapter. No CPU or
// original game executable is linked. Captured events are lab inputs only.
#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
#include "adapter.h"
#include <memory>
#include <limits>

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
    if (size > static_cast<size_t>(std::numeric_limits<std::streamsize>::max()) ||
        !stream || !stream.write(static_cast<const char*>(bytes), static_cast<std::streamsize>(size)))
        throw std::runtime_error("cannot write output");
}
int main(int argc, char** argv) {
    try {
        if (argc != 5) throw std::runtime_error("replay INITIAL_RAM EVENTS OUT_PCM OUT_RAM");
        auto ram = readFile(argv[1]), events = readFile(argv[2]);
        if (ram.size() != 65536 || events.size() % 42) throw std::runtime_error("input size");
        std::unique_ptr<UnirallyDsp, decltype(&unirally_dsp_destroy)> dsp(
            unirally_dsp_create(), unirally_dsp_destroy);
        if (!dsp) throw std::runtime_error("DSP allocation");
        for (size_t i = 0; i < ram.size(); ++i)
            unirally_dsp_write_ram(dsp.get(), static_cast<uint16_t>(i), ram[i]);
        std::array<int16_t, 2> buffer{};
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
                pending = unirally_dsp_run(dsp.get(), static_cast<uint32_t>(address), buffer.data());
                if (pending < 0) throw std::runtime_error("DSP run arguments");
                clocks += address; consumed = 0;
                break;
            case 2:
                if (consumed + 2 > pending || uint16_t(buffer[static_cast<size_t>(consumed)]) != address ||
                    uint16_t(buffer[static_cast<size_t>(consumed + 1)]) != value)
                    throw std::runtime_error("PCM mismatch at pair " + std::to_string(pcm.size() / 4));
                pcm.insert(pcm.end(), event + 36, event + 40);
                consumed += 2;
                if (consumed == pending) pending = consumed = 0;
                break;
            case 3: unirally_dsp_write_register(dsp.get(), static_cast<uint8_t>(address), static_cast<uint8_t>(value)); break;
            case 4: unirally_dsp_write_ram(dsp.get(), static_cast<uint16_t>(address), uint8_t(value)); break;
            case 5: case 6: case 7: case 8: case 9: case 10: case 11: case 12: case 13: case 14: case 15: break;
            default: throw std::runtime_error("unknown event kind");
            }
        }
        if (consumed != pending) throw std::runtime_error("undelivered samples");
        for (size_t i = 0; i < ram.size(); ++i)
            ram[i] = unirally_dsp_read_ram(dsp.get(), static_cast<uint16_t>(i));
        writeFile(argv[3], pcm.data(), pcm.size());
        writeFile(argv[4], ram.data(), ram.size());
        std::cout << "events=" << sequence << " dsp_clocks=" << clocks << " stereo_pairs=" << pcm.size() / 4 << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
