#include "audio_cpu_boot.hpp"
#include "audio_cpu_clock.hpp"
#include <stdexcept>

namespace unirally {
namespace {
using Clock = AudioCpuWorkClock;
// $80:91D1; R-0075. Bank-80 listing 91D1-9311; hardware reset work from pinned CPU::main.
void reset_cpu(Clock& c) {
    c.rom_reads(3);
    c.update_register();
    c.update_register();
    c.change_widths();
    c.change_widths();
    c.update_register();
    c.rom_reads(4);
    c.load_constant(2);
    c.update_register();
    c.rom_reads(3);
    c.ram_writes(2);
    c.restore_register(2);
    c.load_constant();
    c.save_register();
    c.restore_register();
    c.load_constant();
    c.store_port();
    c.set_fast_rom(true);
}
void clear_ppu_controls(Clock& c) {
    c.load_constant();
    c.store_port();
    c.load_constant(2);
    c.store_port(2);
    c.load_constant();
    c.store_port();
    for (unsigned i = 0; i < 31; ++i) c.store_port();
    c.load_constant();
    c.store_port();
    for (unsigned i = 0; i < 5; ++i) c.store_port();
    c.load_constant();
    c.store_port();
    for (unsigned i = 0; i < 20; ++i) c.store_port();
    c.load_constant();
    c.store_port();
    c.store_port();
    c.load_constant();
    c.store_port();
    c.store_port();
    c.store_port();
    c.load_constant();
    c.store_port();
    for (unsigned i = 0; i < 11; ++i) c.store_port();
    c.store_port();
    c.set_fast_rom(false);
    c.store_port();
    c.change_widths();
    c.change_widths();
    c.load_constant();
    c.load_constant(2);
}
void clear_work_ram(Clock& c) {
    for (unsigned i = 0; i < 65536; ++i) {
        c.store_ram(1, true);
        c.store_ram(1, true);
        c.update_register();
        c.branch(i < 65535);
    }
}
void select_asset_directory(Clock& c) {
    c.load_constant(2);
    c.load_constant();
    c.store_direct(2);
    c.store_direct();
    c.store_ram();
    c.store_ram();
    c.rom_reads(3);
    c.call_local();
}
// $80:B612; R-0075. Bank-80 B612-B625 clears 32,768 words using two six-clock MMIO writes.
void clear_vram(Clock& c) {
    c.save_register();
    c.change_widths();
    c.load_constant(2);
    c.store_port(2);
    c.load_constant(2);
    for (unsigned i = 0; i < 32768; ++i) {
        c.store_port(2);
        c.update_register();
        c.branch(i < 32767);
    }
    c.restore_register();
    c.return_local();
    c.call_local();
}
void initialize_frontend_storage(Clock& c) {
    c.load_constant();
    c.store_port();
    c.rom_reads(4);
    c.ram_reads();
    c.load_constant();
    c.store_ram(1, true);
    c.load_constant();
    c.store_ram();
    c.load_constant(2);
    c.load_constant();
    c.load_constant(2);
    for (unsigned i = 0; i < 128; ++i) {
        c.store_ram(1, false, true);
        c.store_ram(1, false, true);
        for (unsigned j = 0; j < 5; ++j) c.update_register();
        c.branch(i < 127);
    }
    c.load_constant();
    c.load_constant(2);
    c.load_constant(2);
    for (unsigned i = 0; i < 32; ++i) {
        c.store_ram(1, false, true);
        c.update_register();
        c.update_register();
        c.branch(i < 31);
    }
    c.load_constant();
    c.store_port();
    c.load_constant(2);
    c.load_constant();
    c.call_far();
}
// $82:B2B0; R-0075. Bank-82 B2B0-B2DC reads the five-byte asset record. Both cold records use
// ordinary byte content. The raw loader below also preserves source-bank wrapping.
void read_asset_directory_value(Clock& c, unsigned bytes) {
    c.begin_instruction();
    c.rom_reads(2);
    c.ram_reads(3);
    if (bytes > 1) c.rom_reads(bytes - 1);
    c.last_cycle();
    c.rom_reads(1);
}
void read_asset_descriptor(Clock& c) {
    c.store_direct();
    c.store_direct();
    c.change_widths();
    c.read_direct(2);
    c.update_register();
    c.update_register();
    c.branch(true);
    c.read_direct(2);
    c.update_register();
    c.update_register();
    read_asset_directory_value(c, 2);
    c.update_register();
    c.update_register();
    c.update_register();
    read_asset_directory_value(c, 2);
    c.store_direct(2);
    c.load_constant(2);
    c.change_widths();
    for (unsigned i = 0; i < 3; ++i) c.update_register();
    read_asset_directory_value(c, 1);
    c.load_constant();
    c.store_direct();
    read_asset_directory_value(c, 1);
    c.load_constant();
    c.update_register();
    c.return_local();
}
// $82:B296; R-0075. B296-B29E reads one byte, increments the word cursor and returns flags.
void read_asset_byte(Clock& c, std::uint16_t& cursor) {
    c.call_local();
    c.read_rom(1, false, true);
    c.update_register();
    const bool wrapped = ++cursor == 0;
    c.branch(wrapped);
    if (wrapped) {
        c.update_register();
        c.save_register();
        c.restore_register();
        c.update_register();
        c.save_register();
        c.restore_register();
        c.update_register();
        c.load_constant(2);
        cursor = 0x8000;
    }
    c.load_constant();
    c.return_local();
}
void finish_asset_upload(Clock& c) {
    c.update_register();
    c.restore_register();
    c.restore_register();
    c.return_far();
}
void upload_palette(Clock& c, unsigned bytes, std::uint16_t cursor = 0x8000) {
    c.save_register();
    c.save_register();
    c.update_register();
    c.change_widths();
    c.save_register();
    c.update_register();
    c.store_port();
    c.restore_register();
    c.call_local();
    read_asset_descriptor(c);
    c.change_widths();
    c.store_direct();
    c.save_register();
    c.restore_register();
    c.read_direct(2);
    c.update_register();
    for (unsigned i = 0; i < bytes / 2; ++i) {
        read_asset_byte(c, cursor);
        c.store_port();
        read_asset_byte(c, cursor);
        c.store_port();
        c.update_register();
        c.update_register();
        c.branch(i + 1 < bytes / 2);
    }
    finish_asset_upload(c);
}
void upload_tiles(Clock& c, unsigned bytes, std::uint16_t cursor = 0x8000) {
    c.save_register();
    c.save_register();
    c.update_register();
    c.change_widths();
    c.store_port(2);
    c.call_local();
    read_asset_descriptor(c);
    c.change_widths();
    c.store_direct();
    c.save_register();
    c.restore_register();
    c.read_direct();
    c.load_constant();
    c.branch(false);
    c.read_direct(2);
    c.update_register();
    for (unsigned i = 0; i < bytes / 2; ++i) {
        read_asset_byte(c, cursor);
        c.store_port(1, true);
        read_asset_byte(c, cursor);
        c.store_port(1, true);
        c.update_register();
        c.update_register();
        c.branch(i + 1 < bytes / 2);
    }
    finish_asset_upload(c);
}
void prepare_audio_storage(Clock& c) {
    c.change_widths();
    c.load_constant(2);
    c.store_ram(2);
    c.load_constant(2);
    c.store_ram(2);
    c.change_widths();
    c.load_constant(2);
    c.call_far();
}
void begin_sound_upload(Clock& c) {
    c.call_local();
    c.save_register();
    c.change_widths();
    for (unsigned i = 0; i < 3; ++i) c.save_register(2);
    c.change_widths();
    c.load_constant();
    c.write_audio_port(0, 255);
}
}
// $82:B183; $82:B1DB; R-0075. Identified raw graphics transport.
void native_audio_cpu_upload_graphics_asset(Clock& c, const AudioCpuGraphicsAsset& asset,
                                            bool palette) {
    if (asset.compressed || asset.address < 0x8000 || asset.bank > 127 || asset.bytes == 0
        || asset.bytes > 65536 || (asset.bytes & 1))
        throw std::invalid_argument("unidentified raw graphics work domain");
    if (palette)
        upload_palette(c, asset.bytes, asset.address);
    else
        upload_tiles(c, asset.bytes, asset.address);
}
std::array<std::uint64_t, 12> native_audio_cpu_boot_prefix(const AudioBootAssetSizes& sizes,
                                                           AudioCpuWorkObserver* observer) {
    Clock clock(observer);
    return native_audio_cpu_boot_prefix(clock, sizes);
}
std::array<std::uint64_t, 12> native_audio_cpu_boot_prefix(AudioCpuWorkClock& c,
                                                           const AudioBootAssetSizes& sizes) {
    if (c.ticks() != 0 || sizes.palette_bytes != 32 || sizes.tile_bytes != 8192)
        throw std::invalid_argument("unidentified cold audio asset-clock domain");
    c.idle(22);
    c.ram_reads();
    c.idle();
    c.ram_writes(3);
    c.rom_reads(2);
    std::array<std::uint64_t, 12> result{};
    unsigned index = 0;
    const auto mark = [&] { result.at(index++) = c.ticks(); };
    mark();
    reset_cpu(c);
    mark();
    clear_ppu_controls(c);
    mark();
    clear_work_ram(c);
    mark();
    select_asset_directory(c);
    mark();
    clear_vram(c);
    mark();
    initialize_frontend_storage(c);
    mark();
    upload_palette(c, sizes.palette_bytes);
    mark();
    c.load_constant(2);
    c.load_constant();
    c.call_far();
    mark();
    upload_tiles(c, sizes.tile_bytes);
    mark();
    prepare_audio_storage(c);
    mark();
    begin_sound_upload(c);
    mark();
    return result;
}
} // namespace unirally
