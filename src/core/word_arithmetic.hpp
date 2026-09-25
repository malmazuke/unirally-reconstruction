#pragma once
// The original's 16-bit word arithmetic, shared by the race engine's systems.

#include <cstdint>
#include <span>
#include <stdexcept>

namespace unirally {

inline bool negative(std::uint16_t value) {
    return (value & 0x8000U) != 0;
}

inline std::uint16_t add_word(std::uint16_t left, std::uint16_t right) {
    return static_cast<std::uint16_t>(static_cast<std::uint32_t>(left) + right);
}

inline unsigned content_word(std::span<const std::uint8_t> bytes, unsigned index) {
    if (index + 1 >= bytes.size()) throw std::out_of_range("movement table word is unavailable");
    return bytes[index] | (static_cast<unsigned>(bytes[index + 1]) << 8U);
}

inline unsigned integer_sqrt(unsigned value) {
    unsigned root = 0;
    while ((root + 1U) * (root + 1U) <= value) ++root;
    return root;
}

} // namespace unirally
