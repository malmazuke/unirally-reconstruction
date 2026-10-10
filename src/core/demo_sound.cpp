// The idle demo's sounds on its own screens (DEMO-AUDIO, R-0096). Its title loads the demo's
// sound set and slides the title in with the forward slide's sound; the race that follows plays
// with that set, and the way back to the main menu loads the title's set again.

#include "front_end_screens.hpp"

namespace unirally::front_end_screens {
namespace {

// $80:9440-9455: the demo's session starts on the title's frame 58, after its picture's loads.
constexpr std::uint32_t demo_session_frame = 57;
// $80:A0F7 after the race's timer exit: the title set's session, as after a menus' race.
constexpr std::uint32_t return_session_frame = 5;
// The timer's way back enables NMIs on its frame 101 with one pending (R-0088); the frame's work
// runs late and enters its frame wait (`$80:FADF`) there, a frame before the arrow's next turn.
constexpr std::uint32_t timer_return_first_wait = 101;

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

bool demo_title_wait_moves(const FrontEndState& state) {
    return state.screen == FrontEndScreen::demo_title && state.script_frame == demo_title_hold_frame
        && title_will_hold(state);
}

void held_demo_title_sounds(FrontEndState& state) {
    play_menu_sound(state, MenuSound::forward_slide);
    state.sound_cues.push_back(audio_dispatch(AudioDispatchSite::frame_wait));
}

bool demo_return_wait_added(const FrontEndState& state) {
    return state.screen == FrontEndScreen::demo_return && !state.demo_return_interrupted
        && state.script_frame == timer_return_first_wait;
}

void demo_return_sounds(FrontEndState& state) {
    if (state.script_frame == return_session_frame)
        state.sound_cues.push_back(audio_load(AudioSessionLoad::title_return));
}

} // namespace unirally::front_end_screens
