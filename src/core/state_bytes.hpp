#pragma once
// Little-endian byte writers and the bounds-checked reader of the serialized race states.

#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <vector>

namespace unirally {

inline void put8(std::vector<std::uint8_t>& out, std::uint8_t value) {
    out.push_back(value);
}

inline void put16(std::vector<std::uint8_t>& out, std::uint16_t value) {
    put8(out, static_cast<std::uint8_t>(value));
    put8(out, static_cast<std::uint8_t>(value >> 8));
}

inline void put32(std::vector<std::uint8_t>& out, std::uint32_t value) {
    put16(out, static_cast<std::uint16_t>(value));
    put16(out, static_cast<std::uint16_t>(value >> 16));
}

inline void put_bool(std::vector<std::uint8_t>& out, bool value) {
    put8(out, value ? 1 : 0);
}

class Reader {
public:
    explicit Reader(std::span<const std::uint8_t> bytes) : bytes_(bytes) {}
    std::uint8_t u8() {
        require(1);
        return bytes_[offset_++];
    }
    std::uint16_t u16() {
        const auto lo = u8();
        return static_cast<std::uint16_t>(lo | (static_cast<unsigned>(u8()) << 8));
    }
    std::uint32_t u32() {
        const auto lo = u16();
        return static_cast<std::uint32_t>(lo | (static_cast<std::uint32_t>(u16()) << 16));
    }
    bool flag() {
        const auto value = u8();
        if (value > 1) throw std::invalid_argument("movement state flag is not binary");
        return value != 0;
    }
    void require_end() const {
        if (offset_ != bytes_.size())
            throw std::invalid_argument("movement state has trailing bytes");
    }

private:
    void require(std::size_t count) const {
        if (bytes_.size() - offset_ < count)
            throw std::invalid_argument("movement state is truncated");
    }
    std::span<const std::uint8_t> bytes_;
    std::size_t offset_{};
};

} // namespace unirally
