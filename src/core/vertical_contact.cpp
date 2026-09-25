#include "vertical_contact.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>

namespace unirally {
namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::invalid_argument(message);
}
std::uint8_t byte(std::span<const std::uint8_t> data, unsigned offset) {
    if (offset >= data.size()) throw std::out_of_range("vertical contact content is incomplete");
    return data[offset];
}
unsigned tile(std::uint16_t word) {
    return ((word & 0x03f0U) >> 2U) + ((word & 15U) >> 1U);
}
bool nonnegative_difference(std::uint8_t left, std::uint8_t right) {
    return ((static_cast<unsigned>(left) - right) & 0x80U) == 0;
}
int signed_word(std::uint16_t value) {
    return value < 0x8000U ? static_cast<int>(value) : static_cast<int>(value) - 65536;
}
std::uint16_t arithmetic_shift(std::uint16_t value, unsigned count) {
    require(count < 16, "vertical slope shift exceeds word width");
    auto result = value;
    for (unsigned i = 0; i < count; ++i) {
        result = static_cast<std::uint16_t>((result >> 1U) | (result & 0x8000U));
    }
    return result;
}
struct Probe {
    std::uint8_t penetration{0xa0}, angle{};
    std::uint16_t descriptor{};
    std::uint8_t direction{};
};
Probe preprocess(const FlatContactContent& content, SamplePoint point, std::uint16_t descriptor,
                 std::uint16_t x, std::uint16_t y) {
    if ((descriptor & 0x03ffU) == 0) {
        if ((descriptor & 0x1c00U) == 0x1c00U) return {0x7f, 0, descriptor};
        return {};
    }
    require((descriptor & 1U) == 0, "vertical contact reaches special remapped descriptor");
    const auto index = tile(descriptor);
    const bool horizontal = (byte(content.flags, index) & 1U) != 0;
    auto column = (static_cast<unsigned>(point.x) + (x & 15U)) & 15U;
    auto local_y = (static_cast<unsigned>(point.y) + (y & 15U)) & 15U;
    if (!horizontal && (descriptor & 0x8000U)) local_y = (~local_y) & 15U;
    const bool mirrored = (descriptor & 0x4000U) != 0;
    if (mirrored) column = (~column) & 15U;
    if (horizontal) std::swap(column, local_y);
    const auto height = byte(content.columns, index * 32U + column * 2U);
    auto angle = byte(content.columns, index * 32U + column * 2U + 1U);
    if (mirrored) angle = static_cast<std::uint8_t>(0U - angle);
    const auto penetration =
        height == 0xa0U
            ? std::uint8_t{0xa0}
            : static_cast<std::uint8_t>(local_y - static_cast<std::uint8_t>(height - 1U));
    return {penetration, angle, descriptor,
            static_cast<std::uint8_t>(horizontal ? (mirrored ? 3 : 4)
                                                 : ((descriptor & 0x8000U) ? 1 : 0))};
}
} // namespace

