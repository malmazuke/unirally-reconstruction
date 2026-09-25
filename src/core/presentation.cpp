#include "presentation.hpp"

#include "content_pack.hpp"
#include "picture.hpp"
#include "race_hud.hpp"
#include "result_screen.hpp"
#include "rider_object.hpp"
#include "zoom_zoo_movement.hpp"
#include "zoom_zoo_pack.hpp"
#include <algorithm>
#include <array>
#include <bitset>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>

// The race picture: the content it draws from, the track's name, and render_classic_race.
namespace unirally {

namespace {

// The decoded track a race runs on: the engine's own entry.
std::span<const std::uint8_t> classic_track_data(const ClassicContentPack& pack,
                                                 ClassicRaceTrack track) {
    return classic_race_content(pack, track).movement.sampling.track;
}

// A track's entry in the ROM's name table (`$83:9FFA`), with its `$FF`.
std::span<const std::uint8_t> classic_track_name_entry(const ClassicContentPack& pack,
                                                       ClassicRaceTrack track) {
    const auto table = pack.entry("presentation.classic.track-names.v1");
    std::size_t at = 0;
    for (unsigned skipped = 0; skipped < track.index; ++skipped) {
        while (at < table.size() && table[at] != 0xffU) ++at;
        if (at == table.size())
            throw std::invalid_argument("track name table is shorter than the track index");
        ++at;
    }
    std::size_t end = at;
    while (end < table.size() && table[end] != 0xffU) ++end;
    if (end == table.size()) throw std::invalid_argument("track name lacks its terminator");
    return table.subspan(at, end - at + 1);
}

// A track's name from the ROM's name table (`$83:9FFA`, TRACK-BREADTH): the
// index-th `$FF`-terminated lowercase string, shown in capitals with spaces
// for underscores, as the menu screens show it.
std::string classic_track_name(const ClassicContentPack& pack, ClassicRaceTrack track) {
    const auto table = pack.entry("presentation.classic.track-names.v1");
    std::size_t at = 0;
    for (unsigned skipped = 0; skipped < track.index; ++skipped) {
        while (at < table.size() && table[at] != 0xffU) ++at;
        if (at == table.size())
            throw std::invalid_argument("track name table is shorter than the track index");
        ++at;
    }
    std::string name;
    for (; at < table.size() && table[at] != 0xffU; ++at) {
        const char c = static_cast<char>(table[at]);
        name.push_back(c == '_' ? ' '
                                : static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
    }
    return name;
}

} // namespace

void ClassicRaceHistoryTracker::reset() {
    look_ = {};
    latest_ = {};
    on_screen_ = {};
    latest_barf_ = on_screen_barf_ = latest_flip_prior_ = on_screen_flip_prior_ = false;
    opponent_finish_frame_.reset();
    window_.reset();
    clock_.reset();
    transition_member_.reset();
}

void ClassicRaceHistoryTracker::observe_update(const ZoomZooState& previous,
                                               const ZoomZooState& updated,
                                               const ClassicContentPack& pack) {
    // R-0036: update N builds its objects with overlays chosen from the look
    // state before its own look step; picture N+1 shows them.
    on_screen_ = latest_;
    on_screen_barf_ = latest_barf_;
    on_screen_flip_prior_ = latest_flip_prior_;
    latest_flip_prior_ = previous.hunter.blink != 0;
    // A skipped update runs no BG scroll routine, so it keeps the last one's.
    if (!previous.hunter.skip_update) latest_barf_ = previous.hunter.effect[0] != 0;
    if (!previous.race.riders[1].finished && updated.race.riders[1].finished)
        opponent_finish_frame_ = updated.movement.frame;
    if (!transition_member_)
        transition_member_ =
            classic_window_transition_member(classic_track_data(pack, updated.track));
    window_.observe_update(previous, updated, *transition_member_);
    clock_.observe_update(previous, updated);
    if (updated.result_updates || zoom_zoo_update_was_paused(previous, updated)) return;
    const auto tables = rider_look_tables(pack);
    const auto engine = classic_race_content(pack, updated.track);
    latest_.pose = rider_overlay_poses(look_, updated, tables);
    advance_rider_look(look_, updated, engine, tables);
}

ClassicRacePresentationContent classic_race_presentation_content(const ClassicContentPack& pack,
                                                                 ClassicRaceTrack track) {
    ClassicRacePresentationContent content;
    content.scenario = classic_race_scenario(track);
    content.riders = rider_object_content(pack);
    // One ROM table serves both tracks (R-0037); its accepted entry name
    // predates DRAGSTER reading it and cannot be renamed.
    content.race_palette_cycle = pack.entry("presentation.zoom.race-palette-cycle.v1");
    // One ROM window family serves both tracks too (R-0040; ZOOM ZOO's
    // selection measured in ZOOM-ZOO-WINDOW-EFFECTS).
    content.window_tables = pack.entry("presentation.effect.classic.window-tables.v1");
    // R-0042: the caption table, likewise one table for both tracks.
    content.captions = pack.entry("presentation.classic.captions.v1");
    content.caption_font = pack.entry("presentation.classic.font.v1");
    content.track = classic_track_data(pack, track);
    content.window_transition_member = classic_window_transition_member(content.track);
    if (track != ClassicRaceTrack::ZoomZoo && track != ClassicRaceTrack::Dragster) {
        // TRACK-BREADTH part 3: the track's own BG1 tiles and its scenery's BG2
        // tiles, map and race palette (scenery = track mod 14, $82:DC20-DD84).
        // The result screen follows the race mode's accepted track (the
        // DRAGSTER assets for a one-run race, titled with the track's own
        // name); EAST's and FLAT FUN's results were compared with the original.
        const auto scenery = track.index % 14U;
        const auto name =
            std::string("scenery.") + char('0' + scenery / 10U) + char('0' + scenery % 10U) + '.';
        content.track_name = classic_track_name(pack, track);
        if (content.scenario.hunter_tour)
            content.hunter_opponent_palette =
                pack.entry("presentation.classic.hunter-opponent-palette.v1");
        content.bg1_tiles = pack.entry(classic_track_entry(track, "bg1-tiles"));
        content.bg2_tiles = pack.entry(name + "bg2-tiles");
        content.bg2_map = pack.entry(name + "bg2-map");
        content.palette = pack.entry(name + "palette");
        if (!content.scenario.tour_race) {
            content.result_assets = pack.entry("presentation.result.classic.font-layout.v1");
            content.result_base_vram = pack.entry("presentation.result.classic.base-vram.v1");
            content.result_palette = pack.entry("presentation.result.classic.palette.v1");
            content.result_palette_tail = pack.entry("presentation.result.classic.palette-tail.v1");
            content.result_track_name = classic_track_name_entry(pack, track);
            // Refuse a name the title font cannot draw when the content loads,
            // not at the first result frame.
            for (const auto byte :
                 content.result_track_name.first(content.result_track_name.size() - 1U))
                if (byte != '_') (void)result_title_tile(static_cast<char>(byte));
        }
        content.geometry = track_geometry(content.track);
        return content;
    }
    switch (track.index) {
    case ClassicRaceTrack::ZoomZoo.index:
        content.track_name = "ZOOM ZOO";
        content.bg1_tiles = pack.entry("zoom.bg1-tiles");
        content.bg2_tiles = pack.entry("zoom.bg2-tiles");
        content.bg2_map = pack.entry("zoom.bg2-map");
        content.palette = pack.entry("zoom.palette");
        break;
    case ClassicRaceTrack::Dragster.index:
        content.track_name = "DRAGSTER";
        content.bg1_tiles = pack.entry("presentation.track.dragster.bg1-tiles.v1");
        content.bg2_tiles = pack.entry("presentation.track.dragster.bg2-tiles.v1");
        content.bg2_map = pack.entry("presentation.track.dragster.bg2-map.v1");
        content.palette = pack.entry("presentation.classic.palette.v1");
        content.result_assets = pack.entry("presentation.result.classic.font-layout.v1");
        content.result_base_vram = pack.entry("presentation.result.classic.base-vram.v1");
        content.result_palette = pack.entry("presentation.result.classic.palette.v1");
        content.result_palette_tail = pack.entry("presentation.result.classic.palette-tail.v1");
        break;
    }
    content.geometry = track_geometry(content.track);
    return content;
}

namespace {

// The authored tour result (M4-16), where no recovered result assets exist: black until the
// original's first visible result frame (108), then the lap graph fades in.
RgbFrame render_authored_result(const ZoomZooState& state,
                                const ClassicRacePresentationContent& content) {
    RgbFrame frame{};
    if (state.result_updates <= 108) return frame;
    rect(frame, 0, 0, 256, 224, {34, 42, 48});
    ui_text(frame, 70, 12,
            state.race.total_times[0] < state.race.total_times[1] ? "WINNER" : "RUNNER UP");
    ui_text(frame, 12, 32, "PLAYER       TOTAL    BEST LAP");
    const unsigned minimum = state.result.graph_minimum, maximum = state.result.graph_maximum;
    for (unsigned i = 0; i < 2; ++i) {
        unsigned best = 60000;
        for (auto lap : state.race.lap_times[i])
            if (lap < 60000) {
                best = std::min(best, unsigned(lap));
            }
        ui_text(frame, 12, 45 + int(i) * 13, i ? "BRONSEN" : "MIKE");
        ui_text(frame, 90, 45 + int(i) * 13, result_time(state.result.published_totals[i]));
        ui_text(frame, 156, 45 + int(i) * 13, result_time(best));
    }
    // $83:905F-90ED excludes sentinels and enforces a 200cs graph range.

    rect(frame, 45, 83, 1, 96, {180, 180, 100});
    rect(frame, 45, 178, 170, 1, {180, 180, 100});
    ui_text(frame, 3, 83, race_time(maximum));
    ui_text(frame, 3, 169, race_time(minimum));
    const unsigned laps = std::clamp<unsigned>(content.scenario.laps, 1U, 10U);
    // Authored layout, not recovered: three laps sit 55 pixels apart as
    // before, and other counts share the same 165-pixel span.
    const int lap_step = 165 / static_cast<int>(laps);
    for (unsigned i = 0; i < 2; ++i)
        for (unsigned lap = 0; lap < laps; ++lap) {
            const auto time = state.race.lap_times[i][lap];
            if (time >= 60000) continue;
            const int y =
                178 - static_cast<int>((time - minimum) * 90U / std::max(1U, maximum - minimum));
            rect(frame, 76 + int(lap) * lap_step + int(i) * 5, y - 2, 4, 4,
                 i ? std::array<std::uint8_t, 3>{255, 190, 70}
                   : std::array<std::uint8_t, 3>{255, 80, 90});
        }
    ui_text(frame, 70, 190, std::string("LAPS ON ") + std::string(content.track_name));
    ui_text(frame, 49, 208, "ENTER TO RACE AGAIN");
    const unsigned brightness = std::min(14U, unsigned(state.result_updates - 108U) * 2U);
    for (auto& channel : frame.pixels)
        channel = static_cast<std::uint8_t>(unsigned(channel) * brightness / 14U);
    return frame;
}

// The race's VRAM: BG1 is 16-by-16 tiles, 128 bytes each, the lower row of 8-by-8 tiles 512
// bytes above the upper (both tracks' packs carry them in this order); BG2's tiles and map.
std::array<std::uint8_t, 65536> race_vram(const ClassicRacePresentationContent& content) {
    std::array<std::uint8_t, 65536> vram{};
    const auto tiles = content.bg1_tiles;
    for (std::size_t tile = 0; tile < tiles.size() / 128; ++tile) {
        const auto destination = 0x4000 + (tile / 8) * 1024 + (tile % 8) * 64;
        std::copy_n(tiles.begin() + static_cast<std::ptrdiff_t>(tile * 128), 64,
                    vram.begin() + static_cast<std::ptrdiff_t>(destination));
        std::copy_n(tiles.begin() + static_cast<std::ptrdiff_t>(tile * 128 + 64), 64,
                    vram.begin() + static_cast<std::ptrdiff_t>(destination + 512));
    }
    std::copy(content.bg2_tiles.begin(), content.bg2_tiles.end(), vram.begin() + 0x2000);
    std::copy(content.bg2_map.begin(), content.bg2_map.end(), vram.begin() + 0xe000);
    return vram;
}

// The race's CGRAM at `brightness` (0-15). Colours 96-111 and 0 are cycled by the race NMI
// from ROM tables every frame (R-0037); neither track keys them to rider poses here.
// $82:DDB0-DDBC: OBJ palette 4 is the opponent character's (asset 6 + $77:0749); the
// scenery palette carries the other tours' (R-0052). The PPU scales each 5-bit channel
// before output conversion (bsnes lightTable: luma*c+0.5), so the fade applies to CGRAM
// words, as in the DRAGSTER result fade.
auto race_cgram(const ZoomZooState& state, const ClassicRacePresentationContent& content,
                unsigned brightness) {
    auto cgram = build_race_cgram(content.palette, false);
    if (content.hunter_opponent_palette.size() == 32)
        std::copy(content.hunter_opponent_palette.begin(), content.hunter_opponent_palette.end(),
                  cgram.begin() + 384);
    apply_classic_race_palette_cycle(cgram, content.race_palette_cycle, state,
                                     content.scenario.initialization_frame + 6U);
    if (brightness < 15U) {
        for (std::size_t at = 0; at < cgram.size(); at += 2) {
            const auto faded = apply_snes_brightness(
                static_cast<std::uint16_t>(cgram[at] | (unsigned(cgram[at + 1]) << 8U)),
                brightness);
            cgram[at] = static_cast<std::uint8_t>(faded);
            cgram[at + 1] = static_cast<std::uint8_t>(faded >> 8U);
        }
    }
    return cgram;
}

// Where the race picture's backgrounds scroll, and the HUNTER effects the vblank that
// opened it set up from the update before it (R-0052).
struct RaceScroll {
    int background_x{}, background_y{}; // BG1's world position
    int bg_x{}, bg_y{};                 // BG2's scroll
    bool hide_track{}, flip{};
    int mosaic{1};
};

// HDMA tables are published before the current camera update. Original end1382..6724 tables
// equal the previous camera minus the initial origin; BG2 uses a logical word shift,
// including negative wrapped scrolls. Paused updates leave the camera still while velocity
// persists, so camera - velocity is the previous camera only when the previous update moved
// it: use the previous update's camera whenever it is available. DRAGSTER publishes the same
// relation (original 3453: camera 25266, BG1 24434, BG2 12217 against origin 832). Effect 0
// ($81:AE90) scrolls BG2 by the camera's full y horizontally and its full x vertically;
// effect 4 ($80:8638) takes BG1 off the main screen; effect 6 ($80:8821) turns on the BG1 and
// BG2 mosaic, its size the NMI counter's low three bits; effect 3 ($83:D581-E081) shows the
// playfield upside down through a per-line BG1 scroll table.
RaceScroll race_scroll(const ZoomZooState& state, const ClassicRacePresentationContent& content,
                       const ZoomZooState* previous_update, const ClassicRaceHistory* history) {
    const auto& geometry = content.geometry;
    const auto track = content.track;
    const bool previous_is_prior =
        previous_update && previous_update->movement.frame <= state.movement.frame;
    const int camera_x = state.race.camera.x,
              camera_y = static_cast<std::int16_t>(state.race.camera.y);
    RaceScroll scroll;
    scroll.background_x = previous_is_prior
                            ? previous_update->race.camera.x & geometry.position_mask
                            : (camera_x - static_cast<std::int16_t>(state.race.camera.velocity_x))
                                  & geometry.position_mask;
    scroll.background_y =
        previous_is_prior
            ? static_cast<std::int16_t>(previous_update->race.camera.y)
            : static_cast<std::int16_t>(static_cast<std::uint16_t>(
                  camera_y - static_cast<std::int16_t>(state.race.camera.velocity_y)));
    const auto origin_x =
        static_cast<std::uint16_t>(((unsigned(word(track, 3)) << 4) - 256U) & 0xfff0U);
    const auto origin_y =
        static_cast<std::uint16_t>(((unsigned(word(track, 5)) << 4) - 256U) & 0xfff0U);
    scroll.bg_x = static_cast<std::uint16_t>(scroll.background_x - origin_x) >> 1U;
    scroll.bg_y = static_cast<std::uint16_t>(scroll.background_y - origin_y) >> 1U;
    const auto* effects = previous_update ? &previous_update->hunter : nullptr;
    const bool barf =
        history ? history->hunter_barf : (effects && effects->effect[hunter_effect::barf_mode]);
    if (barf) {
        scroll.bg_x = static_cast<std::uint16_t>(scroll.background_y - origin_y);
        scroll.bg_y = static_cast<std::uint16_t>(scroll.background_x - origin_x);
    }
    scroll.hide_track = effects && effects->hide_track;
    scroll.flip = effects && effects->blink;
    scroll.mosaic =
        effects && effects->mosaic ? static_cast<int>(state.hunter.mosaic_counter & 7U) + 1 : 1;
    return scroll;
}

// BG2, then BG1 over it. $81:A304-A51B: the playfield is 16,384 coarse cells of 64 units in
// the track's column count (256 by 64 for ZOOM ZOO, 1,024 by 16 for DRAGSTER). BG1 map
// entries with bit 13 set are drawn above priority-2 OBJs; returns those pixels.
std::array<bool, 256 * 224> draw_race_backgrounds(RgbFrame& frame,
                                                  const std::array<std::uint8_t, 65536>& vram,
                                                  const std::array<std::uint8_t, 512>& cgram,
                                                  const ClassicRacePresentationContent& content,
                                                  const RaceScroll& scroll) {
    const auto track = content.track;
    const auto& geometry = content.geometry;
    const int columns = geometry.coarse_columns;
    const int world_height = (16384 / columns) * 64;
    std::array<bool, 256 * 224> bg1_above_objects{};
    for (int y = 0; y < 224; ++y)
        for (int x = 0; x < 256; ++x) {
            // A mosaic block repeats its top-left pixel.
            const int mx = x - x % scroll.mosaic, my = y - y % scroll.mosaic;
            const auto background = background_pixel(
                vram, 0xe000, true, true, 0x2000, false, static_cast<std::int16_t>(scroll.bg_x),
                static_cast<std::int16_t>(scroll.bg_y), mx, my);
            pixel(frame, x, y, colour(cgram, background));
            if (scroll.hide_track) continue;
            // Screen row 0 is scanline 1, as in background_pixel's vertical +1. The BG1 map
            // wraps horizontally: the original's map fetch masks the column with `$0D51`
            // ($81:AD05), so past the playfield's right edge the picture continues from column
            // 0, as the sampler's contact does (TRACK-BREADTH, LOOPER). Rows off the playfield
            // are left blank, which is not yet checked against the original's row test at
            // $81:AD22-AD2C.
            const int world_x = (scroll.background_x + mx) & geometry.position_mask,
                      world_y = scroll.flip ? scroll.background_y + 224 - my
                                            : scroll.background_y + my + 1;
            if (world_y < 0 || world_y >= world_height) continue;
            const auto selector = word(
                track, 15 + static_cast<std::size_t>((world_y / 64) * columns + world_x / 64) * 2);
            const auto descriptor =
                word(track, 0x800f + static_cast<std::size_t>(selector) * 32
                                + static_cast<std::size_t>((world_y % 64) / 16) * 8
                                + static_cast<std::size_t>((world_x % 64) / 16) * 2);
            int px = world_x & 15, py = world_y & 15;
            if (descriptor & 0x4000) px = 15 - px;
            if (descriptor & 0x8000) py = 15 - py;
            const auto tile =
                static_cast<std::uint16_t>(((descriptor & 1023) + (px / 8) + (py / 8) * 16) & 1023);
            const auto value = tile_pixel(vram, 0x4000, tile, px & 7, py & 7);
            if (value) {
                pixel(frame, x, y,
                      colour(cgram,
                             static_cast<std::uint8_t>(((descriptor >> 10) & 7) * 16 + value)));
                bg1_above_objects[static_cast<std::size_t>(y) * 256 + static_cast<std::size_t>(x)] =
                    (descriptor & 0x2000) != 0;
            }
        }
    return bg1_above_objects;
}

// R-0036: the riders, opponent first, from the OBJ tiles and OAM the original published in
// the previous update. Entry 98 (player, tile base 0, palette 3) has priority over entry 99
// (opponent, base $88, palette 4); both use OBJ priority 2 outside a corkscrew. Where a rider
// covers caption or HUD ink (R-0042) the original adds red to the sprite, red = min(31,
// sprite_red + 13), green and blue untouched, measured over 35 such pixels on frame 2100 of
// the M4-16 primary and the same on 2120, 2340 and 2600. That is colour-math arithmetic;
// which PPU configuration produces it, and why the added 13 is not half of the ink's own
// 5-bit 28, are not recovered.
void draw_race_riders(RgbFrame& frame, const ZoomZooState& rider_source,
                      const ClassicRacePresentationContent& content,
                      const ClassicRaceHistory* history, const std::array<std::uint8_t, 512>& cgram,
                      bool flip, const std::array<bool, 256 * 224>& bg1_above_objects,
                      const std::bitset<256 * 224>& caption_ink) {
    for (int rider = 1; rider >= 0; --rider) {
        const auto& source = rider_source.movement.riders[static_cast<std::size_t>(rider)];
        auto oam =
            project_rider_oam(source.motion.x, source.motion.y, rider_source.race.camera.x,
                              rider_source.race.camera.y, source.pose.reflected, content.geometry);
        if (flip)
            oam.y = static_cast<std::uint8_t>(0xe0U - static_cast<std::uint8_t>(oam.y + 0x40U));
        oam.vertical_flip = flip || (history && history->hunter_flip_prior);
        if (!oam.visible) continue;
        const auto overlay =
            history ? history->overlays.pose[static_cast<std::size_t>(rider)] : std::nullopt;
        const auto pixels =
            compose_rider_object(content.riders, source.pose.pose_index, overlay, oam.clip);
        const unsigned object_palette = 128U + (rider ? 4U : 3U) * 16U;
        // R-0047: through the corkscrew the object's priority is toggled to 3
        // ($1516/$151A bit 4), above every BG1 tile.
        const bool raised =
            rider_source.special_tiles[static_cast<std::size_t>(rider)].raised_priority != 0;
        draw_rider_object(pixels, oam, [&](int x, int y, std::uint8_t value) {
            const auto at = static_cast<std::size_t>(y) * 256 + static_cast<std::size_t>(x);
            if (bg1_above_objects[at] && !raised) return;
            const auto index = static_cast<std::uint8_t>(object_palette + value);
            if (!caption_ink.test(at)) {
                pixel(frame, x, y, colour(cgram, index));
                return;
            }
            const auto word = colour_word(cgram, index);
            const auto added =
                static_cast<std::uint16_t>(std::min<unsigned>(31U, (word & 31U) + 13U));
            pixel(frame, x, y,
                  {channel8(added), channel8(static_cast<std::uint16_t>((word >> 5U) & 31U)),
                   channel8(static_cast<std::uint16_t>((word >> 10U) & 31U))});
        });
    }
}

} // namespace

// One race picture, in the PPU's layers: the recovered or authored result once it shows;
// else BG2 and BG1, the caption and HUD (BG3), the riders, the window members over all of
// them, and the pause menu.
RgbFrame render_classic_race(const ZoomZooState& state,
                             const ClassicRacePresentationContent& content,
                             const ZoomZooState* previous_update,
                             const ClassicRaceHistory* history) {
    const auto& scenario = content.scenario;
    const bool recovered_result = !content.result_base_vram.empty();
    if (state.result_updates && !recovered_result) return render_authored_result(state, content);
    RgbFrame frame{};
    if (state.result_updates) {
        // The original publishes the completed result at end-of-frame 3678. The accepted
        // gameplay state reaches ResultScreen on the following update, so presentation
        // consumes the observed loading counter without altering the gameplay transition or
        // its serialization. Until then the race picture stays, with the palette phase and
        // window selection of the last race vblank (R-0037, R-0040).
        const auto finish = classic_finish_view(state);
        const bool result_visible =
            finish.phase == RacePhase::ResultScreen
            || (finish.phase == RacePhase::ResultLoading && finish.outcome == RaceOutcome::PlayerWon
                && finish.result_loading_updates >= 225);
        if (result_visible) {
            render_result_background(frame, finish, state.movement.timer,
                                     {content.palette, content.result_assets,
                                      content.result_base_vram, content.result_palette,
                                      content.result_palette_tail, content.result_track_name});
            return frame;
        }
    }
    const auto vram = race_vram(content);
    // NMI $80:883F-8849 writes INIDISP from the preceding update's $0FF1, clamping (fade-15)
    // at zero; $83:CCC1-CCC9 increments $0FF1 once per race update. Scaling converted pixels
    // made mid-fade frames too bright: green 15 at brightness 8 is 47 in the original, not 64.
    const unsigned prior_fade = classic_race_prior_fade(state, previous_update, scenario);
    const auto brightness = prior_fade > 15U ? prior_fade - 15U : 0U;
    const auto cgram = race_cgram(state, content, brightness);
    // Authored UI has no CGRAM entry; fade it through the same 1.5 output curve.
    const auto ui_scale = std::pow(brightness / 15.0, 1.5);
    const auto ui = [ui_scale](std::array<std::uint8_t, 3> rgb) {
        for (auto& channel : rgb)
            channel = static_cast<std::uint8_t>(std::lround(channel * ui_scale));
        return rgb;
    };
    const std::array<std::uint8_t, 3> ink = ui({255, 240, 220});
    const auto scroll = race_scroll(state, content, previous_update, history);
    const auto bg1_above_objects = draw_race_backgrounds(frame, vram, cgram, content, scroll);
    // R-0040: the countdown and winner windows show colour 0 through colour math. With history
    // the member is the one the vblank published for this picture; a single restored state
    // derives it from its counters and frame (exact when no pause intervened).
    const auto window_index =
        content.window_tables.empty() ? std::optional<unsigned>{}
        : history && history->window_published
            ? history->window_table
            : classic_window_table_index(state, scenario.initialization_frame + 6U,
                                         history ? history->opponent_finish_frame : std::nullopt,
                                         content.window_transition_member);
    const auto window_colour = colour(cgram, 0);
    // Picture N shows the objects of update N-1, like the scroll; without a previous update
    // the riders are drawn from this state, one update ahead. The caption (R-0042) and the HUD
    // (R-0043) are BG3, drawn over the track and under the riders; where no sprite covers the
    // ink it is the flat colour, which matches the original on every other measured frame.
    const auto& rider_source = previous_update ? *previous_update : state;
    std::bitset<256 * 224> caption_ink;
    draw_classic_caption(frame, rider_source, content, colour(cgram, 22), caption_ink);
    draw_classic_hud(
        frame, rider_source, content, history ? history->opponent_finish_frame : std::nullopt,
        history ? std::optional<ClassicHudPublished>(history->published_hud) : std::nullopt,
        colour(cgram, 22), caption_ink);
    draw_race_riders(frame, rider_source, content, history, cgram, scroll.flip, bg1_above_objects,
                     caption_ink);
    // Every member covers both objects as well as the backgrounds: inside the window the
    // original shows the flat window colour and nothing else (ZOOM-ZOO-WINDOW-EFFECTS:
    // start-line frames 1450, 1583 and 1649 of the M4-16 primary and countdown-pause originals
    // with a rider under the countdown sign and the GO letters, and the opponent-won banner
    // over the riding player on 6724-6800 of the countdown pause). R-0040's "0-6 before the
    // riders" came from DRAGSTER frames with no rider under those members; on its release-3213
    // race the opponent sits under the digits on 57 frames, all of which this order matches.
    if (window_index)
        render_window_xor(frame, dragster_window_table(content.window_tables, *window_index),
                          window_colour);
    if (state.pause.selection)
        draw_race_pause_menu(frame, state.pause.selection, ui({15, 30, 30}), ink);
    return frame;
}

void draw_race_pause_menu(RgbFrame& frame, std::uint16_t selection,
                          std::array<std::uint8_t, 3> panel, std::array<std::uint8_t, 3> ink) {
    for (auto& channel : frame.pixels) channel = static_cast<std::uint8_t>(channel / 2U);
    rect(frame, 55, 74, 146, 74, panel);
    ui_text(frame, 109, 83, "PAUSED", ink);
    ui_text(frame, 73, 101, selection == 1 ? "> RESUME" : "  RESUME", ink);
    ui_text(frame, 73, 115, selection == 0xffffU ? "> RESTART RACE" : "  RESTART RACE", ink);
    ui_text(frame, 68, 135, "UP DOWN - ENTER", ink);
}

PresentationContent dragster_presentation_content(const ClassicContentPack& pack) {
    return {pack.entry("physics.track.dragster.data"),
            pack.entry("presentation.track.dragster.bg1-tiles.v1"),
            pack.entry("presentation.track.dragster.bg2-tiles.v1"),
            pack.entry("presentation.track.dragster.bg2-map.v1"),
            pack.entry("presentation.classic.palette.v1"),
            pack.entry("presentation.classic.font.v1"),
            pack.entry("presentation.rider.mike.race-tiles.v1"),
            pack.entry("presentation.result.classic.font-layout.v1"),
            pack.entry("presentation.effect.go-window.v1"),
            pack.entry("presentation.effect.winner-window.v1"),
            pack.entry("presentation.result.classic.base-vram.v1"),
            pack.entry("presentation.result.classic.palette.v1"),
            pack.entry("presentation.result.classic.palette-tail.v1"),
            pack.optional_entry("presentation.zoom.race-palette-cycle.v1"),
            pack.optional_entry("presentation.effect.classic.window-tables.v1")};
}

} // namespace unirally
