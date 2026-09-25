#pragma once

#include "track_sampling.hpp"

namespace unirally {

// Only the vertical, unreflected flat-column branch is recovered. These are
// immutable R-0008 tile records, not sampled/per-frame original output.
struct FlatContactContent {
    std::span<const std::uint8_t> columns; // 16 (height, angle) byte pairs per tile
    std::span<const std::uint8_t> flags;   // one byte per tile
};

struct FlatContactSummary {
    bool supported{};
    std::uint8_t penetration{}; // position units, subtracted from y
    std::int16_t angle{-32};    // zero on this supported branch; -32 when empty
    std::uint16_t selected_word{}; // first eligible descriptor, not deepest
    std::uint8_t selected_high{};  // last winning probe's descriptor high byte
    std::uint8_t tile_flags{};
};

// This member belongs in a future RiderState. Shared position, velocity and
// orientation channels belong to that rider's motion state, not a shadow copy
// here. R-0011-contact maps each field to its original persistent location.
struct RiderContactState {
    std::uint16_t unsupported_count{};
    std::uint16_t previous_unsupported_count{};
    std::uint16_t unsupported_duration{};
    std::uint16_t previous_uncorrected_x{};
    std::uint16_t previous_uncorrected_y{};
    std::uint16_t surface_angle{};
    bool angle_unspecified{};
    std::uint16_t auxiliary_flag{};
    std::uint16_t selected_word{};
    std::uint8_t selected_high{};
    bool recontact{};
};

// Word fields hold original 16-bit bit patterns, including signed velocities.
// Arithmetic wraps explicitly. Position and previous_x_displacement use the
// original position units; velocity scale is owned by the motion contract.
struct ContactMotion {
    std::uint16_t x{};
    std::uint16_t y{};
    std::uint16_t velocity_x{};
    std::uint16_t velocity_y{};
    std::uint16_t previous_x_displacement{};
    std::uint16_t response_a{};
    std::uint16_t response_b{};
    std::uint16_t orientation_impulse{};
};

struct ContactContext {
    std::uint8_t phase{}; // original $0300, NOT the progress phase $0302
    bool opponent{};
    std::uint16_t mode{};
    std::uint16_t cartridge_options{}; // $77:0750, only bit 3 is tested here
    bool loop_top{}; // the rider's loop step `$0355,y` is 9 (vertical contact, flag pair 26)
};

FlatContactSummary summarize_flat_contact(const FlatContactContent& content,
                                         const CollisionPoints& points,
                                         const TrackSamples& samples,
                                         std::uint16_t x, std::uint16_t y);

// Throws std::invalid_argument for an unrecovered branch and std::out_of_range
// for missing content. On rejection, rider and motion remain unchanged.
// Both riders call this every frame after sampling. Captured inputs are used
// only by the isolated research probe; this function has no reference oracle.
void resolve_flat_contact(RiderContactState& rider, ContactMotion& motion,
                          const FlatContactSummary& summary,
                          const ContactContext& context);

} // namespace unirally
