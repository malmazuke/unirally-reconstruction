// The rider-name cheats (R-0094). Each race's setup tests the first rider's name (`$83:FB8A`): a
// rider named "credits" brings up the programmers' picture (`$83:FAE0`) in place of the race,
// which then ends as a restart; one named "faedine" runs HUNTER's tag effects and opponent tier
// on any track for three races. Either name becomes "mike".

#include "front_end_screens.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace unirally::front_end_screens {
namespace {

// `front-end.name-cheats` ($83:FB56-FB89): "credits" padded to 16 bytes, the name it writes (10),
// then "faedine" (16) and its name (10). Seven letters are compared ($83:FBAE, $83:FBD5) and ten
// bytes written ($83:FBBA, $83:FBE2): "mike____", 0xFF, 0xFF.
constexpr std::size_t name_cheats_bytes = 52, credits_name = 0x00, credits_written = 0x10,
                      faedine_name = 0x1a, faedine_written = 0x2a;
constexpr std::size_t compared_letters = 7, written_bytes = 10;
// $83:FBDC: the races "faedine" gives, also the most a count keeps ($83:FB95-FB99).
constexpr std::uint8_t faedine_races = 3;
// `$77:111B`, which the race's `$131F` test reads with `$77:111A` as a word ($82:D978) and the
// demo clears with it; nothing else writes it.
constexpr std::size_t hunter_races_high = 0x111b;

// The picture's assets (`$83:FB03-FB1D`): colours at CGRAM 0, the map at VRAM word 0, the tiles
// at 0x1000.
constexpr unsigned picture_colours = 0xc1, picture_map = 0xc0, picture_tiles = 0xbf;
constexpr unsigned picture_tiles_word = 0x1000;
// $82:D4C4 clears 160 CGRAM bytes, colours 0-79, before the picture loads.
constexpr std::size_t cleared_colour_bytes = 160;
// $83:FAE0-FAFE: BG mode 1, BG1's tiles at 0x1000 (BG12NBA 1), BG1 alone on the main screen.
constexpr std::uint8_t picture_mode = 1, picture_tile_bases = 0x01, picture_main_screen = 0x01;

// The picture's frames, from the first blank frame after the menus' fade (frame 0). The race's
// setup tests the name on frame 7. The loads end in forced blank, and NMI comes on with the race's
// hook (`$80:85A4`, from `$82:DDD5`) in frame 16, which raises `$0FF1` by one a frame to 15 and
// writes it as the brightness. `$80:9869` fades in over the next 7 frames (2 to 14), 501 frame
// waits follow (`$83:FB31-FB39`: Y from 500 to 0), and `$80:9885` fades out over 7 frames (13 to
// 1) and blanks the screen on the last: the race's end.
constexpr std::uint32_t name_test_frame = 7, first_shown_frame = 16, fade_frames = 7,
                        held_frames = 501;
constexpr std::uint32_t first_held_frame = first_shown_frame + fade_frames + 1;
constexpr std::uint32_t first_fade_out_frame = first_held_frame + held_frames;
constexpr std::uint32_t end_frame = first_fade_out_frame + fade_frames - 1;
constexpr std::uint8_t brightest = 15, first_fade_out_brightness = 13;
// $83:C9AC: the player's total the restart reads (R-0060).
constexpr std::uint16_t restart_total = 0xea62;

// Whether the first rider's name starts with the cheat's seven letters at `at`.
bool first_rider_named(const FrontEndState& state, const FrontEndContent& content, std::size_t at) {
    if (content.name_cheats.size() != name_cheats_bytes) return false; // an older pack: no cheats
    return std::equal(content.name_cheats.begin() + static_cast<std::ptrdiff_t>(at),
                      content.name_cheats.begin()
                          + static_cast<std::ptrdiff_t>(at + compared_letters),
                      state.records.rider_names.begin());
}

void write_first_name(FrontEndState& state, const FrontEndContent& content, std::size_t at) {
    std::copy_n(content.name_cheats.begin() + static_cast<std::ptrdiff_t>(at), written_bytes,
                state.records.rider_names.begin());
}

// $83:FB8C-FB9B: one race fewer, kept within 0-3. The count is 8-bit and signed: one that goes
// below 0 (from 0, or from 0x81 and up) stays 0, and one of 4 or more is 3.
std::uint8_t one_race_fewer(std::uint8_t races) {
    const auto fewer = static_cast<std::uint8_t>(races - 1U);
    if (fewer & 0x80U) return 0;
    return std::min(fewer, faedine_races);
}

// $83:FAE0-FB1D in forced blank, after `$82:D6E7`'s clears of VRAM and colours 0-79 and `$82:D7C2`'s
// of the race's object buffer, which the race's hook sends to OAM each frame.
void load_picture(FrontEndState& state, const FrontEndContent& content) {
    state.video.vram.fill(0);
    std::fill_n(state.video.cgram.begin(), cleared_colour_bytes, 0);
    state.video.oam.fill(0);
    load_cgram(state, asset(content, picture_colours), 0);
    load_vram(state, asset(content, picture_map), 0);
    load_vram(state, asset(content, picture_tiles), picture_tiles_word);
    auto& r = state.registers;
    r.mode = picture_mode;
    set_tile_bases(r, picture_tile_bases);
    r.main_screen = picture_main_screen;
    // The hook writes 0 to BG1's and BG2's vertical offsets each frame.
    r.bg[0].vofs = r.bg[1].vofs = 0;
}

// The brightness of picture frame `frame` (`first_shown_frame` on).
std::uint8_t picture_brightness(std::uint32_t frame) {
    if (frame <= first_shown_frame) return 1; // the hook's first count
    if (frame < first_held_frame) return static_cast<std::uint8_t>(2 * (frame - first_shown_frame));
    // Then the hook's own count, one a frame since the frame before the picture showed, to 15.
    if (frame < first_fade_out_frame)
        return static_cast<std::uint8_t>(
            std::min<std::uint32_t>(brightest, frame - (first_shown_frame - 1)));
    return static_cast<std::uint8_t>(first_fade_out_brightness
                                     - 2 * (frame - first_fade_out_frame));
}

} // namespace

