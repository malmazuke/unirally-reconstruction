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
std::uint16_t resource_length(const AudioCpuUploadData& data, unsigned index) {
    return index < data.resource_lengths.size()
             ? data.resource_lengths[index]
             : data.race_resource_lengths.at(index - data.resource_lengths.size());
}
// Static bank-82 listing 812A-8150; directory lengths are data-format values.
std::uint16_t select_resource(Clock& c, const AudioCpuUploadData& data, unsigned index) {
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
        cursor += resource_length(data, i);
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
    return static_cast<std::uint16_t>(cursor);
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
// Static bank-82 8328-8336. XBA retains a hidden accumulator byte and
// consumes two idle cycles; the supplied value is typed descriptor data.
void send_sample_descriptor(Clock& c, std::uint8_t phase, std::uint8_t value) {
    c.call_local();
    c.write_audio_port(3, value);
    c.rom_reads(1);
    c.idle(2);
    c.write_audio_port(2, phase);
    bool ready = false;
    do {
        ready = c.read_audio_ports(2) == phase;
        c.branch(!ready);
    } while (!ready);
    c.update_register();
    c.rom_reads(1);
    c.idle(2);
    c.return_local();
}
void advance_sample_cursor(Clock& c, std::uint16_t& cursor, bool word_mode) {
    c.update_register();
    cursor = static_cast<std::uint16_t>(cursor + 1U);
    c.branch(cursor != 0);
    if (!cursor) {
        increment_direct(c, word_mode ? 2U : 1U);
        c.load_constant(2);
        cursor = 0x8000;
    }
}
// 82C8-8315. Four metadata bytes and BRR bytes share the wrapping phase.
void send_sample_resource(Clock& c, const AudioCpuUploadData& data, unsigned sample,
                          std::uint8_t& phase) {
    c.update_register();
    auto cursor = select_resource(c, data, sample);
    c.read_direct(2);
    c.store_direct();
    c.store_direct();
    c.change_widths();
    read_resource(c, 2);
    advance_sample_cursor(c, cursor, true);
    advance_sample_cursor(c, cursor, true);
    c.update_register();
    c.load_constant(2);
    c.update_register();
    c.change_widths();
    c.restore_register();
    c.update_register();
    ++phase;
    const auto& bytes = data.sample_resources[sample];
    for (std::size_t offset = 0; offset < bytes.size(); ++offset) {
        c.rom_reads(1);
        c.idle(2);
        read_resource(c, 1);
        c.write_audio_port(3, bytes[offset]);
        c.rom_reads(1);
        c.idle(2);
        c.write_audio_port(2, phase);
        advance_sample_cursor(c, cursor, false);
        bool ready = false;
        do {
            ready = c.read_audio_ports(2) == phase;
            c.branch(!ready);
        } while (!ready);
        c.update_register();
        ++phase;
        c.update_register();
        c.branch(offset + 1 < bytes.size());
    }
    c.update_register();
    c.load_constant();
    ++phase;
    c.write_audio_port(2, phase);
    bool ready = false;
    do {
        ready = c.read_audio_ports(2) == phase;
        c.branch(!ready);
    } while (!ready);
    c.update_register();
    ++phase;
}

}
void native_audio_cpu_begin_session(AudioCpuWorkClock& c) {
    begin_next_transfer(c);
}
// $80:A0FC-A10E (title) and $83:CA74-CA89 (first race): driver, tables, score.
void native_audio_cpu_uploads(AudioCpuWorkClock& c, const AudioCpuUploadData& data,
                              AudioSoundSetId set) {
    const bool race = set == AudioSoundSetId::first_race;
    const unsigned tables = race ? 54 : 53, score = race ? 62 : 57;
    const auto& tables_bytes = race ? data.race_tables_transfer : data.menu_transfer;
    const auto& score_bytes = race ? data.race_song_transfer : data.title_transfer;
    if (resource_length(data, 50) != 4445 || tables_bytes.size() != resource_length(data, tables)
        || score_bytes.size() != resource_length(data, score) || tables_bytes.empty()
        || score_bytes.empty())
        throw std::invalid_argument("unidentified audio upload domain");
    for (unsigned group = 0; group < 3; ++group) {
        const unsigned index = group == 0 ? 50 : (group == 1 ? tables : score);
        select_resource(c, data, index);
        wait_ipl_ready(c);
        begin_header(c, group == 0 ? 0x400 : (group == 1 ? 0x1600 : 0x1d00));
        const auto length = resource_length(data, index);
        for (unsigned offset = 0; offset < length; ++offset) {
            const auto value =
                group == 0 ? 0 : (group == 1 ? tables_bytes[offset] : score_bytes[offset]);
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
// Bank-80 A112/A115 and bank-82 82A5-8327. There are 64 selected slots,
// including FF holes; each nonempty slot names identified sample data only.
void native_audio_cpu_upload_samples(AudioCpuWorkClock& c, const AudioCpuUploadData& data,
                                     AudioSoundSetId set) {
    const auto& slots =
        set == AudioSoundSetId::first_race ? data.race_sample_slots : data.sample_slots;
    for (const auto sample : slots) {
        if (sample != 255
            && (sample >= 50 || data.resource_lengths[sample] < 6
                || data.sample_resources[sample].size()
                       != unsigned(data.resource_lengths[sample]) - 2))
            throw std::invalid_argument("unidentified audio sample resource");
    }
    c.load_constant(2);
    c.call_far();
    c.call_local();
    c.save_register();
    c.change_widths();
    c.change_widths();
    c.load_constant();
    c.store_direct();
    c.load_constant();
    std::uint8_t phase = 129;
    for (unsigned slot = 0; slot < slots.size(); ++slot) {
        c.save_register(2);
        c.store_direct(2);
        c.save_register();
        c.rom_reads(1);
        c.idle(2);
        c.rom_reads(5);
        const auto sample = slots[slot];
        send_sample_descriptor(c, phase, sample);
        c.load_constant();
        c.branch(sample != 255);
        if (sample == 255) {
            c.restore_register();
            c.update_register();
            ++phase;
            c.branch(true);
        } else
            send_sample_resource(c, data, sample, phase);
        c.restore_register(2);
        c.update_register();
        increment_direct(c, 1);
        c.branch(slot + 1 < slots.size());
    }
    c.load_constant();
    c.write_audio_port(2, 128);
    bool ready = false;
    do {
        ready = c.read_audio_ports(2) == 128;
        c.branch(!ready);
    } while (!ready);
    c.restore_register();
    c.return_local();
    c.return_far();
}
} // namespace unirally
