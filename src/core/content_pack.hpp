#pragma once

#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace unirally {

// The pack profiles this build reads: the accepted DRAGSTER v1 profile and
// the current two-track profile. The launcher asks the app for this list
// before launching, so a build from before a profile bump is reported as a
// stale build rather than as a bad pack.
std::span<const std::string_view> supported_pack_profiles();

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
