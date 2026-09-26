// A tour's completion (R-0059): `$83:879A`'s completion branch (`$83:881B-88D2`), the medal
// award screen `$83:AEF6` and its animation, the menus' restore `$83:A721`, the unlock rule
// `$83:8853`, then PICK TOUR from inside the scoring. Frames count from the completion's frame,
// q + 3 (R-0057's q, the result's exit).
#include "front_end_screens.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>

namespace unirally::front_end_screens {

namespace {

// The award's script, frames from the completion: `$83:A4E9`'s fade out (brightness 14 to 0,
// then forced blank), `$83:A614`'s sound upload, the loads (`$83:AEF6-AFE3`), `$83:A4D2`'s fade in
// (1 to 15), then the animation from `first_step_frame`.
constexpr std::uint32_t fade_out_frames = 15, blank_frame = 16, reset_frame = 97,
                        background_tiles_frame = 99, podium_map_frame = 100,
                        podium_tiles_frame = 101, colours_frame = 102, objects_frame = 106,
                        rider_frame = 107, first_fade_in_frame = 108, fade_in_frames = 15;
// After the press, frames from the exit test: the fade out, `$83:A721` (its sound upload, then
// the menus' screen at `menus_frame`), the fade in, and PICK TOUR on its last frame.
constexpr std::uint32_t exit_blank_frame = 16, menus_frame = 117, exit_fade_in_frame = 118,
                        pick_tour_frame = 132;

// The award's assets (profile v20).
constexpr unsigned background_map = 0x53, background_tiles = 0x54, podium_map = 0x65,
                   podium_tiles = 0x64, podium_colours = 0x3b, medal_tiles = 0x5c;
// The medal's colours are PICK TOUR's medal palettes, 0x20 bronze and 0x21 silver (`tour_menu.cpp`);
// `$83:AF14` takes 0x1F + the new medal.
constexpr unsigned first_medal_colours = 0x1f;
constexpr unsigned first_background_colours = 0x54; // + the new medal: 0x55, 0x56
// `$83:A67F-A720` loads 0x22 (the gold medal's colours) at 0x90, which the rider's overwrite in
// the same frame; its CGRAM 0 load (the rider's) is omitted, as q + 105 overwrites it.
constexpr unsigned gold_medal_colours = 0x22;
constexpr unsigned background_tiles_word = 0x3000, podium_map_word = 0x1000,
                   podium_tiles_word = 0x2000, medal_tiles_word = 0x6000, second_art_word = 0x6800,
                   cleared_objects_word = 0x7000, cleared_objects_words = 0x800;
constexpr std::uint16_t podium_map_bits = 0x2400; // palette 1, priority
constexpr std::uint8_t award_objects = 0xa3;      // 32 x 32 and 64 x 64, name base 3
constexpr std::uint8_t award_screens = 0x13;      // BG1, BG2, objects
constexpr std::uint16_t rider_pose = 0x1340;
// `$83:B120` (front-end.award-tables): the medal's and the rider's first entries (8 bytes), the
// bounce (25), the rider's poses by step (58 words), the medal's tiles by step (53).
constexpr std::size_t first_entries_at = 0, bounce_at = 9, poses_at = 0x22, medal_tiles_at = 0x96;
// (Byte 8 is unused.) `$83:B0FC` writes the second art's entry 0 attribute as a constant.
constexpr std::uint8_t second_art_attributes = 0x10;
constexpr std::uint8_t first_objects_high = 0x58; // entry 0 small, entry 1 large, 2-3 hidden
// The animation: steps 1 to 0x39 once, then 0x2C to 0x39 again until a press; the medal falls
// 4 lines a step until step 0x1A of `$77:10CB`, and at 0x26 its second art arrives.
constexpr std::uint16_t last_step = 0x3a, repeat_step = 0x2b, medal_start = 0xfffb;
constexpr std::int16_t repeat_medal = 0x26, landed = 0x1a, second_art = 0x26;
constexpr std::uint8_t medal_fall = 4, bounce_base = 0x71, second_art_tile = 0x84;

std::uint16_t word_of(std::span<const std::uint8_t> table, std::size_t at) {
    return static_cast<std::uint16_t>(table[at] | (table[at + 1] << 8U));
}

void fade_down(FrontEndState& state, std::uint32_t frame) { // $83:A4E9, frames 1-16
    if (frame < exit_blank_frame) {
        state.registers.brightness = static_cast<std::uint8_t>(fade_out_frames - frame);
        return;
    }
    state.registers.brightness = 0;
    state.registers.force_blank = true;
}

void fade_up(FrontEndState& state, std::uint32_t step) { // $83:A4D2, steps 1-15
    state.registers.brightness = static_cast<std::uint8_t>(step);
    state.registers.force_blank = false;
}

// $83:A67F-A720 and `$83:AEF6-AF3E` (q + 100): NMI off, the screen's state reset, the award's
// first loads.
void reset_for_award(FrontEndState& state, const FrontEndContent& content) {
    state.cycle.running = false;
    const std::vector<std::uint8_t> zeros(cleared_objects_words * 2, 0);
    load_vram(state, zeros, cleared_objects_word);
    state.text.words.fill(cleared_text);
    state.slide.scroll = 0;
    state.slide.shown_half = 0x1000;
    state.slide.hidden_half = 0x1400;
    auto& r = state.registers;
    r.bg[0].hofs = r.bg[1].hofs = r.bg[0].vofs = r.bg[1].vofs = 0;
    r.main_screen = award_screens;
    r.colour_select = r.colour_math = 0;
    clear_oam_buffer(state); // every entry at (1, 1), hidden, with tile and attributes 0
    for (unsigned entry = 0; entry < 128; ++entry)
        oam_byte(state, entry, 2) = oam_byte(state, entry, 3) = 0;
    load_cgram(state, asset(content, gold_medal_colours), 0x90);
    r.mode = 2;
    const auto medal = state.award.medal;
    load_cgram(state, asset(content, first_medal_colours + medal), 0x80);
    load_cgram(state, asset(content, first_rider_palette + state.rider_menu.rider), 0x90);
    load_vram(state, asset(content, background_map), 0);
}

// $82:B1AE: a map into VRAM from word `word`, `bits` added to each entry (its palette and
// priority).
void load_map(FrontEndState& state, std::span<const std::uint8_t> map, unsigned word,
              std::uint16_t bits) {
    std::vector<std::uint8_t> entries(map.begin(), map.end());
    for (std::size_t at = 0; at + 1 < entries.size(); at += 2)
        entries[at + 1] = static_cast<std::uint8_t>(entries[at + 1] + (bits >> 8U));
    load_vram(state, entries, word);
}

// $80:F814: the rider's pose into its object tiles at VRAM word 0x7000.
void upload_rider(FrontEndState& state, const FrontEndContent& content) {
    upload_pose(state, content, state.award.pose);
}

// $83:B004-B062: a step's build; false for the reset step, which only tests the pads.
bool build_step(FrontEndState& state, const FrontEndContent& content) {
    auto& award = state.award;
    const auto tables = content.award_tables;
    const auto step = static_cast<std::uint16_t>(award.step + 1);
    if (step == last_step) { // $83:AFE8
        award.medal_step = repeat_medal;
        award.step = repeat_step;
        return false;
    }
    award.step = step;
    award.pose = word_of(tables, poses_at + 2U * step);
    const auto medal_step = ++award.medal_step;
    if (medal_step >= 0) {
        if (medal_step < landed)
            oam_byte(state, 0, 1) = static_cast<std::uint8_t>(oam_byte(state, 0, 1) + medal_fall);
        oam_byte(state, 0, 2) = tables[medal_tiles_at + static_cast<std::size_t>(medal_step)];
    }
    return true;
}

// $83:B062-B093: the step's upload and pad read, and from `$77:10CB` 0x26 the bounce.
void upload_step(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads) {
    auto& award = state.award;
    upload_rider(state, content);
    copy_oam(state); // $80:D1E8, which also reads the pads
    award.pads = pads.one;
    if (award.medal_step > second_art) {
        const auto y = static_cast<std::uint8_t>(
            bounce_base
            - content.award_tables[bounce_at
                                   + static_cast<std::size_t>(award.medal_step - second_art)]);
        oam_byte(state, 0, 1) = oam_byte(state, 1, 1) = y;
    }
}

// $80:B6D3 on the pads the last upload read: any of pad 1's twelve buttons.
bool award_left(const FrontEndState& state) {
    constexpr std::uint16_t buttons = 0xfff0;
    return (state.award.pads & buttons) != 0;
}

// $83:B096-B111: the medal's second art in two halves, the pose again, the medal's tile set in
// OAM directly, then the bounce's first height.
void second_art_frame(FrontEndState& state, const FrontEndContent& content, unsigned part) {
    const auto art = content.award_medal_art;
    if (part == 0) {
        load_vram(state, art.first(art.size() / 2), medal_tiles_word);
        return;
    }
    if (part == 1) {
        load_vram(state, art.last(art.size() / 2), second_art_word);
        upload_rider(state, content);
        oam_byte(state, 0, 2) = second_art_tile;
        state.video.oam[2] = second_art_tile;
        state.video.oam[3] = second_art_attributes;
        return;
    }
    const auto y = static_cast<std::uint8_t>(bounce_base - content.award_tables[bounce_at]);
    oam_byte(state, 0, 1) = oam_byte(state, 1, 1) = y;
}

// One frame of the animation, after the fade in: a build, an upload, a test; a reset step is a
// build and a test. True when the test sees a press.
bool animation_frame(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads) {
    auto& award = state.award;
    switch (award.phase) {
    case AwardPhase::build:
        award.phase = build_step(state, content) ? AwardPhase::upload : AwardPhase::test;
        return false;
    case AwardPhase::upload:
        upload_step(state, content, pads);
        award.phase =
            award.medal_step == second_art ? AwardPhase::second_art_low : AwardPhase::test;
        return false;
    case AwardPhase::second_art_low:
        second_art_frame(state, content, 0);
        award.phase = AwardPhase::second_art_high;
        return false;
    case AwardPhase::second_art_high:
        second_art_frame(state, content, 1);
        award.phase = AwardPhase::bounce;
        return false;
    case AwardPhase::bounce:
        second_art_frame(state, content, 2);
        award.phase = AwardPhase::test;
        return false;
    case AwardPhase::test: award.phase = AwardPhase::build; return award_left(state);
    }
    return false;
}

// $83:8853-88C5: the rider's level from the medals of tours 0-7, by exact counts: all 24 gold is
// 3, six tours at silver or better 2, four at bronze or better 1; once 3, no change.
void apply_unlock_rule(FrontEndState& state) {
    auto& records = state.records;
    const auto rider = state.rider_menu.rider;
    auto& level = records.tour_levels[rider & 15U];
    if (level == 3) return;
    unsigned sum = 0, bronze = 0, silver = 0;
    for (unsigned tour = 0; tour < 8; ++tour) {
        const auto medal = records.medals[tour * 16U + rider];
        sum += medal;
        bronze += medal >= 1 ? 1 : 0;
        silver += medal >= 2 ? 1 : 0;
    }
    // The original also writes the level to `$77:10FD`, the pending reveal, whenever a count
    // matches, even if the level is unchanged: PICK TOUR then draws the tours of level - 1, slides,
    // and shows the others four frames later. Native shows the level at once (R-0059).
    if (sum == 24)
        level = 3;
    else if (silver == 6)
        level = 2;
    else if (bronze == 4)
        level = 1;
}

} // namespace

void complete_tour(FrontEndState& state) {
    auto& records = state.records;
    auto& menu = state.tour_menu;
    const auto rider = state.rider_menu.rider;
    const auto first = static_cast<std::size_t>(menu.tour) * tracks_per_tour;
    std::fill_n(records.tracks_done.begin() + first, tracks_per_tour, std::uint8_t{0}); // $83:895B
    menu.track = static_cast<std::uint8_t>(menu.track / tracks_per_tour * tracks_per_tour);
    auto& medal = records.medals[menu.tour * 16U + rider];
    const auto raised = static_cast<std::uint8_t>(medal + 1);
    if (raised <= 3) medal = raised; // $83:8832
    state.award = {};
    state.award.medal = raised;
    state.award.after_completion = true;
    state.screen = FrontEndScreen::tour_award;
    state.script_frame = 0;
    // $83:883E: a gold medal plays its tour's ending (`$83:88FD`), HUNTER's resets. Not recovered:
    // native leaves at once through the award's way out (R-0059).
    if (raised >= 3) state.award.exit_frame = 1;
}

void tour_award_frame(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads) {
    auto& award = state.award;
    const auto frame = state.script_frame;
    if (award.exit_frame == 0) {
        if (frame <= blank_frame) {
            fade_down(state, frame);
            return;
        }
        switch (frame) {
        case reset_frame: reset_for_award(state, content); return;
        case background_tiles_frame:
            load_vram(state, asset(content, background_tiles), background_tiles_word);
            return;
        case podium_map_frame: {
            state.registers.obsel = award_objects;
            load_map(state, asset(content, podium_map), podium_map_word, podium_map_bits);
            return;
        }
        case podium_tiles_frame:
            load_vram(state, asset(content, podium_tiles), podium_tiles_word);
            return;
        case colours_frame:
            load_cgram(state, asset(content, podium_colours), 0x10);
            load_cgram(state, asset(content, first_background_colours + award.medal), 0);
            load_vram(state, asset(content, medal_tiles), medal_tiles_word);
            return;
        case objects_frame:
            std::copy_n(content.award_tables.begin() + first_entries_at, 8,
                        state.oam_buffer.begin());
            high_bits(state, 0) = first_objects_high;
            high_bits(state, 116) = high_bits(state, 124) = four_hidden;
            copy_oam(state);
            return;
        case rider_frame:
            award.pose = rider_pose;
            upload_rider(state, content);
            award.step = 0;
            award.medal_step = static_cast<std::int16_t>(medal_start);
            return;
        default: break;
        }
        if (frame >= first_fade_in_frame && frame < first_fade_in_frame + fade_in_frames) {
            fade_up(state, frame - first_fade_in_frame + 1);
            return;
        }
        if (frame < first_fade_in_frame + fade_in_frames) return;
        if (animation_frame(state, content, pads)) award.exit_frame = 1;
        return;
    }
    const auto exit = ++award.exit_frame - 1; // frames after the exit test
    if (exit <= exit_blank_frame) {
        fade_down(state, exit);
        return;
    }
    if (exit == menus_frame) {
        restore_menu_screen(state, content);
        return;
    }
    if (exit >= exit_fade_in_frame && exit <= pick_tour_frame) {
        fade_up(state, exit - exit_fade_in_frame + 1);
        if (exit < pick_tour_frame) return;
        apply_unlock_rule(state); // $83:8853, then PICK TOUR (`$80:E54C`) in the same frame
        // `$00AC` as restored before the race: 2 (NOW PLAYING was left with Y or X) slides PICK
        // TOUR forward and PICK TRACK back (`$80:E92F`); otherwise the other way round.
        return_to_tour_menu(state, !state.track_menu.returning);
    }
}

// $83:88D1-$80:BC7E after PICK TOUR's return from a completion: `$80:A858` (two frames), then
// PICK TRACK, whether PICK TOUR ended on a choice or on Y or X.
void award_return_frame(FrontEndState& state, const FrontEndContent& content) {
    copy_oam(state);
    if (state.script_frame == 1) {
        load_cgram(state, asset(content, base_palette_low), 0);
        return;
    }
    load_cgram(state, asset(content, base_palette_high), 0x40);
    enter_track_menu(state, state.track_menu.returning);
}

} // namespace unirally::front_end_screens
