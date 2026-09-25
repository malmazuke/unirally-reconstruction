// One race update: the order in which every system runs.

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

void update_reflection_transition(RiderMovementState& rider, ReflectionTransition& transition,
                                  unsigned horizontal, bool inactive_phase,
                                  std::span<const std::uint8_t> table, bool manual = false,
                                  const SpecialTileRider& tiles = {}) {
    // $82:A35B-A49E: A permits a direction-selected turn while airborne,
    // including a turn toward the current facing. Contact A halves velocity.
    // $82:A35D-A362: the corkscrew's reflection lock skips it all.
    if (tiles.reflection_lock) return;
    const bool manual_airborne = rider.contact.unsupported_count == 9 && !transition.step
                              && !(rider.contact.selected_high & 0x80U) && manual
                              && horizontal != 1;
    if (!(rider.contact.unsupported_count == 9 && transition.step) && !manual_airborne
        && !rider.contact.recontact && !inactive_phase)
        return;
    if (!transition.step && !manual_airborne
        && (horizontal == 1 || (horizontal == 2 && rider.pose.reflected)
            || (horizontal == 0 && !rider.pose.reflected)))
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
            transition.step = 9;
            transition.end = 16;
        } else {
            transition.step = 1;
            transition.end = 9;
        }
    }
    transition.pose_base =
        static_cast<std::uint16_t>(content_word(table, 2U * rider.pose.reflected_orientation));
    if (transition.step == transition.end) {
        if (rider.contact.unsupported_count >= 8)
            transition.air_turns = add_word(transition.air_turns, 1);
        transition.completed = 1;
        if (transition.end != 16) rider.pose.reflected = true;
        transition.step = 0;
        transition.pose_override = 0;
    } else {
        transition.pose_override =
            static_cast<std::uint16_t>(transition.pose_base + transition.step + 0x620U);
        transition.step = add_word(transition.step, 1);
    }
}

void integrate_zoom_axis(std::uint16_t& position, std::uint16_t velocity, std::uint16_t& residue) {
    const auto total = static_cast<std::int16_t>(add_word(velocity, residue));
    position = static_cast<std::uint16_t>(static_cast<int>(position) + total / 32);
    residue = static_cast<std::uint16_t>(total % 32);
}

} // namespace

std::uint16_t next_wrong_direction_counter(std::uint16_t previous, std::uint16_t velocity_x,
                                           std::uint16_t marker, unsigned horizontal,
                                           bool native_rewards) {
    const bool moving = !negative(static_cast<std::uint16_t>(velocity_x - 16U))
                     || negative(static_cast<std::uint16_t>(velocity_x - 0xfff0U));
    if (!moving || (marker & 0x8000U) || !((marker & 0x4000U) ? horizontal == 2 : horizontal == 0))
        return 0;
    const auto next = add_word(previous, 1);
    if (next == 180) {
        if (native_rewards)
            return 120; // $82974B/977B repeats the warning after60 further active updates.
        throw std::invalid_argument("ZOOM ZOO wrong-direction reward is unrecovered");
    }
    return next;
}

