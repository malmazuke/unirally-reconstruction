// The gold endings' scripts of JUMPER, BOUNDER, RUNNER and SPRINTER, PICK TOUR's right column
// (R-0062), one namespace a tour: each runs on every frame from `bar_frame` to its t', frames
// counted from the completion's scoring frame (tour_ending.cpp has the scheme around them).
#include "tour_ending_scripts.hpp"

#include <algorithm>
#include <array>
#include <cstdint>

namespace unirally::front_end_screens::ending {

// JUMPER (`$83:BB80`): the riderless uni and the rider's red uni ride in from the left while the
// gold pattern scrolls past; an elephant drops onto the uni and flattens it, then flies off, and
// the red uni rides on. Both unis show the pose at tile 0x100 in their own palettes until the
// flattened uni takes tile 0x108. Its table (`$83:BE9A`): objects 0-2, the elephant's tiles.
namespace jumper {
namespace {

constexpr unsigned tour = 1;
constexpr std::size_t elephant_tiles_at = 12, squashed_tiles = 4;
constexpr unsigned first_objects = 3, uni = 0, red_uni = 1, elephant = 2;
constexpr std::uint8_t first_high = 0x5b, red_uni_high = 0x1a, elephant_high = 0x4a,
                       flattened_high = 0x49, flattened_attr = 0x49, squashed_attr = 0x13,
                       second_pose_tile = 8, uni_speed = 3, elephant_speed = 2, ride_off_speed = 2;
constexpr std::uint16_t red_uni_shown = 0x15, squash_poses = 0x13dc, ride_poses = 0x38,
                        walk_poses = 23, pose_stride = 0x40, scroll_step = 8;
constexpr std::uint32_t setup_frame = 96;
// $83:BC62, $83:BCB2, $83:BD16, $83:BD94, $83:BDDD and $83:BE2C (which ends on t').
constexpr Loop ride_in{111, 64, 1}, drop{ride_in.end(), 55, 1}, squash{drop.end(), 16, 2},
    flatten{squash.end(), 8, 1}, fly_off{flatten.end(), 55, 1}, ride_off{fly_off.end(), 84, 1};

// $83:BEBA: the gold pattern scrolls 8 pixels: `$0090`, the slides' scroll word, to BG1.
void scroll_pattern(FrontEndState& state) {
    state.slide.scroll = static_cast<std::uint16_t>(state.slide.scroll + scroll_step);
    state.registers.bg[0].hofs = state.slide.scroll;
}

// $83:BE6F: the ride's poses from 0x38, a new one every step, 23 in turn.
void build_ride_pose(FrontEndState& state) {
    auto& ending = state.ending;
    ending.pose =
        static_cast<std::uint16_t>((((ending.step * 2U) % walk_poses) * pose_stride) + ride_poses);
}

// Every step's end: the built pose to `word`, the objects, the scroll, the next ride pose.
void ride_step(FrontEndState& state, const FrontEndContent& content, unsigned word) {
    upload_built_pose(state, content, word);
    copy_oam(state);
    scroll_pattern(state);
    build_ride_pose(state);
}

void set_elephant_tile(FrontEndState& state, const FrontEndContent& content, std::size_t index) {
    oam_byte(state, elephant, 2) =
        table_byte(content.ending_tables[tour], elephant_tiles_at + index);
}

void setup(FrontEndState& state, const FrontEndContent& content) {
    load_first_objects(state, content.ending_tables[tour], 0, first_objects);
    high_bits(state, 0) = first_high;
    copy_oam(state);
    state.ending.pose = ride_poses;
    upload_built_pose(state, content, first_pose_word);
    upload_built_pose(state, content, second_pose_word);
    state.ending.step = 0;
}

// $83:BC62-BC9F: the uni rides in 3 pixels a step, the red uni 1, shown from step 0x15.
void ride_in_part(FrontEndState& state, const FrontEndContent& content, unsigned, unsigned wait) {
    if (wait == 1) {
        ride_step(state, content, first_pose_word);
        ++state.ending.step;
        return;
    }
    oam_byte(state, uni, 0) += uni_speed;
    ++oam_byte(state, red_uni, 0);
    if ((state.ending.step & 0xffU) == red_uni_shown) high_bits(state, 0) = red_uni_high;
}

// $83:BCA1-BCFA: the elephant drops in from the top left, walking (`$77:10CB` its step).
void drop_part(FrontEndState& state, const FrontEndContent& content, unsigned step, unsigned wait) {
    auto& ending = state.ending;
    if (wait == 1) {
        ride_step(state, content, first_pose_word);
        ++ending.count;
        ++ending.step;
        return;
    }
    if (step == 0) {
        ending.step = ending.count = 0;
        high_bits(state, 0) = elephant_high; // a word write: entries 4-7 shown, small
        high_bits(state, 4) = four_shown;
    }
    oam_byte(state, elephant, 0) += elephant_speed;
    oam_byte(state, elephant, 1) += elephant_speed;
    set_elephant_tile(state, content, ((ending.count & 0xffU) >> 1) & 3U);
}

// $83:BCFC-BD7E: the elephant lands on the uni, which is squashed (poses 0x13DC on, at tile
// 0x108) while the red uni rides on; the first frame of each pair copies no objects.
void squash_part(FrontEndState& state, const FrontEndContent& content, unsigned step,
                 unsigned wait) {
    auto& ending = state.ending;
    if (wait == 1) {
        upload_built_pose(state, content, second_pose_word);
        scroll_pattern(state);
        ending.pose = static_cast<std::uint16_t>(squash_poses + (ending.step >> 1));
        return;
    }
    if (wait == 2) {
        ride_step(state, content, first_pose_word);
        ++ending.count;
        ++ending.step;
        return;
    }
    if (step == 0) {
        ending.step = ending.count = 0;
        oam_byte(state, red_uni, 2) = second_pose_tile;
        oam_byte(state, uni, 3) = squashed_attr;
    }
    set_elephant_tile(state, content, (ending.count >> 1) + squashed_tiles);
}

// $83:BD81-BDD4: the flattened uni hidden; the red uni's ride goes to tile 0x108.
void flatten_part(FrontEndState& state, const FrontEndContent& content, unsigned step,
                  unsigned wait) {
    auto& ending = state.ending;
    if (wait == 1) {
        ride_step(state, content, second_pose_word);
        ++ending.count;
        ++ending.step;
        return;
    }
    if (step == 0) {
        ending.step = 0;
        high_bits(state, 0) = flattened_high;
        oam_byte(state, uni, 3) = flattened_attr;
    }
    set_elephant_tile(state, content, (ending.count >> 1) + squashed_tiles);
}

// $83:BDD6-BE1C: the elephant flies off to the top right.
void fly_off_part(FrontEndState& state, const FrontEndContent& content, unsigned step,
                  unsigned wait) {
    auto& ending = state.ending;
    if (wait == 1) {
        ride_step(state, content, second_pose_word);
        ++ending.step;
        return;
    }
    if (step == 0) ending.step = 0;
    oam_byte(state, elephant, 0) += elephant_speed;
    oam_byte(state, elephant, 1) -= elephant_speed;
    set_elephant_tile(state, content, ((ending.step & 0xffU) >> 1) & 3U);
}

// $83:BE1E-BE62: the red uni rides off to the right on tile 0x100; on t' the scroll goes back
// to 0.
void ride_off_part(FrontEndState& state, const FrontEndContent& content, unsigned step,
                   unsigned wait) {
    auto& ending = state.ending;
    if (wait == 1) {
        ride_step(state, content, first_pose_word);
        ++ending.step;
        if (step + 1 < ride_off.steps) return;
        state.slide.scroll = 0;
        state.registers.bg[0].hofs = 0;
        return;
    }
    if (step == 0) {
        ending.step = 0;
        oam_byte(state, red_uni, 2) = 0;
    }
    oam_byte(state, red_uni, 0) += ride_off_speed;
}

} // namespace

void script(FrontEndState& state, const FrontEndContent& content, std::uint32_t frame) {
    if (frame == setup_frame) {
        setup(state, content);
        return;
    }
    run_loop(ride_in, frame, state, content, ride_in_part);
    run_loop(drop, frame, state, content, drop_part);
    run_loop(squash, frame, state, content, squash_part);
    run_loop(flatten, frame, state, content, flatten_part);
    run_loop(fly_off, frame, state, content, fly_off_part);
    run_loop(ride_off, frame, state, content, ride_off_part);
}
} // namespace jumper

// BOUNDER (`$83:B7D2`): the riderless uni rides in from the right and the rider's red uni from
// the left; the red uni jumps, bumps a block and a ball pops out and falls away; the red uni,
// its scarf flying, bounds off the top of the screen while the uni sinks down. Both unis show
// the pose at tile 0x100 until the red uni takes tile 0x108; the uni's last poses are built in
// the second buffer (`$16F0`). Its table (`$83:BB44`): objects 0-5, then the bump's seven steps
// of five bytes (the red uni's x and y, the ball's x and y, the block's y).
namespace bounder {
namespace {

constexpr unsigned tour = 3;
constexpr std::size_t bump_steps_at = 24, bump_step_bytes = 5;
constexpr unsigned first_objects = 6, uni = 0, red_uni = 1, bumped_block = 4, ball = 5;
constexpr std::uint8_t first_high = 0x0e, blocks_high = 0x50, red_uni_high = 0x0a,
                       ball_gone_high = 0x54, second_pose_tile = 8, rise = 3, ride_speed = 2,
                       bound_speed = 3;
constexpr std::uint16_t red_uni_shown = 0x17, red_uni_rises = 0x21, ball_falls_from = 7,
                        scarf_poses = 0x1384, sink_poses = 0x13d4, bound_rise = 0x0c,
                        second_bound = 0x15, last_sink_pose = 7;
constexpr std::uint32_t setup_frame = 93;
// $83:B8B9, $83:B925, $83:B9A7, $83:BA26 and $83:BA87 (which ends on t').
constexpr Loop ride_in{108, 38, 1}, bump{ride_in.end(), 7, 1}, ball_falls{bump.end(), 14, 1},
    scarf{ball_falls.end(), 18, 2}, bound_off{scarf.end(), 48, 2};

void setup(FrontEndState& state, const FrontEndContent& content) {
    load_first_objects(state, content.ending_tables[tour], 0, first_objects);
    high_bits(state, 0) = first_high;
    high_bits(state, bumped_block) = blocks_high;
    copy_oam(state);
    state.ending.pose = 0;
    upload_built_pose(state, content, first_pose_word);
    upload_built_pose(state, content, second_pose_word);
    state.ending.step = 0;
}

// The steps of the first three loops end alike: the walk's pose to tile 0x100, the objects,
// the next walk pose.
void walk_step(FrontEndState& state, const FrontEndContent& content) {
    upload_built_pose(state, content, first_pose_word);
    copy_oam(state);
    walk(state);
    ++state.ending.step;
}

// $83:B8B9-B915: the unis ride towards each other, the red uni shown from step 0x17; from
// step 0x21 it rises, jumping.
void ride_in_part(FrontEndState& state, const FrontEndContent& content, unsigned, unsigned wait) {
    if (wait == 1) {
        walk_step(state, content);
        if (state.ending.step >= red_uni_rises) oam_byte(state, red_uni, 1) -= rise;
        return;
    }
    oam_byte(state, uni, 0) -= ride_speed;
    oam_byte(state, red_uni, 0) += ride_speed;
    if ((state.ending.step & 0xffU) == red_uni_shown) high_bits(state, 0) = red_uni_high;
}

// $83:B91E-B99A: the red uni hits the block, which bumps up, and the ball pops out.
void bump_part(FrontEndState& state, const FrontEndContent& content, unsigned step, unsigned wait) {
    if (wait == 1) {
        walk_step(state, content);
        return;
    }
    if (step == 0) state.ending.step = 0;
    const auto table = content.ending_tables[tour];
    const auto at = bump_steps_at + bump_step_bytes * (state.ending.step & 0xffU);
    oam_byte(state, red_uni, 0) = table_byte(table, at);
    oam_byte(state, red_uni, 1) = table_byte(table, at + 1);
    oam_byte(state, ball, 0) = table_byte(table, at + 2);
    oam_byte(state, ball, 1) = table_byte(table, at + 3);
    oam_byte(state, bumped_block, 1) = table_byte(table, at + 4);
}

// $83:B99C-BA0E: the ball rolls right and, from step 7, falls faster and faster (`$77:10A7`).
void ball_falls_part(FrontEndState& state, const FrontEndContent& content, unsigned step,
                     unsigned wait) {
    auto& ending = state.ending;
    if (wait == 1) {
        walk_step(state, content);
        if (ending.step >= ball_falls_from) ++ending.drop;
        return;
    }
    if (step == 0) ending.step = ending.drop = 0;
    ++oam_byte(state, red_uni, 0);
    ++oam_byte(state, ball, 0);
    oam_byte(state, ball, 1) += static_cast<std::uint8_t>(ending.drop);
}

// $83:BA10-BA6A: the ball gone, the red uni takes tile 0x108 and its scarf flies (poses 0x1384
// on, a new one every second step).
void scarf_part(FrontEndState& state, const FrontEndContent& content, unsigned step,
                unsigned wait) {
    auto& ending = state.ending;
    if (wait == 0 && step == 0) {
        ending.step = 0;
        high_bits(state, bumped_block) = ball_gone_high;
        oam_byte(state, red_uni, 2) = second_pose_tile;
    }
    if (wait == 2) {
        upload_built_pose(state, content, second_pose_word);
        copy_oam(state);
        ending.pose = static_cast<std::uint16_t>(scarf_poses + (ending.step >> 1));
        ++ending.step;
        return;
    }
    if (wait == 1) copy_oam(state);
    ++oam_byte(state, red_uni, 0);
}

// $83:BAED-BB36 after each step of the bound: at step 0x15 the red uni bounds again, and from it
// the uni's sinking poses (0x13D4 on, held at the eighth) are built in the second buffer.
void after_bound_step(FrontEndState& state) {
    auto& ending = state.ending;
    set_low_byte(ending.drop, ending.drop - 1U);
    ++ending.step;
    if (ending.step < second_bound || ending.step >= bound_off.steps) return;
    if (ending.step == second_bound) set_low_byte(ending.drop, bound_rise);
    ending.second_pose = static_cast<std::uint16_t>(sink_poses + ending.count);
    if ((ending.count & 0xffU) < last_sink_pose) set_low_byte(ending.count, ending.count + 1U);
}

// $83:BA6C-BB25: the red uni bounds off to the top right, rising by `$77:10A7` (a byte, rounded
// down to even) as it counts down; the uni's second picture goes to tile 0x100 once built
// (`$83:AB25`).
void bound_off_part(FrontEndState& state, const FrontEndContent& content, unsigned step,
                    unsigned wait) {
    auto& ending = state.ending;
    if (wait == 1) {
        copy_oam(state);
        if (ending.count & 0xffU) upload_pose(state, content, ending.second_pose, first_pose_word);
        return;
    }
    if (wait == 2) {
        upload_built_pose(state, content, second_pose_word);
        copy_oam(state);
        ending.pose = static_cast<std::uint16_t>(scarf_poses + ((ending.step & 0xfU) >> 1));
        after_bound_step(state);
        return;
    }
    if (step == 0) {
        ending.step = ending.count = 0;
        set_low_byte(ending.drop, bound_rise);
        oam_byte(state, red_uni, 2) = second_pose_tile;
    }
    oam_byte(state, red_uni, 0) += bound_speed;
    oam_byte(state, red_uni, 1) -= static_cast<std::uint8_t>(ending.drop & 0xfeU);
}

} // namespace

void script(FrontEndState& state, const FrontEndContent& content, std::uint32_t frame) {
    if (frame == setup_frame) {
        setup(state, content);
        return;
    }
    run_loop(ride_in, frame, state, content, ride_in_part);
    run_loop(bump, frame, state, content, bump_part);
    run_loop(ball_falls, frame, state, content, ball_falls_part);
    run_loop(scarf, frame, state, content, scarf_part);
    run_loop(bound_off, frame, state, content, bound_off_part);
}
} // namespace bounder

// RUNNER (`$83:C715`): a riderless uni walks in from the right and a ten-ton weight drops on it.
// Its table (`$83:C89A`): objects 0-3.
namespace runner {
namespace {

constexpr unsigned tour = 5;
constexpr unsigned first_objects = 4, weight = 0, uni = 1;
constexpr std::uint8_t first_high = 0x28, squashed_high = 0x2c, drop_speed = 8;
constexpr std::uint32_t objects_frame = 93, poses_frame = 94;
constexpr std::uint16_t weight_drops = 0x89;
// $83:C7F7, then 61 waits each with an OAM copy (`$83:C886`) to t'.
constexpr Loop walk_in{109, 151, 1}, hold{walk_in.end(), 61, 1};

void setup_objects(FrontEndState& state, const FrontEndContent& content) {
    load_first_objects(state, content.ending_tables[tour], 0, first_objects);
    high_bits(state, 0) = first_high;
    copy_oam(state);
}

void setup_poses(FrontEndState& state, const FrontEndContent& content) {
    state.ending.pose = 0;
    upload_built_pose(state, content, first_pose_word);
    upload_built_pose(state, content, second_pose_word);
    state.ending.step = 0;
}

// $83:C7F7-C86B: the uni walks a pixel left; from step 0x89 the weight drops.
void walk_in_part(FrontEndState& state, const FrontEndContent& content, unsigned, unsigned wait) {
    auto& ending = state.ending;
    if (wait == 0) {
        --oam_byte(state, uni, 0);
        return;
    }
    upload_built_pose(state, content, first_pose_word);
    copy_oam(state);
    walk(state); // $83:C80C-C83E, a copy of `$83:B79C`
    ++ending.step;
    if (ending.step >= weight_drops) oam_byte(state, weight, 1) += drop_speed;
}

// $83:C86E-C88E: the weight lands (back up a step) and the uni under it is hidden.
void hold_part(FrontEndState& state, const FrontEndContent&, unsigned step, unsigned wait) {
    if (wait == 1) {
        copy_oam(state);
        return;
    }
    if (step > 0) return;
    oam_byte(state, weight, 1) -= drop_speed;
    high_bits(state, 0) = squashed_high;
}

} // namespace

void script(FrontEndState& state, const FrontEndContent& content, std::uint32_t frame) {
    if (frame == objects_frame) setup_objects(state, content);
    if (frame == poses_frame) setup_poses(state, content);
    run_loop(walk_in, frame, state, content, walk_in_part);
    run_loop(hold, frame, state, content, hold_part);
}
} // namespace runner

// SPRINTER (`$83:BED0`): a riderless uni walks in from the right and wobbles; the rider's red
// uni cartwheels in from the left and knocks it tumbling away. The knocked uni's poses are built
// in the second buffer (`$16F0`). No object tiles are loaded: the menus' stay. Its table
// (`$83:C0F6`): objects 0-1, then the wobble's sixteen pose words; the objects' copy reads twelve
// bytes, so entry 2 is the first two pose words (hidden).
namespace sprinter {
namespace {

constexpr unsigned tour = 7;
constexpr std::size_t wobble_poses_at = 8;
constexpr unsigned first_objects = 3, uni = 0, red_uni = 1;
constexpr std::uint8_t first_high = 0x5e, red_uni_high = 0x4a, cartwheel_speed = 4;
constexpr std::uint32_t objects_frame = bar_frame, second_pose_frame = 93;
constexpr std::uint16_t red_uni_shown = 0x0b, cartwheel_poses = 0x13b5, cartwheel_pose_count = 17,
                        knocked_from = 0x1b, first_tumble_rise = 0xfff4;
// $83:BFA5, $83:BFE2 and $83:C034 (which ends on t').
constexpr Loop walk_in{108, 144, 1}, wobble{walk_in.end(), 17, 2}, knock{wobble.end(), 70, 1};

void setup_objects(FrontEndState& state, const FrontEndContent& content) {
    load_first_objects(state, content.ending_tables[tour], 0, first_objects);
    high_bits(state, 0) = first_high;
    copy_oam(state);
    state.ending.pose = 0;
    upload_built_pose(state, content, first_pose_word);
}

// $83:BFA5-BFD0: the uni walks a pixel left.
void walk_in_part(FrontEndState& state, const FrontEndContent& content, unsigned, unsigned wait) {
    if (wait == 0) {
        --oam_byte(state, uni, 0);
        return;
    }
    upload_built_pose(state, content, first_pose_word);
    copy_oam(state);
    walk(state);
    ++state.ending.step;
}

// $83:BFD2-C020: the uni wobbles (the table's sixteen poses in turn), a pose every second frame.
void wobble_part(FrontEndState& state, const FrontEndContent& content, unsigned step,
                 unsigned wait) {
    auto& ending = state.ending;
    if (wait == 0 && step == 0) ending.step = ending.count = 0;
    if (wait != 2) return;
    upload_built_pose(state, content, first_pose_word);
    copy_oam(state);
    const auto at = wobble_poses_at + 2U * (ending.step & 0xfU);
    const auto table = content.ending_tables[tour];
    ending.pose =
        static_cast<std::uint16_t>(table_byte(table, at) | table_byte(table, at + 1) << 8U);
    ++ending.step;
}

// $83:C0BD-C0C9: the knocked uni's rise or fall for a step: `$77:10A7`'s low byte as a signed
// byte divided by four (two shifts right, the sign copied back from bit 5).
std::uint8_t tumble_step(std::uint16_t rise) {
    const auto quarter = static_cast<std::uint8_t>((rise & 0xffU) >> 2);
    return (quarter & 0x20U) ? static_cast<std::uint8_t>(quarter | 0xc0U) : quarter;
}

// $83:C022-C0E7: the red uni cartwheels in (poses 0x13B5 counted down, seventeen in turn, at
// tile 0x108), shown from step 0x0B; from step 0x1B the uni is knocked away, rising then falling
// by `$77:10A7`, its poses from 0 on in the second buffer, sent to tile 0x100 every step
// (`$83:AB25`). `$77:10CB` counts the knocked steps to 0x2C.
void knock_part(FrontEndState& state, const FrontEndContent& content, unsigned step,
                unsigned wait) {
    auto& ending = state.ending;
    if (wait == 0) {
        if (step == 0) {
            ending.step = ending.count = 0;
            ending.drop = first_tumble_rise;
        }
        oam_byte(state, red_uni, 0) += cartwheel_speed;
        if ((ending.step & 0xffU) == red_uni_shown) high_bits(state, 0) = red_uni_high;
        return;
    }
    upload_built_pose(state, content, second_pose_word);
    upload_pose(state, content, ending.second_pose, first_pose_word);
    copy_oam(state);
    ending.pose = static_cast<std::uint16_t>(cartwheel_poses - ending.step % cartwheel_pose_count);
    ending.second_pose = ending.count;
    ++ending.step;
    if (ending.step < knocked_from) return;
    ++oam_byte(state, uni, 0);
    oam_byte(state, uni, 1) += tumble_step(ending.drop);
    ++ending.drop;
    ++ending.count;
}

} // namespace

void script(FrontEndState& state, const FrontEndContent& content, std::uint32_t frame) {
    if (frame == objects_frame) setup_objects(state, content);
    if (frame == second_pose_frame) state.ending.second_pose = 0; // $83:BF89-BF93
    run_loop(walk_in, frame, state, content, walk_in_part);
    run_loop(wobble, frame, state, content, wobble_part);
    run_loop(knock, frame, state, content, knock_part);
}
} // namespace sprinter

} // namespace unirally::front_end_screens::ending
