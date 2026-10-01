#ifndef UNIRALLY_SPC_DSP_ADAPTER_H
#define UNIRALLY_SPC_DSP_ADAPTER_H

#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32)
# if defined(UNIRALLY_DSP_BUILD)
#  define UNIRALLY_DSP_API __declspec(dllexport)
# else
#  define UNIRALLY_DSP_API __declspec(dllimport)
# endif
#else
# define UNIRALLY_DSP_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* DSP hardware only. No CPU, sequencer, cartridge or device audio interface.
 * Units: one DSP clock; 32 clocks generate one signed 16-bit stereo pair.
 * Register reads mirror bit 7; writes with bit 7 set are ignored.
 * The caller serializes access. All RAM/echo state belongs to this instance. */
struct UnirallyDsp;
UNIRALLY_DSP_API struct UnirallyDsp* unirally_dsp_create(void);
UNIRALLY_DSP_API void unirally_dsp_destroy(struct UnirallyDsp* dsp);
UNIRALLY_DSP_API void unirally_dsp_reset(struct UnirallyDsp* dsp, int soft);
UNIRALLY_DSP_API void unirally_dsp_write_register(struct UnirallyDsp* dsp, uint8_t address, uint8_t value);
UNIRALLY_DSP_API uint8_t unirally_dsp_read_register(const struct UnirallyDsp* dsp, uint8_t address);
UNIRALLY_DSP_API void unirally_dsp_write_ram(struct UnirallyDsp* dsp, uint16_t address, uint8_t value);
UNIRALLY_DSP_API uint8_t unirally_dsp_read_ram(const struct UnirallyDsp* dsp, uint16_t address);
/* Return 0 or 2 samples; -1 for invalid arguments. Clocks must be 1..32. */
UNIRALLY_DSP_API int unirally_dsp_run(struct UnirallyDsp* dsp, uint32_t clocks, int16_t stereo[2]);
/* Snapshot format version 1 includes 64K shared RAM/echo and DSP histories.
 * Load only a snapshot produced by this exact adapter version. Size and magic
 * are checked; it is not a parser for untrusted SPC files or emulator states. */
UNIRALLY_DSP_API size_t unirally_dsp_state_size(void);
UNIRALLY_DSP_API int unirally_dsp_save(struct UnirallyDsp* dsp, uint8_t* state, size_t size);
UNIRALLY_DSP_API int unirally_dsp_load(struct UnirallyDsp* dsp, const uint8_t* state, size_t size);

#ifdef __cplusplus
}
#endif
#endif
