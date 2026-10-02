#pragma once
#include "audio_cue.hpp"

#include <cstdint>
#include <span>

namespace unirally {
struct ZoomZooState;

// A race update's sound queue work, appended to `ZoomZooState::sound_cues` in the original's
// program order (AUDIO-FIRST-RACE, R-0076). Commands are the sound driver's: 2 starts an
// effect, 3 sets the music fade rate, 6/11 clear/set a score flag that effects test.
namespace race_sound {
// $83:F65E-F67A: a paused update's eight dispatcher calls; the update that opens the pause also
// fades the music out (command 3, 0x80) and sends it at $83:F68A-F68E ($1365 latches that once).
void pause_frame(ZoomZooState& next, bool opening);
// $83:F930-F93B: CONTINUE fades the music back in (0x7F) and sends it at $83:F937-F93B.
void pause_continue(ZoomZooState& next);
// $83:CD6E and $83:CD9F: the race loop's two dispatcher calls.
void dispatch(ZoomZooState& next, AudioDispatchSite site);
// $83:E59C-E785: a published countdown update's beep, then its dispatcher call.
// `countdown` is `$11C5` before this update's decrement.
void countdown(ZoomZooState& next, std::uint16_t countdown, bool stunt_event);
// $83:E7F2-E830: from the finish display's 180th update, fade the music every update.
void finish_fade(ZoomZooState& next);
// $81:828E-82B2: a counted crossing sets flag 20 at speed 256 or more, else clears it.
void checkpoint(ZoomZooState& next, std::uint16_t velocity_x);
// $82:999A-9A49: the skid flag 21 (player) or 22 (opponent) follows the brake latch.
void brake_skid(ZoomZooState& next, unsigned rider, bool skidding);
// $82:A507-A5F3: a rotation sets, clearing clears, flag 42 (player) or 43 (opponent); the
// audio side keeps the original's sound latches `$1003`/`$1005`.
void rotation(ZoomZooState& next, unsigned rider, bool rotating);
// $81:C191-C216 (player) and $81:C2D0-C357 (opponent): an announcement read from its queue
// plays its voice from `$81:C441`.
void announcement_voice(ZoomZooState& next, unsigned rider, std::uint8_t event,
                        std::span<const std::uint8_t> voices);
// The race's load before its first update, by frames left until the native initialization:
// the start's countdown cue ($82:D84A), which the upload's queue reset drops, then the race
// sound set's upload ($83:CA72). D-0010 calibration from R-0076's DRAGSTER load.
AudioCueList loading(std::uint32_t frames_to_initialization);
} // namespace race_sound
} // namespace unirally
