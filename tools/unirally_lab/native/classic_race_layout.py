"""Named byte layout of the 742-byte shared race state (URZZ000B / URDG0001), and
of the 34 special-tile bytes and 60 checkpoint flags the other tracks' state (URTRnn03)
appends (R-0047, R-0048).

Diagnostic only: names follow the native serializer order so a first
divergence can be reported as a field instead of a bare offset.
"""
from __future__ import annotations

RIDER = [
    ('x', 2), ('y', 2), ('velocity_x', 2), ('velocity_y', 2), ('previous_x_displacement', 2),
    ('response_a', 2), ('response_b', 2), ('orientation_impulse', 2),
    ('unsupported_count', 2), ('previous_unsupported_count', 2), ('unsupported_duration', 2),
    ('previous_uncorrected_x', 2), ('previous_uncorrected_y', 2), ('surface_angle', 2),
    ('auxiliary_flag', 2), ('selected_word', 2), ('angle_unspecified', 1), ('selected_high', 1), ('recontact', 1),
    ('boost', 2), ('vertical_boost', 2), ('progress_adjustment', 2), ('marker_word', 2), ('previous_tag', 2),
    ('transition_count', 2), ('transition_rejected', 1),
    ('jump_pending', 2), ('jump_impulse_phase', 2), ('jump_baseline', 2), ('jump_previous_input', 2),
    ('orientation', 2), ('reflected_orientation', 2), ('animation_phase', 2), ('animation_increment', 2),
    ('pose_previous_x', 2), ('pose_previous_y', 2), ('displacement_remainder', 2), ('target_orientation', 2),
    ('pose_index', 2), ('displacement_history0', 2), ('displacement_history1', 2), ('displacement_history2', 2),
    ('rolling_level', 2), ('alternate_animation_phase', 2), ('rolling', 1), ('reflected', 1),
    ('idle_active', 2), ('idle_wobble_offset', 2), ('idle_bias', 2), ('idle_velocity', 2), ('idle_previous_bias', 2),
    ('idle_direction_adjustment', 2), ('idle_cycle_latched', 2), ('idle_cycle_counter', 2), ('idle_orientation_reference', 2),
    ('quarter_previous_quadrant', 2), ('forward_turns', 2), ('reverse_turns', 2), ('forward_quarters', 2), ('reverse_quarters', 2),
    ('quarter_initialized', 1), ('quarter_reflected_at_start', 1),
    ('residue_x', 2), ('residue_y', 2), ('throttle', 2), ('previous_brake', 2), ('launch_override', 2), ('small_motion_counter', 2),
]
REFLECTION = [(n, 2) for n in ('reflection_step', 'reflection_end', 'reflection_pose_base', 'pose_override', 'reflection_completed',
              'reflection_hold', 'drive_pose_enabled', 'air_turns', 'direction_latch', 'base_velocity_cap', 'brake_input',
              'rotate_negative_input', 'rotate_positive_input', 'jump_input', 'wrong_direction_counter')]
SURFACE = [(n, 2) for n in ('surface_mode', 'surface_angle_mode', 'tile_mode', 'leading_support', 'tile_pose', 'animation_delta', 'tile_pose_enabled')]
RACE_RIDER = [(n, 2) for n in ('laps_remaining', 'checkpoint', 'next_checkpoint', 'start_line_latch', 'checkpoint_display_countdown',
              'finished', 'time_minutes', 'time_tens', 'time_seconds', 'time_tenths', 'time_hundredths')]
ROLL = [(n, 2) for n in ('roll_input_latched', 'roll_prior_orientation', 'roll_prior_reflection', 'roll_pose_base', 'roll_step',
        'roll_held_updates', 'roll_bounce_charge', 'roll_completed_rolls', 'roll_held_rotations', 'roll_bounce_active',
        'roll_support_count_mirror', 'roll_prior_step')]


