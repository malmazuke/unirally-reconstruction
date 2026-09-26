// The gold endings' scripts of the tours after CRAWLER (R-0062), one
// namespace a tour: each runs on every frame from `bar_frame` to its t', frames counted from the
// completion's scoring frame (tour_ending.cpp has the scheme around them and CRAWLER's).
#include "tour_ending_scripts.hpp"

#include <algorithm>
#include <array>
#include <cstdint>

namespace unirally::front_end_screens::ending {

// SHUFFLER (`$83:B1EB`): a riderless uni rolls in from the right with a turtle walking behind
// it; the rider's red uni drops in and bounces away; the turtle's tongue reaches out, pulls the
// uni over and swallows it. Its table (`$83:B4DF`): objects 0-3, objects 4-7 (the turtle
// eating, four quarters), the walking turtle's four tiles, the eating turtle's three.
namespace shuffler {
namespace {

constexpr unsigned tour = 2;
constexpr std::size_t first_objects_at = 0, eating_objects_at = 16, turtle_tiles_at = 32,
                      eating_tiles_at = 36;
constexpr unsigned first_objects = 4, turtle_top = 0, uni = 1, red_uni = 2, turtle_bottom = 3,
                   eating_turtle = 4;
constexpr std::uint8_t first_high = 0x28, eating_high = 0x69, swallowed_high = 0x55;
constexpr std::uint32_t setup_frame = 95;
// $83:B2CD, $83:B388, $83:B3FF, $83:B44F and $83:B496, then 61 waits to t'.
constexpr Loop roll{110, 151, 1}, bounce{roll.end(), 9, 2}, tongue_out{bounce.end(), 6, 2},
    pull_over{tongue_out.end(), 8, 2}, swallow{pull_over.end(), 6, 2};
constexpr std::uint16_t turtle_starts = 0x1d, red_uni_drops = 0x89, bounce_poses = 0x621,
                        pull_poses = 0x1390, walk_poses = 23, pose_stride = 0x40;
constexpr std::uint8_t lower_tile_row = 0x20, bounce_height = 8, ground_y = 0x69,
                       pulled_attr = 0x13, pulled_x = 0x57, last_tongue_step = 5;
// The eating turtle's four quarters: tiles t, t + 1, t + 0x10 and t + 0x11.
constexpr std::array<std::uint8_t, 4> quarter_tiles{0x00, 0x01, 0x10, 0x11};

void setup(FrontEndState& state, const FrontEndContent& content) {
    load_first_objects(state, content.ending_tables[tour], first_objects_at, first_objects);
    high_bits(state, 0) = first_high;
    copy_oam(state);
    state.ending.pose = 0;
    upload_built_pose(state, content, first_pose_word);
    upload_built_pose(state, content, second_pose_word);
    state.ending.step = 0;
}

// $83:B2CD: the uni rolls a pixel left; from step 0x1D the turtle follows, walking.
void roll_start(FrontEndState& state, const FrontEndContent& content) {
    --oam_byte(state, uni, 0);
    const auto step = state.ending.step;
    if (step < turtle_starts) return;
    --oam_byte(state, turtle_top, 0);
    --oam_byte(state, turtle_bottom, 0);
    const auto tile = table_byte(content.ending_tables[tour], turtle_tiles_at + ((step & 6U) >> 1));
    oam_byte(state, turtle_top, 2) = tile;
    oam_byte(state, turtle_bottom, 2) = static_cast<std::uint8_t>(tile + lower_tile_row);
}

// $83:B301: the wheel turns backwards: the walk's poses counted down from step 0x100.
void roll_end(FrontEndState& state, const FrontEndContent& content) {
    auto& ending = state.ending;
    upload_built_pose(state, content, first_pose_word);
    copy_oam(state);
    ending.pose =
        static_cast<std::uint16_t>((((0x100U - ending.step) >> 1) % walk_poses) * pose_stride);
    ++ending.step;
    if (ending.step >= red_uni_drops) oam_byte(state, red_uni, 1) += bounce_height;
}

void roll_part(FrontEndState& state, const FrontEndContent& content, unsigned, unsigned wait) {
    wait == 0 ? roll_start(state, content) : roll_end(state, content);
}

// $83:B374-B3D7: the red uni is put back on the ground and bounces up out of sight while the
// uni's poses 0x621 on play.
void bounce_part(FrontEndState& state, const FrontEndContent& content, unsigned step,
                 unsigned wait) {
    auto& ending = state.ending;
    auto& red_uni_y = oam_byte(state, red_uni, 1);
    if (wait == 0) {
        if (step == 0) {
            ending.step = 0;
            red_uni_y = ground_y;
        }
        red_uni_y -= bounce_height;
    } else if (wait == 1) {
        red_uni_y -= bounce_height;
        copy_oam(state);
    } else {
        upload_built_pose(state, content, first_pose_word);
        copy_oam(state);
        ending.pose = static_cast<std::uint16_t>(bounce_poses + ending.step);
        ++ending.step;
    }
}

// $83:B401 and $83:B498: the eating turtle's four tiles, from its tile for the step.
void set_eating_tiles(FrontEndState& state, const FrontEndContent& content) {
    const auto tile = table_byte(content.ending_tables[tour],
                                 eating_tiles_at + ((state.ending.step & 0xffU) >> 1));
    for (unsigned k = 0; k < quarter_tiles.size(); ++k)
        oam_byte(state, eating_turtle + k, 2) = static_cast<std::uint8_t>(tile + quarter_tiles[k]);
}

// $83:B3DB-B434: the eating turtle is shown in place of the walking one (and the red uni hidden)
// and its tongue comes out.
void tongue_out_part(FrontEndState& state, const FrontEndContent& content, unsigned step,
                     unsigned wait) {
    if (wait == 1) return;
    if (wait == 2) {
        copy_oam(state);
        ++state.ending.step;
        return;
    }
    if (step == 0) {
        const auto table = content.ending_tables[tour];
        for (std::size_t k = 0; k < quarter_tiles.size() * oam_entry_bytes; ++k)
            state.oam_buffer[eating_turtle * oam_entry_bytes + k] =
                table_byte(table, eating_objects_at + k);
        high_bits(state, 0) = eating_high;
        high_bits(state, eating_turtle) = four_shown;
        state.ending.step = 0;
    }
    set_eating_tiles(state, content);
}

// $83:B436-B486: the tongue pulls the uni over (poses 0x1390 on), turned and moved to it.
void pull_over_part(FrontEndState& state, const FrontEndContent& content, unsigned step,
                    unsigned wait) {
    auto& ending = state.ending;
    if (wait == 1) return;
    if (wait == 2) {
        upload_built_pose(state, content, first_pose_word);
        copy_oam(state);
        ++ending.step;
        return;
    }
    if (step == 0) {
        ending.step = 0;
        oam_byte(state, uni, 3) = pulled_attr;
        oam_byte(state, uni, 0) = pulled_x;
        --oam_byte(state, uni, 1);
    }
    ending.pose = static_cast<std::uint16_t>(pull_poses + ending.step);
}

// $83:B488-B4C8: the uni swallowed (objects 0-3 hidden), the tongue goes back, its tiles counted
// down from step 5.
void swallow_part(FrontEndState& state, const FrontEndContent& content, unsigned step,
                  unsigned wait) {
    if (wait == 1) return;
    if (wait == 2) {
        copy_oam(state);
        --state.ending.step;
        return;
    }
    if (step == 0) {
        state.ending.step = last_tongue_step;
        high_bits(state, 0) = swallowed_high;
    }
    set_eating_tiles(state, content);
}

} // namespace

void script(FrontEndState& state, const FrontEndContent& content, std::uint32_t frame) {
    if (frame == setup_frame) {
        setup(state, content);
        return;
    }
    run_loop(roll, frame, state, content, roll_part);
    run_loop(bounce, frame, state, content, bounce_part);
    run_loop(tongue_out, frame, state, content, tongue_out_part);
    run_loop(pull_over, frame, state, content, pull_over_part);
    run_loop(swallow, frame, state, content, swallow_part);
}
} // namespace shuffler

// WALKER (`$83:B506`): a riderless uni walks in from the right; an arrow flies in, hits it and
// drops, and the uni falls apart; the rider's red uni rolls in from the left and on across the
// screen. Its table (`$83:B790`): objects 0-2.
namespace walker {
namespace {

constexpr unsigned tour = 4;
constexpr unsigned first_objects = 3, uni = 0, red_uni = 1, arrow = 2;
constexpr std::uint8_t first_high = 0x4e, red_uni_high = 0x4a;
constexpr std::uint32_t setup_frame = 93;
// $83:B5ED, $83:B64D, $83:B6B6 and $83:B712 (which ends on t').
constexpr Loop walk_in{108, 128, 1}, hit{walk_in.end(), 32, 2}, red_uni_in{hit.end(), 25, 1},
    roll_on{red_uni_in.end(), 68, 1};
constexpr std::uint16_t arrow_flies = 0x70, fall_poses = 0xa5e, red_uni_shown = 0x0f,
                        wobble_poses = 0x13b8, roll_poses = 0x13c0, roll_count = 0x43,
                        held_step = 0x27, held_pose = 6;
constexpr std::uint8_t arrow_speed = 7, arrow_drift = 2, red_uni_speed = 3;

void setup(FrontEndState& state, const FrontEndContent& content) {
    load_first_objects(state, content.ending_tables[tour], 0, first_objects);
    high_bits(state, 0) = first_high;
    copy_oam(state);
    state.ending.pose = 0;
    upload_built_pose(state, content, first_pose_word);
    upload_built_pose(state, content, second_pose_word);
    state.ending.step = 0;
}

// $83:B5ED-B63C: the uni walks a pixel left; from step 0x70 the arrow flies in.
void walk_in_part(FrontEndState& state, const FrontEndContent& content, unsigned, unsigned wait) {
    auto& ending = state.ending;
    if (wait == 0) {
        --oam_byte(state, uni, 0);
        return;
    }
    upload_built_pose(state, content, first_pose_word);
    copy_oam(state);
    walk(state);
    ++ending.step;
    if (ending.step >= arrow_flies) oam_byte(state, arrow, 0) += arrow_speed;
}

// $83:B63F-B6AD: the arrow drops, faster and faster (`$77:10A7` counts its fall), drifting left;
// the uni falls apart (poses 0xA5E on).
void hit_part(FrontEndState& state, const FrontEndContent& content, unsigned step, unsigned wait) {
    auto& ending = state.ending;
    if (wait == 0) {
        if (step == 0) ending.step = ending.drop = 0;
        return;
    }
    if (wait == 1) {
        upload_built_pose(state, content, first_pose_word);
        copy_oam(state);
        return;
    }
    set_low_byte(ending.drop, ending.drop + 1U); // the fall counter is a byte here
    oam_byte(state, arrow, 1) += static_cast<std::uint8_t>((ending.drop & 0xffU) >> 2);
    oam_byte(state, arrow, 0) -= arrow_drift;
    ending.pose = static_cast<std::uint16_t>(fall_poses + ending.step);
    ++ending.step;
}

// $83:B6AF-B705: the red uni rolls in from the left, shown from step 0x0F (poses 0x13B8, eight
// in turn), sent to tile 0x108.
void red_uni_in_part(FrontEndState& state, const FrontEndContent& content, unsigned step,
                     unsigned wait) {
    auto& ending = state.ending;
    if (wait == 0) {
        if (step == 0) ending.step = 0;
        oam_byte(state, red_uni, 0) += red_uni_speed;
        if ((ending.step & 0xffU) == red_uni_shown) high_bits(state, 0) = red_uni_high;
        return;
    }
    upload_built_pose(state, content, second_pose_word);
    copy_oam(state);
    ending.pose = static_cast<std::uint16_t>(wobble_poses + (ending.step & 7U));
    ++ending.step;
}

// $83:B707-B782: the red uni rolls on (poses 0x13C0, a new one every second step, held at step
// 0x27) while `$77:10CB` counts to 0x43.
void roll_on_part(FrontEndState& state, const FrontEndContent& content, unsigned step,
                  unsigned wait) {
    auto& ending = state.ending;
    if (wait == 0) {
        if (step == 0) ending.step = ending.count = 0;
        oam_byte(state, red_uni, 0) += red_uni_speed;
        return;
    }
    upload_built_pose(state, content, second_pose_word);
    copy_oam(state);
    const auto pose_step = ending.step == held_step ? held_pose : ending.step >> 1;
    ending.pose = static_cast<std::uint16_t>(roll_poses + pose_step);
    if (ending.count >= roll_count) return;
    ++ending.count;
    if (ending.step != held_step) ++ending.step;
}

} // namespace

void script(FrontEndState& state, const FrontEndContent& content, std::uint32_t frame) {
    if (frame == setup_frame) {
        setup(state, content);
        return;
    }
    run_loop(walk_in, frame, state, content, walk_in_part);
    run_loop(hit, frame, state, content, hit_part);
    run_loop(red_uni_in, frame, state, content, red_uni_in_part);
    run_loop(roll_on, frame, state, content, roll_on_part);
}
} // namespace walker

// HOPPER (`$83:C11E`): a riderless uni walks in from the right; the rider's red uni drops in
// behind it and breathes fire at it; the uni goes up in a blast and is left a burnt frame. Its
// table (`$83:C458`): the blast's twenty poses (words), objects 0-4, the flame's eight tiles.
namespace hopper {
namespace {

constexpr unsigned tour = 6;
constexpr std::size_t blast_poses_at = 0, blast_pose_count = 20, first_objects_at = 40,
                      flame_tiles_at = 60;
// Object 0 the blast (tile 0x180), 1 the red uni (0x108), 2 and 3 the flame, 4 the uni (0x100).
// Entries 5-7 are left shown, small, at (1, 1) with tile 0 (hi1 = 0x02).
constexpr unsigned first_objects = 5, red_uni = 1, flame_tip = 2, flame = 3, uni = 4;
constexpr std::uint8_t first_high = 0x0a, second_high = 0x02, burnt_high = 0x59;
constexpr std::uint32_t objects_frame = 93, poses_frame = 94;
// $83:C23D, $83:C29D, $83:C30C, then two loops entered after their first wait (`$83:C358` and
// `$83:C3E0`), the second ending on t'.
constexpr Loop walk_in{109, 173, 1}, turn{walk_in.end(), 18, 2}, fire{turn.end(), 16, 2},
    blast_up{fire.end() - 1, 24, 2}, burn_out{blast_up.end() - 1, 60, 2};
constexpr std::uint16_t red_uni_poses = 0x1420, red_uni_drops = 0x9f, uni_turn_poses = 0x1417,
                        red_uni_turn_poses = 0x621, red_uni_fire_poses = 0x1402,
                        burnt_poses = 0x140f, last_burnt_pose = 7;
constexpr std::uint8_t red_uni_landing_y = 0x69, drop_speed = 8, red_uni_drift = 2,
                       flame_tip_offset = 2;

void setup_objects(FrontEndState& state, const FrontEndContent& content) {
    load_first_objects(state, content.ending_tables[tour], first_objects_at, first_objects);
    high_bits(state, 0) = first_high;
    high_bits(state, uni) = second_high;
    copy_oam(state);
}

void setup_poses(FrontEndState& state, const FrontEndContent& content) {
    upload_pose(state, content, 0, first_pose_word);
    state.ending.pose = red_uni_poses;
    upload_built_pose(state, content, second_pose_word);
    state.ending.step = 0;
}

// $83:C23D-C286: the uni walks a pixel left; from step 0x9F the red uni drops in.
void walk_in_part(FrontEndState& state, const FrontEndContent& content, unsigned, unsigned wait) {
    auto& ending = state.ending;
    if (wait == 0) {
        --oam_byte(state, uni, 0);
        return;
    }
    upload_built_pose(state, content, first_pose_word);
    copy_oam(state);
    walk(state);
    ++ending.step;
    if (ending.step < red_uni_drops) return;
    oam_byte(state, red_uni, 0) += red_uni_drift;
    oam_byte(state, red_uni, 1) += drop_speed;
}

// $83:C289-C300: the red uni lands and turns (poses 0x621 on, to tile 0x108) while the uni turns
// to face it (poses 0x1417 on), each a new pose every second step.
void turn_part(FrontEndState& state, const FrontEndContent& content, unsigned step, unsigned wait) {
    auto& ending = state.ending;
    if (wait == 0) {
        if (step == 0) {
            ending.step = 0;
            oam_byte(state, red_uni, 0) -= red_uni_drift;
            oam_byte(state, red_uni, 1) = red_uni_landing_y;
        }
        --oam_byte(state, red_uni, 0);
        return;
    }
    const auto first = wait == 1;
    upload_built_pose(state, content, first ? first_pose_word : second_pose_word);
    copy_oam(state);
    ending.pose = static_cast<std::uint16_t>((first ? uni_turn_poses : red_uni_turn_poses)
                                             + (ending.step >> 1));
    if (!first) ++ending.step;
}

// $83:C302-C338: the flame's tiles, a new pair every second step.
void fire_part(FrontEndState& state, const FrontEndContent& content, unsigned step, unsigned wait) {
    auto& ending = state.ending;
    if (wait == 1) return;
    if (wait == 2) {
        copy_oam(state);
        ++ending.step;
        return;
    }
    if (step == 0) ending.step = 0;
    const auto tile =
        table_byte(content.ending_tables[tour], flame_tiles_at + ((ending.step & 0xffU) >> 1));
    oam_byte(state, flame, 2) = tile;
    oam_byte(state, flame_tip, 2) = static_cast<std::uint8_t>(tile + flame_tip_offset);
}

// $83:C35F and $83:C3E7: the blast's pose for `$77:10CB`, its twenty in turn.
std::uint16_t blast_pose(const FrontEndState& state, const FrontEndContent& content) {
    const auto at = blast_poses_at + 2U * (state.ending.count % blast_pose_count);
    const auto table = content.ending_tables[tour];
    return static_cast<std::uint16_t>(table_byte(table, at) | table_byte(table, at + 1) << 8U);
}

// $83:C33A-C3B9 and $83:C3BB-C449: after the first wait the blast's pose goes to `blast_word`
// and the next is built; after the second the red uni's (or the burnt uni's) pose goes to tile
// 0x108 and the next is built. The first step starts after its first wait.
void blast_part(FrontEndState& state, const FrontEndContent& content, unsigned step, unsigned wait,
                unsigned blast_word, std::uint16_t (*next_pose)(std::uint16_t)) {
    auto& ending = state.ending;
    if (wait == 0) return;
    if (wait == 1 && step > 0) {
        upload_built_pose(state, content, blast_word);
        copy_oam(state);
    }
    if (wait == 1) {
        ending.pose = blast_pose(state, content);
        return;
    }
    upload_built_pose(state, content, second_pose_word);
    copy_oam(state);
    ending.pose = next_pose(ending.step);
    ++ending.count;
    ++ending.step;
}

// $83:C33A: the blast grows at tile 0x180 while the red uni keeps firing (poses 0x1402 on).
void blast_up_part(FrontEndState& state, const FrontEndContent& content, unsigned step,
                   unsigned wait) {
    if (step == 0 && wait == 1) {
        set_low_byte(state.ending.step, 0);
        set_low_byte(state.ending.count, 0);
    }
    blast_part(state, content, step, wait, third_pose_word, [](std::uint16_t s) {
        return static_cast<std::uint16_t>(red_uni_fire_poses + (s >> 1));
    });
}

// $83:C3BB: the blast goes on over the uni's tiles (0x100), the uni's object hidden and the burnt
// uni's poses (0x140F on, held at the eighth) at tile 0x108.
void burn_out_part(FrontEndState& state, const FrontEndContent& content, unsigned step,
                   unsigned wait) {
    if (step == 0 && wait == 1) {
        state.ending.step = 0;
        high_bits(state, 0) = burnt_high;
    }
    blast_part(state, content, step, wait, first_pose_word, [](std::uint16_t s) {
        return static_cast<std::uint16_t>(burnt_poses
                                          + std::min<unsigned>(s >> 1, last_burnt_pose));
    });
}

} // namespace

void script(FrontEndState& state, const FrontEndContent& content, std::uint32_t frame) {
    if (frame == bar_frame) clear_third_pose_tiles(state);
    if (frame == objects_frame) setup_objects(state, content);
    if (frame == poses_frame) setup_poses(state, content);
    run_loop(walk_in, frame, state, content, walk_in_part);
    run_loop(turn, frame, state, content, turn_part);
    run_loop(fire, frame, state, content, fire_part);
    run_loop(blast_up, frame, state, content, blast_up_part);
    run_loop(burn_out, frame, state, content, burn_out_part);
}
} // namespace hopper

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

} // namespace unirally::front_end_screens::ending
