#pragma once
// Original rider "look" animation, recovered in R-0036.
//
// Each rider turns its seat and head toward the other rider, glances back
// after a while, or follows a scripted glance while its idle cycle is latched.
// The original keeps this state outside the race simulation (the words listed
// on RiderLook): nothing in gameplay reads it, so the serialized race state
// does not carry it. It only selects the upper-body overlay frame composed into
// each rider object.
//
// The per-rider words below keep the original 16-bit bit patterns.
#include <array>
#include <cstdint>
#include <optional>
#include <span>

namespace unirally {

class ClassicContentPack;
struct ZoomZooState;
struct ZoomZooContent;

struct RiderLookTables {
    // Packed `presentation.rider.look-tables.v1`: $82:833B (48 bytes of
    // distance, height and glance steps), $83:EC2E (side-view overlay bases),
    // $17:C606 (scripted sequence ranges) and $17:C614 (sequence bytes).
    std::span<const std::uint8_t> bytes;
};
RiderLookTables rider_look_tables(const ClassicContentPack& pack);

// One rider's look state. The original keeps the two riders' words as pairs:
// $0D49/$0D4B, and pairs within $1259-$1273 and $0D5F-$0D71. The look selects
// the overlay frame ($0D45/$0D47).
struct RiderLook {
    std::uint16_t head{};            // $0D49: 0 is the neutral pose 9
    std::uint16_t target{};          // $1259
    std::uint16_t looking_back{};    // $1269
    std::uint16_t distance_step{};   // $126D
    std::uint16_t glance_timer{};    // $1271, negative while resting
    std::uint16_t sequence_cursor{}; // $0D5F
    std::uint16_t sequence_end{};    // $0D63
    std::uint16_t sequence_delay{};  // $0D67
    std::uint16_t sequence_target{}; // $125D
    std::uint16_t sequence_number{}; // $0D6F
    bool operator==(const RiderLook&) const = default;
};
struct RiderLookState {
    std::array<RiderLook, 2> riders{};
    bool operator==(const RiderLookState&) const = default;
};

// $83:EC8E-$83:ED76: the overlay pose each rider's object uses in `updated`.
// It reads the look state from before that update's look step, and the
// updated state's reflected orientation and pose override.
std::array<std::optional<std::uint16_t>, 2> rider_overlay_poses(const RiderLookState& look,
                                                                const ZoomZooState& updated,
                                                                const RiderLookTables& tables);

// $82:87C9-$82:8926: move `head` one step toward `target`. Heads travel along
// four arcs, 1..17 (9 is neutral and stored as 0), 18..25, 26..33 and 34
// upward, which join at 9/18, 2/26 and 7/34.
void step_rider_head(RiderLook& look);

// $82:836D-$82:8926 for the race update that produced `updated`. Only one
// rider steps per update: the player on odd contact phases, the opponent on
// even ones. The caller skips updates the pause menu diverted.
void advance_rider_look(RiderLookState& look, const ZoomZooState& updated,
                        const ZoomZooContent& content, const RiderLookTables& tables);

// True when the update from `previous` to `updated` was diverted by the pause
// menu, which does not run the look, overlay or window-driver steps: the
// engine's suspended-update clock advanced. That covers updates after the
// resume on which Start is still held, where the selection is already zero.
bool zoom_zoo_update_was_paused(const ZoomZooState& previous, const ZoomZooState& updated);

} // namespace unirally
