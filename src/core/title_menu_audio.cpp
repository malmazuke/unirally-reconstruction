#include "title_menu_audio.hpp"
#include <algorithm>
#include <stdexcept>

namespace unirally {
namespace {
bool valid_hunter_work(const AudioCpuHunterWorkState& state) {
    const auto& d = state.decorations;
    return d.pair_step <= 7 && d.pair_cycle <= 7 && d.trio_step <= 7 && d.wave_delay <= 1
        && d.sway <= 19
        && std::all_of(d.wave.begin(), d.wave.end(), [](auto value) { return value <= 19; })
        && (!state.first_press || state.wait_started)
        && (state.timed_remaining <= 1200 || state.timed_remaining == 65535)
        && (state.reveal_remaining <= 74 || state.reveal_remaining == 65535)
        && state.credits_frame < 96;
}
} // namespace
NativeTitleMenuAudio::NativeTitleMenuAudio(const TitleMenuAudioContent& content,
                                           AudioControllerSource& controllers,
                                           AudioEngineEventSink* events)
    : content_(&content), engine_(content.score, content.pitch, controllers, events) {
    cartridge_.fill(255);
    if (content.has_race_set()) engine_.set_race_sound_set(content.race_score);
}
// R-0075. Compose the frozen native cold work through F5C8, before its first hold frame.
void NativeTitleMenuAudio::initialize_title() {
    if (phase_ != TitleMenuAudioPhase::cold) throw std::logic_error("audio already initialized");
    auto& c = engine_.cpu();
    native_audio_cpu_boot_prefix(c);
    finish_title_initialization();
}
// $80:8858; R-0075. Re-enter boot work on the current raster and retained
// DSP/port/timer state. WRAM is cleared; cartridge records remain owned.
void NativeTitleMenuAudio::restart_title() {
    if (phase_ != TitleMenuAudioPhase::hunter_reset_ready)
        throw std::logic_error("audio HUNTER reset entry incomplete");
    native_audio_cpu_boot_prefix(engine_.cpu(), {}, true);
    queue_ = {};
    scene_ = {};
    scene_.title_levels_pending = cartridge_[0x10d0];
    scene_.menu_mode = cartridge_[0x10ad];
    title_ = {};
    text_ = {};
    menu_ = {};
    hunter_ = {};
    pending_action_ = AudioCpuMenuAction::waiting;
    engine_.interrupt() = {};
    cartridge_[0x0742] &= 0xfd;
    engine_.interrupt().cartridge_flags = cartridge_[0x0742];
    finish_title_initialization();
    phase_ = TitleMenuAudioPhase::warm_title_hold;
}
void NativeTitleMenuAudio::finish_title_initialization() {
    auto& c = engine_.cpu();
    const auto& assets = content_->graphics;
    native_audio_cpu_uploads(c, content_->upload);
    native_audio_cpu_finish_driver_entry(c);
    native_audio_cpu_upload_samples(c, content_->upload);
    native_audio_bootstrap_queue(c, queue_);
    native_audio_wait_vblank(c, queue_);
    native_audio_finish_waited_frame(c, scene_);
    native_audio_upload_base_palette(c, queue_, scene_);
    native_audio_load_nintendo_graphics(c, scene_, assets[31], assets[80], assets[74]);
    native_audio_finish_nintendo_screen(c, queue_, scene_);
    native_audio_load_title_graphics(c, scene_, engine_.interrupt(), assets[27], assets[78],
                                     assets[72]);
    native_audio_first_title_interrupt(c);
    native_audio_title_fade(c, queue_, scene_, false);
    native_audio_begin_title_hold(c);
    phase_ = TitleMenuAudioPhase::title_hold;
}
bool NativeTitleMenuAudio::title_frame() {
    const bool warm = phase_ == TitleMenuAudioPhase::warm_title_hold;
    if (!warm && phase_ != TitleMenuAudioPhase::title_hold)
        throw std::logic_error("audio title is not waiting");
    const bool more = native_audio_title_hold_frame(engine_.cpu(), queue_, scene_, title_);
    if (!more)
        phase_ =
            warm ? TitleMenuAudioPhase::warm_title_complete : TitleMenuAudioPhase::title_complete;
    return more;
}
// R-0075. Cold cartridge validation/defaults, native text/upload/fade and menu setup.
void NativeTitleMenuAudio::reveal_menu() {
    if (phase_ != TitleMenuAudioPhase::title_complete
        && phase_ != TitleMenuAudioPhase::warm_title_complete)
        throw std::logic_error("audio title incomplete");
    auto& c = engine_.cpu();
    c.call_local();
    native_audio_title_fade(c, queue_, scene_, true);
    c.return_local();
    native_audio_menu_first_palette(c, queue_, scene_, engine_.interrupt(), content_->graphics[1]);
    native_audio_menu_graphics(c, scene_, content_->graphics);
    native_audio_menu_oam(c, scene_);
    const bool cleared = native_audio_begin_menu_records(c, queue_, scene_, cartridge_,
                                                         content_->cartridge_signature);
    if (cleared) {
        native_audio_menu_first_record_defaults(c, cartridge_, content_->cartridge_defaults,
                                                content_->track_types);
        native_audio_menu_league_defaults(c, cartridge_);
        native_audio_finish_menu_records(c, scene_, cartridge_, content_->cartridge_defaults);
    } else {
        native_audio_reset_menu_selection(c, scene_, cartridge_);
    }
    native_audio_begin_menu_text(c, text_, content_->menu_text, content_->characters);
    native_audio_reveal_main_menu(c, queue_, scene_, text_, cartridge_);
    native_audio_begin_menu_input(c, scene_, cartridge_, menu_);
    phase_ = TitleMenuAudioPhase::menu;
}
AudioCpuMenuAction NativeTitleMenuAudio::menu_frame() {
    if (phase_ != TitleMenuAudioPhase::menu) throw std::logic_error("audio menu is not waiting");
    const auto action = native_audio_menu_input_frame(engine_.cpu(), queue_, scene_, cartridge_,
                                                      menu_, content_->arrow_positions);
    pending_action_ = action;
    if (action != AudioCpuMenuAction::waiting) phase_ = TitleMenuAudioPhase::menu_exit;
    return action;
}
bool NativeTitleMenuAudio::hunter_entry_frame() {
    if (phase_ == TitleMenuAudioPhase::menu_exit && pending_action_ == AudioCpuMenuAction::hunter) {
        native_audio_begin_hunter_code(engine_.cpu(), scene_, engine_.interrupt(), cartridge_);
        hunter_remaining_ = 30;
        phase_ = TitleMenuAudioPhase::hunter_entry;
    }
    if (phase_ != TitleMenuAudioPhase::hunter_entry)
        throw std::logic_error("audio HUNTER entry is not waiting");
    const bool more =
        native_audio_hunter_code_frame(engine_.cpu(), queue_, scene_, hunter_remaining_);
    if (!more) phase_ = TitleMenuAudioPhase::hunter_ready;
    return more;
}
void NativeTitleMenuAudio::hunter_first_fade() {
    if (phase_ != TitleMenuAudioPhase::hunter_ready)
        throw std::logic_error("audio HUNTER transition incomplete");
    native_audio_hunter_first_fade(engine_.cpu(), queue_);
    phase_ = TitleMenuAudioPhase::hunter_faded;
}
void NativeTitleMenuAudio::hunter_reveal_first_page() {
    hunter_begin_first_page();
    while (hunter_reveal_frame()) {}
}
void NativeTitleMenuAudio::hunter_begin_first_page() {
    if (phase_ != TitleMenuAudioPhase::hunter_faded)
        throw std::logic_error("audio HUNTER first fade incomplete");
    native_audio_hunter_begin_first_page(engine_.cpu(), queue_, scene_, content_->graphics,
                                         content_->reveal_offsets, content_->reveal_brightness);
    hunter_.reveal_remaining = 74;
    phase_ = TitleMenuAudioPhase::hunter_first_reveal;
}
bool NativeTitleMenuAudio::hunter_reveal_frame() {
    const bool second = phase_ == TitleMenuAudioPhase::hunter_second_reveal;
    if (!second && phase_ != TitleMenuAudioPhase::hunter_first_reveal)
        throw std::logic_error("audio HUNTER reveal is not active");
    const bool more =
        native_audio_hunter_reveal_frame(engine_.cpu(), queue_, scene_, hunter_.reveal_remaining);
    if (!more)
        phase_ = second ? TitleMenuAudioPhase::hunter_timed_wait
                        : TitleMenuAudioPhase::hunter_first_page;
    return more;
}
bool NativeTitleMenuAudio::hunter_page_wait_frame() {
    if (phase_ != TitleMenuAudioPhase::hunter_first_page)
        throw std::logic_error("audio HUNTER first page is not waiting");
    if (!hunter_.wait_started)
        hunter_.decorations.delay = static_cast<std::uint8_t>(menu_.idle_remaining);
    const bool more = native_audio_hunter_page_wait_frame(engine_.cpu(), queue_, scene_, hunter_);
    if (!more) phase_ = TitleMenuAudioPhase::hunter_page_pressed;
    return more;
}
void NativeTitleMenuAudio::hunter_reveal_second_page() {
    hunter_begin_second_page();
    while (hunter_reveal_frame()) {}
}
void NativeTitleMenuAudio::hunter_begin_second_page() {
    if (phase_ != TitleMenuAudioPhase::hunter_page_pressed)
        throw std::logic_error("audio HUNTER first page press incomplete");
    native_audio_hunter_begin_second_page(engine_.cpu(), queue_, scene_, content_->graphics,
                                          content_->reveal_offsets, content_->reveal_brightness);
    hunter_.reveal_remaining = 74;
    phase_ = TitleMenuAudioPhase::hunter_second_reveal;
}
bool NativeTitleMenuAudio::hunter_timed_wait_frame() {
    if (phase_ != TitleMenuAudioPhase::hunter_timed_wait)
        throw std::logic_error("audio HUNTER second page is not waiting");
    const bool more = native_audio_hunter_timed_frame(engine_.cpu(), queue_, hunter_, cartridge_);
    if (!more) phase_ = TitleMenuAudioPhase::hunter_credits_ready;
    return more;
}
std::array<std::uint64_t, 4> NativeTitleMenuAudio::hunter_prepare_credits() {
    if (phase_ != TitleMenuAudioPhase::hunter_credits_ready)
        throw std::logic_error("audio HUNTER timed wait incomplete");
    const auto marks = native_audio_hunter_prepare_credits(
        engine_.cpu(), queue_, engine_.interrupt(), content_->graphics, content_->credits_text,
        content_->characters);
    phase_ = TitleMenuAudioPhase::hunter_credits_prepared;
    return marks;
}
void NativeTitleMenuAudio::hunter_build_first_pose() {
    if (phase_ != TitleMenuAudioPhase::hunter_credits_prepared)
        throw std::logic_error("audio HUNTER credits setup incomplete");
    auto& c = engine_.cpu();
    c.load_constant(2);
    c.store_direct(2);
    c.load_constant(2);
    c.store_direct(2);
    c.call_far();
    native_audio_build_pose_work(c, 0x1350, content_->pose_pointers, content_->pose_frames);
    phase_ = TitleMenuAudioPhase::hunter_first_pose;
}
void NativeTitleMenuAudio::hunter_build_second_pose() {
    if (phase_ != TitleMenuAudioPhase::hunter_first_pose)
        throw std::logic_error("audio HUNTER first pose incomplete");
    auto& c = engine_.cpu();
    c.load_constant(2);
    c.store_direct(2);
    c.call_far();
    native_audio_upload_pose_work(c);
    c.load_constant(2);
    c.store_direct(2);
    c.load_constant(2);
    c.call_far();
    native_audio_build_pose_work(c, 0x1369, content_->pose_pointers, content_->pose_frames);
    phase_ = TitleMenuAudioPhase::hunter_second_pose;
}
void NativeTitleMenuAudio::hunter_finish_credits_setup() {
    if (phase_ != TitleMenuAudioPhase::hunter_second_pose)
        throw std::logic_error("audio HUNTER second pose incomplete");
    native_audio_hunter_finish_credits_setup(engine_.cpu(), queue_);
    hunter_.credits_frame = 0;
    phase_ = TitleMenuAudioPhase::hunter_credits_loop;
}
bool NativeTitleMenuAudio::hunter_credits_frame() {
    if (phase_ != TitleMenuAudioPhase::hunter_credits_loop)
        throw std::logic_error("audio HUNTER credits are not waiting");
    const bool more = native_audio_hunter_credits_frame(
        engine_.cpu(), queue_, hunter_, cartridge_, content_->credits_poses,
        content_->pose_pointers, content_->pose_frames);
    if (!more) phase_ = TitleMenuAudioPhase::hunter_leaving;
    return more;
}
void NativeTitleMenuAudio::hunter_finish_credits() {
    if (phase_ != TitleMenuAudioPhase::hunter_leaving)
        throw std::logic_error("audio HUNTER credits exit incomplete");
    engine_.cpu().call_local();
    native_audio_hunter_fade_down(engine_.cpu(), queue_);
    engine_.cpu().jump_far();
    phase_ = TitleMenuAudioPhase::hunter_reset_ready;
}
TitleMenuAudioState NativeTitleMenuAudio::snapshot() {
    return {phase_,
            pending_action_,
            content_->identity,
            engine_.snapshot(),
            queue_,
            scene_,
            title_,
            text_,
            menu_,
            cartridge_,
            hunter_remaining_,
            hunter_,
            cued_frame_};
}
void NativeTitleMenuAudio::restore(const TitleMenuAudioState& state) {
    if (state.content_identity != content_->identity)
        throw std::invalid_argument("audio state content identity differs");
    if (state.phase > TitleMenuAudioPhase::cued || state.pending_action > AudioCpuMenuAction::hunter
        || (state.phase == TitleMenuAudioPhase::hunter_entry && state.hunter_remaining > 30)
        || ((state.phase == TitleMenuAudioPhase::hunter_first_reveal
             || state.phase == TitleMenuAudioPhase::hunter_second_reveal)
            && state.hunter.reveal_remaining > 74)
        || !valid_hunter_work(state.hunter) || state.scene.phase > 31 || state.queue.read_index > 15
        || state.queue.write_index > 15
        || (state.queue.expected_phase != 64 && state.queue.expected_phase != 128)
        || state.title.code_index > 4 || state.menu.selection > 5
        || ((state.phase == TitleMenuAudioPhase::title_hold
             || state.phase == TitleMenuAudioPhase::warm_title_hold)
            && state.title.remaining > 110))
        throw std::invalid_argument("invalid native title/menu continuation");
    engine_.restore(state.engine);
    phase_ = state.phase;
    pending_action_ = state.pending_action;
    hunter_remaining_ = state.hunter_remaining;
    hunter_ = state.hunter;
    cued_frame_ = state.cued_frame;
    queue_ = state.queue;
    scene_ = state.scene;
    title_ = state.title;
    text_ = state.text;
    menu_ = state.menu;
    cartridge_ = state.cartridge;
}
} // namespace unirally
