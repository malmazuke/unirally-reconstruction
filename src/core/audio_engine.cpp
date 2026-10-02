#include "audio_engine.hpp"
#include <limits>
#include <stdexcept>
#include <utility>

namespace unirally {
namespace {
struct CpuYield {};
constexpr std::uint64_t cpu_frequency = 21281370, smp_frequency = 2050560;
}
NativeAudioEngine::NativeAudioEngine(const AudioSoundSet& score, const AudioPitchData& pitch,
                                     AudioControllerSource& controllers,
                                     AudioEngineEventSink* events)
    : sets_{&score, nullptr}, pitch_(&pitch), controllers_(&controllers), events_(events) {}
const AudioSoundSet& NativeAudioEngine::set(AudioSoundSetId id) const {
    const auto* found = sets_.at(static_cast<std::size_t>(id));
    if (!found) throw std::invalid_argument("audio content lacks this sound set");
    return *found;
}
void NativeAudioEngine::begin_sound_set_upload(AudioSoundSetId id) {
    set(id);
    uploading_set_ = id;
    if (!driver_) retain_uploading_set();
}
void NativeAudioEngine::retain_uploading_set() {
    const auto& uploading = set(uploading_set_);
    ipl_.retain_sound_set(static_cast<std::uint16_t>(uploading.tables.size()),
                          static_cast<std::uint16_t>(uploading.score.size()));
}
std::uint64_t NativeAudioEngine::smp_ticks() const {
    return driver_ ? driver_->ticks() : ipl_.state().ticks;
}
void NativeAudioEngine::check_cpu_yield(std::uint64_t ticks) const {
    if (ticks * cpu_frequency >= cpu_completed_ * smp_frequency) throw CpuYield{};
}
void NativeAudioEngine::advance_clock(std::uint64_t ticks) {
    dsp_.advance_to(((ticks + 63) / 64) * 32);
    constexpr std::uint64_t force_lead = 768ULL * 24 * 24000000;
    if (ticks * cpu_frequency > cpu_completed_ * smp_frequency + force_lead) throw CpuYield{};
}
std::uint8_t NativeAudioEngine::read_port(std::uint64_t ticks, std::uint8_t port) {
    check_cpu_yield(ticks);
    const auto value = incoming_.at(port);
    emit('R', ticks, port, value);
    return value;
}
void NativeAudioEngine::write_port(std::uint64_t ticks, std::uint8_t port, std::uint8_t value) {
    check_cpu_yield(ticks);
    outgoing_.at(port) = value;
    emit('P', ticks, port, value);
}
void NativeAudioEngine::write_ram(std::uint64_t ticks, std::uint16_t address, std::uint8_t value) {
    dsp_.write_ram(ticks, address, value);
    emit('N', ticks, address, value);
}
void NativeAudioEngine::write_dsp(std::uint64_t ticks, std::uint8_t reg, std::uint8_t value) {
    dsp_.write_register(ticks, reg, value);
    emit('D', ticks, reg, value);
}
void NativeAudioEngine::clear_ports(std::uint64_t ticks, std::uint8_t port) {
    check_cpu_yield(ticks);
    incoming_.at(port) = incoming_.at(port + 1U) = 0;
    emit('Z', ticks, port, 0);
}
void NativeAudioEngine::scanline(std::uint64_t ticks, std::uint64_t completed_ticks) {
    cpu_master_ = ticks;
    cpu_completed_ = completed_ticks;
    synchronize();
}
std::uint8_t NativeAudioEngine::read_audio_port(std::uint64_t ticks, std::uint8_t port) {
    cpu_master_ = cpu_completed_ = ticks;
    synchronize();
    const auto value = outgoing_.at(port);
    emit('Q', smp_ticks(), port, value);
    return value;
}
void NativeAudioEngine::write_audio_port(std::uint64_t ticks, std::uint8_t port,
                                         std::uint8_t value) {
    cpu_master_ = cpu_completed_ = ticks;
    synchronize();
    incoming_.at(port) = value;
    emit('C', smp_ticks(), port, value);
}
void NativeAudioEngine::nonmaskable_interrupt(AudioCpuWorkClock& clock) {
    ++interrupt_count_;
    native_audio_title_interrupt(clock, interrupt_state_);
}
std::uint16_t NativeAudioEngine::controller_input(std::uint64_t ticks, unsigned port) {
    return controllers_->controller_word(ticks, port);
}
void NativeAudioEngine::emit(char kind, std::uint64_t ticks, std::uint16_t address,
                             std::uint8_t value) {
    if (events_) events_->event(kind, ticks, cpu_master_, address, value);
}
// R-0075. CPU scanline/IO boundaries resume a retained native SMP access.
void NativeAudioEngine::synchronize() {
    if (smp_ticks() * cpu_frequency >= cpu_completed_ * smp_frequency) return;
    try {
        for (;;) {
            if (!driver_) {
                ipl_.run_until(std::numeric_limits<std::uint64_t>::max());
                active_set_ = uploading_set_;
                driver_ = std::make_unique<TitleMenuAudioDriver>(
                    set(active_set_), *pitch_, *this, ipl_timers_, ipl_.state().ticks, true, true);
            }
            driver_->run_until(std::numeric_limits<std::uint64_t>::max());
            const auto exited = driver_->snapshot();
            if (!exited.stopped_for_ipl)
                throw std::logic_error("native audio driver returned before IPL exit");
            ipl_timers_ = exited.timers;
            ipl_ = AudioIplHandshake(*this, exited.ticks);
            retain_uploading_set();
            driver_.reset();
        }
    } catch (const CpuYield&) {}
    if (pcm_sink_) collect_pcm();
}
void NativeAudioEngine::collect_pcm() {
    auto generated = dsp_.take_pcm();
    if (pcm_sink_) {
        if (!generated.empty()) pcm_sink_->append_pcm(generated);
    } else {
        pending_pcm_.insert(pending_pcm_.end(), generated.begin(), generated.end());
    }
}
void NativeAudioEngine::set_pcm_sink(AudioPcmSink* sink) {
    if (pcm_sink_) throw std::logic_error("native PCM sink already attached");
    collect_pcm();
    pcm_sink_ = sink;
    if (pcm_sink_ && !pending_pcm_.empty()) {
        pcm_sink_->append_pcm(pending_pcm_);
        pending_pcm_.clear();
    }
}
std::vector<std::int16_t> NativeAudioEngine::take_pcm() {
    collect_pcm();
    return std::exchange(pending_pcm_, {});
}
void NativeAudioEngine::finish_pcm_to(std::uint64_t clocks) {
    dsp_.advance_to(clocks);
}
AudioEngineState NativeAudioEngine::snapshot() {
    collect_pcm();
    AudioEngineState state;
    state.cpu = cpu_.snapshot();
    state.cpu_master = cpu_master_;
    state.cpu_completed = cpu_completed_;
    state.ipl = ipl_.state();
    state.ipl_timers = ipl_timers_;
    state.driver_present = static_cast<bool>(driver_);
    if (driver_) state.driver = driver_->snapshot();
    state.incoming = incoming_;
    state.outgoing = outgoing_;
    state.interrupt = interrupt_state_;
    state.interrupt_count = interrupt_count_;
    state.dsp = dsp_.snapshot();
    state.pending_pcm = pending_pcm_;
    state.sound_set = active_set_;
    state.uploading_sound_set = uploading_set_;
    return state;
}
void NativeAudioEngine::restore(const AudioEngineState& state) {
    if (state.cpu_master > state.cpu.ticks || state.cpu_completed > state.cpu_master
        || state.pending_pcm.size() % 2 || state.interrupt.palette_delay > 6
        || state.interrupt.palette_index > 3)
        throw std::invalid_argument("invalid native audio engine continuation");
    AudioCpuWorkClock candidate_cpu;
    candidate_cpu.restore(state.cpu);
    if (state.sound_set > AudioSoundSetId::first_race
        || state.uploading_sound_set > AudioSoundSetId::first_race)
        throw std::invalid_argument("invalid native audio sound set");
    const auto& active = set(state.sound_set);
    const auto& uploading = set(state.uploading_sound_set);
    AudioIplHandshake candidate_ipl(*this);
    candidate_ipl.restore(state.ipl);
    candidate_ipl.retain_sound_set(static_cast<std::uint16_t>(uploading.tables.size()),
                                   static_cast<std::uint16_t>(uploading.score.size()));
    AudioTimers candidate_timers;
    candidate_timers.restore(state.ipl_timers);
    if (state.ipl_timers.ticks > state.ipl.ticks)
        throw std::invalid_argument("retained timer clock follows IPL");
    std::unique_ptr<TitleMenuAudioDriver> candidate_driver;
    if (state.driver_present) {
        candidate_driver = std::make_unique<TitleMenuAudioDriver>(
            active, *pitch_, *this, AudioTimersState{}, 0, false, true);
        candidate_driver->restore(state.driver);
    }
    NativeAudioDsp candidate_dsp;
    candidate_dsp.restore(state.dsp);
    auto candidate_pcm = state.pending_pcm;
    cpu_.restore(state.cpu);
    ipl_.restore(state.ipl);
    ipl_.retain_sound_set(static_cast<std::uint16_t>(uploading.tables.size()),
                          static_cast<std::uint16_t>(uploading.score.size()));
    ipl_timers_ = state.ipl_timers;
    driver_ = std::move(candidate_driver);
    dsp_ = std::move(candidate_dsp);
    cpu_master_ = state.cpu_master;
    cpu_completed_ = state.cpu_completed;
    incoming_ = state.incoming;
    outgoing_ = state.outgoing;
    interrupt_state_ = state.interrupt;
    interrupt_count_ = state.interrupt_count;
    pending_pcm_ = std::move(candidate_pcm);
    active_set_ = state.sound_set;
    uploading_set_ = state.uploading_sound_set;
}
} // namespace unirally
