#pragma once
#include "audio_voice.hpp"

namespace unirally {
// Elapsed SMP ticks from arithmetic entry 09B4 to the command poll at 0992,
// with default memory wait states. This is semantic work accounting; it neither
// executes instructions nor reads code bytes. R-0075's clock experiment.
// Pitch data is required for nonzero convergence because the preceding note
// slide can change the target. Scripted envelopes remain outside this domain.
std::uint32_t audio_voice_work_ticks(const AudioVoiceArithmetic& voice, std::uint8_t update_counter,
                                     const AudioPitchData* pitch_data = nullptr);
} // namespace unirally
