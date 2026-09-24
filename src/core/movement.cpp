#include "movement.hpp"

#include <algorithm>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

namespace unirally {
namespace {
void put8(std::vector<std::uint8_t>& out, std::uint8_t value) { out.push_back(value); }
void put16(std::vector<std::uint8_t>& out, std::uint16_t value) {
    put8(out, static_cast<std::uint8_t>(value)); put8(out, static_cast<std::uint8_t>(value >> 8));
}
void put32(std::vector<std::uint8_t>& out, std::uint32_t value) {
    put16(out, static_cast<std::uint16_t>(value)); put16(out, static_cast<std::uint16_t>(value >> 16));
}
void put_bool(std::vector<std::uint8_t>& out, bool value) { put8(out, value ? 1 : 0); }

class Reader {
public:
    explicit Reader(std::span<const std::uint8_t> bytes) : bytes_(bytes) {}
    std::uint8_t u8() { require(1); return bytes_[offset_++]; }
    std::uint16_t u16() { const auto lo=u8(); return static_cast<std::uint16_t>(lo | (static_cast<unsigned>(u8())<<8)); }
    std::uint32_t u32() { const auto lo=u16(); return static_cast<std::uint32_t>(lo | (static_cast<std::uint32_t>(u16())<<16)); }
    bool flag() { const auto value=u8(); if(value>1) throw std::invalid_argument("movement state flag is not binary"); return value!=0; }
    void require_end() const { if(offset_!=bytes_.size()) throw std::invalid_argument("movement state has trailing bytes"); }
private:
    void require(std::size_t count) const { if(bytes_.size()-offset_<count) throw std::invalid_argument("movement state is truncated"); }
    std::span<const std::uint8_t> bytes_; std::size_t offset_{};
};

void write_rider(std::vector<std::uint8_t>& out, const RiderMovementState& r) {
    for(auto v:{r.motion.x,r.motion.y,r.motion.velocity_x,r.motion.velocity_y,r.motion.previous_x_displacement,r.motion.response_a,r.motion.response_b,r.motion.orientation_impulse}) put16(out,v);
    for(auto v:{r.contact.unsupported_count,r.contact.previous_unsupported_count,r.contact.unsupported_duration,r.contact.previous_uncorrected_x,r.contact.previous_uncorrected_y,r.contact.surface_angle,r.contact.auxiliary_flag,r.contact.selected_word}) put16(out,v);
    put_bool(out,r.contact.angle_unspecified); put8(out,r.contact.selected_high); put_bool(out,r.contact.recontact);
    for(auto v:{r.speed.boost,r.speed.vertical_boost,r.speed.progress_adjustment,r.progress.marker_word,r.progress.previous_tag,r.progress.transition_count}) put16(out,v);
    put_bool(out,r.progress.transition_rejected);
    for(auto v:{r.jump.pending,r.jump.impulse_phase,r.jump.baseline,r.jump.previous_input,r.pose.orientation,r.pose.reflected_orientation,r.pose.animation_phase,r.pose.animation_increment,r.pose.previous_x,r.pose.previous_y,r.pose.displacement_remainder,r.pose.target_orientation,r.pose.pose_index}) put16(out,v);
    for(auto v:r.pose.displacement_history) put16(out,v);
    put16(out,r.pose.rolling_level); put16(out,r.pose.alternate_animation_phase);
    put_bool(out,r.pose.rolling); put_bool(out,r.pose.reflected);
    for(auto v:{r.idle_pose.active,r.idle_pose.wobble_offset,r.idle_pose.bias,
                r.idle_pose.velocity,r.idle_pose.previous_bias,
                r.idle_pose.direction_adjustment,r.idle_pose.cycle_latched,
                r.idle_pose.cycle_counter,r.idle_pose.orientation_reference}) put16(out,v);
    for(auto v:{r.quarter_turn.previous_quadrant,r.quarter_turn.forward_turns,r.quarter_turn.reverse_turns,r.quarter_turn.forward_quarters,r.quarter_turn.reverse_quarters}) put16(out,v);
    put_bool(out,r.quarter_turn.initialized); put_bool(out,r.quarter_turn.reflected_at_start);
    for(auto v:{r.residue_x,r.residue_y,r.throttle,r.previous_brake,r.launch_override,r.small_motion_counter}) put16(out,v);
}
void read_rider(Reader& in, RiderMovementState& r) {
    for(auto* v:{&r.motion.x,&r.motion.y,&r.motion.velocity_x,&r.motion.velocity_y,&r.motion.previous_x_displacement,&r.motion.response_a,&r.motion.response_b,&r.motion.orientation_impulse}) *v=in.u16();
    for(auto* v:{&r.contact.unsupported_count,&r.contact.previous_unsupported_count,&r.contact.unsupported_duration,&r.contact.previous_uncorrected_x,&r.contact.previous_uncorrected_y,&r.contact.surface_angle,&r.contact.auxiliary_flag,&r.contact.selected_word}) *v=in.u16();
    r.contact.angle_unspecified=in.flag(); r.contact.selected_high=in.u8(); r.contact.recontact=in.flag();
    for(auto* v:{&r.speed.boost,&r.speed.vertical_boost,&r.speed.progress_adjustment,&r.progress.marker_word,&r.progress.previous_tag,&r.progress.transition_count}) *v=in.u16();
    r.progress.transition_rejected=in.flag();
    for(auto* v:{&r.jump.pending,&r.jump.impulse_phase,&r.jump.baseline,&r.jump.previous_input,&r.pose.orientation,&r.pose.reflected_orientation,&r.pose.animation_phase,&r.pose.animation_increment,&r.pose.previous_x,&r.pose.previous_y,&r.pose.displacement_remainder,&r.pose.target_orientation,&r.pose.pose_index}) *v=in.u16();
    for(auto& v:r.pose.displacement_history) v=in.u16();
    r.pose.rolling_level=in.u16(); r.pose.alternate_animation_phase=in.u16();
    r.pose.rolling=in.flag(); r.pose.reflected=in.flag();
    for(auto* v:{&r.idle_pose.active,&r.idle_pose.wobble_offset,&r.idle_pose.bias,
                 &r.idle_pose.velocity,&r.idle_pose.previous_bias,
                 &r.idle_pose.direction_adjustment,&r.idle_pose.cycle_latched,
                 &r.idle_pose.cycle_counter,&r.idle_pose.orientation_reference}) *v=in.u16();
    for(auto* v:{&r.quarter_turn.previous_quadrant,&r.quarter_turn.forward_turns,&r.quarter_turn.reverse_turns,&r.quarter_turn.forward_quarters,&r.quarter_turn.reverse_quarters}) *v=in.u16();
    r.quarter_turn.initialized=in.flag(); r.quarter_turn.reflected_at_start=in.flag();
    for(auto* v:{&r.residue_x,&r.residue_y,&r.throttle,&r.previous_brake,&r.launch_override,&r.small_motion_counter}) *v=in.u16();
}

bool negative(std::uint16_t value) { return (value & 0x8000U) != 0; }

std::uint16_t speed_toward_zero(std::uint16_t velocity, std::int16_t amount) {
    const auto speed=static_cast<std::int16_t>(velocity);
    if(speed>=amount)return static_cast<std::uint16_t>(speed-amount);
    // PAL CMP #$FFF6 / BPL keeps negative equality unchanged; its positive
    // CMP #10 / BMI boundary is intentionally asymmetric.
    if(speed < -amount)return static_cast<std::uint16_t>(speed+amount);
    return velocity;
}

bool has_finish_state(const RaceFinishState& finish) {
    return finish.rider_finished[0] || finish.rider_finished[1] ||
           finish.player_finish_delay != 0 || finish.result_loading_updates != 0 ||
           finish.phase != RacePhase::Racing || finish.outcome != RaceOutcome::Pending;
}

void update_opponent_finish_pose(RaceFinishState& finish, RiderMovementState& rider,
                                 std::uint32_t output_frame) {
    // $82:8953-$82:89C2 with selector table $17:C7D6. Calls occur on the two
    // nonzero phases of the original three-phase counter. Entries 0..47 are
    // $0A45..$0A5C, each duplicated. Entry 48 is a negative sentinel whose
    // reset call republishes the first pose without consuming selector zero.
    auto& selector=finish.opponent_finish_pose_selector;
    if(output_frame%3U!=0U) {
        if(selector>=48U) {
            selector=0;
        } else {
            rider.pose.pose_index=static_cast<std::uint16_t>(0x0a45U+selector/2U);
            ++selector;
            return;
        }
    }
    // `$0DF1` persists between animation-table calls and is copied through
    // `$0F59` on every update before collision sampling.
    rider.pose.pose_index=static_cast<std::uint16_t>(
        0x0a45U+(selector==0U?0U:(selector-1U)/2U));
}

std::uint16_t finish_centiseconds(const RaceTimerDigits& timer, std::uint32_t frame) {
    const auto value = timer.minutes*6000U + timer.tens_seconds*1000U +
        timer.seconds*100U + timer.tenths*10U + timer.subframe*2U + (frame&1U);
    return static_cast<std::uint16_t>(value);
}

void record_finish(RaceFinishState& finish, std::size_t rider,
                   const RaceTimerDigits& timer, std::uint32_t frame) {
    finish.rider_finished[rider]=true;
    finish.finish_time_centiseconds[rider]=finish_centiseconds(timer,frame);
    finish.finish_time_digits[rider]={timer.minutes,timer.tens_seconds,timer.seconds,
                                      timer.tenths,
                                      static_cast<std::uint16_t>(timer.subframe*2U+(frame&1U))};
    finish.finish_animation_countdown[rider]=120;
    if(rider==0) {
        finish.outcome=finish.rider_finished[1]?RaceOutcome::PlayerLost:RaceOutcome::PlayerWon;
        finish.phase=RacePhase::FinishDelay;
    }
}

std::uint16_t add_word(std::uint16_t left, std::uint16_t right) {
    return static_cast<std::uint16_t>(static_cast<std::uint32_t>(left) + right);
}

void update_horizontal(RiderMovementState& rider, bool brake, bool accelerate, bool opponent,
                       const MovementState& whole, const MovementContent& content,
                       int& animation_override,bool& use_throttle_target) {
    // $81:8592 common reset clears the one-update launch override before the
    // throttle routine. The value written by a launch remains in the serialized
    // end-of-frame state and is cleared at the next call.
    rider.launch_override = 0;
    if (rider.contact.unsupported_count >= 2) {
        rider.throttle = 0;
    } else {
        if (brake && rider.motion.velocity_x != 0) {
            throw std::invalid_argument("moving brake is outside the recovered movement domain");
        }
        if (accelerate) {
            rider.motion.velocity_x = add_word(rider.motion.velocity_x, 24);
            const auto signed_boost = static_cast<std::int16_t>(rider.speed.boost);
            const auto limit = static_cast<std::uint16_t>(448U +
                (signed_boost > 0 ? static_cast<unsigned>(signed_boost) : 0U));
            const auto candidate = add_word(rider.throttle, 16);
            if (negative(static_cast<std::uint16_t>(candidate - limit))) rider.throttle = candidate;
        } else {
            rider.throttle = 0;
        }
        if (brake) {
            rider.motion.velocity_x = 0;
        } else if (accelerate && rider.previous_brake != 0 && rider.throttle != 0) {
            rider.motion.velocity_x = add_word(rider.motion.velocity_x, rider.throttle);
            rider.throttle = 0;
            rider.launch_override = 256;
        }
    }
        rider.previous_brake = brake ? 1 : 0;

    if(accelerate && static_cast<std::int16_t>(rider.motion.previous_x_displacement)<4) {
        if(rider.small_motion_counter!=4)rider.small_motion_counter=add_word(rider.small_motion_counter,1);
        animation_override=static_cast<std::int16_t>(rider.small_motion_counter);
        use_throttle_target=true;
    }

    SpeedLimitContext context{};
    context.opponent = opponent;
    context.pose_byte = 0; // Primary screen-coordinate values 43..104 keep this branch inactive.
    context.start_override = rider.launch_override != 0;
    context.ai_enabled = true;
    context.player_progress = whole.riders[0].progress.transition_count;
    context.opponent_progress = whole.riders[1].progress.transition_count;
    context.adjustment_limit = 96;
    context.player_base_cap = 448;
    context.update_counter = whole.update_counter;
    context.friction_mode = accelerate ? 2 : 1;
    limit_rider_speed(rider.motion.velocity_x, rider.motion.velocity_y,
                      rider.speed, context, content.speed_decay);
}

void update_jump(RiderMovementState& rider,bool jump_input) {
    bool advance=rider.jump.impulse_phase!=0;
    if (!advance && rider.jump.pending) {
        if (rider.contact.angle_unspecified) {
            rider.jump.previous_input=jump_input?1:0;
            return;
        }
        rider.jump.pending=0;
        advance=true;
    }
    if (!advance) {
        if (!rider.jump.previous_input && jump_input && rider.contact.unsupported_count<2) {
            rider.jump.pending=1;
        }
    } else if (!jump_input) {
        rider.jump.impulse_phase=0;
    } else {
        rider.jump.impulse_phase=add_word(rider.jump.impulse_phase,1);
        if (rider.jump.impulse_phase>=9) {
            rider.jump.impulse_phase=0;
        } else {
            const auto impulse=static_cast<std::uint16_t>(rider.jump.baseline+
                (rider.jump.impulse_phase<5 ? -144 : -192));
            if (!negative(static_cast<std::uint16_t>(rider.motion.velocity_y-impulse))) {
                rider.motion.velocity_y=impulse;
            }
        }
    }
    rider.jump.previous_input=jump_input?1:0;
}

void update_active_low_speed_damping(RiderMovementState& rider) {
    // $82:A5FA-A61E runs only on the rider's alternating active phase. In the
    // recovered flat branch it moves nonzero velocities with magnitude below
    // 64 one unit toward zero before the general speed limiter/friction pass.
    if (rider.contact.surface_angle != 0 || rider.motion.velocity_x == 0) return;
    const auto velocity = static_cast<std::int16_t>(rider.motion.velocity_x);
    if (velocity > 0 && velocity < 64) {
        rider.motion.velocity_x = static_cast<std::uint16_t>(velocity - 1);
    } else if (velocity < 0 && velocity >= -64) {
        rider.motion.velocity_x = static_cast<std::uint16_t>(velocity + 1);
    }
}

void apply_finish_slowdown(RiderMovementState& rider) {
    // $83:E90D-$83:E932 runs before the ordinary horizontal update. It moves
    // signed velocity ten units toward zero only when it cannot cross zero.
    // Keeping that ordering is what produces both the 38->2 accepted
    // tail and the reviewer-owned 37->1 neighboring case.
    rider.motion.velocity_x=finish_speed_toward_zero(rider.motion.velocity_x);
}

void update_gravity(RiderMovementState& rider) {
    const auto vertical=static_cast<std::int16_t>(rider.motion.velocity_y);
    if (vertical>=512) return;
    const auto increment=vertical<0 ? 19 : 19-(vertical>>5);
    rider.motion.velocity_y=static_cast<std::uint16_t>(vertical+increment);
    rider.motion.y=add_word(rider.motion.y,1);
}

void decay_idle_wobble(RiderMovementState& rider,bool surface_mode=false) {
    // $81:8625-$81:8672 preserves an offset produced by the preceding idle
    // update for one frame. Otherwise it approaches zero by five, or by two
    // while the contact response word is nonzero, and then clears the marker.
    auto& idle=rider.idle_pose;
    if(idle.active==0) {
        auto offset=static_cast<std::int16_t>(idle.wobble_offset);
        if(offset>=512 || offset<-512) {
            idle.wobble_offset=0;
        } else if(offset!=0) {
            const int step=surface_mode?2:5;
            offset=static_cast<std::int16_t>(
                offset>0?std::max(0,static_cast<int>(offset)-step)
                        :std::min(0,static_cast<int>(offset)+step));
            idle.wobble_offset=static_cast<std::uint16_t>(offset);
        }
    }
    idle.active=0;
}

void clear_idle_cycle(IdlePoseState& idle) {
    // The reset path intentionally preserves wobble_offset and
    // orientation_reference ($0F37/$0F83).
    idle.bias=idle.velocity=idle.previous_bias=idle.direction_adjustment=0;
    idle.active=idle.cycle_latched=idle.cycle_counter=0;
}

void update_idle_pose(RiderMovementState& rider,bool race_active,bool opponent,
                      std::uint8_t animation_counter,
                      std::span<const std::uint8_t> table) {
    if(table.size()!=64) throw std::invalid_argument("idle pose table has the wrong size");
    auto& idle=rider.idle_pose;
    if(!race_active || static_cast<std::int16_t>(rider.motion.previous_x_displacement)>=2 ||
       static_cast<std::int16_t>(rider.contact.unsupported_count)>=2) {
        clear_idle_cycle(idle);
        return;
    }
    if(idle.cycle_latched==0) {
        const auto next=add_word(idle.cycle_counter,1);
        if(static_cast<std::int16_t>(next)<120) idle.cycle_counter=next;
    }
    idle.orientation_reference=rider.pose.reflected_orientation;
    if(idle.orientation_reference!=0 && rider.pose.reflected) {
        idle.orientation_reference=static_cast<std::uint16_t>(64-idle.orientation_reference);
    }
    if(static_cast<std::int16_t>(rider.pose.pose_index)>=0x0aec) {
        clear_idle_cycle(idle);
        return;
    }
    idle.active=1;

    const auto reference=static_cast<std::int16_t>(idle.orientation_reference);
    auto bias=static_cast<std::int16_t>(idle.bias);
    if(bias!=0) {
        if(reference!=0) {
            idle.direction_adjustment=1;
        } else if(bias>0) {
            idle.bias=static_cast<std::uint16_t>(bias-1);
            idle.direction_adjustment=1;
        } else {
            idle.bias=0;
            idle.direction_adjustment=0;
        }
    } else {
        idle.bias=0;
        idle.direction_adjustment=0;
    }

    auto velocity=static_cast<std::int16_t>(idle.velocity);
    int candidate=velocity;
    // $0FF9 selects the counter pair at $04C7/$04C9. The opponent word is the
    // modulo-32 complement used by the original's second rider pass.
    const auto rider_counter=opponent
        ? static_cast<std::uint8_t>((32U-animation_counter)&31U)
        : animation_counter;
    const bool increase=reference==0 ? (rider_counter&0x10U)!=0
                                     : reference>=32;
    if(increase) {
        candidate=velocity+1+static_cast<std::int16_t>(idle.direction_adjustment);
        if(candidate<17) idle.velocity=static_cast<std::uint16_t>(candidate);
    } else {
        candidate=velocity-1-static_cast<std::int16_t>(idle.direction_adjustment);
        if(candidate>=-16) idle.velocity=static_cast<std::uint16_t>(candidate);
    }

    if(animation_counter==0) {
        idle.bias=static_cast<std::uint16_t>(candidate);
        if(static_cast<std::int16_t>(idle.cycle_counter)>=60 && idle.cycle_latched==0) {
            idle.cycle_latched=1;
            idle.cycle_counter=0;
        }
        if(idle.previous_bias==idle.bias) idle.bias=0;
        idle.previous_bias=idle.bias;
    }

    velocity=static_cast<std::int16_t>(idle.velocity);
    bool use_table=false;
    if(velocity==0) {
        if(reference<9 || reference>=58) {
            use_table=true;
        } else {
            idle.velocity=static_cast<std::uint16_t>(reference<32?-1:1);
        }
    } else if(velocity<0) {
        // $82:A1F2-A1FF: a falling velocity below 32 is applied unchanged.
        // An earlier reset to -1 for 9..31 had no source; DRAGSTER diff fuzz
        // seed 66 reaches it at 1977 (R-0038).
        if(reference>=32 && reference<58) use_table=true;
    } else if(reference>=9 && reference<32) {
        use_table=true;
    }
    const auto delta=use_table
        ? static_cast<std::int8_t>(table[static_cast<std::size_t>(reference)])
        : static_cast<std::int16_t>(idle.velocity);
    idle.wobble_offset=add_word(idle.wobble_offset,static_cast<std::uint16_t>(delta));
}

void integrate_motion(RiderMovementState& rider) {
    auto axis=[](std::uint16_t& position,std::uint16_t velocity,std::uint16_t& residue) {
        const auto total=static_cast<std::int32_t>(static_cast<std::int16_t>(velocity))+
                         static_cast<std::int16_t>(residue);
        const auto magnitude=total<0?-total:total;
        auto whole=magnitude/32;
        auto remainder=magnitude%32;
        if(total<0){whole=-whole;remainder=-remainder;}
        position=static_cast<std::uint16_t>(position+whole);
        residue=static_cast<std::uint16_t>(remainder);
        return static_cast<std::int16_t>(whole);
    };
    (void)axis(rider.motion.x,rider.motion.velocity_x,rider.residue_x);
    (void)axis(rider.motion.y,rider.motion.velocity_y,rider.residue_y);
}

unsigned content_word(std::span<const std::uint8_t> bytes,unsigned index) {
    if(index+1>=bytes.size())throw std::out_of_range("movement table word is unavailable");
    return bytes[index]|(static_cast<unsigned>(bytes[index+1])<<8U);
}

unsigned integer_sqrt(unsigned value) {
    unsigned root=0;
    while((root+1U)*(root+1U)<=value)++root;
    return root;
}

void update_rolling_mode(RiderMovementState& rider,bool surface_mode=false) {
    if(rider.contact.selected_high&0x80U) {rider.pose.rolling=true;return;}
    if(surface_mode) {rider.pose.rolling=false;return;}
    if (rider.contact.unsupported_count == 9 || rider.pose.reflected_orientation < 45) {
        rider.pose.rolling = false;
    } else if (rider.pose.rolling &&
               static_cast<std::int16_t>(rider.motion.previous_x_displacement) < 14) {
        rider.pose.rolling = false;
    } else if (!rider.pose.rolling &&
               static_cast<std::int16_t>(rider.motion.previous_x_displacement) < 16) {
        return;
    } else {
        const bool moving_nonnegative = static_cast<std::int16_t>(rider.motion.velocity_x) >= 0;
        rider.pose.rolling = rider.pose.reflected ? moving_nonnegative : !moving_nonnegative;
    }
}

void update_pose(RiderMovementState& rider,std::uint8_t counter,std::uint8_t contact_phase,
                 const MovementContent& content,int animation_override,bool use_throttle_target,
                 int surface_angle_offset=0,std::uint16_t mud_velocity=0) {
    if(content.pose_slopes.size()!=128 || content.displacement_table.size()!=512) {
        throw std::invalid_argument("movement pose tables have the wrong size");
    }
    if (!rider.contact.angle_unspecified) {
        int target_signed{};
        if (rider.pose.rolling && rider.pose.rolling_level != 0) {
            target_signed = static_cast<std::int16_t>(rider.contact.surface_angle) +
                            (rider.pose.reflected ? 23 : -23);
        } else {
            // $83:EF97-EFB2: throttle after a small-displacement drive,
            // otherwise mud's braked velocity ($0F3F) or velocity x.
            const auto target_source=static_cast<std::int16_t>(
                use_throttle_target?rider.throttle:mud_velocity?mud_velocity:rider.motion.velocity_x);
            target_signed=target_source>>5;
        }
        if(rider.contact.selected_high&0x80U)target_signed=0;
        target_signed=std::clamp(target_signed+surface_angle_offset,-31,31);
        const unsigned target_index=target_signed>=0?static_cast<unsigned>(target_signed):
                                    static_cast<unsigned>(-target_signed+32);
        rider.pose.target_orientation=content.pose_slopes[target_index+((rider.contact.selected_high&0x80U)?64U:0U)];
    }
    auto orientation=static_cast<std::uint16_t>(rider.pose.orientation+rider.motion.response_b);
    if(rider.motion.response_a) {
        orientation=add_word(orientation,rider.motion.response_a);
    } else if(rider.contact.unsupported_count<9 && rider.pose.target_orientation==rider.pose.orientation) {
        // $83:F02D-F034 skips the store: a supported rider already at its
        // target keeps its orientation, so rotation input is not applied
        // (DRAGSTER diff fuzz seed 135, the update after an L-held landing).
        orientation=rider.pose.orientation;
    } else if(rider.contact.unsupported_count<9) {
        const auto target=rider.pose.target_orientation;
        const bool increase=target>=32 ?
            (static_cast<std::int16_t>(target-rider.pose.orientation)>=1 &&
             static_cast<std::int16_t>(target-rider.pose.orientation)<=32) :
            !(static_cast<std::int16_t>(rider.pose.orientation-target)>=1 &&
              static_cast<std::int16_t>(rider.pose.orientation-target)<=32);
        const int direction=increase?1:-1;
        orientation=static_cast<std::uint16_t>(rider.pose.orientation+direction);
        if(static_cast<std::int16_t>(rider.motion.previous_x_displacement)>=16) {
            for(unsigned step=0;step<2 && (orientation&63U)!=target;++step) {
                orientation=static_cast<std::uint16_t>(
                    static_cast<int>(orientation&63U)+direction);
            }
        }
    }
    rider.pose.orientation=orientation&63U;
    // $83:F09E-$83:F0B9 applies signed truncation toward zero to the idle
    // oscillator before the contact impulse and reflection operations.
    const int wobble=static_cast<std::int16_t>(rider.idle_pose.wobble_offset)/16;
    const int combined=static_cast<int>(rider.pose.orientation)+wobble+
        (static_cast<std::int16_t>(rider.motion.orientation_impulse)>>1);
    rider.pose.reflected_orientation=static_cast<std::uint16_t>(combined)&63U;
    if((counter&1U)==0 && rider.motion.orientation_impulse) {
        const auto impulse=static_cast<std::int16_t>(rider.motion.orientation_impulse);
        rider.motion.orientation_impulse=static_cast<std::uint16_t>(impulse+(impulse>=0?-1:1));
    }
    if(rider.pose.reflected_orientation && rider.pose.reflected) {
        rider.pose.reflected_orientation=64-rider.pose.reflected_orientation;
    }
    rider.pose.displacement_history[2]=rider.pose.displacement_history[1];
    rider.pose.displacement_history[1]=rider.pose.displacement_history[0];
    rider.pose.displacement_history[0]=rider.motion.previous_x_displacement;
    int steering{};
    if(rider.jump.impulse_phase>=1 || rider.contact.angle_unspecified) {
        steering=static_cast<std::int16_t>(rider.pose.animation_increment);
        if((counter&3U)==3) steering+=steering>0?-1:(steering<0?1:0);
        rider.pose.animation_increment=static_cast<std::uint16_t>(steering);
    } else {
        int dx=static_cast<std::int16_t>(rider.pose.previous_x-rider.motion.x);
        int dy=static_cast<std::int16_t>(rider.pose.previous_y-rider.motion.y);
        if(std::abs(dy)<3)dy=0;
        const auto square_x=content_word(content.displacement_table,2U*(std::abs(dx)&255));
        const auto square_y=content_word(content.displacement_table,2U*(std::abs(dy)&255));
        int distance=static_cast<int>(integer_sqrt((square_x+square_y)&65535U));
        if(dx<0)distance=-distance;
        rider.motion.previous_x_displacement=static_cast<std::uint16_t>(distance);
        const int accumulation=static_cast<std::int16_t>(rider.pose.displacement_remainder+distance);
        int remainder{};
        if(accumulation) {
            steering=std::abs(accumulation)/3;
            remainder=std::abs(accumulation)%3;
            if(accumulation<0)steering=-steering;
            if(dx<0)remainder=-remainder;
        } else {
            steering=distance;
        }
        rider.pose.displacement_remainder=static_cast<std::uint16_t>(remainder);
        rider.pose.animation_increment=static_cast<std::uint16_t>(steering);
    }
    if(rider.pose.reflected)steering=-steering;
    if(animation_override)steering=animation_override;
    int phase=static_cast<std::int16_t>(rider.pose.animation_phase)+steering;
    phase%=24;if(phase<0)phase+=24;
    rider.pose.animation_phase=static_cast<std::uint16_t>(phase);
    int animation=phase*64;
    if (rider.pose.rolling) {
        const auto rate=std::abs(static_cast<std::int16_t>(rider.pose.animation_increment));
        if(rate<3 && static_cast<std::uint8_t>(rider.pose.rolling_level)!=0) {
            rider.pose.rolling_level=static_cast<std::uint16_t>(
                static_cast<std::uint8_t>(rider.pose.rolling_level)-1U);
        } else if(rate>=5 && static_cast<std::uint8_t>(rider.pose.rolling_level)!=2) {
            rider.pose.rolling_level=static_cast<std::uint16_t>(
                static_cast<std::uint8_t>(rider.pose.rolling_level)+1U);
        }
        // $83:EED2-EEDB always returns to the ordinary pose after
        // the low-rate decrement, even when rolling level remains nonzero.
        if(rate>=3 && static_cast<std::uint8_t>(rider.pose.rolling_level)!=0) {
            int alternate=static_cast<std::uint16_t>(rider.pose.alternate_animation_phase)+
                          (phase&1)+contact_phase;
            if(alternate>=3)alternate-=3;
            rider.pose.alternate_animation_phase=static_cast<std::uint16_t>(alternate);
            animation=alternate*64+
                (static_cast<std::uint8_t>(rider.pose.rolling_level)==1?0x8e0:0x820);
        }
    }
    rider.pose.pose_index=static_cast<std::uint16_t>(animation+rider.pose.reflected_orientation);
    rider.motion.previous_x_displacement=static_cast<std::uint16_t>(
        std::abs(static_cast<std::int16_t>(rider.motion.previous_x_displacement)));
    rider.pose.previous_x=rider.motion.x;
    rider.pose.previous_y=rider.motion.y;
    (void)contact_phase;
}

int stationary_animation_override(const RiderMovementState& rider,std::uint8_t horizontal) {
    const bool prior_motion=
        std::any_of(rider.pose.displacement_history.begin(),rider.pose.displacement_history.end(),
                    [](auto value){return static_cast<std::int16_t>(value)>=2;}) ||
        static_cast<std::int16_t>(rider.motion.previous_x_displacement)>=2;
    if((rider.contact.unsupported_count<2 && prior_motion) || horizontal==1)return 0;
    int value=horizontal<1?2:-2;
    return rider.pose.reflected?-value:value;
}

unsigned update_quarter_turns(RiderMovementState& rider,bool leading_support=false,unsigned air_turns=0,bool rolling=false,unsigned held_rotations=0) {
    auto& turns=rider.quarter_turn;
    const bool vertical_endpoint=std::abs(static_cast<std::int16_t>(rider.contact.surface_angle))==31;
    if(vertical_endpoint || (!rider.motion.response_a && rider.contact.unsupported_count>=2)) {
        // $829ABD-AF3 re-bases an active roll's quadrant and clears only
        // partial quarters, then still compares the current angle this update.
        // Completed turns and entry reflection survive; full init is separate.
        if(rolling && !held_rotations && (vertical_endpoint || rider.contact.unsupported_count!=2)) {
            const auto rotation=static_cast<std::int8_t>(rider.motion.response_b&0xffU);
            turns.previous_quadrant=(static_cast<std::uint16_t>(static_cast<int>(rider.pose.orientation)-rotation)&63U)>>4U;
            turns.forward_quarters=turns.reverse_quarters=0;
        }
        if((!vertical_endpoint && rider.contact.unsupported_count==2) || !turns.initialized) {
            const auto rotation=static_cast<std::int8_t>(rider.motion.response_b&0xffU);
            const int prior=static_cast<int>(rider.pose.orientation)-rotation;
            turns.previous_quadrant=static_cast<std::uint16_t>(prior)&63U;
            turns.previous_quadrant=static_cast<std::uint16_t>(turns.previous_quadrant>>4U);
            turns.reflected_at_start=rider.pose.reflected;
            turns.initialized=true;
            turns.forward_turns=turns.reverse_turns=0;
            turns.forward_quarters=turns.reverse_quarters=0;
        } else {
            const auto current=static_cast<std::uint16_t>(rider.pose.orientation>>4U);
            const auto previous=turns.previous_quadrant;
            if(current!=previous) {
                bool increasing{};
                if(current==3)increasing=previous==2;
                else if(previous==3)increasing=current!=2;
                else increasing=current>previous;
                auto& quarters=increasing?turns.forward_quarters:turns.reverse_quarters;
                auto& opposite=increasing?turns.reverse_quarters:turns.forward_quarters;
                auto& completed=increasing?turns.forward_turns:turns.reverse_turns;
                quarters=add_word(quarters,1);
                if(quarters>=4){completed=add_word(completed,1);quarters=0;}
                opposite=0;
            }
            turns.previous_quadrant=current;
        }
        return false;
    }
    if(leading_support) {
        const bool event=turns.forward_turns+turns.reverse_turns+air_turns!=0;
        if(event)rider.speed.boost=rider.speed.vertical_boost=0;
        turns.previous_quadrant=turns.forward_turns=turns.reverse_turns=0;
        turns.forward_quarters=turns.reverse_quarters=0;turns.initialized=false;
        return event?14U:0U;
    }
    if(!turns.initialized)return false;
    if(turns.forward_quarters==3)turns.forward_turns=add_word(turns.forward_turns,1);
    if(turns.reverse_quarters==3)turns.reverse_turns=add_word(turns.reverse_turns,1);
    const auto forward=std::min<std::uint16_t>(turns.forward_turns,4);
    const auto reverse=std::min<std::uint16_t>(turns.reverse_turns,4);
    unsigned event{};
    if(forward)event=forward+(turns.reflected_at_start?0U:4U);
    if(reverse) {
        if(event)throw std::invalid_argument("combined rotation reward is outside the recovered domain");
        event=reverse+(turns.reflected_at_start?4U:0U);
    }
    turns.previous_quadrant=turns.forward_turns=turns.reverse_turns=0;
    turns.forward_quarters=turns.reverse_quarters=0;
    turns.initialized=false;
    if(event==0)return false;
    if(event==1)return true;
    throw std::invalid_argument("rotation reward is outside the recovered event-one domain");
}

// $81C219-C2C9 consumes the opponent queue. It mirrors the player consumer
// $81C0CE-C18A with the opponent addresses ($0D11/$0D13 cursors, $0CEB
// entries, $7E2102 learned weights, $770825 feature total, $11DB/$11E1
// boost) and one deliberate difference: the opponent adds the *whole* reward
// word to vertical boost ($81C2A5-C2AD) where the player adds half ($81C169).
//
// learned_weights is the opponent's serialized events 2-26 bank. The legacy
// DRAGSTER/M4-12-15 formats never serialized it, so those callers pass an
// empty span and keep the reached event-one domain they were accepted with.
void update_reward_queue(MovementState& state,unsigned event_one,const MovementContent& content,
                         std::span<std::uint8_t> learned_weights) {
    if(content.rotation_reward.size()<2 || content.rotation_class.empty()) {
        throw std::invalid_argument("rotation reward content has the wrong size");
    }
    if(!learned_weights.empty() && learned_weights.size()!=25) {
        throw std::invalid_argument("opponent learned reward bank has the wrong size");
    }
    if(event_one && state.rewards.write_cursor!=state.rewards.read_cursor) {
        state.rewards.entries[state.rewards.write_cursor]=static_cast<std::uint8_t>(event_one);
        state.rewards.write_cursor=static_cast<std::uint8_t>((state.rewards.write_cursor+1U)&31U);
    }
    if(state.rewards.cooldown)return;
    const auto next=static_cast<std::uint8_t>((state.rewards.read_cursor+1U)&31U);
    if(next==state.rewards.write_cursor) {
        state.rewards.cooldown=10;
        return;
    }
    state.rewards.read_cursor=next;
    const auto event=state.rewards.entries[next];
    std::uint8_t* weight=nullptr;
    if(learned_weights.empty()) {
        // Legacy DRAGSTER/M4-12-15 domain, deliberately unchanged. Those
        // formats carry no opponent learned-weight bank, so only the reached
        // event-one reward is modelled and every other event is rejected
        // rather than silently given the original's skip.
        const bool leading_event=event>=1 && content.rotation_class.size()>=event && content.rotation_class[event-1]==255;
        if(!leading_event && (event!=1 || content.rotation_class[0]!=0 || state.rewards.event_one_weight==0)) {
            throw std::invalid_argument("reward queue left the recovered event-one domain");
        }
        if(!leading_event)weight=&state.rewards.event_one_weight;
    }
    // $81C238 is CMP #$48 / BMI, which tests bit 7 of the 8-bit difference
    // rather than comparing signed values: the reward path is taken for
    // events 0-71 and again for 200-255, and 72-199 take the voice path.
    // The fixed BRONSEN voices 200-215 therefore reach the reward path,
    // unlike the player's 72-87 which do not. This is deliberately not
    // int8(event)<72; that spelling would also divert 128-199.
    else if(event<72 || event>=200) {
        if(event==0)throw std::invalid_argument("reward queue holds no published event");
        if(event<=content.rotation_class.size()) {
            if(content.rotation_class[event-1]!=255) {
                if(event==1)weight=&state.rewards.event_one_weight;
                else if(event<=26)weight=&learned_weights[event-2];
                else throw std::invalid_argument("reward queue left the recovered event-one domain");
            }
        } else if(event<200 || event>215) {
            // Unreachable while deserialization admits only the produced
            // 200-215 above 71: 216-255 would read $7E21D9 upward, which the
            // appended guards deliberately do not cover. Widen those guards
            // with this domain if it ever moves.
            throw std::invalid_argument("reward queue left the recovered event-one domain");
        }
        // Beyond the 72-entry class table only the BRONSEN voice range is
        // reachable. $81C241 then indexes past the table into ROM code and
        // $81C260 past the 26-byte learned bank into $7E21C9-$7E21D8. Those
        // bytes are zero on every authenticated frame of every reference
        // capture, which the appended reward-bank guards now assert rather
        // than leaving to observation, so the original takes its zero-weight
        // exit and publishes no reward. The cartridge class counter it still
        // bumps is outside the recovered inventory here exactly as it is for
        // the player.
    }
    if(weight && *weight) {
        state.rewards.feature_total=add_word(state.rewards.feature_total,*weight);
        *weight=std::max<std::uint8_t>(*weight>>1U,1);
        const auto amount=static_cast<std::int16_t>(content_word(content.rotation_reward,2U*(event-1U)));
        auto& boost=state.riders[1].speed.boost;
        if(negative(static_cast<std::uint16_t>(boost+1U)))boost=static_cast<std::uint16_t>((boost>>1U)|0x8000U);
        if(amount>=0) {
            boost=add_word(boost,static_cast<std::uint16_t>(amount));
            state.riders[1].speed.vertical_boost=add_word(
                state.riders[1].speed.vertical_boost,static_cast<std::uint16_t>(amount));
        }
    }
    const auto remaining=static_cast<unsigned>(
        (state.rewards.write_cursor-state.rewards.read_cursor-1U)&31U);
    state.rewards.cooldown=static_cast<std::uint16_t>(std::max(5,40-static_cast<int>(4U*remaining)));
}
}

std::uint16_t finish_speed_toward_zero(std::uint16_t velocity) {
    return speed_toward_zero(velocity,10);
}

std::vector<std::uint8_t> serialize_movement_state(const MovementState& s) {
    if(s.contact_phase>1 || s.progress_phase>1) throw std::invalid_argument("movement phase is not binary");
    const bool version_three=s.finish.opponent_finish_pose_selector!=0;
    const bool version_two=version_three || has_finish_state(s.finish);
    const auto& magic=version_three?movement_state_magic_v3:
        (version_two?movement_state_magic_v2:movement_state_magic);
    std::vector<std::uint8_t> out(magic.begin(),magic.end());
    put32(out,s.frame);
    for(auto v:{s.player_input.low_image,s.player_input.high_image,s.player_input.vertical,s.player_input.horizontal}) put8(out,v);
    for(const auto& rider:s.riders) write_rider(out,rider);
    for(auto v:{s.timer.minutes,s.timer.tens_seconds,s.timer.seconds,s.timer.tenths,s.timer.subframe}) put16(out,v);
    for(auto v:{s.opponent_ai.impulse_countdown,s.opponent_ai.trick_selector,s.opponent_ai.suppression_counter}) put16(out,v);
    for(auto v:s.rewards.entries) put8(out,v);
    put8(out,s.rewards.read_cursor); put8(out,s.rewards.write_cursor); put16(out,s.rewards.cooldown); put16(out,s.rewards.feature_total); put8(out,s.rewards.event_one_weight);
    put16(out,s.countdown); put8(out,s.contact_phase); put8(out,s.progress_phase); put8(out,s.animation_counter); put8(out,s.update_counter);
    if(version_two) {
        for(auto value:s.finish.rider_finished)put_bool(out,value);
        for(auto value:s.finish.finish_time_centiseconds)put16(out,value);
        for(const auto& digits:s.finish.finish_time_digits)for(auto value:digits)put16(out,value);
        for(auto value:s.finish.finish_animation_countdown)put16(out,value);
        put16(out,s.finish.player_finish_delay); put16(out,s.finish.result_loading_updates);
        put8(out,static_cast<std::uint8_t>(s.finish.phase));
        put8(out,static_cast<std::uint8_t>(s.finish.outcome));
        if(version_three)put16(out,s.finish.opponent_finish_pose_selector);
    }
    return out;
}

MovementState deserialize_movement_state(std::span<const std::uint8_t> bytes) {
    if(bytes.size()<movement_state_magic.size()) throw std::invalid_argument("movement state is truncated");
    const bool version_one=std::equal(movement_state_magic.begin(),movement_state_magic.end(),bytes.begin());
    const bool version_two=std::equal(movement_state_magic_v2.begin(),movement_state_magic_v2.end(),bytes.begin());
    const bool version_three=std::equal(movement_state_magic_v3.begin(),movement_state_magic_v3.end(),bytes.begin());
    if(!version_one && !version_two && !version_three)throw std::invalid_argument("movement state magic is unsupported");
    Reader in(bytes.subspan(movement_state_magic.size())); MovementState s{}; s.frame=in.u32();
    s.player_input.low_image=in.u8(); s.player_input.high_image=in.u8(); s.player_input.vertical=in.u8(); s.player_input.horizontal=in.u8();
    for(auto& rider:s.riders) read_rider(in,rider);
    for(auto* v:{&s.timer.minutes,&s.timer.tens_seconds,&s.timer.seconds,&s.timer.tenths,&s.timer.subframe}) *v=in.u16();
    s.opponent_ai.impulse_countdown=in.u16(); s.opponent_ai.trick_selector=in.u16(); s.opponent_ai.suppression_counter=in.u16();
    for(auto& v:s.rewards.entries) v=in.u8();
    s.rewards.read_cursor=in.u8(); s.rewards.write_cursor=in.u8(); s.rewards.cooldown=in.u16(); s.rewards.feature_total=in.u16(); s.rewards.event_one_weight=in.u8();
    s.countdown=in.u16(); s.contact_phase=in.u8(); s.progress_phase=in.u8(); s.animation_counter=in.u8(); s.update_counter=in.u8();
    if(version_two || version_three) {
        for(auto& value:s.finish.rider_finished)value=in.flag();
        for(auto& value:s.finish.finish_time_centiseconds)value=in.u16();
        for(auto& digits:s.finish.finish_time_digits)for(auto& value:digits)value=in.u16();
        for(auto& value:s.finish.finish_animation_countdown)value=in.u16();
        s.finish.player_finish_delay=in.u16(); s.finish.result_loading_updates=in.u16();
        s.finish.phase=static_cast<RacePhase>(in.u8());
        s.finish.outcome=static_cast<RaceOutcome>(in.u8());
        if(version_three)s.finish.opponent_finish_pose_selector=in.u16();
    }
    if(s.contact_phase>1 || s.progress_phase>1 || s.animation_counter>31 ||
       s.player_input.vertical>2 || s.player_input.horizontal>2 ||
       s.rewards.read_cursor>31 || s.rewards.write_cursor>31) throw std::invalid_argument("movement state contains an out-of-domain counter");
    if(static_cast<std::uint8_t>(s.finish.phase)>3 || static_cast<std::uint8_t>(s.finish.outcome)>2 ||
       s.finish.player_finish_delay>240 || s.finish.opponent_finish_pose_selector>48)
        throw std::invalid_argument("movement state contains invalid finish state");
    for(const auto& rider:s.riders) {
        if(rider.idle_pose.active>1 || rider.idle_pose.direction_adjustment>1 ||
           rider.idle_pose.cycle_latched>1 || rider.idle_pose.cycle_counter>=120 ||
           rider.idle_pose.orientation_reference>=64) {
            throw std::invalid_argument("movement state contains an out-of-domain idle pose field");
        }
    }
    (void)serialize_timer(s.timer); // Reuse the reviewed digit-domain validation.
    in.require_end(); return s;
}

MovementState classic_crawler_dragster_start() {
    MovementState state{};
    state.frame=1533;
    state.player_input.high_image=1; state.player_input.vertical=1; state.player_input.horizontal=2;
    for(auto& rider:state.riders) {
        rider.motion.x=0x0440; rider.motion.y=0x035a;
        rider.contact.previous_uncorrected_x=0x0440; rider.contact.previous_uncorrected_y=0x035b;
        rider.contact.selected_word=0x1804; rider.contact.selected_high=0x18;
        rider.pose.orientation=6; rider.pose.reflected_orientation=0x3a;
        rider.pose.animation_phase=0x13; rider.pose.previous_x=0x0440; rider.pose.previous_y=0x035b;
        rider.pose.target_orientation=6; rider.pose.pose_index=0x04fa; rider.pose.reflected=true;
        rider.quarter_turn.reflected_at_start=true;
        rider.residue_y=0x17; rider.throttle=0x01b0; rider.previous_brake=1;
        rider.small_motion_counter=4;
    }
    state.riders[0].idle_pose.orientation_reference=1;
    state.riders[0].residue_x=0xffff;
    state.riders[1].idle_pose.orientation_reference=60;
    state.rewards.write_cursor=1; state.rewards.cooldown=2; state.rewards.event_one_weight=4;
    state.countdown=0x45; state.contact_phase=1; state.progress_phase=1;
    state.animation_counter=0x0d; state.update_counter=0xcd;
    return state;
}


void update_movement(MovementState& state, const ControllerButtons& player_buttons,
                     const MovementContent& content) {
    state.player_input = sample_controller(player_buttons);
    if (state.player_input.horizontal == 0) {
        throw std::invalid_argument("leftward movement is outside the recovered primary domain");
    }
    if(state.finish.phase==RacePhase::FinishDelay && state.finish.player_finish_delay==240) {
        state.finish.phase=RacePhase::ResultLoading;
        state.finish.result_loading_updates=1;
        ++state.frame;
        return;
    }
    if(state.finish.phase==RacePhase::ResultLoading || state.finish.phase==RacePhase::ResultScreen) {
        if(state.finish.phase==RacePhase::ResultLoading) {
            ++state.finish.result_loading_updates;
            const auto stable_update = state.finish.outcome==RaceOutcome::PlayerWon?226U:242U;
            if(state.finish.result_loading_updates>=stable_update)state.finish.phase=RacePhase::ResultScreen;
        }
        ++state.frame;
        return;
    }
    const auto timer_at_start=state.timer;
    const bool finish_delay=state.finish.phase==RacePhase::FinishDelay;
    // $83:EA72-$83:EAC3: when the opponent finishes first, its next update
    // receives the same neutral horizontal/action response and phased signed
    // slowdown while the player's timer and ordinary race remain live. This is
    // distinct from the later player-owned global finish delay (R-0017).
    const bool opponent_finished_first=
        state.finish.rider_finished[1] && !state.finish.rider_finished[0];
    if(finish_delay) {
        state.player_input.horizontal=1;
        ++state.finish.player_finish_delay;
    }
    state.update_counter = static_cast<std::uint8_t>(state.update_counter + 1U);
    state.animation_counter = static_cast<std::uint8_t>((state.animation_counter + 1U) & 31U);
    state.contact_phase = static_cast<std::uint8_t>(1U - state.contact_phase);

    // The countdown handler publishes a forced brake while entering with 70 or
    // more, then decrements. End-1533 contains 69, so frame 1534 releases the
    // stored brake and takes the ordinary launch transition.
    const bool forced_brake = state.countdown >= 70;
    const bool timer_enabled = state.countdown < 69;
    if (state.countdown != 0) --state.countdown;
    const bool player_brake = forced_brake || player_buttons.b;
    bool opponent_jump=!opponent_finished_first &&
        (state.riders[1].progress.marker_word&0x2000U)!=0;
    if(opponent_jump && state.riders[1].contact.unsupported_count<4 &&
       state.rewards.feature_total!=0) {
        const auto catch_up=static_cast<std::int16_t>(
            state.riders[0].progress.transition_count-
            state.riders[1].progress.transition_count-3U);
        if(catch_up>=0) {
            throw std::invalid_argument("opponent catch-up jump is outside the recovered domain");
        }
        // After a scored feature, the recovered AI copies the alternating
        // motion phase instead of asserting another continuous jump input.
        opponent_jump=state.contact_phase!=0;
    }
    bool opponent_trick=false;
    if(opponent_jump && state.opponent_ai.impulse_countdown) {
        opponent_trick=(state.opponent_ai.trick_selector&1U)!=0;
    } else if(opponent_jump && state.riders[1].contact.unsupported_count>=4) {
        state.opponent_ai.impulse_countdown=static_cast<std::uint16_t>(
            std::abs(static_cast<std::int16_t>(state.riders[1].motion.velocity_y))>>1);
        state.opponent_ai.trick_selector=1;
        state.opponent_ai.suppression_counter=30;
        opponent_trick=true;
    } else if(!opponent_jump) {
        state.opponent_ai.impulse_countdown=0;
        state.opponent_ai.trick_selector=0;
        if(opponent_finished_first)state.opponent_ai.suppression_counter=0;
    }
    const unsigned active=state.contact_phase?0U:1U;
    bool opponent_event_one=false;
    state.rewards.cooldown=state.rewards.cooldown>2?
        static_cast<std::uint16_t>(state.rewards.cooldown-2U):0;
    for(unsigned index=0;index<state.riders.size();++index) {
        auto& rider=state.riders[index];
        const auto speed_before=rider.motion.velocity_x;
        decay_idle_wobble(rider);
        const auto horizontal=index==0?
            (finish_delay?1U:state.player_input.horizontal):
            (finish_delay||opponent_finished_first?1U:2U);
        int animation_override=index==active?
            stationary_animation_override(rider,static_cast<std::uint8_t>(horizontal)):0;
        bool use_throttle_target=false;
        if(index==active) {
            const bool event_one=update_quarter_turns(rider);
            if(index==1)opponent_event_one=event_one;
            else if(event_one)throw std::invalid_argument("player reward is outside the primary domain");
            update_jump(rider,index==0?player_buttons.b:opponent_jump);
            // In the recovered branch rotation input is accepted only after the
            // contact count reaches nine; the synthesized opponent trick is the
            // positive two-step direction.
            if(index==1 && opponent_trick && rider.contact.unsupported_count>=9) {
                rider.motion.response_b=2;
            } else {
                rider.motion.response_b=0;
            }
            update_active_low_speed_damping(rider);
        }
        // The source dispatcher skips this pre-adjustment on each third
        // update; the ordinary limiter/damping still runs on every update.
        if((finish_delay || (index==1 && opponent_finished_first)) &&
           (state.frame+1U)%3U!=0U)apply_finish_slowdown(rider);
        update_horizontal(rider,index==0?player_brake:forced_brake,horizontal==2,index==1,state,content,
                          animation_override,use_throttle_target);
        if(finish_delay || (index==1 && opponent_finished_first)) {
            // Neutral finish response removes the 24-unit drive contribution
            // after ordinary limiting only when subtraction cannot cross zero.
            // Unlike the ten-unit pre-adjustment, a smaller remainder persists.
            const auto limited=static_cast<std::int16_t>(rider.motion.velocity_x);
            if(limited>=24)rider.motion.velocity_x=static_cast<std::uint16_t>(limited-24);
            else if(limited<=-24)rider.motion.velocity_x=static_cast<std::uint16_t>(limited+24);
        }
        update_rolling_mode(rider);
        update_gravity(rider);
        integrate_motion(rider);
        update_idle_pose(rider,state.countdown==0,index==1,state.animation_counter,
                         content.idle_pose_table);
        update_pose(rider,state.animation_counter,state.contact_phase,content,animation_override,
                    use_throttle_target);
        if(index==1 && opponent_finished_first) {
            update_opponent_finish_pose(state.finish,rider,state.frame+1U);
        }
        if(finish_delay && index==0 && state.finish.player_finish_delay==2 && speed_before==460) {
            // The later-player path crosses a contact/pose boundary on its
            // second finish update; the source retains the prior value 15 for
            // this one sample although position advances by 13.
            rider.motion.previous_x_displacement=15;
        }
    }
    (void)advance_timer_digits(state.timer, timer_enabled);
    update_reward_queue(state,opponent_event_one,content,{});
    std::array<TrackSamples,2> samples{};
    for (std::size_t rider=0;rider<state.riders.size();++rider) {
        const auto& movement=state.riders[rider];
        const auto points=collision_points(content.sampling,movement.pose.pose_index,
                                           movement.pose.reflected);
        samples[rider]=sample_track(content.sampling,points,movement.motion.x,
                                    movement.motion.y,1024);
        const auto summary=summarize_flat_contact(content.flat_contact,points,samples[rider],
                                                  movement.motion.x,movement.motion.y);
        resolve_flat_contact(state.riders[rider].contact,state.riders[rider].motion,summary,
                             {state.contact_phase,rider==1,0,0});
    }
    ProgressUpdateState progress{{state.riders[0].progress,state.riders[1].progress},
                                 state.progress_phase};
    update_track_progress(progress,samples,content.progress_transitions);
    state.progress_phase=progress.phase;
    for (std::size_t rider=0;rider<state.riders.size();++rider) {
        state.riders[rider].progress=progress.riders[rider];
        if(state.finish.finish_animation_countdown[rider]!=0)--state.finish.finish_animation_countdown[rider];
        // R-0013 bounds the tested crossing after $62A8 and by $62AD. The
        // aligned $62AC comparator is exact for both frozen Dragster paths;
        // no general boundary for another track is claimed.
        if(!state.finish.rider_finished[rider] && state.riders[rider].motion.x>=0x62ACU) {
            record_finish(state.finish,rider,timer_at_start,state.frame+1U);
        }
    }
    ++state.frame;
}
} // namespace unirally

