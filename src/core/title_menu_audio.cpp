#include "title_menu_audio.hpp"
#include <stdexcept>

namespace unirally {
NativeTitleMenuAudio::NativeTitleMenuAudio(const TitleMenuAudioContent& content,
                                           AudioControllerSource& controllers,
                                           AudioEngineEventSink* events)
    : content_(&content), engine_(content.score, content.pitch, controllers, events) {
    cartridge_.fill(255);
}
// R-0075. Compose the frozen native cold work through F5C8, before its first hold frame.
void NativeTitleMenuAudio::initialize_title() {
    if (phase_ != TitleMenuAudioPhase::cold) throw std::logic_error("audio already initialized");
    auto& c = engine_.cpu();
    const auto& assets = content_->graphics;
    native_audio_cpu_boot_prefix(c);
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
    if (phase_ != TitleMenuAudioPhase::title_hold)
        throw std::logic_error("audio title is not waiting");
    const bool more = native_audio_title_hold_frame(engine_.cpu(), queue_, scene_, title_);
    if (!more) phase_ = TitleMenuAudioPhase::title_complete;
    return more;
}
// R-0075. Cold cartridge validation/defaults, native text/upload/fade and menu setup.
void NativeTitleMenuAudio::reveal_menu() {
    if (phase_ != TitleMenuAudioPhase::title_complete)
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
    if (!cleared)
        throw std::logic_error("warm cartridge continuation remains outside audio domain");
    native_audio_menu_first_record_defaults(c, cartridge_, content_->cartridge_defaults,
                                            content_->track_types);
    native_audio_menu_league_defaults(c, cartridge_);
    native_audio_finish_menu_records(c, scene_, cartridge_, content_->cartridge_defaults);
    native_audio_begin_menu_text(c, text_, content_->menu_text, content_->characters);
    native_audio_reveal_main_menu(c, queue_, scene_, text_, cartridge_);
    native_audio_begin_menu_input(c, scene_, cartridge_, menu_);
    phase_ = TitleMenuAudioPhase::menu;
}
AudioCpuMenuAction NativeTitleMenuAudio::menu_frame() {
    if (phase_ != TitleMenuAudioPhase::menu) throw std::logic_error("audio menu is not waiting");
    const auto action = native_audio_menu_input_frame(engine_.cpu(), queue_, scene_, cartridge_,
                                                      menu_, content_->arrow_positions);
    if (action != AudioCpuMenuAction::waiting) phase_ = TitleMenuAudioPhase::menu_exit;
    return action;
}
TitleMenuAudioState NativeTitleMenuAudio::snapshot() {
    return {phase_, engine_.snapshot(), queue_, scene_, title_, text_, menu_, cartridge_};
}
void NativeTitleMenuAudio::restore(const TitleMenuAudioState& state) {
    if (state.phase > TitleMenuAudioPhase::menu_exit || state.scene.phase > 31
        || state.queue.read_index > 15 || state.queue.write_index > 15
        || (state.queue.expected_phase != 64 && state.queue.expected_phase != 128)
        || state.title.code_index > 4 || state.menu.selection > 5
        || (state.phase == TitleMenuAudioPhase::title_hold && state.title.remaining > 110))
        throw std::invalid_argument("invalid native title/menu continuation");
    engine_.restore(state.engine);
    phase_ = state.phase;
    queue_ = state.queue;
    scene_ = state.scene;
    title_ = state.title;
    text_ = state.text;
    menu_ = state.menu;
    cartridge_ = state.cartridge;
}
} // namespace unirally
