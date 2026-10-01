#include "audio_cpu_scene.hpp"
#include <iostream>
#include <stdexcept>

int main() {
    try {
        unirally::AudioCpuWorkClock clock;
        unirally::AudioCpuSceneWorkState scene;
        std::array<std::uint8_t, 8192> cartridge{};
        std::array<std::uint8_t, 1158> defaults{};
        // The tenth settings word is the final word of the identified table.
        // A nonzero sentinel catches a truncated table or an omitted final copy.
        defaults[1156] = 0x5b;
        defaults[1157] = 0xa1;
        unirally::native_audio_finish_menu_records(clock, scene, cartridge, defaults);
        if (cartridge[0x0750] != 0x5b || cartridge[0x0751] != 0xa1)
            throw std::runtime_error("terminal settings word was not copied intact");
        if (scene.menu_mode != 0)
            throw std::runtime_error("menu reset did not clear the mode");
        std::cout << "terminal settings word and menu reset pass\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
