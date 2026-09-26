#pragma once
// Inside the gold endings (tour_ending.cpp, tour_ending_scripts.cpp, R-0062): what the tours'
// scripts share, and the scripts of the tours after CRAWLER.
#include "front_end_screens.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

namespace unirally::front_end_screens::ending {

// Frames count from the completion's scoring frame; the scripts run from `bar_frame`, the frame
// of the bar's and the objects' loads.
inline constexpr std::uint32_t bar_frame = 92;
inline constexpr std::uint8_t ending_screens = 0x13; // BG1, BG2, objects
// Where `$83:8E3A`'s pose buffer goes: object tiles 0x100, 0x108 and 0x180.
inline constexpr unsigned first_pose_word = 0x7000, second_pose_word = 0x7080,
                          third_pose_word = 0x7800;
inline constexpr std::size_t oam_entry_bytes = 4;

// One loop of a script: `steps` passes of `waits` waits each (`$83:A923`), the first pass
// starting in frame `first`. Pass k's code after its w-th wait runs in frame
// first + waits * k + w, so its code after its last wait and pass k + 1's code before its first
// wait share a frame, and the loop's end is the next loop's first frame.
struct Loop {
    std::uint32_t first{}, steps{}, waits{};
    [[nodiscard]] constexpr std::uint32_t end() const { return first + steps * waits; }
};

// A loop's code for pass `step` after its `wait`-th wait: 0 is the code before the first wait,
// `waits` the code after the last.
using LoopPart = void (*)(FrontEndState&, const FrontEndContent&, unsigned step, unsigned wait);
// Runs the loop's code that falls in `frame`, if any.
void run_loop(const Loop& loop, std::uint32_t frame, FrontEndState& state,
              const FrontEndContent& content, LoopPart part);

// $83:A923 between the steps: the pose built last (`TourEnding::pose`), sent to `word`.
void upload_built_pose(FrontEndState& state, const FrontEndContent& content, unsigned word);
// $83:B79C: the walk's poses, a new one every second step, 23 in turn.
void walk(FrontEndState& state);
// A tour's first objects from its table into the OAM buffer.
void load_first_objects(FrontEndState& state, std::span<const std::uint8_t> table, std::size_t at,
                        unsigned entries);
// $83:C4A4 (CRAWLER) and $83:C126 (HOPPER): object tiles 0x180-0x1FF (VRAM 0x7800-0x7FFF), where
// the menus left the medals' tiles, cleared by a DMA on frame 89. Native clears them on
// `bar_frame`; the screen is blank either way.
void clear_third_pose_tiles(FrontEndState& state);
// A counter's byte write (`STA` with an 8-bit accumulator): the low byte only.
void set_low_byte(std::uint16_t& word, unsigned value);
// Byte `at` of a tour's table; throws if the table is short.
std::uint8_t table_byte(std::span<const std::uint8_t> table, std::size_t at);

namespace shuffler {
void script(FrontEndState& state, const FrontEndContent& content, std::uint32_t frame);
}
namespace walker {
void script(FrontEndState& state, const FrontEndContent& content, std::uint32_t frame);
}
namespace hopper {
void script(FrontEndState& state, const FrontEndContent& content, std::uint32_t frame);
}
namespace jumper {
void script(FrontEndState& state, const FrontEndContent& content, std::uint32_t frame);
}
namespace bounder {
void script(FrontEndState& state, const FrontEndContent& content, std::uint32_t frame);
}
namespace runner {
void script(FrontEndState& state, const FrontEndContent& content, std::uint32_t frame);
}

} // namespace unirally::front_end_screens::ending
