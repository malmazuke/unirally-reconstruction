#pragma once
#include "audio_score.hpp"
#include "audio_timers.hpp"

namespace unirally {
// A bus access is observed at its SMP tick, before any later driver work.
// The diagnostic adapter supplies CPU writes; the cold producer is pending.
class AudioDriverBus {
public:
    virtual ~AudioDriverBus() = default;
    virtual std::uint8_t read_port(std::uint64_t ticks, std::uint8_t port) = 0;
    virtual void write_port(std::uint64_t ticks, std::uint8_t port, std::uint8_t value) = 0;
    virtual void write_dsp(std::uint64_t ticks, std::uint8_t reg, std::uint8_t value) = 0;
    virtual void clear_ports(std::uint64_t ticks, std::uint8_t first_port) = 0;
    virtual void write_ram(std::uint64_t ticks, std::uint16_t address, std::uint8_t value) = 0;
};

// Semantic post-upload loop. Its diagnostic entry tick and initial hardware
// phase are explicit conditions, not a cold product initializer. R-0075.
// Full iteration calls can pass a requested horizon; adapters retain only
// events at/before it. A resumable pending-phase representation is still open.
class TitleMenuAudioDriver {
public:
    TitleMenuAudioDriver(const TitleMenuAudioData& data, const AudioPitchData& pitch,
                         AudioDriverBus& bus, const AudioTimersState& timers,
                         std::uint64_t entry_ticks, bool driver_boot = false);
    void run_until(std::uint64_t ticks);
    std::uint64_t ticks() const { return ticks_; }
    const TitleMenuAudioScore& score() const { return score_; }

private:
    TitleMenuAudioScore score_;
    AudioTimers timers_;
    AudioDriverBus* bus_;
    std::uint64_t ticks_;
    std::uint8_t command_phase_ = 128;
    std::uint8_t music_counter_ = 0, effect_counter_ = 0, update_counter_ = 0;
    std::uint8_t master_volume_ = 127;
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
};
} // namespace unirally
