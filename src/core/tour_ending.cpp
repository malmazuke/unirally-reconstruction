// A tour's gold ending (R-0062): `$83:883E` dispatches the third completion of a tour through
// `$83:88FD` to the tour's routine. Every tour's ending is one scheme: the award's fade out,
// `$83:A507` (the award's set-up with another song), a gold mode-2 screen and its loads, a fade
// in, a script of fixed length that reads no pad, then the award's way back a frame later.
// Frames count from the completion's scoring frame, s.
#include "front_end_screens.hpp"

#include <array>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <vector>

namespace unirally::front_end_screens {

namespace {

// The scheme, frames from s: the fade out ends on frame 16; `$83:A575-A613` and the tour's first
// loads run on frame 89, BG1's tiles on 90, the bar's map on 91, its tiles, colours and the
// objects' tiles on 92; each tour then sets up its objects and fades in (`EndingLayout`).
constexpr std::uint32_t blank_frame = 16, reset_frame = 89, pattern_tiles_frame = 90,
                        bar_map_frame = 91, bar_frame = 92, fade_in_frames = 15;
// The way back runs a frame later than after the award (R-0062).
constexpr std::uint32_t way_back_delay = 1;

// The screen: BG1 the gold pattern (map 0x53, tiles 0x54, colours 0x57 at CGRAM 0), BG2 the bar
// (map 0x52 with palette 1 and priority, tiles 0x4C, colours 0x3C at CGRAM 0x10).
constexpr unsigned pattern_map = 0x53, pattern_tiles = 0x54, gold_colours = 0x57, bar_map = 0x52,
                   bar_tiles = 0x4c, bar_colours = 0x3c, uni_colours = 0x19;
constexpr unsigned pattern_tiles_word = 0x3000, bar_map_word = 0x1000, bar_tiles_word = 0x2000,
                   object_tiles_word = 0x6000;
constexpr std::uint16_t bar_map_bits = 0x2400;
constexpr std::uint8_t ending_screens = 0x13; // BG1, BG2, objects

// Where `$83:8E3A`'s pose buffer goes: object tiles 0x100 (the uni), 0x108 and 0x180.
constexpr unsigned first_pose_word = 0x7000, second_pose_word = 0x7080;
constexpr std::size_t oam_entry_bytes = 4;

// On the way back the menus' screen comes back and NMI is turned on in frame t' + 118
// (`$83:A90B`); for most tours that write lands inside the frame's vertical blank and NMI's hook
// runs at once, but for WALKER and JUMPER it lands after it and the hook first runs a frame later
// (walker-gold, locked-gold and all-gold alike). Why their copies end later is not recovered.
constexpr std::uint32_t menus_back_frame = 118;

// One tour's ending: where the rider's colours go (the tour's own at the next row), the tour's
// object colours and tiles, OBSEL, the first frame of the fade in and the script's last frame
// (t'), the script, which runs on every frame from `bar_frame` to t', and whether NMI's hook
// comes back a frame late.
struct EndingLayout {
    unsigned rider_colours_at{};
    unsigned tour_colours{}; // 0: none
    unsigned object_tiles{}; // 0: none; the menus' tiles stay
    std::uint8_t obsel{};
    std::uint32_t fade_in_frame{}, last_frame{};
    void (*script)(FrontEndState&, const FrontEndContent&, std::uint32_t){};
    bool late_nmi_hook{};
};

// $83:A923 between the steps: the pose built on the last frame, sent to `word`.
void upload_built_pose(FrontEndState& state, const FrontEndContent& content, unsigned word) {
    upload_pose(state, content, state.ending.pose, word);
}

// $83:B79C: the walk's poses, a new one every second step, 23 in turn.
void walk(FrontEndState& state) {
    constexpr unsigned walk_poses = 23, pose_stride = 0x40;
    auto& ending = state.ending;
    ending.pose = static_cast<std::uint16_t>(((ending.step >> 1U) % walk_poses) * pose_stride);
}

// A tour's first objects from its table into the OAM buffer.
void load_first_objects(FrontEndState& state, std::span<const std::uint8_t> table, std::size_t at,
                        unsigned entries) {
    const auto bytes = entries * oam_entry_bytes;
    if (table.size() < at + bytes) throw std::invalid_argument("ending table is short");
    for (std::size_t k = 0; k < bytes; ++k) state.oam_buffer[k] = table[at + k];
}

// COLDATA: bits 5, 6 and 7 choose red, green and blue, bits 0-4 their intensity.
void write_fixed_colour(FrontEndState& state, std::uint8_t data) {
    auto& colour = state.registers.fixed_colour;
    for (unsigned channel = 0; channel < 3; ++channel) {
        if (!(data & (0x20U << channel))) continue;
        const auto shift = 5U * channel;
        colour = static_cast<std::uint16_t>((colour & ~(0x1fU << shift))
                                            | (unsigned(data & 0x1fU) << shift));
    }
}

// CRAWLER (`$83:C49C`): the rider walks in from the right past a gold nugget, stops, and a flash
// of colour math leaves a hole where the nugget was. Its table (`$83:C6C0`): objects 0-7, then
// objects 8-9, then the flash's fourteen COLDATA triples.
namespace crawler {
constexpr std::size_t first_objects_at = 0, hole_objects_at = 32, flash_colours_at = 40;
constexpr std::uint32_t setup_frame = 95, walk_frame = 111, walk_steps = 131,
                        flash_frame = walk_frame + walk_steps, flash_steps = 14;
constexpr unsigned uni = 1, first_flash = 4, hole = 8;
constexpr std::uint8_t first_high = 0x68, flash_high = 0x69, hole_high = 0x65,
                       hole_objects_high = 0x50;
constexpr std::uint8_t flash_math = 0x33; // add: BG1, BG2, objects, backdrop
constexpr unsigned clear_word = 0x7800, clear_words = 0x400;
// The hole: BG2 map words 0x124E-0x1251 and 0x126E-0x1271 take tile 0x64, which the menus'
// asset 0x46 left in VRAM.
constexpr std::array<unsigned, 2> hole_rows{0x124e, 0x126e};
constexpr std::uint8_t hole_tile = 0x64;

void flash_step(FrontEndState& state, const FrontEndContent& content, unsigned index) {
    const auto table = content.ending_tables[0];
    copy_oam(state);
    auto& r = state.registers;
    r.main_screen = ending_screens;
    const auto countdown = 13 - index; // the loop's Y
    const auto set_flash_tiles = [&](std::uint8_t tile) {
        for (unsigned entry = first_flash; entry < first_flash + 4; ++entry)
            oam_byte(state, entry, 2) = tile;
    };
    if (countdown == 12 || countdown == 6) set_flash_tiles(0x40);
    if (countdown == 10) set_flash_tiles(0x44);
    if (countdown == 8) {
        r.main_screen = 0; // the picture is the backdrop and the fixed colour
        high_bits(state, 0) = hole_high;
        for (const auto row : hole_rows)
            load_vram(
                state,
                std::vector<std::uint8_t>{hole_tile, 0, hole_tile, 0, hole_tile, 0, hole_tile, 0},
                row);
        for (std::size_t k = 0; k < 2 * oam_entry_bytes; ++k)
            state.oam_buffer[hole * oam_entry_bytes + k] = table[hole_objects_at + k];
        high_bits(state, hole) = hole_objects_high;
    }
    if (countdown == 4) high_bits(state, first_flash) = four_hidden;
    for (unsigned write = 0; write < 3; ++write)
        write_fixed_colour(state, table[flash_colours_at + 3U * index + write]);
}

void script(FrontEndState& state, const FrontEndContent& content, std::uint32_t frame) {
    auto& ending = state.ending;
    if (frame == bar_frame) { // CRAWLER alone clears these tiles (on frame 89; the screen is blank)
        load_vram(state, std::vector<std::uint8_t>(clear_words * 2, 0), clear_word);
        return;
    }
    if (frame == setup_frame) {
        load_first_objects(state, content.ending_tables[0], first_objects_at, 8);
        high_bits(state, 0) = first_high;
        high_bits(state, first_flash) = four_hidden;
        copy_oam(state);
        ending.pose = 0;
        upload_built_pose(state, content, first_pose_word);
        ending.step = 0;
        return;
    }
    if (frame == walk_frame - 1)
        --oam_byte(state, uni, 0); // the walk's first step, before its wait
    if (frame >= walk_frame && frame < flash_frame) {
        const auto k = frame - walk_frame;
        upload_built_pose(state, content, first_pose_word);
        copy_oam(state);
        walk(state);
        ending.step = static_cast<std::uint16_t>(k + 1);
        if (k + 1 < walk_steps) {
            --oam_byte(state, uni, 0);
            return;
        }
        // After the last step: the nugget hidden, the flash's objects shown, colour math on.
        high_bits(state, 0) = flash_high;
        high_bits(state, first_flash) = four_shown;
        state.registers.colour_select = 0;
        state.registers.colour_math = flash_math;
        return;
    }
    if (frame >= flash_frame && frame < flash_frame + flash_steps)
        flash_step(state, content, frame - flash_frame);
}
} // namespace crawler

// One loop of a script: `steps` passes of `waits` waits each (`$83:A923`), the first pass
// starting in frame `first`. Pass k's code after its w-th wait runs in frame
// first + waits * k + w, so its code after its last wait and pass k + 1's code before its first
// wait share a frame, and the loop's end is the next loop's first frame.
struct Loop {
    std::uint32_t first{}, steps{}, waits{};
    [[nodiscard]] constexpr std::uint32_t end() const { return first + steps * waits; }
};

// Runs a loop's code for `frame`: part(step, w), where w = 0 is the code before the pass's first
// wait and w = `waits` the code after its last.
template <typename Part>
void run_loop(const Loop& loop, std::uint32_t frame, Part part) {
    if (frame < loop.first || frame > loop.end()) return;
    const auto offset = frame - loop.first;
    const auto step = offset / loop.waits, wait = offset % loop.waits;
    if (wait == 0 && step > 0) part(step - 1, loop.waits);
    if (step < loop.steps) part(step, wait);
}

std::uint8_t table_byte(std::span<const std::uint8_t> table, std::size_t at) {
    if (at >= table.size()) throw std::invalid_argument("ending table is short");
    return table[at];
}

// SHUFFLER (`$83:B1EB`): a riderless uni rolls in from the right with a turtle walking behind
// it; the rider's red uni drops in and bounces away; the turtle's tongue reaches out, pulls the
// uni over and swallows it. Its table (`$83:B4DF`): objects 0-3, objects 4-7 (the turtle
// eating, four quarters), the walking turtle's four tiles, the eating turtle's three.
namespace shuffler {
constexpr unsigned tour = 2;
constexpr std::size_t first_objects_at = 0, eating_objects_at = 16, turtle_tiles_at = 32,
                      eating_tiles_at = 36;
constexpr unsigned turtle_top = 0, uni = 1, red_uni = 2, turtle_bottom = 3, eating_turtle = 4;
constexpr std::uint8_t first_high = 0x28, eating_high = 0x69, swallowed_high = 0x55;
constexpr std::uint32_t setup_frame = 95;
// $83:B2CD, $83:B388, $83:B3FF, $83:B44F and $83:B496, then 61 waits to t'.
constexpr Loop roll{110, 151, 1}, bounce{roll.end(), 9, 2}, tongue_out{bounce.end(), 6, 2},
    pull_over{tongue_out.end(), 8, 2}, swallow{pull_over.end(), 6, 2};
constexpr std::uint16_t turtle_starts = 0x1d, red_uni_drops = 0x89, bounce_poses = 0x621,
                        pull_poses = 0x1390, walk_poses = 23, pose_stride = 0x40;
constexpr std::uint8_t lower_tile_row = 0x20, bounce_height = 8, pulled_attr = 0x13,
                       pulled_x = 0x57;

void setup(FrontEndState& state, const FrontEndContent& content) {
    load_first_objects(state, content.ending_tables[tour], first_objects_at, 4);
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

// $83:B374-B3D7: the red uni is put back on the ground and bounces up out of sight while the
// uni's poses 0x621 on play.
void bounce_part(FrontEndState& state, const FrontEndContent& content, unsigned step,
                 unsigned wait) {
    auto& ending = state.ending;
    auto& red_uni_y = oam_byte(state, red_uni, 1);
    if (wait == 0) {
        if (step == 0) {
            ending.step = 0;
            red_uni_y = 0x69;
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
    const std::array<std::uint8_t, 4> offsets{0x00, 0x01, 0x10, 0x11};
    for (unsigned k = 0; k < 4; ++k)
        oam_byte(state, eating_turtle + k, 2) = static_cast<std::uint8_t>(tile + offsets[k]);
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
        for (std::size_t k = 0; k < 4 * oam_entry_bytes; ++k)
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
        state.ending.step = 5;
        high_bits(state, 0) = swallowed_high;
    }
    set_eating_tiles(state, content);
}

void script(FrontEndState& state, const FrontEndContent& content, std::uint32_t frame) {
    if (frame == setup_frame) {
        setup(state, content);
        return;
    }
    run_loop(roll, frame, [&](unsigned, unsigned wait) {
        wait == 0 ? roll_start(state, content) : roll_end(state, content);
    });
    const auto part = [&](auto function) {
        return
            [&, function](unsigned step, unsigned wait) { function(state, content, step, wait); };
    };
    run_loop(bounce, frame, part(bounce_part));
    run_loop(tongue_out, frame, part(tongue_out_part));
    run_loop(pull_over, frame, part(pull_over_part));
    run_loop(swallow, frame, part(swallow_part));
}
} // namespace shuffler

// WALKER (`$83:B506`): a riderless uni walks in from the right; an arrow flies in, hits it and
// drops, and the uni falls apart; the rider's red uni rolls in from the left and on across the
// screen. Its table (`$83:B790`): objects 0-2.
namespace walker {
constexpr unsigned tour = 4;
constexpr unsigned uni = 0, red_uni = 1, arrow = 2;
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
    load_first_objects(state, content.ending_tables[tour], 0, 3);
    high_bits(state, 0) = first_high;
    copy_oam(state);
    state.ending.pose = 0;
    upload_built_pose(state, content, first_pose_word);
    upload_built_pose(state, content, second_pose_word);
    state.ending.step = 0;
}

// $83:B5ED-B63C: the uni walks a pixel left; from step 0x70 the arrow flies in.
void walk_in_part(FrontEndState& state, const FrontEndContent& content, unsigned wait) {
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

// $83:B63F-B6AD: the arrow drops, faster and faster (`$10A7` counts its fall), drifting left;
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
    // The fall counter is a byte here.
    const auto drop = static_cast<std::uint8_t>(ending.drop + 1);
    ending.drop = static_cast<std::uint16_t>((ending.drop & 0xff00U) | drop);
    oam_byte(state, arrow, 1) += static_cast<std::uint8_t>(drop >> 2);
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
// 0x27) while `$10CB` counts to 0x43.
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

void script(FrontEndState& state, const FrontEndContent& content, std::uint32_t frame) {
    if (frame == setup_frame) {
        setup(state, content);
        return;
    }
    run_loop(walk_in, frame, [&](unsigned, unsigned wait) { walk_in_part(state, content, wait); });
    const auto part = [&](auto function) {
        return
            [&, function](unsigned step, unsigned wait) { function(state, content, step, wait); };
    };
    run_loop(hit, frame, part(hit_part));
    run_loop(red_uni_in, frame, part(red_uni_in_part));
    run_loop(roll_on, frame, part(roll_on_part));
}
} // namespace walker

// The eight tours' endings by `$00D0`; a tour whose ending is not recovered has no script and
// leaves at once through the award's way out.
const std::array<EndingLayout, 8> endings{{
    {0xc0, 0x40, 0x60, 0xa3, 96, 336, crawler::script}, // CRAWLER `$83:C49C`
    {},
    {0x80, 0x41, 0x61, 0xa3, 96, 380, shuffler::script}, // SHUFFLER `$83:B1EB`
    {},
    {0x80, 0, 0x5e, 0x83, 94, 393, walker::script, true}, // WALKER `$83:B506`
    {},
    {},
    {},
}};

// $83:A507's frame 89 and the tour routine's first loads.
void first_loads(FrontEndState& state, const FrontEndContent& content, const EndingLayout& layout) {
    reset_award_screen(state, content);
    auto& r = state.registers;
    r.mode = 2;
    r.main_screen = ending_screens;
    r.bg[0].vofs = r.bg[1].vofs = 0;
    load_cgram(state, asset(content, gold_colours), 0);
    load_cgram(state, asset(content, first_rider_palette + state.rider_menu.rider),
               layout.rider_colours_at);
    load_cgram(state, asset(content, uni_colours), layout.rider_colours_at + 0x10);
    if (layout.tour_colours)
        load_cgram(state, asset(content, layout.tour_colours), layout.rider_colours_at + 0x20);
    load_map(state, asset(content, pattern_map), 0, 0);
}

void bar_loads(FrontEndState& state, const FrontEndContent& content, const EndingLayout& layout) {
    load_vram(state, asset(content, bar_tiles), bar_tiles_word);
    load_cgram(state, asset(content, bar_colours), 0x10);
    state.registers.obsel = layout.obsel;
    if (layout.object_tiles)
        load_vram(state, asset(content, layout.object_tiles), object_tiles_word);
}

} // namespace

void start_tour_ending(FrontEndState& state) {
    state.ending = {};
    state.ending.tour = state.tour_menu.tour;
    state.screen = FrontEndScreen::tour_ending;
    state.script_frame = 0;
}

void tour_ending_frame(FrontEndState& state, const FrontEndContent& content) {
    const auto& layout = endings[state.ending.tour];
    const auto frame = state.script_frame;
    if (frame > layout.last_frame) {
        const auto exit = frame - layout.last_frame;
        if (layout.late_nmi_hook && exit == menus_back_frame)
            restore_menu_screen(state, content); // NMI's hook runs from the next frame
        else
            way_back_frame(state, content, exit, way_back_delay);
        return;
    }
    if (frame <= blank_frame) {
        fade_down(state, frame);
        return;
    }
    switch (frame) {
    case reset_frame: first_loads(state, content, layout); return;
    case pattern_tiles_frame:
        load_vram(state, asset(content, pattern_tiles), pattern_tiles_word);
        return;
    case bar_map_frame:
        load_map(state, asset(content, bar_map), bar_map_word, bar_map_bits);
        return;
    case bar_frame: bar_loads(state, content, layout); break;
    default: break;
    }
    if (frame < bar_frame) return;
    if (frame >= layout.fade_in_frame && frame < layout.fade_in_frame + fade_in_frames)
        fade_up(state, frame - layout.fade_in_frame + 1);
    layout.script(state, content, frame);
}

bool has_tour_ending(unsigned tour) {
    return tour < endings.size() && endings[tour].script != nullptr;
}

} // namespace unirally::front_end_screens
