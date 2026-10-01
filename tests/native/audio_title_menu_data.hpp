#pragma once
#include "content_pack.hpp"
#include "title_menu_audio.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace audio_test {
inline std::vector<std::uint8_t> read(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file)
        throw std::runtime_error("cannot read identified native audio input: " + path.string());
    return {std::istreambuf_iterator<char>(file), {}};
}
template <std::size_t Size>
inline void load(std::array<std::uint8_t, Size>& out, const std::filesystem::path& path) {
    const auto bytes = read(path);
    if (bytes.size() != Size) throw std::invalid_argument("native audio input size differs");
    std::copy(bytes.begin(), bytes.end(), out.begin());
}
inline unirally::TitleMenuAudioContent content(const std::filesystem::path& root) {
    if (std::filesystem::is_regular_file(root))
        return unirally::title_menu_audio_content(unirally::ClassicContentPack(root));
    unirally::TitleMenuAudioContent data;
    load(data.identity, root / "cpu-content-identity.bin");
    data.score = {read(root / "menu-tables.bin"), read(root / "title-score.bin")};
    const auto notes = read(root / "pitch-table.bin");
    if (notes.size() != 194) throw std::invalid_argument("invalid pitch input");
    for (unsigned i = 0; i < data.pitch.notes.size(); ++i)
        data.pitch.notes[i] =
            static_cast<std::uint16_t>(notes[2 * i] | unsigned(notes[2 * i + 1]) << 8);
    load(data.pitch.sample_fraction, root / "sample-fractions.bin");
    load(data.pitch.sample_transpose, root / "sample-transpose.bin");
    std::ifstream lengths(root / "cpu-resource-lengths.txt");
    for (auto& length : data.upload.resource_lengths) {
        unsigned value;
        if (!(lengths >> value) || value > 65535)
            throw std::invalid_argument("invalid audio resource length");
        length = static_cast<std::uint16_t>(value);
    }
    data.upload.menu_transfer = read(root / "cpu-menu-upload.bin");
    data.upload.title_transfer = read(root / "cpu-title-upload.bin");
    load(data.upload.sample_slots, root / "cpu-sample-slots.bin");
    for (const auto sample : data.upload.sample_slots) {
        if (sample == 255) continue;
        if (sample >= 58) throw std::invalid_argument("invalid sample resource");
        data.upload.sample_resources[sample] =
            read(root
                 / ("cpu-sample-resource-" + std::string(sample < 10 ? "0" : "")
                    + std::to_string(sample) + ".bin"));
    }
    for (const auto* name : {"cpu-graphics-assets.txt", "cpu-menu-graphics-assets-v2.txt"}) {
        std::ifstream assets(root / name);
        unsigned id, bank, address, bytes, compressed;
        while (assets >> id >> bank >> address >> bytes >> compressed) {
            if (id >= 128 || bank > 127 || address > 65535 || compressed > 1)
                throw std::invalid_argument("invalid native graphics metadata");
            data.graphics[id] = {static_cast<std::uint8_t>(bank),
                                 static_cast<std::uint16_t>(address), bytes, compressed != 0};
        }
        if (!assets.eof()) throw std::invalid_argument("native graphics metadata truncated");
    }
    load(data.cartridge_defaults, root / "cpu-cartridge-defaults-v2.bin");
    load(data.track_types, root / "cpu-track-types.bin");
    load(data.cartridge_signature, root / "cpu-cartridge-signature.bin");
    data.menu_text = read(root / "cpu-menu-text.bin");
    load(data.characters, root / "cpu-character-table.bin");
    load(data.arrow_positions, root / "cpu-menu-arrow-positions.bin");
    return data;
}
inline void write(const std::filesystem::path& path, std::span<const std::uint8_t> bytes) {
    std::ofstream file(path, std::ios::binary);
    if (!file
        || !file.write(reinterpret_cast<const char*>(bytes.data()),
                       static_cast<std::streamsize>(bytes.size())))
        throw std::runtime_error("cannot write native audio state");
}
inline void write_pcm(std::ostream& out, std::span<const std::int16_t> pcm) {
    for (const auto sample : pcm) {
        const auto bits = static_cast<std::uint16_t>(sample);
        out.put(static_cast<char>(bits & 255));
        out.put(static_cast<char>(bits >> 8));
    }
    if (!out) throw std::runtime_error("native PCM write failed");
}
} // namespace audio_test
