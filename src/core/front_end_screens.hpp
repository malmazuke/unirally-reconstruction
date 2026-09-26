#pragma once
// Inside the front end (front_end.cpp, screen_slide.cpp, rider_menu.cpp, tour_menu.cpp,
// track_menu.cpp, now_playing.cpp): what every screen shares, the loads, the OAM buffer, the
// arrow, the pads, and each screen's frame.
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

// The OAM high table: two bits an entry, four entries a byte. Bit 0 is the ninth x bit, which
// the menus set to push an entry off the right edge (hidden); bit 1 makes it large.
inline std::uint8_t& high_bits(FrontEndState& state, unsigned first_entry) {
    return state.oam_buffer[oam_high_table + first_entry / 4];
}
inline constexpr std::uint8_t hidden_bit(unsigned entry) {
    return static_cast<std::uint8_t>(1U << ((entry % 4) * 2));
}
inline constexpr std::uint8_t large_bit(unsigned entry) {
    return static_cast<std::uint8_t>(2U << ((entry % 4) * 2));
}
inline constexpr std::uint8_t four_hidden = 0x55, four_shown = 0x00;

// Assets: rider r's sprite palette is asset 6 + r (R-0055); the base BG palette's halves are
// assets 35 and 36 (`$80:A858`).
inline constexpr unsigned first_rider_palette = 6, base_palette_low = 35, base_palette_high = 36;
// A record's holder that is no rider, SOMEONE; from 0x11 on, the computer opponents (`$017F`).
inline constexpr std::uint8_t someone = 0x10;
// After a race (R-0057), the return's script frames from the race's last frame: NMI runs the
// arrow on the sound upload's last frame and `$80:D20E`'s first, then from the OAM copy on
// (the restore frame).
inline constexpr std::uint32_t upload_last_frame = 74, menu_screen_frame = 75, restore_frame = 101;
// The result screen's third frame, its tail (`$80:9579`); after a lap result's second frame's
// overrun it starts without a frame wait.
inline constexpr std::uint32_t result_tail_frame = 3;
// The high-table bits the result screens clear to show the player's new-best markers, entries
// 96 and 98 (`$80:CFB9`, `$80:9051`).
inline constexpr std::uint8_t player_best_markers = 0xee;
// The one-player tours: five tracks each; HUNTER is tour 8.
inline constexpr std::uint8_t tracks_per_tour = 5, hunter = 8;
// The object tiles at VRAM word 0x7A00 that PICK TOUR and PICK TRACK swap (`$83:94D0`,
// `$83:94FF`).
inline constexpr unsigned swapped_object_tiles_word = 0x7a00;
// The decoration animator's tables inside `front-end.decoration-frames` ($83:9AF9 on).
inline constexpr std::size_t pair_tiles = 0x00, cycle_tiles = 0x08, left_sway = 0x10,
                             right_sway = 0x1a, trio_tiles = 0x2e, wave_tiles = 0x38;

// The pads as the SNES reads them. The one-player screens read pad 1 only (`$83:9543`); Down
// counts Select where `$80:B794` tests it. Choose is B, Start or A (`$80:B71D`); back is Y or X
// (`$80:B74A`).
inline constexpr std::uint16_t pad_up = 0x0800, pad_down = 0x0400, pad_left = 0x0200,
                               pad_right = 0x0100, pad_select = 0x2000;
inline constexpr std::uint16_t choose_buttons = 0x9080, back_buttons = 0x4040;

// The `n`th 0xFF-terminated string of `table`, without its 0xFF.
std::span<const std::uint8_t> nth_string(std::span<const std::uint8_t> table, unsigned n);

inline std::uint8_t& oam_byte(FrontEndState& state, unsigned entry, unsigned field) {
    return state.oam_buffer[entry * 4 + field];
}

// Leaving the result (R-0057, R-0060): `$83:879A` scores on the exit's second frame, after
// `$83:A923`'s wait; when `$80:C786` placed a time its checksums run past their frame, and the
// scoring comes a frame later. Neither of those frames moves the arrow.
inline std::uint32_t result_scoring_frame(const FrontEndState& state) {
    return state.race_result.record_placed ? 3 : 2;
}
// $83:9E47: the rider's best time (or score) on the track, `$77:0829 + 2 x (50 x rider + track)`.
inline std::uint16_t& personal_best(OnePlayerRecords& records, unsigned rider, unsigned track) {
    return records.best[rider * 50U + track];
}
inline std::uint16_t personal_best(const OnePlayerRecords& records, unsigned rider,
                                   unsigned track) {
    return records.best[rider * 50U + track];
}
// $83:A721 after the award: the menus' registers, colours, VRAM, text and objects as `$80:D20E`
// leaves them, without its reset of the menus' words; the logo held up; NMI on.
void restore_menu_screen(FrontEndState& state, const FrontEndContent& content);
// $83:8E3A and `$80:F818`: pose `pose` into the five rows of six object tiles at VRAM word
// 0x7000.
void upload_pose(FrontEndState& state, const FrontEndContent& content, std::uint16_t pose);
// $80:C6D5, the printer's EF: entry 104 + `object` at the text map word `position`.
void place_printed_object(FrontEndState& state, unsigned object, unsigned position);
// $80:98A4: the arrow flies off the left edge.
void send_arrow_off(FrontEndState& state);

