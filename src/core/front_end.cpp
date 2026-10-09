#include "front_end.hpp"

#include "content_pack.hpp"
#include "front_end_screens.hpp"
#include "text_printer.hpp"

#include <algorithm>
#include <array>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

// The front end from power-on to the main menu's choice (R-0054). The boot is a fixed script:
// the frames on which the original loads each screen, waits for a frame, fades and so on are
// those of a cold start under the reference emulator (the loads and the sound program's upload
// take a fixed number of frames). The main menu then runs its loop once per frame; 1P leads to
// the rider menu (rider_menu.cpp).
namespace unirally {

namespace front_end_screens {
namespace {

// Asset ids (the directory at $82:B332) and where the front end loads them.
constexpr unsigned early_objects = 88, early_palette = 5;                         // $80:A09A
constexpr unsigned nintendo_palette = 31, nintendo_map = 80, nintendo_tiles = 74; // $80:B08C
constexpr unsigned title_palette = 27, title_map = 78, title_tiles = 72;          // $80:F55F
constexpr unsigned menu_palette = 1, menu_object_palette = 2, menu_text_palette = 28;
constexpr unsigned menu_objects = 89, menu_objects_high = 91, menu_map = 77, menu_bg2_tiles = 70,
                   menu_bg1_tiles = 68, menu_bg1_tiles_high = 69; // $80:D20E
constexpr std::size_t menu_text_tiles_dma = 1920;                 // $80:A877: asset 69's start

// The arrow's parking place, off the left of the screen ($83:99FA), and where the main menu and
// the screen setup `$80:A16A` send it: the 1P entry.
constexpr std::uint16_t parked_x = 0xfd00, parked_y = 0x0700;
constexpr std::uint16_t first_entry_x = 0x04fd, first_entry_y = 0x0580;
constexpr std::uint16_t entry_spacing = 0x0180; // 24 pixels in sixteenths
constexpr std::uint16_t wrap_to_first_y = 0x0400, wrap_to_last_y = 0x0d00;
constexpr std::uint8_t arrow_y_offset = 0x12, shadow_offset = 7;
constexpr std::uint16_t shadow_sign_offset = 0x0070;

// The main menu ($80:ABC8).
constexpr std::int16_t first_idle = 480, idle_after_move = 1500;
constexpr std::uint8_t menu_entries = 5;
constexpr std::uint16_t choose_buttons = 0x9080, down_buttons = 0x2400, up_buttons = 0x0800;
// The two codes: Left, A, L and R (the WIPE RAM menu); B, Down, L and R (HUNTER's ending).
constexpr std::uint16_t wipe_ram_code = 0x02b0, hunter_ending_code = 0x8430;

// Fades ($80:9869, $80:9885): seven frames of brightness 2, 4, ..., 14, or 13, 11, ..., 1.
constexpr unsigned fade_frames = 7;

// The boot's frames (R-0054's frame model).
constexpr std::uint32_t nintendo_registers_frame = 97, nintendo_load_frame = 99,
                        nintendo_layers_frame = 103, nintendo_fade_in = 104,
                        nintendo_fade_out = 222, title_load_frame = 228, cycle_start_frame = 250,
                        title_fade_in = 251, title_fade_out = 369, menu_load_frame = 377,
                        menu_objects_frame = 402, menu_map_filled_frame = 403,
                        menu_map_copy_frame = 407, menu_palette_frame = 408, menu_tiles_frame = 409,
                        menu_fade_in = 410, menu_palette_again_frame = 418, main_menu_frame = 419;

// $80:FAF5: spin, then move a quarter of the way to the target. The quarter is an arithmetic
// shift, so a positive gap under 4 moves nothing: x settles three sixteenths short.
std::uint16_t quarter_step(std::uint16_t gap) {
    std::uint16_t step = static_cast<std::uint16_t>(gap >> 2U);
    if (step & 0x2000U) step |= 0xc000U;
    return step;
}

void update_arrow(FrontEndState& state, const FrontEndContent& content) {
    auto& arrow = state.arrow;
    arrow.spin = arrow.spin == 0 ? 31 : static_cast<std::uint8_t>(arrow.spin - 1);
    const auto tile = content.arrow_frames[arrow.spin >> 1U];
    state.oam_buffer[arrow_entry * 4 + 2] = tile;
    state.oam_buffer[shadow_entry * 4 + 2] = tile;
    if (arrow.target_x != arrow.x) {
        arrow.x = static_cast<std::uint16_t>(
            arrow.x + quarter_step(static_cast<std::uint16_t>(arrow.target_x - arrow.x)));
        set_oam_x_high(state, arrow_entry, (arrow.x & 0x8000U) != 0);
        set_oam_x_high(state, shadow_entry, ((arrow.x + shadow_sign_offset) & 0x8000U) != 0);
        const auto column = static_cast<std::uint8_t>(arrow.x >> 4U);
        state.oam_buffer[arrow_entry * 4] = column;
        state.oam_buffer[shadow_entry * 4] = static_cast<std::uint8_t>(column + shadow_offset);
    }
    if (arrow.target_y != arrow.y) {
        arrow.y = static_cast<std::uint16_t>(
            arrow.y + quarter_step(static_cast<std::uint16_t>(arrow.target_y - arrow.y)));
        const auto row = static_cast<std::uint8_t>((arrow.y >> 4U) - arrow_y_offset);
        state.oam_buffer[arrow_entry * 4 + 1] = row;
        state.oam_buffer[shadow_entry * 4 + 1] = static_cast<std::uint8_t>(row + shadow_offset);
    }
}

// $80:FA60, from the NMI: every seventh frame the four colours 108-111 rotate by one.
void step_palette_cycle(FrontEndState& state, const FrontEndContent& content) {
    auto& cycle = state.cycle;
    if (--cycle.delay >= 0) return;
    cycle.delay = 6;
    if (--cycle.phase < 0) cycle.phase = 3;
    for (unsigned k = 0; k < 4; ++k) {
        const auto from = static_cast<std::size_t>(cycle.phase) * 2 + k * 2;
        const auto colour = static_cast<std::size_t>(111 - k) * 2;
        state.video.cgram[colour] = content.cycle_colours[from];
        state.video.cgram[colour + 1] = content.cycle_colours[from + 1];
    }
}

void send_arrow_to_first_entry(FrontEndState& state) {
    state.arrow.target_x = first_entry_x;
    state.arrow.target_y = first_entry_y;
}

// $80:A16A: the direct-page state a screen starts from.
void reset_screen_state(FrontEndState& state) {
    send_arrow_to_first_entry(state);
    state.cycle.delay = 0;
    state.cycle.phase = 0;
    state.menu = {};
}

void load_nintendo_screen(FrontEndState& state, const FrontEndContent& content) {
    load_cgram(state, content.base_palette, 0); // $80:A8A8's colours
    park_arrow(state);                          // $83:99F6
    state.registers.force_blank = true;
    load_cgram(state, asset(content, nintendo_palette), 0);
    load_vram(state, asset(content, nintendo_map), 0x1000);
    load_vram(state, asset(content, nintendo_tiles), 0x2000);
}

// The title ($80:F55F at 228): BG1 only, 8bpp, its own 256 colours. After the title code
// (`$80:F564-F580`) the levels it saved come back and its flag is cleared.
void load_title(FrontEndState& state, const FrontEndContent& content) {
    reset_screen_state(state); // $80:A16A
    auto& records = state.records;
    if (records.cheat) {
        records.tour_levels = records.levels_before_cheat;
        records.cheat = false;
    }
    state.title_code_step = 0;
    auto& r = state.registers;
    r.force_blank = true;
    set_tile_bases(r, 0x01);
    r.main_screen = 0x01;
    r.mode = 3;
    r.obsel = 0x63;
    load_cgram(state, asset(content, title_palette), 0);
    load_vram(state, asset(content, title_map), 0);
    load_vram(state, asset(content, title_tiles), 0x1000);
}

// $80:ACD5 (405-407): the menu text, printed into the cleared map ($0200) and copied to BG2's
// shown half.
void copy_menu_text(FrontEndState& state, const FrontEndContent& content) {
    print_main_menu(state, content);
    load_text(state, state.slide.shown_half);
}

// The boot's fades: boot frame `frame`, the fade's first `first_frame`.
void fade(FrontEndState& state, std::uint32_t frame, std::uint32_t first_frame, bool in) {
    const auto step = frame - first_frame;
    copy_oam(state);
    if (in) {
        state.registers.brightness = static_cast<std::uint8_t>(2 * (step + 1));
        state.registers.force_blank = false;
        return;
    }
    state.registers.brightness = static_cast<std::uint8_t>(13 - 2 * step);
    if (step == fade_frames - 1) state.registers.force_blank = true;
}

// A pad's rocker D-pad cannot report opposing directions (bsnes `sfc/controller/gamepad`, as
// `with_physical_dpad` for the race, R-0041): Up with Down, or Left with Right, reads as neither.
std::uint16_t physical_pad(std::uint16_t word) {
    constexpr std::uint16_t up = 0x0800, down = 0x0400, left = 0x0200, right = 0x0100;
    if ((word & up) && (word & down)) word = static_cast<std::uint16_t>(word & ~(up | down));
    if ((word & left) && (word & right)) word = static_cast<std::uint16_t>(word & ~(left | right));
    return word;
}

bool within(std::uint32_t frame, std::uint32_t first, std::uint32_t count) {
    return frame >= first && frame < first + count;
}

// The frames on which the original waits for vblank in `$80:FADF` (and so moves the arrow):
// first frame and count; the last range runs on (R-0054's frame model).
struct FrameWaits {
    std::uint32_t first, count;
};
constexpr std::array<FrameWaits, 5> boot_frame_waits{{
    {98, 2},    // $80:B08C, then $80:A8A8
    {104, 125}, // the Nintendo screen's fades and hold
    {251, 127}, // the title's fades and hold, then $80:D20E and $80:A8A8
    {403, 1},   // $80:D36F
    {407, 0},   // $80:ACD5 and the main menu, every frame from here
}};

// The boot's frame: the frames since power-on, or since a soft reset. After a reset the sound
// program's upload (`$80:A09A`) ends `reset_upload_delay` frames later than at power-on, so the
// boot's frames 97-403 come that much later; the records' wipe (404-406), which the reset does not
// need, is skipped (HUNTER-ENDING). Empty on the frames the upload adds.
std::optional<std::uint32_t> boot_frame_number(const FrontEndState& state) {
    constexpr std::uint32_t wipe_frames = 3;
    const auto delay = state.reset_upload_delay;
    const auto frame = state.frame - state.boot_start;
    if (!state.after_soft_reset || frame < nintendo_registers_frame) return frame;
    if (frame < nintendo_registers_frame + delay) return std::nullopt;
    if (frame <= menu_map_filled_frame + delay) return frame - delay;
    return frame - delay + wipe_frames;
}

bool waits_for_frame(const FrontEndState& state);
// Whether the frame just run ended in a frame wait, which polls the sound queue ($80:FADF,
// $83:A923; R-0076): the next frame starts after one, except that the `$83:A923` waits that
// leave the arrow alone (the award's, the ending's, and `$83:879A`'s before the result's
// scoring) poll it too. A race's frames are its own.
bool ends_with_queue_wait(const FrontEndState& state) {
    if (state.screen == FrontEndScreen::race) return false;
    if (state.screen == FrontEndScreen::race_result_exit
        && state.script_frame + 1 == result_scoring_frame(state))
        return true;
    if (state.screen == FrontEndScreen::tour_award) return award_frame_waits(state);
    if (state.screen == FrontEndScreen::tour_ending) return tour_ending_frame_waits(state);
    if (state.screen == FrontEndScreen::hunter_ending) return hunter_ending_queue_waits(state);
    return waits_for_frame(state);
}

bool waits_for_frame(const FrontEndState& state) {
    // After a race NMI is off until `$80:D377` (R-0057) but for the sound upload's last frame and
    // `$80:D20E`'s first; then from the OAM copy on. `$83:879A`'s frame waits (`$83:A923`) leave
    // the arrow alone.
    const auto next = state.script_frame + 1;
    if (state.screen == FrontEndScreen::demo_title)
        return next <= 9 || next == 35 || (next >= 134 && next <= 458);
    if (state.screen == FrontEndScreen::demo_return) return next == 75 || next == 76 || next >= 103;
    if (state.screen == FrontEndScreen::race_return)
        return next == upload_last_frame || next == menu_screen_frame || next >= restore_frame;
    if (state.screen == FrontEndScreen::race_result_exit)
        return next == 1 || next > result_scoring_frame(state);
    // The award's waits are `$83:A923`'s, which leave the arrow alone.
    if (state.screen == FrontEndScreen::tour_award || state.screen == FrontEndScreen::tour_ending)
        return false;
    if (state.screen == FrontEndScreen::hunter_ending) return hunter_ending_waits(state);
    // The lap result's second frame (`$80:8FFD-910E`) runs past its frame's end, so the tail
    // after it starts without a frame wait (R-0058).
    if (state.screen == FrontEndScreen::race_result && state.race_result.times.lap_race)
        return next != result_tail_frame;
    // So does the stunt result's (R-0067).
    if (state.screen == FrontEndScreen::stunt_result) return next != stunt_text_frame;
    // $80:D494-D4BF runs through the name save and return without a frame wait.
    if (state.screen == FrontEndScreen::rename_return) return false;
    if (state.screen == FrontEndScreen::define_player_after_confirm) return false;
    if (state.screen == FrontEndScreen::records_detail_entry && next == 1) return false;
    if (state.screen == FrontEndScreen::league_podium_entry) return next <= 7;
    if (state.screen == FrontEndScreen::league_podium) return false;
    if (state.screen == FrontEndScreen::league_podium_exit)
        return next == upload_last_frame + 2 || next == menu_screen_frame + 2
            || next >= restore_frame + 2;
    // Every other screen after the boot waits for each frame.
    if (state.screen != FrontEndScreen::boot) return true;
    const auto frame = boot_frame_number(state);
    if (!frame) return false;
    for (const auto& waits : boot_frame_waits) {
        if (waits.count == 0 && *frame >= waits.first) return true;
        if (within(*frame, waits.first, waits.count)) return true;
    }
    return false;
}

// $80:F5C0-F612, a pass of the title's 111: pad 1's word matching the code's next word
// (`$80:F618`: Up, Left, Up, R, A) moves the code on, and any other word leaves it where it is.
// The fifth saves the levels, opens every tour (level 3) and sets the flag `$77:10D0`, then calls
// `$80:B124`, which is not read. The word after the code (`$80:F622`'s code bytes) has bit 1 set,
// which no pad gives. On a cold start the records' wipe (boot frame 403) undoes it all.
void check_title_code(FrontEndState& state, std::uint16_t pad) {
    constexpr std::array<std::uint16_t, 5> title_code{pad_up, pad_left, pad_up, pad_r, pad_a};
    auto& step = state.title_code_step;
    if (step >= title_code.size() || pad != title_code[step]) return;
    if (++step < title_code.size()) return;
    auto& records = state.records;
    records.levels_before_cheat = records.tour_levels;
    records.tour_levels.fill(3);
    records.cheat = true;
}

// $80:8C4E (boot frame 403): cartridge RAM that does not start with the signature `$83:8000` is
// wiped to a cold start's records (`$83:FB41`, frames 403-405). Native's power-on has no records
// of its own, so it always wipes; after a soft reset the signature is there. Then `$83:8B23`
// clears the one-player flag `$77:10AD` and the pending reveal.
void check_records(FrontEndState& state, const FrontEndContent& content) {
    if (!state.after_soft_reset) {
        state.records = cold_start_records();
        if (content.rider_names.size() != state.records.rider_names.size())
            throw std::invalid_argument("the original rider-name table has the wrong size");
        std::copy(content.rider_names.begin(), content.rider_names.end(),
                  state.records.rider_names.begin());
        if (content.league_names.size() != state.records.league_names.size())
            throw std::invalid_argument("the original league-name table has the wrong size");
        std::copy(content.league_names.begin(), content.league_names.end(),
                  state.records.league_names.begin());
    }
    state.one_player = false;
    state.records.pending_reveal = 0;
}

// The boot's scripted work for one frame, after the NMI and the frame wait.
void boot_frame(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads) {
    const auto frame = boot_frame_number(state);
    if (!frame) return;
    const auto f = *frame;
    // After a soft reset the boot's sound program load (`$80:A09A`) stops the running driver
    // first: a session from a running driver, as after a race (R-0077, `all-gold`).
    constexpr std::uint32_t reset_sound_load_frame = 28;
    if (state.after_soft_reset && f == reset_sound_load_frame)
        state.sound_cues.push_back(audio_load(AudioSessionLoad::reset_boot));
    if (f == 24) {
        clear_oam_buffer(state);
        load_cgram(state, asset(content, early_palette), 0xe0);
        load_vram(state, asset(content, early_objects), 0x7000);
    } else if (f == nintendo_registers_frame) {
        set_early_registers(state);
    } else if (f == nintendo_load_frame) {
        load_nintendo_screen(state, content);
    } else if (f == nintendo_layers_frame) {
        state.registers.main_screen = 0x02;
        state.registers.sub_screen = 0;
    } else if (within(f, nintendo_fade_in, fade_frames)) {
        fade(state, f, nintendo_fade_in, true);
    } else if (within(f, nintendo_fade_out, fade_frames)) {
        fade(state, f, nintendo_fade_out, false);
        if (f == title_load_frame) load_title(state, content);
    } else if (within(f, title_fade_in, fade_frames)) {
        fade(state, f, title_fade_in, true);
    } else if (f > title_fade_in + fade_frames - 1 && f < title_fade_out) {
        copy_oam(state); // $80:D1EC
        check_title_code(state, pads.one);
    } else if (within(f, title_fade_out, fade_frames)) {
        fade(state, f, title_fade_out, false);
    } else if (f == menu_load_frame) {
        load_main_menu_screen(state, content);
    } else if (f == menu_objects_frame) {
        lay_out_menu_objects(state);
    } else if (f == menu_map_filled_frame) {
        copy_oam(state); // $80:D372
        check_records(state, content);
    } else if (f == menu_map_copy_frame) {
        copy_menu_text(state, content);
    } else if (f == menu_palette_frame || f == menu_palette_again_frame) {
        reload_menu_palette(state, content);
    } else if (f == menu_tiles_frame || f == main_menu_frame) {
        reload_menu_text_tiles(state, content);
        if (f == main_menu_frame) start_main_menu(state);
    } else if (within(f, menu_fade_in, fade_frames)) {
        fade(state, f, menu_fade_in, true);
    }
}

// One pass of the main menu's loop ($80:ABE3), after its frame wait.
void run_main_menu(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads) {
    copy_oam(state); // $80:D1EC
    // The codes come first, as exact words on either pad, the WIPE RAM code before the other
    // (`$80:ABEB-AC0A`).
    const auto entered = [&](std::uint16_t code) { return pads.one == code || pads.two == code; };
    if (entered(wipe_ram_code)) {
        state.mode_chosen = true;
        state.mode = FrontEndMode::wipe_ram_code;
        return;
    }
    if (entered(hunter_ending_code)) {
        enter_hunter_code(state);
        return;
    }
    auto& menu = state.menu;
    // `$80:AC0F-AC14`: the count is stored only while it stays positive; the demo is mode 5.
    if (menu.idle == 0) {
        menu.selection = static_cast<std::uint8_t>(FrontEndMode::demo);
        state.mode = FrontEndMode::demo;
        state.screen = FrontEndScreen::demo_title;
        return;
    }
    --menu.idle;
    const auto pressed = [&](std::uint16_t buttons) {
        return (pads.one & buttons) != 0 || (pads.two & buttons) != 0;
    };
    if (pressed(choose_buttons)) {
        const auto mode = static_cast<FrontEndMode>(menu.selection);
        if (mode == FrontEndMode::one_player || mode == FrontEndMode::two_player
            || mode == FrontEndMode::versus) {
            state.mode = mode;
            enter_rider_menu(state);
            return;
        }
        if (mode == FrontEndMode::league) {
            state.mode = mode;
            state.logo.raised = true;
            enter_league_slots(state, content);
            return;
        }
        if (mode == FrontEndMode::options) {
            enter_options_menu(state, content);
            return;
        }
        state.mode_chosen = true;
        state.mode = mode;
        return;
    }
    const bool down = pressed(down_buttons);
    const bool up = !down && pressed(up_buttons);
    if (!down && !up) {
        state.latches = {};
        return;
    }
    if (state.latches.moved) return;
    state.latches = {.moved = true};
    play_menu_sound(state, MenuSound::navigate); // $80:AC3F
    menu.idle = idle_after_move;
    if (down) {
        if (++menu.selection >= menu_entries) {
            menu.selection = 0;
            state.arrow.target_y = wrap_to_first_y;
        }
    } else if (menu.selection-- == 0) {
        menu.selection = menu_entries - 1;
        state.arrow.target_y = wrap_to_last_y;
    }
    state.arrow.target_x =
        static_cast<std::uint16_t>(content.menu_arrow_columns[menu.selection] << 7U);
    state.arrow.target_y = static_cast<std::uint16_t>(down ? state.arrow.target_y + entry_spacing
                                                           : state.arrow.target_y - entry_spacing);
}

// $80:F622, the NMI hook: the logo slides 2 lines a frame, up to 0x52 or down to 0.
void scroll_logo(FrontEndState& state) {
    constexpr std::uint8_t logo_top = 0x52;
    auto& logo = state.logo;
    const auto next = static_cast<std::uint8_t>(logo.raised ? logo.offset + 2 : logo.offset - 2);
    if (logo.raised ? next > logo_top : (next & 0x80U) != 0) return;
    logo.offset = next;
    state.registers.bg[0].vofs = next;
}

// The colours HDMA wrote during the last picture stay in CGRAM, and the registers it wrote hold
// their last values.
void keep_line_writes(FrontEndState& state) {
    for (const auto& colour : state.line_colours) {
        state.video.cgram[colour.index * 2U] = static_cast<std::uint8_t>(colour.colour);
        state.video.cgram[colour.index * 2U + 1] = static_cast<std::uint8_t>(colour.colour >> 8U);
    }
    state.line_colours.clear();
    for (const auto& write : state.line_registers) apply_line_register(state.registers, write);
    state.line_registers.clear();
}

} // namespace

void set_background(SnesBackground& bg, std::uint8_t sc) {
    bg.map_word = static_cast<std::uint16_t>((sc & 0xfcU) << 8U);
    bg.map_size = sc & 3U;
}

void set_tile_bases(SnesVideoRegisters& registers, std::uint8_t nba) {
    registers.bg[0].tile_word = static_cast<std::uint16_t>((nba & 0x0fU) << 12U);
    registers.bg[1].tile_word = static_cast<std::uint16_t>((nba >> 4U) << 12U);
}

void run_nmi_hook(FrontEndState& state, const FrontEndContent& content) {
    scroll_logo(state);
    step_palette_cycle(state, content);
    if (state.cycle.pending_nmi) {
        state.cycle.pending_nmi = false;
        step_palette_cycle(state, content);
    }
}

// $80:A09A (frame 24): every OAM entry at (1, 1), every high bit 0x55; the early loads.
void clear_oam_buffer(FrontEndState& state) {
    for (std::size_t entry = 0; entry < 128; ++entry) {
        state.oam_buffer[entry * 4] = 1;
        state.oam_buffer[entry * 4 + 1] = 1;
    }
    std::fill(state.oam_buffer.begin() + oam_high_table, state.oam_buffer.end(),
              std::uint8_t{0x55});
}

// The Nintendo screen ($80:A09A registers at 97, $80:B08C loads at 99): BG2 only, 4bpp.
void set_early_registers(FrontEndState& state) {
    auto& r = state.registers;
    set_background(r.bg[0], 0x02);
    set_background(r.bg[1], 0x13);
    set_tile_bases(r, 0x23);
    r.main_screen = 0x11;
    r.mode = 3;
    r.obsel = 0x63;
    r.sub_screen = 0x10;
    r.colour_select = 0x02;
    r.colour_math = 0x00;
}

void load_object_palette(FrontEndState& state, const FrontEndContent& content) {
    load_cgram(state,
               asset(content, state.one_player
                                  ? first_rider_palette + (state.rider_menu.rider & 15U)
                                  : menu_object_palette),
               0xf0);
}

void set_menu_registers(SnesVideoRegisters& r) {
    set_background(r.bg[0], 0x02);
    set_background(r.bg[1], 0x13);
    set_tile_bases(r, 0x23);
    r.main_screen = 0x13;
    r.mode = 3;
    r.obsel = 0x63;
    r.sub_screen = 0x10;
    r.colour_select = 0x02;
    r.colour_math = 0x7f;
}

// The main menu's screen ($80:D20E at 377): BG1 (8bpp logo) and BG2 (4bpp checks and text)
// with the objects, which the subscreen adds at half (the arrow's shadow).
void load_main_menu_screen(FrontEndState& state, const FrontEndContent& content) {
    load_cgram(state, content.base_palette, 0); // $80:A8A8
    state.registers.force_blank = true;
    reset_screen_state(state); // $80:A16A
    set_menu_registers(state.registers);
    load_cgram(state, asset(content, menu_palette), 0x70);
    load_object_palette(state, content); // $83:91F7
    load_cgram(state, asset(content, menu_text_palette), 0xd0);
    load_vram(state, asset(content, menu_objects), 0x6000);
    load_vram(state, asset(content, menu_objects_high), 0x7d00);
    load_vram(state, asset(content, menu_map), 0);
    load_vram(state, asset(content, menu_bg2_tiles), 0x2000);
    load_vram(state, asset(content, menu_bg1_tiles), 0x31a0);
    load_vram(state, asset(content, menu_bg1_tiles_high), 0x3d80);
}

void restore_menu_screen(FrontEndState& state, const FrontEndContent& content) {
    auto& r = state.registers;
    set_menu_registers(r);
    load_cgram(state, asset(content, menu_palette), 0x70);
    load_object_palette(state, content); // $83:91FB
    load_cgram(state, asset(content, menu_text_palette), 0xd0);
    load_cgram(state, asset(content, base_palette_low), 0);
    load_cgram(state, asset(content, base_palette_high), 0x40);
    load_vram(state, asset(content, menu_objects), 0x6000);
    load_vram(state, asset(content, early_objects), 0x7000);
    load_vram(state, asset(content, menu_objects_high), 0x7d00);
    load_vram(state, asset(content, menu_map), 0);
    load_vram(state, asset(content, menu_bg2_tiles), 0x2000);
    load_vram(state, asset(content, menu_bg1_tiles_high), 0x3d80);
    load_vram(state, asset(content, menu_bg1_tiles), 0x31a0); // after 0x45, over its end
    state.text.words.fill(cleared_text);
    load_text(state, state.slide.shown_half);
    state.arrow.x = 0;
    state.arrow.y = 0x0f00;
    lay_out_menu_objects(state);
    // $83:A721 reads the waiting tiles of entries 104-111 at `$9B31` in bank `$83`, the wave's
    // tiles, where `$80:D2C1` reads the code bytes at `$80:9B31`.
    for (unsigned k = 0; k < 8; ++k)
        oam_byte(state, 104 + k, 2) = content.decoration_frames[wave_tiles + 7 - k];
    copy_oam(state);          // $80:9314
    state.logo.raised = true; // $80:F53B
    state.logo.offset = 0x52;
    r.bg[0].vofs = 0x52;
    state.cycle.running = true; // NMI on (`$83:A90E`)
}

void place_printed_object(FrontEndState& state, unsigned object, unsigned position) {
    constexpr unsigned first_printed_object = 104, row_words = 32;
    const auto entry = first_printed_object + object;
    oam_byte(state, entry, 0) = static_cast<std::uint8_t>((position % row_words) * 8 - 1);
    oam_byte(state, entry, 1) = static_cast<std::uint8_t>((position / row_words) * 8 - 1);
}

void send_arrow_off(FrontEndState& state) {
    state.arrow.target_x = parked_x;
    state.arrow.target_y = parked_y;
}

std::span<const std::uint8_t> asset(const FrontEndContent& content, unsigned id) {
    const auto data = content.assets[id];
    if (data.empty()) throw std::invalid_argument("front-end asset is not in the pack");
    return data;
}

void load_vram(FrontEndState& state, std::span<const std::uint8_t> data, unsigned word) {
    for (std::size_t at = 0; at < data.size(); ++at)
        state.video.vram[(word * 2 + at) & 0xffffU] = data[at];
}

void load_cgram(FrontEndState& state, std::span<const std::uint8_t> data, unsigned colour) {
    for (std::size_t at = 0; at < data.size(); ++at)
        state.video.cgram[(colour * 2 + at) & 0x1ffU] = data[at];
}

void load_text(FrontEndState& state, unsigned word) {
    for (std::size_t k = 0; k < state.text.words.size(); ++k) {
        const auto at = (word * 2 + k * 2) & 0xffffU;
        state.video.vram[at] = static_cast<std::uint8_t>(state.text.words[k]);
        state.video.vram[at + 1] = static_cast<std::uint8_t>(state.text.words[k] >> 8U);
    }
}

std::span<const std::uint8_t> nth_string(std::span<const std::uint8_t> table, unsigned n) {
    std::size_t at = 0;
    for (unsigned k = 0; k <= n; ++k) {
        std::size_t end = at;
        while (end < table.size() && table[end] != 0xff) ++end;
        if (end == table.size())
            throw std::invalid_argument("front-end string is not in its table");
        if (k == n) return table.subspan(at, end - at);
        at = end + 1;
    }
    return {};
}

void copy_oam(FrontEndState& state) {
    state.video.oam = state.oam_buffer;
}

void set_oam_x_high(FrontEndState& state, unsigned entry, bool high) {
    auto& bits = state.oam_buffer[oam_high_table + entry / 4];
    const auto mask = static_cast<std::uint8_t>(1U << ((entry % 4) * 2));
    bits = high ? static_cast<std::uint8_t>(bits | mask) : static_cast<std::uint8_t>(bits & ~mask);
}

void park_arrow(FrontEndState& state) {
    state.arrow.x = state.arrow.target_x = parked_x;
    state.arrow.y = state.arrow.target_y = parked_y;
}

// $80:D2C1-D36C (frame 402, and after each rider menu): the main menu's object layout. Entries 100-103 wait off the right
// edge; 104-111 wait below the screen with their tiles ($80:9B31) and attributes ($80:D383);
// the arrow (palette 7, priority 3) and its shadow (palette 5, priority 0) are large.
void lay_out_menu_objects(FrontEndState& state) {
    for (std::size_t entry = 0; entry < 128; ++entry) {
        state.oam_buffer[entry * 4] = 1;
        state.oam_buffer[entry * 4 + 1] = 1;
        state.oam_buffer[entry * 4 + 2] = 0;
        state.oam_buffer[entry * 4 + 3] = 0;
    }
    std::fill(state.oam_buffer.begin() + oam_high_table, state.oam_buffer.end(),
              std::uint8_t{0x55});
    constexpr std::uint8_t arrow_start_x = 0xd0, arrow_start_y = 0x5e;
    constexpr std::uint8_t arrow_attributes = 0x3e;  // palette 7, priority 3
    constexpr std::uint8_t shadow_attributes = 0x0a; // palette 5, priority 0
    state.oam_buffer[arrow_entry * 4] = state.oam_buffer[shadow_entry * 4] = arrow_start_x;
    state.oam_buffer[arrow_entry * 4 + 1] = state.oam_buffer[shadow_entry * 4 + 1] = arrow_start_y;
    state.oam_buffer[arrow_entry * 4 + 3] = arrow_attributes;
    state.oam_buffer[shadow_entry * 4 + 3] = shadow_attributes;
    constexpr std::array<std::uint8_t, 8> waiting_tiles{32, 218, 41, 255,
                                                        0,  10,  10, 10}; // $80:9B31
    constexpr std::array<std::uint8_t, 8> waiting_attributes{31, 29, 27, 25,
                                                             23, 21, 19, 17}; // $80:D383
    for (std::size_t k = 0; k < 8; ++k) {
        const std::size_t entry = 104 + k;
        state.oam_buffer[entry * 4 + 1] = 0xef;
        state.oam_buffer[entry * 4 + 2] = waiting_tiles[7 - k];
        state.oam_buffer[entry * 4 + 3] = waiting_attributes[7 - k];
    }
    // Entries 104-111 are small; entries 100-103 carry tiles 0x0E, 0x2C, 0x2E, 0x2E at columns
    // 0x12 and 0x22, pushed past the right edge by their ninth x bit (high bits 0x55 and 0xD5:
    // the arrow and shadow large, x9 set).
    state.oam_buffer[oam_high_table + 104 / 4] = 0;
    state.oam_buffer[oam_high_table + 108 / 4] = 0;
    constexpr std::array<std::uint8_t, 4> right_edge_tiles{0x0e, 0x2c, 0x2e, 0x2e};
    constexpr std::array<std::uint8_t, 4> right_edge_columns{0x12, 0x12, 0x22, 0x22};
    for (std::size_t k = 0; k < 4; ++k) {
        state.oam_buffer[(100 + k) * 4] = right_edge_columns[k];
        state.oam_buffer[(100 + k) * 4 + 2] = right_edge_tiles[k];
    }
    constexpr std::uint8_t large_with_ninth_bit = 0xd5;
    state.oam_buffer[oam_high_table + arrow_entry / 4] = large_with_ninth_bit;
    state.oam_buffer[oam_high_table + shadow_entry / 4] = large_with_ninth_bit;
    park_arrow(state); // $83:99F6
    // The decoration animator's wave starts over ($80:D335-D343, the steps at $80:D37B).
    state.decorations.wave = {0x00, 0x03, 0x05, 0x08, 0x0a, 0x0d, 0x0f, 0x12};
    state.decorations.wave_delay = 1;
}

void print_main_menu(FrontEndState& state, const FrontEndContent& content) {
    state.text.words.fill(cleared_text);
    print_text(state.text, state.printer, content.main_menu_text, content.character_table);
}

void reload_menu_palette(FrontEndState& state, const FrontEndContent& content) {
    copy_oam(state);
    load_cgram(state, content.base_palette, 0);
}

void reload_menu_text_tiles(FrontEndState& state, const FrontEndContent& content) {
    copy_oam(state);
    load_vram(state, asset(content, menu_bg1_tiles_high).first(menu_text_tiles_dma), 0x3d80);
}

void start_main_menu(FrontEndState& state) {
    state.screen = FrontEndScreen::main_menu;
    state.one_player = false;        // $80:AD18
    state.local_result_seen = false; // A fresh local run uses its first-race loading timing.
    state.menu.selection = 0;
    state.menu.idle = first_idle;
    state.latches = {};
    send_arrow_to_first_entry(state);
}

} // namespace front_end_screens

using namespace front_end_screens;

namespace {
std::string asset_name(unsigned id) {
    std::string name = "front-end.asset.";
    name += static_cast<char>('0' + id / 100);
    name += static_cast<char>('0' + id / 10 % 10);
    name += static_cast<char>('0' + id % 10);
    return name;
}
} // namespace

namespace {

void load_options_content(FrontEndContent& content, const ClassicContentPack& pack) {
    content.options_menu_text = pack.entry("front-end.options-menu-text");
    content.options_arrow_columns = pack.entry("front-end.options-arrow-columns");
    content.records_menu_text = pack.entry("front-end.records-menu-text");
    content.records_arrow_columns = pack.entry("front-end.records-arrow-columns");
    content.rename_who_title = pack.entry("front-end.rename-who-title");
    content.define_player_who_title = pack.entry("front-end.define-player-who-title");
    content.define_player_warning = pack.entry("front-end.define-player-warning");
    content.define_player_confirm_prompt = pack.entry("front-end.define-player-confirm-prompt");
    content.rename_prompt = pack.entry("front-end.rename-prompt");
    content.keyboard_text = pack.entry("front-end.keyboard-text");
    content.league_slot_text = pack.entry("front-end.league-slot-text");
    content.league_names = pack.entry("front-end.league-names");
    content.league_warning = pack.entry("front-end.league-warning");
    content.league_title = pack.entry("front-end.league-title");
    content.league_minimum = pack.entry("front-end.league-minimum");
    content.league_maximum = pack.entry("front-end.league-maximum");
    content.league_prompt = pack.entry("front-end.league-prompt");
    content.league_table_text = pack.entry("front-end.league-table-text");
    content.league_awards_text = pack.entry("front-end.league-awards-text");
    content.league_continue_text = pack.entry("front-end.league-continue-text");
    for (const unsigned id : {0x7fU, 0x80U, 0x91U, 0x92U, 0xa2U, 0xa3U})
        content.assets[id] = pack.entry(asset_name(id));
    content.track_records_text = pack.entry("front-end.track-records-text");
    content.track_records_objects = pack.entry("front-end.track-records-objects");
    content.high_scores_text = pack.entry("front-end.high-scores-text");
    content.player_scores_text = pack.entry("front-end.player-scores-text");
    content.player_scores_values = pack.entry("front-end.player-scores-values");
    content.group_scores_text = pack.entry("front-end.group-scores-text");
    content.group_scores_empty = pack.entry("front-end.group-scores-empty");
}

} // namespace

FrontEndContent front_end_content(const ClassicContentPack& pack) {
    FrontEndContent content;
    for (const unsigned id :
         {1U, 2U, 5U, 27U, 28U, 31U, 68U, 69U, 70U, 72U, 74U, 77U, 78U, 80U, 88U, 89U, 91U})
        content.assets[id] = pack.entry(asset_name(id));
    content.base_palette = pack.entry("front-end.base-palette");
    content.character_table = pack.entry("front-end.character-table");
    content.main_menu_text = pack.entry("front-end.main-menu-text");
    content.arrow_frames = pack.entry("front-end.arrow-frames");
    content.menu_arrow_columns = pack.entry("front-end.menu-arrow-columns");
    content.cycle_colours = pack.entry("front-end.cycle-colours");
    for (unsigned id = 6; id <= 21; ++id) content.assets[id] = pack.entry(asset_name(id));
    content.rider_names = pack.entry("front-end.rider-names");
    content.rider_menu_title = pack.entry("front-end.pick-rider-title");
    content.two_player_first_title = pack.entry("front-end.two-player-first-title");
    content.two_player_second_title = pack.entry("front-end.two-player-second-title");
    content.versus_first_title = pack.entry("front-end.versus-first-title");
    content.versus_second_title = pack.entry("front-end.versus-second-title");
    content.decoration_frames = pack.entry("front-end.decoration-frames");
    content.uni_pictures = rider_object_content(pack);
    for (unsigned id = 32; id <= 36; ++id) content.assets[id] = pack.entry(asset_name(id));
    content.medal_tiles = pack.entry("front-end.medal-tiles");
    content.tour_menu_text = pack.entry("front-end.tour-menu-text");
    content.tour_badge_places = pack.entry("front-end.tour-badge-places");
    content.tour_levels = pack.entry("front-end.tour-levels");
    content.tour_arrow_targets = pack.entry("front-end.tour-arrow-targets");
    content.tour_badge_pictures = pack.entry("front-end.tour-badge-pictures");
    content.medal_places = pack.entry("front-end.medal-places");
    content.medal_attributes = pack.entry("front-end.medal-attributes");
    for (unsigned id : {22U, 23U, 24U, 25U, 26U, 37U})
        content.assets[id] = pack.entry(asset_name(id));
    content.tour_names = pack.entry("front-end.tour-names");
    content.track_menu_tiles = pack.entry("front-end.track-menu-tiles");
    content.track_menu_layout = pack.entry("front-end.track-menu-layout");
    content.track_menu_text = pack.entry("front-end.track-menu-text");
    content.medal_words = pack.entry("front-end.medal-words");
    content.marker_tiles = pack.entry("front-end.marker-tiles");
    content.race_kind_words = pack.entry("front-end.race-kind-words");
    content.now_playing_text = pack.entry("front-end.now-playing-text");
    content.time_words = pack.entry("front-end.time-words");
    content.laps = pack.entry("front-end.laps");
    content.qualifying_scores = pack.entry("front-end.qualifying-scores");
    content.track_names = pack.entry("presentation.classic.track-names.v1");
    content.result_text = pack.entry("front-end.result-text");
    content.result_fifth_row = pack.entry("front-end.result-fifth-row");
    content.local_continue_text = pack.entry("front-end.local-continue-text");
    content.vs_champions_header = pack.entry("front-end.vs-champions-header");
    content.vs_champions_row = pack.entry("front-end.vs-champions-row");
    content.pick_challenger_title = pack.entry("front-end.pick-challenger-title");
    load_options_content(content, pack);
    content.result_icons = pack.entry("front-end.result-icons");
    content.lap_result_text = pack.entry("front-end.lap-result-text");
    content.lap_result_record = pack.entry("front-end.lap-result-record");
    content.lap_result_player = pack.entry("front-end.lap-result-player");
    content.lap_result_opponent = pack.entry("front-end.lap-result-opponent");
    for (const unsigned id : {0x3bU, 0x53U, 0x54U, 0x55U, 0x56U, 0x5cU, 0x64U, 0x65U})
        content.assets[id] = pack.entry(asset_name(id));
    content.award_tables = pack.entry("front-end.award-tables");
    content.award_medal_art = pack.entry("front-end.award-medal-art");
    for (const unsigned id : {0x3cU, 0x3eU, 0x3fU, 0x40U, 0x41U, 0x42U, 0x43U, 0x4cU, 0x52U, 0x57U,
                              0x5eU, 0x5fU, 0x60U, 0x61U, 0x62U, 0x63U})
        content.assets[id] = pack.entry(asset_name(id));
    constexpr std::array<const char*, 8> endings{"crawler", "jumper", "shuffler", "bounder",
                                                 "walker",  "runner", "hopper",   "sprinter"};
    for (std::size_t tour = 0; tour < endings.size(); ++tour)
        content.ending_tables[tour] = pack.entry(std::string("front-end.ending-") + endings[tour]);
    for (const unsigned id :
         {0x00U, 0x3aU, 0x5dU, 0x66U, 0x67U, 0x68U, 0x69U, 0x6bU, 0x6cU, 0x6dU, 0x6eU, 0x6fU})
        content.assets[id] = pack.entry(asset_name(id));
    content.reveal_brightness = pack.entry("front-end.reveal-brightness");
    content.reveal_offsets = pack.entry("front-end.reveal-offsets");
    content.credits_text = pack.entry("front-end.credits-text");
    content.credits_poses = pack.entry("front-end.credits-poses");
    content.credits_objects = pack.entry("front-end.credits-objects");
    // STUNT-RESULT (profile v25): the stunt result's heads' colours (asset 0x26 + rider) and texts.
    for (unsigned id = 0x26; id <= 0x39; ++id) content.assets[id] = pack.entry(asset_name(id));
    content.stunt_result_text = pack.entry("front-end.stunt-result-text");
    content.stunt_tally_cells = pack.entry("front-end.stunt-tally-cells");
    return content;
}

OnePlayerRecords cold_start_records() {
    OnePlayerRecords records;
    constexpr std::uint16_t cold_best = 0xea5f, no_record_time = 0xea60; // 9:59.99, no time
    constexpr std::uint8_t stunt_place = 2;
    for (std::size_t k = 0; k < records.best.size(); ++k)
        records.best[k] = k % 5 == stunt_place ? 0 : cold_best;
    for (auto& holders : records.record_holders) holders.fill(someone);
    for (auto& pairings : records.league_pairings) pairings.fill(someone);
    records.league_best_holder.fill(someone);
    constexpr std::size_t riders = 16;
    std::fill_n(records.medals.begin() + hunter * riders, riders, std::uint8_t{2});
    // $83:936E: no time on the races, 0 on the stunt events.
    for (auto& times : records.record_times)
        for (std::size_t track = 0; track < times.size(); ++track)
            times[track] = track % 5 == stunt_place ? 0 : no_record_time;
    // `$77:1073` stays 0 until a rider is chosen (`$80:BBE3-BBE5`, R-0067).
    records.tries = 0;
    return records;
}

FrontEndState start_front_end() {
    return {};
}

void return_from_demo(FrontEndState& state, std::uint32_t exit_frame, std::uint16_t demo_elapsed) {
    // Saturates: only the first title differs (R-0070, R-0087).
    if (state.demo_cycles < 255) ++state.demo_cycles;
    state.frame = exit_frame + 1U;
    state.screen = FrontEndScreen::demo_return;
    state.script_frame = 0;
    // R-0070: a pad exit before the warning threshold stays blank for two
    // more pictures before the menu's return script. The timer exit does not.
    state.demo_return_interrupted = demo_elapsed != 0x076b;
    // $83:E267-E276 calls $82:8035 twice before testing a late pad press.
    // The observed late-press return has one fewer blank picture.
    state.demo_return_wait = state.demo_return_interrupted ? (demo_elapsed >= 0x0714 ? 1 : 2) : 0;
    // R-0087: after the races on these tracks the timer's return holds its frame 100 once. The
    // original's sound transfer there runs longer; its cause in the sound processor is open.
    constexpr std::array<std::uint8_t, 3> held_return_tracks{20, 24, 36};
    state.demo_return_held =
        !state.demo_return_interrupted
        && std::find(held_return_tracks.begin(), held_return_tracks.end(), state.tour_menu.track)
               != held_return_tracks.end();
    state.mode_chosen = false;
    state.registers.force_blank = true;
    state.line_registers.clear();
    state.cycle.running = false;
    state.cycle.delay = state.cycle.phase = 0;
}

namespace {

// The idle demos take the race tracks below HUNTER's in turn (R-0087): `$80:949C-94B8` advances
// `$77:10C8`, from 0 again at 40, and again while the track is a stunt event (race mode 2, every
// tour's third track). `$83:C912-C994` flips `$77:1115` between a split race and a one-view one; the
// rider is the track plus the race counter `$77:10B1` plus the menu's palette-cycle phase `$00C9`
// (saved by `$83:9894` as `$77:0F34`), modulo 16. A split race's opponent is the track less
// both, or 13 more when that is the rider; a one-view race keeps the mode's opponent 1
// (`$80:9491`).
void choose_idle_demo(FrontEndState& state) {
    constexpr unsigned demo_tracks = 40, stunt_place = 2, riders = 16;
    auto& records = state.records;
    // `$80:949C-94A7`: one more, and 0 from 40 up (`CMP #$28`, `BCC`), not a modulo.
    do {
        const unsigned next = records.demo_track + 1U;
        records.demo_track = static_cast<std::uint8_t>(next < demo_tracks ? next : 0);
    } while (records.demo_track % front_end_screens::tracks_per_tour == stunt_place);
    // `$83:C8EF-C8FB` and `$83:C91C-C92A` store a counter outside its range back as 0.
    if (records.race_song_counter >= race_song_count) records.race_song_counter = 0;
    const unsigned counter = records.race_song_counter;
    if (records.demo_split > 1) records.demo_split = 0;
    records.demo_split = static_cast<std::uint8_t>(1U - records.demo_split);
    const unsigned track = records.demo_track;
    const unsigned phase = static_cast<std::uint8_t>(state.cycle.phase);
    state.tour_menu.track = records.demo_track;
    state.demo_split_race = records.demo_split != 0;
    state.rider_menu.rider = static_cast<std::uint8_t>((track + counter + phase) % riders);
    state.now_playing.opponent = 1;
    if (state.demo_split_race) {
        auto opponent = (track - counter - phase) % riders;
        if (opponent == state.rider_menu.rider) opponent = (opponent + 13U) % riders;
        state.now_playing.opponent = static_cast<std::uint8_t>(opponent);
    }
}

// The frames from the demo's choice to its race's initialization (R-0087): the menus' sound-load
// offset for the track (R-0077's measurement, the loading before `$83:CA08`), then 7. The demo
// skips the sound session itself (`$83:C9F6-CA05`). Equal on all 34 cycles of a cold lap.
std::uint32_t idle_demo_loading_frames(ClassicRaceTrack track) {
    constexpr std::uint32_t after_sound_load_offset = 7;
    return race_sound_load_offset(track) + after_sound_load_offset;
}

// The original's idle title has a 31-line brightness wave. Its 16-level
// profile repeats every picture, moving three lines down the display. The
// region above the wave is bright; below it the display is blank (R-0069).
void demo_title_frame(FrontEndState& state, const FrontEndContent& content) {
    const auto frame = state.script_frame;
    if (frame == 1) {
        load_title(state, content);
        clear_oam_buffer(state);
        copy_oam(state);
    }
    state.line_registers.clear();
    state.registers.bg[0].vofs = 0;
    if (frame < 134) {
        state.registers.force_blank = true;
        return;
    }
    state.registers.force_blank = false;
    state.registers.brightness = 15;
    if (frame < 208) {
        constexpr std::array<std::uint8_t, 31> wave{1,  3,  5,  7,  8,  9,  10, 11, 12, 12, 13,
                                                    13, 14, 14, 15, 15, 15, 15, 14, 14, 13, 13,
                                                    12, 12, 11, 10, 9,  8,  7,  5,  3};
        constexpr std::array<int, 31> source_offsets{40, 35, 30, 27, 26, 25, 24, 23, 22, 21, 20,
                                                     19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9,
                                                     8,  7,  6,  5,  4,  3,  2,  -1, -6};
        const auto wave_start = 2U + 3U * (frame - 134U);
        for (unsigned row = wave_start; row < 224U && row < wave_start + wave.size(); ++row) {
            state.line_registers.push_back({static_cast<std::uint8_t>(row),
                                            SnesLineRegisterName::display, wave[row - wave_start]});
            // BG1's per-line vertical offset reverses the title image inside
            // the band, with five eased rows at its two edges.
            state.line_registers.push_back(
                {static_cast<std::uint8_t>(row), SnesLineRegisterName::bg1_vertical_offset,
                 static_cast<std::uint16_t>(source_offsets[row - wave_start]
                                            - int(row - wave_start))});
        }
        if (wave_start + wave.size() < 224U)
            state.line_registers.push_back({static_cast<std::uint8_t>(wave_start + wave.size()),
                                            SnesLineRegisterName::display, 0x80});
    }
    constexpr unsigned fade_start = 452;
    if (frame >= fade_start && frame < fade_start + 6U)
        state.registers.brightness = static_cast<std::uint8_t>(13U - 2U * (frame - fade_start));
    if (frame >= fade_start + 6U) {
        state.registers.force_blank = true;
        state.cycle.running = false;
    }
    // The race setup chooses the demo on the frame it writes the track (`$77:074A`), and the race
    // starts when the track has loaded (R-0087).
    constexpr unsigned choice_frame = 451;
    if (frame == choice_frame) choose_idle_demo(state);
    if (frame == choice_frame + idle_demo_loading_frames(ClassicRaceTrack{state.tour_menu.track})) {
        // The demo's race keeps the song counter: `$83:C9F6-CA05` skips `$83:CA08` (R-0077).
        state.mode_chosen = true;
        state.screen = FrontEndScreen::race;
    }
}

void demo_return_frame(FrontEndState& state, const FrontEndContent& content) {
    const auto frame = state.script_frame;
    if (frame == 90) {
        load_main_menu_screen(state, content);
        lay_out_menu_objects(state);
        reload_menu_palette(state, content);
        copy_menu_text(state, content);
        reload_menu_text_tiles(state, content);
        state.logo.raised = false;
        state.logo.offset = 0;
        state.registers.bg[0].vofs = 0;
        state.registers.brightness = 14;
        state.registers.force_blank = true;
        state.cycle.running = false;
        state.cycle.delay = state.cycle.phase = 0;
    }
    // The interrupted return enables the palette hook one picture earlier (R-0070, $00C8/$00C9
    // trace at frames 5103-5117); the timer's return has an NMI pending (R-0088).
    if (frame == (state.demo_return_interrupted ? 100U : 101U)) {
        state.cycle.running = true;
        state.cycle.pending_nmi =
            !state.demo_return_interrupted && state.demo_cycles > 1 && !state.demo_title_was_short;
    }
    if (frame == 102) state.arrow.spin = 4;
    if (within(frame, 109, fade_frames)) fade(state, frame, 109, true);
    if (frame == 118) {
        state.demo_title_was_short = state.demo_title_held = false;
        start_main_menu(state);
    }
}

} // namespace

namespace front_end_screens {

// The power-on path again (`$80:91D1`): every register and all work RAM reset (VRAM is cleared on
// the boot's frame 18, `$80:B612`, while the screen is blank), the boot from its first frame; the
// cartridge RAM's records and one-player flag are kept, and CGRAM and OAM until the boot reloads
// them. The flag makes `$83:91F7` load rider 0's colours for the arrow, red, until `$83:8B23`
// clears it.
void soft_reset(FrontEndState& state) {
    auto reset = start_front_end();
    reset.frame = reset.boot_start = state.frame;
    reset.after_soft_reset = true;
    reset.records = state.records;
    reset.one_player = state.one_player;
    reset.video.cgram = state.video.cgram;
    reset.video.oam = state.video.oam;
    state = std::move(reset);
}

} // namespace front_end_screens

namespace {

void dispatch_front_end_screen(FrontEndState& state, const FrontEndContent& content,
                               FrontEndScreen screen, FrontEndPads physical) {
    switch (screen) {
    case FrontEndScreen::boot: boot_frame(state, content, physical); break;
    case FrontEndScreen::main_menu: run_main_menu(state, content, physical); break;
    case FrontEndScreen::rider_menu_entry: rider_menu_entry_frame(state, content); break;
    case FrontEndScreen::rider_menu: rider_menu_frame(state, content, physical); break;
    case FrontEndScreen::rider_menu_exit: rider_menu_exit_frame(state, content); break;
    case FrontEndScreen::main_menu_return: main_menu_return_frame(state, content); break;
    case FrontEndScreen::tour_menu_entry: tour_menu_entry_frame(state, content); break;
    case FrontEndScreen::tour_menu: tour_menu_frame(state, content, physical); break;
    case FrontEndScreen::track_menu_entry: track_menu_entry_frame(state, content); break;
    case FrontEndScreen::track_menu: track_menu_frame(state, content, physical); break;
    case FrontEndScreen::track_menu_exit: track_menu_exit_frame(state, content); break;
    case FrontEndScreen::now_playing_entry: now_playing_entry_frame(state, content); break;
    case FrontEndScreen::now_playing: now_playing_frame(state, content, physical); break;
    case FrontEndScreen::race_fade: race_fade_frame(state); break;
    case FrontEndScreen::race: break; // the race engine's frames
    case FrontEndScreen::race_return: race_return_frame(state, content); break;
    case FrontEndScreen::race_result: race_result_frame(state, content, physical); break;
    case FrontEndScreen::race_result_exit: race_result_exit_frame(state, content, physical); break;
    case FrontEndScreen::tour_award: tour_award_frame(state, content, physical); break;
    case FrontEndScreen::race_restart: race_restart_frame(state); break;
    case FrontEndScreen::award_return: award_return_frame(state, content); break;
    case FrontEndScreen::tour_ending: tour_ending_frame(state, content); break;
    case FrontEndScreen::hunter_ending: hunter_ending_frame(state, content, physical); break;
    case FrontEndScreen::hunter_code: hunter_code_frame(state); break;
    case FrontEndScreen::stunt_result: stunt_result_frame(state, content, physical); break;
    case FrontEndScreen::demo_title: demo_title_frame(state, content); break;
    case FrontEndScreen::demo_return: demo_return_frame(state, content); break;
    case FrontEndScreen::local_continue_entry: local_continue_entry_frame(state, content); break;
    case FrontEndScreen::local_continue: local_continue_frame(state, content, physical); break;
    case FrontEndScreen::vs_champions_entry: vs_champions_entry_frame(state, content); break;
    case FrontEndScreen::vs_champions: vs_champions_frame(state, content, physical); break;
    case FrontEndScreen::options_entry: options_entry_frame(state, content); break;
    case FrontEndScreen::options_return: options_return_frame(state, content); break;
    case FrontEndScreen::options_menu: options_menu_frame(state, content, physical); break;
    case FrontEndScreen::records_entry: records_entry_frame(state, content); break;
    case FrontEndScreen::records_menu: records_menu_frame(state, content, physical); break;
    case FrontEndScreen::rename_entry: rename_entry_frame(state, content); break;
    case FrontEndScreen::rename_keyboard: rename_keyboard_frame(state, content, physical); break;
    case FrontEndScreen::rename_commit: rename_commit_frame(state, content); break;
    case FrontEndScreen::rename_return: rename_return_frame(state, content); break;
    case FrontEndScreen::define_player_warning_entry:
        define_player_warning_entry_frame(state, content);
        break;
    case FrontEndScreen::define_player_warning: define_player_warning_frame(state, physical); break;
    case FrontEndScreen::define_player_after_confirm:
        define_player_after_confirm_frame(state, content);
        break;
    case FrontEndScreen::league_slots_entry: league_slots_entry_frame(state, content); break;
    case FrontEndScreen::league_slots: league_slots_frame(state, content, physical); break;
    case FrontEndScreen::league_table_entry: league_table_entry_frame(state, content); break;
    case FrontEndScreen::league_awards: league_awards_frame(state, content, physical); break;
    case FrontEndScreen::league_table: league_table_frame(state, content, physical); break;
    case FrontEndScreen::league_podium_entry: league_podium_entry_frame(state, content); break;
    case FrontEndScreen::league_podium: league_podium_frame(state, content, physical); break;
    case FrontEndScreen::league_podium_exit: league_podium_exit_frame(state, content); break;
    case FrontEndScreen::league_continue_entry: league_continue_entry_frame(state, content); break;
    case FrontEndScreen::league_continue: league_continue_frame(state, content, physical); break;
    case FrontEndScreen::league_warning_entry: league_warning_entry_frame(state, content); break;
    case FrontEndScreen::league_warning: league_warning_frame(state, physical); break;
    case FrontEndScreen::records_detail_entry: records_detail_entry_frame(state, content); break;
    case FrontEndScreen::records_detail: records_detail_frame(state, content, physical); break;
    case FrontEndScreen::records_detail_exit: records_detail_exit_frame(state, content); break;
    }
}

} // namespace

void update_front_end(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads) {
    state.sound_cues.clear();
    if (state.mode_chosen) return;
    if (state.screen == FrontEndScreen::demo_return && state.demo_return_wait != 0) {
        --state.demo_return_wait;
        ++state.frame;
        return;
    }
    if (state.screen == FrontEndScreen::demo_return && state.demo_return_held
        && state.script_frame == 100) {
        state.demo_return_held = false;
        ++state.frame;
        return;
    }
    // Every title after the first holds its blank frame 133 once while the sound program loads,
    // unless the sound processor ends that wait a frame sooner (a laboratory replay's short title,
    // R-0087); the wave, the fade and the choice follow a picture later (R-0088).
    if (state.screen == FrontEndScreen::demo_title && state.script_frame == demo_title_hold_frame
        && state.demo_cycles != 0 && !state.demo_title_held) {
        state.demo_title_held = true;
        state.demo_title_was_short = state.demo_title_short;
        state.demo_title_short = false;
        if (!state.demo_title_was_short) {
            ++state.frame;
            return;
        }
    }
    keep_line_writes(state);
    // NMIs are enabled at the end of the title's loads (`$80:F5B8`); the hook runs from then on:
    // the logo's slide, then the palette cycle.
    if (state.screen == FrontEndScreen::boot && boot_frame_number(state) == cycle_start_frame)
        state.cycle.running = true;
    if (state.cycle.running) run_nmi_hook(state, content);
    if (waits_for_frame(state)) update_arrow(state, content);
    const FrontEndPads physical{physical_pad(pads.one), physical_pad(pads.two)};
    const auto screen = state.screen;
    ++state.script_frame;
    dispatch_front_end_screen(state, content, screen, physical);
    if (state.screen != screen) state.script_frame = 0;
    if (ends_with_queue_wait(state))
        state.sound_cues.push_back(audio_dispatch(AudioDispatchSite::frame_wait));
    ++state.frame;
}

RgbFrame render_front_end(const FrontEndState& state) {
    return render_snes_screen(state.video, state.registers, state.line_colours,
                              state.line_registers);
}

} // namespace unirally
