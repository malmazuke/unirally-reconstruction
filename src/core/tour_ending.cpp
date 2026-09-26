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
constexpr unsigned first_pose_word = 0x7000;
constexpr std::size_t oam_entry_bytes = 4;

// One tour's ending: where the rider's colours go (the tour's own at the next row), the tour's
// object colours and tiles, OBSEL, the first frame of the fade in and the script's last frame
// (t'), and the script, which runs on every frame from `bar_frame` to t'.
struct EndingLayout {
    unsigned rider_colours_at{};
    unsigned tour_colours{}; // 0: none
    unsigned object_tiles{}; // 0: none; the menus' tiles stay
    std::uint8_t obsel{};
    std::uint32_t fade_in_frame{}, last_frame{};
    void (*script)(FrontEndState&, const FrontEndContent&, std::uint32_t){};
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

// The eight tours' endings by `$00D0`; a tour whose ending is not recovered has no script and
// leaves at once through the award's way out.
const std::array<EndingLayout, 8> endings{{
    {0xc0, 0x40, 0x60, 0xa3, 96, 336, crawler::script}, // CRAWLER `$83:C49C`
    {},
    {},
    {},
    {},
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
        way_back_frame(state, content, frame - layout.last_frame, way_back_delay);
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