VerticalContactSummary summarize_vertical_contact(const FlatContactContent& content,
                                                  const CollisionPoints& points,
                                                  const TrackSamples& samples, std::uint16_t x,
                                                  std::uint16_t y) {
    std::array<Probe, 10> probes{};
    for (std::size_t i = 0; i < probes.size(); ++i)
        probes[i] = preprocess(content, points[i], samples[i], x, y);
    VerticalContactSummary result{};
    std::uint8_t support = probes[0].penetration == 0xa0U ? 0xff : probes[0].penetration,
                 angle = 0xe0;
    // $81:8FEF-902D initializes an ordinary vertical first probe before
    // the remaining ordered reduction. A later winner can clear $0F5D.
    if (probes[0].penetration < 0x80U) {
        result.leading_support = true;
        result.any_nonnegative_probe = true;
        if (probes[0].direction < 2) {
            result.penetration = probes[0].penetration;
            result.inverted_vertical = probes[0].direction == 1;
        } else {
            result.horizontal_penetration = probes[0].penetration;
            result.horizontal_direction = probes[0].direction;
        }
        angle = probes[0].angle;
        result.selected_word = probes[0].descriptor;
        result.selected_high = static_cast<std::uint8_t>(probes[0].descriptor >> 8U);
    }
    for (std::size_t i = 1; i < probes.size(); ++i) {
        const auto& probe = probes[i];
        if (probe.penetration == 0xa0U) {
            if ((probe.descriptor & 0x01ffU) != 0 && result.selected_word == 0)
                result.selected_word = probe.descriptor;
            continue;
        }
        if (nonnegative_difference(probe.penetration, support)) {
            support = probe.penetration;
            result.leading_support = i < 2 && probe.penetration < 0x80U;
            if ((probe.descriptor & 1U) != 0 || result.selected_word == 0)
                result.selected_word = probe.descriptor;
            if (nonnegative_difference(probe.angle, angle))
                result.selected_high = static_cast<std::uint8_t>(probe.descriptor >> 8U);
            angle = probe.angle;
        }
        if (probe.penetration < 0x80U) result.any_nonnegative_probe = true;
        if (probe.direction < 2 && probe.penetration < 0x80U
            && nonnegative_difference(probe.penetration, result.penetration)) {
            result.penetration = probe.penetration;
            result.inverted_vertical = (probe.descriptor & 0x8000U) != 0;
            result.angle =
                static_cast<std::int16_t>(probe.angle < 128 ? static_cast<int>(probe.angle)
                                                            : static_cast<int>(probe.angle) - 256);
        }
    }
    for (const auto& probe : probes) {
        if (probe.direction >= 2 && probe.penetration < 128
            && nonnegative_difference(probe.penetration, result.horizontal_penetration)) {
            result.horizontal_penetration = probe.penetration;
            result.horizontal_direction = probe.direction;
        }
    }
    result.supported = support < 0x80U;
    result.angle = static_cast<std::int16_t>(angle < 128 ? static_cast<int>(angle)
                                                         : static_cast<int>(angle) - 256);
    result.boundary_marker = result.penetration == 127;
    result.tile_flags = byte(content.flags, tile(result.selected_word));
    return result;
}

