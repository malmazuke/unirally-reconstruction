#pragma once

#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace unirally {

// Schema-1 Classic pack view. The repository command verifies SHA-256 before
// launching the native process; this reader independently enforces the binary
// structure, exact profile/start identity and complete logical-ID inventory.
class ClassicContentPack {
public:
    explicit ClassicContentPack(const std::filesystem::path& path);
    std::span<const std::uint8_t> entry(const std::string& logical_id) const;
    // Empty when this pack profile does not carry the entry.
    std::span<const std::uint8_t> optional_entry(const std::string& logical_id) const;

private:
    struct Entry { std::size_t offset{}, size{}; };
    std::vector<std::uint8_t> bytes_;
    std::unordered_map<std::string, Entry> entries_;
};

} // namespace unirally
