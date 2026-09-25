// The HUNTER tour's tag and its eight timed effects (R-0052).
//
// On the HUNTER tour the player is "it": touching the opponent starts one of eight effects,
// chosen by the player's x & 7. The effect is announced at the front of the queue, runs
// for 500 updates (hedgehog speed runs its own course) and ends with a blank announcement;
// no new tag counts while it runs. `$12D1`, a palette mode that would replace the
// effects, is zero on every HUNTER race.

#include "hunter_effects.hpp"

#include "announcements.hpp"
#include "reward_queue.hpp"
#include "word_arithmetic.hpp"
#include "zoom_zoo_movement.hpp"

#include <array>
#include <cstdint>
#include <span>
#include <stdexcept>

namespace unirally {
namespace {

constexpr std::size_t blink_table_size = 64;
// An effect's timer counts 500 updates down to 0. Some effects blink over their opening
// 50 updates (timer 450 and up) and their closing ones (below 50, or 60 for invisible
// track), reading the blink table.
constexpr std::uint16_t effect_updates = 500, opening_start = 450;
constexpr std::uint16_t closing_updates = 50, invisible_track_closing_updates = 60;
// The tag boxes: x + 8 to x + 40 and y to y + 40.
constexpr std::uint16_t box_left = 8, box_width = 0x20, box_height = 0x28;
// Hedgehog speed's freezes grow to 20 skipped updates, then shrink by 2 to none.
constexpr std::uint16_t longest_freeze = 20;
// Screen flip turns the riders' sprites upside down: y becomes 0xE0 - (y + 0x40).
constexpr std::uint8_t flip_bottom = 0xe0, flip_sprite_height = 0x40;

// The priority in which a set effect flag runs ($83:CED9-D100) and each effect's
// announcement.
constexpr std::array<unsigned, hunter_effect::count> priority{
    hunter_effect::screen_flip,  hunter_effect::hedgehog_speed,  hunter_effect::slow_motion,
    hunter_effect::power_bounce, hunter_effect::barf_mode,       hunter_effect::invisible_track,
    hunter_effect::wobble_mode,  hunter_effect::control_reversed};
constexpr std::array<std::uint8_t, hunter_effect::count> announcement_of{
    announcement::barf_mode_on,   announcement::hedgehog_speed,  announcement::power_bounce_on,
    announcement::screen_flip_on, announcement::invisible_track, announcement::slow_motion_on,
    announcement::wobble_mode_on, announcement::control_reversed};

// The tag's comparisons read the N flag of a - b, a wrapping "a < b" on 16-bit words.
bool below(std::uint16_t a, std::uint16_t b) {
    return negative(static_cast<std::uint16_t>(a - b));
}

bool within(std::uint16_t v, std::uint16_t low, std::uint16_t high) {
    return !below(v, low) && below(v, high);
}

// $83:D104-D1C2: the tag counts only once the riders' progress counts have differed by 2
// or more (latched), and only when the opponent's box overlaps the player's.
bool tagged(HunterEffects& hunter, const std::array<RiderMovementState, 2>& riders) {
    if (!hunter.latched) {
        const auto apart = static_cast<std::uint16_t>(riders[0].progress.transition_count
                                                      - riders[1].progress.transition_count);
        if (!below(apart, 2) || below(apart, 0xffffU)) hunter.latched = 1;
    }
    if (!hunter.latched) return false;
    const auto px0 = static_cast<std::uint16_t>(riders[0].motion.x + box_left);
    const auto px1 = static_cast<std::uint16_t>(px0 + box_width);
    const auto py0 = riders[0].motion.y, py1 = static_cast<std::uint16_t>(py0 + box_height);
    const auto ox0 = static_cast<std::uint16_t>(riders[1].motion.x + box_left);
    const auto ox1 = static_cast<std::uint16_t>(ox0 + box_width);
    const auto oy0 = riders[1].motion.y, oy1 = static_cast<std::uint16_t>(oy0 + box_height);
    const bool x_hit = ox0 == px0 || within(ox0, px0, px1) || within(ox1, px0, px1);
    const bool y_hit = within(oy0, py0, py1) || within(oy1, py0, py1);
    return x_hit && y_hit;
}

// One update of an effect's 500-update timer; 0 is its last update.
std::uint16_t count_down(HunterEffects& hunter, unsigned effect) {
    auto left = hunter.timer[effect];
    if (!left) left = effect_updates;
    hunter.timer[effect] = --left;
    return left;
}

// The effect's end ($83:D275): the blank announcement, and the HUD's message buffers reset.
void finish(ZoomZooState& state, unsigned effect) {
    auto& hunter = state.hunter;
    hunter.message = announcement::effect_over;
    push_front_player_announcement(state, announcement::effect_over);
    hunter.hud_event = 0;
    hunter.effect[effect] = 0;
    hunter.active = 0;
}

// $83:D530-D580 (bytes): freezes of 1, 2 ... 20 skipped updates, then 18, 16 ... down to
// none, which ends the effect without an announcement.
void run_hedgehog_speed(HunterEffects& hunter) {
    if (hunter.pulse) {
        hunter.pulse = static_cast<std::uint16_t>(hunter.pulse - 1U);
        hunter.skip_update = 1;
    } else if (hunter.pulse_shrinking) {
        hunter.pulse = hunter.pulse_length =
            static_cast<std::uint16_t>((hunter.pulse_length - 2U) & 0xffU);
        if (!hunter.pulse_length) {
            hunter.pulse_shrinking = 0;
            hunter.effect[hunter_effect::hedgehog_speed] = 0;
            hunter.active = 0;
        }
    } else {
        hunter.pulse = hunter.pulse_length =
            static_cast<std::uint16_t>((hunter.pulse_length + 1U) & 0xffU);
        if (hunter.pulse_length == longest_freeze) hunter.pulse_shrinking = 1;
    }
}

// $83:D2C9-D348: the picture is flipped except where the blink table hides it over the
// opening and closing 50 updates; the table's sense is opposite at the two ends. Each
// update the flip shows alternates its scroll table.
void run_screen_flip(ZoomZooState& state, std::span<const std::uint8_t> blink) {
    auto& hunter = state.hunter;
    hunter.blink = 0;
    const auto left = count_down(hunter, hunter_effect::screen_flip);
    if (!left) finish(state, hunter_effect::screen_flip);
    if (!below(left, opening_start)) {
        if (blink[left - opening_start]) return;
    } else if (below(left, closing_updates) && !blink[left]) {
        return;
    }
    hunter.blink = 1;
    hunter.wave_phase = static_cast<std::uint16_t>(1U - hunter.wave_phase);
    // $83:D581-E081: either phase's scroll table ends at $83:E04F, which turns the riders'
    // sprites upside down. The player's sprite is the camera's published screen position.
    auto& screen = state.race.camera.screen_xy;
    const auto y = static_cast<std::uint8_t>(screen >> 8U);
    const auto flipped =
        static_cast<std::uint8_t>(flip_bottom - static_cast<std::uint8_t>(y + flip_sprite_height));
    screen = static_cast<std::uint16_t>((unsigned(flipped) << 8U) | (screen & 0xffU));
}

// $83:D3FC-D473 (invisible track: BG1 off) and $83:D349-D3BB (wobble mode: mosaic): on
// except where the blink table turns it off over the opening 50 and the closing 60
// (invisible track) or 50 updates; again the table's sense is opposite at the two ends.
void run_blinking_effect(ZoomZooState& state, unsigned effect,
                         std::span<const std::uint8_t> blink) {
    auto& hunter = state.hunter;
    auto& on = effect == hunter_effect::invisible_track ? hunter.hide_track : hunter.mosaic;
    on = 1;
    const auto left = count_down(hunter, effect);
    if (!left) {
        finish(state, effect);
        on = 0;
        return;
    }
    const auto closing = effect == hunter_effect::invisible_track ? invisible_track_closing_updates
                                                                  : closing_updates;
    if (!below(left, opening_start)) {
        if (!blink[left - opening_start]) on = 0;
    } else if (below(left, closing) && blink[left]) {
        on = 0;
    }
}

// $83:D4E8-D52F: slow motion skips three updates in four.
void run_slow_motion(ZoomZooState& state) {
    if (!count_down(state.hunter, hunter_effect::slow_motion))
        finish(state, hunter_effect::slow_motion);
    if ((state.hunter.timer[hunter_effect::slow_motion] & 3U) != 3U) state.hunter.skip_update = 1;
}

void run_effect(ZoomZooState& state, unsigned effect, std::span<const std::uint8_t> blink) {
    switch (effect) {
    case hunter_effect::hedgehog_speed: run_hedgehog_speed(state.hunter); break;
    case hunter_effect::screen_flip: run_screen_flip(state, blink); break;
    case hunter_effect::invisible_track:
    case hunter_effect::wobble_mode: run_blinking_effect(state, effect, blink); break;
    case hunter_effect::slow_motion: run_slow_motion(state); break;
    default:
        // $83:D474 (barf mode), $83:D4AE (power bounce), $83:D28A (control reversed): the
        // timer alone; their consumers read the running flag.
        if (!count_down(state.hunter, effect)) finish(state, effect);
        break;
    }
}

} // namespace

// $83:CEC9-D600 (R-0052), at the end of every update ($83:CDAA), skipped ones included.
void update_hunter_effects(ZoomZooState& state, std::span<const std::uint8_t> blink) {
    if (!classic_race_scenario(state.track).hunter_tour) return;
    if (blink.size() != blink_table_size)
        throw std::invalid_argument("HUNTER blink pattern is missing");
    auto& hunter = state.hunter;
    if (!hunter.active && !state.race.riders[0].finished && !state.race.riders[1].finished
        && tagged(hunter, state.movement.riders)) {
        hunter.effect[state.movement.riders[0].motion.x & 7U] = 1;
        hunter.active = 1;
    }
    // The first set flag in priority order runs and clears the others. On the update it
    // starts it is announced at the front of the queue ($81:C55B), and every effect but
    // hedgehog speed names its HUD message ($12AF). Sound $021F is not played.
    for (const auto effect : priority) {
        if (!hunter.effect[effect]) continue;
        if (hunter.effect[effect] == 1) {
            hunter.effect[effect] = 2;
            push_front_player_announcement(state, announcement_of[effect]);
            if (effect != hunter_effect::hedgehog_speed) hunter.message = announcement_of[effect];
        }
        for (unsigned other = 0; other < hunter_effect::count; ++other)
            if (other != effect) hunter.effect[other] = 0;
        run_effect(state, effect, blink);
        return;
    }
}

// $80:8821-882A: the race NMI that opens each update counts $0563 while the wobble mode
// mosaic the previous update left is on.
void count_hunter_mosaic(ZoomZooState& state) {
    if (state.hunter.mosaic)
        state.hunter.mosaic_counter =
            static_cast<std::uint16_t>((state.hunter.mosaic_counter + 1U) & 0xffU);
}

} // namespace unirally
