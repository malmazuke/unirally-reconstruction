#include "title_menu_audio.hpp"
namespace unirally {
namespace audio_state_detail {
class Archive;
}
void visit(audio_state_detail::Archive& a, AudioVoiceArithmetic& v);
void visit(audio_state_detail::Archive& a, AudioScoreVoice& v);
void visit(audio_state_detail::Archive& a, AudioScoreState& v);
void visit(audio_state_detail::Archive& a, AudioTimerState& v);
void visit(audio_state_detail::Archive& a, AudioTimersState& v);
void visit(audio_state_detail::Archive& a, AudioDriverPendingIo& v);
void visit(audio_state_detail::Archive& a, AudioDriverContinuation& v);
void visit(audio_state_detail::Archive& a, AudioDriverSnapshot& v);
void visit(audio_state_detail::Archive& a, AudioCpuClockState& v);
void visit(audio_state_detail::Archive& a, AudioIplState& v);
void visit(audio_state_detail::Archive& a, AudioCpuInterruptWorkState& v);
void visit(audio_state_detail::Archive& a, AudioDspState& v);
void visit(audio_state_detail::Archive& a, AudioEngineState& v);
void visit(audio_state_detail::Archive& a, AudioCpuQueueState& v);
void visit(audio_state_detail::Archive& a, AudioCpuSceneWorkState& v);
void visit(audio_state_detail::Archive& a, AudioCpuTitleHoldState& v);
void visit(audio_state_detail::Archive& a, AudioCpuTextWorkState& v);
void visit(audio_state_detail::Archive& a, AudioCpuMenuInputState& v);
void visit(audio_state_detail::Archive& a, TitleMenuAudioState& v);
}
#include "audio_state_archive.hpp"
#include <array>

