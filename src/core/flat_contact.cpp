#include "flat_contact.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>

namespace unirally {
namespace {

void require(bool condition, const char* explanation) {
    if (!condition) throw std::invalid_argument(explanation);
}

std::uint8_t content_byte(std::span<const std::uint8_t> bytes, unsigned index) {
    if (index >= bytes.size()) throw std::out_of_range("flat contact content is unavailable");
    return bytes[index];
}

unsigned tile_index(std::uint16_t descriptor) {
    // $81:8BF4–8C02 and $81:9163–9176. The low bit is a separate flag.
    return ((descriptor & 0x03F0U) >> 2) + ((descriptor & 0x000FU) >> 1);
}

bool negative_difference(std::uint8_t left, std::uint8_t right) {
    return ((static_cast<unsigned>(left) - right) & 0x80U) != 0;
}

struct Probe {
    std::uint8_t penetration{0xA0};
    std::uint8_t angle{};
    std::uint16_t descriptor{};
};

Probe preprocess(const FlatContactContent& content, SamplePoint point, std::uint16_t descriptor,
                 std::uint16_t x, std::uint16_t y) {
    // Marker-only words supply no collision descriptor. Marker progression
    // is observed separately from the raw TrackSamples by track_progress.
    if ((descriptor & 0x03FFU) == 0) return {};
    require((descriptor & 0xC001U) == 0, "unrecovered terrain direction or special descriptor");
    const auto tile = tile_index(descriptor);
    require((content_byte(content.flags, tile) & 1U) == 0,
            "unrecovered horizontal collision column");
    const unsigned column = (static_cast<unsigned>(point.x) + (x & 15U)) & 15U;
    const unsigned local_y = (static_cast<unsigned>(point.y) + (y & 15U)) & 15U;
    const auto height = content_byte(content.columns, tile * 32U + column * 2U);
    const auto angle = content_byte(content.columns, tile * 32U + column * 2U + 1U);
    require(angle == 0, "unrecovered non-flat collision angle");
    if (height == 0xA0U) return {0xA0, angle, descriptor};
    // DEC and SBC both use eight-bit arithmetic ($81:8CC8–8CD3).
    const auto previous_height = static_cast<std::uint8_t>(static_cast<unsigned>(height) - 1U);
    const auto penetration = static_cast<std::uint8_t>(local_y - previous_height);
    require(penetration < 0x80U, "unrecovered negative supported penetration");
    return {penetration, angle, descriptor};
}

unsigned coarse_landing_angle(unsigned dx, unsigned half_dy) {
    // $81:982C–98A8: repeated subtraction, with the terminal angle not stored.
    // Both differences must be nonnegative in the recovered quadrant. Guarded
    // endpoints bound each loop even when one displacement is zero.
    unsigned angle = 16;
    if (dx >= half_dy) {
        while (dx >= half_dy) {
            dx -= half_dy;
            const unsigned next = angle - 4U;
            if (next == 0) break;
            angle = next;
        }
    } else {
        while (half_dy >= dx) {
            half_dy -= dx;
            const unsigned next = angle + 4U;
            if (next == 36) break;
            angle = next;
        }
        if (angle == 32) angle = 31;
    }
    return angle;
}

void resolve_recontact(RiderContactState& rider, ContactMotion& motion,
                       const FlatContactSummary& summary, const ContactContext& context) {
    const auto dx = static_cast<std::uint16_t>(motion.x - rider.previous_uncorrected_x);
    const auto dy = static_cast<std::uint16_t>(motion.y - rider.previous_uncorrected_y);
    require(dx < 0x8000U && dy < 0x8000U, "unrecovered landing displacement quadrant");
    const unsigned angle = coarse_landing_angle(dx, static_cast<unsigned>(dy) >> 1);
    // Surface angle is zero. The helper increments toward the movement angle
    // in groups of five. Equality within the first group selects 0xFFFF: no
    // coefficient transform. This is an algorithm predicate, never a frame
    // number or observed-coordinate tuple. $81:98BF–98F9, $81:996A.
    require(angle <= 5, "unrecovered landing velocity transform");
    require(motion.previous_x_displacement >= 3 && motion.previous_x_displacement < 32U,
            "unrecovered landing response magnitude branch");
    require((summary.selected_high & 0x80U) == 0, "unrecovered reflected landing response");
    require(rider.unsupported_duration < 120, "unrecovered long airborne response");
    require(context.opponent && (context.cartridge_options & 8U) == 0,
            "unrecovered player or option-dependent landing response");
    // The temporary response A computed at $81:93F9 is cleared by $81:9474
    // for this mode/duration. $81:9428 publishes this impulse for next motion.
    motion.orientation_impulse =
        static_cast<std::uint16_t>((motion.previous_x_displacement >> 2) + 1U);
    motion.response_a = 0;
    rider.recontact = true;
    // Sentinel branch ($81:94C7–94CD) preserves both velocities and response B.
}

} // namespace

FlatContactSummary summarize_flat_contact(const FlatContactContent& content,
                                          const CollisionPoints& points,
                                          const TrackSamples& samples, std::uint16_t x,
                                          std::uint16_t y) {
    std::array<Probe, 10> probes{};
    for (std::size_t i = 0; i < probes.size(); ++i) {
        probes[i] = preprocess(content, points[i], samples[i], x, y);
    }
    require(probes[0].penetration == 0xA0U, "unrecovered first-probe contact");
    FlatContactSummary result{};
    std::uint8_t deepest = 0xFF;
    for (std::size_t i = 1; i < probes.size(); ++i) {
        const auto probe = probes[i];
        if (probe.penetration == 0xA0U) {
            if ((probe.descriptor & 0x01FFU) != 0 && result.selected_word == 0) {
                result.selected_word = probe.descriptor;
            }
            continue;
        }
        require(i >= 2, "unrecovered second-probe contact");
        if (!negative_difference(probe.penetration, deepest)) {
            deepest = probe.penetration;
            if (result.selected_word == 0) result.selected_word = probe.descriptor;
            result.selected_high = static_cast<std::uint8_t>(probe.descriptor >> 8);
            result.angle = 0;
        }
        result.penetration = std::max(result.penetration, probe.penetration);
    }
    result.supported = (deepest & 0x80U) == 0;
    result.tile_flags = content_byte(content.flags, tile_index(result.selected_word));
    require(result.tile_flags == 0 || result.tile_flags == 18 || result.tile_flags == 20,
            "unrecovered contact tile flags");
    return result;
}

void resolve_flat_contact(RiderContactState& rider, ContactMotion& motion,
                          const FlatContactSummary& summary, const ContactContext& context) {
    require(context.phase <= 1 && context.mode == 0, "unrecovered contact phase or mode");
    require(rider.unsupported_count <= 9 && rider.auxiliary_flag == 0,
            "unrecovered incoming contact state");
    require(summary.tile_flags == 0 || summary.tile_flags == 18 || summary.tile_flags == 20,
            "unrecovered contact tile flags");
    require((summary.selected_word & 0xC001U) == 0 && (summary.selected_high & 0xC0U) == 0,
            "unrecovered contact summary direction or special descriptor");
    require(summary.penetration < 0x80U
                && (summary.supported ? summary.angle == 0
                                      : summary.angle == -32 && summary.penetration == 0),
            "inconsistent flat contact summary");
    // Compute transactionally so a newly reached branch cannot half-update a
    // rider before the caller reports the missing behavior.
    auto next_rider = rider;
    auto next_motion = motion;
    next_rider.previous_unsupported_count = rider.unsupported_count;
    next_rider.selected_word = summary.selected_word;
    next_rider.selected_high = summary.selected_high;
    next_rider.recontact = false;
    next_rider.angle_unspecified = !summary.supported;
    if (!summary.supported) {
        next_rider.unsupported_count =
            std::min<std::uint16_t>(9, static_cast<std::uint16_t>(rider.unsupported_count + 1U));
        next_rider.unsupported_duration =
            static_cast<std::uint16_t>(rider.unsupported_duration + 1U);
        next_rider.auxiliary_flag = 0;
    } else {
        require(motion.velocity_y < 0x8000U, "unrecovered supported upward velocity");
        next_rider.surface_angle = 0;
        if (rider.unsupported_count >= 9) {
            resolve_recontact(next_rider, next_motion, summary, context);
        } else {
            if (context.phase == 0) {
                next_motion.response_a = 0;
                next_motion.response_b = 0;
            }
            next_motion.velocity_y = 0;
        }
        next_rider.unsupported_count = 0;
        next_rider.unsupported_duration = 0;
    }
    // Save the integrated position BEFORE collision correction ($81:97FD,
    // $81:9810). The next landing compares against these saved coordinates.
    next_rider.previous_uncorrected_x = motion.x;
    next_rider.previous_uncorrected_y = motion.y;
    next_motion.y = static_cast<std::uint16_t>(motion.y - summary.penetration);
    rider = next_rider;
    motion = next_motion;
}

} // namespace unirally
