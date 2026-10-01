// DSP-only adapter for the pinned SPC_DSP. LGPL-2.1-or-later, like the library.
// No original game code is fetched, decoded, uploaded or executed here.
#include "adapter.h"
#include <array>
#include <cstring>
#include <new>

// The sole bsnes configuration reference selects cubic interpolation. The
// recovered PAL domain uses Gaussian interpolation, fixed false here. Keep
// upstream's six source files byte-for-byte; no bsnes system/CPU is linked.
static struct {
    struct { struct { bool cubic = false; } dsp; } hacks;
} configuration;
#include "SPC_DSP.cpp"

static_assert(sizeof(SPC_DSP::sample_t) == sizeof(int16_t));
struct UnirallyDsp {
    std::array<uint8_t, 65536> ram{};
    SPC_DSP hardware;
    // Leave capacity beyond the maximum pair: upstream switches to its extra
    // buffer when the caller buffer fills, before sample_count is queried.
    std::array<SPC_DSP::sample_t, 4> output{};
    UnirallyDsp() {
        hardware.init(ram.data(), ram.data());
        hardware.reset();
        hardware.set_output(output.data(), 4);
    }
};
static constexpr size_t stateSize = 8 + 65536 + SPC_DSP::state_size;
static constexpr std::array<uint8_t, 8> stateHeader{'U', 'D', 'S', 'P', 1, 0, 0, 0};
static void savePart(unsigned char** out, void* state, size_t size) {
    std::memcpy(*out, state, size);
    *out += size;
}
static void loadPart(unsigned char** in, void* state, size_t size) {
    std::memcpy(state, *in, size);
    *in += size;
}
UnirallyDsp* unirally_dsp_create() { return new(std::nothrow) UnirallyDsp; }
void unirally_dsp_destroy(UnirallyDsp* dsp) { delete dsp; }
void unirally_dsp_reset(UnirallyDsp* dsp, int soft) {
    if (soft) dsp->hardware.soft_reset();
    else { dsp->ram.fill(0); dsp->hardware.reset(); }
    dsp->hardware.set_output(dsp->output.data(), 4);
}
void unirally_dsp_write_register(UnirallyDsp* dsp, uint8_t address, uint8_t value) {
    if (address < 128) dsp->hardware.write(address, value);
}
uint8_t unirally_dsp_read_register(const UnirallyDsp* dsp, uint8_t address) {
    return static_cast<uint8_t>(dsp->hardware.read(address & 127));
}
void unirally_dsp_write_ram(UnirallyDsp* dsp, uint16_t address, uint8_t value) { dsp->ram[address] = value; }
uint8_t unirally_dsp_read_ram(const UnirallyDsp* dsp, uint16_t address) { return dsp->ram[address]; }
int unirally_dsp_run(UnirallyDsp* dsp, uint32_t clocks, int16_t stereo[2]) {
    if (!dsp || !stereo || clocks == 0 || clocks > 32) return -1;
    dsp->hardware.run(static_cast<int>(clocks));
    const int count = dsp->hardware.sample_count();
    for (int i = 0; i < count; ++i) stereo[i] = dsp->output[static_cast<size_t>(i)];
    dsp->hardware.set_output(dsp->output.data(), 4);
    return count;
}
size_t unirally_dsp_state_size() { return stateSize; }
int unirally_dsp_save(UnirallyDsp* dsp, uint8_t* state, size_t size) {
    if (!dsp || !state || size != stateSize) return 0;
    std::memset(state, 0, size);
    std::memcpy(state, stateHeader.data(), stateHeader.size());
    std::memcpy(state + 8, dsp->ram.data(), dsp->ram.size());
    unsigned char* position = state + 8 + dsp->ram.size();
    dsp->hardware.copy_state(&position, savePart);
    return 1;
}
int unirally_dsp_load(UnirallyDsp* dsp, const uint8_t* state, size_t size) {
    if (!dsp || !state || size != stateSize ||
        std::memcmp(state, stateHeader.data(), stateHeader.size()) != 0) return 0;
    std::memcpy(dsp->ram.data(), state + 8, dsp->ram.size());
    // copy_state's callback only reads its input in this direction.
    unsigned char* position = const_cast<unsigned char*>(state + 8 + dsp->ram.size());
    dsp->hardware.copy_state(&position, loadPart);
    dsp->hardware.set_output(dsp->output.data(), 4);
    return 1;
}