void update_zoom_zoo(ZoomZooState& state, const ControllerButtons& requested_buttons,
                     const ZoomZooContent& content) {
    // NMI $808642-865B skips controller publication through prior fade4;
    // first controller publication uses prior fade5 (native update1382).
    const auto gated_request = state.native_initialization && state.fade_level < 5
                                 ? ControllerButtons{}
                                 : requested_buttons;
    validate_zoom_zoo_content_state(state, content);
    const auto scenario = classic_race_scenario(state.track);
    if (state.native_initialization && state.race.finish_delay == 240) {
        // Original graph load begins on the update following finish display240.
        // The authenticated load is black for108 updates, then seven brightness
        // steps. Preserve a final-race archive; the original reuses that memory.
        if (state.result_updates < stable_result_updates(state)) ++state.result_updates;
        state.result = zoom_result_fields(state.race, state.result_updates, scenario.tour_race);
        ++state.movement.frame;
        return;
    }
    if ((gated_request.y && !state.native_initialization)
        || (gated_request.select && !state.native_initialization)
        || (gated_request.start && !state.native_initialization)
        || ((!state.native_initialization)
            && (gated_request.up || gated_request.down || gated_request.a))
        || (!state.complete_race && gated_request.left)
        || (gated_request.x && !state.native_initialization)
        || ((!state.native_initialization)
            && (gated_request.left_shoulder || gated_request.right_shoulder)))
        throw std::invalid_argument("ZOOM ZOO controller is outside the recovered domain");
    if (state.movement.frame < (state.native_initialization ? scenario.initialization_frame : 1649U)
        || (!state.native_initialization
            && state.movement.frame >= (state.sustained ? 9999U : 1849U)))
        throw std::invalid_argument("ZOOM ZOO update is outside the declared trial horizon");
    // A SNES pad's rocker cannot close both contacts of one axis, and the
    // controller port publishes `up & !down` and `left & !right` (the audited
    // core's sfc/controller/gamepad says so in those terms). Measured on the
    // original: opposing directions leave WRAM byte-identical to a released
    // D-pad, so the engine never sees them and what this game's own branches
    // would do with both is not recovered behaviour (R-0041). Every caller
    // passes what a device asked for; the rocker is applied here, once, for
    // both tracks. `buttons` is what the port publishes; the guard above
    // deliberately reads the request instead, so the historical continuation
    // domain is exactly the accepted one.
    const auto buttons = with_physical_dpad(gated_request);
    auto next = state;
    auto& whole = next.movement;
    count_hunter_mosaic(next);
    // $83:CC9A-CCA2: an update the HUNTER effects skip ($128B, R-0052) runs
    // only the effects themselves and the hints ($83:CDAA); the race, its
    // clocks, the controller reader ($82:AA71, the axes and button words) and
    // the pause menu wait. The NMI still publishes the pad images $0311-$0314
    // ($80:87E9-87FE).
    next.hunter.skip_update = 0;
    if (state.hunter.skip_update) {
        const auto pad = sample_controller(buttons);
        whole.player_input.low_image = pad.low_image;
        whole.player_input.high_image = pad.high_image;
        update_hunter_effects(next, content.hunter_blink);
        if (state.native_initialization) update_tutorial_hints(next);
        ++whole.frame;
        state = next;
        return;
    }
    whole.player_input = sample_controller(buttons);
    whole.contact_phase = static_cast<std::uint8_t>(1U - whole.contact_phase);
    whole.progress_phase = static_cast<std::uint8_t>(1U - whole.progress_phase);
    whole.animation_counter = static_cast<std::uint8_t>((whole.animation_counter + 1U) & 31U);
    whole.update_counter = static_cast<std::uint8_t>(whole.update_counter + 1U);
    if (whole.countdown >= (state.native_initialization ? 271U : 69U))
        throw std::invalid_argument("ZOOM ZOO countdown is outside continuation domain");
    if (!state.native_initialization && whole.countdown) --whole.countdown;
    if (state.native_initialization && next.fade_level < 30) ++next.fade_level;
    auto& player_input = next.reflection[0];
    // $82:AAE2-AAFB: B drives $0331 (jump); Y drives $0325 (brake).
    player_input.brake_input = buttons.y;
    player_input.jump_input = buttons.b;
    player_input.rotate_negative_input = buttons.left_shoulder;
    player_input.rotate_positive_input = buttons.right_shoulder;
    bool pressed_a = buttons.a;
    if (state.hunter.effect[hunter_effect::control_reversed]) {
        // $82:AC5A-ACA7: the HUNTER effect 7 reverses the controls the port
        // reader has published: left and right, the two rotations, and Y
        // (brake) with A. It swaps the opponent's port-2 words too, which the
        // AI then overwrites.
        whole.player_input.horizontal =
            static_cast<std::uint8_t>(2U - whole.player_input.horizontal);
        std::swap(player_input.rotate_negative_input, player_input.rotate_positive_input);
        const bool brake = player_input.brake_input != 0;
        player_input.brake_input = pressed_a;
        pressed_a = brake;
    }
    // $83CD05-CD35: controller/global phase clocks are sampled before the
    // pause menu diverts this update. Race, AI, queues and hints do not advance.
    if (state.native_initialization
        && (state.pause.selection || (buttons.start && !state.race.riders[0].finished))) {
        auto& pause = next.pause;
        if (!pause.selection) pause.selection = 1;
        if (whole.player_input.vertical != 1)
            pause.selection = whole.player_input.vertical ? 0xffff : 1;
        if (!buttons.start)
            pause.released = 1;
        else if (pause.released) {
            if (negative(pause.selection)) {
                // Authored standalone navigation: this menu says RESTART RACE.
                // Original Retire/tour progression is deliberately not emulated.
                restart_zoom_zoo(next, content);
                state = next;
                return;
            }
            pause.selection = 0;
        }
        ++pause.suspended_updates;
        if (next.fade_level >= 5 && whole.countdown) ++pause.suspended_countdown_updates;
        next.opponent_horizontal = 1;
        auto& opponent = next.reflection[1];
        opponent.brake_input = opponent.jump_input = opponent.rotate_negative_input =
            opponent.rotate_positive_input = 0;
        ++whole.frame;
        state = next;
        return;
    }
    if (state.native_initialization && next.pause.released && !buttons.start)
        next.pause.released = 0;
    const bool inverted_marker = update_opponent_controller(next);
    // $83:E7A2-E7BF also releases both riders' A ($031D/$031F) and X
    // ($0321/$0323) publications while the countdown holds the brakes.
    bool countdown_releases_actions = false;
    if (state.native_initialization && whole.countdown) {
        // $83:E59C-E7BD: countdown presentation feeds braking and start boost.
        if (next.fade_level >= 5) {
            if (whole.countdown < 130 && whole.countdown > 100) {
                for (unsigned i = 0; i < 2; ++i)
                    if (!next.reflection[i].brake_input) next.start_boost[i] = 0;
            }
            if (whole.countdown < 70) {
                for (unsigned i = 0; i < 2; ++i)
                    if (!next.reflection[i].brake_input) {
                        whole.riders[i].speed.boost =
                            add_word(whole.riders[i].speed.boost, next.start_boost[i]);
                        next.start_boost[i] = 0;
                    }
            }
            --whole.countdown;
        }
        if (state.movement.countdown >= 70 || next.fade_level < 5) {
            if (next.fade_level >= 5 && state.movement.countdown <= 100)
                for (auto& input : next.reflection) input.brake_input = 1;
            if (!player_input.brake_input) whole.player_input.horizontal = 1;
            if (!next.reflection[1].brake_input) next.opponent_horizontal = 1;
            for (auto& input : next.reflection) {
                input.brake_input = 1;
                input.jump_input = 0;
            }
            countdown_releases_actions = true;
        }
    }
    const bool player_a = pressed_a && !countdown_releases_actions;
    const bool player_x = buttons.x && !countdown_releases_actions;
    // The opponent's A and X are the selector's bits 1 and 2 only on updates
    // the AI stores them; after an inverted marker both stay released (R-0048).
    const auto opponent_trick =
        countdown_releases_actions
            ? 0U
            : (unsigned(whole.opponent_ai.trick_selector) & (inverted_marker ? ~6U : ~0U));
    if (state.complete_race) update_zoom_finish(next, content);
    const unsigned active = whole.progress_phase ? 0U : 1U;
    unsigned reward = 0;
    if (state.native_initialization) {
        auto& cooldown = next.player_announcements.queue.cooldown;
        cooldown = cooldown > 2 ? static_cast<std::uint16_t>(cooldown - 2U) : 0;
    }
    whole.rewards.cooldown =
        whole.rewards.cooldown > 2 ? static_cast<std::uint16_t>(whole.rewards.cooldown - 2U) : 0;
    std::array<std::uint16_t, 2> contact_skip{};
    for (unsigned index = 0; index < 2; ++index) {
        auto& rider = whole.riders[index];
        auto& transition = next.reflection[index];
        auto& surface = next.surface[index];
        if (state.complete_race) update_zoom_checkpoint(next, index, content);
        surface.tile_mode = 0;
        surface.animation_delta = 0;
        surface.tile_pose = 0;
        surface.tile_pose_enabled = 0;
        const unsigned horizontal =
            index == 0 ? whole.player_input.horizontal : next.opponent_horizontal;
        auto& tiles = next.special_tiles[index];
        // $81:859D-85A8: the reflection lock counts down while the previous
        // update left surface mode clear.
        // $81:8592-8599: flag pair 8's counter falls by one, not below zero.
        if (tiles.slow_counter) --tiles.slow_counter;
        if (!surface.mode && tiles.reflection_lock) --tiles.reflection_lock;
        update_loop_cooldown(rider, tiles, transition);
        decay_idle_wobble(rider, surface.mode != 0);
        surface.mode = 0;
        rider.launch_override = 0;
        update_special_tile_counters(tiles, transition, rider.contact.selected_high);
        // Transient words of this update: $0F3B and $0F3F (mud), $0F5B (corkscrew).
        SpecialTileUpdate special{};
        const auto descriptor = rider.contact.selected_word;
        const auto tile = ((descriptor & 0x3f0U) >> 2U) + ((descriptor & 15U) >> 1U);
        if (tile >= content.movement.flat_contact.flags.size())
            throw std::out_of_range("ZOOM ZOO tile flag is missing");
        // $81:82BB-82F3 dispatches the selected tile's flag pair through the
        // table at $81:82F5 unless the auxiliary flag is set. Pairs 18 and 22
        // are a bare RTS ($81:84AC); pair 20 is the checkpoint tile, which
        // update_zoom_checkpoint runs. Pair 4 ($81:875C) is unrecovered, and
        // the table has no entry above pair 28 (R-0051).
        const auto tile_behavior = rider.contact.auxiliary_flag
                                     ? 0U
                                     : unsigned(content.movement.flat_contact.flags[tile] & 0xfeU);
        if (tile_behavior == 4 || tile_behavior >= 30)
            throw std::invalid_argument("movement reaches unrecovered tile flag pair "
                                        + std::to_string(tile_behavior));
        if (tile_behavior == 28 && !tiles.reflection_lock) {
            // $81:8316-834B: unless the reflection is locked, a push along the
            // descriptor's facing: velocity x by $20 and x by 8.
            const bool mirrored = (descriptor & 0x4000U) != 0;
            rider.motion.velocity_x = add_word(rider.motion.velocity_x, mirrored ? 0xffe0U : 0x20U);
            rider.motion.x = static_cast<std::uint16_t>(rider.motion.x + (mirrored ? 0xfff8U : 8U));
        }
        if (tile_behavior == 8) {
            // $81:8554-858D: the slow tile sets the tile mode and counts;
            // at 8 it holds velocity x to +-$20 (N-flag compares) instead.
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
        // $81:8950-8998: flag pair 12 drives in steps of 1 and sets the
        // animation delta ($0F3D) to 2 against the held direction, as seen
        // from the rider's facing; the brake path treats it as mud ($0FB1).
        int tile_animation = 0;
        if (tile_behavior == 12) {
            if (horizontal != 1)
                tile_animation = ((horizontal == 2) != rider.pose.reflected) ? -2 : 2;
            special.drive_step = 1;
            special.crank_brake = 1;
        }
        if (tile_behavior == 26)
            update_loop_tile(rider, tiles, surface, transition, special, content.loop_offsets);
        if (tile_behavior == 6) {
            const auto angle = static_cast<std::int16_t>(rider.contact.surface_angle);
            if (std::abs(angle) >= 31) {
                const auto vx = static_cast<std::int16_t>(rider.motion.velocity_x);
                if (vx != 0)
                    rider.motion.velocity_x =
                        static_cast<std::uint16_t>(vx < 0 ? ((vx >> 1) + 1) : ((vx >> 1) - 1));
            } else if (!transition.brake_input
                       && horizontal == ((descriptor & 0x4000U) ? 0U : 2U)) {
                rider.motion.velocity_x =
                    add_word(rider.motion.velocity_x,
                             (descriptor & 0x4000U) ? static_cast<std::uint16_t>(-4) : 4);
            }
            surface.mode = 1;
            surface.angle = rider.contact.surface_angle;
        }
        if (tile_behavior == 2) apply_boost_tile(rider, surface, 0);
        if (tile_behavior == 24 && !surface.leading_support) {
            // $81:84B2-84EB: away from leading support, the tile carries the
            // rider one unit along the descriptor's facing and raises it:
            // velocity y falls by $40 but not below -$220 (an N-flag compare).
            rider.launch_override = 80;
            rider.motion.x = static_cast<std::uint16_t>(rider.motion.x
                                                        + ((descriptor & 0x4000U) ? 0xffffU : 1U));
            auto raised = static_cast<std::uint16_t>(rider.motion.velocity_y - 0x40U);
            if (negative(static_cast<std::uint16_t>(raised - 0xfde0U))) raised = 0xfde0U;
            rider.motion.velocity_y = raised;
            surface.tile_pose = 1;
            surface.tile_pose_enabled = 1;
        }
        if (tile_behavior == 14) update_mud_tile(rider, tiles, surface, special);
        if (tile_behavior == 16) {
            // $81:89F7-8A29: with jump held the tile pushes velocity x by 4
            // the way the D-pad points; otherwise it only sets the tile pose.
            if (transition.jump_input) {
                if (horizontal != 1)
                    rider.motion.velocity_x = static_cast<std::uint16_t>(
                        rider.motion.velocity_x + (horizontal == 0 ? 0xfffcU : 4U));
            } else
                surface.tile_pose = 1;
        }
        if (tile_behavior == 10) {
            update_corkscrew_tile(rider, tiles, surface, transition, special,
                                  content.corkscrew_heights);
            // Each step ends with $81:8949 storing 1 at $0EA3 by absolute
            // address, the player's rolling flag: the player's own update
            // stores its flag back over it, but the opponent's corkscrew
            // sets the player's flag after the player's update has run.
            if (index == 1 && special.corkscrew_stepped) whole.riders[0].pose.rolling = true;
        }
        contact_skip[index] = special.contact_skip;
        int animation_override = tile_animation;
        bool throttle_target = false;
        if (index == 0)
            update_reflection_transition(rider, transition, horizontal, index != active,
                                         content.reflection_pose_table,
                                         state.native_initialization && player_a, tiles);
        // The opponent's X comes from trick selector bit 2 ($0323) rather than
        // from a controller; the selector is retained state, so it re-derives
        // each update for as long as the impulse holds.
        if (state.native_initialization && index == active)
            update_z_flip(next, index, index == 0 ? player_x : (opponent_trick & 4U) != 0, content);
        if (index == active || surface.leading_support) {
            if (state.native_initialization)
                announce_landing_tricks(next, index, content);
            else {
                const bool landed = rider.motion.response_a || rider.contact.unsupported_count < 2;
                const auto event =
                    update_quarter_turns(rider, surface.leading_support != 0, transition.air_turns);
                if (landed) transition.air_turns = 0;
                if (index == 0 && event && (!state.complete_race || event != 14))
                    throw std::invalid_argument("ZOOM ZOO player reward is unrecovered");
                if (event && index == 1) reward = event;
            }
        }
        if (index == active) {
            // $82:A027-A068 initializes the direction latch on first opposition.
            if (!transition.direction_latch
                && (negative(rider.motion.velocity_x) ? horizontal != 0 : horizontal != 2))
                transition.direction_latch = 0xffff;
            else if (negative(transition.direction_latch)
                     && ((negative(rider.motion.velocity_x) && horizontal == 0)
                         || (!negative(rider.motion.velocity_x) && horizontal == 2)))
                transition.direction_latch = 48;
            // $82:A069-A0B6 returns without a store when the rider moved or
            // the D-pad is centred, so flag pair 12's delta survives it.
            if (const auto stationary =
                    stationary_animation_override(rider, static_cast<std::uint8_t>(horizontal)))
                animation_override = stationary;
            // $82A8CD-A8EE rejects the loop's cooldown ($0359), a tile-enabled
            // pose ($0F41), leading or inverted contact before consuming
            // pending/previous input.
            if (!tiles.loop_cooldown && !surface.tile_pose_enabled && !surface.leading_support
                && !(rider.contact.selected_high & 0x80U))
                update_jump(rider, transition.jump_input != 0);
            // $82:A5FC: mud's drive step replaces the low-speed damping.
            if (!special.drive_step) update_active_low_speed_damping(rider);
            transition.drive_pose_enabled = 0;
            // $82:A241: the corkscrew latch skips the completed-turn hold.
            if (transition.completed && !tiles.corkscrew_latch) {
                if (static_cast<std::int16_t>(rider.motion.previous_x_displacement) < 2)
                    transition.completed = 0;
                else {
                    transition.hold = 30;
                    if (rider.contact.unsupported_count != 9 && horizontal != 1)
                        transition.drive_pose_enabled = 1;
                }
            }
            // $82:A49F-A4F9 in the original's order: both rotations, a
            // shallow supported contact or the corkscrew latch ($82:A4CC)
            // clear the rotation; surface mode and leading support then
            // return with it unchanged ($82:A4DC-A4E9, R-0047); no rotation
            // clears it. `$0F89` ($0E6F) is guarded zero.
            const bool clear =
                tiles.corkscrew_latch
                || (transition.rotate_negative_input && transition.rotate_positive_input)
                || (std::abs(static_cast<std::int16_t>(rider.contact.surface_angle)) < 30
                    && rider.contact.unsupported_count < 9);
            if (clear)
                rider.motion.response_b = 0;
            else if (surface.mode || surface.leading_support) {
            } else if (!transition.rotate_negative_input && !transition.rotate_positive_input)
                rider.motion.response_b = 0;
            else {
                // $82A49F-A5F9 halves the rotation step while A is held. The
                // rate was keyed to the player's button, so the opponent always
                // rotated at the fast step; its A arrives from trick selector
                // bit 1 instead and must slow it the same way.
                const bool holding_a =
                    index == 0 ? player_a
                               : (state.native_initialization && (opponent_trick & 2U) != 0);
                rider.motion.response_b = static_cast<std::uint16_t>(
                    (rider.motion.response_b & 0xff00U)
                    | (transition.rotate_negative_input ? (holding_a ? 255U : 254U)
                                                        : (holding_a ? 1U : 2U)));
            }
            const auto previous_wrong_direction = transition.wrong_direction_counter;
            transition.wrong_direction_counter = next_wrong_direction_counter(
                previous_wrong_direction, rider.motion.velocity_x, rider.progress.marker_word,
                horizontal, state.native_initialization);
            if (state.native_initialization && previous_wrong_direction == 179
                && transition.wrong_direction_counter == 120) {
                // $829751-9762 / $829781-9792: fixed one-player scenario $77074B=1.
                if (index == 0)
                    queue_player_announcement(next, announcement::wrong_way);
                else
                    queue_opponent_announcement(whole, announcement::wrong_way);
            }
        }
        update_rolling_mode(rider, surface.mode != 0);
        if (index == 1)
            update_reflection_transition(
                rider, transition, horizontal, index != active, content.reflection_pose_table,
                state.native_initialization && (opponent_trick & 2U) != 0, tiles);
        // $82:98D6: the corkscrew's physics hold skips the whole drive routine,
        // brake latch included, and leaves $0E7B as the other rider set it.
        if (!tiles.physics_hold) {
            update_drive(rider, transition, horizontal, animation_override, throttle_target,
                         next.charge_announced[index], surface.leading_support != 0,
                         state.native_initialization && next.rolls[index].bounce_active != 0,
                         special.drive_step ? special.drive_step : 24,
                         tiles.mud_cooldown != 0 || special.crank_brake);
            next.drive_target_latch = throttle_target ? 0 : 1;
        }
        update_idle_pose(rider,
                         next.drive_target_latch && !surface.leading_support
                             && transition.pose_override == 0,
                         index == 1, whole.animation_counter, content.movement.idle_pose_table);
        if (rider.idle_pose.active) surface.tile_mode = 1;
        // $82:A971-A97C: the corkscrew's float or physics hold suspends gravity.
        if (!tiles.corkscrew_float && !tiles.physics_hold) update_gravity(rider);
        SpeedLimitContext limit{};
        limit.opponent = index == 1;
        limit.ai_enabled = true;
        // $150B has no writer in the declared continuation: preserve its seed
        // byte. Player boost below 16 makes the optional subtraction inert.
        if (index == 0 && rider.speed.boost >= 16 && !state.native_initialization)
            throw std::invalid_argument("ZOOM ZOO player boost requires unrecovered camera state");
        limit.pose_byte = index == 1 ? next.opponent_retained_oam_x
                                     : static_cast<std::uint8_t>(next.race.camera.screen_xy);
        limit.drag = state.complete_race
                  && (index == 0 ? next.race.provisional_1225 : next.race.provisional_1227);
        limit.start_override = rider.launch_override != 0;
        limit.player_progress = whole.riders[0].progress.transition_count;
        limit.opponent_progress = whole.riders[1].progress.transition_count;
        limit.adjustment_limit = race_adjustment_limit(scenario);
        // $82:A77E-A797: the opponent's cap rises by $1283 * 2 while the player
        // leads; $1283 is 64 on the HUNTER tour and 0 elsewhere (LOCKED-TOURS).
        limit.ai_adjustment = scenario.ai_adjustment;
        limit.player_base_cap = next.reflection[0].base_velocity_cap;
        limit.update_counter = whole.update_counter;
        limit.friction_mode = static_cast<std::uint16_t>(horizontal);
        // $82:A6FD: and the speed limiter.
        if (!tiles.physics_hold)
            limit_rider_speed(rider.motion.velocity_x, rider.motion.velocity_y, rider.speed, limit,
                              content.movement.speed_decay);
        integrate_zoom_axis(rider.motion.x, rider.motion.velocity_x, rider.residue_x);
        rider.motion.x &= track_geometry(content.movement.sampling.track).position_mask;
        integrate_zoom_axis(rider.motion.y, rider.motion.velocity_y, rider.residue_y);
        // $82:A6BB-A6F5: skipped on flag pair 8 ($0F2D).
        if (rider.contact.surface_angle && !special.slow_tile && rider.contact.unsupported_count < 2
            && !(rider.contact.selected_high & 0x80U)) {
            rider.motion.y = static_cast<std::uint16_t>(
                static_cast<int>(rider.motion.y)
                + (negative(rider.motion.velocity_y) ? -1 : (surface.mode ? 1 : 4)));
        }
        surface.animation_delta = static_cast<std::uint16_t>(animation_override);
        update_pose(rider, whole.animation_counter, whole.contact_phase, content.movement,
                    animation_override, next.drive_target_latch == 0,
                    surface.mode ? static_cast<std::int16_t>(surface.angle) : 0,
                    special.mud_velocity, special.slow_tile != 0);
        if (transition.pose_override) rider.pose.pose_index = transition.pose_override;
        if (index == active)
            advance_track_progress(rider.progress, content.movement.progress_transitions);
    }
    // $81:C73E-C75B: when the race clock would reach 10:00 it holds 9:59.9 and
    // marks both riders finished, whatever their laps.
    if (advance_timer_digits(whole.timer, whole.countdown < 68) && state.native_initialization)
        for (auto& rider : next.race.riders) rider.finished = 1;
    if (state.native_initialization)
        show_next_player_announcement(next, content.movement, content.captions);
    update_opponent_announcements(whole, reward, content.movement,
                                  state.native_initialization
                                      ? std::span<std::uint8_t>{next.learned_weights[1]}
                                      : std::span<std::uint8_t>{});
    if (state.complete_race)
        update_zoom_camera(next, track_geometry(content.movement.sampling.track));
    for (unsigned index = 0; index < 2; ++index) {
        // $81:8CFC / $81:8E43: while the corkscrew carries a rider its whole
        // contact update is skipped ($0DFB/$0DFD).
        if (contact_skip[index]) continue;
        auto& rider = whole.riders[index];
        const auto points = collision_points(content.movement.sampling, rider.pose.pose_index,
                                             rider.pose.reflected);
        const auto samples =
            sample_track(content.movement.sampling, points, rider.motion.x, rider.motion.y,
                         track_geometry(content.movement.sampling.track).coarse_columns);
        const auto summary = summarize_vertical_contact(content.movement.flat_contact, points,
                                                        samples, rider.motion.x, rider.motion.y);
        if (content.slope_coefficients.size() != 18 && content.slope_coefficients.size() != 128)
            throw std::invalid_argument("ZOOM ZOO slope coefficients missing");
        resolve_vertical_contact(
            rider.contact, rider.motion, summary,
            {whole.contact_phase, index == 1, next.surface[index].mode, 0xc200,
             next.special_tiles[index].loop_step == 9,
             index == 0 && next.hunter.effect[hunter_effect::power_bounce] != 0},
            content.slope_coefficients.subspan(state.sustained && next.surface[index].mode ? 64 : 0,
                                               state.sustained ? 32 : 9),
            content.slope_coefficients.subspan(
                state.sustained ? (next.surface[index].mode ? 96 : 32) : 9),
            content.landing_matrices,
            index == 0 ? whole.player_input.horizontal : next.opponent_horizontal,
            rider.pose.pose_index, rider.pose.reflected);
        // $8191F4-920C clears leading support on the auxiliary boundary
        // return, even when an earlier probe initially established support.
        next.surface[index].leading_support =
            rider.contact.auxiliary_flag == 1 ? false : summary.leading_support;
        if (state.native_initialization)
            next.rolls[index].support_count_mirror = rider.contact.unsupported_count;
        observe_track_markers(rider.progress, samples);
    }
    if (state.complete_race)
        update_zoom_visibility(next, track_geometry(content.movement.sampling.track));
    update_hunter_effects(next, content.hunter_blink);
    if (state.native_initialization) update_tutorial_hints(next);
    ++whole.frame;
    state = next;
}

} // namespace unirally