std::span<const std::uint8_t> asset(const FrontEndContent& content, unsigned id);
// The VRAM copier `$82:B1DB` from word address `word`; CGRAM `$82:B183` from colour `colour`.
void load_vram(FrontEndState& state, std::span<const std::uint8_t> data, unsigned word);
void load_cgram(FrontEndState& state, std::span<const std::uint8_t> data, unsigned colour);
// `$80:937B`: the text map to VRAM from word `word`.
void load_text(FrontEndState& state, unsigned word);
void copy_oam(FrontEndState& state); // $80:9318
// The entry's ninth x bit: set, the menus' way to hide an entry (`$83:961E`).
void set_oam_x_high(FrontEndState& state, unsigned entry, bool high);
void park_arrow(FrontEndState& state); // $83:99FA

// $80:A09A (boot frame 24): every OAM entry at (1, 1), every high bit set.
void clear_oam_buffer(FrontEndState& state);
// The registers `$80:A09A` sets (boot frame 97): BG1 and BG2 maps, mode 3, objects.
void set_early_registers(FrontEndState& state);
// $83:91F7: colour 0xF0 on, the rider's palette in a one-player game, else asset 2.
void load_object_palette(FrontEndState& state, const FrontEndContent& content);
// $80:D20E: the main menu's screen (boot frame 377), its text left out.
void load_main_menu_screen(FrontEndState& state, const FrontEndContent& content);
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

// tour_menu.cpp: PICK TOUR after a rider is chosen (`$80:BBF7-BC0B`) or back from PICK TRACK
// (`$80:BC03`), a frame of its set-up and of its loop.
void enter_tour_menu(FrontEndState& state);
// From `$80:E550`: back from PICK TRACK (`$80:BC03`, `$00AC` = 1, sliding back), or from a
// completion, when `$00AC` as the race left it decides.
void return_to_tour_menu(FrontEndState& state, bool slides_back = true);
void tour_menu_entry_frame(FrontEndState& state, const FrontEndContent& content);
void tour_menu_frame(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads);
std::uint16_t word_at(std::span<const std::uint8_t> table, std::size_t at);
// $80:E7A1: the tour at `cursor` is open to the rider.
bool tour_open(const FrontEndState& state, const FrontEndContent& content, unsigned cursor);
// $83:8D8F: tour `tour`'s 5 x 5 picture, or the "?" picture if it is locked, at text map word
// `place`.
void draw_tour_picture(FrontEndState& state, const FrontEndContent& content, unsigned tour,
                       unsigned place);

// track_menu.cpp: PICK TRACK after a tour is chosen, or back from NOW PLAYING (`$80:BC4D`).
void enter_track_menu(FrontEndState& state, bool returning);
void track_menu_entry_frame(FrontEndState& state, const FrontEndContent& content);
void track_menu_frame(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads);
void track_menu_exit_frame(FrontEndState& state, const FrontEndContent& content);

// now_playing.cpp: NOW PLAYING after a track is chosen, and Race's fade.
void enter_now_playing(FrontEndState& state);
void now_playing_entry_frame(FrontEndState& state, const FrontEndContent& content);
void now_playing_frame(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads);
void race_fade_frame(FrontEndState& state);

// race_result.cpp: after a one-player race, the menus' return and the result screen.
void begin_race_return(FrontEndState& state, const FrontEndContent& content, std::uint32_t frame,
                       const RaceTimes& times);
void race_return_frame(FrontEndState& state, const FrontEndContent& content);
void race_result_frame(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads);
// $80:88DD-8914 after a restart from the race's pause menu: the logo up, a blank text faded in,
// then NOW PLAYING (R-0060).
void race_restart_frame(FrontEndState& state);
void race_result_exit_frame(FrontEndState& state, const FrontEndContent& content,
                            FrontEndPads pads);

// award.cpp: a tour's completion (R-0059). `complete_tour` on the scoring frame; then the award's
// frames and, after PICK TOUR, the way to PICK TRACK.
void complete_tour(FrontEndState& state);
void tour_award_frame(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads);
void award_return_frame(FrontEndState& state, const FrontEndContent& content);

// lap_result.cpp: the lap result (R-0058). Its build on the result's first frame, after the
// common part; its streams on the second; one step of its graph.
void build_lap_result(FrontEndState& state, const FrontEndContent& content);
void print_lap_result(FrontEndState& state, const FrontEndContent& content);
void step_lap_graph(FrontEndState& state);
// $80:9017: the unsigned minimum of the ten lap slots, zero slots not skipped, the result's best
// lap; `$80:C868`: the minimum of the slots that are not zero, from 0xEA62, the records' best lap.
std::uint16_t best_lap(const std::array<std::uint16_t, 10>& laps);
std::uint16_t record_lap(const std::array<std::uint16_t, 10>& laps);

} // namespace unirally::front_end_screens
