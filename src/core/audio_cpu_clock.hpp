#pragma once
#include <array>
#include <cstdint>
#include <span>

namespace unirally {
class AudioCpuWorkClock;
struct AudioRevealHdmaChannelState {
    std::uint16_t cursor = 0;
    std::uint8_t line_counter = 0;
    bool completed = false, transfer = false;
    bool operator==(const AudioRevealHdmaChannelState&) const = default;
};
// Direct channel-5 mode-2 and channel-6 mode-0 reveal tables at $0C70.
// The copied 110 bytes and following cleared RAM are mutable native data.
struct AudioRevealHdmaState {
    std::array<std::uint8_t, 256> ram{};
    std::array<AudioRevealHdmaChannelState, 2> channels{};
    bool enabled = false, setup_triggered = false, run_triggered = false;
    unsigned setup_position = 12;
    std::uint8_t pending = 0; // 0 none, 1 setup, 2 scanline transfer
    bool operator==(const AudioRevealHdmaState&) const = default;
};
// Native clock continuation at a semantic call boundary. Units: PAL CPU master clocks.
struct AudioCpuClockState {
    std::uint64_t ticks = 0, completed_step_ticks = 0, scanline = 0;
    bool refreshed = false, fast_rom = false;
    unsigned pending_dma_bytes = 0;
    bool dma_active = false;
    bool nmi_enabled = false, nmi_valid = false, nmi_line = false, nmi_hold = false;
    bool nmi_transition = false, nmi_pending = false, irq_lock = false;
    bool in_interrupt = false, auto_joypad_enabled = false;
    unsigned auto_joypad_counter = 33;
    std::array<std::uint16_t, 2> latched_controllers{}, controller_words{};
    AudioRevealHdmaState reveal_hdma;
    bool operator==(const AudioCpuClockState&) const = default;
};

class AudioCpuWorkObserver {
public:
    virtual ~AudioCpuWorkObserver() = default;
    // CPU::scanline synchronizes SMP before the enclosing bus step subtracts
    // its clocks from the CPU/SMP balance. Keep that completed-step clock.
    virtual void scanline(std::uint64_t master_ticks, std::uint64_t completed_step_ticks) = 0;
    virtual std::uint8_t read_audio_port(std::uint64_t master_ticks, std::uint8_t port);
    virtual void write_audio_port(std::uint64_t master_ticks, std::uint8_t port,
                                  std::uint8_t value) = 0;
    virtual void nonmaskable_interrupt(AudioCpuWorkClock& clock);
    virtual std::uint16_t controller_input(std::uint64_t master_ticks, unsigned port);
};
// Semantic CPU work, in master clocks, for the pinned PAL/version-2 bus.
// ROM reads account for bus work only: this class reads no ROM/opcode bytes.
// The recovered cold domain supports one DMA channel and native NMI work;
// Direct reveal HDMA uses two identified channels; IRQ, other HDMA configurations
// and interlaced raster timing remain outside this clock domain.
// R-0075. DMA alignment counts bus work separately from DRAM refresh stalls.
class AudioCpuWorkClock {
public:
    explicit AudioCpuWorkClock(AudioCpuWorkObserver* observer = nullptr) : observer_(observer) {}
    std::uint64_t ticks() const { return ticks_; }
    AudioCpuClockState snapshot() const;
    void restore(const AudioCpuClockState& state);
    void set_fast_rom(bool enabled) { fast_rom_ = enabled; }
    void step(unsigned clocks);
    // Semantic instruction boundaries and the one-bus-cycle-early NMI test.
    // Raw bus helpers do not read or interpret an instruction stream.
    void begin_instruction();
    void last_cycle();
    void rom_reads(unsigned count);
    void ram_reads(unsigned count = 1);
    void ram_writes(unsigned count = 1);
    void idle(unsigned count = 1);
    void load_constant(unsigned bytes = 1);
    void update_register();
    void change_widths();
    void save_register(unsigned bytes = 1);
    void restore_register(unsigned bytes = 1);
    void call_local();
    void call_far();
    void return_local();
    void return_far();
    void return_ram_far();
    void store_ram(unsigned bytes = 1, bool long_address = false, bool indexed = false);
    void store_direct(unsigned bytes = 1);
    void read_direct(unsigned bytes = 1);
    void read_ram(unsigned bytes = 1, bool long_address = false, bool indexed = false);
    void read_rom(unsigned bytes = 1, bool long_address = false, bool indexed = false);
    // Identified LoROM data below bank80 has the same 8-clock reads as RAM.
    void read_slow_rom(unsigned bytes = 1, bool long_address = false, bool indexed = false) {
        read_ram(bytes, long_address, indexed);
    }
    void read_stack(unsigned bytes = 1);
    void modify_direct_byte();
    void modify_direct_word();
    void modify_ram_byte();
    void move_ram_byte();
    void exchange_accumulator_bytes();
    void jump_far();
    void jump_indirect();
    void set_nmi_enabled(bool enabled);
    std::uint16_t read_controller(unsigned port);
    void store_port(unsigned bytes = 1, bool long_address = false);
    void read_port(unsigned bytes = 1);
    void branch(bool taken);
    void branch_long();
    void request_dma(unsigned bytes);
    void copy_reveal_tables(std::span<const std::uint8_t, 72> offsets,
                            std::span<const std::uint8_t, 38> brightness);
    void increment_reveal_header(unsigned offset);
    void set_reveal_hdma_enabled(bool enabled);
    void clear_reveal_work_ram();
    void write_audio_port(std::uint8_t port, std::uint8_t value);
    void write_audio_word(std::uint8_t first_port, std::uint16_t value);
    std::uint16_t read_audio_ports(std::uint8_t first_port, unsigned bytes = 1);
    bool read_vertical_blank();

private:
    std::uint64_t ticks_ = 0, completed_step_ticks_ = 0, scanline_ = 0;
    bool refreshed_ = false, fast_rom_ = false;
    unsigned pending_dma_bytes_ = 0;
    bool dma_active_ = false;
    bool nmi_enabled_ = false, nmi_valid_ = false, nmi_line_ = false, nmi_hold_ = false;
    bool nmi_transition_ = false, nmi_pending_ = false, irq_lock_ = false;
    bool in_interrupt_ = false;
    bool auto_joypad_enabled_ = false;
    unsigned auto_joypad_counter_ = 33;
    std::array<std::uint16_t, 2> latched_controllers_{}, controller_words_{};
    AudioRevealHdmaState reveal_hdma_;
    AudioCpuWorkObserver* observer_;
    void begin_bus(unsigned clocks);
    void poll_nmi();
    void poll_controllers();
    void poll_hdma();
    unsigned run_reveal_hdma(bool setup);
    void reload_reveal_channel(unsigned index, unsigned& bus_clocks);
    std::uint8_t read_reveal_byte(unsigned offset, unsigned& bus_clocks);
};
} // namespace unirally