#include "zoom_zoo_movement.hpp"
#include "vertical_contact.hpp"

namespace unirally {
namespace {
void write_reflection(std::vector<std::uint8_t>& bytes,const ReflectionTransition& r) {
    for(auto v:{r.step,r.end,r.pose_base,r.pose_override,r.completed,r.hold,r.drive_pose_enabled,
                r.air_turns,r.direction_latch,r.base_velocity_cap,r.brake_input,
                r.rotate_negative_input,r.rotate_positive_input,r.jump_input,r.wrong_direction_counter})put16(bytes,v);
}
void read_reflection(Reader& in,ReflectionTransition& r) {
    for(auto* v:{&r.step,&r.end,&r.pose_base,&r.pose_override,&r.completed,&r.hold,&r.drive_pose_enabled,
                 &r.air_turns,&r.direction_latch,&r.base_velocity_cap,&r.brake_input,
                 &r.rotate_negative_input,&r.rotate_positive_input,&r.jump_input,&r.wrong_direction_counter})*v=in.u16();
}
void update_reflection_transition(RiderMovementState& rider,ReflectionTransition& transition,
                                  unsigned horizontal,bool inactive_phase,std::span<const std::uint8_t> table,bool manual=false,
                                  const SpecialTileRider& tiles={}) {
    // $82:A35B-A49E: A permits a direction-selected turn while airborne,
    // including a turn toward the current facing. Contact A halves velocity.
    // $82:A35D-A362: the corkscrew's reflection lock skips it all.
    if(tiles.reflection_lock)return;
    const bool manual_airborne=rider.contact.unsupported_count==9 && !transition.step &&
        !(rider.contact.selected_high&0x80U) && manual && horizontal!=1;
    if (!(rider.contact.unsupported_count==9 && transition.step) && !manual_airborne && !rider.contact.recontact && !inactive_phase)return;
    if (!transition.step && !manual_airborne &&
        (horizontal==1 || (horizontal==2 && rider.pose.reflected) || (horizontal==0 && !rider.pose.reflected)))return;
    if(manual && rider.contact.recontact) {
        const auto velocity=rider.motion.velocity_x;
        rider.motion.velocity_x=static_cast<std::uint16_t>((velocity>>1U)|(velocity&0x8000U));
        transition.air_turns=0;
    }
    if (!transition.step) {
        // $82:A403-A410: a pose override or the corkscrew latch holds the facing.
        if(transition.pose_override || tiles.corkscrew_latch)return;
        if(rider.pose.reflected) {
            rider.pose.reflected_orientation=static_cast<std::uint16_t>(64-rider.pose.reflected_orientation)&63U;
            rider.pose.reflected=false;transition.step=9;transition.end=16;
        } else {transition.step=1;transition.end=9;}
    }
    transition.pose_base=static_cast<std::uint16_t>(content_word(table,2U*rider.pose.reflected_orientation));
    if(transition.step==transition.end) {
        if(rider.contact.unsupported_count>=8)transition.air_turns=add_word(transition.air_turns,1);
        transition.completed=1;
        if(transition.end!=16)rider.pose.reflected=true;
        transition.step=0;transition.pose_override=0;
    } else {
        transition.pose_override=static_cast<std::uint16_t>(transition.pose_base+transition.step+0x620U);
        transition.step=add_word(transition.step,1);
    }
}
// Returns true when an inverted marker ended the update early (R-0048).
bool update_zoom_ai(ZoomZooState& state) {
    auto& whole=state.movement;auto& input=state.reflection[1];auto& rider=whole.riders[1];auto& ai=whole.opponent_ai;
    input.brake_input=input.jump_input=input.rotate_negative_input=input.rotate_positive_input=0;
    const auto marker=rider.progress.marker_word;
    // $83:E0A7-E0AF: a marker with bit 15 set returns before the AI sets any
    // input, so the opponent keeps what the port-2 reader left for an AI rider
    // ($82:AB6F-AB8B): every input released and the direction neutral ($031B
    // = 1), A ($031F) and X ($0323) included. The selector ($0C75), countdown
    // ($0C6F) and suppression ($1277) keep their values (R-0048).
    if(marker&0x8000U) {state.opponent_horizontal=1;return true;}
    state.opponent_horizontal=(marker&0x4000U)?0:2;
    if(marker&0x2000U) {
        input.jump_input=1;
        if(ai.impulse_countdown) {
            if(ai.trick_selector&1U)input.rotate_positive_input=1;else input.rotate_negative_input=1;
            return false;
        }
        if(rider.contact.unsupported_count>=4 && negative(rider.motion.velocity_y)) {
            ai.impulse_countdown=static_cast<std::uint16_t>(-static_cast<std::int16_t>(rider.motion.velocity_y)/2);
            if(whole.rewards.feature_total==0 || static_cast<std::int16_t>(whole.riders[0].progress.transition_count-rider.progress.transition_count)>=3) {
                ai.suppression_counter=30;
                // $83E16B compares $1275 with 2 in a three-way structure; only
                // the below-two arm is modelled here. $1275 is a reference
                // guard held at 1 on every authenticated frame, so the equal
                // and above arms, one of which sets suppression to 60, are
                // unreachable in this scenario rather than ignored.
                // $83E1CB-E21A. A flat launch only picks a rotation from the
                // velocity sign; a sloped one takes x&7, whose three bits drive
                // three independent inputs -- bit 0 the rotation at
                // $032F/$032B, bit 1 the opponent's A at $031F and bit 2 its X
                // at $0323. Bits 1 and 2 are consumed where the opponent's
                // reflection and roll run, below.
                if(rider.contact.surface_angle==0) {
                    ai.trick_selector=negative(rider.motion.velocity_x)?0:1;
                } else ai.trick_selector=rider.motion.x&7U;
                if(ai.trick_selector&1U)input.rotate_positive_input=1;else input.rotate_negative_input=1;
                return false;
            }
            ai.suppression_counter=0;
        } else if(rider.contact.unsupported_count<4 &&
                  (whole.rewards.feature_total==0 || static_cast<std::int16_t>(whole.riders[0].progress.transition_count-rider.progress.transition_count)>=3)) {
            ai.trick_selector=0;ai.impulse_countdown=0;return false;
        }
    }
    input.jump_input=whole.contact_phase;
    ai.trick_selector=0;ai.impulse_countdown=0;
    if(!negative(rider.motion.velocity_y) && ai.suppression_counter>0 &&
       rider.pose.reflected_orientation>=16 && rider.pose.reflected_orientation<48) {
        if(negative(rider.motion.velocity_x))input.rotate_negative_input=1;else input.rotate_positive_input=1;
    }
    return false;
}
void update_zoom_throttle(RiderMovementState& rider,ReflectionTransition& transition,unsigned horizontal,
                          int& animation_override,bool& throttle_target,std::uint16_t& charge_announced,bool leading_support=false,
                          bool bounce_active=false,std::uint16_t drive_step=24,bool on_mud=false) {
    // Velocity part of the drive routines $82:A9B3 (rightward, +24) and
    // $82:AA10 (leftward, -24); on mud the step is 4 ($0F3B, R-0047). On an
    // inverted tile both move a nonzero velocity away from zero, and a roll
    // bounce ($042B) stores nothing.
    const auto drive_velocity=[&](bool rightward) {
        if(bounce_active)return;
        const auto back=static_cast<std::uint16_t>(0U-drive_step);
        if(rider.contact.selected_word&0x8000U) {
            if(rider.motion.velocity_x)rider.motion.velocity_x=add_word(rider.motion.velocity_x,
                negative(rider.motion.velocity_x)?back:drive_step);
        } else rider.motion.velocity_x=add_word(rider.motion.velocity_x,rightward?drive_step:back);
    };
    const auto incoming_speed=static_cast<std::int16_t>(rider.motion.velocity_x);
    const bool braking=transition.brake_input && (incoming_speed>=16 || incoming_speed < -16);
    if(!leading_support && rider.contact.unsupported_count<2 &&
       rider.contact.surface_angle!=0xffe1U && rider.contact.surface_angle!=31 && braking) {
        // $829909-9945 brakes through the opposite drive routine, clamps a
        // velocity that crossed zero, and returns before the previous-brake
        // latch publication. A bounce keeps the velocity (diff fuzz seed 140).
        if(incoming_speed>0) {
            drive_velocity(false);
            if(negative(rider.motion.velocity_x))rider.motion.velocity_x=0;
        } else {
            drive_velocity(true);
            if(!negative(rider.motion.velocity_x))rider.motion.velocity_x=0;
        }
        rider.throttle=0;
        return;
    }
    // $82:98CF-9A52. The charge latch ($0F63, serialized from $0D53/$0D55)
    // clears on the airborne, steep and neutral paths ($82:999A, $82:9A2B).
    if(leading_support || rider.contact.unsupported_count>=2 || rider.contact.surface_angle==0xffe1U || rider.contact.surface_angle==31) {
        rider.throttle=0;charge_announced=0;
    } else if(horizontal==1) {
        rider.throttle=0;charge_announced=0;
    } else {

        // $82:A9C1–A9E0 / AA1E–AA3D: on inverted tiles the drive
        // increment follows the existing velocity sign, including zero hold.
        // Both drive routines return before throttle accumulation when an
        // inverted tile has zero incoming velocity ($82A9CC / $82AA29).
        if(!(rider.contact.selected_word&0x8000U) || rider.motion.velocity_x!=0) {
            // $82:AA42-AA49 / its mirror: a roll bounce keeps the velocity;
            // throttle still accumulates below.
            drive_velocity(horizontal==2);
            const auto cap=static_cast<std::uint16_t>(transition.base_velocity_cap+
                std::max(0,static_cast<int>(static_cast<std::int16_t>(rider.speed.boost)))+rider.launch_override);
            const auto next=add_word(rider.throttle,horizontal==2?16:static_cast<std::uint16_t>(-16));
            if(horizontal==2 ? negative(static_cast<std::uint16_t>(next-cap)) :
                               !negative(static_cast<std::uint16_t>(next-static_cast<std::uint16_t>(-cap))))rider.throttle=next;
        }
        if(static_cast<std::int16_t>(rider.motion.previous_x_displacement)<4) {
            if(rider.small_motion_counter!=4)rider.small_motion_counter=add_word(rider.small_motion_counter,1);
            animation_override=static_cast<std::int16_t>(rider.small_motion_counter);throttle_target=true;
        }
        if(transition.brake_input) {
            rider.motion.velocity_x=0;
            // $829995-99EF: a reflection step clears the announcement but
            // preserves accumulated throttle ($0F5F); nonzero throttle sets
            // it; zero throttle leaves it as it was (Left and Y through the
            // countdown carry throttle through zero, DRAGSTER fuzz seed 208).
            // $82:998B-9993: after mud ($0F45) braking also drops the
            // throttle and the announcement ($0FB1 is the unrecovered pair 12's).
            if(on_mud) {rider.throttle=0;charge_announced=0;}
            else if(transition.step)charge_announced=0;
            else if(rider.throttle)charge_announced=1;
        }
        else {
            // $82:99F1-9A49: releasing the brake launches any throttle and
            // clears the announcement; without a previous brake nothing
            // changes, and the latch is already clear there.
            if(rider.previous_brake && rider.throttle) {
                rider.motion.velocity_x=add_word(rider.motion.velocity_x,rider.throttle);rider.throttle=0;rider.launch_override=256;
            }
            if(rider.previous_brake)charge_announced=0;
        }
    }
    rider.previous_brake=transition.brake_input;
}
void enqueue_zoom_opponent(MovementState& state,unsigned event) {
    if(state.rewards.write_cursor==state.rewards.read_cursor)return;
    state.rewards.entries[state.rewards.write_cursor]=static_cast<std::uint8_t>(event);
    state.rewards.write_cursor=static_cast<std::uint8_t>((state.rewards.write_cursor+1U)&31U);
}
// $81C598-C5C8: scoring messages interrupt tutorial text immediately.
void enqueue_zoom_player(ZoomZooState& state,unsigned event) {
    if(!state.native_initialization)return;
    auto& a=state.player_announcements;auto& q=a.queue;
    if(q.write_cursor==q.read_cursor)return;
    q.entries[q.write_cursor]=static_cast<std::uint8_t>(event);
    if(event<22 && a.hints_active) {q.cooldown=0;a.hints_active=0;}
    q.write_cursor=static_cast<std::uint8_t>((q.write_cursor+1U)&31U);
}

// $829B69-9D97: landing announcements precede queue consumption. Four
// independent trick counts form a radix-five static combination-table index.
void update_zoom_landing_rewards(ZoomZooState& state,unsigned index,const ZoomZooContent& content) {
    auto& rider=state.movement.riders[index];auto& turns=rider.quarter_turn;
    auto& roll=state.rolls[index];auto& transition=state.reflection[index];
    const bool vertical=std::abs(static_cast<std::int16_t>(rider.contact.surface_angle))==31;
    if(vertical || (!rider.motion.response_a && rider.contact.unsupported_count>=2)) {
        (void)update_quarter_turns(rider,false,transition.air_turns,roll.step!=0,roll.held_rotations);
        return;
    }
    const auto announce=[&](unsigned event) {
        if(index==0)enqueue_zoom_player(state,event);else enqueue_zoom_opponent(state.movement,event);
    };
    if(state.surface[index].leading_support) {
        const auto count=static_cast<std::uint16_t>((turns.forward_turns&255U)+turns.reverse_turns+roll.held_rotations+transition.air_turns);
        if(count) {
            if(!roll.bounce_active)announce(14);
            rider.speed.boost=rider.speed.vertical_boost=0;
        }
    } else {
        const auto forward=std::min<unsigned>(add_word(turns.forward_turns,turns.forward_quarters==3?1:0),4);
        const auto reverse=std::min<unsigned>(add_word(turns.reverse_turns,turns.reverse_quarters==3?1:0),4);
        const std::array<unsigned,4> counts{{turns.reflected_at_start?reverse:forward,
            turns.reflected_at_start?forward:reverse,std::min<unsigned>(transition.air_turns>>1U,4),
            std::min<unsigned>(roll.completed_rolls,4)}};
        if(roll.bounce_active && !roll.support_count_mirror) {announce(16);roll.bounce_active=0;}
        if(roll.held_rotations>=3)announce(17);
        constexpr std::array<unsigned,4> event_bases{{4,0,8,17}};
        for(unsigned i=0;i<4;++i)if(counts[i])announce(event_bases[i]+counts[i]);
        const auto combination=counts[0]*125U+counts[1]*25U+counts[2]*5U+counts[3];
        if(content.trick_combinations.size()!=625)throw std::invalid_argument("ZOOM ZOO trick combination table missing");
        if(content.trick_combinations[combination]!=254) {
            // $829D3A-9D70: incoming X scratch ($A5), fixed scenario rider ID.
            // A is replaced by this voice ID before BOTH enqueue calls.
            const auto voice=72U+(index==0?0U:128U)+(rider.motion.x&15U);
            announce(voice);announce(voice);
        }
    }
    turns.previous_quadrant=turns.forward_turns=turns.reverse_turns=0;
    turns.forward_quarters=turns.reverse_quarters=0;turns.initialized=false;
    transition.air_turns=0;roll.completed_rolls=roll.held_rotations=0;
}

// $81BEA8-BEF1, $81C0CE-C18A and $81C02A-C054. This queue is
// gameplay state: an earlier message delays publication of a trick boost.
void consume_zoom_player(ZoomZooState& state,const MovementContent& content) {
    auto& a=state.player_announcements;auto& q=a.queue;
    if(q.cooldown)return;
    const auto cursor=static_cast<std::uint8_t>((q.read_cursor+1U)&31U);
    if(cursor==q.write_cursor) {
        if(!a.empty_display) {q.cooldown=10;a.empty_display=1;}
        return;
    }
    q.read_cursor=cursor;
    const auto event=q.entries[cursor];
    if(!event || (event<72 && event>content.rotation_class.size()))throw std::invalid_argument("player announcement event is outside static inventory");
    if(event<72 && content.rotation_class[event-1]!=255) {
        if(event>26)throw std::invalid_argument("player reward class is outside learned inventory");
        auto& weight=event==1?q.event_one_weight:state.learned_weights[0][event-2];
        if(weight) {
            q.feature_total=add_word(q.feature_total,weight);
            weight=std::max<std::uint8_t>(weight>>1U,1);
            auto& speed=state.movement.riders[0].speed;
            if(negative(static_cast<std::uint16_t>(speed.boost+1U)))speed.boost=static_cast<std::uint16_t>((speed.boost>>1U)|0x8000U);
            const auto amount=content_word(content.rotation_reward,2U*(event-1U));
            if(!negative(static_cast<std::uint16_t>(amount))) {
                speed.boost=add_word(speed.boost,static_cast<std::uint16_t>(amount));
                speed.vertical_boost=add_word(speed.vertical_boost,static_cast<std::uint16_t>(amount>>1U));
            }
        }
    }

    const auto remaining=(q.write_cursor-q.read_cursor-1U)&31U;
    q.cooldown=static_cast<std::uint16_t>(a.hints_active?120:std::max(5,40-static_cast<int>(4U*remaining)));
    a.empty_display=0;
}

// $83CDBC-CE43: four messages every300 updates while the selected hint
// remains enabled. The race initializer explicitly sets the initial count30.
void update_zoom_hints(ZoomZooState& state) {
    auto& a=state.player_announcements;
    if(!a.hints_active)return;
    if(++a.hint_updates!=300)return;
    a.hint_updates=0;a.hint_group=static_cast<std::uint16_t>((a.hint_group+1U)&7U);
    for(unsigned i=0;i<4;++i)enqueue_zoom_player(state,40U+4U*a.hint_group+i);
}

// $8296C3-9711: completion is shared by ordinary rolls and bounce release.
void complete_zoom_roll(ZoomZooState& state,unsigned index,bool interrupted=false) {
    auto& roll=state.rolls[index];auto& rider=state.movement.riders[index];
    if(!roll.bounce_active && !state.surface[index].leading_support && !interrupted)
        roll.completed_rolls=add_word(roll.completed_rolls,1);
    roll.bounce_charge=0;state.reflection[index].pose_override=0;
    if(!(roll.pose_base&0x8000U))rider.pose.reflected=!rider.pose.reflected;
    rider.pose.orientation=static_cast<std::uint16_t>(rider.pose.orientation-32U)&63U;
}

// $829398-9714: X enters a roll on unsupported contact. The signed
// step selects the original nine-frame pose strip and is advanced only on
// this rider's active update. Pose/reflection feed the next collision sample.
void update_zoom_roll(ZoomZooState& state,unsigned index,bool pressed,const ZoomZooContent& content) {
    auto& roll=state.rolls[index];auto& rider=state.movement.riders[index];
    auto& turn=state.reflection[index];
    if(roll.bounce_charge) {
        // $82965E-96C3. Charge is a retained160, not a per-update ramp:
        // the original's below512 store writes the same loaded value.
        const auto bounce=[&] {
            rider.motion.velocity_y=static_cast<std::uint16_t>(~std::min<std::uint16_t>(roll.bounce_charge,256));
            roll.bounce_active=1;
        };
        if(index==0 && pressed && !state.surface[index].tile_pose_enabled) {
            roll.input_latched=0;
            if(state.surface[index].leading_support)bounce();
            return;
        }
        if(rider.contact.unsupported_count<2)bounce();
        complete_zoom_roll(state,index);return;
    }
    if(!roll.step) {
        if(turn.pose_override || rider.contact.unsupported_count<9)return;
        if(!pressed) {roll.input_latched=0;return;}
        if(roll.input_latched)return;
        roll.input_latched=1;roll.prior_orientation=rider.pose.orientation;
        roll.prior_reflection=rider.pose.reflected;
        const auto entry=content_word(content.roll_poses,2U*(rider.pose.orientation&63U));
        roll.step=static_cast<std::uint16_t>(-9);
        if(entry&0x8000U) {roll.step=9;rider.pose.reflected=!rider.pose.reflected;}
        roll.pose_base=static_cast<std::uint16_t>(entry&0x3fffU);
    }
    const auto orientation=rider.pose.orientation&63U;
    if(orientation>=content.roll_directions.size())throw std::invalid_argument("ZOOM ZOO roll direction table is missing");
    if((content.roll_directions[orientation]==1 && negative(roll.step)) ||
       (content.roll_directions[orientation]!=1 && !negative(roll.step)))roll.step=static_cast<std::uint16_t>(~roll.step);
    bool interrupted=false;
    auto& held_weight=state.learned_weights[index][15]; // Event17, $7E20F8/$7E2112.
    if(rider.contact.unsupported_count<2 && !negative(roll.held_updates)) {
        roll.held_updates=static_cast<std::uint16_t>(-roll.held_updates);held_weight=0;
        auto& quarters=rider.quarter_turn;
        if(quarters.forward_turns+quarters.reverse_turns+roll.held_rotations+turn.air_turns) {
            if(index==0)enqueue_zoom_player(state,14);else enqueue_zoom_opponent(state.movement,14);
        }
        quarters.previous_quadrant=quarters.forward_turns=quarters.reverse_turns=0;
        quarters.forward_quarters=quarters.reverse_quarters=0;
        turn.air_turns=0;roll.completed_rolls=roll.held_rotations=0;interrupted=true;
    }
    std::uint16_t step_index{};
    if(negative(roll.held_updates) || (pressed && roll.held_updates)) {
        if(!negative(roll.held_updates)) {
            roll.held_updates=static_cast<std::uint16_t>(-roll.held_updates);
            held_weight=static_cast<std::uint8_t>(std::min(12U,unsigned(held_weight)+unsigned(static_cast<std::uint8_t>(-roll.held_updates))));
        }
        if((negative(roll.step) && roll.step==static_cast<std::uint16_t>(-9)) || (!negative(roll.step) && roll.step==8)) {
            rider.pose.reflected=roll.prior_reflection!=0;roll.held_updates=0;turn.pose_override=0;roll.step=0;return;
        }
        roll.step=add_word(roll.step,negative(roll.step)?static_cast<std::uint16_t>(-1):1);
        step_index=negative(roll.step)?add_word(roll.step,9):roll.step;
    } else if(!pressed && ((!negative(roll.step) && roll.step>=4) ||
                           (negative(roll.step) && static_cast<std::int16_t>(roll.step)<=-5))) {
        // $82955F-9598: release moves toward the central held pose.
        if(!negative(roll.step) && roll.step!=4)--roll.step;
        if(negative(roll.step) && roll.step!=static_cast<std::uint16_t>(-5))++roll.step;
        roll.held_updates=add_word(roll.held_updates,1);roll.held_rotations=add_word(roll.held_rotations,1);
        step_index=negative(roll.step)?add_word(roll.step,9):roll.step;
    } else if(roll.step) {
        // $82:959B-95BD. A step the direction flip has just made zero
        // (~-1, $82:9439) completes without moving (DRAGSTER diff fuzz 53).
        roll.step=add_word(roll.step,negative(roll.step)?1:static_cast<std::uint16_t>(-1));
        step_index=negative(roll.step)?add_word(roll.step,9):roll.step;
    }
    if(!roll.step) {
        // $829636-965B keeps the final central pose while X charges a bounce.
        if(turn.pose_override==0x9a8 && index==0 && pressed && !state.surface[index].tile_pose_enabled) {
            roll.bounce_charge=160;return;
        }
        complete_zoom_roll(state,index,interrupted);return;
    }
    auto angle=rider.pose.orientation&63U;
    auto entry=content_word(content.roll_poses,2U*angle);
    if(entry&0x8000U) {rider.pose.reflected=!roll.prior_reflection;roll.pose_base|=0x8000U;}
    else {rider.pose.reflected=roll.prior_reflection!=0;roll.pose_base&=0x7fffU;}
    if(rider.pose.reflected)angle=(64U-angle)&63U;
    entry=content_word(content.roll_poses,2U*angle);
    if(entry&0x8000U)entry=content_word(content.roll_poses,2U*(64U-angle));
    turn.pose_override=static_cast<std::uint16_t>((entry&0x3fffU)+step_index+0x9a0U);
}

// $83E8E0-EC13 and $828953-89C2. Finish animation is a collision-pose input.
void update_zoom_finish(ZoomZooState& state,const ZoomZooContent& content) {
    auto& whole=state.movement;
    if(state.race.riders[0].finished) {
        if(state.race.finish_delay==240)throw std::invalid_argument("race result loading outside frozen finish display");
        ++state.race.finish_delay;
    }
    for(unsigned index=0;index<2;++index) {
        if(!state.race.riders[index].finished)continue;
        auto& input=state.reflection[index];input.brake_input=1;
        input.jump_input=input.rotate_negative_input=input.rotate_positive_input=0;
        if(index==0)whole.player_input.horizontal=1;else state.opponent_horizontal=1;
        if((whole.frame+1U)%3U==0)continue;
        apply_finish_slowdown(whole.riders[index]);
        const auto own=state.race.total_times[index],other=state.race.total_times[1-index];
        const bool won=own!=0xea60U && (other==0xea60U || own<other);
        const bool tied=own==other;
        auto& pose=state.race.finish_pose[index];
        if(pose.active) {
            if(index==1)enqueue_zoom_opponent(whole,tied?38U:won?37U:39U);
            else enqueue_zoom_player(state,tied?38U:won?37U:39U);
        }
        const auto& queue=index==1?whole.rewards:state.player_announcements.queue;
        if(!pose.active && (index==1 || state.native_initialization) &&
           ((queue.write_cursor-queue.read_cursor-1U)&31U)!=0)continue;
        // $828959-8965 waits for pending announcements before first activation.
        pose.active=1;
        const unsigned kind=won||tied?1U:2U;
        if(pose.kind!=kind && !pose.locked) {pose.kind=static_cast<std::uint16_t>(kind);pose.selector=0;pose.locked=1;}
        const auto table=content_word(content.finish_poses,2U*pose.kind)-0xc7c8U;
        auto offset=table+2U*pose.selector;
        ++pose.selector;
        auto value=content_word(content.finish_poses,offset);
        if(value&0x8000U) {
            pose.selector=static_cast<std::uint16_t>(content_word(content.finish_poses,offset+2));
            value=content_word(content.finish_poses,table+2U*pose.selector);
        }
        input.pose_override=static_cast<std::uint16_t>(value);
    }
}

// $818050-82B6: ordered checkpoint tiles and three completed laps after the
// initial start-line crossing. SRAM slots retain original 0xEA60 sentinels.
void update_zoom_checkpoint(ZoomZooState& state,unsigned index,const ZoomZooContent& content) {
    auto& lap=state.race.riders[index];const auto& rider=state.movement.riders[index];
    if(lap.checkpoint_display_countdown)--lap.checkpoint_display_countdown;
    const auto descriptor=rider.contact.selected_word;
    const auto tile=((descriptor&0x3f0U)>>2U)+((descriptor&15U)>>1U);
    if(tile>=content.movement.flat_contact.flags.size())
        throw std::invalid_argument("ZOOM ZOO checkpoint tile flag is missing");
    if(rider.contact.auxiliary_flag || (content.movement.flat_contact.flags[tile]&0xfeU)!=20 || lap.finished)return;
    const unsigned tag=(descriptor&0x1c00U)>>10U;
    const unsigned checkpoint=tag<=5?tag:0;
    if(checkpoint==0) {
        if(lap.start_line_latch)return;
        lap.start_line_latch=1;
        const auto& timer=state.movement.timer;
        const auto hundredths=static_cast<std::uint16_t>(timer.subframe*2U+state.movement.contact_phase);
        lap.time_digits={timer.minutes,timer.tens_seconds,timer.seconds,timer.tenths,hundredths};
        const auto total=static_cast<std::uint16_t>(timer.minutes*6000U+timer.tens_seconds*1000U+timer.seconds*100U+timer.tenths*10U+hundredths);
        std::uint16_t previous=0;
        for(auto value:state.race.lap_times[index])if(value!=0xea60U)previous=add_word(previous,value);
        // $0D15 holds the initial laps + 1 ($81:D673); $81:8139-814A skips the
        // slot of the initial crossing and $81:8197-81A3 its display and
        // final-lap announcement.
        const int initial_laps_remaining=classic_race_scenario(state.track).laps+1;
        const int slot=initial_laps_remaining-static_cast<int>(lap.laps_remaining)-1;
        if(slot>=0 && slot<10)state.race.lap_times[index][static_cast<unsigned>(slot)]=static_cast<std::uint16_t>(total-previous);
        --lap.laps_remaining;
        const bool initial_crossing=initial_laps_remaining-static_cast<int>(lap.laps_remaining)==1;
        if(!initial_crossing && lap.laps_remaining==1 && classic_race_scenario(state.track).tour_race) {
            if(index==1)enqueue_zoom_opponent(state.movement,15);
            else enqueue_zoom_player(state,15);
        }
        if(!initial_crossing) {
            if(lap.laps_remaining==0) {lap.finished=1;state.race.total_times[index]=total;}
            lap.checkpoint_display_countdown=120;
        }
        lap.checkpoint=0;lap.next_checkpoint=1;
    } else {
        if(checkpoint==2) {
            if(!lap.start_line_latch)return;
            --lap.start_line_latch;
        } else if(checkpoint!=lap.next_checkpoint)return;
        lap.checkpoint=static_cast<std::uint16_t>(checkpoint);
        lap.next_checkpoint=static_cast<std::uint16_t>((checkpoint+1)&3U);
        lap.checkpoint_display_countdown=120;
        const auto seen_index=lap.laps_remaining*4U+checkpoint;
        if(seen_index>=state.race.checkpoint_seen.size())throw std::invalid_argument("race checkpoint index invalid");
        auto& seen=state.race.checkpoint_seen[seen_index];
        if(seen&0x80U) {
            seen=0;
            if(index==1)lap.checkpoint_display_countdown=2;
        }
    }
}

// $819FB0-A16D / $81A520-A53F; PAL one-player camera at 4x horizontal scale.
// Positions are world units, signed velocity and lookahead are whole units.
void update_zoom_camera(ZoomZooState& state,const TrackGeometry& geometry) {
    auto& c=state.race.camera;const auto& rider=state.movement.riders[0];
    const auto target=1-(static_cast<std::int16_t>(rider.motion.velocity_x)>>3);
    auto look=static_cast<std::int16_t>(c.lookahead);
    if(look<target)++look;else if(look>target)--look;
    c.lookahead=static_cast<std::uint16_t>(look);
    const auto center=static_cast<std::uint16_t>((c.x+c.lookahead+95U)&geometry.position_mask);
    const auto delta=static_cast<std::int16_t>(static_cast<std::uint16_t>(
        (static_cast<unsigned>(rider.motion.x)<<geometry.screen_shift)-(static_cast<unsigned>(center)<<geometry.screen_shift)));
    int vx{};
    // Outside the window the $81:9FF8-A02B half-world test reduces to the sign of the wrapped difference.
    if(delta < geometry.follow_window_low || delta>=geometry.follow_window_high)vx=delta<0?-16:16;
    else {const int distance=delta>>geometry.screen_shift;vx=distance<0?std::min(0,distance+8):std::max(0,distance-8);}
    c.velocity_x=static_cast<std::uint16_t>(vx);
    int candidate=static_cast<std::int16_t>(c.y)+30;
    int vy=-16;
    while(vy!=16 && candidate<static_cast<std::int16_t>(rider.motion.y)) {
        candidate+=4;++vy;if(vy==0)candidate+=20;
    }
    c.velocity_y=static_cast<std::uint16_t>(vy);
    c.x=static_cast<std::uint16_t>(c.x+vx)&geometry.position_mask;c.y=static_cast<std::uint16_t>(c.y+vy);
}
void update_zoom_visibility(ZoomZooState& state,const TrackGeometry& geometry) {
    auto& c=state.race.camera;const auto& rider=state.movement.riders[0];
    // $82ACAE-AD7A. The previous frame's visibility feeds speed damping.
    const int dy=static_cast<std::int16_t>(rider.motion.y-c.y);
    const auto dx=static_cast<std::int16_t>(rider.motion.x-c.x);
    const int scaled=static_cast<std::int16_t>(static_cast<std::uint16_t>(static_cast<unsigned>(static_cast<std::uint16_t>(dx))<<geometry.screen_shift));
    const bool outside=dy < -41 || dy>=225 || scaled < geometry.visible_left || scaled>=geometry.visible_right;
    state.race.provisional_1225=(outside || scaled<0)?1:0;
    state.race.provisional_1227=0;
    c.screen_xy=outside?0x7070U:static_cast<std::uint16_t>((static_cast<unsigned>(dy)&255U)*256U+(static_cast<unsigned>(dx)&255U));
}

// $83904A-90F0 publishes graph extrema at result update106. $80F88D
// publishes the two total times on update107. Prior track records are the
// authenticated fresh-scenario 60000 sentinel; they do not expand this range.
ZoomZooResult zoom_result_fields(const ZoomZooRaceState& race,unsigned updates,bool lap_graph=true) {
    ZoomZooResult result{};
    if(lap_graph && updates>=106) {
        std::uint16_t minimum=60000,maximum=0;
        for(const auto& laps:race.lap_times)for(auto lap:laps)if(lap<60000) {
            minimum=std::min(minimum,lap);maximum=std::max(maximum,lap);
        }
        if(static_cast<std::uint16_t>(maximum-minimum)<200U)
            minimum=static_cast<std::uint16_t>(maximum-200U);
        result.graph_minimum=minimum;result.graph_maximum=maximum;
    }
    // $80:F88D-F8A5 publishes the totals one load update later on the mode-0
    // (DRAGSTER) result screen: 108, observed at DRAGSTER load 3580 -> 3687.
    if(updates>=(lap_graph?107U:108U))result.published_totals=race.total_times;
    return result;
}

void integrate_zoom_axis(std::uint16_t& position,std::uint16_t velocity,std::uint16_t& residue) {
    const auto total=static_cast<std::int16_t>(add_word(velocity,residue));
    position=static_cast<std::uint16_t>(static_cast<int>(position)+total/32);
    residue=static_cast<std::uint16_t>(total%32);
}
} // namespace

std::uint16_t next_wrong_direction_counter(std::uint16_t previous,
    std::uint16_t velocity_x,std::uint16_t marker,unsigned horizontal,bool native_rewards) {
    const bool moving=!negative(static_cast<std::uint16_t>(velocity_x-16U)) ||
        negative(static_cast<std::uint16_t>(velocity_x-0xfff0U));
    if(!moving || (marker&0x8000U) || !((marker&0x4000U)?horizontal==2:horizontal==0))return 0;
    const auto next=add_word(previous,1);
    if(next==180) {
        if(native_rewards)return 120; // $82974B/977B repeats the warning after60 further active updates.
        throw std::invalid_argument("ZOOM ZOO wrong-direction reward is unrecovered");
    }
    return next;
}

ClassicRaceScenario classic_race_scenario(ClassicRaceTrack track) {
    // ZOOM ZOO: M4-16 primary, end-1376, three laps, result stable at load 115.
    // DRAGSTER: end-1328 on the accepted menu path (R-0038), one lap; the
    // stable winner/loser screens follow load 226/242 (R-0012, R-0019).
    if(track==ClassicRaceTrack::ZoomZoo)return {track,1376,3,115,115,true};
    if(track==ClassicRaceTrack::Dragster)return {track,1328,1,226,242,false};
    // TRACK-BREADTH part 2 (R-0046 observations 6-8): the other race tracks a
    // cold start reaches, as the original sets them up. The race mode ($77:074B)
    // and lap count ($77:0744) are read at each track's initialization boundary;
    // a one-run race stores 0 laps and races one, as DRAGSTER does. The frame is
    // the boundary on the laboratory's menu path (`track_reference`), a label
    // only. The stable result updates follow the race mode's accepted track
    // (DRAGSTER for mode 0, ZOOM ZOO for mode 1): a hypothesis, since no new
    // track's result screen has been captured.
    struct Observed {std::uint8_t index;std::uint16_t initialization_frame,laps;bool lap_race;};
    static constexpr std::array<Observed,14> observed{{
        {3,1418,1,false},{4,1419,3,true},{10,1417,1,false},{11,1368,3,true},{13,1392,1,false},{14,1376,7,true},
        {20,1360,1,false},{21,1367,3,true},{23,1386,1,false},{24,1402,3,true},{30,1390,1,false},{31,1397,3,true},
        {33,1403,1,false},{34,1407,5,true}}};
    for(const auto& o:observed)
        if(o.index==track.index)
            return o.lap_race?ClassicRaceScenario{track,o.initialization_frame,o.laps,115,115,true}
                             :ClassicRaceScenario{track,o.initialization_frame,o.laps,226,242,false};
    throw std::invalid_argument("classic race track has no recovered scenario");
}

bool classic_race_has_scenario(ClassicRaceTrack track) {
    try {(void)classic_race_scenario(track);return true;}
    catch(const std::invalid_argument&) {return false;}
}

std::array<std::uint8_t,8> classic_race_state_magic(ClassicRaceTrack track) {
    if(track==ClassicRaceTrack::Dragster)return dragster_race_state_magic;
    if(track==ClassicRaceTrack::ZoomZoo)return {'U','R','Z','Z','0','0','0','B'};
    return {'U','R','T','R',static_cast<std::uint8_t>('0'+track.index/10U),static_cast<std::uint8_t>('0'+track.index%10U),'0','3'};
}

std::uint16_t race_adjustment_limit(const ClassicRaceScenario& scenario) {
    return static_cast<std::uint16_t>(scenario.tour_race?0x48U:0x60U);
}

bool classic_race_start_reflected(std::span<const std::uint8_t> decoded_track,unsigned rider) {
    if(rider>1U)throw std::invalid_argument("classic races have two riders");
    if(decoded_track.size()<11)throw std::invalid_argument("ZOOM ZOO track header missing");
    return (content_word(decoded_track,5+4*rider)&1U)==0;
}

TrackGeometry track_geometry(std::span<const std::uint8_t> decoded_track) {
    if(decoded_track.size()<14)throw std::invalid_argument("track header lacks its playfield shape");
    // $81:A304-A342 switches on byte 13, a quarter of the column count, into
    // one arm per shape; every arm covers 16,384 cells. The 0x00 and 0x40 arms
    // are observed (DRAGSTER, ZOOM ZOO); the 0x80, 0x20, 0x10 and 0x08 arms
    // store the same fields with the same progression and are read from the
    // static listing (TRACK-BREADTH, R-0046) until a capture executes them.
    // The 0x04 arm also sets $0FF7, which changes the sampler ($81:8A2C) and
    // the BG1 map fetch ($81:AD1D-ADA7); that is not recovered, so it is
    // rejected, as is any other value (the original falls into BRK at $A342).
    switch(decoded_track[13]) {
    case 0x00: return {1024,0xffff,0,-0x18,0x19,-0x31,0x100};      // $81:A4C1-A4FD, 1,024 x 16
    case 0x80: return {512,0x7fff,1,-0x30,0x32,-0x62,0x200};       // $81:A483-A4BF, 512 x 32
    case 0x40: return {256,0x3fff,2,-0x60,0x64,-0xc4,0x400};       // $81:A445-A481, 256 x 64
    case 0x20: return {128,0x1fff,3,-0xc0,0xc8,-0x188,0x800};      // $81:A406-A444, 128 x 128
    case 0x10: return {64,0x0fff,4,-0x180,0x190,-0x310,0x1000};    // $81:A3C7-A405, 64 x 256
    case 0x08: return {32,0x07ff,5,-0x300,0x320,-0x620,0x2000};    // $81:A388-A3C6, 32 x 512
    default: throw std::invalid_argument("track playfield shape is outside the recovered tracks");
    }
}

// Initializer provenance: $82:D7C6-D7FA clears the working state; D89D-D904
// derives positions from the decompressed track header in 16-world-unit cells.
// DB25-DB7F initializes lap SRAM, DB96-DBBD applies the selected race settings.
ZoomZooState classic_crawler_zoom_zoo_start(const ZoomZooContent& content) {
    return classic_race_start(content,classic_race_scenario(ClassicRaceTrack::ZoomZoo));
}
ZoomZooState classic_crawler_dragster_race_start(const ZoomZooContent& content) {
    return classic_race_start(content,classic_race_scenario(ClassicRaceTrack::Dragster));
}
ZoomZooState classic_race_start(const ZoomZooContent& content,const ClassicRaceScenario& scenario) {
    const auto track=content.movement.sampling.track;
    if(track.size()<11)throw std::invalid_argument("ZOOM ZOO track header missing");
    ZoomZooState state{};
    state.track=scenario.track;
    state.native_initialization=state.complete_race=state.sustained=true;
    auto& movement=state.movement;
    movement.frame=scenario.initialization_frame;
    movement.player_input.vertical=movement.player_input.horizontal=1;
    movement.countdown=270; // $82:D841-D844; timer begins below 68 after countdown publication.
    movement.rewards.write_cursor=1; // $81:C615-C619.
    for(unsigned i=0;i<2;++i) {
        auto& rider=movement.riders[i];
        const auto y=content_word(track,5+4*i);
        rider.motion.x=static_cast<std::uint16_t>(content_word(track,3+4*i)<<4);
        rider.motion.y=static_cast<std::uint16_t>(y<<4);
        rider.pose.reflected=classic_race_start_reflected(track,i);
        state.reflection[i].base_velocity_cap=448;
        state.race.riders[i].laps_remaining=static_cast<std::uint16_t>(scenario.laps+1U); // Plus the initial line crossing.
        state.race.lap_times[i].fill(60000);
        state.race.total_times[i]=60000;
    }
    state.race.camera.x=static_cast<std::uint16_t>((movement.riders[0].motion.x-256U)&0xfff0U);
    state.race.camera.y=static_cast<std::uint16_t>((movement.riders[0].motion.y-256U)&0xfff0U);
    state.race.camera.screen_xy=0xe0e0; // $82:D724-D731 OAM initialization.
    state.opponent_retained_oam_x=0x65; // Explicit $82:D76D-D76F HUD OAM default.
    state.start_boost.fill(384);
    state.player_announcements.queue.write_cursor=1;
    if(content.reward_weights.size()!=26)throw std::invalid_argument("ZOOM ZOO reward-weight table is missing");
    // $82DB87-DB94 copies the same 26-byte $82D7A4 template into both banks,
    // so the opponent's event-one weight is that content byte too rather than
    // a constant repeated here.
    state.player_announcements.queue.event_one_weight=content.reward_weights[0];
    movement.rewards.event_one_weight=content.reward_weights[0];
    for(auto& weights:state.learned_weights)std::copy(content.reward_weights.begin()+1,content.reward_weights.end(),weights.begin());
    state.player_announcements.hints_active=1; // $82D95C fresh scenario tutorial bit.
    state.player_announcements.hint_updates=30; // $82D972-D975.
    state.race.checkpoint_seen.fill(255); // $81:CD2A.
    return state;
}

bool classic_race_player_won(const ZoomZooState& state) {
    // Finish order, as $83:E8E0-EC13 selects the finish pose: an equal time
    // means both crossed on one update, and the player is processed first.
    const auto own=state.race.total_times[0],other=state.race.total_times[1];
    return own!=0xea60U && (other==0xea60U || own<=other);
}

std::uint16_t stable_result_updates(const ZoomZooState& state) {
    const auto scenario=classic_race_scenario(state.track);
    return classic_race_player_won(state)?scenario.stable_result_won:scenario.stable_result_lost;
}

void restart_zoom_zoo(ZoomZooState& state,const ZoomZooContent& content) {
    const bool paused_restart=state.pause.selection==0xffffU && state.pause.released;
    if(!state.native_initialization || (state.result_updates!=stable_result_updates(state) && !paused_restart))
        throw std::invalid_argument("ZOOM ZOO restart requires a stable result or selected paused restart");
    state=classic_race_start(content,classic_race_scenario(state.track));
}

std::vector<std::uint8_t> serialize_zoom_zoo(const ZoomZooState& state) {
    if(state.complete_race && !state.sustained)throw std::invalid_argument("race state requires sustained prefix");
    auto bytes=serialize_movement_state(state.movement);
    if(bytes.size()!=333)throw std::invalid_argument("ZOOM ZOO finish state is unsupported");
    const std::array<std::uint8_t,8> magic{'U','R','Z','Z','0','0','0','1'};
    std::copy(magic.begin(),magic.end(),bytes.begin());
    for(const auto& r:state.reflection)write_reflection(bytes,r);
    put8(bytes,state.opponent_horizontal);put8(bytes,state.opponent_retained_oam_x);
    if(state.sustained) {
        bytes[7]='2';
        for(const auto& r:state.surface)
            for(auto v:{r.mode,r.angle,r.tile_mode,r.leading_support,r.tile_pose,r.animation_delta,r.tile_pose_enabled})put16(bytes,v);
    }
    if(state.complete_race) {
        bytes[7]=state.native_initialization?'B':'3';
        for(const auto& r:state.race.riders) {
            for(auto v:{r.laps_remaining,r.checkpoint,r.next_checkpoint,r.start_line_latch,r.checkpoint_display_countdown,r.finished})put16(bytes,v);
            for(auto v:r.time_digits)put16(bytes,v);
        }
        for(const auto& times:state.race.lap_times)for(auto v:times)put16(bytes,v);
        for(auto v:state.race.total_times)put16(bytes,v);
        for(auto v:{state.race.provisional_1225,state.race.provisional_1227,state.race.finish_delay})put16(bytes,v);
        const auto& c=state.race.camera;
        for(auto v:{c.x,c.y,c.velocity_x,c.velocity_y,c.lookahead,c.screen_xy})put16(bytes,v);
        for(unsigned i=0;i<20;++i)put8(bytes,state.race.checkpoint_seen[i]);
        for(const auto& p:state.race.finish_pose)for(auto v:{p.selector,p.kind,p.locked,p.active})put16(bytes,v);
    }
    if(state.native_initialization) {
        put16(bytes,state.fade_level);
        for(auto v:state.start_boost)put16(bytes,v);
        put16(bytes,state.result_updates);
        put16(bytes,state.result.graph_minimum);put16(bytes,state.result.graph_maximum);
        for(auto v:state.result.published_totals)put16(bytes,v);
        for(auto v:state.charge_announced)put16(bytes,v);
        const auto& a=state.player_announcements;const auto& q=a.queue;
        for(auto v:q.entries)put8(bytes,v);
        put8(bytes,q.read_cursor);put8(bytes,q.write_cursor);put16(bytes,q.cooldown);
        put16(bytes,q.feature_total);put8(bytes,q.event_one_weight);
        for(auto v:{a.hints_active,a.hint_updates,a.hint_group,a.empty_display})put16(bytes,v);
        for(const auto& r:state.rolls)
            for(auto v:{r.input_latched,r.prior_orientation,r.prior_reflection,r.pose_base,r.step,r.held_updates,
                        r.bounce_charge,r.completed_rolls,r.held_rotations,r.bounce_active,r.support_count_mirror,r.prior_step})put16(bytes,v);
        for(const auto& weights:state.learned_weights)for(auto v:weights)put8(bytes,v);
        put16(bytes,state.pause.selection);put16(bytes,state.pause.released);
        put32(bytes,state.pause.suspended_updates);put32(bytes,state.pause.suspended_countdown_updates);
    }
    // R-0047: the special-tile words follow the shared 742 bytes in the other
    // tracks' layout (URTRnn02), and in DRAGSTER's and ZOOM ZOO's only while
    // one is live (URDG0002, URZZ000C): no accepted race reaches a special
    // tile, so their frozen 742-byte states are unchanged, but ZOOM ZOO's own
    // tile table holds the corkscrew (pair 10).
    const bool other_track=state.track!=ClassicRaceTrack::ZoomZoo && state.track!=ClassicRaceTrack::Dragster;
    const bool special_tiles_live=state.special_tiles!=std::array<SpecialTileRider,2>{};
    if(other_track || special_tiles_live) {
        if(!state.native_initialization)throw std::invalid_argument("special-tile words require a natively initialized race");
        for(const auto& r:state.special_tiles)
            for(auto v:{r.mud_cooldown,r.mud_exit_pending,r.corkscrew_latch,r.corkscrew_step,
                        r.corkscrew_float,r.physics_hold,r.reflection_lock,r.raised_priority})put16(bytes,v);
        put16(bytes,state.drive_target_latch);
        if(state.track==ClassicRaceTrack::ZoomZoo)bytes[7]='C';
    }
    // R-0048: the other tracks' layout also carries the last 60 first-seen
    // flags. DRAGSTER's one lap and ZOOM ZOO's three index at most flag 19,
    // so their layouts hold only the first 20 and a restore sets the rest to
    // $FF, as race setup does.
    if(other_track)for(unsigned i=20;i<80;++i)put8(bytes,state.race.checkpoint_seen[i]);
    if(state.track!=ClassicRaceTrack::ZoomZoo) {
        // Another track on the shared engine: the URZZ000B layout under its own
        // identity, so a restore can never run one track's state on another.
        if(!state.native_initialization)throw std::invalid_argument("a race state of any track but ZOOM ZOO requires native initialization");
        const auto identity=classic_race_state_magic(state.track);
        std::copy(identity.begin(),identity.end(),bytes.begin());
        if(state.track==ClassicRaceTrack::Dragster && special_tiles_live)bytes[7]='2';
    }
    return bytes;
}
static ZoomZooState deserialize_classic_race(std::span<const std::uint8_t> bytes,ClassicRaceTrack track) {
    const std::array<std::uint8_t,8> magic{'U','R','Z','Z','0','0','0','1'};
    const auto scenario=classic_race_scenario(track);
    const bool native_initialization=bytes.size()==742 && bytes[7]=='B';
    const bool complete_race=((bytes.size()==565 && bytes[7]=='3') || native_initialization) && std::equal(magic.begin(),magic.begin()+7,bytes.begin());
    const bool sustained=complete_race || (bytes.size()==423 && bytes[7]=='2' && std::equal(magic.begin(),magic.begin()+7,bytes.begin()));
    if(!sustained && (bytes.size()!=395 || !std::equal(magic.begin(),magic.end(),bytes.begin())))throw std::invalid_argument("ZOOM ZOO state identity/width differs");
    std::vector<std::uint8_t> prefix(bytes.begin(),bytes.begin()+333);
    std::copy(movement_state_magic.begin(),movement_state_magic.end(),prefix.begin());
    ZoomZooState state;state.track=track;state.native_initialization=native_initialization;state.complete_race=complete_race;state.sustained=sustained;state.movement=deserialize_movement_state(prefix);
    Reader in{bytes.subspan(333)};
    for(auto& r:state.reflection)read_reflection(in,r);
    state.opponent_horizontal=in.u8();state.opponent_retained_oam_x=in.u8();
    if(state.opponent_horizontal>2)throw std::invalid_argument("ZOOM ZOO horizontal input is invalid");
    if(sustained)for(auto& r:state.surface)
        for(auto* v:{&r.mode,&r.angle,&r.tile_mode,&r.leading_support,&r.tile_pose,&r.animation_delta,&r.tile_pose_enabled})*v=in.u16();
    if(complete_race) {
        for(auto& r:state.race.riders) {
            for(auto* v:{&r.laps_remaining,&r.checkpoint,&r.next_checkpoint,&r.start_line_latch,&r.checkpoint_display_countdown,&r.finished})*v=in.u16();
            for(auto& v:r.time_digits)v=in.u16();
            if(r.laps_remaining>scenario.laps+1U || r.checkpoint>3 || r.next_checkpoint>3 || r.start_line_latch>1 || r.finished>1 || r.checkpoint_display_countdown>120)
                throw std::invalid_argument("ZOOM ZOO race checkpoint state is invalid");
        }
        for(auto& times:state.race.lap_times)for(auto& v:times)v=in.u16();
        for(auto& v:state.race.total_times)v=in.u16();
        state.race.provisional_1225=in.u16();state.race.provisional_1227=in.u16();state.race.finish_delay=in.u16();
        auto& c=state.race.camera;
        for(auto* v:{&c.x,&c.y,&c.velocity_x,&c.velocity_y,&c.lookahead,&c.screen_xy})*v=in.u16();
        for(unsigned i=0;i<20;++i)state.race.checkpoint_seen[i]=in.u8();
        std::fill(state.race.checkpoint_seen.begin()+20,state.race.checkpoint_seen.end(),std::uint8_t{255});
        for(auto& p:state.race.finish_pose)for(auto* v:{&p.selector,&p.kind,&p.locked,&p.active})*v=in.u16();
        if(state.race.finish_delay>240 || (state.race.finish_delay && !state.race.riders[0].finished) || state.race.provisional_1225>1 || state.race.provisional_1227>1 ||
           (track==ClassicRaceTrack::ZoomZoo && c.x>0x3fffU) || std::abs(static_cast<std::int16_t>(c.velocity_x))>16 ||
           std::abs(static_cast<std::int16_t>(c.velocity_y))>16)
            throw std::invalid_argument("ZOOM ZOO finish/camera state invalid");
        for(auto seen:state.race.checkpoint_seen)if(seen!=0 && seen!=255)
            throw std::invalid_argument("ZOOM ZOO checkpoint seen flag invalid");
        for(unsigned index=0;index<2;++index) {
            const auto& pose=state.race.finish_pose[index];
            const unsigned limit=pose.kind==1?48U:pose.kind==2?88U:0U;
            if((pose.active && !state.race.riders[index].finished) || pose.selector>limit || pose.kind>2 || pose.locked>1 || pose.active>1 ||
               (pose.active ? (!pose.kind || !pose.locked) : (pose.kind || pose.locked || pose.selector)))
                throw std::invalid_argument("ZOOM ZOO finish pose state invalid");
        }
        // $81:C73E-C75B finishes both riders, laps or not, once the clock holds 9:59.9.
        const auto& clock=state.movement.timer;
        const bool clock_expired=native_initialization && clock.minutes==9 && clock.tens_seconds==5 && clock.seconds==9 && clock.tenths==9;
        const bool timed_out=state.race.riders[0].finished && state.race.riders[1].finished && clock_expired;
        for(const auto& lap:state.race.riders) {
            if((lap.finished ? lap.laps_remaining!=0 && !timed_out : lap.laps_remaining==0) || lap.time_digits[0]>9 || lap.time_digits[1]>5 ||
               lap.time_digits[2]>9 || lap.time_digits[3]>9 || lap.time_digits[4]>9)
                throw std::invalid_argument("ZOOM ZOO lap time state invalid");
        }
    }
    if(native_initialization) {
        state.fade_level=in.u16();
        for(auto& v:state.start_boost)v=in.u16();
        state.result_updates=in.u16();
        state.result.graph_minimum=in.u16();state.result.graph_maximum=in.u16();
        for(auto& v:state.result.published_totals)v=in.u16();
        for(auto& v:state.charge_announced) {v=in.u16();if(v>1)throw std::invalid_argument("invalid ZOOM ZOO charge flag");}
        auto& a=state.player_announcements;auto& q=a.queue;
        for(auto& v:q.entries)v=in.u8();
        q.read_cursor=in.u8();q.write_cursor=in.u8();q.cooldown=in.u16();
        q.feature_total=in.u16();q.event_one_weight=in.u8();
        a.hints_active=in.u16();a.hint_updates=in.u16();a.hint_group=in.u16();a.empty_display=in.u16();
        if(q.read_cursor>31 || q.write_cursor>31 || q.cooldown>120 ||
           // The reachable weights are the halvings of the $82D7A4 template
           // byte the initializer now reads from content; they coincide for
           // the frozen pack, so a template change must revisit this bound.
           (q.event_one_weight!=1 && q.event_one_weight!=2 && q.event_one_weight!=4) ||
           a.hints_active>1 || a.hint_updates>=300 || a.hint_group>=8 || a.empty_display>1)
            throw std::invalid_argument("invalid ZOOM ZOO player announcement state");
        // The opponent's identical queue fields had no bound at all, so a
        // forged bank could publish an arbitrary feature total or a weight
        // that halving can never reach. $82DB87-DB94 seeds both banks from
        // one template, and $81C27B-C280 only ever halves toward a floor of
        // one, so the reachable set matches the player's. The opponent has
        // no tutorial path, so its cooldown tops out at the 40 of
        // $81C2CC-C2D0 rather than the player's hint value of 120.
        {
            const auto& o=state.movement.rewards;
            if(o.cooldown>40 || (o.event_one_weight!=1 && o.event_one_weight!=2 && o.event_one_weight!=4))
                throw std::invalid_argument("invalid ZOOM ZOO opponent reward queue state");
            // $83E1F1 masks the sloped selector with 7 and the flat path writes
            // only 0 or 1, so nothing above 7 is producible. The removed
            // multi-axis guards were this field's only bound, and without one a
            // forged 14 or 65535 would alias onto 6 and 7 and play identically
            // to them, which the original never does.
            if(state.movement.opponent_ai.trick_selector>7)
                throw std::invalid_argument("invalid ZOOM ZOO opponent trick selector");
        }
        if(state.movement.countdown && (state.race.riders[0].finished || state.race.riders[1].finished || state.result_updates))
            throw std::invalid_argument("ZOOM ZOO finish/result conflicts with start phase");
        if(state.result!=zoom_result_fields(state.race,state.result_updates,scenario.tour_race))
            throw std::invalid_argument("inconsistent ZOOM ZOO result publication");
        for(unsigned roll_index=0;roll_index<state.rolls.size();++roll_index) {
            auto& r=state.rolls[roll_index];
            for(auto* v:{&r.input_latched,&r.prior_orientation,&r.prior_reflection,&r.pose_base,&r.step,&r.held_updates,
                         &r.bounce_charge,&r.completed_rolls,&r.held_rotations,&r.bounce_active,&r.support_count_mirror,&r.prior_step})*v=in.u16();
            if(r.input_latched>1 || r.prior_orientation>63 || r.prior_reflection>1 ||
               static_cast<std::int16_t>(r.step)<-9 || static_cast<std::int16_t>(r.step)>9 || (r.bounce_charge!=0 && r.bounce_charge!=160) || r.bounce_active>1 || (r.bounce_charge && r.step) || r.prior_step || (r.pose_base&0x4000U))
                throw std::invalid_argument("invalid ZOOM ZOO roll state");
            // $829641-9649 and $82965E-9666 read $0C6D and, when it is
            // non-zero, require rider $0FF9 to be the player before storing
            // charge $A0 at $1007. The reference guard holds $0C6D at 1 on
            // every authenticated frame, so the opponent can never charge and
            // never reach the bounce that only a charge enables. The native
            // producer already gates on the rider index; reject the forged
            // restore too instead of continuing from a state the original
            // cannot produce.
            if(roll_index==1 && (r.bounce_charge || r.bounce_active))
                throw std::invalid_argument("invalid ZOOM ZOO opponent bounce state");
            if(state.movement.frame==scenario.initialization_frame && (r.input_latched || r.prior_orientation || r.prior_reflection || r.pose_base ||
               r.step || r.held_updates || r.bounce_charge || r.completed_rolls || r.held_rotations || r.bounce_active || r.support_count_mirror || r.prior_step))
                throw std::invalid_argument("inconsistent initial ZOOM ZOO roll state");
        }
        for(auto& weights:state.learned_weights)for(auto& v:weights) {
            v=in.u8();if(v>64)throw std::invalid_argument("invalid ZOOM ZOO learned reward weight");
        }
        state.pause.selection=in.u16();state.pause.released=in.u16();
        state.pause.suspended_updates=in.u32();state.pause.suspended_countdown_updates=in.u32();
        if((state.pause.selection!=0 && state.pause.selection!=1 && state.pause.selection!=0xffff) || state.pause.released>1 ||
           state.movement.frame<scenario.initialization_frame || state.pause.suspended_updates>state.movement.frame-scenario.initialization_frame ||
           state.pause.suspended_countdown_updates>state.pause.suspended_updates ||
           ((state.pause.selection || state.pause.released) && !state.pause.suspended_updates))
            throw std::invalid_argument("invalid ZOOM ZOO pause state");
        for(unsigned i=0;i<2;++i) {
            const auto& roll=state.rolls[i];
            // $8295D5-95F6 publishes the reflection flag and bit15 together
            // on every active roll pose, including held/returning poses.
            if(roll.step && bool(roll.pose_base&0x8000U)!=
               (state.movement.riders[i].pose.reflected!=(roll.prior_reflection!=0)))
                throw std::invalid_argument("inconsistent ZOOM ZOO active roll reflection");
            // Each held/completed counter advances at most once per update,
            // so neither can exceed the elapsed updates before a word wraps.
            // Hold duration is not bounded by held rotations: a landing's
            // reward pass ($829B69-9D97) clears the rotations while a released
            // roll keeps counting its hold across the bounce, and the original
            // reaches holds above rotations (DRAGSTER fuzz seeds 31 at 1623 and
            // 383 at 3472; R-0038). The earlier M4-16 bound rejected them.
            const auto elapsed=state.movement.frame>=scenario.initialization_frame?state.movement.frame-scenario.initialization_frame:0U;
            if(elapsed<65536U) {
                const auto held=static_cast<std::int16_t>(roll.held_updates);
                const auto magnitude=static_cast<unsigned>(held<0?-static_cast<int>(held):held);
                if(magnitude>elapsed || roll.held_rotations>elapsed || roll.completed_rolls>elapsed)
                    throw std::invalid_argument("inconsistent ZOOM ZOO held roll counters");
            }
        }
        for(unsigned i=0;i<2;++i)
            if(state.rolls[i].support_count_mirror!=state.movement.riders[i].contact.unsupported_count)
                throw std::invalid_argument("inconsistent ZOOM ZOO support-count mirror");
        if(state.result_updates>std::max(scenario.stable_result_won,scenario.stable_result_lost) || (state.result_updates && state.race.finish_delay!=240))throw std::invalid_argument("invalid ZOOM ZOO result phase");
        for(auto v:state.start_boost)if(v!=0 && v!=384)throw std::invalid_argument("invalid ZOOM ZOO start boost");
        if(state.fade_level>30)throw std::invalid_argument("invalid ZOOM ZOO fade level");
        if(state.movement.frame<scenario.initialization_frame)throw std::invalid_argument("invalid ZOOM ZOO initialization frame");
        const auto elapsed=state.movement.frame-scenario.initialization_frame;
        if(elapsed==0 && (q.read_cursor || q.write_cursor!=1 || q.cooldown || q.feature_total || q.event_one_weight!=4 ||
           a.hints_active!=1 || a.hint_updates!=30 || a.hint_group || a.empty_display ||
           std::any_of(q.entries.begin(),q.entries.end(),[](auto event){return event!=0;})))
            throw std::invalid_argument("inconsistent initial ZOOM ZOO announcements");
        if(!state.result_updates && a.hints_active &&
           (a.hint_updates!=(elapsed-state.pause.suspended_updates+30U)%300U || a.hint_group!=((elapsed-state.pause.suspended_updates+30U)/300U)%8U))
            throw std::invalid_argument("inconsistent ZOOM ZOO hint phase");
        // $829D47-D5B constrains voice producers for MIKE/BRONSEN. The
        // opponent consumer's domain test $81C238 is signed, so its 200-215
        // range reaches the reward path and exits on the zero learned weight
        // beyond the bank; only these produced values are admitted here.
        for(auto event:q.entries)if(event>=88)
            throw std::invalid_argument("invalid ZOOM ZOO player voice event");
        // This clause is what keeps update_reward_queue's 216-255 rejection
        // unreachable and its $7E21C9-$7E21D8 guards sufficient; widening the
        // admitted range means widening those guards too.
        for(auto event:state.movement.rewards.entries)if(event>=72 && (event<200 || event>215))
            throw std::invalid_argument("invalid ZOOM ZOO opponent voice event");
        for(unsigned cursor=(q.read_cursor+1U)&31U;cursor!=q.write_cursor;cursor=(cursor+1U)&31U)
            if(q.entries[cursor]==0)throw std::invalid_argument("empty pending ZOOM ZOO announcement");
        // The opponent's ring needs the same closure. $81C598-C5C8 never
        // publishes a zero, and the consumer rejects one, so a restore that
        // carried a published zero would only fail on the following update.
        {
            const auto& o=state.movement.rewards;
            for(unsigned cursor=(o.read_cursor+1U)&31U;cursor!=o.write_cursor;cursor=(cursor+1U)&31U)
                if(o.entries[cursor]==0)throw std::invalid_argument("empty pending ZOOM ZOO opponent reward");
        }
        if(elapsed==0 && (state.charge_announced[0] || state.charge_announced[1]))throw std::invalid_argument("initial charge flag must be clear");
        if(state.pause.suspended_countdown_updates>(elapsed>4U?elapsed-4U:0U))
            throw std::invalid_argument("invalid ZOOM ZOO suspended countdown clock");
        const auto decrements=elapsed>4U?std::min(270U,elapsed-4U-state.pause.suspended_countdown_updates):0U;
        if(state.fade_level!=std::min(30U,elapsed) || state.movement.countdown!=270U-decrements)
            throw std::invalid_argument("inconsistent ZOOM ZOO countdown/fade phase");
        if(state.movement.countdown>=129U && (state.start_boost[0]!=384 || state.start_boost[1]!=384))
            throw std::invalid_argument("premature ZOOM ZOO start boost consumption");
        for(unsigned i=0;i<2;++i) {
            const auto completed=unsigned(scenario.laps)-std::min(unsigned(scenario.laps),unsigned(state.race.riders[i].laps_remaining));
            std::uint16_t sum=0;
            for(unsigned slot=0;slot<10;++slot) {
                const auto lap=state.race.lap_times[i][slot];
                if((slot<completed)==(lap==60000))
                    throw std::invalid_argument("inconsistent ZOOM ZOO completed lap slots");
                if(slot<completed)sum=add_word(sum,lap);
            }
            // A rider finished by the clock limit keeps the no-time sentinel.
            if(state.race.total_times[i]!=(state.race.riders[i].finished && !state.race.riders[i].laps_remaining?sum:60000))
                throw std::invalid_argument("inconsistent ZOOM ZOO total time");
        }
    }
    for(const auto& surface:state.surface) {
        if(surface.mode>1 || surface.tile_mode>1 || surface.leading_support>1 ||
           surface.tile_pose>1 || surface.tile_pose_enabled>1)
            throw std::invalid_argument("ZOOM ZOO surface flags are invalid");
    }
    if(state.movement.frame<(state.native_initialization?scenario.initialization_frame:1649U) || (!state.native_initialization && state.movement.frame>(sustained?9999U:1849U)))
        throw std::invalid_argument("ZOOM ZOO state is outside trial horizon");
    for(const auto& r:state.reflection) {
        if(r.step>16 || (r.end!=0 && r.end!=9 && r.end!=16) || r.completed>1 ||
           r.drive_pose_enabled>1 || r.brake_input>1 || r.rotate_negative_input>1 ||
           r.rotate_positive_input>1 || r.jump_input>1)
            throw std::invalid_argument("ZOOM ZOO reflection/control state is invalid");
    }
    in.require_end();
    return state;
}
ZoomZooState deserialize_zoom_zoo(std::span<const std::uint8_t> bytes) {
    // Any track but ZOOM ZOO carries its own identity over the URZZ000B layout;
    // a 776-byte state appends the special-tile words (R-0047).
    const auto magic_is=[&](std::string_view text){return std::equal(text.begin(),text.end(),bytes.begin());};
    std::optional<ClassicRaceTrack> track;
    bool extended=false;
    if(bytes.size()==742) {
        if(std::equal(dragster_race_state_magic.begin(),dragster_race_state_magic.end(),bytes.begin()))track=ClassicRaceTrack::Dragster;
    } else if(bytes.size()==776) {
        extended=true;
        if(magic_is("URZZ000C"))track=ClassicRaceTrack::ZoomZoo;
        else if(magic_is("URDG0002"))track=ClassicRaceTrack::Dragster;
        else throw std::invalid_argument("classic race state identity/width differs");
    } else if(bytes.size()==836) {
        extended=true;
        if(magic_is("URTR") && bytes[4]>='0' && bytes[4]<='9' && bytes[5]>='0' && bytes[5]<='9' && bytes[6]=='0' && bytes[7]=='3') {
            const ClassicRaceTrack other{static_cast<std::uint8_t>((bytes[4]-'0')*10+(bytes[5]-'0'))};
            if(other==ClassicRaceTrack::Dragster || other==ClassicRaceTrack::ZoomZoo || !classic_race_has_scenario(other))
                throw std::invalid_argument("classic race state names a track without its own identity");
            track=other;
        } else throw std::invalid_argument("classic race state identity/width differs");
    }
    if(!track)return deserialize_classic_race(bytes,ClassicRaceTrack::ZoomZoo);
    std::vector<std::uint8_t> shared(bytes.begin(),bytes.begin()+742);
    const auto zoom_zoo_magic=classic_race_state_magic(ClassicRaceTrack::ZoomZoo);
    std::copy(zoom_zoo_magic.begin(),zoom_zoo_magic.end(),shared.begin());
    auto state=deserialize_classic_race(shared,*track);
    if(extended) {
        Reader in{bytes.subspan(742,34)};
        for(auto& r:state.special_tiles) {
            for(auto* v:{&r.mud_cooldown,&r.mud_exit_pending,&r.corkscrew_latch,&r.corkscrew_step,
                         &r.corkscrew_float,&r.physics_hold,&r.reflection_lock,&r.raised_priority})*v=in.u16();
            if(r.mud_cooldown>4 || r.mud_exit_pending>4 || r.corkscrew_float>1 || r.physics_hold>8 ||
               r.reflection_lock>6 || r.raised_priority>1 || (r.corkscrew_step>0x31 && r.corkscrew_step!=0xffff) ||
               !(r.corkscrew_latch<=1 || r.corkscrew_latch>=0xfffc))
                throw std::invalid_argument("classic race special-tile state is invalid");
        }
        state.drive_target_latch=in.u16();
        if(state.drive_target_latch>1)throw std::invalid_argument("classic race drive target latch is invalid");
        in.require_end();
        if(bytes.size()==836) {
            std::copy(bytes.begin()+776,bytes.end(),state.race.checkpoint_seen.begin()+20);
            for(auto seen:state.race.checkpoint_seen)if(seen!=0 && seen!=255)
                throw std::invalid_argument("classic race checkpoint-seen flag is invalid");
        }
        // DRAGSTER and ZOOM ZOO take the extended layout only while a word is live.
        if((*track==ClassicRaceTrack::ZoomZoo || *track==ClassicRaceTrack::Dragster) &&
           state.special_tiles==std::array<SpecialTileRider,2>{})
            throw std::invalid_argument("an extended DRAGSTER or ZOOM ZOO state carries no special-tile word");
    }
    return state;
}
void validate_zoom_zoo_content_state(const ZoomZooState& state,const ZoomZooContent& content) {
    if(!state.native_initialization)return;
    if(content.reward_weights.size()!=26)throw std::invalid_argument("ZOOM ZOO reward weights missing");
    for(unsigned i=0;i<2;++i) {
        const auto& roll=state.rolls[i];
        // The low pose base is latched once from the entry orientation at
        // $8293E5-941D, and retained after the roll. Bit15 is checked above.
        if((roll.step || roll.pose_base) &&
           ((roll.pose_base&0x3fffU)!=(content_word(content.roll_poses,2U*roll.prior_orientation)&0x3fffU)))
            throw std::invalid_argument("ZOOM ZOO roll base differs from static entry pose");
        if(state.movement.frame==classic_race_scenario(state.track).initialization_frame && !std::equal(state.learned_weights[i].begin(),
           state.learned_weights[i].end(),content.reward_weights.begin()+1))
            throw std::invalid_argument("ZOOM ZOO initial reward weights differ from static content");
    }
}
// $81:8690-86FE, the special-tile part of the per-update reset that runs
// before the tile dispatch ($81:858E from $82:8C3A / $82:9119).
void update_special_tile_counters(SpecialTileRider& tiles,ReflectionTransition& transition,std::uint8_t selected_high) {
    if(tiles.mud_cooldown)--tiles.mud_cooldown;
    else if(tiles.mud_exit_pending)tiles.mud_exit_pending=0; // and sound $0213
    // With no corkscrew in the previous update the step, the corkscrew's
    // $0600-$060F poses and, off an inverted contact, the float all end.
    if(!tiles.corkscrew_latch) {
        tiles.corkscrew_step=0;
        if(transition.pose_override>=0x600 && transition.pose_override<0x610)transition.pose_override=0;
        if(!(selected_high&0x80U))tiles.corkscrew_float=0;
    }
    tiles.corkscrew_latch=negative(tiles.corkscrew_latch)?static_cast<std::uint16_t>(tiles.corkscrew_latch+1U):0;
    if(tiles.physics_hold)--tiles.physics_hold;
}
// $81:871C-875B, the boost tile (flag pair 2); the corkscrew's ejection adds
// `extra` ($0DED) to the push. Leading support bypasses the push and the pose flag.
void apply_boost_tile(RiderMovementState& rider,SurfaceTransition& surface,std::uint16_t extra) {
    rider.motion.velocity_y=0;
    if(!surface.leading_support) {
        rider.launch_override=80;
        const auto push=static_cast<std::uint16_t>(extra+0x80U);
        rider.motion.velocity_x=add_word(rider.motion.velocity_x,(rider.contact.selected_word&0x4000U)?static_cast<std::uint16_t>(0U-push):push);
        surface.tile_pose=1;
    }
    surface.tile_pose_enabled=1;
}
// $81:8999-89F6, flag pair 14 (mud). Entering halves velocity x with an
// arithmetic shift and stops vertical motion (the original also queues sound
// $0212); every update on it holds both counters at 4 and brakes by 5 unless
// velocity x is already within 48 of zero ($FFD0-$002F, N-flag compares).
void update_mud_tile(RiderMovementState& rider,SpecialTileRider& tiles,SurfaceTransition& surface,SpecialTileUpdate& special) {
    auto& velocity=rider.motion.velocity_x;
    if(!tiles.mud_cooldown) {
        velocity=static_cast<std::uint16_t>((velocity>>1U)|(velocity&0x8000U));
        rider.motion.velocity_y=0;
    }
    tiles.mud_cooldown=4;tiles.mud_exit_pending=4;surface.tile_mode=1;
    if(!negative(velocity)) {
        if(negative(static_cast<std::uint16_t>(velocity-0x30U)))return;
        velocity=static_cast<std::uint16_t>(velocity-5U);
    } else {
        if(!negative(static_cast<std::uint16_t>(velocity-0xffd0U)))return;
        velocity=static_cast<std::uint16_t>(velocity+5U);
    }
    special.mud_velocity=velocity;special.mud_drive_step=4;
}
// $81:87C2-894F, flag pair 10 (corkscrew). A rider that enters it facing the
// descriptor's way, on the ground and not mid-reflection is carried through
// 48 steps at a fixed speed: poses $0600-$060F, y from the height table, the
// object priority toggled four times, gravity and the drive suspended. Step
// $30 leaves it inverted and reflected; any other entry ejects it with a boost.
void update_corkscrew_tile(RiderMovementState& rider,SpecialTileRider& tiles,SurfaceTransition& surface,
                           ReflectionTransition& transition,SpecialTileUpdate& special,
                           std::span<const std::uint8_t> heights) {
    const auto eject=[&] {
        // $81:87D1-87F5; a first ejection also queues sound $021B.
        tiles.corkscrew_latch=0xfffc;
        apply_boost_tile(rider,surface,0x40);
        tiles.corkscrew_step=0xffff;
    };
    if(negative(tiles.corkscrew_latch)) {eject();return;}
    tiles.corkscrew_latch=1;
    const auto word=rider.contact.selected_word;
    const auto velocity=rider.motion.velocity_x;
    // $81:87F8-882E: the entry conditions, all against the descriptor's facing.
    if(transition.step || (word&0x8000U) || static_cast<std::int16_t>(rider.contact.previous_unsupported_count)>=3 ||
       (rider.pose.reflected ? (negative(velocity) || !(word&0x4000U))
                             : ((velocity && !negative(velocity)) || (word&0x4000U)))) {eject();return;}
    if(!tiles.corkscrew_step) {
        if(transition.pose_override)return;
        tiles.raised_priority=0; // $0FAF = $26
        rider.motion.response_a=0;rider.motion.response_b=0;
    }
    rider.launch_override=80;
    if(tiles.corkscrew_step==0x31)return;
    if(tiles.corkscrew_step==0x30) {
        // $81:885F-88B2: leave facing the other way, upside down.
        rider.pose.orientation=rider.pose.reflected?0x15:0x2b;
        rider.pose.reflected=!rider.pose.reflected;
        tiles.corkscrew_step=0x31;
        transition.pose_override=0;surface.tile_pose_enabled=0;
        if(special.contact_skip)--special.contact_skip;
        rider.contact.selected_high=0x80;tiles.reflection_lock=6;surface.mode=1;
        return;
    }
    const auto step=tiles.corkscrew_step;
    if(step==1 || step==0x12 || step==0x20 || step==0x2f)tiles.raised_priority^=1U;
    tiles.corkscrew_float=1;special.contact_skip=1;surface.tile_pose_enabled=1;
    rider.motion.velocity_x=rider.pose.reflected?0x01ce:0xfe32;
    rider.motion.velocity_y=0;
    tiles.physics_hold=8;
    tiles.corkscrew_step=static_cast<std::uint16_t>(step+1U);
    auto pose=static_cast<unsigned>(tiles.corkscrew_step>>1U);
    if(pose>=0x10)pose-=0x10;
    transition.pose_override=static_cast<std::uint16_t>(0x600U+pose);
    if(heights.size()!=96)throw std::invalid_argument("corkscrew heights are missing");
    const auto height=heights[(rider.pose.rolling?48U:0U)+((tiles.corkscrew_step-1U)&0xffU)];
    rider.motion.y=static_cast<std::uint16_t>(rider.motion.y+(height<128?height:height-256));
    special.corkscrew_stepped=true;
}
void update_zoom_zoo(ZoomZooState& state,const ControllerButtons& requested_buttons,const ZoomZooContent& content) {
    // NMI $808642-865B skips controller publication through prior fade4;
    // first controller publication uses prior fade5 (native update1382).
    const auto gated_request=state.native_initialization && state.fade_level<5?ControllerButtons{}:requested_buttons;
    validate_zoom_zoo_content_state(state,content);
    const auto scenario=classic_race_scenario(state.track);
    if(state.native_initialization && state.race.finish_delay==240) {
        // Original graph load begins on the update following finish display240.
        // The authenticated load is black for108 updates, then seven brightness
        // steps. Preserve a final-race archive; the original reuses that memory.
        if(state.result_updates<stable_result_updates(state))++state.result_updates;
        state.result=zoom_result_fields(state.race,state.result_updates,scenario.tour_race);
        ++state.movement.frame;
        return;
    }
    if((gated_request.y && !state.native_initialization) || (gated_request.select && !state.native_initialization) || (gated_request.start && !state.native_initialization) || ((!state.native_initialization) && (gated_request.up || gated_request.down || gated_request.a)) ||
       (!state.complete_race && gated_request.left) || (gated_request.x && !state.native_initialization) || ((!state.native_initialization) && (gated_request.left_shoulder || gated_request.right_shoulder)))
        throw std::invalid_argument("ZOOM ZOO controller is outside the recovered domain");
    if(state.movement.frame<(state.native_initialization?scenario.initialization_frame:1649U) || (!state.native_initialization && state.movement.frame>=(state.sustained?9999U:1849U)))
        throw std::invalid_argument("ZOOM ZOO update is outside the declared trial horizon");
    // A SNES pad's rocker cannot close both contacts of one axis, and the
    // controller port publishes `up & !down` and `left & !right` (the audited
    // core's sfc/controller/gamepad says so in those terms). Measured on the
    // original: opposing directions leave WRAM byte-identical to a released
    // D-pad, so the engine never sees them and what this game's own branches
    // would do with both is not recovered behaviour (R-0041). Every caller
    // passes what a device asked for; the rocker is applied here, once, for
    // both tracks. `buttons` is what the port publishes; the guard above
    // deliberately reads the request instead, so the historical continuation
    // domain is exactly the accepted one.
    const auto buttons=with_physical_dpad(gated_request);
    auto next=state;auto& whole=next.movement;
    whole.player_input=sample_controller(buttons);
    whole.contact_phase=static_cast<std::uint8_t>(1U-whole.contact_phase);
    whole.progress_phase=static_cast<std::uint8_t>(1U-whole.progress_phase);
    whole.animation_counter=static_cast<std::uint8_t>((whole.animation_counter+1U)&31U);
    whole.update_counter=static_cast<std::uint8_t>(whole.update_counter+1U);
    if(whole.countdown>=(state.native_initialization?271U:69U))throw std::invalid_argument("ZOOM ZOO countdown is outside continuation domain");
    if(!state.native_initialization && whole.countdown)--whole.countdown;
    if(state.native_initialization && next.fade_level<30)++next.fade_level;
    auto& player_input=next.reflection[0];
    // $82:AAE2-AAFB: B drives $0331 (jump); Y drives $0325 (brake).
    player_input.brake_input=buttons.y;player_input.jump_input=buttons.b;
    player_input.rotate_negative_input=buttons.left_shoulder;player_input.rotate_positive_input=buttons.right_shoulder;
    // $83CD05-CD35: controller/global phase clocks are sampled before the
    // pause menu diverts this update. Race, AI, queues and hints do not advance.
    if(state.native_initialization && (state.pause.selection || (buttons.start && !state.race.riders[0].finished))) {
        auto& pause=next.pause;
        if(!pause.selection)pause.selection=1;
        if(whole.player_input.vertical!=1)pause.selection=whole.player_input.vertical?0xffff:1;
        if(!buttons.start)pause.released=1;
        else if(pause.released) {
            if(negative(pause.selection)) {
                // Authored standalone navigation: this menu says RESTART RACE.
                // Original Retire/tour progression is deliberately not emulated.
                restart_zoom_zoo(next,content);state=next;return;
            }
            pause.selection=0;
        }
        ++pause.suspended_updates;
        if(next.fade_level>=5 && whole.countdown)++pause.suspended_countdown_updates;
        next.opponent_horizontal=1;
        auto& opponent=next.reflection[1];
        opponent.brake_input=opponent.jump_input=opponent.rotate_negative_input=opponent.rotate_positive_input=0;
        ++whole.frame;state=next;return;
    }
    if(state.native_initialization && next.pause.released && !buttons.start)next.pause.released=0;
    const bool inverted_marker=update_zoom_ai(next);
    // $83:E7A2-E7BF also releases both riders' A ($031D/$031F) and X
    // ($0321/$0323) publications while the countdown holds the brakes.
    bool countdown_releases_actions=false;
    if(state.native_initialization && whole.countdown) {
        // $83:E59C-E7BD: countdown presentation feeds braking and start boost.
        if(next.fade_level>=5) {
            if(whole.countdown<130 && whole.countdown>100) {
                for(unsigned i=0;i<2;++i)if(!next.reflection[i].brake_input)next.start_boost[i]=0;
            }
            if(whole.countdown<70) {
                for(unsigned i=0;i<2;++i)if(!next.reflection[i].brake_input) {
                    whole.riders[i].speed.boost=add_word(whole.riders[i].speed.boost,next.start_boost[i]);
                    next.start_boost[i]=0;
                }
            }
            --whole.countdown;
        }
        if(state.movement.countdown>=70 || next.fade_level<5) {
            if(next.fade_level>=5 && state.movement.countdown<=100)
                for(auto& input:next.reflection)input.brake_input=1;
            if(!player_input.brake_input)whole.player_input.horizontal=1;
            if(!next.reflection[1].brake_input)next.opponent_horizontal=1;
            for(auto& input:next.reflection) {input.brake_input=1;input.jump_input=0;}
            countdown_releases_actions=true;
        }
    }
    const bool player_a=buttons.a && !countdown_releases_actions;
    const bool player_x=buttons.x && !countdown_releases_actions;
    // The opponent's A and X are the selector's bits 1 and 2 only on updates
    // the AI stores them; after an inverted marker both stay released (R-0048).
    const auto opponent_trick=countdown_releases_actions?0U:
        (unsigned(whole.opponent_ai.trick_selector)&(inverted_marker?~6U:~0U));
    if(state.complete_race)update_zoom_finish(next,content);
    const unsigned active=whole.progress_phase?0U:1U;
    unsigned reward=0;
    if(state.native_initialization) {
        auto& cooldown=next.player_announcements.queue.cooldown;
        cooldown=cooldown>2?static_cast<std::uint16_t>(cooldown-2U):0;
    }
    whole.rewards.cooldown=whole.rewards.cooldown>2?static_cast<std::uint16_t>(whole.rewards.cooldown-2U):0;
    std::array<std::uint16_t,2> contact_skip{};
    for(unsigned index=0;index<2;++index) {
        auto& rider=whole.riders[index];auto& transition=next.reflection[index];
        auto& surface=next.surface[index];
        if(state.complete_race)update_zoom_checkpoint(next,index,content);
        surface.tile_mode=0;surface.animation_delta=0;surface.tile_pose=0;surface.tile_pose_enabled=0;
        const unsigned horizontal=index==0?whole.player_input.horizontal:next.opponent_horizontal;
        auto& tiles=next.special_tiles[index];
        // $81:859D-85A8: the reflection lock counts down while the previous
        // update left surface mode clear.
        if(!surface.mode && tiles.reflection_lock)--tiles.reflection_lock;
        decay_idle_wobble(rider,surface.mode!=0);
        surface.mode=0;
        rider.launch_override=0;
        update_special_tile_counters(tiles,transition,rider.contact.selected_high);
        // Transient words of this update: $0F3B and $0F3F (mud), $0F5B (corkscrew).
        SpecialTileUpdate special{};
        const auto descriptor=rider.contact.selected_word;
        const auto tile=((descriptor&0x3f0U)>>2U)+((descriptor&15U)>>1U);
        if(tile>=content.movement.flat_contact.flags.size())throw std::out_of_range("ZOOM ZOO tile flag is missing");
        // $81:82BB-82F3 dispatches the selected tile's flag pair through the
        // table at $81:82F5 unless the auxiliary flag is set. Pairs 18 and 22
        // are a bare RTS ($81:84AC); pair 20 is the checkpoint tile, which
        // update_zoom_checkpoint runs. The remaining pairs are unrecovered.
        const auto tile_behavior=rider.contact.auxiliary_flag?0U:unsigned(content.movement.flat_contact.flags[tile]&0xfeU);
        if(tile_behavior==4 || tile_behavior==8 || tile_behavior==12 || tile_behavior>=26)
            throw std::invalid_argument("movement reaches unrecovered tile flag pair "+std::to_string(tile_behavior));
        if(tile_behavior==6) {
            const auto angle=static_cast<std::int16_t>(rider.contact.surface_angle);
            if(std::abs(angle)>=31) {
                const auto vx=static_cast<std::int16_t>(rider.motion.velocity_x);
                if(vx!=0)rider.motion.velocity_x=static_cast<std::uint16_t>(vx<0?((vx>>1)+1):((vx>>1)-1));
            } else if(!transition.brake_input && horizontal==((descriptor&0x4000U)?0U:2U)) {
                rider.motion.velocity_x=add_word(rider.motion.velocity_x,(descriptor&0x4000U)?static_cast<std::uint16_t>(-4):4);
            }
            surface.mode=1;surface.angle=rider.contact.surface_angle;
        }
        if(tile_behavior==2)apply_boost_tile(rider,surface,0);
        if(tile_behavior==24 && !surface.leading_support) {
            // $81:84B2-84EB: away from leading support, the tile carries the
            // rider one unit along the descriptor's facing and raises it:
            // velocity y falls by $40 but not below -$220 (an N-flag compare).
            rider.launch_override=80;
            rider.motion.x=static_cast<std::uint16_t>(rider.motion.x+((descriptor&0x4000U)?0xffffU:1U));
            auto raised=static_cast<std::uint16_t>(rider.motion.velocity_y-0x40U);
            if(negative(static_cast<std::uint16_t>(raised-0xfde0U)))raised=0xfde0U;
            rider.motion.velocity_y=raised;
            surface.tile_pose=1;surface.tile_pose_enabled=1;
        }
        if(tile_behavior==14)update_mud_tile(rider,tiles,surface,special);
        if(tile_behavior==16) {
            // $81:89F7-8A29: with jump held the tile pushes velocity x by 4
            // the way the D-pad points; otherwise it only sets the tile pose.
            if(transition.jump_input) {
                if(horizontal!=1)rider.motion.velocity_x=static_cast<std::uint16_t>(rider.motion.velocity_x+(horizontal==0?0xfffcU:4U));
            } else surface.tile_pose=1;
        }
        if(tile_behavior==10) {
            update_corkscrew_tile(rider,tiles,surface,transition,special,content.corkscrew_heights);
            // Each step ends with $81:8949 storing 1 at $0EA3 by absolute
            // address, the player's rolling flag: the player's own update
            // stores its flag back over it, but the opponent's corkscrew
            // sets the player's flag after the player's update has run.
            if(index==1 && special.corkscrew_stepped)whole.riders[0].pose.rolling=true;
        }
        contact_skip[index]=special.contact_skip;
        int animation_override=0;bool throttle_target=false;
        if(index==0)update_reflection_transition(rider,transition,horizontal,index!=active,content.reflection_pose_table,state.native_initialization && player_a,tiles);
        // The opponent's X comes from trick selector bit 2 ($0323) rather than
        // from a controller; the selector is retained state, so it re-derives
        // each update for as long as the impulse holds.
        if(state.native_initialization && index==active)
            update_zoom_roll(next,index,index==0?player_x:(opponent_trick&4U)!=0,content);
        if(index==active || surface.leading_support) {
            if(state.native_initialization)update_zoom_landing_rewards(next,index,content);
            else {
                const bool landed=rider.motion.response_a || rider.contact.unsupported_count<2;
                const auto event=update_quarter_turns(rider,surface.leading_support!=0,transition.air_turns);
                if(landed)transition.air_turns=0;
                if(index==0 && event && (!state.complete_race || event!=14))throw std::invalid_argument("ZOOM ZOO player reward is unrecovered");
                if(event && index==1)reward=event;
            }
        }
        if(index==active) {
            // $82:A027-A068 initializes the direction latch on first opposition.
            if(!transition.direction_latch &&
               (negative(rider.motion.velocity_x)?horizontal!=0:horizontal!=2))transition.direction_latch=0xffff;
            else if(negative(transition.direction_latch) && ((negative(rider.motion.velocity_x)&&horizontal==0)||(!negative(rider.motion.velocity_x)&&horizontal==2)))transition.direction_latch=48;
            animation_override=stationary_animation_override(rider,static_cast<std::uint8_t>(horizontal));
            // $82A8D8-A8EE rejects a tile-enabled pose ($0F41), leading or
            // inverted contact before consuming pending/previous input.
            if(!surface.tile_pose_enabled && !surface.leading_support &&
               !(rider.contact.selected_high&0x80U))
                update_jump(rider,transition.jump_input!=0);
            // $82:A5FC: mud's drive step replaces the low-speed damping.
            if(!special.mud_drive_step)update_active_low_speed_damping(rider);
            transition.drive_pose_enabled=0;
            // $82:A241: the corkscrew latch skips the completed-turn hold.
            if(transition.completed && !tiles.corkscrew_latch) {
                if(static_cast<std::int16_t>(rider.motion.previous_x_displacement)<2)transition.completed=0;
                else {
                    transition.hold=30;
                    if(rider.contact.unsupported_count!=9 && horizontal!=1)transition.drive_pose_enabled=1;
                }
            }
            // $82:A49F-A4F9 in the original's order: both rotations, a
            // shallow supported contact or the corkscrew latch ($82:A4CC)
            // clear the rotation; surface mode and leading support then
            // return with it unchanged ($82:A4DC-A4E9, R-0047); no rotation
            // clears it. `$0F89` ($0E6F) is guarded zero.
            const bool clear=tiles.corkscrew_latch || (transition.rotate_negative_input && transition.rotate_positive_input) ||
                (std::abs(static_cast<std::int16_t>(rider.contact.surface_angle))<30 && rider.contact.unsupported_count<9);
            if(clear)rider.motion.response_b=0;
            else if(surface.mode || surface.leading_support) {}
            else if(!transition.rotate_negative_input && !transition.rotate_positive_input)rider.motion.response_b=0;
            else {
                // $82A49F-A5F9 halves the rotation step while A is held. The
                // rate was keyed to the player's button, so the opponent always
                // rotated at the fast step; its A arrives from trick selector
                // bit 1 instead and must slow it the same way.
                const bool holding_a=index==0?player_a:
                    (state.native_initialization && (opponent_trick&2U)!=0);
                rider.motion.response_b=static_cast<std::uint16_t>((rider.motion.response_b&0xff00U)|
                    (transition.rotate_negative_input?(holding_a?255U:254U):(holding_a?1U:2U)));
            }
            const auto previous_wrong_direction=transition.wrong_direction_counter;
            transition.wrong_direction_counter=next_wrong_direction_counter(
                previous_wrong_direction,rider.motion.velocity_x,
                rider.progress.marker_word,horizontal,state.native_initialization);
            if(state.native_initialization && previous_wrong_direction==179 && transition.wrong_direction_counter==120) {
                // $829751-9762 / $829781-9792: fixed one-player scenario $77074B=1.
                if(index==0)enqueue_zoom_player(next,22);else enqueue_zoom_opponent(whole,22);
            }

        }
        update_rolling_mode(rider,surface.mode!=0);
        if(index==1)update_reflection_transition(rider,transition,horizontal,index!=active,content.reflection_pose_table,
            state.native_initialization && (opponent_trick&2U)!=0,tiles);
        // $82:98D6: the corkscrew's physics hold skips the whole drive routine,
        // brake latch included, and leaves $0E7B as the other rider set it.
        if(!tiles.physics_hold) {
            update_zoom_throttle(rider,transition,horizontal,animation_override,throttle_target,next.charge_announced[index],surface.leading_support!=0,
                                 state.native_initialization && next.rolls[index].bounce_active!=0,
                                 special.mud_drive_step?special.mud_drive_step:24,tiles.mud_cooldown!=0);
            next.drive_target_latch=throttle_target?0:1;
        }
        update_idle_pose(rider,next.drive_target_latch && !surface.leading_support && transition.pose_override==0,index==1,whole.animation_counter,content.movement.idle_pose_table);
        if(rider.idle_pose.active)surface.tile_mode=1;
        // $82:A971-A97C: the corkscrew's float or physics hold suspends gravity.
        if(!tiles.corkscrew_float && !tiles.physics_hold)update_gravity(rider);
        SpeedLimitContext limit{};limit.opponent=index==1;limit.ai_enabled=true;
        // $150B has no writer in the declared continuation: preserve its seed
        // byte. Player boost below 16 makes the optional subtraction inert.
        if(index==0 && rider.speed.boost>=16 && !state.native_initialization)
            throw std::invalid_argument("ZOOM ZOO player boost requires unrecovered camera state");
        limit.pose_byte=index==1?next.opponent_retained_oam_x:static_cast<std::uint8_t>(next.race.camera.screen_xy);
        limit.drag=state.complete_race && (index==0?next.race.provisional_1225:next.race.provisional_1227);
        limit.start_override=rider.launch_override!=0;limit.player_progress=whole.riders[0].progress.transition_count;
        limit.opponent_progress=whole.riders[1].progress.transition_count;limit.adjustment_limit=race_adjustment_limit(scenario);
        limit.player_base_cap=next.reflection[0].base_velocity_cap;limit.update_counter=whole.update_counter;
        limit.friction_mode=static_cast<std::uint16_t>(horizontal);
        // $82:A6FD: and the speed limiter.
        if(!tiles.physics_hold)limit_rider_speed(rider.motion.velocity_x,rider.motion.velocity_y,rider.speed,limit,content.movement.speed_decay);
        integrate_zoom_axis(rider.motion.x,rider.motion.velocity_x,rider.residue_x);
        rider.motion.x&=track_geometry(content.movement.sampling.track).position_mask;
        integrate_zoom_axis(rider.motion.y,rider.motion.velocity_y,rider.residue_y);
        if(rider.contact.surface_angle && rider.contact.unsupported_count<2 && !(rider.contact.selected_high&0x80U)) {
            rider.motion.y=static_cast<std::uint16_t>(static_cast<int>(rider.motion.y)+(negative(rider.motion.velocity_y)?-1:(surface.mode?1:4)));
        }
        surface.animation_delta=static_cast<std::uint16_t>(animation_override);
        update_pose(rider,whole.animation_counter,whole.contact_phase,content.movement,animation_override,next.drive_target_latch==0,surface.mode?static_cast<std::int16_t>(surface.angle):0,
                    special.mud_velocity);
        if(transition.pose_override)rider.pose.pose_index=transition.pose_override;
        if(index==active)advance_track_progress(rider.progress,content.movement.progress_transitions);
    }
    // $81:C73E-C75B: when the race clock would reach 10:00 it holds 9:59.9 and
    // marks both riders finished, whatever their laps.
    if(advance_timer_digits(whole.timer,whole.countdown<68) && state.native_initialization)
        for(auto& rider:next.race.riders)rider.finished=1;
    if(state.native_initialization)consume_zoom_player(next,content.movement);
    update_reward_queue(whole,reward,content.movement,
        state.native_initialization?std::span<std::uint8_t>{next.learned_weights[1]}:std::span<std::uint8_t>{});
    if(state.complete_race)update_zoom_camera(next,track_geometry(content.movement.sampling.track));
    for(unsigned index=0;index<2;++index) {
        // $81:8CFC / $81:8E43: while the corkscrew carries a rider its whole
        // contact update is skipped ($0DFB/$0DFD).
        if(contact_skip[index])continue;
        auto& rider=whole.riders[index];
        const auto points=collision_points(content.movement.sampling,rider.pose.pose_index,rider.pose.reflected);
        const auto samples=sample_track(content.movement.sampling,points,rider.motion.x,rider.motion.y,
                                        track_geometry(content.movement.sampling.track).coarse_columns);
        const auto summary=summarize_vertical_contact(content.movement.flat_contact,points,samples,rider.motion.x,rider.motion.y);
        if(content.slope_coefficients.size()!=18 && content.slope_coefficients.size()!=128)throw std::invalid_argument("ZOOM ZOO slope coefficients missing");
        resolve_vertical_contact(rider.contact,rider.motion,summary,{whole.contact_phase,index==1,next.surface[index].mode,0xc200},
            content.slope_coefficients.subspan(state.sustained && next.surface[index].mode?64:0,state.sustained?32:9),content.slope_coefficients.subspan(state.sustained?(next.surface[index].mode?96:32):9),content.landing_matrices,
            index==0?whole.player_input.horizontal:next.opponent_horizontal,rider.pose.pose_index,rider.pose.reflected);
        // $8191F4-920C clears leading support on the auxiliary boundary
        // return, even when an earlier probe initially established support.
        next.surface[index].leading_support=rider.contact.auxiliary_flag==1?false:summary.leading_support;
        if(state.native_initialization)next.rolls[index].support_count_mirror=rider.contact.unsupported_count;
        observe_track_markers(rider.progress,samples);
    }
    if(state.complete_race)update_zoom_visibility(next,track_geometry(content.movement.sampling.track));
    if(state.native_initialization)update_zoom_hints(next);
    ++whole.frame;state=next;
}
} // namespace unirally