namespace {

// A contact update's inputs and outputs: `incoming` and `motion` as the rider arrived (after
// the tile pair's own changes), `next` and `moved` as it leaves.
struct ContactStep {
    const VerticalContactSummary& summary;
    const ContactContext& context;
    const RiderContactState& incoming;
    const ContactMotion& motion;
    RiderContactState& next;
    ContactMotion& moved;
    unsigned magnitude{}; // the surface angle's magnitude, 0-31
    unsigned flag_pair{};
};

constexpr std::uint16_t airborne_count = 9; // the unsupported count's ceiling
constexpr unsigned wall_angle = 31, steep_angle = 28, slope_response_limit = 26;
constexpr std::uint8_t high_tile = 0x80, high_tile_mirrored = 0x40;
constexpr unsigned loop_pair = 26, lift_pair = 24, slow_pair = 8;
constexpr std::uint16_t long_airtime = 120;
constexpr std::size_t landing_matrix_table_size = 1512;
constexpr unsigned landing_matrix_size = 504, landing_matrix_row = 8;
constexpr int directions = 62, half_directions = 31;

std::uint16_t counted(std::uint16_t count) {
    return std::min<std::uint16_t>(airborne_count, static_cast<std::uint16_t>(count + 1U));
}

// $81:9185-91D1 dispatches on the selected tile's flag pair (flag & 0xFE) before the
// auxiliary and support tests. Pair 24 clears the unsupported count $0F33 and duration $0FBF
// as whole words, after $81:8F9A has snapshotted the incoming count. Pairs 8 and 16 set
// $1349, read only at $81:9685 on a path ($81:966F-9690) that continued contact enters only
// for a magnitude of 31 or more, which the response has already taken ($81:9286); no capture
// executes it, so it is inert. Pair 8 changes the response below ($81:92D9, $81:96FF,
// $81:97E6). Pair 26 (the loop) clears both probe penetrations, $28 and $2C, at the loop's
// top (step 9), so neither the boundary test ($81:91E3, which reads both) nor the correction
// moves the rider (R-0051). Every other pair takes no branch here (R-0047).
void apply_tile_pair(VerticalContactSummary& summary, RiderContactState& incoming,
                     const ContactContext& context, unsigned flag_pair) {
    if (flag_pair == loop_pair && context.loop_top) {
        summary.penetration = 0;
        summary.horizontal_penetration = 0;
        summary.boundary_marker = false;
    }
    if (flag_pair == lift_pair) {
        incoming.unsupported_count = 0;
        incoming.unsupported_duration = 0;
    }
}

// R-0025: the landing's coarse angle from the signed displacement since the last uncorrected
// position. $81:984D-98A8 uses bounded subtraction, not division; a zero subtrahend still
// ends at the angle's endpoint (4 or 31).
int coarse_landing_angle(const ContactStep& s) {
    const auto dx_word = static_cast<std::uint16_t>(s.motion.x - s.incoming.previous_uncorrected_x);
    const auto dy_word = static_cast<std::uint16_t>(s.motion.y - s.incoming.previous_uncorrected_y);
    const auto dx = std::abs(signed_word(dx_word));
    const auto half_dy = std::abs(signed_word(dy_word)) / 2;
    const int magnitude_angle = dx >= half_dy
                                  ? (half_dy == 0 ? 4 : std::max(4, 16 - 4 * (dx / half_dy)))
                                  : (dx == 0 ? 31 : std::min(31, 16 + 4 * (half_dy / dx)));
    return signed_word(dx_word) < 0 ? -magnitude_angle : magnitude_angle;
}

// $81:931C-9358: the landing's rotation response, from the reflected low six pose bits (not
// the surface target) against the direction of arrival; every comparison is on signed
// original words. A rider arriving across the surface turns by up to 2 with its speed; one
// arriving along it decays its responses. Off the leading support the response becomes an
// orientation impulse, and a long flight (120 updates or more) landing nearly flat on its
// back takes the long-airtime matrix. Returns whether it does.
bool landing_rotation(ContactStep& s, int coarse, unsigned pose_index, bool reflected) {
    const auto& summary = s.summary;
    int pose_direction = static_cast<int>(pose_index & 63U);
    if (reflected && pose_direction) pose_direction = 64 - pose_direction;
    const int surface_direction = summary.angle < 0 ? directions + summary.angle : summary.angle;
    const bool negative_dx =
        signed_word(static_cast<std::uint16_t>(s.motion.x - s.incoming.previous_uncorrected_x)) < 0;
    const bool negative_dy =
        signed_word(static_cast<std::uint16_t>(s.motion.y - s.incoming.previous_uncorrected_y)) < 0;
    int orientation_angle = (negative_dx != negative_dy) ? -coarse : coarse;
    orientation_angle = (!negative_dx && !negative_dy)
                          ? std::max(orientation_angle, static_cast<int>(summary.angle))
                          : std::min(orientation_angle, static_cast<int>(summary.angle));
    const int coarse_direction =
        orientation_angle < 0 ? directions + orientation_angle : orientation_angle;
    const int direction_difference =
        (coarse_direction - surface_direction + directions) % directions;
    if (direction_difference == 0 || direction_difference == half_directions) {
        const auto decay = [](std::uint16_t value) {
            return static_cast<std::uint16_t>(value
                                              + (signed_word(value) < 0 ? 1 : (value ? -1 : 0)));
        };
        s.moved.response_a = decay(s.moved.response_a);
        s.moved.response_b = decay(s.moved.response_b);
        return false;
    }
    const int displacement = signed_word(s.motion.previous_x_displacement);
    const int speed_turn = static_cast<int>(s.motion.previous_x_displacement >> 4U);
    int response;
    if (direction_difference > half_directions)
        response = (displacement < 3 && pose_direction < 32)
                     ? -1
                     : (displacement < 1 ? 1 : std::max(-2, -1 - speed_turn));
    else
        response = (displacement < 3 && pose_direction > 32)
                     ? 1
                     : (displacement < 1 ? 1 : std::min(2, speed_turn + 1));
    if (summary.selected_high & high_tile) response = 0;
    s.moved.response_a = static_cast<std::uint16_t>(response);
    if (summary.leading_support) return false;
    if (response) {
        const auto impulse =
            static_cast<std::uint16_t>((s.motion.previous_x_displacement >> 2U) + 1U);
        s.moved.orientation_impulse =
            response < 0 ? static_cast<std::uint16_t>(0U - impulse) : impulse;
    }
    if (s.context.mode == 0 && summary.angle < 30 && summary.angle >= -30
        && s.incoming.unsupported_duration >= long_airtime && pose_direction >= 32) {
        s.moved.response_a = static_cast<std::uint16_t>(response < 0 ? 1 : -1);
        return true;
    }
    s.moved.response_a = 0;
    return false;
}

// The landing matrices (1,512 bytes: three matrices of 63 angle rows) turn the arrival
// velocity into the landing's. A landing well off the surface's angle, on the leading support,
// after a long flight or under HUNTER effect 2 takes one: by the angle's error, matrix 1 or 2;
// matrix 0 for the HUNTER effect ($81:94A8-94C3, $0572, R-0052); matrix 2, with the angle
// pushed by 5, when the rider drives against its velocity.
void apply_landing_matrix(ContactStep& s, int coarse, bool long_airtime_matrix, unsigned horizontal,
                          std::span<const std::uint8_t> landing_matrices) {
    const auto& summary = s.summary;
    const auto angle_difference = std::abs(coarse - static_cast<int>(summary.angle));
    if (!(angle_difference > 5 || summary.leading_support || long_airtime_matrix
          || s.context.landing_matrix_zero))
        return;
    require(landing_matrices.size() == landing_matrix_table_size,
            "landing coefficient matrices are missing");
    unsigned bucket =
        angle_difference ? static_cast<unsigned>(std::min(3, (angle_difference - 1) / 5)) : 0U;
    if (summary.leading_support) bucket = bucket > 1 ? bucket - 1 : 1;
    unsigned matrix = (bucket <= 1 && !long_airtime_matrix) ? 1U : 2U;
    if (s.context.landing_matrix_zero) matrix = 0;
    int angle = (summary.selected_high & high_tile) ? -summary.angle : summary.angle;
    const auto velocity = signed_word(s.moved.velocity_x);
    if ((velocity > 0 && horizontal == 0) || (velocity < 0 && horizontal == 2)) {
        angle = std::clamp(angle + (velocity > 0 ? -5 : 5), -31, 31);
        matrix = 2;
    }
    const auto angle_index = static_cast<unsigned>(angle < 0 ? 31 - angle : angle);
    const auto offset = matrix * landing_matrix_size + angle_index * landing_matrix_row;
    const auto multiply = [&](std::uint16_t value, unsigned coefficient) {
        const auto low = byte(landing_matrices, offset + coefficient * 2U);
        const auto high = byte(landing_matrices, offset + coefficient * 2U + 1U);
        const auto raw = high ? high : low;
        const auto signed_coefficient =
            raw < 128 ? static_cast<int>(raw) : static_cast<int>(raw) - 256;
        const int product = signed_word(value) * signed_coefficient;
        if (high) return static_cast<std::uint16_t>(product);
        // $0566 is the middle/high product word. ASL follows selection, so negative products
        // round down before doubling.
        const int upper = product >= 0 ? product / 256 : -((-product + 255) / 256);
        return static_cast<std::uint16_t>(upper * 2);
    };
    const auto vx = s.moved.velocity_x, vy = s.moved.velocity_y;
    s.moved.velocity_y = static_cast<std::uint16_t>(multiply(vy, 0) + multiply(vx, 1));
    s.moved.velocity_x = static_cast<std::uint16_t>(multiply(vy, 2) + multiply(vx, 3));
}

// A landing: after a flight (9 updates) or onto the leading support. Onto a shallow surface
// it re-contacts with the rotation response and, when needed, the landing matrix; pair 8
// ($81:92D9-92F0) and a steep surface re-contact straight to the correction.
void land(ContactStep& s, unsigned horizontal, unsigned pose_index, bool reflected,
          std::span<const std::uint8_t> landing_matrices) {
    s.next.recontact = true;
    if (s.magnitude >= steep_angle || s.flag_pair == slow_pair) return;
    const int coarse = coarse_landing_angle(s);
    // Player selection alone does not replace the matrix; HUNTER effect 2 ($132B) does.
    require((s.context.cartridge_options & 8U) == 0, "unrecovered landing option");
    const bool long_airtime_matrix = landing_rotation(s, coarse, pose_index, reflected);
    apply_landing_matrix(s, coarse, long_airtime_matrix, horizontal, landing_matrices);
}

// Continued contact on a slope: the responses clear on the ordinary phase, and off the
// leading support velocity y follows the slope (the shift and multiplier tables by angle;
// $81:96FA-970B: under surface mode pair 8 keeps it) and velocity x takes half the angle.
void follow_slope(ContactStep& s, std::span<const std::uint8_t> shifts,
                  std::span<const std::uint8_t> multipliers) {
    const auto& summary = s.summary;
    if (!summary.leading_support && s.context.phase == 0) {
        s.moved.response_a = 0;
        s.moved.response_b = 0;
    }
    const auto shifted = arithmetic_shift(s.motion.velocity_x, byte(shifts, s.magnitude));
    const auto product =
        static_cast<std::uint16_t>(static_cast<unsigned>(shifted) * byte(multipliers, s.magnitude));
    if (!summary.leading_support && s.magnitude < slope_response_limit
        && !(s.context.mode && s.flag_pair == slow_pair)) {
        s.moved.velocity_y = summary.angle < 0 ? static_cast<std::uint16_t>(1U - product) : product;
        if (summary.selected_high & high_tile)
            s.moved.velocity_y = static_cast<std::uint16_t>(0U - s.moved.velocity_y);
    }
    const int contribution = summary.angle < 0 ? -static_cast<int>(s.magnitude / 2U)
                                               : static_cast<int>(s.magnitude / 2U);
    if (!summary.leading_support && s.magnitude < steep_angle
        && !(summary.selected_high & high_tile))
        s.moved.velocity_x =
            static_cast<std::uint16_t>(static_cast<int>(s.motion.velocity_x) + contribution);
}

// $81:92FE-9309 sends a full steep landing straight to the correction; the vertical-to-
// horizontal conversion belongs only to continued contact: velocity y, scaled by the steep
// angle's tables and slowed by 10, becomes velocity x.
void convert_steep_contact(ContactStep& s, std::span<const std::uint8_t> shifts,
                           std::span<const std::uint8_t> multipliers) {
    const auto& summary = s.summary;
    if (s.magnitude != steep_angle || summary.leading_support
        || s.incoming.unsupported_count >= airborne_count)
        return;
    const auto shifted = arithmetic_shift(s.moved.velocity_y, byte(shifts, s.magnitude));
    auto velocity =
        static_cast<std::uint16_t>(static_cast<unsigned>(shifted) * byte(multipliers, s.magnitude));
    if (summary.angle < 0) velocity = static_cast<std::uint16_t>(1U - velocity);
    const auto reduced =
        static_cast<std::uint16_t>(velocity + (signed_word(velocity) < 0 ? 10 : -10));
    if (signed_word(reduced) >= 0) velocity = reduced;
    s.moved.velocity_x =
        (summary.selected_high & high_tile) ? static_cast<std::uint16_t>(0U - velocity) : velocity;
}

// A supported contact: the surface angle, the counters, then the wall, the landing or the
// slope. $81:924E-9275 first removes motion into an inverted contact face.
void support(ContactStep& s, std::span<const std::uint8_t> shifts,
             std::span<const std::uint8_t> multipliers,
             std::span<const std::uint8_t> landing_matrices, unsigned horizontal,
             unsigned pose_index, bool reflected) {
    const auto& summary = s.summary;
    require(s.magnitude < shifts.size(), "vertical response angle outside recovered coefficients");
    if (signed_word(s.moved.velocity_y) < 0 && (summary.selected_high & high_tile)
        && ((summary.selected_high & high_tile_mirrored) ? signed_word(s.moved.velocity_x) < 0
                                                         : signed_word(s.moved.velocity_x) >= 0))
        s.moved.velocity_x = 0;
    s.next.surface_angle = static_cast<std::uint16_t>(summary.angle);
    s.next.angle_unspecified = s.magnitude == wall_angle;
    s.next.unsupported_count = 0;
    s.next.unsupported_duration = 0;
    if (s.magnitude >= wall_angle) {
        // A wall: a quarter of velocity x, and the rider counts as unsupported.
        s.moved.velocity_x = arithmetic_shift(s.moved.velocity_x, 2);
        s.next.unsupported_count = counted(s.incoming.unsupported_count);
        s.next.unsupported_duration =
            static_cast<std::uint16_t>(s.incoming.unsupported_duration + 1U);
    } else if (s.incoming.unsupported_count >= airborne_count
               || (summary.leading_support && s.incoming.unsupported_count >= 2)) {
        land(s, horizontal, pose_index, reflected, landing_matrices);
    } else {
        follow_slope(s, shifts, multipliers);
    }
    convert_steep_contact(s, shifts, multipliers);
}

// The correction: out of the winning vertical probe's penetration (pair 8 keeps the
// horizontal one too), then out of a horizontal one.
void correct_position(ContactStep& s) {
    const auto& summary = s.summary;
    s.next.previous_uncorrected_x = s.motion.x;
    auto horizontal_penetration = summary.horizontal_penetration;
    const bool slow = (summary.tile_flags & 0xfeU) == slow_pair;
    if (summary.penetration >= horizontal_penetration || slow) {
        if (!slow) horizontal_penetration = 0;
        s.next.previous_uncorrected_y = s.motion.y;
        s.moved.y = static_cast<std::uint16_t>(s.motion.y
                                               + (summary.inverted_vertical
                                                      ? summary.penetration
                                                      : -static_cast<int>(summary.penetration)));
    }
    if (summary.horizontal_direction >= 3)
        s.moved.x = static_cast<std::uint16_t>(s.motion.x
                                               + (summary.horizontal_direction == 3
                                                      ? horizontal_penetration
                                                      : -static_cast<int>(horizontal_penetration)));
}

} // namespace

