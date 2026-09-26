// HUNTER's gold ending (`$83:AB9A`, HUNTER-ENDING). After the award's fade out, a newspaper page
// rolls down (`$80:E2CF`, HDMA on BG1's vertical offset and INIDISP) and waits for a press on
// either pad (`$80:C202`); a second page rolls down and waits for a press on pad 1 or 1,201
// frames (`$83:AC1F`); then the credits, eleven faces on animated unis, until a press; then a
// fade and `JML $80:8858`, the power-on entry. With the title code's flag (`$77:10D0`) one CHEAT!
// page replaces both newspapers. Nothing is written to the records: the medal and the tracks done
// were stored on the scoring frame, and there is no unlock rule.
#include "front_end_screens.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace unirally::front_end_screens {

namespace {

// A newspaper page: its colours (CGRAM 0), map (VRAM word 0) and tiles (VRAM word 0x1000), and the
// part's frame of its reveal's first HDMA picture. The CPU copies the tiles over eight frames,
// SPEEDKING's over nine, so its reveal starts a frame later (measured, not derived).
struct NewspaperPage {
    unsigned colours{}, map{}, tiles{};
    std::uint32_t reveal_frame{};
};
constexpr NewspaperPage daily_news{0x67, 0x6e, 0x6b, 25}; // `$83:ABDA-ABF5`
constexpr NewspaperPage speedking{0x68, 0x6f, 0x6c, 26};  // `$83:ABFD-AC1B`
constexpr NewspaperPage cheat_page{0x66, 0x6d, 0x69, 25}; // `$83:ABB9-ABD4`
constexpr unsigned page_tiles_word = 0x1000;
// The pages' screen (`$83:AB9F-ABB0`): BG1 only, mode 1, its map at word 0 (32 x 32), its tiles
// at 0x1000.
constexpr std::uint8_t page_screens = 0x01, page_mode = 1, page_map = 0x00, page_tile_bases = 0x01;

// A page's part, frames from its start: `$83:A4E9`'s fade ends on frame 16, where the page's
// colours and map load; its tiles from 17. The reveal copies its tables the frame before its
// first HDMA picture, then takes 75 steps a frame, then a frame that turns HDMA off and shows
// the whole page (`$80:E369-E377`).
constexpr std::uint32_t page_load_frame = 16, page_tiles_frame = 17, reveal_steps = 75;
// The reveal's tables in `HunterEnding::reveal_tables` (`$0C70`, `$0CB8`): each starts with two
// single-write entries whose line counts grow by 3 a step, the first entries on odd steps.
constexpr std::size_t offsets_at = 0, brightness_at = 72, offset_entry = 3, brightness_entry = 2;
constexpr std::uint8_t reveal_growth = 3;
constexpr unsigned screen_rows = 224;
// $83:AC1F: 1,201 passes at most (Y from 0x4B0 down past 0).
constexpr std::uint32_t timed_wait_passes = 0x4b1;
// $80:B6D3: any of a pad's twelve buttons.
constexpr std::uint16_t twelve_buttons = 0xfff0;

// The credits (`$83:AC32-ADF8`), frames from the part's start: the fade ends on 16 with the
// set-up; the menus' tile and map loads end on their frames; on 45 the colours, the text, the
// objects and the first pose; on 46 the pose's lower half; the fade in (`$83:A4D2`) on 47-61;
// then the loop, a pass a frame.
constexpr std::uint32_t credits_setup_frame = 16, credits_shown_frame = 45, lower_pose_frame = 46,
                        credits_fade_in = 47, credits_loop_frame = 62;
struct TimedLoad {
    std::uint32_t frame;
    unsigned asset, word;
};
constexpr std::array<TimedLoad, 6> credits_loads{{
    {21, 0x5b, 0x7d00}, // the menus' objects' high tiles
    {22, 0x4d, 0x0000}, // the logo's map
    {24, 0x46, 0x2000}, // the checks' tiles
    {29, 0x44, 0x31a0}, // the logo's tiles
    {34, 0x45, 0x3d80},
    {41, 0x5d, 0x6000}, // the faces
}};
constexpr unsigned credits_colours = 0x00, menu_colours = 0x01, menu_objects = 0x59;
// Colours 0x80-0xF0 (`$83:ACDD-AD21`): riders 0, 4, 3, 2, 1, 5 and 8, then the faces'.
constexpr std::array<unsigned, 8> credits_object_colours{0x06, 0x0a, 0x09, 0x08,
                                                         0x07, 0x0b, 0x0e, 0x3a};
constexpr std::uint8_t credits_logo_offset = 0x4f; // `$83:AC6D`: `$00AD` and BG1VOFS
// The objects (`$83:AD33-AD63`): entries 0-32 from `credits_objects`, 0-31 large, 32 large and
// 33-35 hidden.
constexpr std::size_t credits_object_entries = 33;
constexpr std::uint8_t four_large = 0xaa, one_large_three_hidden = 0x56;
// The unis' upper halves at object tile 0x100, their lower halves at 0x140 (`$83:AD67-AD8F`): a
// pose and the pose 0x19 further on. Every pose of the loop is held two frames, 96 steps in all.
constexpr unsigned upper_pose_word = 0x7000, lower_pose_word = 0x7500;
constexpr std::uint16_t lower_half_poses = 0x19, pose_steps = 0x60;

// The leaving fade (`$83:ADFA`) ends on frame 16 with the reset.
constexpr std::uint32_t reset_frame = 16;

const NewspaperPage& page_of(const HunterEnding& ending) {
    if (ending.part == HunterEndingPart::second_page) return speedking;
    return ending.cheat_page ? cheat_page : daily_news;
}

// The part's frame for the script frame `script_frame`.
std::uint32_t part_frame(const FrontEndState& state, std::uint32_t script_frame) {
    return script_frame - state.hunter.part_start;
}

// The next part starts counting from this frame.
void begin_part(FrontEndState& state, HunterEndingPart part) {
    state.hunter.part = part;
    state.hunter.part_start = state.script_frame;
}

std::uint16_t word_at(std::span<const std::uint8_t> table, std::size_t at) {
    return static_cast<std::uint16_t>(table[at] | (table[at + 1] << 8U));
}

// HDMA in direct mode through a table: each entry a line count (bit 7 set: a write on every
// line), then the data; a count of 0, or the end of the tables native keeps, ends it (the byte
// after them, `$0CDE`, is 0 in the capture). `bytes` is 1, or 2 for the two writes of mode 2
// (BG1VOFS's low byte, then its high byte). A write on the table's line n is first seen on row n.
void add_hdma_writes(std::vector<SnesLineRegister>& writes, std::span<const std::uint8_t> table,
                     unsigned bytes, SnesLineRegisterName name) {
    std::size_t at = 0;
    unsigned line = 0;
    while (line < screen_rows && at < table.size() && table[at] != 0) {
        const bool repeat = (table[at] & 0x80U) != 0;
        const unsigned count = (table[at] & 0x7fU) == 0 ? 0x80U : table[at] & 0x7fU;
        ++at;
        for (unsigned k = 0; k < count && line < screen_rows; ++k, ++line) {
            if (k > 0 && !repeat) continue;
            if (at + bytes > table.size()) return;
            const std::uint16_t value = bytes == 2 ? word_at(table, at) : table[at];
            writes.push_back({static_cast<std::uint8_t>(line), name, value});
            if (repeat) at += bytes;
        }
        if (!repeat) at += bytes;
    }
}

// The picture's HDMA writes: channel 5 to BG1VOFS from `$0C70`, which runs on into channel 6's
// table (its last entry's lines are blanked), and channel 6 to INIDISP from `$0CB8`.
void write_reveal_lines(FrontEndState& state) {
    const std::span<const std::uint8_t> tables = state.hunter.reveal_tables;
    auto& writes = state.line_registers;
    writes.clear();
    add_hdma_writes(writes, tables.subspan(offsets_at), 2,
                    SnesLineRegisterName::bg1_vertical_offset);
    add_hdma_writes(writes, tables.subspan(brightness_at), 1, SnesLineRegisterName::display);
    std::stable_sort(writes.begin(), writes.end(),
                     [](const auto& a, const auto& b) { return a.row < b.row; });
}

// $80:E2D8-E2F6: the tables copied to work RAM.
void copy_reveal_tables(FrontEndState& state, const FrontEndContent& content) {
    auto& tables = state.hunter.reveal_tables;
    std::copy(content.reveal_offsets.begin(), content.reveal_offsets.end(),
              tables.begin() + offsets_at);
    std::copy(content.reveal_brightness.begin(), content.reveal_brightness.end(),
              tables.begin() + brightness_at);
}

// $80:E322-E367: HDMA on at step 0; each later step waits, copies OAM (`$80:9318`) and lengthens
// both tables' first entry (Y even, the odd steps) or second entry by 3 lines.
void reveal_step(FrontEndState& state, std::uint32_t step) {
    auto& ending = state.hunter;
    if (step == 0) {
        ending.reveal_on = true;
    } else {
        copy_oam(state);
        const std::size_t entry = step % 2 == 1 ? 0 : 1;
        ending.reveal_tables[offsets_at + entry * offset_entry] += reveal_growth;
        ending.reveal_tables[brightness_at + entry * brightness_entry] += reveal_growth;
    }
    write_reveal_lines(state);
}

// $80:E369-E377: HDMA off, the page at full brightness, BG1's offset back to 0.
void end_reveal(FrontEndState& state) {
    state.hunter.reveal_on = false;
    state.line_registers.clear();
    state.registers.force_blank = false;
    state.registers.brightness = 15;
    state.registers.bg[0].vofs = 0;
}

// $83:AB9F-ABB0: the pages' registers, set once before the first page.
void set_page_registers(FrontEndState& state) {
    auto& r = state.registers;
    r.main_screen = page_screens;
    r.mode = page_mode;
    set_background(r.bg[0], page_map);
    set_tile_bases(r, page_tile_bases);
}

// A page's fade out, loads and reveal; true on the frames after the reveal, the page's wait.
bool page_frame(FrontEndState& state, const FrontEndContent& content, std::uint32_t frame) {
    const auto& page = page_of(state.hunter);
    if (frame <= page_load_frame) {
        fade_down(state, frame);
        if (frame < page_load_frame) return false;
        if (state.hunter.part == HunterEndingPart::first_page) set_page_registers(state);
        load_cgram(state, asset(content, page.colours), 0);
        load_vram(state, asset(content, page.map), 0);
        return false;
    }
    if (frame == page_tiles_frame) load_vram(state, asset(content, page.tiles), page_tiles_word);
    if (frame + 1 == page.reveal_frame) copy_reveal_tables(state, content);
    if (frame < page.reveal_frame) return false;
    const auto step = frame - page.reveal_frame;
    if (step <= reveal_steps) {
        reveal_step(state, step);
        return false;
    }
    if (step == reveal_steps + 1) {
        end_reveal(state);
        return false;
    }
    return true;
}

// The frame after the page's reveal that is the wait's first pass.
std::uint32_t first_wait_frame(const HunterEnding& ending) {
    return page_of(ending).reveal_frame + reveal_steps + 2;
}

// $80:C206: the pads read with an OAM copy (`$80:D1EC`) and the decorations stepped; any bit of
// either pad on two passes running ends the wait, and `$80:C236` hides entries 30-35.
bool first_page_press(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads) {
    copy_oam(state);
    step_decorations(state, content);
    const bool press = (pads.one | pads.two) != 0;
    auto& ending = state.hunter;
    if (!ending.press_seen || !press) {
        ending.press_seen = press;
        return false;
    }
    high_bits(state, 32) = four_hidden;
    high_bits(state, 28) = static_cast<std::uint8_t>((high_bits(state, 28) & 0x0fU) | 0x50U);
    return true;
}

// $80:B6D3: pad 1's twelve buttons; pad 2's too from the main menu (`$77:0742` bit 10 clear).
bool ending_press(const FrontEndState& state, FrontEndPads pads) {
    return (pads.one & twelve_buttons) != 0
        || (state.hunter.both_pads && (pads.two & twelve_buttons) != 0);
}

// $83:AC1F-AC30, pass `pass` (from 1): a wait (`$83:A923`), the pads read with an OAM copy
// (`$80:D1E8`) and the test; true when a press or the last pass ends it.
bool timed_wait_pass(FrontEndState& state, FrontEndPads pads, std::uint32_t pass) {
    copy_oam(state);
    return ending_press(state, pads) || pass == timed_wait_passes;
}

// Both pages' part: the page, then its wait; the next part begins on the frame the wait ends.
void pages_frame(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads,
                 std::uint32_t frame) {
    if (!page_frame(state, content, frame)) return;
    auto& ending = state.hunter;
    const bool timed = ending.cheat_page || ending.part == HunterEndingPart::second_page;
    if (!timed) {
        if (first_page_press(state, content, pads))
            begin_part(state, HunterEndingPart::second_page);
        return;
    }
    if (timed_wait_pass(state, pads, frame - first_wait_frame(ending) + 1))
        begin_part(state, HunterEndingPart::credits);
}

// $83:AC32-AC92 after the fade: the text map cleared (`$83:8B51`), the menus' registers, the logo
// low (NMI's hook then slides it to 0x51), the colours 0-127, the menus' objects.
void set_up_credits(FrontEndState& state, const FrontEndContent& content) {
    state.text.words.fill(cleared_text);
    set_menu_registers(state.registers);
    state.logo.offset = credits_logo_offset;
    state.registers.bg[0].vofs = credits_logo_offset;
    load_cgram(state, asset(content, credits_colours), 0);
    load_cgram(state, asset(content, menu_colours), 0x70);
    load_vram(state, asset(content, menu_objects), 0x6000);
}

// $83:ACDD-AD8F: the riders' and faces' colours; "--WHODUNNIT--" printed by the bank-83 printer
// (`$83:A93A`, the menus' glyphs and forms) and sent to BG2's shown half; the objects; the first
// pose's upper half.
void show_credits(FrontEndState& state, const FrontEndContent& content) {
    for (std::size_t k = 0; k < credits_object_colours.size(); ++k)
        load_cgram(state, asset(content, credits_object_colours[k]),
                   static_cast<unsigned>(0x80 + 0x10 * k));
    print_text(state.text, state.printer, content.credits_text, content.character_table);
    load_text(state, state.slide.shown_half);
    std::copy_n(content.credits_objects.begin(), credits_object_entries * 4,
                state.oam_buffer.begin());
    for (unsigned entry = 0; entry < 32; entry += 4) high_bits(state, entry) = four_large;
    high_bits(state, 32) = one_large_three_hidden;
    copy_oam(state); // $80:9314
    upload_pose(state, content, word_at(content.credits_poses, 0), upper_pose_word);
}

// $83:AD9B-ADF8, one pass: the pose of step `$0036` (each held two steps) and its lower half, the
// wait, their uploads, the pads read with an OAM copy (`$80:D1E8`), the test.
bool credits_pass(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads) {
    auto& ending = state.hunter;
    const auto pose = word_at(content.credits_poses, (ending.pose_step >> 1U) * 2U);
    upload_pose(state, content, pose, upper_pose_word);
    upload_pose(state, content, static_cast<std::uint16_t>(pose + lower_half_poses),
                lower_pose_word);
    copy_oam(state);
    if (ending_press(state, pads)) return true;
    ending.pose_step = static_cast<std::uint16_t>((ending.pose_step + 1) % pose_steps);
    return false;
}

void credits_frame(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads,
                   std::uint32_t frame) {
    if (frame <= credits_setup_frame) {
        fade_down(state, frame);
        if (frame == credits_setup_frame) set_up_credits(state, content);
        return;
    }
    for (const auto& load : credits_loads)
        if (frame == load.frame) load_vram(state, asset(content, load.asset), load.word);
    if (frame == credits_shown_frame) {
        show_credits(state, content);
    } else if (frame == lower_pose_frame) {
        const auto pose =
            static_cast<std::uint16_t>(word_at(content.credits_poses, 0) + lower_half_poses);
        upload_pose(state, content, pose, lower_pose_word);
    } else if (frame >= credits_fade_in && frame < credits_loop_frame) {
        fade_up(state, frame - credits_fade_in + 1);
    } else if (frame >= credits_loop_frame && credits_pass(state, content, pads)) {
        begin_part(state, HunterEndingPart::leaving);
    }
}

} // namespace

