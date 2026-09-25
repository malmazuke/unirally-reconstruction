// The announcement queues: trick rewards and their speed boosts, voices and captions.

#include "reward_queue.hpp"

#include "rider_pose.hpp"
#include "word_arithmetic.hpp"
#include "zoom_zoo_movement.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <span>
#include <stdexcept>

namespace unirally {

// $81C219-C2C9 consumes the opponent queue. It mirrors the player consumer
// $81C0CE-C18A with the opponent addresses ($0D11/$0D13 cursors, $0CEB
// entries, $7E2102 learned weights, $770825 feature total, $11DB/$11E1
// boost) and one deliberate difference: the opponent adds the *whole* reward
// word to vertical boost ($81C2A5-C2AD) where the player adds half ($81C169).
//
// learned_weights is the opponent's serialized events 2-26 bank. The legacy
// DRAGSTER/M4-12-15 formats never serialized it, so those callers pass an
// empty span and keep the reached event-one domain they were accepted with.
void update_reward_queue(MovementState& state, unsigned event_one, const MovementContent& content,
                         std::span<std::uint8_t> learned_weights) {
    if (content.rotation_reward.size() < 2 || content.rotation_class.empty()) {
        throw std::invalid_argument("rotation reward content has the wrong size");
    }
    if (!learned_weights.empty() && learned_weights.size() != 25) {
        throw std::invalid_argument("opponent learned reward bank has the wrong size");
    }
    if (event_one && state.rewards.write_cursor != state.rewards.read_cursor) {
        state.rewards.entries[state.rewards.write_cursor] = static_cast<std::uint8_t>(event_one);
        state.rewards.write_cursor =
            static_cast<std::uint8_t>((state.rewards.write_cursor + 1U) & 31U);
    }
    if (state.rewards.cooldown) return;
    const auto next = static_cast<std::uint8_t>((state.rewards.read_cursor + 1U) & 31U);
    if (next == state.rewards.write_cursor) {
        state.rewards.cooldown = 10;
        return;
    }
    state.rewards.read_cursor = next;
    const auto event = state.rewards.entries[next];
    std::uint8_t* weight = nullptr;
    if (learned_weights.empty()) {
        // Legacy DRAGSTER/M4-12-15 domain, deliberately unchanged. Those
        // formats carry no opponent learned-weight bank, so only the reached
        // event-one reward is modelled and every other event is rejected
        // rather than silently given the original's skip.
        const bool leading_event = event >= 1 && content.rotation_class.size() >= event
                                && content.rotation_class[event - 1] == 255;
        if (!leading_event
            && (event != 1 || content.rotation_class[0] != 0
                || state.rewards.event_one_weight == 0)) {
            throw std::invalid_argument("reward queue left the recovered event-one domain");
        }
        if (!leading_event) weight = &state.rewards.event_one_weight;
    }
    // $81C238 is CMP #$48 / BMI, which tests bit 7 of the 8-bit difference
    // rather than comparing signed values: the reward path is taken for
    // events 0-71 and again for 200-255, and 72-199 take the voice path.
    // The fixed BRONSEN voices 200-215 therefore reach the reward path,
    // unlike the player's 72-87 which do not. This is deliberately not
    // int8(event)<72; that spelling would also divert 128-199.
    else if (event < 72 || event >= 200) {
        if (event == 0) throw std::invalid_argument("reward queue holds no published event");
        if (event <= content.rotation_class.size()) {
            if (content.rotation_class[event - 1] != 255) {
                if (event == 1)
                    weight = &state.rewards.event_one_weight;
                else if (event <= 26)
                    weight = &learned_weights[event - 2];
                else
                    throw std::invalid_argument("reward queue left the recovered event-one domain");
            }
        } else if ((event < 200 || event > 215) && (event < 232 || event > 247)) {
            // Unreachable while deserialization admits only the produced
            // voices above 71: 200-215 (character 17) and, on the HUNTER tour,
            // 232-247 (character 20, R-0052). Any other would read learned-bank
            // bytes the guards do not cover. Widen those guards with this
            // domain if it ever moves.
            throw std::invalid_argument("reward queue left the recovered event-one domain");
        }
        // Beyond the 72-entry class table only the opponent's voice ranges
        // are reachable. $81C241 then indexes past the table into ROM code and
        // $81C260 past the 26-byte learned bank into $7E21C9-$7E21D8 (HUNTER:
        // $7E21E9-$7E21F8, guarded by track_reference, zero on every HUNTER
        // capture). Those
        // bytes are zero on every authenticated frame of every reference
        // capture, which the appended reward-bank guards now assert rather
        // than leaving to observation, so the original takes its zero-weight
        // exit and publishes no reward. The cartridge class counter it still
        // bumps is outside the recovered inventory here exactly as it is for
        // the player.
    }
    if (weight && *weight) {
        state.rewards.feature_total = add_word(state.rewards.feature_total, *weight);
        *weight = std::max<std::uint8_t>(*weight >> 1U, 1);
        const auto amount =
            static_cast<std::int16_t>(content_word(content.rotation_reward, 2U * (event - 1U)));
        auto& boost = state.riders[1].speed.boost;
        if (negative(static_cast<std::uint16_t>(boost + 1U)))
            boost = static_cast<std::uint16_t>((boost >> 1U) | 0x8000U);
        if (amount >= 0) {
            boost = add_word(boost, static_cast<std::uint16_t>(amount));
            state.riders[1].speed.vertical_boost =
                add_word(state.riders[1].speed.vertical_boost, static_cast<std::uint16_t>(amount));
        }
    }
    const auto remaining =
        static_cast<unsigned>((state.rewards.write_cursor - state.rewards.read_cursor - 1U) & 31U);
    state.rewards.cooldown =
        static_cast<std::uint16_t>(std::max(5, 40 - static_cast<int>(4U * remaining)));
}

void enqueue_zoom_opponent(MovementState& state, unsigned event) {
    if (state.rewards.write_cursor == state.rewards.read_cursor) return;
    state.rewards.entries[state.rewards.write_cursor] = static_cast<std::uint8_t>(event);
    state.rewards.write_cursor = static_cast<std::uint8_t>((state.rewards.write_cursor + 1U) & 31U);
}

// $81C598-C5C8: scoring messages interrupt tutorial text immediately.
void enqueue_zoom_player(ZoomZooState& state, unsigned event) {
    if (!state.native_initialization) return;
    auto& a = state.player_announcements;
    auto& q = a.queue;
    if (q.write_cursor == q.read_cursor) return;
    q.entries[q.write_cursor] = static_cast<std::uint8_t>(event);
    if (event < 22 && a.hints_active) {
        q.cooldown = 0;
        a.hints_active = 0;
    }
    q.write_cursor = static_cast<std::uint8_t>((q.write_cursor + 1U) & 31U);
}

// $829B69-9D97: landing announcements precede queue consumption. Four
// independent trick counts form a radix-five static combination-table index.
void update_zoom_landing_rewards(ZoomZooState& state, unsigned index,
                                 const ZoomZooContent& content) {
    auto& rider = state.movement.riders[index];
    auto& turns = rider.quarter_turn;
    auto& roll = state.rolls[index];
    auto& transition = state.reflection[index];
    const bool vertical = std::abs(static_cast<std::int16_t>(rider.contact.surface_angle)) == 31;
    if (vertical || (!rider.motion.response_a && rider.contact.unsupported_count >= 2)) {
        (void)update_quarter_turns(rider, false, transition.air_turns, roll.step != 0,
                                   roll.held_rotations);
        return;
    }
    const auto announce = [&](unsigned event) {
        if (index == 0)
            enqueue_zoom_player(state, event);
        else
            enqueue_zoom_opponent(state.movement, event);
    };
    if (state.surface[index].leading_support) {
        const auto count =
            static_cast<std::uint16_t>((turns.forward_turns & 255U) + turns.reverse_turns
                                       + roll.held_rotations + transition.air_turns);
        if (count) {
            if (!roll.bounce_active) announce(14);
            rider.speed.boost = rider.speed.vertical_boost = 0;
        }
    } else {
        const auto forward = std::min<unsigned>(
            add_word(turns.forward_turns, turns.forward_quarters == 3 ? 1 : 0), 4);
        const auto reverse = std::min<unsigned>(
            add_word(turns.reverse_turns, turns.reverse_quarters == 3 ? 1 : 0), 4);
        const std::array<unsigned, 4> counts{{turns.reflected_at_start ? reverse : forward,
                                              turns.reflected_at_start ? forward : reverse,
                                              std::min<unsigned>(transition.air_turns >> 1U, 4),
                                              std::min<unsigned>(roll.completed_rolls, 4)}};
        if (roll.bounce_active && !roll.support_count_mirror) {
            announce(16);
            roll.bounce_active = 0;
        }
        if (roll.held_rotations >= 3) announce(17);
        constexpr std::array<unsigned, 4> event_bases{{4, 0, 8, 17}};
        for (unsigned i = 0; i < 4; ++i)
            if (counts[i]) announce(event_bases[i] + counts[i]);
        const auto combination = counts[0] * 125U + counts[1] * 25U + counts[2] * 5U + counts[3];
        if (content.trick_combinations.size() != 625)
            throw std::invalid_argument("ZOOM ZOO trick combination table missing");
        if (content.trick_combinations[combination] != 254) {
            // $829D3A-9D70: incoming X scratch ($A5) and the rider's character
            // ($77:0748,X >> 1, sixteen voices each). A is replaced by this
            // voice ID before BOTH enqueue calls.
            const unsigned character =
                index == 0 ? 0U : classic_race_scenario(state.track).opponent_character;
            const auto voice = (72U + (character >> 1U) * 16U + (rider.motion.x & 15U)) & 0xffU;
            announce(voice);
            announce(voice);
        }
    }
    turns.previous_quadrant = turns.forward_turns = turns.reverse_turns = 0;
    turns.forward_quarters = turns.reverse_quarters = 0;
    turns.initialized = false;
    transition.air_turns = 0;
    roll.completed_rolls = roll.held_rotations = 0;
}

namespace {

// $81BEA8-BEF1, $81C0CE-C18A and $81C02A-C054. This queue is
// gameplay state: an earlier message delays publication of a trick boost.
// R-0052: a caption row's identity, the smallest event whose caption has the
// same sixteen characters, or 0 for a blank row.
std::uint16_t canonical_caption(std::span<const std::uint8_t> captions, unsigned event) {
    if (captions.size() != 4080) throw std::invalid_argument("HUNTER caption table is missing");
    const auto text = captions.subspan((event - 1U) * 16U, 16U);
    if (std::all_of(text.begin(), text.end(), [](auto c) { return c == ' '; })) return 0;
    for (unsigned other = 1; other < event; ++other)
        if (std::equal(text.begin(), text.end(), captions.begin() + (other - 1U) * 16U))
            return static_cast<std::uint16_t>(other);
    return static_cast<std::uint16_t>(event);
}

} // namespace

void consume_zoom_player(ZoomZooState& state, const MovementContent& content,
                         std::span<const std::uint8_t> captions) {
    auto& a = state.player_announcements;
    auto& q = a.queue;
    if (q.cooldown) return;
    const auto cursor = static_cast<std::uint8_t>((q.read_cursor + 1U) & 31U);
    const bool hunter = classic_race_scenario(state.track).hunter_tour;
    if (cursor == q.write_cursor) {
        if (!a.empty_display) {
            // $81:BF32-BFB7: on the HUNTER tour, a dry queue after a shown
            // announcement moves the pending HUNTER message into the HUD buffer
            // the empty row shows (R-0052).
            if (hunter && state.hunter.shown) {
                state.hunter.shown = 0;
                // Event $23 (an effect's end) is sixteen spaces: a blank buffer.
                if (state.hunter.message) {
                    state.hunter.hud_event =
                        state.hunter.message == 0x23 ? 0 : state.hunter.message;
                    state.hunter.message = 0;
                }
                state.hunter.caption = state.hunter.hud_event;
            }
            q.cooldown = 10;
            a.empty_display = 1;
        }
        return;
    }
    q.read_cursor = cursor;
    const auto event = q.entries[cursor];
    if (hunter && event) {
        state.hunter.shown = 1;
        state.hunter.caption = canonical_caption(captions, event);
    }
    if (!event || (event < 72 && event > content.rotation_class.size()))
        throw std::invalid_argument("player announcement event is outside static inventory");
    if (event < 72 && content.rotation_class[event - 1] != 255) {
        if (event > 26)
            throw std::invalid_argument("player reward class is outside learned inventory");
        auto& weight = event == 1 ? q.event_one_weight : state.learned_weights[0][event - 2];
        if (weight) {
            q.feature_total = add_word(q.feature_total, weight);
            weight = std::max<std::uint8_t>(weight >> 1U, 1);
            auto& speed = state.movement.riders[0].speed;
            if (negative(static_cast<std::uint16_t>(speed.boost + 1U)))
                speed.boost = static_cast<std::uint16_t>((speed.boost >> 1U) | 0x8000U);
            const auto amount = content_word(content.rotation_reward, 2U * (event - 1U));
            if (!negative(static_cast<std::uint16_t>(amount))) {
                speed.boost = add_word(speed.boost, static_cast<std::uint16_t>(amount));
                speed.vertical_boost =
                    add_word(speed.vertical_boost, static_cast<std::uint16_t>(amount >> 1U));
            }
        }
    }

    const auto remaining = (q.write_cursor - q.read_cursor - 1U) & 31U;
    q.cooldown = static_cast<std::uint16_t>(
        a.hints_active ? 120 : std::max(5, 40 - static_cast<int>(4U * remaining)));
    a.empty_display = 0;
}

// $81:C55B-C597: an announcement at the front of the player's queue, written
// at the read cursor, which then steps back, so it shows next. A full queue
// (read at the write cursor) takes nothing.
void push_front_zoom_player(ZoomZooState& state, unsigned event) {
    auto& q = state.player_announcements.queue;
    if (q.read_cursor == q.write_cursor) return;
    q.entries[q.read_cursor] = static_cast<std::uint8_t>(event);
    q.read_cursor = static_cast<std::uint8_t>((q.read_cursor - 1U) & 31U);
}

// $83CDBC-CE43: four messages every300 updates while the selected hint
// remains enabled. The race initializer explicitly sets the initial count30.
void update_zoom_hints(ZoomZooState& state) {
    auto& a = state.player_announcements;
    if (!a.hints_active) return;
    if (++a.hint_updates != 300) return;
    a.hint_updates = 0;
    a.hint_group = static_cast<std::uint16_t>((a.hint_group + 1U) & 7U);
    for (unsigned i = 0; i < 4; ++i) enqueue_zoom_player(state, 40U + 4U * a.hint_group + i);
}

} // namespace unirally
