#pragma once
#include <cstdint>
#include <span>

namespace unirally {

// Velocities live in the shared rider motion state. This record owns only
// modifiers that persist between updates; R-0011-speed maps the original words.
struct SpeedModifiers {
    std::uint16_t boost{};
    std::uint16_t vertical_boost{};
    std::uint16_t progress_adjustment{};
};
struct SpeedLimitContext {
    bool opponent{};
    bool skip{};
    bool drag{};
    std::uint8_t pose_byte{}; // provisional meaning; original $1513/$150B
    bool start_override{};
    bool ai_enabled{};
    std::uint16_t player_progress{};
    std::uint16_t opponent_progress{};
    std::uint16_t ai_adjustment{};
    std::uint16_t adjustment_limit{};
    std::uint16_t player_base_cap{}; // original $11D3 is used for BOTH riders
    std::uint8_t update_counter{};
    std::uint16_t friction_mode{};
    std::uint8_t cartridge_mode{};
};
struct SpeedDecayContent {
    std::span<const std::uint8_t> masks;      // 9 bytes from PAL ROM file 0x051B
    std::span<const std::uint8_t> decrements; // 9 little-endian words at 0x0524
};

// Word references carry original signed 16-bit velocity bit patterns (1/32
// position unit per update). Changes are atomic on unsupported input/content.
// The caller advances the global byte counter before either rider's update.
void limit_rider_speed(std::uint16_t& velocity_x, std::uint16_t& velocity_y,
                       SpeedModifiers& modifiers, const SpeedLimitContext& context,
                       const SpeedDecayContent& content);

} // namespace unirally