// $80:F0D6: the logo raised (`$80:F51B`, `$77:0742` bit 1) and the arrow sent off (`$80:98A4`);
// then 31 frames of a wait and an OAM copy (`$80:9318`), and `JML $83:AB9A` on the last, which
// counts as the ending's frame 0. From the main menu `$77:0742` bits 9 and 10 are clear, so the
// ending's pad test reads pad 2 too.
void enter_hunter_code(FrontEndState& state) {
    state.decorations.delay = static_cast<std::uint8_t>(state.menu.idle); // the shared $0089
    state.logo.raised = true;
    send_arrow_off(state);
    state.screen = FrontEndScreen::hunter_code;
}

void hunter_code_frame(FrontEndState& state) {
    constexpr std::uint32_t code_frames = 31;
    copy_oam(state);
    if (state.script_frame == code_frames) start_hunter_ending(state, true);
}

void start_hunter_ending(FrontEndState& state, bool both_pads) {
    state.hunter = {};
    state.hunter.cheat_page = state.records.cheat; // $83:ABB3
    state.hunter.both_pads = both_pads;
    state.screen = FrontEndScreen::hunter_ending;
    state.script_frame = 0;
}

void hunter_ending_frame(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads) {
    const auto frame = part_frame(state, state.script_frame);
    switch (state.hunter.part) {
    case HunterEndingPart::first_page:
    case HunterEndingPart::second_page: pages_frame(state, content, pads, frame); return;
    case HunterEndingPart::credits: credits_frame(state, content, pads, frame); return;
    case HunterEndingPart::leaving:
        fade_down(state, frame);
        if (frame == reset_frame) soft_reset(state);
        return;
    }
}

// $80:FADF's frames: the reveal's (`$80:E322`, `E332`, `E369`) and the first page's wait
// (`$80:C20C`, `C222`); the fades' and the other waits are `$83:A923`'s.
bool hunter_ending_waits(const FrontEndState& state) {
    const auto& ending = state.hunter;
    if (ending.part != HunterEndingPart::first_page && ending.part != HunterEndingPart::second_page)
        return false;
    const auto frame = part_frame(state, state.script_frame + 1);
    const auto& page = page_of(ending);
    if (frame < page.reveal_frame) return false;
    if (frame < first_wait_frame(ending)) return true;
    return ending.part == HunterEndingPart::first_page && !ending.cheat_page;
}

} // namespace unirally::front_end_screens
