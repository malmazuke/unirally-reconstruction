#include "presentation.hpp"

#include "announcements.hpp"
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

// The caption and HUD (BG3) ink is CGRAM colour 27 through the rider's colour math (race_ink):
// MIKE's (28,0,0) is also CGRAM 22's. Where a rider covers it the original adds colour 27's 13
// to the sprite's red (draw_race_riders, R-0042).
constexpr std::uint8_t bg3_ink_colour = 27;
constexpr unsigned ink_red_add = 13;
// $82:D57F-D607: channel 5's colour math table gives lines 0-110 the upper view's entry and the
// lines from 111 the lower view's.
constexpr int split_view_line = 111;
// OBJ palettes 3 and 4 in CGRAM bytes: the player's and the opponent's sprite colours.
constexpr std::size_t rider_palette_at = 352, opponent_palette_at = 384, sprite_palette_size = 32;
// $82:D4DC: four bytes a rider.
constexpr std::size_t colour_math_size = 4;
// $82:DC3D: NEON's scenery number.
constexpr unsigned neon_scenery = 14;

// A character's sprite palette: the front end's asset 6 + character, the bytes the race
// loader copies ($82:DD90-DDBC).
std::span<const std::uint8_t> character_palette(const ClassicContentPack& pack,
                                                unsigned character) {
    const auto asset = std::to_string(6U + character);
    return pack.entry("front-end.asset." + std::string(3U - asset.size(), '0') + asset);
}

} // namespace

std::uint16_t classic_race_ink(std::uint16_t ink, std::span<const std::uint8_t> colour_math) {
    if (colour_math.size() != colour_math_size)
        throw std::invalid_argument("the rider's colour math is missing (pack v21)");
    std::array<unsigned, 3> fixed{};
    for (std::size_t write = 0; write < 3; ++write)
        for (unsigned channel = 0; channel < 3; ++channel)
            if (colour_math[write] & (0x20U << channel)) fixed[channel] = colour_math[write] & 31U;
    const bool subtract = (colour_math[3] & 0x80U) != 0;
    unsigned word = 0;
    for (unsigned channel = 0; channel < 3; ++channel) {
        const auto base = (unsigned(ink) >> (5U * channel)) & 31U;
        const auto value = subtract ? (base > fixed[channel] ? base - fixed[channel] : 0U)
                                    : std::min(31U, base + fixed[channel]);
        word |= value << (5U * channel);
    }
    return static_cast<std::uint16_t>(word);
}

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
// index-th 0xFF-terminated lowercase string, shown in capitals with spaces
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

// R-0068: colours 96-111 as NOW PLAYING leaves them: its base palette's upper half from colour
// 64 ($80:A858, front-end.asset.036), and the cycle $80:FA60 at phase 0, colour 111 - k taking
// the cycle's word k.
std::array<std::uint8_t, 32> now_playing_leftover_colours(const ClassicContentPack& pack) {
    constexpr std::size_t base_from = 64, first_colour = 96, cycled_colours = 4;
    const auto base = pack.entry("front-end.asset.036");
    const auto cycle = pack.entry("front-end.cycle-colours");
    std::array<std::uint8_t, 32> colours{};
    const auto from = (first_colour - base_from) * 2U;
    if (base.size() < from + colours.size() - 2U * cycled_colours || cycle.size() < 8)
        throw std::invalid_argument("NOW PLAYING's palette is short");
    std::copy_n(base.begin() + static_cast<std::ptrdiff_t>(from),
                colours.size() - 2U * cycled_colours, colours.begin());
    for (std::size_t k = 0; k < cycled_colours; ++k) {
        colours[colours.size() - 2U * (k + 1U)] = cycle[2U * k];
        colours[colours.size() - 2U * (k + 1U) + 1U] = cycle[2U * k + 1U];
    }
    return colours;
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
    neon_green_.reset();
    on_screen_neon_green_.reset();
    pending_opponent_caption_ = latest_opponent_caption_ = on_screen_opponent_caption_ = 0;
}

