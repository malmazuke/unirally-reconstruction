#pragma once
#include "audio_cpu_interrupt.hpp"
#include "audio_cpu_upload.hpp"
#include "audio_dsp.hpp"
#include "audio_ipl.hpp"

namespace unirally {
class AudioControllerSource {
public:
    virtual ~AudioControllerSource() = default;
    virtual std::uint16_t controller_word(std::uint64_t cpu_ticks, unsigned port) = 0;
};
class AudioPcmSink {
public:
    virtual ~AudioPcmSink() = default;
    // Interleaved signed stereo at 32040 pairs/second; observe output only.
    virtual void append_pcm(std::span<const std::int16_t> samples) = 0;
};
class AudioEngineEventSink {
public:
    virtual ~AudioEngineEventSink() = default;
    virtual void event(char kind, std::uint64_t smp_ticks, std::uint64_t cpu_ticks,
                       std::uint16_t address, std::uint8_t value) = 0;
};
struct AudioEngineState {
    AudioCpuClockState cpu;
    std::uint64_t cpu_master = 0, cpu_completed = 0;
    AudioIplState ipl;
    AudioTimersState ipl_timers;
    bool driver_present = false;
    AudioDriverSnapshot driver;
    std::array<std::uint8_t, 4> incoming{}, outgoing{};
    AudioCpuInterruptWorkState interrupt;
    std::uint32_t interrupt_count = 0;
    AudioDspState dsp;
    std::vector<std::int16_t> pending_pcm;
    // The driver's current sound set and the one the CPU's latest session uploads.
    AudioSoundSetId sound_set = AudioSoundSetId::title,
                    uploading_sound_set = AudioSoundSetId::title;
};
// One native CPU/IPL/sequencer/DSP clock owner. Sources supply controller words
// only. Event sinks observe output; neither supplies timestamps or commands.
class NativeAudioEngine final : public AudioDriverBus, public AudioCpuWorkObserver {
public:
    NativeAudioEngine(const AudioSoundSet& score, const AudioPitchData& pitch,
                      AudioControllerSource& controllers, AudioEngineEventSink* events = nullptr);
    NativeAudioEngine(const NativeAudioEngine&) = delete;
    NativeAudioEngine& operator=(const NativeAudioEngine&) = delete;
    NativeAudioEngine(NativeAudioEngine&&) = delete;
    NativeAudioEngine& operator=(NativeAudioEngine&&) = delete;
    AudioCpuWorkClock& cpu() { return cpu_; }
    AudioCpuInterruptWorkState& interrupt() { return interrupt_state_; }
    // A set other than the title's, when the content carries it: the race songs (packs v32 and
    // v33), the award and the endings (v34).
    void set_sound_set(AudioSoundSetId id, const AudioSoundSet& set) {
        sets_.at(static_cast<std::size_t>(id)) = &set;
    }
    // The CPU designates the set its next upload session carries, before the FF request.
    void begin_sound_set_upload(AudioSoundSetId set);
    AudioSoundSetId sound_set() const { return active_set_; }
    std::vector<std::int16_t> take_pcm();
    void set_pcm_sink(AudioPcmSink* sink);
    AudioEngineState snapshot();
    void restore(const AudioEngineState& state);
    // Laboratory packaging boundary only; product work advances through native accesses.
    void finish_pcm_to(std::uint64_t dsp_clocks);
    unsigned clock_sync_step() const override { return 2; }
    void advance_clock(std::uint64_t ticks) override;
    std::uint8_t read_port(std::uint64_t ticks, std::uint8_t port) override;
    void write_port(std::uint64_t ticks, std::uint8_t port, std::uint8_t value) override;
    void write_ram(std::uint64_t ticks, std::uint16_t address, std::uint8_t value) override;
    void write_dsp(std::uint64_t ticks, std::uint8_t reg, std::uint8_t value) override;
    void clear_ports(std::uint64_t ticks, std::uint8_t first_port) override;
    void scanline(std::uint64_t master_ticks, std::uint64_t completed_step_ticks) override;
    std::uint8_t read_audio_port(std::uint64_t ticks, std::uint8_t port) override;
    void write_audio_port(std::uint64_t ticks, std::uint8_t port, std::uint8_t value) override;
    void nonmaskable_interrupt(AudioCpuWorkClock& clock) override;
    std::uint16_t controller_input(std::uint64_t ticks, unsigned port) override;

private:
    AudioCpuWorkClock cpu_{this};
    NativeAudioDsp dsp_;
    AudioIplHandshake ipl_{*this};
    AudioTimersState ipl_timers_;
    std::array<const AudioSoundSet*, sound_set_count> sets_{};
    AudioSoundSetId active_set_ = AudioSoundSetId::title, uploading_set_ = AudioSoundSetId::title;
    const AudioSoundSet& set(AudioSoundSetId id) const;
    void retain_uploading_set();
    const AudioPitchData* pitch_;
    AudioControllerSource* controllers_;
    AudioEngineEventSink* events_;
    AudioPcmSink* pcm_sink_ = nullptr;
    std::unique_ptr<TitleMenuAudioDriver> driver_;
    std::array<std::uint8_t, 4> incoming_{}, outgoing_{};
    std::uint64_t cpu_master_ = 0, cpu_completed_ = 0;
    AudioCpuInterruptWorkState interrupt_state_;
    std::uint32_t interrupt_count_ = 0;
    std::vector<std::int16_t> pending_pcm_;
    void synchronize();
    void emit(char kind, std::uint64_t ticks, std::uint16_t address, std::uint8_t value);
    void collect_pcm();
    std::uint64_t smp_ticks() const;
    void check_cpu_yield(std::uint64_t ticks) const;
};
} // namespace unirally
