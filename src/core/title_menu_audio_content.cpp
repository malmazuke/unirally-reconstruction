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
std::string sample_entry(unsigned sample) {
    return "audio.sample." + std::string(sample < 10 ? "0" : "") + std::to_string(sample);
}
// R-0076, pack v32: the first race's transfers, directory lengths and samples.
void load_race_upload(const ClassicContentPack& pack, AudioCpuUploadData& out) {
    const auto lengths = pack.entry("audio.race-resource-lengths");
    if (lengths.size() != out.race_resource_lengths.size() * 2)
        throw std::invalid_argument("audio race directory size differs");
    for (unsigned i = 0; i < out.race_resource_lengths.size(); ++i)
        out.race_resource_lengths[i] = word(lengths, i * 2);
    out.race_tables_transfer = bytes_entry(pack, "audio.race-tables-transfer");
    out.race_song_transfer = bytes_entry(pack, "audio.race-song-1-transfer");
    copy_entry(pack, "audio.race-sample-slots", out.race_sample_slots);
    for (const auto sample : out.race_sample_slots) {
        if (sample == 255) continue;
        if (sample >= 50) throw std::invalid_argument("audio sample resource differs");
        if (out.sample_resources[sample].empty())
            out.sample_resources[sample] = bytes_entry(pack, sample_entry(sample).c_str());
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
    constexpr std::array<unsigned, 19> hunter_ids{0,   6,   7,   8,   9,   10,  11,  14,  58, 93,
                                                  102, 103, 104, 105, 107, 108, 109, 110, 111};
    const auto hunter = pack.optional_entry("audio.hunter-graphics-work-directory");
    if (hunter.empty()) return;
    if (hunter.size() != hunter_ids.size() * 5)
        throw std::invalid_argument("HUNTER graphics metadata size differs");
    for (unsigned i = 0; i < hunter_ids.size(); ++i) {
        const auto offset = i * 5;
        out[hunter_ids[i]] = {static_cast<std::uint8_t>(hunter[offset] & 127),
                              word(hunter, offset + 1), word(hunter, offset + 3),
                              (hunter[offset] & 128) != 0};
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
    if (!pack.optional_entry("audio.race-tables").empty()) {
        out.race_score = {bytes_entry(pack, "audio.race-tables"),
                          bytes_entry(pack, "audio.race-song-1")};
        load_race_upload(pack, out.upload);
    }
    load_graphics(pack, out.graphics);
    copy_entry(pack, "audio.cartridge-defaults", out.cartridge_defaults);
    copy_entry(pack, "audio.track-types", out.track_types);
    std::copy_n(out.cartridge_defaults.begin(), out.cartridge_signature.size(),
                out.cartridge_signature.begin());
    out.menu_text = bytes_entry(pack, "front-end.main-menu-text");
    copy_entry(pack, "front-end.character-table", out.characters);
    copy_entry(pack, "front-end.menu-arrow-columns", out.arrow_positions);
    copy_entry(pack, "front-end.reveal-offsets", out.reveal_offsets);
    copy_entry(pack, "front-end.reveal-brightness", out.reveal_brightness);
    copy_entry(pack, "front-end.credits-text", out.credits_text);
    copy_entry(pack, "front-end.credits-poses", out.credits_poses);
    out.pose_pointers = bytes_entry(pack, "presentation.rider.pose-pointers.v1");
    out.pose_frames = bytes_entry(pack, "presentation.rider.pose-frames.v1");
    return out;
}
} // namespace unirally
