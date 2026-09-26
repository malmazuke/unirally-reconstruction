// The slides between the menus' texts (`$80:E233` forward, `$80:E27E` back) and the main
// menu's decoration animator (`$83:9A1E`) that the forward slide and PICK TOUR run (R-0055).
#include "front_end_screens.hpp"

#include <utility>

namespace unirally::front_end_screens {

namespace {

// The slide ($80:E233, $80:E27E): 39 passes; the speed rises by 1 while the countdown is 31 or
// more and falls by 1 below 7, so the passes add up to 256 pixels.
constexpr std::int8_t slide_passes = 38, accelerate_from = 31, decelerate_below = 7;

constexpr std::uint8_t pair_steps = 8, wave_steps = 20;

void step_slide_speed(ScreenSlide& slide) {
    if (slide.countdown >= accelerate_from)
        ++slide.speed;
    else if (slide.countdown < decelerate_below)
        --slide.speed;
}

} // namespace

bool step_down(std::uint8_t& counter, std::uint8_t last) {
    --counter;
    if ((counter & 0x80U) == 0) return false;
    counter = last;
    return true;
}

void step_decorations(FrontEndState& state, const FrontEndContent& content) {
    auto& decorations = state.decorations;
    const auto frames = content.decoration_frames;
    if (step_down(decorations.delay, 2)) {
        if (step_down(decorations.pair_step, pair_steps - 1))
            step_down(decorations.pair_cycle, pair_steps - 1);
        oam_byte(state, 98, 2) = oam_byte(state, 99, 2) =
            frames[cycle_tiles + decorations.pair_cycle];
        oam_byte(state, 96, 2) = oam_byte(state, 97, 2) =
            frames[pair_tiles + decorations.pair_step];
        step_down(decorations.trio_step, pair_steps - 1);
        for (unsigned k = 0; k < 3; ++k)
            oam_byte(state, 112 + k, 2) = frames[trio_tiles + decorations.trio_step + k];
    }
    if (!step_down(decorations.wave_delay, 1)) return;
    for (unsigned k = 8; k-- > 0;) {
        step_down(decorations.wave[k], wave_steps - 1);
        oam_byte(state, 104 + k, 2) = frames[wave_tiles + decorations.wave[k]];
    }
    step_down(decorations.sway, wave_steps - 1);
    const auto sway_left = frames[left_sway + decorations.sway],
               sway_right = frames[right_sway + decorations.sway];
    oam_byte(state, 100, 0) = static_cast<std::uint8_t>(0x08 + sway_left);
    oam_byte(state, 101, 0) = static_cast<std::uint8_t>(0x08 + sway_right);
    oam_byte(state, 102, 0) = static_cast<std::uint8_t>(0x18 + sway_left);
    oam_byte(state, 103, 0) = static_cast<std::uint8_t>(0x18 + sway_right);
}

void start_slide(FrontEndState& state, const FrontEndContent& content, bool back) {
    auto& slide = state.slide;
    slide.back = back;
    slide.countdown = slide_passes;
    slide.speed = 0;
    step_slide_speed(slide);
    if (!back) step_decorations(state, content); // `JMP ($0056)`: only the forward slide
}

bool slide_frame(FrontEndState& state, const FrontEndContent& content) {
    auto& slide = state.slide;
    copy_oam(state);
    slide.scroll = static_cast<std::uint16_t>(slide.back ? slide.scroll - slide.speed
                                                         : slide.scroll + slide.speed);
    state.registers.bg[1].hofs = slide.scroll;
    if (--slide.countdown >= 0) {
        step_slide_speed(slide);
        if (!slide.back) step_decorations(state, content);
        return false;
    }
    std::swap(slide.hidden_half, slide.shown_half);
    return true;
}

} // namespace unirally::front_end_screens
