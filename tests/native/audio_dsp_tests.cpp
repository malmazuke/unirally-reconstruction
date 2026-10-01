#include "adapter.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

using Device = std::unique_ptr<UnirallyDsp, decltype(&unirally_dsp_destroy)>;
static Device device() {
    Device result(unirally_dsp_create(), unirally_dsp_destroy);
    if (!result) throw std::runtime_error("DSP allocation failed");
    return result;
}
static void require(bool result, const char* message) {
    if (!result) throw std::runtime_error(message);
}
static void setSyntheticVoice(UnirallyDsp* dsp) {
    // Authored BRR loop, unrelated to game content. Direct-gain voice with echo
    // and noise enabled exercises RAM writes, decoding and hardware histories.
    constexpr std::array<uint8_t, 9> brr{0x63, 0x12, 0x34, 0x56, 0x70, 0xfe, 0xdc, 0xba, 0x98};
    for (size_t i = 0; i < brr.size(); ++i)
        unirally_dsp_write_ram(dsp, static_cast<uint16_t>(0x3000 + i), brr[i]);
    for (uint16_t address : {uint16_t{0x2000}, uint16_t{0x2002}}) {
        unirally_dsp_write_ram(dsp, address, 0);
        unirally_dsp_write_ram(dsp, static_cast<uint16_t>(address + 1), 0x30);
    }
    const std::array<std::array<uint8_t, 2>, 18> registers{{
        {0x5d, 0x20}, {0x00, 127}, {0x01, 83}, {0x02, 0}, {0x03, 0x10},
        {0x04, 0}, {0x05, 0}, {0x07, 127}, {0x0c, 127}, {0x1c, 127},
        {0x2c, 37}, {0x3c, 51}, {0x4d, 1}, {0x6d, 0x40}, {0x7d, 1},
        {0x0f, 127}, {0x6c, 3}, {0x4c, 1},
    }};
    for (auto reg : registers) unirally_dsp_write_register(dsp, reg[0], reg[1]);
}
static std::vector<int16_t> run(UnirallyDsp* dsp, uint32_t clocks, uint32_t batch) {
    std::vector<int16_t> result;
    while (clocks) {
        const auto amount = std::min(clocks, batch);
        std::array<int16_t, 2> stereo{};
        const int count = unirally_dsp_run(dsp, amount, stereo.data());
        require(count == 0 || count == 2, "invalid sample count");
        for (int i = 0; i < count; ++i) result.push_back(stereo[static_cast<size_t>(i)]);
        clocks -= amount;
    }
    return result;
}
int main() {
    try {
        auto continuous = device(), saved = device(), restored = device();
        setSyntheticVoice(continuous.get()); setSyntheticVoice(saved.get());
        const auto prefix = run(continuous.get(), 325, 32);
        require(prefix == run(saved.get(), 325, 1), "clock batching changes PCM");
        std::vector<uint8_t> snapshot(unirally_dsp_state_size());
        require(unirally_dsp_save(saved.get(), snapshot.data(), snapshot.size()) == 1, "save failed");
        require(unirally_dsp_load(restored.get(), snapshot.data(), snapshot.size()) == 1, "load failed");
        const auto expected = run(continuous.get(), 9607, 32);
        require(expected == run(saved.get(), 9607, 32), "save perturbs continuation");
        require(expected == run(restored.get(), 9607, 1), "fresh restore changes continuation");
        require(std::any_of(expected.begin(), expected.end(), [](auto value) { return value != 0; }),
                "synthetic voice is silent");
        for (uint32_t address = 0; address < 65536; ++address)
            require(unirally_dsp_read_ram(continuous.get(), static_cast<uint16_t>(address)) ==
                    unirally_dsp_read_ram(restored.get(), static_cast<uint16_t>(address)), "echo RAM differs");
        std::array<int16_t, 2> stereo{};
        require(unirally_dsp_run(saved.get(), 0, stereo.data()) == -1, "zero clock accepted");
        require(unirally_dsp_run(saved.get(), 33, stereo.data()) == -1, "oversized batch accepted");
        require(unirally_dsp_load(saved.get(), snapshot.data(), snapshot.size() - 1) == 0, "truncated state accepted");
        snapshot[4] = 2;
        require(unirally_dsp_load(saved.get(), snapshot.data(), snapshot.size()) == 0, "unknown version accepted");
        std::cout << "DSP clock batching, save non-perturbation, fresh restore and shared echo RAM pass\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
