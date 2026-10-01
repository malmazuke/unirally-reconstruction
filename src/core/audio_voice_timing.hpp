#pragma once
#include "audio_voice.hpp"

namespace unirally {
// Elapsed SMP ticks from arithmetic entry 09B4 to the command poll at 0992,
// with default memory wait states. This is semantic work accounting; it neither
// executes instructions nor reads code bytes. R-0075's clock experiment.
// The current timed domain excludes note-space slides, pitch convergence and
// scripted envelopes. Their value calculations remain separate.
std::uint32_t audio_voice_work_ticks(const AudioVoiceArithmetic& voice, std::uint8_t update_counter);
}  // namespace unirally
