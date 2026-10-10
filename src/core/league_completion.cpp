// League tour boundaries: podium, then keep/reset/exit ($80:BE76-BEE2, R-0073).
#include "front_end_screens.hpp"

#include <algorithm>
#include <array>

namespace unirally::front_end_screens {
namespace {
constexpr std::array<unsigned, 3> podium_objects{98, 96, 97};
constexpr std::array<unsigned, 3> podium_tile_words{0x6080, 0x6880, 0x6000};

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
} // namespace

void enter_league_podium(FrontEndState& state) {
    state.records.league_cycle_complete = false;
    state.league.podium_poses = {0x0a45, 0x0a4d, 0x0a55};
    state.league.podium_phase = 0;
    state.screen = FrontEndScreen::league_podium_entry;
}

void league_podium_entry_frame(FrontEndState& state, const FrontEndContent& content) {
    const auto frame = state.script_frame;
    if (frame < 7) {
        state.registers.brightness = static_cast<std::uint8_t>(15 - frame * 2);
        return;
    }
    if (frame == 7) {
        state.saved = {state.menu,        state.cycle,       state.logo.offset, state.slide,
                       state.decorations, state.latches,     state.rider_menu,  state.tour_menu,
                       state.track_menu,  state.now_playing, state.printer,     state.arrow.spin};
        state.cycle.running = false;
        state.slide.scroll = 0;
        state.slide.shown_half = 0x1000;
        state.slide.hidden_half = 0x1400;
    }
    if (frame <= 98) state.registers.force_blank = true;
    if (frame == 98) setup_podium(state, content);
    if (frame >= 99) {
        league_podium_frame(state, content, {});
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
    if (any_button_pressed(pads)) {
        begin_menu_restore(state, content);
        state.screen = FrontEndScreen::league_podium_exit;
    }
}

void league_podium_exit_frame(FrontEndState& state, const FrontEndContent& content) {
    constexpr std::uint32_t podium_restore_frame = restore_frame + 2;
    const auto frame = state.script_frame;
    // Podium teardown has two more blank frames than the retained race returns.
    if (frame <= 2) return;
    if (frame <= 105) {
        // Left on podium phase 2 or 3, the arrow's spin, which `$83:987D` puts back with the
        // menus, comes back a frame after the rest (R-0095).
        const auto spin = state.arrow.spin;
        state.script_frame -= 2;
        race_return_frame(state, content);
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
    state.menu.selection = 0;
    state.arrow.target_x = 0x0100;
    state.arrow.target_y = 0x0580;
    state.latches = {};
    state.screen = FrontEndScreen::league_continue;
}

void league_continue_frame(FrontEndState& state, const FrontEndContent& content,
                           FrontEndPads pads) {
    copy_oam(state);
    step_decorations(state, content);
    constexpr std::array<std::uint8_t, 3> columns{2, 1, 8}; // $80:FC2F
    const auto pad = static_cast<std::uint16_t>(pads.one | pads.two);
    const bool down = (pad & (pad_down | pad_select)) != 0;
    const bool up = !down && (pad & pad_up) != 0;
    if (!down && !up)
        state.latches.moved = false;
    else if (!state.latches.moved) {
        state.latches.moved = true;
        state.menu.selection =
            static_cast<std::uint8_t>((state.menu.selection + (down ? 1U : 2U)) % 3U);
    }
    state.arrow.target_x = static_cast<std::uint16_t>(columns[state.menu.selection] * 128);
    state.arrow.target_y = static_cast<std::uint16_t>(0x0580 + state.menu.selection * 0x0180);
    const auto buttons = static_cast<std::uint16_t>(pads.one | pads.two);
    if (!(buttons & choose_buttons)) return;
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
    enter_tour_menu(state);
}
} // namespace unirally::front_end_screens
