#pragma once
#include "audio_score.hpp"
#include "audio_timers.hpp"
#include <vector>

namespace unirally {
// These are authored semantic continuations, not original program counters.
// R-0075: queued work is computed from native state and identified data only.
enum class AudioDriverPhase : std::uint8_t {
    boot_prefix,
    boot_ready,
    boot_ready_check,
    descriptor_begin,
    descriptor_wait,
    descriptor_check,
    slot_received,
    loop_low_received,
    loop_high_received,
    fraction_received,
    transpose_received,
    bulk_begin,
    bulk_compare,
    bulk_negative_first,
    bulk_negative_second,
    bulk_byte_received,
    bulk_after,
    slot_finished,
    samples_ready,
    samples_ready_check,
    boot_finish,
    iteration,
    poll_begin,
    poll_bulk_check,
    poll_bulk_confirm,
    poll_header_first,
    poll_header_compare,
    poll_header_captured,
    execute_command,
    music_timer,
    music_counter,
    effect_timer,
    effect_counter,
    update_begin,
    score_voice,
    score_voice_next,
    output_voice,
    output_voice_next,
    update_end,
    stop_begin,
    stop_wait,
    stop_pulse,
    stop_finish,
    exited
};
enum class AudioDriverIoKind : std::uint8_t {
    read_port,
    write_port,
    write_dsp,
    write_ram,
    read_timer,
    timer_target,
    timer_control,
    clear_ports,
    continue_phase
};
struct AudioDriverPendingIo {
    std::uint64_t ticks = 0;
    AudioDriverIoKind kind = AudioDriverIoKind::continue_phase;
    std::uint16_t address = 0;
    std::uint8_t value = 0;
};
struct AudioDriverContinuation {
    AudioDriverPhase phase = AudioDriverPhase::iteration;
    AudioDriverPhase poll_return = AudioDriverPhase::music_timer;
    AudioDriverPhase descriptor_return = AudioDriverPhase::slot_received;
    std::vector<AudioDriverPendingIo> operations;
    std::size_t next_operation = 0;
    std::array<std::uint8_t, 4> port_reads{};
    std::array<std::uint8_t, 3> timer_reads{};
    std::uint8_t first_header = 0, command_header = 0;
    bool effects = false, deferred_timer_step = false;
    std::uint8_t voice = 0, upload_phase = 129, slot = 0, sample = 255;
    std::uint16_t sample_cursor = 0x3000, loop_sum = 0;
    std::uint8_t sample_offset = 0, stop_volume = 0;
};
// Pointer-free native state. DSP/history and CPU transport are separate owners
// and must be included by the eventual complete audio snapshot. No full-save
// or product acceptance is implied by this representation.
struct AudioDriverSnapshot {
    AudioScoreState score;
    AudioTimersState timers;
    std::uint64_t ticks = 0;
    std::uint8_t command_phase = 128;
    std::uint8_t music_counter = 0, effect_counter = 0, update_counter = 0;
    std::uint8_t master_volume = 127;
    bool stopped_for_ipl = false;
    AudioDriverContinuation continuation;
};
} // namespace unirally