// One update of a rider's contact with vertical columns: the counters, then support (a wall,
// a landing or a slope), then the position correction.
void resolve_vertical_contact(RiderContactState& rider, ContactMotion& motion,
                              const VerticalContactSummary& probed, const ContactContext& context,
                              std::span<const std::uint8_t> shifts,
                              std::span<const std::uint8_t> multipliers,
                              std::span<const std::uint8_t> landing_matrices, unsigned horizontal,
                              unsigned pose_index, bool reflected) {
    require(context.phase <= 1 && context.mode <= 1, "unsupported vertical contact phase/mode");
    require(rider.unsupported_count <= airborne_count && probed.penetration < 128,
            "unsupported vertical contact state");
    auto summary = probed;
    const auto flag_pair = static_cast<unsigned>(summary.tile_flags & 0xfeU);
    auto incoming = rider;
    apply_tile_pair(summary, incoming, context, flag_pair);
    auto next = incoming;
    auto moved = motion;
    next.previous_unsupported_count = rider.unsupported_count;
    next.selected_word = summary.selected_word;
    next.selected_high = summary.selected_high;
    next.recontact = false;
    next.angle_unspecified = true;
    if (!summary.any_nonnegative_probe) next.auxiliary_flag = 0;
    if (summary.boundary_marker) next.auxiliary_flag = 1;
    if (next.auxiliary_flag == 1) {
        // A boundary: only the counters advance (the duration's low byte wraps).
        next.unsupported_duration = static_cast<std::uint16_t>(
            (incoming.unsupported_duration & 0xff00U)
            | static_cast<std::uint8_t>(incoming.unsupported_duration + 1U));
        next.unsupported_count = counted(incoming.unsupported_count);
        rider = next;
        return;
    }
    ContactStep step{summary,
                     context,
                     incoming,
                     motion,
                     next,
                     moved,
                     static_cast<unsigned>(std::abs(static_cast<int>(summary.angle))),
                     flag_pair};
    if (!summary.supported) {
        next.unsupported_count = counted(incoming.unsupported_count);
        next.unsupported_duration = static_cast<std::uint16_t>(incoming.unsupported_duration + 1U);
        next.angle_unspecified = true;
        next.auxiliary_flag = 0;
    } else {
        support(step, shifts, multipliers, landing_matrices, horizontal, pose_index, reflected);
    }
    correct_position(step);
    rider = next;
    motion = moved;
}
} // namespace unirally
