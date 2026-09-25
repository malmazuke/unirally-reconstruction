#include "rider_look.hpp"

#include "content_pack.hpp"
#include "track_sampling.hpp"
#include "zoom_zoo_movement.hpp"

#include <stdexcept>

namespace unirally {
namespace {

// Offsets inside presentation.rider.look-tables.v1.
constexpr std::size_t forward_distance_table = 0;   // $82:833B, nine words
constexpr std::size_t height_step_table = 16;       // $82:834B, nine words
constexpr std::size_t glance_distance_table = 32;   // $82:835B, eight words
constexpr std::size_t side_overlay_base_table = 48; // $83:EC2E
constexpr std::size_t sequence_range_table = 144;   // $17:C606
constexpr std::size_t sequence_bytes = 158;         // $17:C614
constexpr std::size_t look_table_bytes = 594;

constexpr std::uint16_t neutral_head = 9;

std::uint16_t table_word(const RiderLookTables& tables, std::size_t offset) {
    if (tables.bytes.size() != look_table_bytes || offset + 2 > tables.bytes.size())
        throw std::invalid_argument("rider look table read is outside the packed tables");
    return static_cast<std::uint16_t>(tables.bytes[offset]
                                      | (static_cast<unsigned>(tables.bytes[offset + 1]) << 8U));
}

std::uint16_t wrap(unsigned value) {
    return static_cast<std::uint16_t>(value);
}

// N after CMP: bit 15 of the wrapped difference.
bool compares_negative(std::uint16_t value, std::uint16_t operand) {
    return (wrap(static_cast<unsigned>(value) - operand) & 0x8000U) != 0;
}

} // namespace

void step_rider_head(RiderLook& look) {
    const std::uint16_t raw_target = look.target;
    const std::uint16_t target = raw_target ? raw_target : neutral_head;
    const std::uint16_t head = look.head ? look.head : neutral_head;
    const auto down = wrap(head - 1U);
    const auto up = wrap(head + 1U);
    const auto toward = [&](std::uint16_t goal) {
        return head == goal ? head : (compares_negative(head, goal) ? up : down);
    };
    std::uint16_t next{};
    if (compares_negative(target, 0x12)) {
        if (compares_negative(head, 0x12))
            next = toward(target);
        else
            next = head == 0x12 ? 9 : head == 0x1a ? 2 : head == 0x22 ? 7 : down;
    } else if (compares_negative(target, 0x1a)) {
        if (compares_negative(head, 0x12))
            next = head == 9 ? 0x12 : toward(9);
        else if (compares_negative(head, 0x1a))
            next = toward(raw_target);
        else if (compares_negative(head, 0x22))
            next = head == 0x1a ? 2 : down;
        else
            next = head == 0x22 ? 7 : down;
    } else if (compares_negative(target, 0x22)) {
        if (compares_negative(head, 0x12))
            next = head == 2 ? 0x1a : toward(2);
        else if (compares_negative(head, 0x1a))
            next = head == 0x12 ? 9 : down;
        else if (compares_negative(head, 0x22))
            next = toward(raw_target);
        else
            next = head == 0x22 ? 7 : down;
    } else {
        if (compares_negative(head, 0x12))
            next = head == 7 ? 0x22 : toward(7);
        else if (compares_negative(head, 0x1a))
            next = head == 0x12 ? 9 : down;
        else if (compares_negative(head, 0x22))
            next = head == 0x1a ? 2 : down;
        else
            next = toward(raw_target);
    }
    look.head = next == neutral_head ? 0 : next;
}

namespace {

struct HeadPoint {
    std::uint16_t x{}, y{};
};

HeadPoint head_point(const ZoomZooState& state, std::size_t rider, const ZoomZooContent& content) {
    // $1265-$1268 hold collision point 0 of the current pose ($81:9FA6).
    const auto& movement = state.movement.riders[rider];
    const auto points = collision_points(content.movement.sampling, movement.pose.pose_index,
                                         movement.pose.reflected);
    return {wrap(static_cast<unsigned>(points[0].x) + movement.motion.x),
            wrap(static_cast<unsigned>(points[0].y) + movement.motion.y)};
}

// $82:83F7-$82:8464: choose the vertical target 1..17 for a rider within the
// forward distance step at `distance`.
std::optional<std::uint16_t> height_target(const RiderLookTables& tables, std::uint16_t distance,
                                           HeadPoint own, HeadPoint other) {
    std::uint16_t accumulated = 0;
    std::uint16_t height = own.y;
    if (compares_negative(own.y, other.y)) {
        for (std::uint16_t target = 9;; --target) {
            accumulated = wrap(accumulated + table_word(tables, height_step_table + distance));
            height = wrap(height + accumulated);
            if (!compares_negative(height, other.y)) return target;
            if (compares_negative(wrap(target - 1U), 1)) return std::nullopt;
        }
    }
    for (std::uint16_t target = 9;; ++target) {
        accumulated = wrap(accumulated + table_word(tables, height_step_table + distance));
        height = wrap(height - accumulated);
        if (compares_negative(height, other.y) || height == other.y) return target;
        if (target + 1U == 0x12) return std::nullopt;
    }
}

void look_for_rider(RiderLook& look, RiderLook& player_look, std::size_t rider,
                    const ZoomZooState& updated, const ZoomZooContent& content,
                    const RiderLookTables& tables) {
    const auto own = head_point(updated, rider, content);
    const auto other = head_point(updated, 1 - rider, content);
    const bool reflected = updated.movement.riders[rider].pose.reflected;
    enum class Outcome { none, target, glance } outcome = Outcome::none;
    std::uint16_t target = 0;
    const bool within_height = !compares_negative(wrap(own.y + 0x100U), other.y)
                            && compares_negative(wrap(own.y - 0x100U), other.y);
    if (within_height) {
        const bool other_ahead = reflected
                                   ? compares_negative(own.x, other.x)
                                   : !(compares_negative(own.x, other.x) || own.x == other.x);
        if (!other_ahead) {
            outcome = Outcome::glance;
        } else {
            // $82:83D2/$82:846F: find the distance step that passes the other rider.
            look.looking_back = 0;
            std::uint16_t reach = own.x;
            std::size_t step = 0;
            for (;;) {
                reach = reflected ? wrap(reach + table_word(tables, forward_distance_table + step))
                                  : wrap(reach - table_word(tables, forward_distance_table + step));
                step += 2;
                if (step == 0x12) break;
                const bool keep_going = reflected
                                          ? (compares_negative(reach, other.x) || reach == other.x)
                                          : !compares_negative(reach, other.x);
                if (!keep_going) break;
            }
            if (step != 0x12) {
                step -= 2;
                look.distance_step = static_cast<std::uint16_t>(step);
                if (const auto height =
                        height_target(tables, static_cast<std::uint16_t>(step), own, other)) {
                    outcome = Outcome::target;
                    target = *height;
                }
            }
        }
    }
    if (outcome == Outcome::glance) {
        // $82:8495 / $82:86C0: look back until the timer expires, then rest for
        // 1..64 updates chosen from the update counter.
        outcome = Outcome::none;
        const std::uint16_t limit = rider == 0 ? 0xb4 : 0x3c;
        if (look.glance_timer & 0x8000U) {
            look.glance_timer = wrap(look.glance_timer + 1U);
            look.looking_back = 0;
        } else {
            look.glance_timer = wrap(look.glance_timer + 1U);
            if (look.glance_timer == limit) {
                look.glance_timer =
                    wrap(((updated.movement.update_counter & 0x3fU) ^ 0xffffU) + 1U);
                look.looking_back = 0;
            } else {
                look.looking_back = 1;
                std::uint16_t reach = own.x;
                for (std::size_t step = 0; step < 0x10; step += 2) {
                    if (reflected) {
                        reach = wrap(reach - table_word(tables, glance_distance_table + step));
                        if (compares_negative(reach, other.x)) {
                            outcome = Outcome::target;
                            target = static_cast<std::uint16_t>(step / 2 + 0x12);
                            break;
                        }
                    } else {
                        reach = wrap(reach + table_word(tables, glance_distance_table + step));
                        if (!compares_negative(reach, other.x)) {
                            outcome = Outcome::target;
                            target = static_cast<std::uint16_t>(step / 2 + 0x22);
                            break;
                        }
                    }
                }
            }
        }
    }
    if (outcome == Outcome::none) {
        // $82:8511 and $82:873C both store to the player's $126D.
        player_look.distance_step = 0x11;
        look.target = 0;
    } else {
        look.target = target;
    }
    if (look.target != 0) {
        step_rider_head(look);
        return;
    }
    // $82:852B / $82:8756: scripted glances while the idle cycle is latched.
    const bool latched = updated.movement.riders[rider].idle_pose.cycle_latched != 0;
    if (!latched || (look.sequence_cursor == 0 && look.head != 0)) {
        look.sequence_cursor = look.sequence_end = look.sequence_delay = 0;
        look.sequence_target = look.target = 0;
        step_rider_head(look);
        return;
    }
    if (look.sequence_cursor == 0 && rider == 1) {
        // $82:8927 is reached only from the opponent's copy of this routine.
        look.sequence_number = wrap(look.sequence_number + 1U);
        if (look.sequence_number == 6) look.sequence_number = 0;
        const std::size_t range = sequence_range_table + 2U * look.sequence_number;
        look.sequence_cursor = table_word(tables, range);
        look.sequence_end = table_word(tables, range + 2);
        look.sequence_delay = 1;
    }
    look.target = look.sequence_target;
    look.sequence_delay = wrap(look.sequence_delay - 1U);
    if (look.sequence_delay == 0) {
        if (look.sequence_cursor == look.sequence_end) {
            // The original also clears the rider's idle-cycle latch here.
            look.sequence_cursor = look.sequence_end = look.sequence_delay = 0;
            look.sequence_target = look.target = 0;
        } else {
            const auto entry = table_word(tables, sequence_bytes + look.sequence_cursor);
            // Eight-bit stores: only the low bytes change.
            look.sequence_target =
                static_cast<std::uint16_t>((look.sequence_target & 0xff00U) | (entry & 0xffU));
            look.target = static_cast<std::uint16_t>((look.target & 0xff00U) | (entry & 0xffU));
            look.sequence_delay =
                static_cast<std::uint16_t>((look.sequence_delay & 0xff00U) | (entry >> 8U));
            look.sequence_cursor = wrap(look.sequence_cursor + 2U);
        }
    }
    step_rider_head(look);
}

} // namespace

RiderLookTables rider_look_tables(const ClassicContentPack& pack) {
    return {pack.entry("presentation.rider.look-tables.v1")};
}

std::array<std::optional<std::uint16_t>, 2> rider_overlay_poses(const RiderLookState& look,
                                                                const ZoomZooState& updated,
                                                                const RiderLookTables& tables) {
    std::array<std::optional<std::uint16_t>, 2> poses{};
    for (std::size_t rider = 0; rider < 2; ++rider) {
        const auto head = look.riders[rider].head;
        if (updated.reflection[rider].pose_override != 0 || head == 0) continue;
        const auto base = wrap(((static_cast<unsigned>(head) - 1U) << 5U) + 0x0aecU);
        const auto orientation = updated.movement.riders[rider].pose.reflected_orientation;
        if (!compares_negative(orientation, 0x31)) {
            poses[rider] = wrap(base + wrap(orientation - 0x21U));
        } else if (compares_negative(orientation, 0x10)) {
            poses[rider] = wrap(base + orientation);
        } else if (!compares_negative(orientation, 0x28) || compares_negative(orientation, 0x19)) {
            const auto side = !compares_negative(orientation, 0x28) ? wrap(orientation - 0x1fU)
                                                                    : wrap(orientation - 0x10U);
            const auto side_base = table_word(
                tables, side_overlay_base_table + 2U * (static_cast<std::size_t>(head) - 1U));
            poses[rider] = wrap(side_base + side + 0x100cU);
        }
    }
    return poses;
}

void advance_rider_look(RiderLookState& look, const ZoomZooState& updated,
                        const ZoomZooContent& content, const RiderLookTables& tables) {
    const std::size_t rider = updated.movement.contact_phase != 0 ? 0 : 1;
    look_for_rider(look.riders[rider], look.riders[0], rider, updated, content, tables);
}

bool zoom_zoo_update_was_paused(const ZoomZooState& previous, const ZoomZooState& updated) {
    // The engine counts every update the menu diverts ($83:CD05-CD35): the one
    // that opens it, those with it open, the one that resumes, and any after
    // that on which Start is still held (the menu selection is already zero
    // there; ZOOM ZOO pause-countdown original: Start held 1460-1462 after the
    // resume on 1460, and the channel-6 window stays off through frame 1463).
    // A HUNTER effect's skipped update ($128B, R-0052) diverts it the same
    // way: the race routines, the look step and the drivers do not run.
    return updated.pause.suspended_updates != previous.pause.suspended_updates
        || previous.hunter.skip_update != 0;
}

} // namespace unirally
