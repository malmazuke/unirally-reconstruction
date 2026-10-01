#include "audio_cpu_upload.hpp"
#include <stdexcept>

namespace unirally {
namespace {
using Clock = AudioCpuWorkClock;
void read_resource(Clock& c, unsigned bytes) {
    c.rom_reads(2);
    c.ram_reads(3);
    c.rom_reads(bytes);
}
void increment_direct(Clock& c, unsigned bytes) {
    c.rom_reads(2);
    c.ram_reads(bytes);
    c.idle();
    c.ram_writes(bytes);
}
// Static bank-82 listing 812A-8150; directory lengths are data-format values.
void select_resource(Clock& c, const AudioCpuUploadData& data, unsigned index) {
    c.call_local();
    c.save_register();
    c.change_widths();
    c.load_constant();
    c.store_direct();
    c.change_widths();
    c.update_register();
    c.load_constant(2);
    c.update_register();
    c.load_constant(2);
    c.store_direct(2);
    unsigned cursor = 0x8000;
    for (unsigned i = 0; i < index; ++i) {
        c.update_register();
        c.branch(false);
        c.update_register();
        read_resource(c, 2);
        cursor += data.resource_lengths[i];
        const bool carry = cursor > 65535;
        c.branch(!carry);
        if (carry) {
            increment_direct(c, 2);
            c.update_register();
            c.load_constant(2);
            cursor -= 0x8000;
        }
        c.store_direct(2);
        c.branch(true);
    }
    c.update_register();
    c.branch(true);
    c.restore_register();
    c.return_local();
}
// 8151-8160. All three identified payloads stay within their source bank.
void advance_source(Clock& c) {
    c.call_local();
    c.save_register();
    c.change_widths();
    increment_direct(c, 2);
    c.branch(true);
    c.restore_register();
    c.return_local();
}
void wait_ipl_ready(Clock& c) {
    c.change_widths();
    c.load_constant(2);
    bool ready = false;
    do {
        ready = c.read_audio_ports(0, 2) == 0xbbaa;
        c.branch(!ready);
    } while (!ready);
}
void begin_header(Clock& c, std::uint16_t destination) {
    read_resource(c, 2);
    c.update_register();
    advance_source(c);
    advance_source(c);
    read_resource(c, 2);
    c.save_register(2);
    advance_source(c);
    advance_source(c);
    c.change_widths();
    c.store_direct();
    read_resource(c, 1);
    c.write_audio_port(2, static_cast<std::uint8_t>(destination));
    advance_source(c);
    read_resource(c, 1);
    c.write_audio_port(3, static_cast<std::uint8_t>(destination >> 8));
    c.load_constant();
    c.write_audio_port(1, 1);
    bool ready = false;
    do {
        c.load_constant();
        c.write_audio_port(0, 204);
        ready = c.read_audio_ports(0) == 204;
        c.branch(!ready);
    } while (!ready);
}
void send_byte(Clock& c, std::uint8_t phase, std::uint8_t value, bool last) {
    advance_source(c);
    read_resource(c, 1);
    c.write_audio_port(1, value);
    c.read_direct();
    c.write_audio_port(0, phase);
    bool ready = false;
    do {
        ready = c.read_audio_ports(0) == phase;
        c.branch(!ready);
    } while (!ready);
    increment_direct(c, 1);
    c.update_register();
    c.branch(!last);
}
void finish_transfer(Clock& c, std::uint16_t entry, unsigned length) {
    c.change_widths();
    c.restore_register(2);
    c.branch(entry != 0);
    if (!entry) c.load_constant(2);
    c.save_register(2);
    c.write_audio_word(2, entry ? entry : 0xffc0);
    c.change_widths();
    c.write_audio_port(1, 0);
    c.read_direct();
    c.update_register();
    c.load_constant();
    c.write_audio_port(0, static_cast<std::uint8_t>(length + 3));
}
void return_from_transfer(Clock& c) {
    c.change_widths();
    c.restore_register(2);
    c.load_constant(2);
    c.branch(true);
    c.change_widths();
    c.restore_register(2);
    c.restore_register(2);
    c.restore_register(2);
    c.restore_register();
    c.return_local();
    c.return_far();
}
void begin_next_transfer(Clock& c) {
    c.change_widths();
    c.load_constant(2);
    c.call_far();
    c.call_local();
    c.save_register();
    c.change_widths();
    for (unsigned i = 0; i < 3; ++i) c.save_register(2);
    c.change_widths();
    c.load_constant();
    c.write_audio_port(0, 255);
}
}
void native_audio_cpu_uploads(AudioCpuWorkClock& c, const AudioCpuUploadData& data) {
    if (data.resource_lengths[50] != 4445 || data.resource_lengths[53] != 627
        || data.resource_lengths[57] != 2206 || data.menu_transfer.size() != 627
        || data.title_transfer.size() != 2206)
        throw std::invalid_argument("unidentified cold audio upload domain");
    for (unsigned group = 0; group < 3; ++group) {
        const unsigned index = group == 0 ? 50 : (group == 1 ? 53 : 57);
        select_resource(c, data, index);
        wait_ipl_ready(c);
        begin_header(c, group == 0 ? 0x400 : (group == 1 ? 0x1600 : 0x1d00));
        const auto length = data.resource_lengths[index];
        for (unsigned offset = 0; offset < length; ++offset) {
            const auto value = group == 0 ? 0
                                          : (group == 1 ? data.menu_transfer[offset]
                                                        : data.title_transfer[offset]);
            send_byte(c, static_cast<std::uint8_t>(offset), static_cast<std::uint8_t>(value),
                      offset + 1 == length);
        }
        finish_transfer(c, group == 2 ? 0x400 : 0, length);
        if (group == 2) return;
        return_from_transfer(c);
        begin_next_transfer(c);
    }
}
// Static bank-82 listing 80FE-8129: initialize the ordinary command queue
// after the driver's native ready acknowledgment, then restore the caller.
void native_audio_cpu_finish_driver_entry(AudioCpuWorkClock& c) {
    c.change_widths();
    c.restore_register(2);
    c.load_constant(2);
    c.branch(false);
    c.change_widths();
    c.load_constant();
    bool ready = false;
    do {
        ready = c.read_audio_ports(2) == 128;
        c.branch(!ready);
    } while (!ready);
    c.store_ram(1, true);
    c.write_audio_port(2, 128);
    c.change_widths();
    c.load_constant(2);
    c.store_ram(2, true);
    c.store_ram(2, true);
    c.change_widths();
    c.restore_register(2);
    c.restore_register(2);
    c.restore_register(2);
    c.restore_register();
    c.return_local();
    c.return_far();
}
} // namespace unirally
