// One race update: the order in which every system runs.
//
// update_zoom_zoo (named for ZOOM ZOO, where the engine was first recovered; it runs every
// race track) advances the race by one update in the original's order:
//
//   1. the result screen, once the finish display is over;
//   2. the update's clocks and the controllers (the HUNTER effects may skip the rest);
//   3. the pause menu, which can take the whole update;
//   4. the opponent's AI and the start countdown;
//   5. the finish, then each rider: checkpoints, the tile under it, its tricks and landing,
//      its controls, drive, pose and physics;
//   6. the race clock, the announcement queues, the camera;
//   7. each rider's contact with the track, the visibility, the HUNTER effects, the hints.

#include "announcements.hpp"
#include "hunter_effects.hpp"
#include "opponent_ai.hpp"
#include "race_camera.hpp"
#include "race_progress.hpp"
#include "reward_queue.hpp"
#include "rider_motion.hpp"
#include "rider_pose.hpp"
#include "trick_roll.hpp"
#include "vertical_contact.hpp"
#include "word_arithmetic.hpp"
#include "zoom_zoo_movement.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string>

namespace unirally {
namespace {

// The tile flag pairs the dispatch at $81:82BB runs (R-0047, R-0051), named by what they
// do where no record names them. 18 and 22 do nothing; 20, the checkpoint tile, runs with
// the checkpoints; 4 is unrecovered and nothing lies past 28.
namespace tile_pair {
inline constexpr unsigned boost = 2, unrecovered = 4, surface_drive = 6, slow = 8;
inline constexpr unsigned corkscrew = 10, slow_drive = 12, mud = 14, jump_driven = 16;
inline constexpr unsigned lift = 24, loop = 26, push = 28, past_table = 30;
} // namespace tile_pair

// The fade reaches 30; the pad is published once it passes 4 ($80:8642-865B).
constexpr std::uint16_t brightest_fade = 30, first_published_fade = 5;
// A capture-restored state runs within the trial horizon.
constexpr std::uint32_t trial_first_frame = 1649, trial_last_frame = 1849,
                        sustained_last_frame = 9999;
// The countdown: 270 updates natively (68 in a capture-restored state); the start boosts
// are dropped for a rider not braking between 129 and 101, spent below 70; the brakes are
// held from 100.
constexpr unsigned longest_native_countdown = 271, longest_restored_countdown = 69;
constexpr std::uint16_t boost_drop_first = 130, boost_drop_last = 100, boost_spend = 70;
constexpr std::uint16_t brakes_from = 100, clock_running_below = 68;
constexpr std::uint16_t finish_display_updates = 240;
// The riders are fully airborne after 9 updates without support.
constexpr std::uint16_t airborne_updates = 9, off_ground_updates = 2;
constexpr std::uint8_t high_tile = 0x80;
constexpr std::uint16_t mirrored_tile = 0x4000; // the descriptor's facing flag
// The reflection transition's poses start at 0x620; a turn runs steps 1-9, or 9-16 when it
// starts facing left.
constexpr std::uint16_t reflection_poses = 0x620;
constexpr std::uint16_t turn_first = 1, turn_middle = 9, turn_last = 16;
// A completed turn holds its drive pose for 30 updates.
constexpr std::uint16_t turn_hold = 30;
// Rotation: 1 a update with A held, else 2 (254 and 255 the other way); cleared on a
// shallow slope (under 30).
constexpr std::int16_t shallow_slope = 30;
// The wrong-direction warning after 180 active updates, then every 60 ($82:974B/977B).
constexpr std::uint16_t wrong_way_warning = 180, wrong_way_repeat = 120;

// The request the engine sees: nothing before the fade publishes the pad.
ControllerButtons gate_controller(const ZoomZooState& state, const ControllerButtons& request) {
    return state.native_initialization && state.fade_level < first_published_fade
             ? ControllerButtons{}
             : request;
}

// The original's graph load begins on the update after the 240-update finish display: 108
// black updates, then seven brightness steps. The final race stays as the archive.
void advance_result_screen(ZoomZooState& state, const ClassicRaceScenario& scenario) {
    if (state.result_updates < stable_result_updates(state)) ++state.result_updates;
    state.result = zoom_result_fields(state.race, state.result_updates, scenario.tour_race);
    ++state.movement.frame;
}

// Capture-restored states admit only the controls and frames their captures covered.
void check_update_domain(const ZoomZooState& state, const ControllerButtons& request,
                         const ClassicRaceScenario& scenario) {
    const bool restored = !state.native_initialization;
    if ((restored
         && (request.y || request.select || request.start || request.up || request.down || request.a
             || request.x || request.left_shoulder || request.right_shoulder))
        || (!state.complete_race && request.left))
        throw std::invalid_argument("ZOOM ZOO controller is outside the recovered domain");
    if (state.movement.frame < (restored ? trial_first_frame : scenario.initialization_frame)
        || (restored
            && state.movement.frame >= (state.sustained ? sustained_last_frame : trial_last_frame)))
        throw std::invalid_argument("ZOOM ZOO update is outside the declared trial horizon");
}

// $83:CC9A-CCA2: an update the HUNTER effects skip ($128B, R-0052) runs only the effects and
// the hints ($83:CDAA); the race, its clocks, the controller reader ($82:AA71, the axes and
// button words) and the pause menu wait. The NMI still publishes the pad images $0311-$0314
// ($80:87E9-87FE).
void run_skipped_update(ZoomZooState& next, const ControllerButtons& buttons,
                        const ZoomZooContent& content) {
    const auto pad = sample_controller(buttons);
    next.movement.player_input.low_image = pad.low_image;
    next.movement.player_input.high_image = pad.high_image;
    update_hunter_effects(next, content.hunter_blink);
    if (next.native_initialization) update_tutorial_hints(next);
    ++next.movement.frame;
}

void advance_clocks(ZoomZooState& next, const ControllerButtons& buttons) {
    auto& whole = next.movement;
    whole.player_input = sample_controller(buttons);
    whole.contact_phase = static_cast<std::uint8_t>(1U - whole.contact_phase);
    whole.progress_phase = static_cast<std::uint8_t>(1U - whole.progress_phase);
    whole.animation_counter = static_cast<std::uint8_t>((whole.animation_counter + 1U) & 31U);
    whole.update_counter = static_cast<std::uint8_t>(whole.update_counter + 1U);
    if (whole.countdown
        >= (next.native_initialization ? longest_native_countdown : longest_restored_countdown))
        throw std::invalid_argument("ZOOM ZOO countdown is outside continuation domain");
    if (!next.native_initialization && whole.countdown) --whole.countdown;
    if (next.native_initialization && next.fade_level < brightest_fade) ++next.fade_level;
}

// $82:AAE2-AAFB: B drives the jump ($0331), Y the brake ($0325), the shoulders the
// rotations. Returns whether A is pressed. $82:AC5A-ACA7: the HUNTER's control reversal
// swaps left and right, the two rotations, and Y with A; it swaps the opponent's port-2
// words too, which the AI then overwrites.
bool read_player_buttons(ZoomZooState& next, const ControllerButtons& buttons, bool reversed) {
    auto& input = next.reflection[0];
    input.brake_input = buttons.y;
    input.jump_input = buttons.b;
    input.rotate_negative_input = buttons.left_shoulder;
    input.rotate_positive_input = buttons.right_shoulder;
    bool pressed_a = buttons.a;
    if (reversed) {
        auto& horizontal = next.movement.player_input.horizontal;
        horizontal = static_cast<std::uint8_t>(2U - horizontal);
        std::swap(input.rotate_negative_input, input.rotate_positive_input);
        const bool brake = input.brake_input != 0;
        input.brake_input = pressed_a;
        pressed_a = brake;
    }
    return pressed_a;
}

// $83:CD05-CD35: the pause menu takes the update once the controller and phase clocks are
// sampled; the race, the AI, the queues and the hints wait. Start opens it (unless the
// player has finished); up and down choose; releasing and pressing Start again resumes, or
// restarts the race from RESTART RACE, this menu's authored choice (the original's Retire
// and tour progression are not emulated). Returns true when it took the update.
bool run_pause_menu(const ZoomZooState& state, ZoomZooState& next, const ControllerButtons& buttons,
                    const ZoomZooContent& content) {
    if (!state.native_initialization
        || !(state.pause.selection || (buttons.start && !state.race.riders[0].finished)))
        return false;
    auto& pause = next.pause;
    auto& whole = next.movement;
    if (!pause.selection) pause.selection = 1;
    if (whole.player_input.vertical != direction::neutral)
        pause.selection = whole.player_input.vertical ? 0xffff : 1;
    if (!buttons.start) {
        pause.released = 1;
    } else if (pause.released) {
        if (negative(pause.selection)) {
            restart_zoom_zoo(next, content);
            return true;
        }
        pause.selection = 0;
    }
    ++pause.suspended_updates;
    if (next.fade_level >= first_published_fade && whole.countdown)
        ++pause.suspended_countdown_updates;
    next.opponent_horizontal = direction::neutral;
    auto& opponent = next.reflection[1];
    opponent.brake_input = opponent.jump_input = opponent.rotate_negative_input =
        opponent.rotate_positive_input = 0;
    ++whole.frame;
    return true;
}

// $83:E59C-E7BD: the start countdown. A rider not braking while it passes 129-101 loses its
// start boost, and one not braking below 70 spends it. Until 70 (and before the fade
// publishes the pad) both riders brake and neither jumps, and $83:E7A2-E7BF releases
// their A and X; returns true while it does.
bool run_countdown(const ZoomZooState& state, ZoomZooState& next) {
    auto& whole = next.movement;
    if (!state.native_initialization || !whole.countdown) return false;
    const bool published = next.fade_level >= first_published_fade;
    if (published) {
        if (whole.countdown < boost_drop_first && whole.countdown > boost_drop_last)
            for (unsigned i = 0; i < 2; ++i)
                if (!next.reflection[i].brake_input) next.start_boost[i] = 0;
        if (whole.countdown < boost_spend)
            for (unsigned i = 0; i < 2; ++i)
                if (!next.reflection[i].brake_input) {
                    whole.riders[i].speed.boost =
                        add_word(whole.riders[i].speed.boost, next.start_boost[i]);
                    next.start_boost[i] = 0;
                }
        --whole.countdown;
    }
    if (state.movement.countdown < boost_spend && published) return false;
    if (published && state.movement.countdown <= brakes_from)
        for (auto& input : next.reflection) input.brake_input = 1;
    if (!next.reflection[0].brake_input) whole.player_input.horizontal = direction::neutral;
    if (!next.reflection[1].brake_input) next.opponent_horizontal = direction::neutral;
    for (auto& input : next.reflection) {
        input.brake_input = 1;
        input.jump_input = 0;
    }
    return true;
}

// This update's trick buttons: the player's A and X, and the opponent's A and X, which are
// its trick selector's bits 1 and 2 (the selector's bit 0 chose the rotation).
struct TrickButtons {
    bool player_a{}, player_x{};
    unsigned opponent_trick{};
    bool opponent_a() const { return (opponent_trick & 2U) != 0; }
    bool opponent_x() const { return (opponent_trick & 4U) != 0; }
};

// What one rider's update leaves for the rest of the update.
struct RiderOutcome {
    std::uint16_t contact_skip{};
    unsigned reward{}; // the opponent's published event (capture-restored races)
};

void update_reflection_transition(RiderMovementState& rider, ReflectionTransition& transition,
                                  unsigned horizontal, bool inactive_phase,
                                  std::span<const std::uint8_t> table, bool manual = false,
                                  const SpecialTileRider& tiles = {}) {
    // $82:A35B-A49E: A permits a direction-selected turn while airborne, including a turn
    // toward the current facing. Contact A halves velocity. $82:A35D-A362: the corkscrew's
    // reflection lock skips it all.
    if (tiles.reflection_lock) return;
    const bool manual_airborne = rider.contact.unsupported_count == airborne_updates
                              && !transition.step && !(rider.contact.selected_high & high_tile)
                              && manual && horizontal != direction::neutral;
    if (!(rider.contact.unsupported_count == airborne_updates && transition.step)
        && !manual_airborne && !rider.contact.recontact && !inactive_phase)
        return;
    if (!transition.step && !manual_airborne
        && (horizontal == direction::neutral
            || (horizontal == direction::right && rider.pose.reflected)
            || (horizontal == direction::left && !rider.pose.reflected)))
        return;
    if (manual && rider.contact.recontact) {
        const auto velocity = rider.motion.velocity_x;
        rider.motion.velocity_x =
            static_cast<std::uint16_t>((velocity >> 1U) | (velocity & 0x8000U));
        transition.air_turns = 0;
    }
    if (!transition.step) {
        // $82:A403-A410: a pose override or the corkscrew latch holds the facing.
        if (transition.pose_override || tiles.corkscrew_latch) return;
        if (rider.pose.reflected) {
            rider.pose.reflected_orientation =
                static_cast<std::uint16_t>(64 - rider.pose.reflected_orientation) & 63U;
            rider.pose.reflected = false;
            transition.step = turn_middle;
            transition.end = turn_last;
        } else {
            transition.step = turn_first;
            transition.end = turn_middle;
        }
    }
    transition.pose_base =
        static_cast<std::uint16_t>(content_word(table, 2U * rider.pose.reflected_orientation));
    if (transition.step == transition.end) {
        if (rider.contact.unsupported_count >= 8)
            transition.air_turns = add_word(transition.air_turns, 1);
        transition.completed = 1;
        if (transition.end != turn_last) rider.pose.reflected = true;
        transition.step = 0;
        transition.pose_override = 0;
    } else {
        transition.pose_override =
            static_cast<std::uint16_t>(transition.pose_base + transition.step + reflection_poses);
        transition.step = add_word(transition.step, 1);
    }
}

void integrate_zoom_axis(std::uint16_t& position, std::uint16_t velocity, std::uint16_t& residue) {
    const auto total = static_cast<std::int16_t>(add_word(velocity, residue));
    position = static_cast<std::uint16_t>(static_cast<int>(position) + total / 32);
    residue = static_cast<std::uint16_t>(total % 32);
}

// $81:858E-871B: the per-update reset before the tile dispatch. $81:859D-85A8: the
// reflection lock counts down while the previous update left surface mode clear;
// $81:8592-8599: flag pair 8's counter falls by one, not below zero.
void begin_rider_update(ZoomZooState& next, unsigned index, const ZoomZooContent& content) {
    auto& rider = next.movement.riders[index];
    auto& transition = next.reflection[index];
    auto& surface = next.surface[index];
    auto& tiles = next.special_tiles[index];
    if (next.complete_race) update_zoom_checkpoint(next, index, content);
    surface.tile_mode = 0;
    surface.animation_delta = 0;
    surface.tile_pose = 0;
    surface.tile_pose_enabled = 0;
    if (tiles.slow_counter) --tiles.slow_counter;
    if (!surface.mode && tiles.reflection_lock) --tiles.reflection_lock;
    update_loop_cooldown(rider, tiles, transition);
    decay_idle_wobble(rider, surface.mode != 0);
    surface.mode = 0;
    rider.launch_override = 0;
    update_special_tile_counters(tiles, transition, rider.contact.selected_high);
}

// $81:82BB-82F3: the selected tile's flag pair, through the table at $81:82F5 unless the
// auxiliary flag is set.
unsigned tile_behavior_under(const RiderMovementState& rider, const ZoomZooContent& content) {
    const auto descriptor = rider.contact.selected_word;
    const auto tile = ((descriptor & 0x3f0U) >> 2U) + ((descriptor & 15U) >> 1U);
    if (tile >= content.movement.flat_contact.flags.size())
        throw std::out_of_range("ZOOM ZOO tile flag is missing");
    const auto behavior = rider.contact.auxiliary_flag
                            ? 0U
                            : unsigned(content.movement.flat_contact.flags[tile] & 0xfeU);
    if (behavior == tile_pair::unrecovered || behavior >= tile_pair::past_table)
        throw std::invalid_argument("movement reaches unrecovered tile flag pair "
                                    + std::to_string(behavior));
    return behavior;
}

// $81:8316-834B: unless the reflection is locked, a push along the descriptor's facing:
// velocity x by 0x20 and x by 8.
void run_push_tile(RiderMovementState& rider, const SpecialTileRider& tiles) {
    if (tiles.reflection_lock) return;
    const bool mirrored = (rider.contact.selected_word & mirrored_tile) != 0;
    rider.motion.velocity_x = add_word(rider.motion.velocity_x, mirrored ? 0xffe0U : 0x20U);
    rider.motion.x = static_cast<std::uint16_t>(rider.motion.x + (mirrored ? 0xfff8U : 8U));
}

// $81:8554-858D: the slow tile sets the tile mode and counts; at 8 it holds velocity x to
// +-0x20 (N-flag compares) instead.
void run_slow_tile(RiderMovementState& rider, SpecialTileRider& tiles, SurfaceTransition& surface,
                   SpecialTileUpdate& special) {
    surface.tile_mode = 1;
    const auto count = static_cast<std::uint16_t>(tiles.slow_counter + 1U);
    if (count != 8)
        tiles.slow_counter = count;
    else if (!negative(rider.motion.velocity_x)) {
        if (!negative(static_cast<std::uint16_t>(rider.motion.velocity_x - 0x20U)))
            rider.motion.velocity_x = 0x20;
    } else if (negative(static_cast<std::uint16_t>(rider.motion.velocity_x - 0xffe0U)))
        rider.motion.velocity_x = 0xffe0;
    special.slow_tile = 1;
}

// $81:8950-8998: flag pair 12 drives in steps of 1 and sets the animation delta ($0F3D) to 2
// against the held direction, as seen from the rider's facing; the brake path treats it as
// mud ($0FB1). Returns the animation delta.
int run_slow_drive_tile(const RiderMovementState& rider, unsigned horizontal,
                        SpecialTileUpdate& special) {
    int animation = 0;
    if (horizontal != direction::neutral)
        animation = ((horizontal == direction::right) != rider.pose.reflected) ? -2 : 2;
    special.drive_step = 1;
    special.crank_brake = 1;
    return animation;
}

// Flag pair 6 sets surface mode. On a wall it halves velocity x (rounding away from zero);
// elsewhere, unless braking, the D-pad along the descriptor's facing drives by 4.
void run_surface_drive_tile(RiderMovementState& rider, const ReflectionTransition& transition,
                            SurfaceTransition& surface, unsigned horizontal) {
    const auto descriptor = rider.contact.selected_word;
    const auto angle = static_cast<std::int16_t>(rider.contact.surface_angle);
    if (std::abs(angle) >= 31) {
        const auto vx = static_cast<std::int16_t>(rider.motion.velocity_x);
        if (vx != 0)
            rider.motion.velocity_x =
                static_cast<std::uint16_t>(vx < 0 ? ((vx >> 1) + 1) : ((vx >> 1) - 1));
    } else if (!transition.brake_input
               && horizontal
                      == ((descriptor & mirrored_tile) ? direction::left : direction::right)) {
        rider.motion.velocity_x =
            add_word(rider.motion.velocity_x,
                     (descriptor & mirrored_tile) ? static_cast<std::uint16_t>(-4) : 4);
    }
    surface.mode = 1;
    surface.angle = rider.contact.surface_angle;
}

// $81:84B2-84EB: away from leading support, the lift carries the rider one unit along the
// descriptor's facing and raises it: velocity y falls by 0x40 but not below -0x220 (an
// N-flag compare).
void run_lift_tile(RiderMovementState& rider, SurfaceTransition& surface) {
    if (surface.leading_support) return;
    rider.launch_override = 80;
    rider.motion.x = static_cast<std::uint16_t>(
        rider.motion.x + ((rider.contact.selected_word & mirrored_tile) ? 0xffffU : 1U));
    auto raised = static_cast<std::uint16_t>(rider.motion.velocity_y - 0x40U);
    if (negative(static_cast<std::uint16_t>(raised - 0xfde0U))) raised = 0xfde0U;
    rider.motion.velocity_y = raised;
    surface.tile_pose = 1;
    surface.tile_pose_enabled = 1;
}

// $81:89F7-8A29: with jump held the tile pushes velocity x by 4 the way the D-pad points;
// otherwise it only sets the tile pose.
void run_jump_driven_tile(RiderMovementState& rider, const ReflectionTransition& transition,
                          SurfaceTransition& surface, unsigned horizontal) {
    if (!transition.jump_input) {
        surface.tile_pose = 1;
        return;
    }
    if (horizontal != direction::neutral)
        rider.motion.velocity_x = static_cast<std::uint16_t>(
            rider.motion.velocity_x + (horizontal == direction::left ? 0xfffcU : 4U));
}

// The tile under the rider for this update. Returns the tile's animation delta.
int run_tile(ZoomZooState& next, unsigned index, unsigned horizontal, const ZoomZooContent& content,
             SpecialTileUpdate& special) {
    auto& rider = next.movement.riders[index];
    auto& transition = next.reflection[index];
    auto& surface = next.surface[index];
    auto& tiles = next.special_tiles[index];
    const auto behavior = tile_behavior_under(rider, content);
    int animation = 0;
    if (behavior == tile_pair::push) run_push_tile(rider, tiles);
    if (behavior == tile_pair::slow) run_slow_tile(rider, tiles, surface, special);
    if (behavior == tile_pair::slow_drive)
        animation = run_slow_drive_tile(rider, horizontal, special);
    if (behavior == tile_pair::loop)
        update_loop_tile(rider, tiles, surface, transition, special, content.loop_offsets);
    if (behavior == tile_pair::surface_drive)
        run_surface_drive_tile(rider, transition, surface, horizontal);
    if (behavior == tile_pair::boost) apply_boost_tile(rider, surface, 0);
    if (behavior == tile_pair::lift) run_lift_tile(rider, surface);
    if (behavior == tile_pair::mud) update_mud_tile(rider, tiles, surface, special);
    if (behavior == tile_pair::jump_driven)
        run_jump_driven_tile(rider, transition, surface, horizontal);
    if (behavior == tile_pair::corkscrew) {
        update_corkscrew_tile(rider, tiles, surface, transition, special,
                              content.corkscrew_heights);
        // Each step ends with $81:8949 storing 1 at $0EA3 by absolute address, the player's
        // rolling flag: the player's own update stores its flag back over it, but the
        // opponent's corkscrew sets the player's flag after the player's update has run.
        if (index == 1 && special.corkscrew_stepped) next.movement.riders[0].pose.rolling = true;
    }
    return animation;
}

// The player's reflection transition, the X trick on the rider's active update, and a
// landing's tricks. A capture-restored race knows only the rewards its captures reached.
unsigned run_tricks_and_landing(const ZoomZooState& state, ZoomZooState& next, unsigned index,
                                unsigned active, unsigned horizontal, const TrickButtons& buttons,
                                const ZoomZooContent& content) {
    auto& rider = next.movement.riders[index];
    auto& transition = next.reflection[index];
    const auto& surface = next.surface[index];
    if (index == 0)
        update_reflection_transition(
            rider, transition, horizontal, index != active, content.reflection_pose_table,
            state.native_initialization && buttons.player_a, next.special_tiles[index]);
    // The opponent's X is its trick selector's bit 2 ($0323), retained state, so it
    // re-derives each update for as long as the impulse holds.
    if (state.native_initialization && index == active)
        update_z_flip(next, index, index == 0 ? buttons.player_x : buttons.opponent_x(), content);
    if (index != active && !surface.leading_support) return 0;
    if (state.native_initialization) {
        announce_landing_tricks(next, index, content);
        return 0;
    }
    const bool landed =
        rider.motion.response_a || rider.contact.unsupported_count < off_ground_updates;
    const auto event =
        update_quarter_turns(rider, surface.leading_support != 0, transition.air_turns);
    if (landed) transition.air_turns = 0;
    if (index == 0 && event && (!state.complete_race || event != announcement::wipeout))
        throw std::invalid_argument("ZOOM ZOO player reward is unrecovered");
    return event && index == 1 ? event : 0;
}

// $82:A49F-A4F9, in the original's order: both rotations, a shallow supported contact or
// the corkscrew latch ($82:A4CC) clear the rotation; surface mode and leading support then
// keep it ($82:A4DC-A4E9, R-0047); no rotation clears it. Otherwise it turns 2 a update, 1
// while A is held ($82:A49F-A5F9; the opponent's A is its trick selector's bit 1).
// `$0F89` ($0E6F) is guarded zero.
void update_rotation(RiderMovementState& rider, const ReflectionTransition& transition,
                     const SurfaceTransition& surface, const SpecialTileRider& tiles,
                     bool holding_a) {
    const bool clear =
        tiles.corkscrew_latch
        || (transition.rotate_negative_input && transition.rotate_positive_input)
        || (std::abs(static_cast<std::int16_t>(rider.contact.surface_angle)) < shallow_slope
            && rider.contact.unsupported_count < airborne_updates);
    if (clear) {
        rider.motion.response_b = 0;
    } else if (surface.mode || surface.leading_support) {
    } else if (!transition.rotate_negative_input && !transition.rotate_positive_input) {
        rider.motion.response_b = 0;
    } else {
        rider.motion.response_b = static_cast<std::uint16_t>((rider.motion.response_b & 0xff00U)
                                                             | (transition.rotate_negative_input
                                                                    ? (holding_a ? 255U : 254U)
                                                                    : (holding_a ? 1U : 2U)));
    }
}

// The controls a rider applies on its active update: the direction latch, the standing
// animation, the jump, the low-speed damping, a completed turn's hold, the rotation and
// the wrong-direction warning.
void run_active_controls(const ZoomZooState& state, ZoomZooState& next, unsigned index,
                         unsigned horizontal, const TrickButtons& buttons,
                         const SpecialTileUpdate& special, int& animation_override) {
    auto& rider = next.movement.riders[index];
    auto& transition = next.reflection[index];
    const auto& surface = next.surface[index];
    const auto& tiles = next.special_tiles[index];
    // $82:A027-A068 initializes the direction latch on first opposition.
    if (!transition.direction_latch
        && (negative(rider.motion.velocity_x) ? horizontal != direction::left
                                              : horizontal != direction::right))
        transition.direction_latch = 0xffff;
    else if (negative(transition.direction_latch)
             && ((negative(rider.motion.velocity_x) && horizontal == direction::left)
                 || (!negative(rider.motion.velocity_x) && horizontal == direction::right)))
        transition.direction_latch = 48;
    // $82:A069-A0B6 returns without a store when the rider moved or the D-pad is centred, so
    // flag pair 12's delta survives it.
    if (const auto stationary =
            stationary_animation_override(rider, static_cast<std::uint8_t>(horizontal)))
        animation_override = stationary;
    // $82:A8CD-A8EE: the loop's cooldown ($0359), a tile-enabled pose ($0F41), leading or
    // inverted contact hold the jump input back.
    if (!tiles.loop_cooldown && !surface.tile_pose_enabled && !surface.leading_support
        && !(rider.contact.selected_high & high_tile))
        update_jump(rider, transition.jump_input != 0);
    // $82:A5FC: a tile's drive step replaces the low-speed damping.
    if (!special.drive_step) update_active_low_speed_damping(rider);
    transition.drive_pose_enabled = 0;
    // $82:A241: the corkscrew latch skips the completed-turn hold.
    if (transition.completed && !tiles.corkscrew_latch) {
        if (static_cast<std::int16_t>(rider.motion.previous_x_displacement) < 2) {
            transition.completed = 0;
        } else {
            transition.hold = turn_hold;
            if (rider.contact.unsupported_count != airborne_updates
                && horizontal != direction::neutral)
                transition.drive_pose_enabled = 1;
        }
    }
    const bool holding_a =
        index == 0 ? buttons.player_a : (state.native_initialization && buttons.opponent_a());
    update_rotation(rider, transition, surface, tiles, holding_a);
    const auto previous_wrong_direction = transition.wrong_direction_counter;
    transition.wrong_direction_counter = next_wrong_direction_counter(
        previous_wrong_direction, rider.motion.velocity_x, rider.progress.marker_word, horizontal,
        state.native_initialization);
    if (state.native_initialization && previous_wrong_direction == wrong_way_warning - 1
        && transition.wrong_direction_counter == wrong_way_repeat) {
        // $82:9751-9762 / $82:9781-9792: the fixed one-player scenario ($77:074B = 1).
        if (index == 0)
            queue_player_announcement(next, announcement::wrong_way);
        else
            queue_opponent_announcement(next.movement, announcement::wrong_way);
    }
}

// $82:A77E-A797 and the limiter's context: the opponent's cap rises by $1283 * 2 while the
// player leads ($1283 is 64 on the HUNTER tour, 0 elsewhere; LOCKED-TOURS). $150B has no
// writer in the declared continuation, so the pose byte keeps its seed: the player's
// published screen x, the opponent's retained OAM x.
SpeedLimitContext speed_limit_context(const ZoomZooState& state, const ZoomZooState& next,
                                      unsigned index, unsigned horizontal,
                                      const ClassicRaceScenario& scenario) {
    const auto& whole = next.movement;
    const auto& rider = whole.riders[index];
    SpeedLimitContext limit{};
    limit.opponent = index == 1;
    limit.ai_enabled = true;
    limit.pose_byte = index == 1 ? next.opponent_retained_oam_x
                                 : static_cast<std::uint8_t>(next.race.camera.screen_xy);
    limit.drag = state.complete_race
              && (index == 0 ? next.race.provisional_1225 : next.race.provisional_1227);
    limit.start_override = rider.launch_override != 0;
    limit.player_progress = whole.riders[0].progress.transition_count;
    limit.opponent_progress = whole.riders[1].progress.transition_count;
    limit.adjustment_limit = race_adjustment_limit(scenario);
    limit.ai_adjustment = scenario.ai_adjustment;
    limit.player_base_cap = next.reflection[0].base_velocity_cap;
    limit.update_counter = whole.update_counter;
    limit.friction_mode = static_cast<std::uint16_t>(horizontal);
    return limit;
}

// Gravity, the speed limit, the position, and the settle onto a sloped surface.
void run_physics(const ZoomZooState& state, ZoomZooState& next, unsigned index, unsigned horizontal,
                 const SpecialTileUpdate& special, const ClassicRaceScenario& scenario,
                 const ZoomZooContent& content) {
    auto& rider = next.movement.riders[index];
    const auto& tiles = next.special_tiles[index];
    const auto& surface = next.surface[index];
    // $82:A971-A97C: the corkscrew's float or physics hold suspends gravity.
    if (!tiles.corkscrew_float && !tiles.physics_hold) update_gravity(rider);
    // A player boost below 16 makes the limiter's optional subtraction inert.
    if (index == 0 && rider.speed.boost >= 16 && !state.native_initialization)
        throw std::invalid_argument("ZOOM ZOO player boost requires unrecovered camera state");
    const auto limit = speed_limit_context(state, next, index, horizontal, scenario);
    // $82:A6FD: and the speed limiter.
    if (!tiles.physics_hold)
        limit_rider_speed(rider.motion.velocity_x, rider.motion.velocity_y, rider.speed, limit,
                          content.movement.speed_decay);
    integrate_zoom_axis(rider.motion.x, rider.motion.velocity_x, rider.residue_x);
    rider.motion.x &= track_geometry(content.movement.sampling.track).position_mask;
    integrate_zoom_axis(rider.motion.y, rider.motion.velocity_y, rider.residue_y);
    // $82:A6BB-A6F5: skipped on flag pair 8 ($0F2D).
    if (rider.contact.surface_angle && !special.slow_tile
        && rider.contact.unsupported_count < off_ground_updates
        && !(rider.contact.selected_high & high_tile))
        rider.motion.y = static_cast<std::uint16_t>(
            static_cast<int>(rider.motion.y)
            + (negative(rider.motion.velocity_y) ? -1 : (surface.mode ? 1 : 4)));
}

// One rider's update, in the original's order.
RiderOutcome update_rider(const ZoomZooState& state, ZoomZooState& next, unsigned index,
                          unsigned active, const TrickButtons& buttons,
                          const ClassicRaceScenario& scenario, const ZoomZooContent& content) {
    auto& whole = next.movement;
    auto& rider = whole.riders[index];
    auto& transition = next.reflection[index];
    auto& surface = next.surface[index];
    auto& tiles = next.special_tiles[index];
    begin_rider_update(next, index, content);
    const unsigned horizontal =
        index == 0 ? whole.player_input.horizontal : next.opponent_horizontal;
    // Transient words of this update: $0F3B and $0F3F (mud), $0F5B (corkscrew).
    SpecialTileUpdate special{};
    int animation_override = run_tile(next, index, horizontal, content, special);
    RiderOutcome outcome{special.contact_skip, 0};
    bool throttle_target = false;
    outcome.reward =
        run_tricks_and_landing(state, next, index, active, horizontal, buttons, content);
    if (index == active)
        run_active_controls(state, next, index, horizontal, buttons, special, animation_override);
    update_rolling_mode(rider, surface.mode != 0);
    if (index == 1)
        update_reflection_transition(rider, transition, horizontal, index != active,
                                     content.reflection_pose_table,
                                     state.native_initialization && buttons.opponent_a(), tiles);
    // $82:98D6: the corkscrew's physics hold skips the whole drive routine, brake latch
    // included, and leaves $0E7B as the other rider set it.
    if (!tiles.physics_hold) {
        update_drive(rider, transition, horizontal, animation_override, throttle_target,
                     next.charge_announced[index], surface.leading_support != 0,
                     state.native_initialization && next.rolls[index].bounce_active != 0,
                     special.drive_step ? special.drive_step : 24,
                     tiles.mud_cooldown != 0 || special.crank_brake);
        next.drive_target_latch = throttle_target ? 0 : 1;
    }
    update_idle_pose(
        rider, next.drive_target_latch && !surface.leading_support && transition.pose_override == 0,
        index == 1, whole.animation_counter, content.movement.idle_pose_table);
    if (rider.idle_pose.active) surface.tile_mode = 1;
    run_physics(state, next, index, horizontal, special, scenario, content);
    surface.animation_delta = static_cast<std::uint16_t>(animation_override);
    update_pose(rider, whole.animation_counter, whole.contact_phase, content.movement,
                animation_override, next.drive_target_latch == 0,
                surface.mode ? static_cast<std::int16_t>(surface.angle) : 0, special.mud_velocity,
                special.slow_tile != 0);
    if (transition.pose_override) rider.pose.pose_index = transition.pose_override;
    if (index == active)
        advance_track_progress(rider.progress, content.movement.progress_transitions);
    return outcome;
}

// $81:8CFC / $81:8E43: each rider's contact with the track, unless the corkscrew carries it
// ($0DFB/$0DFD). $81:91F4-920C clears leading support on the auxiliary boundary return,
// even when an earlier probe established support.
void update_rider_contact(const ZoomZooState& state, ZoomZooState& next, unsigned index,
                          const ZoomZooContent& content) {
    auto& whole = next.movement;
    auto& rider = whole.riders[index];
    const auto& sampling = content.movement.sampling;
    const auto points = collision_points(sampling, rider.pose.pose_index, rider.pose.reflected);
    const auto samples = sample_track(sampling, points, rider.motion.x, rider.motion.y,
                                      track_geometry(sampling.track).coarse_columns);
    const auto summary = summarize_vertical_contact(content.movement.flat_contact, points, samples,
                                                    rider.motion.x, rider.motion.y);
    if (content.slope_coefficients.size() != 18 && content.slope_coefficients.size() != 128)
        throw std::invalid_argument("ZOOM ZOO slope coefficients missing");
    const bool surface_mode = next.surface[index].mode != 0;
    resolve_vertical_contact(
        rider.contact, rider.motion, summary,
        {whole.contact_phase, index == 1, next.surface[index].mode, 0xc200,
         next.special_tiles[index].loop_step == 9,
         index == 0 && next.hunter.effect[hunter_effect::power_bounce] != 0},
        content.slope_coefficients.subspan(state.sustained && surface_mode ? 64 : 0,
                                           state.sustained ? 32 : 9),
        content.slope_coefficients.subspan(state.sustained ? (surface_mode ? 96 : 32) : 9),
        content.landing_matrices,
        index == 0 ? whole.player_input.horizontal : next.opponent_horizontal,
        rider.pose.pose_index, rider.pose.reflected);
    next.surface[index].leading_support =
        rider.contact.auxiliary_flag == 1 ? false : summary.leading_support;
    if (state.native_initialization)
        next.rolls[index].support_count_mirror = rider.contact.unsupported_count;
    observe_track_markers(rider.progress, samples);
}

// $81:C73E-C75B: when the race clock would reach 10:00 it holds 9:59.9 and marks both
// riders finished, whatever their laps. Then the queues, the camera, each rider's contact,
// the visibility, the HUNTER effects and the hints.
void finish_update(const ZoomZooState& state, ZoomZooState& next,
                   const std::array<RiderOutcome, 2>& outcomes, const ZoomZooContent& content) {
    auto& whole = next.movement;
    if (advance_timer_digits(whole.timer, whole.countdown < clock_running_below)
        && state.native_initialization)
        for (auto& rider : next.race.riders) rider.finished = 1;
    if (state.native_initialization)
        show_next_player_announcement(next, content.movement, content.captions);
    update_opponent_announcements(whole, outcomes[1].reward, content.movement,
                                  state.native_initialization
                                      ? std::span<std::uint8_t>{next.learned_weights[1]}
                                      : std::span<std::uint8_t>{});
    if (state.complete_race)
        update_zoom_camera(next, track_geometry(content.movement.sampling.track));
    for (unsigned index = 0; index < 2; ++index)
        if (!outcomes[index].contact_skip) update_rider_contact(state, next, index, content);
    if (state.complete_race)
        update_zoom_visibility(next, track_geometry(content.movement.sampling.track));
    update_hunter_effects(next, content.hunter_blink);
    if (state.native_initialization) update_tutorial_hints(next);
    ++whole.frame;
}

} // namespace

// The wrong-direction counter counts the rider's active updates moving (16 or more either
// way) against the marker's direction; the warning comes at 180, then every 60.
std::uint16_t next_wrong_direction_counter(std::uint16_t previous, std::uint16_t velocity_x,
                                           std::uint16_t marker, unsigned horizontal,
                                           bool native_rewards) {
    const bool moving = !negative(static_cast<std::uint16_t>(velocity_x - 16U))
                     || negative(static_cast<std::uint16_t>(velocity_x - 0xfff0U));
    const bool against =
        (marker & 0x4000U) ? horizontal == direction::right : horizontal == direction::left;
    if (!moving || (marker & 0x8000U) || !against) return 0;
    const auto next = add_word(previous, 1);
    if (next == wrong_way_warning) {
        if (native_rewards) return wrong_way_repeat; // $82:974B/977B
        throw std::invalid_argument("ZOOM ZOO wrong-direction reward is unrecovered");
    }
    return next;
}

void update_zoom_zoo(ZoomZooState& state, const ControllerButtons& requested_buttons,
                     const ZoomZooContent& content) {
    const auto request = gate_controller(state, requested_buttons);
    validate_zoom_zoo_content_state(state, content);
    const auto scenario = classic_race_scenario(state.track);
    if (state.native_initialization && state.race.finish_delay == finish_display_updates) {
        advance_result_screen(state, scenario);
        return;
    }
    check_update_domain(state, request, scenario);
    // A SNES pad's rocker cannot close both contacts of one axis, and the controller port
    // publishes up & !down and left & !right (R-0041): opposing directions leave WRAM as a
    // released D-pad does. The rocker is applied here, once, for every caller; the domain
    // check above reads the request, so the accepted continuation domain is unchanged.
    const auto buttons = with_physical_dpad(request);
    auto next = state;
    auto& whole = next.movement;
    count_hunter_mosaic(next);
    next.hunter.skip_update = 0;
    if (state.hunter.skip_update) {
        run_skipped_update(next, buttons, content);
        state = next;
        return;
    }
    advance_clocks(next, buttons);
    const bool pressed_a = read_player_buttons(
        next, buttons, state.hunter.effect[hunter_effect::control_reversed] != 0);
    if (run_pause_menu(state, next, buttons, content)) {
        state = next;
        return;
    }
    if (state.native_initialization && next.pause.released && !buttons.start)
        next.pause.released = 0;
    const bool ai_off = update_opponent_controller(next);
    const bool countdown_holds = run_countdown(state, next);
    // The opponent's A and X are its selector's bits only on updates the AI stores them;
    // with the AI off both stay released (R-0048).
    const TrickButtons trick_buttons{
        pressed_a && !countdown_holds, buttons.x && !countdown_holds,
        countdown_holds ? 0U : (unsigned(whole.opponent_ai.trick_selector) & (ai_off ? ~6U : ~0U))};
    if (state.complete_race) update_zoom_finish(next, content);
    const unsigned active = whole.progress_phase ? 0U : 1U;
    if (state.native_initialization) {
        auto& cooldown = next.player_announcements.queue.cooldown;
        cooldown = cooldown > 2 ? static_cast<std::uint16_t>(cooldown - 2U) : 0;
    }
    whole.rewards.cooldown =
        whole.rewards.cooldown > 2 ? static_cast<std::uint16_t>(whole.rewards.cooldown - 2U) : 0;
    std::array<RiderOutcome, 2> outcomes{};
    for (unsigned index = 0; index < 2; ++index)
        outcomes[index] =
            update_rider(state, next, index, active, trick_buttons, scenario, content);
    finish_update(state, next, outcomes, content);
    state = next;
}

} // namespace unirally
