#include "audio_cpu_boot.hpp"
#include <array>
#include <iostream>
#include <stdexcept>

int main() {
    try {
        // Original cold CPU trace, PAL a1105819..., R-0075. These values are
        // expected evidence; the native function receives no timestamp seeds.
        constexpr std::array<std::uint64_t, 12> expected{186,      594,      2752,     7834602,
                                                         7834832,  10265646, 10291180, 10299848,
                                                         10299950, 12284432, 12284690, 12284978};
        const auto actual = unirally::native_audio_cpu_boot_prefix();
        if (actual != expected) throw std::runtime_error("cold CPU milestone clocks differ");
        bool rejected = false;
        try {
            unirally::native_audio_cpu_boot_prefix({32, 8190});
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        if (!rejected) throw std::runtime_error("unidentified asset-clock domain was accepted");
        for (auto tick : actual) std::cout << tick << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
