// The announcement queues: trick rewards and their speed boosts, voices and captions.
//
// Each rider has a 32-entry ring of one-byte announcement events (announcements.hpp). A
// trick, a lap or a result queues its event; one update in a few shows the next event.
// Showing a trick event with a reward class also rewards it: the rider's speed boost
// grows by the event's reward word, scaled by a learned weight that halves with every
// reward of that event. The player's queue also drives the caption row and the tutorial
// hints; the opponent's is never shown, only rewarded (R-0035, R-0052).

#include "reward_queue.hpp"

#include "announcements.hpp"
#include "rider_pose.hpp"
#include "word_arithmetic.hpp"
#include "zoom_zoo_movement.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <span>
#include <stdexcept>

namespace unirally {
namespace {

constexpr unsigned queue_slots_mask = 31; // 32 entries
// $81:C02A-C054: a shown announcement sets the queue's cooldown to 40, 4 less for each
// announcement still waiting, but at least 5; with hints on, 120. A queue found empty sets
// 10. The race update lowers the cooldown by 2 a update, so these are 20, 60 and 5
// updates.
constexpr int longest_display = 40, display_saved_per_waiting = 4, shortest_display = 5;
constexpr std::uint16_t hint_display = 120, empty_queue_wait = 10;
// The reward class table has no class for an event whose entry is 255: it rewards nothing.
constexpr std::uint8_t no_reward_class = 255;
// The learned weights cover events 1-26: event 1's own weight, then a 25-byte bank.
constexpr unsigned last_learned_event = 26;
constexpr std::size_t learned_bank_size = last_learned_event - 1;
// The event whose announcement is a blank HUD buffer on the HUNTER tour.
constexpr std::uint16_t blank_caption = 0;
// $83:CDBC-CE43: a hint group every 300 updates, eight groups.
constexpr std::uint16_t hint_interval = 300;
constexpr unsigned hint_groups_mask = 7;
// A caption is sixteen characters, one per event 1-255.
constexpr std::size_t caption_length = 16, caption_table_size = 255 * caption_length;
// The trick combination table holds one entry for each count of the four trick kinds
// (0-4 each, 625 in all); 254 is "no combination".
constexpr std::size_t combination_table_size = 625;
constexpr std::uint8_t no_combination = 254;
constexpr unsigned most_counted_tricks = 4;

void require(bool condition, const char* refusal) {
    if (!condition) throw std::invalid_argument(refusal);
}

std::uint8_t next_slot(std::uint8_t cursor) {
    return static_cast<std::uint8_t>((cursor + 1U) & queue_slots_mask);
}

unsigned waiting_announcements(const RewardQueueState& queue) {
    return (queue.write_cursor - queue.read_cursor - 1U) & queue_slots_mask;
}

std::uint16_t display_updates(const RewardQueueState& queue) {
    const auto saved = display_saved_per_waiting * static_cast<int>(waiting_announcements(queue));
    return static_cast<std::uint16_t>(std::max(shortest_display, longest_display - saved));
}

// $81:C238 is CMP #$48 then BMI: it tests bit 7 of the 8-bit difference event - 72, not a
// signed comparison, so events 0-71 take the reward path and so do 200-255; 72-199 are
// voices. BRONSEN's voices 200-215 reach the reward path this way. Do not rewrite it as
// event < 72, nor as int8(event) < 72, which would also take 128-199 (R-0035).
bool takes_reward_path(std::uint8_t event) {
    return (static_cast<std::uint8_t>(event - announcement::first_voice) & 0x80U) != 0;
}

bool is_opponent_voice(std::uint8_t event) {
    return (event >= announcement::bronsen_first_voice && event <= announcement::bronsen_last_voice)
        || (event >= announcement::hunter_first_voice && event <= announcement::hunter_last_voice);
}

// Legacy DRAGSTER and M4-12 to M4-15 states carry no learned bank, so only the event-one
// reward they reached is modelled; any other rewarded event is refused rather than given
// the original's skip.
std::uint8_t* legacy_reward_weight(RewardQueueState& queue, std::uint8_t event,
                                   std::span<const std::uint8_t> classes) {
    const bool no_reward =
        event >= 1 && classes.size() >= event && classes[event - 1] == no_reward_class;
    if (no_reward) return nullptr;
    require(event == announcement::roll && classes[0] == 0 && queue.event_one_weight != 0,
            "reward queue left the recovered event-one domain");
    return &queue.event_one_weight;
}

// The weight that scales the opponent's reward for `event`, or none.
std::uint8_t* opponent_reward_weight(RewardQueueState& queue, std::uint8_t event,
                                     std::span<const std::uint8_t> classes,
                                     std::span<std::uint8_t> learned_weights) {
    if (!takes_reward_path(event)) return nullptr;
    require(event != 0, "reward queue holds no published event");
    if (event <= classes.size()) {
        if (classes[event - 1] == no_reward_class) return nullptr;
        if (event == announcement::roll) return &queue.event_one_weight;
        require(event <= last_learned_event, "reward queue left the recovered event-one domain");
        return &learned_weights[event - 2];
    }
    // Past the class table only the opponent's voices get here: the original then reads
    // past the table into code ($81:C241) and past the learned bank ($81:C260) into
    // $7E:21C9-21D8 (HUNTER: $7E:21E9-21F8). Those bytes are zero on every reference
    // capture and the reward-bank guards assert it, so it rewards nothing (R-0035).
    // Deserialization admits no other event here; widen its guards if this ever moves.
    require(is_opponent_voice(event), "reward queue left the recovered event-one domain");
    return nullptr;
}

// A boost below -1 is halved, keeping its sign, before a reward adds to it. The test is
// bit 15 of boost + 1, so 0x7FFF is halved too (into 0xBFFF).
void halve_negative_boost(std::uint16_t& boost) {
    if (negative(static_cast<std::uint16_t>(boost + 1U)))
        boost = static_cast<std::uint16_t>((boost >> 1U) | 0x8000U);
}

// A reward counts the weight into the feature total, halves the weight (never below 1)
// and adds the event's reward word to the boost. The opponent adds the whole word to its
// vertical boost too, where the player adds half ($81:C2A5-C2AD against $81:C169).
void reward_opponent(MovementState& state, std::uint8_t& weight, std::uint8_t event,
                     const MovementContent& content) {
    state.rewards.feature_total = add_word(state.rewards.feature_total, weight);
    weight = std::max<std::uint8_t>(weight >> 1U, 1);
    const auto amount =
        static_cast<std::int16_t>(content_word(content.rotation_reward, 2U * (event - 1U)));
    auto& speed = state.riders[1].speed;
    halve_negative_boost(speed.boost);
    if (amount >= 0) {
        speed.boost = add_word(speed.boost, static_cast<std::uint16_t>(amount));
        speed.vertical_boost = add_word(speed.vertical_boost, static_cast<std::uint16_t>(amount));
    }
}

} // namespace

// The opponent's queue, once per update ($81:C219-C2C9, the player's consumer
// $81:C0CE-C18A with the opponent's words: entries $0CEB, cursors $0D11/$0D13, learned
// weights $7E:2102, feature total $77:0825, boost $11DB/$11E1; R-0035): queue the event this update
// published, then, once the last announcement's display time is over, show the next one
// and reward it. `learned_weights` is the opponent's bank for events 2-26; the legacy
// DRAGSTER and M4-12 to M4-15 states never serialized it and pass it empty.
void update_opponent_announcements(MovementState& state, unsigned published_event,
                                   const MovementContent& content,
                                   std::span<std::uint8_t> learned_weights) {
    require(content.rotation_reward.size() >= 2 && !content.rotation_class.empty(),
            "rotation reward content has the wrong size");
    require(learned_weights.empty() || learned_weights.size() == learned_bank_size,
            "opponent learned reward bank has the wrong size");
    auto& queue = state.rewards;
    if (published_event) queue_opponent_announcement(state, published_event);
    if (queue.cooldown) return;
    const auto slot = next_slot(queue.read_cursor);
    if (slot == queue.write_cursor) {
        queue.cooldown = empty_queue_wait;
        return;
    }
    queue.read_cursor = slot;
    const auto event = queue.entries[slot];
    auto* weight =
        learned_weights.empty()
            ? legacy_reward_weight(queue, event, content.rotation_class)
            : opponent_reward_weight(queue, event, content.rotation_class, learned_weights);
    if (weight && *weight) reward_opponent(state, *weight, event, content);
    queue.cooldown = display_updates(queue);
}

// A full queue (write cursor at the read cursor) drops the event.
void queue_opponent_announcement(MovementState& state, unsigned event) {
    auto& queue = state.rewards;
    if (queue.write_cursor == queue.read_cursor) return;
    queue.entries[queue.write_cursor] = static_cast<std::uint8_t>(event);
    queue.write_cursor = next_slot(queue.write_cursor);
}

// $81:C598-C5C8: a scoring event (below 22, "wrong way") interrupts the tutorial hints at
// once. Only a race started natively has a player queue.
void queue_player_announcement(ZoomZooState& state, unsigned event) {
    if (!state.native_initialization) return;
    auto& announcements = state.player_announcements;
    auto& queue = announcements.queue;
    if (queue.write_cursor == queue.read_cursor) return;
    queue.entries[queue.write_cursor] = static_cast<std::uint8_t>(event);
    if (event < announcement::wrong_way && announcements.hints_active) {
        queue.cooldown = 0;
        announcements.hints_active = 0;
    }
    queue.write_cursor = next_slot(queue.write_cursor);
}

namespace {

// Queue `event` for `rider` (0 the player, 1 the opponent).
void announce(ZoomZooState& state, unsigned rider, unsigned event) {
    if (rider == 0)
        queue_player_announcement(state, event);
    else
        queue_opponent_announcement(state.movement, event);
}

// $82:9D3A-9D70: a combination of tricks is praised in the rider's voice: one of sixteen
// voices of its character pair chosen by x & 15, queued twice.
void announce_combination(ZoomZooState& state, unsigned rider) {
    const unsigned character =
        rider == 0 ? 0U : classic_race_scenario(state.track).opponent_character;
    const auto x = state.movement.riders[rider].motion.x;
    const auto voice = (announcement::first_voice
                        + (character >> 1U) * announcement::voices_per_character_pair + (x & 15U))
                     & 0xffU;
    announce(state, rider, voice);
    announce(state, rider, voice);
}

// A landing on the leading support with any turn counted is a wipeout, unless a head
// bounce is running; the boosts are lost either way.
void announce_wipeout(ZoomZooState& state, unsigned rider) {
    auto& motion = state.movement.riders[rider];
    const auto& turns = motion.quarter_turn;
    const auto& roll = state.rolls[rider];
    const auto counted =
        static_cast<std::uint16_t>((turns.forward_turns & 255U) + turns.reverse_turns
                                   + roll.held_rotations + state.reflection[rider].air_turns);
    if (!counted) return;
    if (!roll.bounce_active) announce(state, rider, announcement::wipeout);
    motion.speed.boost = motion.speed.vertical_boost = 0;
}

// A clean landing announces each kind of trick counted in the air (up to four of each)
// and, if the combination table knows the combination, praises it.
void announce_tricks(ZoomZooState& state, unsigned rider, const ZoomZooContent& content) {
    const auto& turns = state.movement.riders[rider].quarter_turn;
    auto& roll = state.rolls[rider];
    // Three quarters of a turn count as a whole one.
    const auto forward = std::min<unsigned>(
        add_word(turns.forward_turns, turns.forward_quarters == 3 ? 1 : 0), most_counted_tricks);
    const auto reverse = std::min<unsigned>(
        add_word(turns.reverse_turns, turns.reverse_quarters == 3 ? 1 : 0), most_counted_tricks);
    // Turned forward is a flip, backward a roll, relative to the facing at take-off.
    const unsigned flips = turns.reflected_at_start ? reverse : forward;
    const unsigned rolls = turns.reflected_at_start ? forward : reverse;
    const unsigned twists =
        std::min<unsigned>(state.reflection[rider].air_turns >> 1U, most_counted_tricks);
    // `roll` is the X trick's state (trick_roll.cpp): its completed spins are z flips.
    const unsigned z_flips = std::min<unsigned>(roll.completed_rolls, most_counted_tricks);
    if (roll.bounce_active && !roll.support_count_mirror) {
        announce(state, rider, announcement::head_bounce);
        roll.bounce_active = 0;
    }
    if (roll.held_rotations >= 3) announce(state, rider, announcement::tabletop);
    const std::array<std::pair<std::uint8_t, unsigned>, 4> kinds{{{announcement::flip, flips},
                                                                  {announcement::roll, rolls},
                                                                  {announcement::twist, twists},
                                                                  {announcement::z_flip, z_flips}}};
    for (const auto& [first, count] : kinds)
        if (count) announce(state, rider, announcement::trick(first, count));
    // The combination is a radix-five number of the four counts.
    const auto combination = flips * 125U + rolls * 25U + twists * 5U + z_flips;
    require(content.trick_combinations.size() == combination_table_size,
            "ZOOM ZOO trick combination table missing");
    if (content.trick_combinations[combination] != no_combination)
        announce_combination(state, rider);
}

} // namespace

// $82:9B69-9D97: a landing announces its tricks before the queues are consumed, then
// the trick counts start again. A landing on a vertical surface, or one the contact
// response did not take, only brings the quarter turns up to date.
void announce_landing_tricks(ZoomZooState& state, unsigned rider, const ZoomZooContent& content) {
    auto& motion = state.movement.riders[rider];
    auto& turns = motion.quarter_turn;
    auto& roll = state.rolls[rider];
    auto& transition = state.reflection[rider];
    const bool vertical = std::abs(static_cast<std::int16_t>(motion.contact.surface_angle)) == 31;
    if (vertical || (!motion.motion.response_a && motion.contact.unsupported_count >= 2)) {
        (void)update_quarter_turns(motion, false, transition.air_turns, roll.step != 0,
                                   roll.held_rotations);
        return;
    }
    if (state.surface[rider].leading_support)
        announce_wipeout(state, rider);
    else
        announce_tricks(state, rider, content);
    turns.previous_quadrant = turns.forward_turns = turns.reverse_turns = 0;
    turns.forward_quarters = turns.reverse_quarters = 0;
    turns.initialized = false;
    transition.air_turns = 0;
    roll.completed_rolls = roll.held_rotations = 0;
}

namespace {

// R-0052: a caption row's identity is the smallest event whose caption has the same
// sixteen characters, or 0 for a blank row.
std::uint16_t canonical_caption(std::span<const std::uint8_t> captions, unsigned event) {
    require(captions.size() == caption_table_size, "HUNTER caption table is missing");
    const auto text = captions.subspan((event - 1U) * caption_length, caption_length);
    if (std::all_of(text.begin(), text.end(), [](auto c) { return c == ' '; }))
        return blank_caption;
    for (unsigned other = 1; other < event; ++other)
        if (std::equal(text.begin(), text.end(), captions.begin() + (other - 1U) * caption_length))
            return static_cast<std::uint16_t>(other);
    return static_cast<std::uint16_t>(event);
}

// $81:BF32-BFB7: on the HUNTER tour, a queue running dry after a shown announcement moves
// the pending HUNTER message into the HUD buffer the empty row shows (R-0052). An
// effect's end is sixteen spaces: a blank buffer.
void show_hunter_message(ZoomZooState& state) {
    auto& hunter = state.hunter;
    if (!hunter.shown) return;
    hunter.shown = 0;
    if (hunter.message) {
        hunter.hud_event =
            hunter.message == announcement::effect_over ? blank_caption : hunter.message;
        hunter.message = 0;
    }
    hunter.caption = hunter.hud_event;
}

// The player's reward: as the opponent's, but half the reward word goes to the vertical
// boost ($81:C169).
void reward_player(ZoomZooState& state, std::uint8_t event, const MovementContent& content) {
    auto& queue = state.player_announcements.queue;
    require(event <= last_learned_event, "player reward class is outside learned inventory");
    auto& weight =
        event == announcement::roll ? queue.event_one_weight : state.learned_weights[0][event - 2];
    if (!weight) return;
    queue.feature_total = add_word(queue.feature_total, weight);
    weight = std::max<std::uint8_t>(weight >> 1U, 1);
    auto& speed = state.movement.riders[0].speed;
    halve_negative_boost(speed.boost);
    const auto amount = content_word(content.rotation_reward, 2U * (event - 1U));
    if (!negative(static_cast<std::uint16_t>(amount))) {
        speed.boost = add_word(speed.boost, static_cast<std::uint16_t>(amount));
        speed.vertical_boost =
            add_word(speed.vertical_boost, static_cast<std::uint16_t>(amount >> 1U));
    }
}

} // namespace

// $81:BEA8-BEF1, $81:C0CE-C18A and $81:C02A-C054: the player's queue shows its next
// announcement once the last one's display time is over, and rewards a trick. This is
// gameplay state: an earlier message delays a trick's boost. The player's queue holds no
// event above 87, so its reward test is spelled event < 72 (see takes_reward_path).
void show_next_player_announcement(ZoomZooState& state, const MovementContent& content,
                                   std::span<const std::uint8_t> captions) {
    auto& announcements = state.player_announcements;
    auto& queue = announcements.queue;
    if (queue.cooldown) return;
    const auto slot = next_slot(queue.read_cursor);
    const bool hunter = classic_race_scenario(state.track).hunter_tour;
    if (slot == queue.write_cursor) {
        if (!announcements.empty_display) {
            if (hunter) show_hunter_message(state);
            queue.cooldown = empty_queue_wait;
            announcements.empty_display = 1;
        }
        return;
    }
    queue.read_cursor = slot;
    const auto event = queue.entries[slot];
    if (hunter && event) {
        state.hunter.shown = 1;
        state.hunter.caption = canonical_caption(captions, event);
    }
    require(event && (event >= announcement::first_voice || event <= content.rotation_class.size()),
            "player announcement event is outside static inventory");
    if (event < announcement::first_voice && content.rotation_class[event - 1] != no_reward_class)
        reward_player(state, event, content);
    queue.cooldown = announcements.hints_active ? hint_display : display_updates(queue);
    announcements.empty_display = 0;
}

// $81:C55B-C597: an announcement at the front of the player's queue, written at the read
// cursor, which then steps back, so it shows next. A full queue (read at the write
// cursor) takes nothing.
void push_front_player_announcement(ZoomZooState& state, unsigned event) {
    auto& queue = state.player_announcements.queue;
    if (queue.read_cursor == queue.write_cursor) return;
    queue.entries[queue.read_cursor] = static_cast<std::uint8_t>(event);
    queue.read_cursor = static_cast<std::uint8_t>((queue.read_cursor - 1U) & queue_slots_mask);
}

// $83:CDBC-CE43: while the tutorial hints are on, every 300 updates queue the next group
// of four. The race start sets the first count to 30.
void update_tutorial_hints(ZoomZooState& state) {
    auto& announcements = state.player_announcements;
    if (!announcements.hints_active) return;
    if (++announcements.hint_updates != hint_interval) return;
    announcements.hint_updates = 0;
    announcements.hint_group =
        static_cast<std::uint16_t>((announcements.hint_group + 1U) & hint_groups_mask);
    for (unsigned i = 0; i < announcement::hints_per_group; ++i)
        queue_player_announcement(
            state, announcement::first_hint
                       + announcement::hints_per_group * announcements.hint_group + i);
}

} // namespace unirally