bool credits_named(const FrontEndState& state, const FrontEndContent& content) {
    return first_rider_named(state, content, credits_name);
}

void check_rider_names(FrontEndState& state, const FrontEndContent& content) {
    auto& races = state.records.hunter_races;
    races = one_race_fewer(races);
    if (first_rider_named(state, content, credits_name)) {
        write_first_name(state, content, credits_written);
    } else if (first_rider_named(state, content, faedine_name)) {
        races = faedine_races;
        write_first_name(state, content, faedine_written);
    }
}

void clear_hunter_races(FrontEndState& state) {
    state.records.hunter_races = 0;
    state.cartridge[hunter_races_high] = 0;
}

void enter_credits_picture(FrontEndState& state) {
    // $83:C8E4-C8E8: NMI and HDMA off; the menus' hook stops until their return restores it.
    state.cycle.running = false;
    state.line_colours.clear();
    state.line_registers.clear();
    state.registers.force_blank = true;
    state.screen = FrontEndScreen::credits_picture;
}

void credits_picture_frame(FrontEndState& state, const FrontEndContent& content) {
    const auto frame = state.script_frame;
    if (frame == name_test_frame) {
        check_rider_names(state, content);
        load_picture(state, content);
    }
    if (frame < first_shown_frame) return;
    if (frame < end_frame) {
        state.registers.brightness = picture_brightness(frame);
        state.registers.force_blank = false;
        return;
    }
    // $80:9885's last write, then `$82:DF05` and the teardown `$83:F97C`: the race is over, a
    // restart for the menus (R-0060), or for the idle demo the timer's return to the main menu.
    state.registers.brightness = 1;
    state.registers.force_blank = true;
    if (state.mode == FrontEndMode::demo) {
        begin_demo_return(state, idle_demo_timer_exit);
        // Measured on one warm boot (track 1): the return holds its frame 100 once, as after the
        // timer's exits on tracks 20, 24 and 36 (R-0087).
        state.demo_return_held = true;
        return;
    }
    RaceTimes times;
    times.player_total = restart_total;
    start_race_return(state, content, times);
}

bool credits_picture_waits(const FrontEndState& state) {
    const auto next = state.script_frame + 1;
    return next > first_shown_frame && next <= end_frame;
}

} // namespace unirally::front_end_screens

namespace unirally {

bool hunter_races_running(const FrontEndState& state) {
    return state.records.hunter_races != 0
        || state.cartridge[front_end_screens::hunter_races_high] != 0;
}

} // namespace unirally
