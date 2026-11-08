#include "lab_outputs.hpp"

#include <cstdio>
#include <string_view>

namespace unirally {

std::string race_state_row(std::uint32_t frame, const ZoomZooState& state) {
    static constexpr char digits[] = "0123456789abcdef";
    const auto bytes = serialize_zoom_zoo(state);
    auto row = std::to_string(frame);
    row.reserve(row.size() + 2 + bytes.size() * 2);
    row.push_back(' ');
    for (const auto byte : bytes) {
        row.push_back(digits[byte >> 4U]);
        row.push_back(digits[byte & 15U]);
    }
    row.push_back('\n');
    return row;
}

std::vector<std::uint8_t> ppm_picture(const RgbFrame& frame) {
    static constexpr std::string_view header = "P6\n256 224\n255\n";
    std::vector<std::uint8_t> bytes(header.begin(), header.end());
    bytes.insert(bytes.end(), frame.pixels.begin(), frame.pixels.end());
    return bytes;
}

std::uint64_t output_digest(std::span<const std::uint8_t> bytes) {
    std::uint64_t digest = 0xcbf29ce484222325ULL;
    for (const auto byte : bytes) {
        digest ^= byte;
        digest *= 0x100000001b3ULL;
    }
    return digest;
}

std::string digest_line(std::uint32_t frame, char kind, std::span<const std::uint8_t> bytes) {
    char line[40];
    std::snprintf(line, sizeof line, "%u %c %016llx\n", frame, kind,
                  static_cast<unsigned long long>(output_digest(bytes)));
    return line;
}

} // namespace unirally
