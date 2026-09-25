#pragma once
// The front end from power-on to the choice of a mode (R-0054) and, for 1P, the rider menu PICK
// YOUR UNI (R-0055) and the tour menu PICK TOUR (R-0056): frame by frame as the original shows
// them. The state mirrors the original's where it is observable (the
// arrow, the palette cycle, the menus' words, the OAM buffer, the text map) so it can be compared
// with captures.
#include "presentation.hpp"
#include "rider_object.hpp"
#include "snes_screen.hpp"
#include "text_printer.hpp"

#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace unirally {

class ClassicContentPack;

// Pack content of the front end (profiles v15 and v16).
struct FrontEndContent {
    std::array<std::span<const std::uint8_t>, 256> assets{}; // by asset id; empty if not packed
    std::span<const std::uint8_t> base_palette, character_table, main_menu_text, arrow_frames;
    std::span<const std::uint8_t> menu_arrow_columns, cycle_colours;
    // The rider menu: the riders' name records (16 bytes each), its title, the decoration
    // animator's tables and the uni pictures (the race riders' pose frames and tiles).
    std::span<const std::uint8_t> rider_names, rider_menu_title, decoration_frames;
    RiderObjectContent uni_pictures;
    // The tour menu (profile v17).
    std::span<const std::uint8_t> medal_tiles, tour_menu_text, tour_badge_places, tour_levels;
    std::span<const std::uint8_t> tour_arrow_targets, tour_badge_pictures, medal_places;
    std::span<const std::uint8_t> medal_attributes, tour_names;
    // PICK TRACK and NOW PLAYING (profile v17), and the race tracks' names.
    std::span<const std::uint8_t> track_menu_tiles, track_menu_layout, track_menu_text, medal_words;
    std::span<const std::uint8_t> marker_tiles, race_kind_words, now_playing_text, time_words;
    std::span<const std::uint8_t> laps, qualifying_scores, track_names;
};
FrontEndContent front_end_content(const ClassicContentPack& pack);

// The main menu's entries, in `$009B` order (COVERAGE-ROADMAP), and the demo it starts when idle;
// then where the main menu's two codes lead (not `$009B` values): the WIPE RAM menu
// (`$80:A9B4`) and code not yet read (`$80:F0D6`).
enum class FrontEndMode : std::uint8_t {
    one_player,
    two_player,
    versus,
    league,
    options,
    demo,
    wipe_ram_code,
    unread_code
};

// The menu arrow ($80:FAF5): position and target in sixteenths of a pixel, and its spin.
struct MenuArrow {
    std::uint16_t x{}, target_x{}, y{}, target_y{}; // $0C60, $0C62, $0C68, $0C6A
    std::uint8_t spin{};                            // $00C6, 31 down to 0
};

// The palette cycle the NMI runs from the title on ($80:FA60).
struct PaletteCycle {
    std::int8_t delay{}; // $00C8: frames to the next step, 6 down to 0
    std::int8_t phase{}; // $00C9: 3 down to 0
    bool running{};      // the NMI hook is installed and NMIs are enabled
};

// The NMI hook `$80:F622` slides the main menu's logo (BG1) up while a screen is on top of it
// and back down after.
struct LogoScroll {
    std::uint8_t offset{}; // $00AD: BG1's vertical offset, 0 to 0x52 in steps of 2
    bool raised{};         // $77:0742 bit 1: slide up (set) or down
};

// The slide between two screens' texts ($80:E233 forward, $80:E27E back): BG2's map is two
// 32-column halves; the new text is copied into the hidden half, which scrolls into view.
struct ScreenSlide {
    std::int8_t countdown{-1}; // $0076: passes left, 38 down to 0, then -1
    std::uint8_t speed{};      // $00A1: pixels a pass, 1 up to 8 and back down (256 in all)
    bool back{};
    std::uint16_t scroll{}; // $0090: BG2's horizontal offset (R-0055)
    std::uint16_t hidden_half{0x1400}, shown_half{0x1000}; // $00A8, $00AA: VRAM words
};

