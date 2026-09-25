#include "front_end.hpp"

#include "content_pack.hpp"
#include "text_printer.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <utility>

// The front end from power-on to the main menu's choice (R-0054). The boot is a fixed script:
// the frames on which the original loads each screen, waits for a frame, fades and so on are
// those of a cold start under the reference emulator (the loads and the sound program's upload
// take a fixed number of frames). The main menu then runs its loop once per frame.
namespace unirally {

namespace {

// Asset ids (the directory at $82:B332) and where the front end loads them.
constexpr unsigned early_objects = 88, early_palette = 5;                         // $80:A09A
constexpr unsigned nintendo_palette = 31, nintendo_map = 80, nintendo_tiles = 74; // $80:B08C
constexpr unsigned title_palette = 27, title_map = 78, title_tiles = 72;          // $80:F55F
constexpr unsigned menu_palette = 1, menu_object_palette = 2, menu_text_palette = 28;
constexpr unsigned menu_objects = 89, menu_objects_high = 91, menu_map = 77, menu_bg2_tiles = 70,
                   menu_bg1_tiles = 68, menu_bg1_tiles_high = 69; // $80:D20E
constexpr std::size_t menu_text_tiles_dma = 1920;                 // $80:A877: asset 69's start

// OAM buffer layout ($0A00): 128 four-byte entries, then the high table at $0C00.
constexpr std::size_t oam_high_table = 512;
constexpr unsigned arrow_entry = 119, shadow_entry = 127;

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
// The two codes: Left, A, L and R; B, Down, L and R.
constexpr std::uint16_t wipe_ram_code = 0x02b0, unread_code = 0x8430;

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

std::span<const std::uint8_t> asset(const FrontEndContent& content, unsigned id) {
    const auto data = content.assets[id];
    if (data.empty()) throw std::invalid_argument("front-end asset is not in the pack");
    return data;
}

// The VRAM copier `$82:B1DB` from word address `word`; CGRAM `$82:B183` from colour `colour`.
void load_vram(FrontEndState& state, std::span<const std::uint8_t> data, unsigned word) {
    for (std::size_t at = 0; at < data.size(); ++at)
        state.video.vram[(word * 2 + at) & 0xffffU] = data[at];
}
void load_cgram(FrontEndState& state, std::span<const std::uint8_t> data, unsigned colour) {
    for (std::size_t at = 0; at < data.size(); ++at)
        state.video.cgram[(colour * 2 + at) & 0x1ffU] = data[at];
}

void copy_oam(FrontEndState& state) { // $80:9318
    state.video.oam = state.oam_buffer;
}

void set_background(SnesBackground& bg, std::uint8_t sc) {
    bg.map_word = static_cast<std::uint16_t>((sc & 0xfcU) << 8U);
    bg.map_size = sc & 3U;
}
void set_tile_bases(SnesVideoRegisters& registers, std::uint8_t nba) {
    registers.bg[0].tile_word = static_cast<std::uint16_t>((nba & 0x0fU) << 12U);
    registers.bg[1].tile_word = static_cast<std::uint16_t>((nba >> 4U) << 12U);
}

// $80:FAF5: spin, then move a quarter of the way to the target. The quarter is an arithmetic
// shift, so a positive gap under 4 moves nothing: x settles three sixteenths short.
std::uint16_t quarter_step(std::uint16_t gap) {
    std::uint16_t step = static_cast<std::uint16_t>(gap >> 2U);
    if (step & 0x2000U) step |= 0xc000U;
    return step;
}

void set_oam_x_high(FrontEndState& state, unsigned entry, bool high) {
    auto& bits = state.oam_buffer[oam_high_table + entry / 4];
    const auto mask = static_cast<std::uint8_t>(1U << ((entry % 4) * 2));
    bits = high ? static_cast<std::uint8_t>(bits | mask) : static_cast<std::uint8_t>(bits & ~mask);
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

void park_arrow(FrontEndState& state) { // $83:99FA
    state.arrow.x = state.arrow.target_x = parked_x;
    state.arrow.y = state.arrow.target_y = parked_y;
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

// $80:A09A (frame 24): every OAM entry at (1, 1), every high bit 0x55; the early loads.
void clear_oam_buffer(FrontEndState& state) {
    for (std::size_t entry = 0; entry < 128; ++entry) {
        state.oam_buffer[entry * 4] = 1;
        state.oam_buffer[entry * 4 + 1] = 1;
    }
    std::fill(state.oam_buffer.begin() + oam_high_table, state.oam_buffer.end(),
              std::uint8_t{0x55});
}

// $80:D2C3-D36C (frame 402): the main menu's object layout. Entries 100-103 wait off the right
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
}

// The Nintendo screen ($80:A09A registers at 97, $80:B08C loads at 99): BG2 only, 4bpp.
void set_nintendo_registers(FrontEndState& state) {
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

void load_nintendo_screen(FrontEndState& state, const FrontEndContent& content) {
    load_cgram(state, content.base_palette, 0); // $80:A8A8
    park_arrow(state);                          // $83:99F6
    state.registers.force_blank = true;
    load_cgram(state, asset(content, nintendo_palette), 0);
    load_vram(state, asset(content, nintendo_map), 0x1000);
    load_vram(state, asset(content, nintendo_tiles), 0x2000);
}

// The title ($80:F55F at 228): BG1 only, 8bpp, its own 256 colours.
void load_title(FrontEndState& state, const FrontEndContent& content) {
    reset_screen_state(state); // $80:A16A
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

// The main menu's screen ($80:D20E at 377): BG1 (8bpp logo) and BG2 (4bpp checks and text)
// with the objects, which the subscreen adds at half (the arrow's shadow).
void load_main_menu(FrontEndState& state, const FrontEndContent& content) {
    load_cgram(state, content.base_palette, 0); // $80:A8A8
    state.registers.force_blank = true;
    reset_screen_state(state); // $80:A16A
    auto& r = state.registers;
    set_background(r.bg[0], 0x02);
    set_background(r.bg[1], 0x13);
    set_tile_bases(r, 0x23);
    r.main_screen = 0x13;
    r.mode = 3;
    r.obsel = 0x63;
    r.sub_screen = 0x10;
    r.colour_select = 0x02;
    r.colour_math = 0x7f;
    load_cgram(state, asset(content, menu_palette), 0x70);
    load_cgram(state, asset(content, menu_object_palette), 0xf0); // $83:91F7
    load_cgram(state, asset(content, menu_text_palette), 0xd0);
    load_vram(state, asset(content, menu_objects), 0x6000);
    load_vram(state, asset(content, menu_objects_high), 0x7d00);
    load_vram(state, asset(content, menu_map), 0);
    load_vram(state, asset(content, menu_bg2_tiles), 0x2000);
    load_vram(state, asset(content, menu_bg1_tiles), 0x31a0);
    load_vram(state, asset(content, menu_bg1_tiles_high), 0x3d80);
}

// $80:ACD5 (405-407): the menu text, printed into the cleared map ($0200) and copied to BG2.
TextMap main_menu_text(const FrontEndContent& content) {
    TextMap map;
    map.words.fill(0x004c);
    TextCursor cursor;
    print_text(map, cursor, content.main_menu_text, content.character_table);
    return map;
}

void copy_menu_text(FrontEndState& state, const FrontEndContent& content) {
    const auto map = main_menu_text(content);
    for (std::size_t k = 0; k < map.words.size(); ++k) {
        state.video.vram[0x2000 + k * 2] = static_cast<std::uint8_t>(map.words[k]);
        state.video.vram[0x2000 + k * 2 + 1] = static_cast<std::uint8_t>(map.words[k] >> 8U);
    }
}

void fade(FrontEndState& state, std::uint32_t first_frame, bool in) {
    const auto step = state.frame - first_frame;
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

bool waits_for_frame(std::uint32_t frame) {
    for (const auto& waits : boot_frame_waits) {
        if (waits.count == 0 && frame >= waits.first) return true;
        if (within(frame, waits.first, waits.count)) return true;
    }
    return false;
}

// The boot's scripted work for one frame, after the NMI and the frame wait.
void boot_frame(FrontEndState& state, const FrontEndContent& content) {
    const auto f = state.frame;
    if (f == 24) {
        clear_oam_buffer(state);
        load_cgram(state, asset(content, early_palette), 0xe0);
        load_vram(state, asset(content, early_objects), 0x7000);
    } else if (f == nintendo_registers_frame) {
        set_nintendo_registers(state);
    } else if (f == nintendo_load_frame) {
        load_nintendo_screen(state, content);
    } else if (f == nintendo_layers_frame) {
        state.registers.main_screen = 0x02;
        state.registers.sub_screen = 0;
    } else if (within(f, nintendo_fade_in, fade_frames)) {
        fade(state, nintendo_fade_in, true);
    } else if (within(f, nintendo_fade_out, fade_frames)) {
        fade(state, nintendo_fade_out, false);
        if (f == title_load_frame) load_title(state, content);
    } else if (within(f, title_fade_in, fade_frames)) {
        fade(state, title_fade_in, true);
    } else if (f > title_fade_in + fade_frames - 1 && f < title_fade_out) {
        copy_oam(state); // $80:D1EC
    } else if (within(f, title_fade_out, fade_frames)) {
        fade(state, title_fade_out, false);
    } else if (f == menu_load_frame) {
        load_main_menu(state, content);
    } else if (f == menu_objects_frame) {
        lay_out_menu_objects(state);
        park_arrow(state); // $83:99F6
    } else if (f == menu_map_filled_frame) {
        copy_oam(state); // $80:D372
    } else if (f == menu_map_copy_frame) {
        copy_menu_text(state, content);
    } else if (f == menu_palette_frame || f == menu_palette_again_frame) {
        load_cgram(state, content.base_palette, 0);
    } else if (f == menu_tiles_frame || f == main_menu_frame) {
        load_vram(state, asset(content, menu_bg1_tiles_high).first(menu_text_tiles_dma), 0x3d80);
        if (f == main_menu_frame) {
            state.in_main_menu = true;
            state.menu.selection = 0;
            state.menu.idle = first_idle;
            state.menu.move_latched = false;
            send_arrow_to_first_entry(state);
        }
    } else if (within(f, menu_fade_in, fade_frames)) {
        fade(state, menu_fade_in, true);
    }
}

// One pass of the main menu's loop ($80:ABE3), after its frame wait.
void run_main_menu(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads) {
    copy_oam(state); // $80:D1EC
    // The codes come first, as exact words on either pad, the WIPE RAM code before the other
    // (`$80:ABEB-AC0A`).
    for (const auto [code, mode] : {std::pair{wipe_ram_code, FrontEndMode::wipe_ram_code},
                                    std::pair{unread_code, FrontEndMode::unread_code}}) {
        if (pads.one == code || pads.two == code) {
            state.mode_chosen = true;
            state.mode = mode;
            return;
        }
    }
    auto& menu = state.menu;
    // `$80:AC0F-AC14`: the count is stored only while it stays positive; the demo is mode 5.
    if (menu.idle == 0) {
        menu.selection = static_cast<std::uint8_t>(FrontEndMode::demo);
        state.mode_chosen = true;
        state.mode = FrontEndMode::demo;
        return;
    }
    --menu.idle;
    const auto pressed = [&](std::uint16_t buttons) {
        return (pads.one & buttons) != 0 || (pads.two & buttons) != 0;
    };
    if (pressed(choose_buttons)) {
        state.mode_chosen = true;
        state.mode = static_cast<FrontEndMode>(menu.selection);
        return;
    }
    const bool down = pressed(down_buttons);
    const bool up = !down && pressed(up_buttons);
    if (!down && !up) {
        menu.move_latched = false;
        return;
    }
    if (menu.move_latched) return;
    menu.move_latched = true;
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

} // namespace

FrontEndContent front_end_content(const ClassicContentPack& pack) {
    FrontEndContent content;
    for (const unsigned id :
         {1U, 2U, 5U, 27U, 28U, 31U, 68U, 69U, 70U, 72U, 74U, 77U, 78U, 80U, 88U, 89U, 91U}) {
        std::string name = "front-end.asset.";
        name += static_cast<char>('0' + id / 100);
        name += static_cast<char>('0' + id / 10 % 10);
        name += static_cast<char>('0' + id % 10);
        content.assets[id] = pack.entry(name);
    }
    content.base_palette = pack.entry("front-end.base-palette");
    content.character_table = pack.entry("front-end.character-table");
    content.main_menu_text = pack.entry("front-end.main-menu-text");
    content.arrow_frames = pack.entry("front-end.arrow-frames");
    content.menu_arrow_columns = pack.entry("front-end.menu-arrow-columns");
    content.cycle_colours = pack.entry("front-end.cycle-colours");
    return content;
}

FrontEndState start_front_end() {
    return {};
}

void update_front_end(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads) {
    if (state.mode_chosen) return;
    // NMIs are enabled at the end of the title's loads (`$80:F5B8`); the hook runs from then on.
    if (state.frame == cycle_start_frame) state.cycle.running = true;
    if (state.cycle.running) step_palette_cycle(state, content);
    if (waits_for_frame(state.frame)) update_arrow(state, content);
    if (state.in_main_menu && state.frame > main_menu_frame)
        run_main_menu(state, content, {physical_pad(pads.one), physical_pad(pads.two)});
    else
        boot_frame(state, content);
    ++state.frame;
}

RgbFrame render_front_end(const FrontEndState& state) {
    return render_snes_screen(state.video, state.registers);
}

} // namespace unirally
