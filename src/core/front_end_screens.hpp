#pragma once
// Inside the front end (front_end.cpp, rider_menu.cpp): what every screen shares, the loads, the
// OAM buffer and the arrow, and each screen's frame.
#include "front_end.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

namespace unirally::front_end_screens {

// OAM buffer layout ($0A00): 128 four-byte entries, then the high table at $0C00.
inline constexpr std::size_t oam_high_table = 512;
inline constexpr unsigned arrow_entry = 119, shadow_entry = 127;
// The word a cleared text map holds ($80:D1FA, $83:8B51).
inline constexpr std::uint16_t cleared_text = 0x004c;

inline std::uint8_t& oam_byte(FrontEndState& state, unsigned entry, unsigned field) {
    return state.oam_buffer[entry * 4 + field];
}

std::span<const std::uint8_t> asset(const FrontEndContent& content, unsigned id);
// The VRAM copier `$82:B1DB` from word address `word`; CGRAM `$82:B183` from colour `colour`.
void load_vram(FrontEndState& state, std::span<const std::uint8_t> data, unsigned word);
void load_cgram(FrontEndState& state, std::span<const std::uint8_t> data, unsigned colour);
// `$80:937B`: the text map to VRAM from word `word`.
void load_text(FrontEndState& state, unsigned word);
void copy_oam(FrontEndState& state); // $80:9318
void set_oam_x_high(FrontEndState& state, unsigned entry, bool high);
void park_arrow(FrontEndState& state); // $83:99FA

// $80:D2C1: every object hidden, the main menu's objects laid out, the arrow parked.
void lay_out_menu_objects(FrontEndState& state);
// The main menu's text into the text map ($80:ACD5: `$80:D1FA` clears it, `$80:C3BC` prints).
void print_main_menu(FrontEndState& state, const FrontEndContent& content);
void reload_menu_palette(FrontEndState& state, const FrontEndContent& content);    // $80:A8A8
void reload_menu_text_tiles(FrontEndState& state, const FrontEndContent& content); // $80:A877
void start_main_menu(FrontEndState& state);                                        // $80:ABC8

// screen_slide.cpp. A count that steps down and wraps from below 0 to `last`, as the original's
// DEC/BPL pairs do; true when it wrapped.
bool step_down(std::uint8_t& counter, std::uint8_t last);
// $83:9A1E: every third pass the tiles of entries 96-99 and 112-114 step; every other pass the
// tiles of entries 104-111 and the columns of entries 100-103.
void step_decorations(FrontEndState& state, const FrontEndContent& content);
// A slide's first pass, before its frame wait; then each pass after its wait, with the next
// pass's work. `slide_frame` is true once the halves have swapped.
void start_slide(FrontEndState& state, const FrontEndContent& content, bool back);
bool slide_frame(FrontEndState& state, const FrontEndContent& content);

// rider_menu.cpp: 1P chosen on the main menu (`$80:BB9C`), or Y or X on PICK TOUR (`$80:BBA3`,
// which slides the rider menu back in); then a frame of each script.
void enter_rider_menu(FrontEndState& state);
void return_to_rider_menu(FrontEndState& state);
void rider_menu_entry_frame(FrontEndState& state, const FrontEndContent& content);
void rider_menu_frame(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads);
void rider_menu_exit_frame(FrontEndState& state, const FrontEndContent& content);
void main_menu_return_frame(FrontEndState& state, const FrontEndContent& content);

// tour_menu.cpp: PICK TOUR after a rider is chosen (`$80:BBF7-BC0B`), a frame of its set-up and
// of its loop.
void enter_tour_menu(FrontEndState& state);
void tour_menu_entry_frame(FrontEndState& state, const FrontEndContent& content);
void tour_menu_frame(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads);

} // namespace unirally::front_end_screens