namespace unirally {
void visit(audio_state_detail::Archive& a, AudioVoiceArithmetic& v) {
    a.fields(v.volume, v.pan, v.pan_step, v.volume_decay, v.left_volume, v.right_volume,
             v.remaining, v.sample, v.gain, v.base_note, v.alternating_note, v.current_note,
             v.alternate_interval, v.alternate_first_period, v.alternate_second_period,
             v.alternate_remaining, v.slide_interval, v.slide_amount, v.slide_remaining,
             v.modulation_direction, v.modulation_amount, v.modulation_delay,
             v.modulation_remaining, v.modulation_period, v.modulation_offset, v.target_pitch,
             v.base_pitch, v.output_pitch, v.convergence_step, v.detune, v.envelope_phase,
             v.envelope_timer, v.envelope_position, v.release_gain, v.release_remaining,
             v.instrument, v.scripted_envelope);
}
void visit(audio_state_detail::Archive& a, AudioScoreVoice& v) {
    a.fields(v.pointer, v.enabled, v.output_enabled, v.effect, v.priority, v.remaining,
             v.per_note_volume, v.next_duration_inline, v.fixed_duration, v.sample, v.transpose,
             v.detune, v.release_relative, v.release_absolute, v.volume, v.pan, v.pan_step,
             v.instrument, v.stack_position, v.pitch_step, v.pitch_delay, v.pitch_rate,
             v.pitch_period, v.pitch_glide, v.pitch_alternate, v.suppress_key_on,
             v.envelope_mode_c9c, v.restart_envelope, v.volume_gain, v.arithmetic);
}
void visit(audio_state_detail::Archive& a, AudioScoreState& v) {
    a.fields(v.voices, v.stack, v.instruments, v.random_state, v.timer2_target, v.music_gain,
             v.effect_gain, v.key_on_pending, v.key_off_pending);
}
void visit(audio_state_detail::Archive& a, AudioTimerState& v) {
    a.fields(v.divider, v.target_counter, v.output, v.target, v.divider_level, v.line, v.enabled);
}
void visit(audio_state_detail::Archive& a, AudioTimersState& v) {
    a.fields(v.timers, v.ticks, v.gate_enabled, v.gate_disabled);
}
void visit(audio_state_detail::Archive& a, AudioDriverPendingIo& v) {
    a.fields(v.ticks, v.kind, v.address, v.value);
}
void visit(audio_state_detail::Archive& a, AudioDriverContinuation& v) {
    a.fields(v.phase, v.poll_return, v.descriptor_return, v.operations, v.port_reads, v.timer_reads,
             v.first_header, v.command_header, v.effects, v.deferred_timer_step, v.voice,
             v.upload_phase, v.slot, v.sample, v.sample_cursor, v.loop_sum, v.sample_offset,
             v.stop_volume);
    a.index(v.next_operation);
}
void visit(audio_state_detail::Archive& a, AudioDriverSnapshot& v) {
    a.fields(v.score, v.timers, v.ticks, v.command_phase, v.music_counter, v.effect_counter,
             v.update_counter, v.master_volume, v.stopped_for_ipl, v.continuation);
}
void visit(audio_state_detail::Archive& a, AudioCpuClockState& v) {
    a.fields(v.ticks, v.completed_step_ticks, v.scanline, v.refreshed, v.fast_rom,
             v.pending_dma_bytes, v.dma_active, v.nmi_enabled, v.nmi_valid, v.nmi_line, v.nmi_hold,
             v.nmi_transition, v.nmi_pending, v.irq_lock, v.in_interrupt, v.auto_joypad_enabled,
             v.auto_joypad_counter, v.latched_controllers, v.controller_words);
}
void visit(audio_state_detail::Archive& a, AudioIplState& v) {
    a.fields(v.ticks, v.next_access_ticks, v.phase, v.destination, v.clear_index, v.byte_index,
             v.header, v.transfer_mode, v.transfer_byte);
}
void visit(audio_state_detail::Archive& a, AudioCpuInterruptWorkState& v) {
    a.fields(v.cartridge_flags, v.scroll, v.palette_delay, v.palette_index);
}
void visit(audio_state_detail::Archive& a, AudioDspState& v) {
    a.fields(v.clocks, v.hardware);
}
void visit(audio_state_detail::Archive& a, AudioEngineState& v) {
    a.fields(v.cpu, v.cpu_master, v.cpu_completed, v.ipl, v.driver_present, v.driver, v.incoming,
             v.outgoing, v.interrupt, v.interrupt_count, v.dsp, v.pending_pcm);
}
void visit(audio_state_detail::Archive& a, AudioCpuQueueState& v) {
    a.fields(v.parameters, v.commands, v.read_index, v.write_index, v.expected_phase);
}
void visit(audio_state_detail::Archive& a, AudioCpuSceneWorkState& v) {
    a.fields(v.phase, v.first_horizontal_flags, v.second_horizontal_flags, v.title_levels_pending,
             v.menu_mode, v.horizontal_current, v.horizontal_target, v.vertical_current,
             v.vertical_target);
}
void visit(audio_state_detail::Archive& a, AudioCpuTitleHoldState& v) {
    a.fields(v.remaining, v.code_index, v.controllers);
}
void visit(audio_state_detail::Archive& a, AudioCpuTextWorkState& v) {
    a.fields(v.cursor, v.attribute, v.prepared, v.drawn);
}
void visit(audio_state_detail::Archive& a, AudioCpuMenuInputState& v) {
    a.fields(v.idle_remaining, v.selection, v.direction_latched, v.controllers);
}
void visit(audio_state_detail::Archive& a, TitleMenuAudioState& v) {
    a.fields(v.phase, v.engine, v.queue, v.scene, v.title, v.text, v.menu, v.cartridge);
}
// URAU0001 includes owned transport, voice/timer continuation, 64KiB DSP RAM/history
// and unconsumed native PCM. Static content and future controller input are excluded.
std::vector<std::uint8_t> serialize_title_menu_audio(const TitleMenuAudioState& state) {
    audio_state_detail::Archive archive;
    std::array<std::uint8_t, 8> magic{'U', 'R', 'A', 'U', '0', '0', '0', '1'};
    auto owned = state;
    archive.fields(magic, owned);
    return archive.take_output();
}
TitleMenuAudioState deserialize_title_menu_audio(std::span<const std::uint8_t> bytes) {
    audio_state_detail::Archive archive(bytes);
    std::array<std::uint8_t, 8> magic{};
    archive.value(magic);
    constexpr std::array<std::uint8_t, 8> expected{'U', 'R', 'A', 'U', '0', '0', '0', '1'};
    if (magic != expected) throw std::invalid_argument("audio state format differs");
    TitleMenuAudioState state;
    archive.value(state);
    archive.require_end();
    return state;
}
} // namespace unirally