void ClassicRaceHistoryTracker::observe_update(const ZoomZooState& previous,
                                               const ZoomZooState& updated,
                                               const ClassicContentPack& pack) {
    // R-0036: update N builds its objects with overlays chosen from the look
    // state before its own look step; picture N+1 shows them.
    on_screen_ = latest_;
    on_screen_opponent_caption_ = latest_opponent_caption_;
    latest_opponent_caption_ = pending_opponent_caption_;
    if (updated.split_screen) {
        const auto& before = previous.movement.rewards;
        const auto& after = updated.movement.rewards;
        if (after.read_cursor != before.read_cursor)
            pending_opponent_caption_ = after.entries[after.read_cursor];
        else if (before.cooldown <= 2 && after.cooldown == announcement::empty_queue_wait)
            pending_opponent_caption_ = 0;
    }
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
    // $83:CDB1 -> $83:CEC9 -> $83:D1CA, after the contact, in every update the pause menu does not
    // divert (R-0068).
    on_screen_neon_green_ = neon_green_;
    const auto scenario = classic_race_scenario(updated.track);
    if (scenario.neon_lighting && !zoom_zoo_update_was_paused(previous, updated))
        neon_green_ = neon_green_step(neon_green_.value_or(0), updated.player_contact_palette,
                                      pack.entry("presentation.neon.green-levels"));
    window_.observe_update(previous, updated, *transition_member_);
    clock_.observe_update(previous, updated);
    if (updated.result_updates || zoom_zoo_update_was_paused(previous, updated)) return;
    const auto tables = rider_look_tables(pack);
    const auto engine = classic_race_content(pack, updated.track);
    latest_.pose = rider_overlay_poses(look_, updated, tables);
    advance_rider_look(look_, updated, engine, tables);
}

void load_other_track_presentation_content(ClassicRacePresentationContent& content,
                                           const ClassicContentPack& pack,
                                           const ClassicRaceScenario& scenario) {
    const auto track = scenario.track;
    // TRACK-BREADTH part 3: the track's own BG1 tiles and its scenery's BG2
    // tiles, map and race palette (scenery = track mod 14, $82:DC20-DD84).
    // The result screen follows the race mode's accepted track (the
    // DRAGSTER assets for a one-run race, titled with the track's own
    // name); EAST's and FLAT FUN's results were compared with the original.
    // $82:DC22-DC40: NEON's scenery is 14, past the other thirteen (R-0068).
    const auto scenery = scenario.neon_lighting ? neon_scenery : track.index % 14U;
    const auto name =
        std::string("scenery.") + char('0' + scenery / 10U) + char('0' + scenery % 10U) + '.';
    content.track_name = classic_track_name(pack, track);
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
            (void)result_title_glyph(static_cast<char>(byte));
    }
    if (scenario.neon_lighting) {
        content.neon_green_levels = pack.entry("presentation.neon.green-levels");
        content.neon_menu_colours = now_playing_leftover_colours(pack);
    }
    content.geometry = track_geometry(content.track);
}

ClassicRacePresentationContent classic_race_presentation_content(const ClassicContentPack& pack,
                                                                 ClassicRaceTrack track) {
    return classic_race_presentation_content(pack, classic_race_scenario(track));
}

