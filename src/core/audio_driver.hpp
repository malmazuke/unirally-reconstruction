#pragma once
#include "audio_driver_pending.hpp"
#include "audio_score.hpp"
#include "audio_timers.hpp"

namespace unirally {
// A bus access is observed at its SMP tick, before any later driver work.
// The diagnostic adapter supplies CPU writes; the cold producer is pending.
class AudioDriverBus {
public:
    virtual ~AudioDriverBus() = default;
    // Coupled hardware can request bounded clock visits between accesses.
    // A visit may suspend before the matching timer step, like pinned SMP::step.
    virtual unsigned clock_sync_step() const { return 0; }
    virtual void advance_clock(std::uint64_t) {}
    virtual std::uint8_t read_port(std::uint64_t ticks, std::uint8_t port) = 0;
    virtual void write_port(std::uint64_t ticks, std::uint8_t port, std::uint8_t value) = 0;
    virtual void write_dsp(std::uint64_t ticks, std::uint8_t reg, std::uint8_t value) = 0;
    virtual void clear_ports(std::uint64_t ticks, std::uint8_t first_port) = 0;
    virtual void write_ram(std::uint64_t ticks, std::uint16_t address, std::uint8_t value) = 0;
};

// Semantic post-upload loop. Its diagnostic entry tick and initial hardware
// phase are explicit conditions, not a cold product initializer. R-0075.
// The laboratory can compare the original synchronous form with an explicit
// pending-phase form, which retains a CPU-yielding access until it completes.
class TitleMenuAudioDriver {
public:
    TitleMenuAudioDriver(const TitleMenuAudioData& data, const AudioPitchData& pitch,
                         AudioDriverBus& bus, const AudioTimersState& timers,
                         std::uint64_t entry_ticks, bool driver_boot = false,
                         bool resumable = false);
    void run_until(std::uint64_t ticks);
    std::uint64_t ticks() const { return ticks_; }
    const TitleMenuAudioScore& score() const { return score_; }
    const AudioTimersState& timers() const { return timers_.state(); }
    bool returned_to_ipl() const { return stopped_for_ipl_; }
    AudioDriverSnapshot snapshot() const;
    void restore(const AudioDriverSnapshot& snapshot);

private:
    TitleMenuAudioScore score_;
    AudioTimers timers_;
    AudioDriverBus* bus_;
    std::uint64_t ticks_;
    std::uint8_t command_phase_ = 128;
    std::uint8_t music_counter_ = 0, effect_counter_ = 0, update_counter_ = 0;
    std::uint8_t master_volume_ = 127;
    bool stopped_for_ipl_ = false;
    bool resumable_ = false, planning_ = false;
    std::uint64_t planned_ticks_ = 0;
    AudioDriverContinuation continuation_;
    void advance(unsigned ticks);
    std::uint8_t read_port(std::uint8_t port);
    void write_port(std::uint8_t port, std::uint8_t value);
    void write_dsp(unsigned ticks, std::uint8_t reg, std::uint8_t value);
    void poll_commands();
    void configure_timers();
    void execute_command(std::uint8_t command, std::uint8_t parameter);
    void update_voices(bool effects);
    void update_score_voice(std::uint8_t voice, bool effects);
    void output_voice(std::uint8_t voice);
    void iteration();
    void boot();
    void initialize_dsp();
    void initialize_voice_ram();
    void load_samples();
    std::uint8_t read_upload_byte(std::uint8_t& phase);
    void load_sample_bytes(std::uint8_t& phase, std::uint16_t& cursor);
    void write_ram(unsigned ticks, std::uint16_t address, std::uint8_t value);
    void stop_for_ipl();
    void write_control(std::uint8_t value);
    void write_target(std::uint8_t timer, std::uint8_t value);
    void clear_ports(std::uint8_t first_port);
    void queue_io(AudioDriverIoKind kind, std::uint16_t address, std::uint8_t value = 0);
    void finish_plan(AudioDriverPhase phase);
    void plan_read_port(std::uint8_t port);
    void plan_read_timer(std::uint8_t timer);
    void plan_descriptor(AudioDriverPhase return_phase);
    void plan_poll(AudioDriverPhase return_phase);
    void run_pending_until(std::uint64_t exclusive_ticks);
    void advance_pending_clock(std::uint64_t ticks);
    void execute_pending_io(const AudioDriverPendingIo& operation);
    void plan_phase();
    void plan_boot_phase();
    void plan_descriptor_phase();
    void plan_sample_phase();
    void plan_bulk_phase();
    void plan_poll_phase();
    void plan_main_phase();
    void plan_update_phase();
    void plan_output_voice();
    void plan_stop_phase();
};
} // namespace unirally
