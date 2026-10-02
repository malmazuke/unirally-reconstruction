#pragma once

#include "flat_contact.hpp"

namespace unirally {

// Vertical columns with either direct or mirrored descriptor geometry. The
// rider's pose reflection is already included in CollisionPoints. Descriptor
// bit 14 mirrors the terrain column independently (R-0024--R-0026).
struct VerticalContactSummary : FlatContactSummary {
    bool any_nonnegative_probe{}, boundary_marker{};
    std::uint8_t horizontal_penetration{}, horizontal_direction{};
    bool inverted_vertical{}; // selected vertical correction adds penetration
    bool leading_support{};   // $0F5D: winning nonnegative probe is one of first two
};

// `neon`: the race sets `$12D1` (NEON, R-0068), which an empty palette-7 probe's angle reads.
VerticalContactSummary summarize_vertical_contact(const FlatContactContent& content,
                                                  const CollisionPoints& points,
                                                  const TrackSamples& samples, std::uint16_t x,
                                                  std::uint16_t y, bool neon = false);

// Original signed slope response, using byte shift/multiplier tables at
// PAL $00:822B/$00:824B. Velocities are 16-bit patterns in 1/32 position units.
// This component rejects unrecovered landing transforms transactionally. Returns whether a
// long flight's flat landing sounded effect 23 ($81:942B-9447, `race_sound::landing_effect`).
bool resolve_vertical_contact(RiderContactState& rider, ContactMotion& motion,
                              const VerticalContactSummary& summary, const ContactContext& context,
                              std::span<const std::uint8_t> shifts,
                              std::span<const std::uint8_t> multipliers,
                              std::span<const std::uint8_t> landing_matrices = {},
                              unsigned horizontal = 1, unsigned pose_index = 0,
                              bool reflected = false);

} // namespace unirally
