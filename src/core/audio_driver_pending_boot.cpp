#include "audio_driver.hpp"
#include <stdexcept>

namespace unirally {
void TitleMenuAudioDriver::plan_descriptor(AudioDriverPhase return_phase) {
    continuation_.descriptor_return = return_phase;
    advance(16);
    plan_read_port(2);
    finish_plan(AudioDriverPhase::descriptor_check);
}
// R-0075, 154D-1556: a descriptor has one stable phase read and a byte read.
void TitleMenuAudioDriver::plan_descriptor_phase() {
    const auto received = continuation_.port_reads[2];
    if (received != continuation_.upload_phase) {
        advance(8);
        plan_read_port(2);
        finish_plan(AudioDriverPhase::descriptor_check);
        return;
    }
    advance(4);
    plan_read_port(3);
    write_port(2, continuation_.upload_phase);
    ++continuation_.upload_phase;
    advance(14);
    finish_plan(continuation_.descriptor_return);
}
// R-0075, 0400-0448 and 14F7-151C. Reads end each work plan before a
// decision depending on the CPU-visible latch, preserving suspension points.
void TitleMenuAudioDriver::plan_boot_phase() {
    switch (continuation_.phase) {
    case AudioDriverPhase::boot_prefix:
        advance(26);
        write_control(48);
        clear_ports(0);
        clear_ports(2);
        advance(16);
        initialize_dsp();
        advance(16);
        initialize_voice_ram();
        advance(74);
        write_port(2, 128);
        finish_plan(AudioDriverPhase::boot_ready);
        break;
    case AudioDriverPhase::boot_ready:
        plan_read_port(2);
        finish_plan(AudioDriverPhase::boot_ready_check);
        break;
    case AudioDriverPhase::boot_ready_check:
        if (continuation_.port_reads[2] != 128) {
            advance(8);
            finish_plan(AudioDriverPhase::boot_ready);
            break;
        }
        advance(8);
        write_port(0, 0);
        write_port(1, 0);
        advance(52);
        continuation_.upload_phase = 129;
        continuation_.slot = 0;
        continuation_.sample_cursor = 0x3000;
        plan_descriptor(AudioDriverPhase::slot_received);
        break;
    case AudioDriverPhase::descriptor_check: plan_descriptor_phase(); break;
    case AudioDriverPhase::samples_ready:
        advance(2);
        plan_read_port(2);
        advance(2);
        finish_plan(AudioDriverPhase::samples_ready_check);
        break;
    case AudioDriverPhase::samples_ready_check:
        if (continuation_.port_reads[2] != 128) {
            advance(8);
            finish_plan(AudioDriverPhase::samples_ready);
            break;
        }
        advance(6);
        write_port(2, 128);
        advance(10);
        finish_plan(AudioDriverPhase::boot_finish);
        break;
    case AudioDriverPhase::boot_finish:
        advance(16);
        configure_timers();
        advance(60);
        write_dsp(20, 0x6c, 51);
        advance(20 + score_.start_music_timed(0));
        advance(10);
        finish_plan(AudioDriverPhase::iteration);
        break;
    default:
        if (continuation_.phase >= AudioDriverPhase::bulk_begin
            && continuation_.phase <= AudioDriverPhase::bulk_after)
            plan_bulk_phase();
        else
            plan_sample_phase();
    }
}
} // namespace unirally