// The main menu's decoration animator ($83:9A1E), which a forward slide runs once a pass. Its
// objects (entries 96-114) are all hidden then, so only the OAM buffer shows it.
struct MenuDecorations {
    std::uint8_t delay{};      // $0089's low byte (the main menu's idle count): 2 down to 0
    std::uint8_t pair_step{};  // $0193: entries 96-97's tile, 7 down to 0
    std::uint8_t pair_cycle{}; // $0194: entries 98-99's tile, a step each time $0193 wraps
    std::uint8_t trio_step{};  // $0191: entries 112-114's tiles, 7 down to 0
    std::uint8_t wave_delay{}; // $008B: 1 down to 0
    std::array<std::uint8_t, 8> wave{}; // $0187-$018E: entries 104-111's tiles, 19 down to 0
    std::uint8_t sway{};                // $0192: entries 100-103's columns, 19 down to 0
};

// The menus' movement latches, the byte $008F: a direction is held since it last moved the
// arrow. The main menu, PICK TRACK and NOW PLAYING write the whole byte, 1 or 0 (`moved`); PICK
// YOUR UNI and PICK TOUR keep Up and Down apart in bits 3 and 2.
struct MenuLatches {
    bool moved{}, up{}, down{};
};

// PICK YOUR UNI ($80:CB04): 16 riders in two columns of eight, rider r at row r / 2, column
// r % 2. The column is the arrow's x target's.
struct RiderMenu {
    std::uint8_t rider{};     // $017D: the rider chosen last; the arrow starts on it
    std::uint8_t row{};       // $000E
    std::uint16_t intro{};    // $0076: the uni's build-up, picture intro / 2; 0 once it loops
    std::uint8_t idle_step{}; // $0190: the loop's step, 39 down to 0
    std::uint16_t picture{};  // the uni picture built for the next frame ($83:8E3A)
    bool back{};              // left with Y or X rather than chosen
    bool returning{};         // entered back from PICK TOUR ($00AC != 2): slides back in
};

// The one-player records the menus read from SRAM, as a cold start leaves them (`$80:8C4E`):
// no levels, medals or done tracks, every rider's best 9:59.99 on the races (0 on the stunt
// events, `$83:9340`) and SOMEONE holding every record (`$83:93D8`). Winning races changes
// them, which is later work (R-0056).
struct OnePlayerRecords {
    std::array<std::uint8_t, 16> tour_levels{};   // $77:10D3 + rider: 0-3, the tours open
    std::array<std::uint8_t, 160> medals{};       // $77:069C + 16 * tour + rider: 0, or 1-3
    std::array<std::uint8_t, 50> tracks_done{};   // $77:1075 + track: won in the current run
    std::array<std::uint16_t, 800> best{};        // $77:0829 + 2 * (50 * rider + track)
    std::array<std::uint8_t, 50> record_holder{}; // $77:0550 + track: a rider, 16 SOMEONE
};
OnePlayerRecords cold_start_records();

// PICK TOUR ($80:E550): the tours in two columns of four, HUNTER below. The cursor is 2 * row +
// column; 8 is HUNTER, 9 HUNTER reached from the right column.
struct TourMenu {
    std::uint8_t tour{};   // $00D0: the tour chosen last; the arrow starts on it
    std::uint8_t cursor{}; // $009B
    std::uint8_t track{};  // $00CE: the first track of the tour the menu was entered with
    std::uint8_t medal{};  // $77:10D1: the rider's medal on the chosen tour
    bool back{};           // left with Y or X
    bool returning{};      // entered back from PICK TRACK ($00AC != 2): slides back in
};

// PICK TRACK ($80:E84E): the tour's five tracks, then (in 1P) the medal line, which steps the
// medal to race for.
struct TrackMenu {
    std::uint8_t cursor{};      // $009B: 0-4 the tracks, 5 the medal line
    bool medal_latched{};       // $77:10D2: the medal line has stepped on this press
    bool medal_stepped{};       // it stepped this frame: the next frame shows it ($80:EA8F)
    std::uint8_t marker_step{}; // $77:10A7: the done-track markers' animation, 13 down to 0
    bool back{};
    bool returning{}; // entered back from NOW PLAYING ($00AC == 2): slides back in
};

