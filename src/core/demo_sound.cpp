// The idle demo's sounds on its own screens (DEMO-AUDIO, R-0096). Its title loads the demo's
// sound set and slides the title in with the forward slide's sound; the race that follows plays
// with that set, and the way back to the main menu loads the title's set again.

#include "front_end_screens.hpp"

namespace unirally::front_end_screens {
namespace {

// $80:9440-9455: the demo's session starts on the title's script frame 57 (`demo-cold`'s frame 957,
// 57 frames after the main menu's last), after its picture's loads.
constexpr std::uint32_t demo_session_frame = 57;
// $80:A0F7: the title set's session five frames after the race's exit, as after a menus' race;
// a pad exit's extra blank frames (R-0070) follow it.
constexpr std::uint32_t return_session_frames = 5;
// The way back enables NMIs on its frame 101 (R-0088); the frame's work runs late and enters its
// frame wait (`$80:FADF`) there, a frame before the arrow's next turn.
constexpr std::uint32_t return_first_wait = 101;
// The way back's two frame waits during its session's upload (frames 74 and 75), a frame later
// after the timer's exits that hold its frame 100 (R-0087: the upload runs a frame longer).
constexpr std::uint32_t return_upload_wait = 74, held_return_upload_wait = 76;

// The demo title's upload ends with the reveal's first frame wait (`$80:E2D5`, then `$80:E322`).
// The title holds its last blank frame once when the upload ends a frame later (every title after
// the first, unless the sound processor ends it sooner: R-0087), and the wait moves with it.
bool title_will_hold(const FrontEndState& state) {
    return state.demo_cycles != 0 && !state.demo_title_held && !state.demo_title_short;
}

} // namespace

void demo_title_sounds(FrontEndState& state) {
    if (state.script_frame == demo_session_frame)
        state.sound_cues.push_back(audio_load(AudioSessionLoad::demo));
    // $80:9477 `JSR $E2D3`: the reveal starts with the forward slide's sound (`$80:B139`).
    if (state.script_frame == demo_title_hold_frame && !title_will_hold(state))
        play_menu_sound(state, MenuSound::forward_slide);
}

bool demo_frame_wait_dropped(const FrontEndState& state) {
    if (state.screen == FrontEndScreen::demo_title)
        return state.script_frame == demo_title_hold_frame && title_will_hold(state);
    return state.screen == FrontEndScreen::demo_return && state.demo_return_held
        && state.script_frame == return_upload_wait;
}

void held_demo_title_sounds(FrontEndState& state) {
    play_menu_sound(state, MenuSound::forward_slide);
    state.sound_cues.push_back(audio_dispatch(AudioDispatchSite::frame_wait));
}

bool demo_frame_wait_added(const FrontEndState& state) {
    if (state.screen != FrontEndScreen::demo_return) return false;
    return state.script_frame == return_first_wait
        || (state.demo_return_held && state.script_frame == held_return_upload_wait);
}

void demo_return_sounds(FrontEndState& state) {
    if (state.script_frame + state.demo_return_blank == return_session_frames)
        state.sound_cues.push_back(audio_load(AudioSessionLoad::title_return));
}

} // namespace unirally::front_end_screens
