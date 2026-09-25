// The HUNTER tour's tag and its eight timed effects (R-0052).

#include "hunter_effects.hpp"

#include "reward_queue.hpp"
#include "word_arithmetic.hpp"
#include "zoom_zoo_movement.hpp"

#include <array>
#include <cstdint>
#include <span>
#include <stdexcept>

namespace unirally {

// $83:CEC9-D600 (R-0052): the HUNTER tour's tag effects, run at the end of
// every update ($83:CDAA), skipped ones included. `$12D1` (a palette mode that
// replaces them) is zero on every HUNTER race.
void update_hunter_effects(ZoomZooState& state, std::span<const std::uint8_t> blink) {
    if (!classic_race_scenario(state.track).hunter_tour) return;
    if (blink.size() != 64) throw std::invalid_argument("HUNTER blink pattern is missing");
    auto& h = state.hunter;
    const auto n = [](std::uint16_t a, std::uint16_t b) {
        return negative(static_cast<std::uint16_t>(a - b));
    };
    // $83:D104-D1C2: the tag. The riders' boxes are x+8 to x+40 and y to y+40;
    // every comparison is an N-flag one.
    const auto& riders = state.movement.riders;
    if (!h.active && !state.race.riders[0].finished && !state.race.riders[1].finished) {
        if (!h.latched) {
            const auto d = static_cast<std::uint16_t>(riders[0].progress.transition_count
                                                      - riders[1].progress.transition_count);
            if (!n(d, 2) || n(d, 0xffffU)) h.latched = 1;
        }
        if (h.latched) {
            const auto px0 = static_cast<std::uint16_t>(riders[0].motion.x + 8U),
                       px1 = static_cast<std::uint16_t>(px0 + 0x20U);
            const auto py0 = riders[0].motion.y, py1 = static_cast<std::uint16_t>(py0 + 0x28U);
            const auto ox0 = static_cast<std::uint16_t>(riders[1].motion.x + 8U),
                       ox1 = static_cast<std::uint16_t>(ox0 + 0x20U);
            const auto oy0 = riders[1].motion.y, oy1 = static_cast<std::uint16_t>(oy0 + 0x28U);
            const bool x_hit =
                ox0 == px0 || (!n(ox0, px0) && n(ox0, px1)) || (!n(ox1, px0) && n(ox1, px1));
            const bool y_hit = (!n(oy0, py0) && n(oy0, py1)) || (!n(oy1, py0) && n(oy1, py1));
            if (x_hit && y_hit) {
                h.effect[riders[0].motion.x & 7U] = 1;
                h.active = 1;
            }
        }
    }
    // $83:CED9-D100: the first flag set, in this order, runs; on the update it
    // is picked it is announced ($81:C55B), and effects other than 1 name
    // their HUD message ($12AF). Sound $021F is not played.
    static constexpr std::array<unsigned, 8> order{3, 1, 5, 2, 0, 4, 6, 7};
    static constexpr std::array<std::uint8_t, 8> event{0x1f, 0x1c, 0x1e, 0x1b,
                                                       0x20, 0x1d, 0x21, 0x22};
    const auto finish = [&](unsigned k) {
        // The effect's end: event $23 and its message, and the HUD's message
        // buffers reset ($83:D275).
        h.message = 0x23;
        push_front_zoom_player(state, 0x23);
        h.hud_event = 0;
        h.effect[k] = 0;
        h.active = 0;
    };
    // The 500-update timer of effects 0, 2, 3, 4, 5, 6 and 7.
    const auto count = [&](unsigned k) {
        auto t = h.timer[k];
        if (!t) t = 500;
        h.timer[k] = --t;
        return t;
    };
    for (const auto k : order) {
        if (!h.effect[k]) continue;
        if (h.effect[k] == 1) {
            h.effect[k] = 2;
            push_front_zoom_player(state, event[k]);
            if (k != 1) h.message = event[k];
        }
        for (unsigned other = 0; other < 8; ++other)
            if (other != k) h.effect[other] = 0;
        switch (k) {
        case 1:
            // $83:D530-D580 (bytes): freezes of 1, 2 ... 20 skipped updates,
            // then 18, 16 ... down to none, which ends the effect unannounced.
            if (h.pulse) {
                h.pulse = static_cast<std::uint16_t>(h.pulse - 1U);
                h.skip_update = 1;
            } else if (h.pulse_shrinking) {
                h.pulse = h.pulse_length =
                    static_cast<std::uint16_t>((h.pulse_length - 2U) & 0xffU);
                if (!h.pulse_length) {
                    h.pulse_shrinking = 0;
                    h.effect[1] = 0;
                    h.active = 0;
                }
            } else {
                h.pulse = h.pulse_length =
                    static_cast<std::uint16_t>((h.pulse_length + 1U) & 0xffU);
                if (h.pulse_length == 20) h.pulse_shrinking = 1;
            }
            break;
        case 3: {
            // $83:D2C9-D348: a blink over the first and last 50 updates, the
            // picture on otherwise; each update it is on alternates its table.
            h.blink = 0;
            const auto t = count(3);
            if (!t) finish(3);
            if (!n(t, 0x1c2)) {
                if (blink[t - 0x1c2U]) break;
            } else if (n(t, 0x32) && !blink[t])
                break;
            h.blink = 1;
            h.wave_phase = static_cast<std::uint16_t>(1U - h.wave_phase);
            // $83:D581-E081: either phase's scroll table ends at $83:E04F,
            // which turns the riders' OAM entries upside down: y becomes
            // $E0 - (y + $40) and the vertical-flip bit is set. The player's
            // entry is the camera's published screen position.
            {
                auto& screen = state.race.camera.screen_xy;
                const auto y = static_cast<std::uint8_t>(screen >> 8U);
                const auto flipped =
                    static_cast<std::uint8_t>(0xe0U - static_cast<std::uint8_t>(y + 0x40U));
                screen = static_cast<std::uint16_t>((unsigned(flipped) << 8U) | (screen & 0xffU));
            }
            break;
        }
        case 4:
        case 6: {
            // $83:D3FC-D473 (4, blinking over its first 50 and last 60
            // updates) and $83:D349-D3BB (6, its first and last 50).
            auto& on = k == 4 ? h.hide_track : h.mosaic;
            on = 1;
            const auto t = count(k);
            if (!t) {
                finish(k);
                on = 0;
                break;
            }
            if (!n(t, 0x1c2)) {
                if (!blink[t - 0x1c2U]) on = 0;
            } else if (n(t, k == 4 ? 0x3cU : 0x32U) && blink[t])
                on = 0;
            break;
        }
        case 5:
            // $83:D4E8-D52F: slow motion, three updates in four skipped.
            if (!count(5)) finish(5);
            if ((h.timer[5] & 3U) != 3U) h.skip_update = 1;
            break;
        default:
            // $83:D474 (0), $83:D4AE (2), $83:D28A (7): the timer alone.
            if (!count(k)) finish(k);
            break;
        }
        break;
    }
}

// $80:8821-882A: the race NMI that opens each update counts $0563 while the
// effect 6 mosaic the previous update left is on (R-0052).
void count_hunter_mosaic(ZoomZooState& state) {
    if (state.hunter.mosaic)
        state.hunter.mosaic_counter =
            static_cast<std::uint16_t>((state.hunter.mosaic_counter + 1U) & 0xffU);
}

} // namespace unirally
