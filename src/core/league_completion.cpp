// League tour boundaries: podium, then keep/reset/exit ($80:BE76-BEE2, R-0073).
#include "front_end_screens.hpp"

#include <algorithm>
#include <array>

namespace unirally::front_end_screens {
namespace {
constexpr std::array<unsigned, 3> podium_objects{98, 96, 97};
constexpr std::array<unsigned, 3> podium_tile_words{0x6080, 0x6880, 0x6000};
// The podium's sound session ($82:E06F) starts on its entry's frame 19, after the fade and the
// picture's loads (R-0099).
constexpr std::uint32_t podium_session_frame = 19;

// $83:F0FF's composed 64 x 64 objects, encoded back into the native VRAM tile layout.
void upload_podium_rider(FrontEndState& state, const FrontEndContent& content, unsigned rank) {
    const auto pixels =
        compose_rider_object(content.uni_pictures, state.league.podium_poses[2 - rank],
                             std::nullopt, RiderRowClip::none);
    for (unsigned row = 0; row < 8; ++row)
        for (unsigned column = 0; column < 8; ++column) {
            std::array<std::uint8_t, 32> tile{};
            for (unsigned y = 0; y < 8; ++y)
                for (unsigned x = 0; x < 8; ++x) {
                    const auto colour = pixels[(row * 8 + y) * 64 + column * 8 + x];
                    for (unsigned plane = 0; plane < 4; ++plane)
                        tile[(plane / 2) * 16 + y * 2 + plane % 2] |=
                            static_cast<std::uint8_t>(((colour >> plane) & 1U) << (7 - x));
                }
            load_vram(state, tile, podium_tile_words[rank] + row * 0x100 + column * 16);
        }
}

void setup_podium(FrontEndState& state, const FrontEndContent& content) {
    state.video.vram.fill(0);
    clear_oam_buffer(state);
    for (unsigned object = 0; object < 128; ++object) high_bits(state, object) = four_hidden;
    // $82:DFF4-E06E: map/tiles and palettes; $82:E103-E12A: mode 1, 64 x 64 objects.
    load_vram(state, asset(content, 0x80), 0x1000);
    load_vram(state, asset(content, 0x92), 0x7000);
    load_vram(state, asset(content, 0x7f), 0);
    load_vram(state, asset(content, 0x91), 0x2000);
    load_cgram(state, asset(content, 0xa3), 0);
    load_cgram(state, asset(content, 0xa2), 0x70);
    auto& r = state.registers;
    r = {};
    r.force_blank = true;
    r.mode = 1;
    r.obsel = 0x83;
    r.main_screen = 0x13;
    r.bg[0] = {0x2000, 0, 0, false, 0, 0};
    r.bg[1] = {0x7000, 3, 0x1000, false, 0, 0};
    // $82:DF9E-DFDC assigns each rank's position, palette and tile bank.
    constexpr std::array<std::uint8_t, 3> x{0x5f, 0x34, 0x8d}, y{0x72, 0x7b, 0x80};
    constexpr std::array<std::uint8_t, 3> tiles{0x08, 0x88, 0x00}, attributes{0x26, 0x28, 0x2a};
    const auto& riders = state.records.league_pairings[state.league.slot];
    for (unsigned rank = 0; rank < 3; ++rank) {
        load_cgram(
            state,
            asset(content, first_rider_palette + (riders[rank] >= someone ? 15U : riders[rank])),
            0xb0 + rank * 16);
        if (riders[rank] >= someone) continue;
        const auto object = podium_objects[rank];
        oam_byte(state, object, 0) = x[rank];
        oam_byte(state, object, 1) = y[rank];
        oam_byte(state, object, 2) = tiles[rank];
        oam_byte(state, object, 3) = attributes[rank];
        high_bits(state, object) = static_cast<std::uint8_t>(
            (high_bits(state, object) & ~(3U << ((object % 4) * 2))) | (2U << ((object % 4) * 2)));
    }
    copy_oam(state);
}

void print_continue(FrontEndState& state, const FrontEndContent& content) {
    state.text.words.fill(cleared_text);
    for (unsigned row = 0; row < 3; ++row) {
        const auto stream = nth_string(content.league_continue_text, row);
        print_text(state.text, state.printer, {stream.data(), stream.size() + 1},
                   content.character_table);
    }
    load_text(state, state.slide.hidden_half);
}

// $80:B93C-B975 with `$00AE` = `$80:FC2F`: the arrow's x target is the entry's column (2, 1 and 8,
// in units of 8 pixels), its y target the entry's row from 0x0580, 0x0180 apart.
void aim_continue_arrow(FrontEndState& state) {
    constexpr std::array<std::uint8_t, 3> columns{2, 1, 8}; // $80:FC2F
    constexpr std::uint16_t first_row_y = 0x0580, row_height = 0x0180;
    state.arrow.target_x = static_cast<std::uint16_t>(columns[state.menu.selection] * 128);
    state.arrow.target_y =
        static_cast<std::uint16_t>(first_row_y + state.menu.selection * row_height);
}

void move_continue_arrow(FrontEndState& state, std::uint16_t pad) {
    constexpr std::uint8_t last_entry = 2; // `$009D`
    const bool down = (pad & (pad_down | pad_select)) != 0;
    const bool up = !down && (pad & pad_up) != 0;
    if (!down && !up) {
        state.latches.moved = false;
        return;
    }
    if (state.latches.moved) return;
    state.latches.moved = true;
    play_menu_sound(state, MenuSound::navigate); // $80:BA13 (Down), $80:B9B1 (Up)
    auto& entry = state.menu.selection;
    if (down)
        entry = entry == last_entry ? 0 : static_cast<std::uint8_t>(entry + 1);
    else
        entry = entry == 0 ? last_entry : static_cast<std::uint8_t>(entry - 1);
    aim_continue_arrow(state);
}

// $82:DE10-DE5B, the loop's exit test on the pad words `$82:AA75-AC4C` sets from `$0311-$0314`
// (`$80:85E8-85FD`, read each frame): any of A, X, L, R, B, Y, Select and Start on either pad, or
// Left or Right held on either pad (`$0319`, `$031B`: 0 Left, 2 Right, 1 neither). Up and Down
// never count (R-0099: rv-up-pad2).
bool leaves_podium(FrontEndPads pads) {
    constexpr std::uint16_t exit_buttons = 0xf3f0; // all but Up (0x0800) and Down (0x0400)
    return ((pads.one | pads.two) & exit_buttons) != 0;
}

// $82:DEBC-DEFE, after a pass's poses: the loop counts its passes to 8 in `$1365`, and the first
// three each send effect gain 255, music gain 127 and the music start with eight dispatcher calls
// (R-0099). A pass that leaves on its exit test returns before it ($82:DE45).
void podium_music_pass(FrontEndState& state) {
    constexpr std::uint16_t counted_passes = 8, music_passes = 3;
    auto& passes = state.league.podium_music_passes;
    if (passes == counted_passes) return;
    if (++passes > music_passes) return;
    // The driver's commands: 8 the effect gain, 7 the music gain, 1 the music start (argument 2,
    // as every session's music start).
    constexpr std::uint8_t effect_gain = 8, music_gain = 7, music_start = 1;
    constexpr std::uint8_t full_gain = 255, usual_music_gain = 127, start_argument = 2;
    constexpr unsigned dispatcher_calls = 8;
    // The audio side places each pass's calls at its own clock (D-0010).
    constexpr std::array<AudioDispatchSite, music_passes> sites{
        AudioDispatchSite::podium_first_pass, AudioDispatchSite::podium_second_pass,
        AudioDispatchSite::podium_third_pass};
    state.sound_cues.push_back(audio_enqueue(effect_gain, full_gain));
    state.sound_cues.push_back(audio_enqueue(music_gain, usual_music_gain));
    state.sound_cues.push_back(audio_enqueue(music_start, start_argument));
    for (unsigned call = 0; call < dispatcher_calls; ++call)
        state.sound_cues.push_back(audio_dispatch(sites[static_cast<std::size_t>(passes) - 1]));
}
} // namespace

void enter_league_podium(FrontEndState& state) {
    state.records.league_cycle_complete = false;
    state.league.podium_poses = {0x0a45, 0x0a4d, 0x0a55};
    state.league.podium_phase = 0;
    state.league.podium_music_passes = 0;
    state.screen = FrontEndScreen::league_podium_entry;
}

void league_podium_entry_frame(FrontEndState& state, const FrontEndContent& content,
                               FrontEndPads pads) {
    const auto frame = state.script_frame;
    if (frame < 7) {
        state.registers.brightness = static_cast<std::uint8_t>(15 - frame * 2);
        return;
    }
    if (frame == 7) {
        // As before a race ($80:99B7-99BE), the menu music fades (rate -112 x 8) at once (R-0098:
        // tour-cues 11408).
        constexpr std::uint8_t music_fade = 3, race_fade_rate = 0x90;
        state.sound_cues.push_back(audio_enqueue(music_fade, race_fade_rate));
        state.sound_cues.push_back(audio_dispatch(AudioDispatchSite::race_choice));
        state.saved = {state.menu,        state.cycle,       state.logo.offset, state.slide,
                       state.decorations, state.latches,     state.rider_menu,  state.tour_menu,
                       state.track_menu,  state.now_playing, state.printer,     state.arrow.spin};
        state.cycle.running = false;
        state.slide.scroll = 0;
        state.slide.shown_half = 0x1000;
        state.slide.hidden_half = 0x1400;
    }
    // $82:E06F: the podium's sound session starts while its picture loads (R-0099).
    if (frame == podium_session_frame)
        state.sound_cues.push_back(audio_load(AudioSessionLoad::podium));
    if (frame <= 98) state.registers.force_blank = true;
    if (frame == 98) setup_podium(state, content);
    if (frame >= 99) {
        // The loop's first pass reads the pads like every other ($82:DE06-DE45): a press held as
        // the podium appears leaves before its music (R-0099: rv-press1).
        league_podium_frame(state, content, pads);
        if (state.screen != FrontEndScreen::league_podium_entry) return;
        fade_up(state, 1);
        if (frame == 99) state.screen = FrontEndScreen::league_podium;
    }
}

void league_podium_frame(FrontEndState& state, const FrontEndContent& content, FrontEndPads pads) {
    // $82:DF06-DF9D increments the pose selectors on phase 2 and wraps at $0A5D.
    state.league.podium_phase = static_cast<std::uint16_t>((state.league.podium_phase + 1) & 3U);
    // NMI uploads the previous pass before the pose selectors advance. The sprite tile
    // banks display third/second/first selectors in winner order ($82:DF9E, R-0073).
    if (state.screen == FrontEndScreen::league_podium)
        for (unsigned rank = 0; rank < 3; ++rank) upload_podium_rider(state, content, rank);
    const auto phase = state.league.podium_phase;
    if (phase == 0 || phase == 2) {
        const unsigned first = phase == 2 ? 0U : 2U;
        const unsigned last = phase == 2 ? 2U : 3U;
        for (unsigned rank = first; rank < last; ++rank) {
            auto& pose = state.league.podium_poses[rank];
            if (++pose == 0x0a5d) pose = 0x0a45;
        }
    }
    // The podium uses the race fade: one brightness step for each two NMI updates.
    fade_up(state, std::min<std::uint32_t>(15, state.script_frame + 1));
    copy_oam(state);
    if (leaves_podium(pads)) {
        begin_menu_restore(state, content);
        state.screen = FrontEndScreen::league_podium_exit;
        return;
    }
    podium_music_pass(state);
}

void league_podium_exit_frame(FrontEndState& state, const FrontEndContent& content) {
    constexpr std::uint32_t podium_restore_frame = restore_frame + 2;
    const auto frame = state.script_frame;
    // Podium teardown has two more blank frames than the retained race returns, but the title's
    // sound program is loaded on the same frame as after a race (`$80:A0F7`, R-0098: the tour-cues
    // schedule).
    constexpr std::uint32_t sound_upload_frame = 4;
    if (frame == sound_upload_frame)
        state.sound_cues.push_back(audio_load(AudioSessionLoad::title_return));
    if (frame <= 2) return;
    if (frame <= 105) {
        // Left on podium phase 2 or 3, the arrow's spin, which `$83:987D` puts back with the
        // menus, comes back a frame after the rest (R-0095).
        const auto spin = state.arrow.spin;
        state.script_frame -= 2;
        restore_menus_after_race(state, content);
        state.script_frame = frame;
        if (podium_arrow_late(state)) {
            if (frame == podium_restore_frame) state.arrow.spin = spin;
            if (frame == podium_restore_frame + 1) state.arrow.spin = state.saved.arrow_spin;
        }
        return;
    }
    if (frame == 106) {
        load_cgram(state, asset(content, base_palette_high), 0x40);
        state.slide.hidden_half = 0x1400;
        state.slide.shown_half = 0x1000;
        state.slide.scroll = 0;
        state.registers.bg[1].hofs = state.registers.bg[1].vofs = 0;
        raise_logo(state);
        print_continue(state, content);
        state.screen = FrontEndScreen::league_continue_entry;
    }
}

void league_continue_entry_frame(FrontEndState& state, const FrontEndContent& content) {
    const auto frame = state.script_frame;
    if (frame == 1) {
        copy_oam(state);
        start_slide(state, content, false);
        return;
    }
    if (frame <= 40) {
        if (!slide_frame(state, content)) return;
        return;
    }
    fade_up(state, 2 * (frame - 40));
    copy_oam(state);
    if (frame < 47) return;
    // $80:B93C's set-up: the first entry, no latches, the arrow sent to it, and its first pass's
    // `$80:EB23`, which turns the hidden PICK TRACK markers (R-0097, R-0098).
    state.menu.selection = 0;
    state.latches = {};
    aim_continue_arrow(state);
    turn_markers(state, content);
    state.screen = FrontEndScreen::league_continue;
}

// `$80:FBEE`, the generic list loop `$80:B93C` on KEEP SCORES, RESET SCORES and EXIT: no
// decorations stepped (R-0095), the pads through the gate, Down (or Select) before Up once a press
// with the move's sound and a wrap past either end (`$80:B983-BA5E`), the hidden markers turned
// on a pass that stays. B, Start or A, and Y or X too, return to `$80:FBF1`, which acts on the
// entry the arrow is on (R-0098).
void league_continue_frame(FrontEndState& state, const FrontEndContent& content,
                           FrontEndPads pads) {
    copy_oam(state); // $80:D1EC
    const auto pad = gated_pads(state, pads);
    if (!(pad & (choose_buttons | back_buttons))) {
        move_continue_arrow(state, pad);
        turn_markers(state, content);
        return;
    }
    if (state.menu.selection == 2) {
        print_main_menu(state, content);
        state.screen = FrontEndScreen::main_menu_return;
        return;
    }
    // $80:FC32: reset the census counters without changing the saved pairing order.
    if (state.menu.selection == 1) {
        state.records.league_scores[state.league.slot].fill(0);
        state.records.league_played[state.league.slot].fill(0);
    }
    state.league.after_podium = true;
    // $80:BEC6: PICK TOUR from `$80:E550` with no `$80:A858` or `$80:A82B` before it, `$00AC` still
    // 1 from `$80:FBD7`: the menu slides back (R-0098).
    return_to_tour_menu(state, true);
}
} // namespace unirally::front_end_screens
