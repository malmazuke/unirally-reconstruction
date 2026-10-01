#include "content_pack.hpp"
#include "title_menu_audio.hpp"
#include <algorithm>
#include <stdexcept>

namespace unirally {
namespace {
std::uint16_t word(std::span<const std::uint8_t> bytes, std::size_t offset) {
    return static_cast<std::uint16_t>(bytes[offset] | unsigned(bytes[offset + 1]) << 8);
}
template <std::size_t Size>
void copy_entry(const ClassicContentPack& pack, const char* name,
                std::array<std::uint8_t, Size>& out) {
    const auto bytes = pack.entry(name);
    if (bytes.size() != Size) throw std::invalid_argument("audio content size differs");
    std::copy(bytes.begin(), bytes.end(), out.begin());
}
std::vector<std::uint8_t> bytes_entry(const ClassicContentPack& pack, const char* name) {
    const auto bytes = pack.entry(name);
    return {bytes.begin(), bytes.end()};
}
void load_upload(const ClassicContentPack& pack, AudioCpuUploadData& out) {
    const auto lengths = pack.entry("audio.resource-lengths");
    for (unsigned i = 0; i < out.resource_lengths.size(); ++i)
        out.resource_lengths[i] = word(lengths, i * 2);
    out.menu_transfer = bytes_entry(pack, "audio.menu-transfer");
    out.title_transfer = bytes_entry(pack, "audio.title-transfer");
    copy_entry(pack, "audio.sample-slots", out.sample_slots);
    for (const auto sample : out.sample_slots) {
        if (sample == 255) continue;
        if (sample >= 50) throw std::invalid_argument("audio sample resource differs");
        const auto name =
            "audio.sample." + std::string(sample < 10 ? "0" : "") + std::to_string(sample);
        out.sample_resources[sample] = bytes_entry(pack, name.c_str());
    }
}
// R-0075: only identified palette/map/tile metadata used by the native work clock.
void load_graphics(const ClassicContentPack& pack, std::array<AudioCpuGraphicsAsset, 128>& out) {
    constexpr std::array<unsigned, 17> ids{1,  2,  5,  27, 28, 31, 68, 69, 70,
                                           72, 74, 77, 78, 80, 88, 89, 91};
    const auto bytes = pack.entry("audio.graphics-work-directory");
    for (unsigned i = 0; i < ids.size(); ++i) {
        const auto offset = i * 5;
        out[ids[i]] = {static_cast<std::uint8_t>(bytes[offset] & 127), word(bytes, offset + 1),
                       word(bytes, offset + 3), (bytes[offset] & 128) != 0};
    }
}
} // namespace
TitleMenuAudioContent title_menu_audio_content(const ClassicContentPack& pack) {
    TitleMenuAudioContent out;
    out.identity = pack.extraction_identity();
    out.score = {bytes_entry(pack, "audio.menu-tables"), bytes_entry(pack, "audio.title-score")};
    const auto notes = pack.entry("audio.pitch-values");
    for (unsigned i = 0; i < out.pitch.notes.size(); ++i) out.pitch.notes[i] = word(notes, i * 2);
    copy_entry(pack, "audio.sample-fractions", out.pitch.sample_fraction);
    copy_entry(pack, "audio.sample-transpose", out.pitch.sample_transpose);
    load_upload(pack, out.upload);
    load_graphics(pack, out.graphics);
    copy_entry(pack, "audio.cartridge-defaults", out.cartridge_defaults);
    copy_entry(pack, "audio.track-types", out.track_types);
    std::copy_n(out.cartridge_defaults.begin(), out.cartridge_signature.size(),
                out.cartridge_signature.begin());
    out.menu_text = bytes_entry(pack, "front-end.main-menu-text");
    copy_entry(pack, "front-end.character-table", out.characters);
    copy_entry(pack, "front-end.menu-arrow-columns", out.arrow_positions);
    return out;
}
} // namespace unirally
