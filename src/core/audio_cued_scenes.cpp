#include "title_menu_audio.hpp"
#include <stdexcept>

namespace unirally {
namespace {
using Clock = AudioCpuWorkClock;
// A PAL frame is 312 lines of 1,364 master clocks. Frame n's work follows the
// vertical-blank boundary (line 225) that ends frame n-1, observed at
// 306,900 + n * 425,568 master clocks from power-on (R-0076).
constexpr std::uint64_t frame_clocks = 425568, first_frame_boundary = 306900;
// D-0010 calibration constants, not recovered timing: master clocks after the
// boundary that starts the frame, the medians of the cold 1P DRAGSTER captures
// (R-0076): `$80:FADF` entry on setup screens, the race's `$83:CD6E`/`$83:CD9F`
// calls, the countdown's and finish fade's calls, NOW PLAYING's Race choice and
// each session's first FF request.
constexpr std::uint64_t frame_wait_anchor = 13082, race_early_anchor = 27920,
                        race_late_anchor = 207728, countdown_anchor = 31564,
                        finish_fade_anchor = 26324, race_choice_anchor = 9908;
constexpr std::uint64_t first_race_load_anchor = 169072, title_return_load_anchor = 344450;

std::uint64_t frame_start(std::uint32_t frame) {
    return first_frame_boundary + (std::uint64_t(frame) - 1) * frame_clocks;
}
// The game is busy elsewhere until an anchor; a past anchor runs at once.
void idle_until(Clock& c, std::uint64_t ticks) {
    while (c.ticks() < ticks) c.idle();
}
std::uint64_t dispatch_anchor(AudioDispatchSite site) {
    switch (site) {
    case AudioDispatchSite::frame_wait: return frame_wait_anchor;
    case AudioDispatchSite::race_early: return race_early_anchor;
    case AudioDispatchSite::race_late: return race_late_anchor;
    case AudioDispatchSite::countdown: return countdown_anchor;
    case AudioDispatchSite::finish_fade: return finish_fade_anchor;
    case AudioDispatchSite::race_choice: return race_choice_anchor;
    }
    throw std::invalid_argument("unknown audio dispatch site");
}
void enqueue_cue(Clock& c, AudioCpuQueueState& queue, std::uint8_t command,
                 std::uint8_t parameter) {
    c.load_constant(2);
    c.call_far();
    native_audio_enqueue(c, queue, command, parameter);
}
// $83:CA94-CBEE after the race samples: effect and music gains, the music start,
// then four dispatcher calls before the race loop.
void start_race_music(Clock& c, AudioCpuQueueState& queue) {
    enqueue_cue(c, queue, 8, 255);
    enqueue_cue(c, queue, 7, 127);
    c.load_constant(2);
    c.store_ram(2);
    c.store_ram(2);
    c.load_constant(2);
    c.store_ram(2);
    c.store_ram(2);
    c.jump_far();
    c.change_widths();
    c.read_ram(2, true);
    c.load_constant(2);
    c.load_constant(2);
    c.branch(false);
    c.branch(true);
    enqueue_cue(c, queue, 1, 2);
    for (unsigned call = 0; call < 4; ++call) {
        c.call_far();
        native_audio_poll_queue(c, queue);
    }
}
} // namespace

// D-0010, AUDIO-FIRST-RACE: after a menu exit the native game reports each frame's
// queue operations in program order; dispatches poll at frame-anchored clocks.
void NativeTitleMenuAudio::cue_frame(std::uint32_t frame, std::span<const AudioCue> cues) {
    if (phase_ == TitleMenuAudioPhase::menu_exit) {
        engine_.cpu().set_nmi_enabled(false); // frame work is reported, not modelled
        phase_ = TitleMenuAudioPhase::cued;
    } else if (phase_ != TitleMenuAudioPhase::cued || frame <= cued_frame_)
        throw std::logic_error("audio cue frame out of order");
    for (const auto& cue : cues) run_cue(frame, cue);
    idle_until(engine_.cpu(), frame_start(frame + 1));
    cued_frame_ = frame;
}
void NativeTitleMenuAudio::run_cue(std::uint32_t frame, const AudioCue& cue) {
    auto& c = engine_.cpu();
    switch (cue.kind) {
    case AudioCueKind::enqueue: enqueue_cue(c, queue_, cue.command, cue.parameter); return;
    case AudioCueKind::dispatch:
        idle_until(c, frame_start(frame) + dispatch_anchor(cue.site));
        c.call_far();
        if (cue.site == AudioDispatchSite::frame_wait)
            native_audio_wait_vblank(c, queue_);
        else
            native_audio_poll_queue(c, queue_);
        return;
    case AudioCueKind::load: load_session(frame, cue.load); return;
    case AudioCueKind::rotation: rotation_sound(cue.command, cue.parameter != 0); return;
    }
    throw std::invalid_argument("unknown audio cue");
}
// $82:A507-A5F3: the race keeps a sound latch per rider (`$1003`, `$1005`) and changes the
// rotation flag (42 player, 43 opponent) and its effect only when the rotation changes.
void NativeTitleMenuAudio::rotation_sound(unsigned rider, bool rotating) {
    constexpr std::uint8_t clear_flag = 6, set_flag = 11, start_effect = 2, first_flag = 42,
                           rotation_effect = 12;
    if (rider > 1) throw std::invalid_argument("rotation cue names no rider");
    auto& latch = rotation_sounding_[rider];
    if (latch == rotating) return;
    latch = rotating;
    auto& c = engine_.cpu();
    enqueue_cue(c, queue_, rotating ? set_flag : clear_flag,
                static_cast<std::uint8_t>(first_flag + rider));
    enqueue_cue(c, queue_, start_effect, rotation_effect);
}
// $82:807E from a running driver: its FF request stops the driver, the IPL then
// takes the session's driver, tables, score and samples (R-0075/R-0076). The
// driver-ready entry reinitializes the command ring, dropping queued cues.
void NativeTitleMenuAudio::load_session(std::uint32_t frame, AudioSessionLoad load) {
    auto& c = engine_.cpu();
    const bool race = load == AudioSessionLoad::first_race;
    const auto set = race ? AudioSoundSetId::first_race : AudioSoundSetId::title;
    idle_until(c, frame_start(frame) + (race ? first_race_load_anchor : title_return_load_anchor));
    engine_.begin_sound_set_upload(set);
    native_audio_cpu_begin_session(c);
    native_audio_cpu_uploads(c, content_->upload, set);
    native_audio_cpu_finish_driver_entry(c);
    queue_ = {};
    native_audio_cpu_upload_samples(c, content_->upload, set);
    rotation_sounding_ = {}; // the race load clears the race's work RAM
    if (race)
        start_race_music(c, queue_);
    else
        native_audio_bootstrap_queue(c, queue_);
}
} // namespace unirally
