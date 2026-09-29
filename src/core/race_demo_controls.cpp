// $83:E254-E55B: the idle demo drives both split-screen riders by the track's
// markers. The two paths are the same routine on adjacent rider words.
#include "race_demo_controls.hpp"

#include <cstdint>

namespace unirally {
namespace {
constexpr std::uint16_t ai_off = 0x8000, leftward = 0x4000, jump_marker = 0x2000;
constexpr std::uint16_t demo_duration = 0x076c, warning_at = 0x0714;
constexpr std::uint16_t turnaround_updates = 30, rotation_window = 50;

void rotate(ReflectionTransition& input, bool positive) {
    if (positive)
        input.rotate_positive_input = 1;
    else
        input.rotate_negative_input = 1;
}

void demo_rider(ZoomZooState& state, unsigned index, DemoTrickButtons& buttons) {
    auto& transition = state.reflection[index];
    auto& rider = state.movement.riders[index];
    auto& control = state.demo;
    auto& horizontal =
        index == 0 ? state.movement.player_input.horizontal : state.opponent_horizontal;
    transition.brake_input = transition.rotate_negative_input = transition.rotate_positive_input =
        transition.jump_input = 0;
    if (rider.progress.marker_word & ai_off) return;
    horizontal = (rider.progress.marker_word & leftward) ? direction::left : direction::right;
    if (control.turnaround[index]) {
        horizontal = static_cast<std::uint8_t>(2U - horizontal);
        --control.turnaround[index];
        return;
    }
    const auto slope = static_cast<std::int16_t>(rider.contact.surface_angle);
    const bool stalled = state.surface[index].mode
                      && ((slope < -26 && horizontal == direction::right)
                          || (slope >= 26 && horizontal == direction::left))
                      && static_cast<std::int16_t>(rider.motion.previous_x_displacement) < 3;
    if (stalled) {
        control.turnaround[index] = turnaround_updates;
        return;
    }
    if (rider.progress.marker_word & jump_marker) {
        transition.jump_input = 1;
        if (!control.airborne_rotation[index] && rider.contact.unsupported_count >= 4
            && static_cast<std::int16_t>(rider.motion.velocity_y) < 0) {
            control.airborne_rotation[index] =
                static_cast<std::uint16_t>(-static_cast<std::int16_t>(rider.motion.velocity_y) / 2);
            control.rotation_window[index] = rotation_window;
            if (rider.contact.surface_angle == 0)
                control.trick_bits[index] =
                    static_cast<std::int16_t>(rider.motion.velocity_x) < 0 ? 0 : 1;
            else
                control.trick_bits[index] = rider.motion.x & 7U;
        }
        if (control.airborne_rotation[index]) {
            const auto bits = control.trick_bits[index];
            rotate(transition, (bits & 1U) != 0);
            buttons.a[index] = (bits & 2U) != 0;
            buttons.x[index] = (bits & 4U) != 0;
            return;
        }
    }
    // $83:E3E0/$83:E524: without a marker jump, use $0300, the alternating
    // contact phase. Countdown may subsequently hold this input released.
    if (!(rider.progress.marker_word & jump_marker))
        transition.jump_input = state.movement.contact_phase;
    control.trick_bits[index] = control.airborne_rotation[index] = 0;
    if (static_cast<std::int16_t>(rider.motion.velocity_y) >= 0 && control.rotation_window[index]
        && rider.pose.reflected_orientation >= 16 && rider.pose.reflected_orientation < 48)
        rotate(transition, static_cast<std::int16_t>(rider.motion.velocity_x) >= 0);
}
} // namespace

DemoTrickButtons update_demo_controllers(ZoomZooState& state, bool pad_pressed) {
    auto& demo = state.demo;
    if (demo.elapsed + 1U == demo_duration || pad_pressed) {
        demo.exit_requested = true;
        state.movement.player_input.horizontal = direction::neutral;
        state.opponent_horizontal = direction::neutral;
        for (auto& input : state.reflection)
            input.brake_input = input.jump_input = input.rotate_negative_input =
                input.rotate_positive_input = 0;
        return {};
    }
    ++demo.elapsed;
    // $83:E267-E276 sends a fade/sound sequence after this point; its visual
    // state is handled by the demo transition, not the race controller.
    (void)warning_at;
    DemoTrickButtons buttons;
    for (unsigned rider = 0; rider < 2; ++rider) demo_rider(state, rider, buttons);
    return buttons;
}
} // namespace unirally