// NOW PLAYING ($80:B18D): the rider against the opponent on the chosen track, Race or Exit.
enum class NowPlayingChoice : std::uint8_t { none, race, exit, back };
struct NowPlaying {
    std::uint8_t opponent{0x10};  // $017F: BRONSEN, SILVIA, GOLDWYN (0x11-0x13) or ANTI-UNI 0x14
    std::uint8_t record_holder{}; // $018F
    NowPlayingChoice choice{};
};

struct MainMenu {
    std::uint8_t selection{}; // $009B
    std::int16_t idle{};      // $0089: frames left before the demo
};

// Where the front end is. The boot, the steps between screens and the way back to the main menu
// are scripts of fixed frames, counted by `FrontEndState::script_frame`.
enum class FrontEndScreen : std::uint8_t {
    boot,              // power-on to the main menu (R-0054)
    main_menu,         // $80:ABC8's loop
    rider_menu_entry,  // $80:BB9C and $80:CB04's set-up: the names printed and slid in
    rider_menu,        // $80:CBC8's loop: PICK YOUR UNI
    rider_menu_exit,   // $80:F4E9, after a choice or Y
    main_menu_return,  // $80:ACD5 with $00A7 set: the main menu slid back in
    tour_menu_entry,   // $80:A858, $80:A82B and $80:E550's set-up: the tours slid in
    tour_menu,         // $80:E5B4's loop: PICK TOUR
    track_menu_entry,  // $80:E84E's set-up: the tracks slid in
    track_menu,        // $80:BAA4's loop: PICK TRACK
    track_menu_exit,   // $80:E9FC, after a choice or Y
    now_playing_entry, // $80:B18D's set-up: the match slid in
    now_playing,       // $80:B467's loop: NOW PLAYING
    race_fade,         // $80:9885 after Race, then the race
};

struct FrontEndState {
    std::uint32_t frame{}; // frames since power-on; the next update is this frame
    FrontEndScreen screen{};
    std::uint32_t script_frame{}; // frames since the current script began
    SnesVideoMemory video{};
    SnesVideoRegisters registers{};
    // The colours HDMA writes during this frame's picture; they stay in CGRAM after it.
    std::vector<SnesLineColour> line_colours;
    std::array<std::uint8_t, 544> oam_buffer{}; // $0A00, copied to OAM by DMA
    TextMap text{};                             // $0200, copied to BG2's map
    TextCursor printer{};                       // $009F and $00B0, kept between prints
    MenuArrow arrow{};
    PaletteCycle cycle{};
    LogoScroll logo{};
    ScreenSlide slide{};
    MenuDecorations decorations{};
    MainMenu menu{};
    MenuLatches latches{};
    RiderMenu rider_menu{};
    OnePlayerRecords records = cold_start_records();
    TourMenu tour_menu{};
    TrackMenu track_menu{};
    NowPlaying now_playing{};
    bool mode_chosen{};
    // For 1P, once NOW PLAYING's Race has faded out: the race is `tour_menu.track` for
    // `rider_menu.rider` against `now_playing.opponent`.
    FrontEndMode mode{};
};

// Controller words as the auto-joypad read gives them (`$4218`, `$421A`): B 0x8000, Y 0x4000,
// Select 0x2000, Start 0x1000, Up 0x0800, Down 0x0400, Left 0x0200, Right 0x0100, A 0x0080,
// X 0x0040, L 0x0020, R 0x0010.
struct FrontEndPads {
    std::uint16_t one{}, two{};
};

FrontEndState start_front_end();
// One frame: its vblank's work, in the original's order. Once a mode is chosen (for 1P, a race)
// the state stops.
void update_front_end(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads);
// The frame the last update produced.
RgbFrame render_front_end(const FrontEndState& state);

} // namespace unirally
