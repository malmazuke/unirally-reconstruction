#include "audio_cpu_boot.hpp"
#include "audio_cpu_interrupt.hpp"
#include "audio_cpu_queue.hpp"
#include "audio_cpu_scene.hpp"
#include "audio_cpu_text.hpp"
#include "audio_cpu_menu_input.hpp"
#include "audio_cpu_upload.hpp"
#include "audio_driver_dsp.hpp"
#include "audio_ipl.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <memory>
#include <stdexcept>

namespace {
struct CpuYield {};
class Bus final : public unirally::AudioDriverBus, public unirally::AudioCpuWorkObserver {
public:
    std::unique_ptr<DriverDspDiagnostic> dsp;
    unirally::AudioCpuInterruptWorkState interrupt_state;
    unsigned interrupt_count = 0;
    std::vector<std::array<std::uint16_t, 2>> controller_schedule;
    std::uint16_t controller_input(std::uint64_t ticks, unsigned port) override {
        const auto frame = ticks / 425568 + 1;
        return frame < controller_schedule.size() ? controller_schedule[frame][port] : 0;
    }
    void nonmaskable_interrupt(unirally::AudioCpuWorkClock& clock) override {
        ++interrupt_count;
        unirally::native_audio_title_interrupt(clock, interrupt_state);
    }
    Bus(std::ostream& output, const unirally::TitleMenuAudioData& score,
        const unirally::AudioPitchData& pitch, bool ready_mode)
        : output_(output), ipl_(*this), score_(&score), pitch_(&pitch), ready_mode_(ready_mode) {}
    std::uint8_t read_port(std::uint64_t ticks, std::uint8_t port) override {
        if (ticks * cpu_frequency >= cpu_completed_ * smp_frequency) throw CpuYield{};
        const auto value = incoming_.at(port);
        emit('R', ticks, port, value);
        return value;
    }
    void write_port(std::uint64_t ticks, std::uint8_t port, std::uint8_t value) override {
        if (ticks * cpu_frequency >= cpu_completed_ * smp_frequency) throw CpuYield{};
        outgoing_.at(port) = value;
        emit('P', ticks, port, value);
    }
    void write_ram(std::uint64_t ticks, std::uint16_t address, std::uint8_t value) override {
        if (dsp) dsp->write_ram(ticks, address, value);
        emit('N', ticks, address, value);
    }
    void write_dsp(std::uint64_t ticks, std::uint8_t reg, std::uint8_t value) override {
        if (dsp) dsp->write_register(ticks, reg, value);
        emit('D', ticks, reg, value);
    }
    void clear_ports(std::uint64_t ticks, std::uint8_t port) override {
        if (ticks * cpu_frequency >= cpu_completed_ * smp_frequency) throw CpuYield{};
        incoming_.at(port) = incoming_.at(port + 1U) = 0;
        emit('Z', ticks, port, 0);
    }
    unsigned clock_sync_step() const override { return 2; }
    void advance_clock(std::uint64_t ticks) override {
        constexpr std::uint64_t force_lead = 768ULL * 24 * 24000000;
        if (ticks * cpu_frequency > cpu_completed_ * smp_frequency + force_lead) throw CpuYield{};
    }
    void scanline(std::uint64_t ticks, std::uint64_t completed_ticks) override {
        cpu_master_ = ticks;
        cpu_completed_ = completed_ticks;
        synchronize();
    }
    std::uint8_t read_audio_port(std::uint64_t ticks, std::uint8_t port) override {
        cpu_master_ = cpu_completed_ = ticks;
        synchronize();
        const auto value = outgoing_.at(port);
        emit('Q', smp_ticks(), port, value);
        return value;
    }
    void write_audio_port(std::uint64_t ticks, std::uint8_t port, std::uint8_t value) override {
        cpu_master_ = cpu_completed_ = ticks;
        synchronize();
        incoming_.at(port) = value;
        emit('C', smp_ticks(), port, value);
    }

private:
    static constexpr std::uint64_t cpu_frequency = 21281370, smp_frequency = 2050560;
    std::ostream& output_;
    unirally::AudioIplHandshake ipl_;
    const unirally::TitleMenuAudioData* score_;
    const unirally::AudioPitchData* pitch_;
    bool ready_mode_;
    std::unique_ptr<unirally::TitleMenuAudioDriver> driver_;
    std::array<std::uint8_t, 4> incoming_{}, outgoing_{};
    std::uint64_t smp_ticks() const { return driver_ ? driver_->ticks() : ipl_.state().ticks; }
    std::uint64_t cpu_master_ = 0, cpu_completed_ = 0;
    void emit(char kind, std::uint64_t ticks, std::uint16_t address, std::uint8_t value) {
        output_ << kind << ' ' << ticks << ' ' << cpu_master_ << ' ' << address << ' ';
        output_ << unsigned(value) << '\n';
    }
    void synchronize() {
        if (smp_ticks() * cpu_frequency >= cpu_completed_ * smp_frequency) return;
        try {
            if (!driver_) {
                ipl_.run_until(std::numeric_limits<std::uint64_t>::max());
                if (ipl_.driver_ready()) {
                    if (!ready_mode_) throw std::logic_error("upload crossed its domain");
                    driver_ = std::make_unique<unirally::TitleMenuAudioDriver>(
                        *score_, *pitch_, *this, unirally::AudioTimersState{}, ipl_.state().ticks,
                        true, true);
                }
            }
            if (driver_) driver_->run_until(std::numeric_limits<std::uint64_t>::max());
        } catch (const CpuYield&) {}
    }
};
std::vector<std::uint8_t> read(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("cannot read identified upload data");
    return {std::istreambuf_iterator<char>(file), {}};
}
}
int main(int argc, char** argv) {
    try {
        if (argc != 3 && argc != 4 && argc != 6)
            throw std::invalid_argument(
                "audio_cpu_upload_runner DATA_DIRECTORY OUTPUT "
                "[ready|samples|queue|vblank|frame|palette|load|nintendo|"
                "title|first-nmi|title-fade|title-hold|title-return|menu-palette|"
                "menu-graphics|menu-oam|menu-clear|menu-records-first|menu-league|"
                "menu-records|menu-text|menu-reveal|menu-input-begin|menu-idle80|menu-down80|menu-"
                "up500|menu-retrigger500|menu-steered1800 [PCM "
                "DSP_CLOCK_LIMIT]]");
        const std::string menu_case = argc >= 4 ? argv[3] : "";
        const bool menu_down80 = menu_case == "menu-down80";
        const bool menu_up500 = menu_case == "menu-up500";
        const bool menu_retrigger500 = menu_case == "menu-retrigger500";
        const bool menu_steered1800 = menu_case == "menu-steered1800";
        const bool menu_idle80 = menu_case == "menu-idle80" || menu_down80 || menu_up500
                              || menu_retrigger500 || menu_steered1800;
        const bool menu_input =
            argc >= 4 && (std::string(argv[3]) == "menu-input-begin" || menu_idle80);
        const bool menu_reveal = argc >= 4 && (std::string(argv[3]) == "menu-reveal" || menu_input);
        const bool menu_text = argc >= 4 && (std::string(argv[3]) == "menu-text" || menu_reveal);
        const bool menu_records =
            argc >= 4 && (std::string(argv[3]) == "menu-records" || menu_text);
        const bool menu_league =
            argc >= 4 && (std::string(argv[3]) == "menu-league" || menu_records);
        const bool menu_records_first =
            argc >= 4 && (std::string(argv[3]) == "menu-records-first" || menu_league);
        const bool menu_clear =
            argc >= 4 && (std::string(argv[3]) == "menu-clear" || menu_records_first);
        const bool menu_oam = argc >= 4 && (std::string(argv[3]) == "menu-oam" || menu_clear);
        const bool menu_graphics =
            argc >= 4 && (std::string(argv[3]) == "menu-graphics" || menu_oam);
        const bool menu_palette =
            argc >= 4 && (std::string(argv[3]) == "menu-palette" || menu_graphics);
        const bool title_return =
            argc >= 4 && (std::string(argv[3]) == "title-return" || menu_palette);
        const bool title_hold = argc >= 4 && (std::string(argv[3]) == "title-hold" || title_return);
        const bool title_fade = argc >= 4 && (std::string(argv[3]) == "title-fade" || title_hold);
        const bool first_nmi = argc >= 4 && (std::string(argv[3]) == "first-nmi" || title_fade);
        const bool title = argc >= 4 && (std::string(argv[3]) == "title" || first_nmi);
        const bool nintendo = argc >= 4 && (std::string(argv[3]) == "nintendo" || title);
        const bool load = argc >= 4 && (std::string(argv[3]) == "load" || nintendo);
        const bool palette = argc >= 4 && (std::string(argv[3]) == "palette" || load);
        const bool frame = argc >= 4 && (std::string(argv[3]) == "frame" || palette);
        const bool vblank = argc >= 4 && (std::string(argv[3]) == "vblank" || frame);
        const bool queue = argc >= 4 && (std::string(argv[3]) == "queue" || vblank);
        const bool samples = argc >= 4 && (std::string(argv[3]) == "samples" || queue);
        const bool ready = argc >= 4 && (std::string(argv[3]) == "ready" || samples);
        if (argc >= 4 && !ready) throw std::invalid_argument("unknown upload mode");
        const std::string root = argv[1];
        unirally::AudioCpuUploadData data;
        std::ifstream lengths(root + "/cpu-resource-lengths.txt");
        for (auto& length : data.resource_lengths) {
            unsigned value;
            if (!(lengths >> value) || value > 65535)
                throw std::runtime_error("invalid resource length");
            length = static_cast<std::uint16_t>(value);
        }
        data.menu_transfer = read(root + "/cpu-menu-upload.bin");
        data.title_transfer = read(root + "/cpu-title-upload.bin");
        if (samples) {
            const auto slots = read(root + "/cpu-sample-slots.bin");
            if (slots.size() != 64) throw std::invalid_argument("invalid sample slots");
            std::copy(slots.begin(), slots.end(), data.sample_slots.begin());
            for (const auto sample : slots) {
                if (sample == 255) continue;
                if (sample >= data.sample_resources.size())
                    throw std::invalid_argument("sample index");
                const auto name = std::to_string(sample);
                data.sample_resources[sample] =
                    read(root + "/cpu-sample-resource-" + (sample < 10 ? "0" : "") + name + ".bin");
            }
        }
        std::ofstream output(argv[2]);
        if (!output) throw std::runtime_error("cannot write native upload events");
        const unirally::TitleMenuAudioData score{read(root + "/menu-tables.bin"),
                                                 read(root + "/title-score.bin")};
        unirally::AudioPitchData pitch;
        const auto notes = read(root + "/pitch-table.bin");
        const auto fraction = read(root + "/sample-fractions.bin");
        const auto transpose = read(root + "/sample-transpose.bin");
        if (notes.size() != 194 || fraction.size() != 64 || transpose.size() != 64)
            throw std::invalid_argument("invalid pitch data");
        for (std::size_t i = 0; i < pitch.notes.size(); ++i)
            pitch.notes[i] =
                static_cast<std::uint16_t>(notes[2 * i] | unsigned(notes[2 * i + 1]) << 8);
        std::copy(fraction.begin(), fraction.end(), pitch.sample_fraction.begin());
        std::copy(transpose.begin(), transpose.end(), pitch.sample_transpose.begin());
        Bus bus(output, score, pitch, ready);
        const std::string menu_input_name =
            menu_down80         ? "cpu-controller-inputs-down500.txt"
            : menu_up500        ? "cpu-controller-inputs-up500.txt"
            : menu_retrigger500 ? "cpu-controller-inputs-retrigger500.txt"
            : menu_steered1800  ? "cpu-controller-inputs-steered1800.txt"
                                : "cpu-controller-inputs.txt";
        std::ifstream controller_inputs(root + "/" + menu_input_name);
        unsigned input_frame, input_first, input_second;
        while (controller_inputs >> input_frame >> input_first >> input_second) {
            if (input_frame != bus.controller_schedule.size() || input_first > 65535
                || input_second > 65535)
                throw std::invalid_argument("invalid native controller schedule");
            bus.controller_schedule.push_back({static_cast<std::uint16_t>(input_first),
                                               static_cast<std::uint16_t>(input_second)});
        }
        if (argc == 6)
            bus.dsp = std::make_unique<DriverDspDiagnostic>(argv[4], std::stoull(argv[5]));
        unirally::AudioCpuWorkClock clock(&bus);
        unirally::native_audio_cpu_boot_prefix(clock);
        unirally::native_audio_cpu_uploads(clock, data);
        if (ready) unirally::native_audio_cpu_finish_driver_entry(clock);
        if (samples) unirally::native_audio_cpu_upload_samples(clock, data);
        if (queue) {
            unirally::AudioCpuQueueState state;
            unirally::native_audio_bootstrap_queue(clock, state);
            if (vblank) unirally::native_audio_wait_vblank(clock, state);
            if (frame) {
                unirally::AudioCpuSceneWorkState scene;
                unirally::native_audio_finish_waited_frame(clock, scene);
                if (palette) unirally::native_audio_upload_base_palette(clock, state, scene);
                if (load) {
                    std::array<unirally::AudioCpuGraphicsAsset, 89> assets;
                    std::ifstream metadata(root + "/cpu-graphics-assets.txt");
                    unsigned id, bank, address, bytes, compressed;
                    while (metadata >> id >> bank >> address >> bytes >> compressed) {
                        if (id >= assets.size() || bank > 127 || address > 65535 || compressed > 1)
                            throw std::invalid_argument("invalid graphics metadata");
                        assets.at(id) = {static_cast<std::uint8_t>(bank),
                                         static_cast<std::uint16_t>(address), bytes,
                                         compressed != 0};
                    }
                    if (!metadata.eof())
                        throw std::invalid_argument("incomplete graphics metadata");
                    unirally::native_audio_load_nintendo_graphics(clock, scene, assets[31],
                                                                  assets[80], assets[74]);
                    if (nintendo)
                        unirally::native_audio_finish_nintendo_screen(clock, state, scene);
                    if (title)
                        unirally::native_audio_load_title_graphics(
                            clock, scene, bus.interrupt_state, assets[27], assets[78], assets[72]);
                    if (first_nmi) {
                        unirally::native_audio_first_title_interrupt(clock);
                        if (title_fade)
                            unirally::native_audio_title_fade(clock, state, scene, false);
                        if (title_hold) {
                            unirally::AudioCpuTitleHoldState hold;
                            unirally::native_audio_begin_title_hold(clock);
                            bool more;
                            do {
                                more = unirally::native_audio_title_hold_frame(clock, state, scene,
                                                                               hold);
                                std::cout << "title_frame_end=" << clock.ticks()
                                          << " pads=" << hold.controllers[0] << ','
                                          << hold.controllers[1] << '\n';
                            } while (more);
                        }
                        if (title_return) {
                            clock.call_local();
                            unirally::native_audio_title_fade(clock, state, scene, true);
                            clock.return_local();
                        }
                        if (menu_palette) {
                            std::ifstream menu_metadata(root
                                                        + (menu_graphics
                                                               ? "/cpu-menu-graphics-assets-v2.txt"
                                                               : "/cpu-menu-graphics-assets.txt"));
                            std::array<unirally::AudioCpuGraphicsAsset, 128> menu_assets{};
                            while (menu_metadata >> id >> bank >> address >> bytes >> compressed) {
                                if (id >= menu_assets.size() || bank > 127 || address > 65535
                                    || compressed > 1)
                                    throw std::invalid_argument("invalid menu resource metadata");
                                menu_assets[id] = {static_cast<std::uint8_t>(bank),
                                                   static_cast<std::uint16_t>(address), bytes,
                                                   compressed != 0};
                            }
                            unirally::native_audio_menu_first_palette(
                                clock, state, scene, bus.interrupt_state, menu_assets[1]);
                            if (menu_graphics)
                                unirally::native_audio_menu_graphics(clock, scene, menu_assets);
                            if (menu_oam) unirally::native_audio_menu_oam(clock, scene);
                            if (menu_clear) {
                                const auto signature = read(root + "/cpu-cartridge-signature.bin");
                                if (signature.size() != 12)
                                    throw std::invalid_argument("invalid cartridge signature");
                                std::array<std::uint8_t, 8192> cartridge;
                                cartridge.fill(255);
                                const bool cleared = unirally::native_audio_begin_menu_records(
                                    clock, state, scene, cartridge,
                                    std::span<const std::uint8_t, 12>(signature.data(), 12));
                                if (!cleared) throw std::logic_error("cold cartridge not cleared");
                                if (menu_records_first) {
                                    const auto defaults =
                                        read(root + "/cpu-cartridge-defaults-v2.bin");
                                    const auto types = read(root + "/cpu-track-types.bin");
                                    if (defaults.size() != 1158 || types.size() != 50)
                                        throw std::invalid_argument("invalid cartridge defaults");
                                    unirally::native_audio_menu_first_record_defaults(
                                        clock, cartridge,
                                        std::span<const std::uint8_t, 1158>(defaults.data(), 1158),
                                        std::span<const std::uint8_t, 50>(types.data(), 50));
                                    if (menu_league)
                                        unirally::native_audio_menu_league_defaults(clock,
                                                                                    cartridge);
                                    if (menu_records)
                                        unirally::native_audio_finish_menu_records(
                                            clock, scene, cartridge,
                                            std::span<const std::uint8_t, 1158>(defaults.data(),
                                                                                1158));
                                    if (menu_text) {
                                        const auto text = read(root + "/cpu-menu-text.bin");
                                        const auto characters =
                                            read(root + "/cpu-character-table.bin");
                                        if (characters.size() != 256)
                                            throw std::invalid_argument("invalid character table");
                                        unirally::AudioCpuTextWorkState printer;
                                        unirally::native_audio_begin_menu_text(
                                            clock, printer, text,
                                            std::span<const std::uint8_t, 256>(characters.data(),
                                                                               256));
                                        if (menu_reveal)
                                            unirally::native_audio_reveal_main_menu(
                                                clock, state, scene, printer, cartridge);
                                        if (menu_input) {
                                            unirally::AudioCpuMenuInputState menu;
                                            unirally::native_audio_begin_menu_input(
                                                clock, scene, cartridge, menu);
                                            const auto positions =
                                                read(root + "/cpu-menu-arrow-positions.bin");
                                            if (positions.size() != 5)
                                                throw std::invalid_argument(
                                                    "invalid arrow position data");
                                            if (menu_idle80)
                                                for (unsigned i = 0; i < (menu_steered1800 ? 1380U : 80U); ++i) {
                                                    const auto action =
                                                        unirally::native_audio_menu_input_frame(
                                                            clock, state, scene, cartridge, menu,
                                                            std::span<const std::uint8_t, 5>(
                                                                positions.data(), 5));
                                                    if (action
                                                        != unirally::AudioCpuMenuAction::waiting)
                                                        throw std::logic_error(
                                                            "menu loop crossed recovered domain");
                                                    std::cout << "menu_frame_end=" << clock.ticks()
                                                              << " pads=" << menu.controllers[0]
                                                              << ',' << menu.controllers[1] << '\n';
                                                }
                                            std::cout
                                                << "menu_selection=" << unsigned(menu.selection)
                                                << " menu_idle=" << menu.idle_remaining << '\n';
                                        }
                                        std::cout << "text_cursor=" << printer.cursor
                                                  << " text_attribute=" << printer.attribute
                                                  << '\n';
                                    }
                                }
                            }
                        }
                        std::cout << "palette_delay=" << unsigned(bus.interrupt_state.palette_delay)
                                  << " palette_index="
                                  << unsigned(bus.interrupt_state.palette_index) << '\n';
                        std::cout << "interrupt_count=" << bus.interrupt_count << '\n';
                    }
                }
                std::cout << "scene_phase=" << unsigned(scene.phase) << '\n';
            }
            std::cout << "queue_read=" << unsigned(state.read_index)
                      << " queue_write=" << unsigned(state.write_index)
                      << " expected_phase=" << unsigned(state.expected_phase) << '\n';
        }
        std::cout << "computed_final_upload_cpu_clock=" << clock.ticks() << '\n';
        if (bus.dsp) bus.dsp->finish();
        if (!output) throw std::runtime_error("native upload events failed");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