def layout():
    fields = [('magic', 8), ('frame', 4), ('input_low', 1), ('input_high', 1), ('input_vertical', 1), ('input_horizontal', 1)]
    for rider in ('player', 'opponent'):
        fields += [(f'{rider}.{n}', w) for n, w in RIDER]
    fields += [(f'timer.{n}', 2) for n in ('minutes', 'tens', 'seconds', 'tenths', 'subframe')]
    fields += [('ai.impulse_countdown', 2), ('ai.trick_selector', 2), ('ai.suppression_counter', 2)]
    fields += [(f'opponent_queue.entry{i}', 1) for i in range(32)]
    fields += [('opponent_queue.read', 1), ('opponent_queue.write', 1), ('opponent_queue.cooldown', 2),
               ('opponent_queue.feature_total', 2), ('opponent_queue.event_one_weight', 1),
               ('countdown', 2), ('contact_phase', 1), ('progress_phase', 1), ('animation_counter', 1), ('update_counter', 1)]
    for rider in ('player', 'opponent'):
        fields += [(f'{rider}.{n}', w) for n, w in REFLECTION]
    fields += [('opponent_horizontal', 1), ('opponent_retained_oam_x', 1)]
    for rider in ('player', 'opponent'):
        fields += [(f'{rider}.{n}', w) for n, w in SURFACE]
    for rider in ('player', 'opponent'):
        fields += [(f'{rider}.race.{n}', w) for n, w in RACE_RIDER]
    for rider in ('player', 'opponent'):
        fields += [(f'{rider}.lap_time{i}', 2) for i in range(10)]
    fields += [('player.total_time', 2), ('opponent.total_time', 2), ('provisional_1225', 2), ('provisional_1227', 2), ('finish_delay', 2)]
    fields += [(f'camera.{n}', 2) for n in ('x', 'y', 'velocity_x', 'velocity_y', 'lookahead', 'screen_xy')]
    fields += [(f'checkpoint_seen{i}', 1) for i in range(20)]
    for rider in ('player', 'opponent'):
        fields += [(f'{rider}.finish_pose.{n}', 2) for n in ('selector', 'kind', 'locked', 'active')]
    fields += [('fade_level', 2), ('player.start_boost', 2), ('opponent.start_boost', 2), ('result_updates', 2),
               ('result.graph_minimum', 2), ('result.graph_maximum', 2), ('result.player_total', 2), ('result.opponent_total', 2),
               ('player.charge_announced', 2), ('opponent.charge_announced', 2)]
    fields += [(f'player_queue.entry{i}', 1) for i in range(32)]
    fields += [('player_queue.read', 1), ('player_queue.write', 1), ('player_queue.cooldown', 2), ('player_queue.feature_total', 2),
               ('player_queue.event_one_weight', 1), ('hints_active', 2), ('hint_updates', 2), ('hint_group', 2), ('empty_display', 2)]
    for rider in ('player', 'opponent'):
        fields += [(f'{rider}.{n}', w) for n, w in ROLL]
    for rider in ('player', 'opponent'):
        fields += [(f'{rider}.learned_weight{e}', 1) for e in range(2, 27)]
    fields += [('pause.selection', 2), ('pause.released', 2), ('pause.suspended_updates', 4), ('pause.suspended_countdown_updates', 4)]
    offsets, cursor = [], 0
    for name, width in fields:
        offsets.append((cursor, width, name))
        cursor += width
    assert cursor == 742, cursor
    return offsets


# R-0047: per rider, then $0E7B. Original words: $0BCB, $0D57, $0DF7, $0DF3,
# $0DFF, $0547, $0BE7 (+2 for the opponent) and bit 4 of $1516 / $151A.
SPECIAL_TILE_RIDER = [(n, 2) for n in ('mud_cooldown', 'mud_exit_pending', 'corkscrew_latch', 'corkscrew_step',
                      'corkscrew_float', 'physics_hold', 'reflection_lock', 'raised_priority')]
SPECIAL_TILE_WORDS = [0xbcb, 0xd57, 0xdf7, 0xdf3, 0xdff, 0x547, 0xbe7]


def special_tile_bytes(wram: bytes) -> bytes:
    """The 36 bytes URTRnn04 appends first (special tiles, $0E7B, $0C73), from original WRAM."""
    out = bytearray()
    for rider in (0, 1):
        for address in SPECIAL_TILE_WORDS:
            out += wram[address+2*rider:address+2*rider+2]
        out += ((wram[0x1516 if rider == 0 else 0x151a] >> 4) & 1).to_bytes(2, 'little')
    return bytes(out+wram[0xe7b:0xe7d]+wram[0xc73:0xc75])  # $0E7B, then $0C73 (LOCKED-TOURS)


def checkpoint_tail_bytes(wram: bytes) -> bytes:
    """The last 60 first-seen flags ($1161-$119C) URTRnn03 appends after them (R-0048)."""
    return bytes(wram[0x1161:0x119d])


LAYOUT = layout() + [(742+16*r+2*i, 2, f'{("player", "opponent")[r]}.{n}')
                     for r in (0, 1) for i, (n, _) in enumerate(SPECIAL_TILE_RIDER)] + [(774, 2, 'drive_target_latch'), (776, 2, 'opponent_turnaround')] \
    + [(778+i, 1, f'checkpoint_seen{20+i}') for i in range(60)]


def describe(left: bytes, right: bytes, limit=24):
    """Differing fields as (name, left value, right value)."""
    result = []
    for offset, width, name in LAYOUT:
        a, b = left[offset:offset+width], right[offset:offset+width]
        if a != b:
            result.append((name, int.from_bytes(a, 'little'), int.from_bytes(b, 'little')))
            if len(result) >= limit:
                break
    return result