ClassicRacePresentationContent
classic_race_presentation_content(const ClassicContentPack& pack,
                                  const ClassicRaceScenario& scenario) {
    const auto track = scenario.track;
    ClassicRacePresentationContent content;
    content.scenario = scenario;
    content.riders = rider_object_content(pack);
    content.rider_palette = character_palette(pack, scenario.pairing.rider);
    content.opponent_palette = character_palette(pack, scenario.pairing.opponent);
    const auto colour_math = pack.entry("race.rider-colour-math");
    if (colour_math.size() < colour_math_size * rider_characters)
        throw std::invalid_argument("the riders' colour math table is short");
    content.rider_colour_math =
        colour_math.subspan(colour_math_size * scenario.pairing.rider, colour_math_size);
    content.opponent_colour_math =
        colour_math.subspan(colour_math_size * scenario.pairing.opponent, colour_math_size);
    // One ROM table serves both tracks (R-0037); its accepted entry name
    // predates DRAGSTER reading it and cannot be renamed.
    content.race_palette_cycle = pack.entry("presentation.zoom.race-palette-cycle.v1");
    // One ROM window family serves both tracks too (R-0040; ZOOM ZOO's
    // selection measured in ZOOM-ZOO-WINDOW-EFFECTS).
    content.window_tables = pack.entry("presentation.effect.classic.window-tables.v1");
    // R-0042: the caption table, likewise one table for both tracks.
    content.captions = pack.entry("presentation.classic.captions.v1");
    content.caption_font = pack.entry("presentation.classic.font.v1");
    content.pause_messages = pack.optional_entry("presentation.race.pause-messages.v1");
    content.rider_names = pack.entry("front-end.rider-names");
    content.track = classic_track_data(pack, track);
    content.window_transition_member = classic_window_transition_member(content.track);
    if (track != ClassicRaceTrack::ZoomZoo && track != ClassicRaceTrack::Dragster) {
        load_other_track_presentation_content(content, pack, scenario);
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

// The original's first visible result frame, counted in result updates (M4-16).
constexpr std::uint16_t authored_result_first_frame = 108;

// The authored tour result (M4-16), where no recovered result assets exist: black until the
// original's first visible result frame, then the lap graph fades in.
RgbFrame render_authored_result(const ZoomZooState& state,
                                const ClassicRacePresentationContent& content) {
    RgbFrame frame{};
    if (state.result_updates <= authored_result_first_frame) return frame;
    rect(frame, 0, 0, 256, 224, {34, 42, 48});
    ui_text(frame, 70, 12,
            state.race.total_times[0] < state.race.total_times[1] ? "WINNER" : "RUNNER UP");
    ui_text(frame, 12, 32, "PLAYER       TOTAL    BEST LAP");
    const unsigned minimum = state.result.graph_minimum, maximum = state.result.graph_maximum;
    for (unsigned i = 0; i < 2; ++i) {
        unsigned best = no_time;
        for (auto lap : state.race.lap_times[i])
            if (lap < no_time) {
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
            if (time >= no_time) continue;
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

// The BG3 ink at `brightness`: CGRAM 27 through the rider's colour math, then faded, as the
// PPU fades the colour math's result.
std::array<std::uint8_t, 3> race_ink_rgb(const ClassicRacePresentationContent& content,
                                         unsigned brightness, bool opponent = false) {
    auto ink = classic_race_ink(
        static_cast<std::uint16_t>(content.palette[2U * bg3_ink_colour]
                                   | (unsigned(content.palette[2U * bg3_ink_colour + 1U]) << 8U)),
        opponent ? content.opponent_colour_math : content.rider_colour_math);
    if (brightness < 15U) ink = apply_snes_brightness(ink, brightness);
    return colour_word_rgb(ink);
}

// The race's CGRAM at `brightness` (0-15). Colours 96-111 and 0 are cycled by the race NMI
// from ROM tables every frame (R-0037); neither track keys them to rider poses here.
// $82:DD90-DDBC: OBJ palettes 3 and 4 are the riders' (asset 6 + $77:0748 and + $77:0749); the
// scenery palettes carry MIKE's and BRONSEN's (R-0052, R-0061). The PPU scales each 5-bit channel
// before output conversion (bsnes lightTable: luma*c+0.5), so the fade applies to CGRAM
// words, as in the DRAGSTER result fade.
// R-0068: NEON's NMI writes colour 0 black and colour 113 its lighting ($80:87BB-87D4) and skips
// the palette cycle ($80:8816-881B), so colours 96-111 keep what the menus left.
void apply_neon_colours(std::array<std::uint8_t, 512>& cgram,
                        const ClassicRacePresentationContent& content, std::uint8_t green) {
    constexpr std::size_t menu_colours_at = 96U * 2U, lit_colour_at = 113U * 2U;
    std::copy(content.neon_menu_colours.begin(), content.neon_menu_colours.end(),
              cgram.begin() + static_cast<std::ptrdiff_t>(menu_colours_at));
    cgram[0] = cgram[1] = 0;
    const auto lit = neon_colour(green);
    cgram[lit_colour_at] = static_cast<std::uint8_t>(lit);
    cgram[lit_colour_at + 1U] = static_cast<std::uint8_t>(lit >> 8U);
}

auto race_cgram(const ZoomZooState& state, const ClassicRacePresentationContent& content,
                unsigned brightness, std::optional<std::uint8_t> neon_green) {
    auto cgram = build_race_cgram(content.palette, false);
    for (const auto& [palette, at] : {std::pair{content.rider_palette, rider_palette_at},
                                      std::pair{content.opponent_palette, opponent_palette_at}})
        if (palette.size() == sprite_palette_size)
            std::copy(palette.begin(), palette.end(),
                      cgram.begin() + static_cast<std::ptrdiff_t>(at));
    if (neon_green)
        apply_neon_colours(cgram, content, *neon_green);
    else
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

// The race's colours at full brightness (`lit`, where the colour math adds) and as the fade
// shows them (`shown`, at `brightness` 0-15).
struct RaceColours {
    std::array<std::uint8_t, 512> lit, shown;
    unsigned brightness{};
};

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
                       const ZoomZooState* previous_update, const ClassicRaceHistory* history,
                       bool second_camera = false) {
    const auto& geometry = content.geometry;
    const auto track = content.track;
    const bool previous_is_prior =
        previous_update && previous_update->movement.frame <= state.movement.frame;
    const auto& camera = second_camera ? state.race.second_camera : state.race.camera;
    const auto& prior = previous_update ? (second_camera ? previous_update->race.second_camera
                                                         : previous_update->race.camera)
                                        : camera;
    const int camera_x = camera.x, camera_y = static_cast<std::int16_t>(camera.y);
    RaceScroll scroll;
    scroll.background_x =
        previous_is_prior
            ? prior.x & geometry.position_mask
            : (camera_x - static_cast<std::int16_t>(camera.velocity_x)) & geometry.position_mask;
    scroll.background_y = previous_is_prior
                            ? static_cast<std::int16_t>(prior.y)
                            : static_cast<std::int16_t>(static_cast<std::uint16_t>(
                                  camera_y - static_cast<std::int16_t>(camera.velocity_y)));
    const auto origin_x = static_cast<std::uint16_t>(
        ((unsigned(word(track, second_camera ? 7 : 3)) << 4) - 256U) & 0xfff0U);
    const auto origin_y = static_cast<std::uint16_t>(
        ((unsigned(word(track, second_camera ? 9 : 5)) << 4) - 256U) & 0xfff0U);
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

// BG2's colour index at a screen pixel (0 where it is transparent).
unsigned race_bg2_pixel(const std::array<std::uint8_t, 65536>& vram, const RaceScroll& scroll,
                        int x, int y) {
    return background_pixel(vram, 0xe000, true, true, 0x2000, false,
                            static_cast<std::int16_t>(scroll.bg_x),
                            static_cast<std::int16_t>(scroll.bg_y), x, y);
}

// BG2, then BG1 over it. $81:A304-A51B: the playfield is 16,384 coarse cells of 64 units in
// the track's column count (256 by 64 for ZOOM ZOO, 1,024 by 16 for DRAGSTER). BG1 map
// entries with bit 13 set are drawn above priority-2 OBJs; returns those pixels.
std::array<bool, 256 * 224> draw_race_backgrounds(RgbFrame& frame,
                                                  const std::array<std::uint8_t, 65536>& vram,
                                                  const std::array<std::uint8_t, 512>& cgram,
                                                  const ClassicRacePresentationContent& content,
                                                  const RaceScroll& top_scroll,
                                                  const RaceScroll* bottom_scroll = nullptr) {
    const auto track = content.track;
    const auto& geometry = content.geometry;
    const int columns = geometry.coarse_columns;
    const int world_height = (16384 / columns) * 64;
    std::array<bool, 256 * 224> bg1_above_objects{};
    // R-0068: NEON's main screen has no BG2 ($80:87B6: BG1, BG3 and the objects), so the
    // backdrop, colour 0, shows behind BG1.
    const bool bg2_on_main = !content.scenario.neon_lighting;
    for (int y = 0; y < 224; ++y)
        for (int x = 0; x < 256; ++x) {
            const auto& scroll = bottom_scroll && y >= 112 ? *bottom_scroll : top_scroll;
            const int viewport_y = bottom_scroll && y >= 112 ? y - 112 : y;
            // A mosaic block repeats its top-left pixel.
            const int mx = x - x % scroll.mosaic, my = viewport_y - viewport_y % scroll.mosaic;
            // The split HDMA changes BG2 scroll at scanline 112, but PPU tile fetches
            // still count scanlines from the top of the full picture.
            const auto background =
                bg2_on_main ? race_bg2_pixel(vram, scroll, mx, bottom_scroll && y >= 112 ? y : my)
                            : 0U;
            pixel(frame, x, y, colour(cgram, static_cast<std::uint8_t>(background)));
            if (scroll.hide_track) continue;
            // Screen row 0 is scanline 1, as in background_pixel's vertical +1. The BG1 map
            // wraps horizontally: the original's map fetch masks the column with `$0D51`
            // ($81:AD05), so past the playfield's right edge the picture continues from column
            // 0, as the sampler's contact does (TRACK-BREADTH, LOOPER). Rows off the playfield
            // are left blank, which is not yet checked against the original's row test at
            // $81:AD22-AD2C. The 65,536-unit playfield (`$0FF7`, track 37) skips that test
            // ($81:AD1D-AD20, $81:AD71-AD74) and its rows take y's sixteen bits (R-0068).
            const int world_x = (scroll.background_x + mx) & geometry.position_mask;
            int world_y =
                scroll.flip ? scroll.background_y + 224 - my : scroll.background_y + my + 1;
            if (geometry.whole_height) world_y &= 0xffff;
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

// The colour math a rider's pixels take: a race's channel 5 adds red where a rider covers the
// BG3 ink (`caption_ink`); NEON's colour math subtracts the sub screen, BG2 (`neon_vram`, at the
// race's BG2 scroll), from the objects of palettes 4-7 ($80:87AC-87B8, R-0068).
struct RaceObjectMath {
    const std::bitset<256 * 224>& caption_ink;
    const std::array<std::uint8_t, 65536>* neon_vram{};
    const RaceScroll* scroll{};
    // $82:D57F-D607: channel 5 sets each view's CGADSUB from its rider's `$82:D4DC` entry; bit 7
    // subtracts the object from the ink instead of adding it (TONY's, R-0080). A one-view race
    // has only the upper view.
    bool upper_subtracts{}, lower_subtracts{}, split{};
};

// Bit 7 of a rider's CGADSUB byte, the fourth of its `$82:D4DC` entry: subtract.
bool colour_math_subtracts(std::span<const std::uint8_t> entry) {
    constexpr std::size_t cgadsub = 3;
    return entry.size() > cgadsub && (entry[cgadsub] & 0x80U) != 0;
}

std::array<std::uint8_t, 3> rider_pixel(const RaceColours& colours, const RaceObjectMath& math,
                                        std::uint8_t index, int x, int y) {
    const auto at = static_cast<std::size_t>(y) * 256 + static_cast<std::size_t>(x);
    std::uint16_t word{};
    if (math.neon_vram) {
        // The sub screen's backdrop, where BG2 is transparent, is the fixed colour, black.
        const auto below = race_bg2_pixel(*math.neon_vram, *math.scroll, x, y);
        const auto sub =
            below ? colour_word(colours.lit, static_cast<std::uint8_t>(below)) : std::uint16_t{};
        word = subtract_colour(colour_word(colours.lit, index), sub);
    } else if (math.caption_ink.test(at)) {
        // The ink (CGRAM 27, red 13) is in front; the colour math adds or subtracts the object
        // behind it ($2130 = 2, the sub screen the objects).
        const auto object = colour_word(colours.lit, index);
        const bool subtracts =
            math.split && y >= split_view_line ? math.lower_subtracts : math.upper_subtracts;
        word = subtracts ? subtract_colour(std::uint16_t{ink_red_add}, object)
                         : static_cast<std::uint16_t>(
                               (object & ~31U)
                               | std::min<unsigned>(31U, (object & 31U) + ink_red_add));
    } else {
        return colour(colours.shown, index);
    }
    return colour_word_rgb(
        colours.brightness < 15U ? apply_snes_brightness(word, colours.brightness) : word);
}

// R-0036: the riders, opponent first, from the OBJ tiles and OAM the original published in
// the previous update. Entry 98 (player, tile base 0, palette 3) has priority over entry 99
// (opponent, base 0x88, palette 4); both use OBJ priority 2 outside a corkscrew. Where a rider
// covers caption or HUD ink (R-0042) the original adds red to the sprite, red = min(31,
// sprite_red + 13), green and blue untouched, measured over 35 such pixels on frame 2100 of
// the M4-16 primary and the same on 2120, 2340 and 2600. The 13 is the red of CGRAM 27, the
// colour the caption's attribute names, on both race palettes; which PPU configuration adds
// it is not recovered, so the measured value is kept. The fade (INIDISP) scales the sum, as it
// scales every colour after the colour math: in the fade-in the sum is as dark as the rest
// (bowl-lose 1340-1341, a stunt event's score field under the rider, R-0068).
void draw_race_riders(RgbFrame& frame, const ZoomZooState& rider_source,
                      const ClassicRacePresentationContent& content,
                      const ClassicRaceHistory* history, const RaceColours& colours, bool flip,
                      const std::array<bool, 256 * 224>& bg1_above_objects,
                      const RaceObjectMath& math) {
    const unsigned viewports = rider_source.split_screen ? 2U : 1U;
    for (unsigned viewport = 0; viewport < viewports; ++viewport)
        for (int order = 0; order < 2; ++order) {
            // Each half gives its followed rider the front OAM slot. On frame
            // 1700 both riders overlap in the lower half; ALICE covers AMY there.
            const int rider = viewport ? order : 1 - order;
            // R-0068: a stunt event's setup sets entry 99's ninth x bit (`$15A3` = 0xE5,
            // $82:D789-D797) and the opponent's object writer, part of its skipped update, never
            // clears it, so the opponent stays at the setup's x 0x60 - 256, off the screen.
            if (rider == 1 && content.scenario.stunt_event && !rider_source.split_screen) continue;
            const auto& source = rider_source.movement.riders[static_cast<std::size_t>(rider)];
            const auto& camera =
                viewport ? rider_source.race.second_camera : rider_source.race.camera;
            if (rider_source.split_screen) {
                const auto dy = static_cast<std::int16_t>(source.motion.y - camera.y);
                if (dy < -41 || dy >= 112) continue;
            }
            auto oam = project_rider_oam(source.motion.x, source.motion.y, camera.x, camera.y,
                                         source.pose.reflected, content.geometry);
            if (viewport) oam.y = static_cast<std::uint8_t>(oam.y + 112U);
            if (flip)
                oam.y = static_cast<std::uint8_t>(0xe0U - static_cast<std::uint8_t>(oam.y + 0x40U));
            oam.vertical_flip = flip || (history && history->hunter_flip_prior);
            if (!oam.visible) continue;
            const auto overlay =
                history ? history->overlays.pose[static_cast<std::size_t>(rider)] : std::nullopt;
            const auto pixels =
                compose_rider_object(content.riders, source.pose.pose_index, overlay, oam.clip);
            // NEON's update gives the player's object palette 7 ($83:D1DC-D1E3, R-0068).
            const unsigned palette = rider ? 4U : content.scenario.neon_lighting ? 7U : 3U;
            const unsigned object_palette = 128U + palette * 16U;
            // R-0047: through the corkscrew the object's priority is toggled to 3
            // ($1516/$151A bit 4), above every BG1 tile.
            const bool raised =
                rider_source.special_tiles[static_cast<std::size_t>(rider)].raised_priority != 0;
            draw_rider_object(pixels, oam, [&](int x, int y, std::uint8_t value) {
                if (rider_source.split_screen && (static_cast<unsigned>(y >= 112) != viewport))
                    return;
                const auto at = static_cast<std::size_t>(y) * 256 + static_cast<std::size_t>(x);
                if (bg1_above_objects[at] && !raised) return;
                // NEON: BG3's ink (priority tiles) is in front of the objects, which a race's
                // colour math shows through (rider_pixel); NEON's leaves BG3 out (R-0068).
                if (math.neon_vram && math.caption_ink.test(at)) return;
                const auto index = static_cast<std::uint8_t>(object_palette + value);
                pixel(frame, x, y, rider_pixel(colours, math, index, x, y));
            });
        }
}

// NEON's green level for the picture of `drawn` (R-0068): the history's, from the setup's 0;
// a single state has none, so it takes the level the lighting settles at for the palette under
// the player. Nothing on another track.
std::optional<std::uint8_t> neon_green_on_screen(const ZoomZooState& drawn,
                                                 const ClassicRacePresentationContent& content,
                                                 const ClassicRaceHistory* history) {
    if (!content.scenario.neon_lighting) return std::nullopt;
    if (history) return history->neon_green.value_or(0);
    if (drawn.player_contact_palette >= content.neon_green_levels.size())
        throw std::invalid_argument("NEON level table is short");
    return content.neon_green_levels[drawn.player_contact_palette];
}

// R-0040: the countdown and winner windows show colour 0 through colour math. With history
// the member is the one the vblank published for this picture; a single restored state
// derives it from its counters and frame (exact when no pause intervened).
std::optional<unsigned> race_window_member(const ZoomZooState& state,
                                           const ClassicRacePresentationContent& content,
                                           const ClassicRaceHistory* history) {
    if (content.window_tables.empty()) return std::nullopt;
    if (history && history->window_published) return history->window_table;
    return classic_window_table_index(state, content.scenario.initialization_frame + 6U,
                                      history ? history->opponent_finish_frame : std::nullopt,
                                      content.window_transition_member);
}

// Recover the five-bit channel before the same SNES brightness operation used by CGRAM.
// channel8 includes the reference core's display gamma, so a linear inverse is incorrect.
std::uint8_t five_bit_channel(std::uint8_t value) {
    static const auto inverse = [] {
        std::array<std::uint8_t, 256> table{};
        for (unsigned byte = 0; byte < table.size(); ++byte) {
            int distance = 256;
            for (unsigned level = 0; level < 32; ++level) {
                const auto candidate =
                    std::abs(static_cast<int>(channel8(static_cast<std::uint16_t>(level)))
                             - static_cast<int>(byte));
                if (candidate < distance) {
                    distance = candidate;
                    table[byte] = static_cast<std::uint8_t>(level);
                }
            }
        }
        return table;
    }();
    return inverse[value];
}

void dim_finished_views(RgbFrame& frame, const ZoomZooState& updated,
                        const ZoomZooState* previous_update, const ClassicRaceScenario& scenario) {
    // $83:E8E0/EA72 publish brightness 7 separately for a finished split view, in every split race
    // (R-0080), from the update the picture's objects come from. R-0073 neutral STUNT: both bytes
    // are 7 at original frame 21700. $83:F695-F6A4 and $83:F947-F950 write both views' bytes: no
    // view is dimmed again while the menu is open, nor on the picture that closes it (R-0079).
    const auto& state = previous_update ? *previous_update : updated;
    if (!state.split_screen || updated.pause.selection
        || (previous_update && classic_pause_menu_closed(*previous_update, updated)))
        return;
    for (unsigned view = 0; view < 2; ++view) {
        if (!state.race.riders[view].finished
            || (scenario.stunt_event && !state.stunt.finish_display))
            continue;
        for (unsigned y = view * 112; y < (view + 1) * 112; ++y)
            for (unsigned x = 0; x < 256; ++x) {
                const auto at = (y * 256 + x) * 3;
                std::uint16_t word = 0;
                for (unsigned channel = 0; channel < 3; ++channel)
                    word = static_cast<std::uint16_t>(
                        word | (five_bit_channel(frame.pixels[at + channel]) << (channel * 5)));
                const auto rgb = colour_word_rgb(apply_snes_brightness(word, 7));
                for (unsigned channel = 0; channel < 3; ++channel)
                    frame.pixels[at + channel] = rgb[channel];
            }
    }
}

// $83:F964: INIDISP while the race is paused; $83:F947 when the menu closes.
constexpr unsigned paused_brightness = 7, full_brightness = 15;

// Puts an earlier picture's pixels back in BG3 cells another writer holds, and clears their ink.
void restore_bg3_cells(RgbFrame& frame, const RgbFrame& backgrounds, const Bg3Cells& cells,
                       std::bitset<256 * 224>& inked) {
    // A tilemap row r shows on lines 8r - 1 to 8r + 6 (R-0042).
    for (int y = 0; y < 224; ++y)
        for (int x = 0; x < 256; ++x) {
            const auto row = static_cast<unsigned>(y + 1) / 8U,
                       column = static_cast<unsigned>(x) / 8U;
            if (row >= 28 || !cells.test(row * 32U + column)) continue;
            const auto at = static_cast<std::size_t>(y) * 256 + static_cast<std::size_t>(x);
            for (std::size_t channel = 0; channel < 3; ++channel)
                frame.pixels[at * 3 + channel] = backgrounds.pixels[at * 3 + channel];
            inked.reset(at);
        }
}

// The picture's INIDISP brightness. NMI $80:883F-8849 writes it from the preceding update's
// $0FF1, clamping (fade-15) at zero; $83:CCC1-CCC9 increments $0FF1 once per race update. Scaling
// converted pixels made mid-fade frames too bright: green 15 at brightness 8 is 47 in the
// original, not 64. $83:F695-F69C and $83:F962-F979: a paused update writes 7 after the NMI's, so
// the race, both views of a split one, shows at brightness 7 while its menu is open, and the
// update that closes it writes 15 ($83:F945-F956), even during the fade-in (R-0078, R-0079).
unsigned race_picture_brightness(const ZoomZooState& state, const ZoomZooState* previous_update,
                                 const ClassicRaceScenario& scenario) {
    if (state.pause.selection != 0) return paused_brightness;
    if (previous_update && classic_pause_menu_closed(*previous_update, state))
        return full_brightness;
    const unsigned prior_fade = classic_race_prior_fade(state, previous_update, scenario);
    return prior_fade > 15U ? prior_fade - 15U : 0U;
}

// The original's pause menu (R-0078), or a league race's message: in the view of the pad that
// paused, in that view's ink (R-0079).
void draw_pause_menu(RgbFrame& frame, const ZoomZooState& state,
                     const ClassicRacePresentationContent& content,
                     std::array<std::uint8_t, 3> upper_ink, std::array<std::uint8_t, 3> lower_ink,
                     std::bitset<256 * 224>& caption_ink) {
    using Choice = ClassicRacePresentationContent::PauseSecondChoice;
    const bool lower = state.pause.lower_view;
    const auto ink = lower ? lower_ink : upper_ink;
    if (classic_pause_shows_message(state)) {
        // $83:F6D3/F6F3: the pauser's rider, pad 1's or pad 2's.
        const auto rider = lower ? state.pairing.opponent : state.pairing.rider;
        draw_classic_pause_message(frame, content, rider, lower, ink, caption_ink);
        return;
    }
    draw_classic_pause_menu(frame, content, state.pause.selection,
                            content.pause_second_choice == Choice::quit ? "quit" : "restart", lower,
                            ink, caption_ink);
}

} // namespace

std::optional<RgbFrame> visible_race_result(const ZoomZooState& state,
                                            const ClassicRacePresentationContent& content) {
    if (!state.result_updates) return std::nullopt;
    if (content.result_base_vram.empty()) return render_authored_result(state, content);
    // The original publishes the result at the end of its loading sequence.
    // Presentation reads that counter without changing the gameplay transition.
    const auto finish = classic_finish_view(state);
    if (!result_screen_visible(finish)) return std::nullopt;
    RgbFrame frame{};
    render_result_background(frame, finish, state.movement.timer,
                             {content.palette, content.result_assets, content.result_base_vram,
                              content.result_palette, content.result_palette_tail,
                              content.result_track_name});
    return frame;
}

// One race picture, in the PPU's layers: the recovered or authored result once it shows;
// else BG2 and BG1, the caption and HUD (BG3), the riders, the window members over all of
// them, and the pause menu.
RgbFrame render_classic_race(const ZoomZooState& state,
                             const ClassicRacePresentationContent& content,
                             const ZoomZooState* previous_update,
                             const ClassicRaceHistory* history) {
    const auto& scenario = content.scenario;
    if (const auto result = visible_race_result(state, content)) return *result;
    RgbFrame frame{};
    const auto vram = race_vram(content);
    const bool paused = state.pause.selection != 0;
    const auto brightness = race_picture_brightness(state, previous_update, scenario);
    // Picture N shows the objects of update N-1, like the scroll; without a previous update
    // the riders are drawn from this state, one update ahead.
    const auto& rider_source = previous_update ? *previous_update : state;
    const auto neon_green = neon_green_on_screen(rider_source, content, history);
    const RaceColours colours{race_cgram(state, content, 15U, neon_green),
                              race_cgram(state, content, brightness, neon_green), brightness};
    const auto& cgram = colours.shown;
    // NEON's NMI turns channel 5, the rider's colour math on the ink, off ($80:87A7) and its
    // colour math leaves BG3 out: the ink is colour 27 itself (R-0068).
    const auto bg3_ink =
        neon_green ? colour(cgram, bg3_ink_colour) : race_ink_rgb(content, brightness);
    const auto scroll = race_scroll(state, content, previous_update, history);
    const auto second_scroll =
        state.split_screen
            ? std::optional<RaceScroll>(race_scroll(state, content, previous_update, history, true))
            : std::nullopt;
    const auto bg1_above_objects = draw_race_backgrounds(frame, vram, cgram, content, scroll,
                                                         second_scroll ? &*second_scroll : nullptr);
    const auto window_index = race_window_member(state, content, history);
    const auto window_colour = colour(cgram, 0);
    // The caption (R-0042) and the HUD (R-0043) are BG3, drawn over the track and under the
    // riders; where no sprite covers the ink it is the flat colour, which matches the original
    // on every other measured frame.
    std::bitset<256 * 224> caption_ink;
    const auto hud =
        history ? std::optional<ClassicHudPublished>(history->published_hud) : std::nullopt;
    // The menu's words replace the HUD's in the cells it writes: keep the backgrounds there.
    const auto backgrounds = paused ? std::optional<RgbFrame>(frame) : std::nullopt;
    const auto menu_cells = classic_pause_shows_message(state)
                              ? classic_pause_message_cells(state.pause.lower_view)
                              : classic_pause_menu_cells(state.pause.lower_view);
    const auto lower_ink = race_ink_rgb(content, brightness, true);
    draw_classic_caption(frame, rider_source, content, hud, bg3_ink, caption_ink);
    const auto draw_hud = [&] {
        draw_classic_hud(frame, rider_source, content,
                         history ? history->opponent_finish_frame : std::nullopt, hud, bg3_ink,
                         lower_ink, history ? history->opponent_caption_event : 0U, caption_ink);
    };
    if (!state.split_screen) draw_hud();
    // The menu's words go in the HUD's layer, under the riders and the window members: on the
    // picture that opens the menu during the countdown the digit's window covers them (R-0078),
    // and a split race's riders cover them (R-0079).
    if (backgrounds) {
        restore_bg3_cells(frame, *backgrounds, menu_cells, caption_ink);
        draw_pause_menu(frame, state, content, bg3_ink, lower_ink, caption_ink);
    }
    const RaceObjectMath math{caption_ink,
                              neon_green ? &vram : nullptr,
                              &scroll,
                              colour_math_subtracts(content.rider_colour_math),
                              colour_math_subtracts(content.opponent_colour_math),
                              state.split_screen};
    draw_race_riders(frame, rider_source, content, history, colours, scroll.flip, bg1_above_objects,
                     math);
    // The split's HUD tile priority covers both riders at the lap-banner overlap; the menu's
    // cells keep the menu.
    if (state.split_screen) {
        const auto under_hud = paused ? std::optional<RgbFrame>(frame) : std::nullopt;
        draw_hud();
        if (under_hud) restore_bg3_cells(frame, *under_hud, menu_cells, caption_ink);
    }
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
    if (state.split_screen)
        for (int y = 111; y <= 112; ++y)
            for (int x = 0; x < 256; ++x) pixel(frame, x, y, {0, 0, 0});
    dim_finished_views(frame, state, previous_update, scenario);
    return frame;
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
