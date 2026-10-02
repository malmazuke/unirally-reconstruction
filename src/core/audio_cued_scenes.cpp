#include "title_menu_audio.hpp"
#include <stdexcept>

namespace unirally {
namespace {
using Clock = AudioCpuWorkClock;
// A PAL frame is 312 lines of 1,364 master clocks. Frame n's work follows the
// vertical-blank boundary (line 225) that ends frame n-1, observed at
// 306,900 + (n - 1) * 425,568 master clocks from power-on (R-0076).
constexpr std::uint64_t frame_clocks = 425568, first_frame_boundary = 306900;
// D-0010 calibration constants, not recovered timing: master clocks after the
// boundary that starts the frame, the medians of the cold 1P DRAGSTER captures
// (R-0076): `$80:FADF` entry on setup screens, the race's `$83:CD6E`/`$83:CD9F`
// calls, the countdown's and finish fade's calls, NOW PLAYING's Race choice and
// each session's first FF request.
constexpr std::uint64_t frame_wait_anchor = 13082, race_early_anchor = 27920,
                        race_late_anchor = 207728, countdown_anchor = 31564,
                        finish_fade_anchor = 26324, race_choice_anchor = 9908, pause_anchor = 12232,
                        pause_fade_anchor = 17214, pause_continue_anchor = 36340;
constexpr std::uint64_t title_return_load_anchor = 344450;
// D-0010 calibration (R-0077): the first FF request of the award's, a gold ending's and the title
// set's sessions, master clocks after their frame's boundary, the medians of the cold captures'
// sessions (24 awards, 3 endings, 27 returns). Each follows a frame wait, early in its frame.
constexpr std::uint64_t award_load_anchor = 2342, ending_load_anchor = 2476,
                        award_return_load_anchor = 1278;
constexpr std::uint32_t longest_session_frames = 81;
// D-0010 calibration by track (R-0077): the race sound session's first FF request's master
// clocks after its frame's boundary, the medians of six races on each of the five tracks the
// menus reach (DRAGSTER's is R-0076's). The race's content load before the request differs by
// track and places the request from 70,000 to 306,000 clocks into its frame.
constexpr std::array<std::uint64_t, 5> race_load_anchors{169072, 124219, 69947, 94109, 306008};
std::uint64_t race_load_anchor(std::uint8_t track) {
    if (track >= race_load_anchors.size())
        throw std::invalid_argument("no measured race sound load for this track");
    return race_load_anchors[track];
}

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
    case AudioDispatchSite::pause: return pause_anchor;
    case AudioDispatchSite::pause_fade: return pause_fade_anchor;
    case AudioDispatchSite::pause_continue: return pause_continue_anchor;
    }
    throw std::invalid_argument("unknown audio dispatch site");
}
void enqueue_cue(Clock& c, AudioCpuQueueState& queue, std::uint8_t command,
                 std::uint8_t parameter) {
    c.load_constant(2);
    c.call_far();
    native_audio_enqueue(c, queue, command, parameter);
}
// $83:CA4F-CBEE after the race samples: effect and music gains, the music start,
// then four dispatcher calls before the race loop. Song counter 4's branch ($83:CB6A) sets
// the music gain to 255 where the other five set 127 (R-0077).
void start_race_music(Clock& c, AudioCpuQueueState& queue, std::uint8_t song_counter) {
    constexpr std::uint8_t usual_music_gain = 127, loud_music_gain = 255, loud_counter = 4;
    enqueue_cue(c, queue, 8, 255);
    enqueue_cue(c, queue, 7, song_counter == loud_counter ? loud_music_gain : usual_music_gain);
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
// A session's set and the clock of its first FF request after its frame's boundary.
struct SessionStart {
    AudioSoundSetId set;
    std::uint64_t anchor;
};
SessionStart session_start(const AudioCue& cue) {
    switch (cue.load) {
    case AudioSessionLoad::race:
        return {race_song_sound_set(race_song_resource(cue.parameter)),
                race_load_anchor(cue.command)};
    case AudioSessionLoad::title_return: return {AudioSoundSetId::title, title_return_load_anchor};
    case AudioSessionLoad::award: return {AudioSoundSetId::award, award_load_anchor};
    case AudioSessionLoad::ending: return {AudioSoundSetId::ending, ending_load_anchor};
    case AudioSessionLoad::award_return:
        return {AudioSoundSetId::title, award_return_load_anchor};
    }
    throw std::invalid_argument("unknown audio session");
}
// $83:A644-A66B (the award) and $83:A537-A55F (an ending): effect gain 255, music gain 79, the
// music start, then five dispatcher calls.
void start_screen_music(Clock& c, AudioCpuQueueState& queue) {
    enqueue_cue(c, queue, 8, 255);
    enqueue_cue(c, queue, 7, 79);
    enqueue_cue(c, queue, 1, 2);
    for (unsigned call = 0; call < 5; ++call) {
        c.call_far();
        native_audio_poll_queue(c, queue);
    }
}
// $83:A75E-A770 after the title set's samples: the music start, effect gain 255, music gain
// 127; the first dispatch is the next frame wait's.
void start_title_music_after_award(Clock& c, AudioCpuQueueState& queue) {
    enqueue_cue(c, queue, 1, 2);
    enqueue_cue(c, queue, 8, 255);
    enqueue_cue(c, queue, 7, 127);
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
        // A frame wait in a frame the CPU has already left (an upload ran past it) never runs.
        if (cue.site == AudioDispatchSite::frame_wait && c.ticks() >= frame_start(frame + 1))
            return;
        idle_until(c, frame_start(frame) + dispatch_anchor(cue.site));
        c.call_far();
        if (cue.site == AudioDispatchSite::frame_wait)
            native_audio_wait_vblank(c, queue_);
        else
            native_audio_poll_queue(c, queue_);
        return;
    case AudioCueKind::load: load_session(frame, cue); return;
    case AudioCueKind::rotation: rotation_sound(cue.command, cue.parameter != 0); return;
    }
    throw std::invalid_argument("unknown audio cue");
}
// Before a menu exit only the title set exists and no frame is cued; a cued scene has ended
// frame `cued_frame` at or after its vertical-blank boundary.
bool NativeTitleMenuAudio::valid_cued_state(const TitleMenuAudioState& state) {
    if (state.phase != TitleMenuAudioPhase::cued)
        return state.cued_frame == 0 && !state.rotation_sounding[0] && !state.rotation_sounding[1]
            && state.engine.sound_set == AudioSoundSetId::title
            && state.engine.uploading_sound_set == AudioSoundSetId::title;
    // Only an upload session runs the clock past a frame's end. The longest, the race load
    // with its samples, starts in frame 1249 and leaves the clock in frame 1329, 79.8 frames
    // past that frame's end (the title reload: 68.3), so a clock 81 frames ahead names a lagging
    // frame. A lag within one session's length is not detectable from the saved state.
    constexpr std::uint32_t last_frame = 0xfffffffeU - longest_session_frames;
    const auto ticks = state.engine.cpu.ticks;
    return state.cued_frame != 0 && state.cued_frame <= last_frame
        && ticks >= frame_start(state.cued_frame + 1)
        && ticks < frame_start(state.cued_frame + 1 + longest_session_frames);
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
void NativeTitleMenuAudio::load_session(std::uint32_t frame, const AudioCue& cue) {
    auto& c = engine_.cpu();
    const auto start = session_start(cue);
    idle_until(c, frame_start(frame) + start.anchor);
    engine_.begin_sound_set_upload(start.set);
    native_audio_cpu_begin_session(c);
    native_audio_cpu_uploads(c, content_->upload, start.set);
    native_audio_cpu_finish_driver_entry(c);
    queue_ = {};
    native_audio_cpu_upload_samples(c, content_->upload, start.set);
    rotation_sounding_ = {}; // the race load clears the race's work RAM
    switch (cue.load) {
    case AudioSessionLoad::race: start_race_music(c, queue_, cue.parameter); return;
    case AudioSessionLoad::title_return: native_audio_bootstrap_queue(c, queue_); return;
    case AudioSessionLoad::award:
    case AudioSessionLoad::ending: start_screen_music(c, queue_); return;
    case AudioSessionLoad::award_return: start_title_music_after_award(c, queue_); return;
    }
}
} // namespace unirally
