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
    for (unsigned i=0; i<count; ++i) {
        result = static_cast<std::uint16_t>((result >> 1U) | (result & 0x8000U));
    }
    return result;
}
struct Probe { std::uint8_t penetration{0xa0}, angle{}; std::uint16_t descriptor{}; std::uint8_t direction{}; };
Probe preprocess(const FlatContactContent& content, SamplePoint point,
                 std::uint16_t descriptor, std::uint16_t x, std::uint16_t y) {
    if ((descriptor & 0x03ffU) == 0) {
        if((descriptor&0x1c00U)==0x1c00U)return {0x7f,0,descriptor};
        return {};
    }
    require((descriptor & 1U)==0,"vertical contact reaches special remapped descriptor");
    const auto index=tile(descriptor);
    const bool horizontal=(byte(content.flags,index)&1U)!=0;
    auto column=(static_cast<unsigned>(point.x)+(x&15U))&15U;
    auto local_y=(static_cast<unsigned>(point.y)+(y&15U))&15U;
    if(!horizontal && (descriptor&0x8000U))local_y=(~local_y)&15U;
    const bool mirrored=(descriptor&0x4000U)!=0;
    if (mirrored) column=(~column)&15U;
    if(horizontal)std::swap(column,local_y);
    const auto height=byte(content.columns,index*32U+column*2U);
    auto angle=byte(content.columns,index*32U+column*2U+1U);
    if (mirrored) angle=static_cast<std::uint8_t>(0U-angle);
    const auto penetration=height==0xa0U ? std::uint8_t{0xa0} :
        static_cast<std::uint8_t>(local_y-static_cast<std::uint8_t>(height-1U));
    return {penetration,angle,descriptor,static_cast<std::uint8_t>(horizontal?(mirrored?3:4):((descriptor&0x8000U)?1:0))};
}
} // namespace

VerticalContactSummary summarize_vertical_contact(const FlatContactContent& content,
                                              const CollisionPoints& points,
                                              const TrackSamples& samples,
                                              std::uint16_t x,std::uint16_t y) {
    std::array<Probe,10> probes{};
    for (std::size_t i=0;i<probes.size();++i) probes[i]=preprocess(content,points[i],samples[i],x,y);
    VerticalContactSummary result{};
    std::uint8_t support=probes[0].penetration==0xa0U ? 0xff : probes[0].penetration, angle=0xe0;
    // $81:8FEF-902D initializes an ordinary vertical first probe before
    // the remaining ordered reduction. A later winner can clear $0F5D.
    if(probes[0].penetration<0x80U) {
        result.leading_support=true;
        result.any_nonnegative_probe=true;
        if(probes[0].direction<2) {
            result.penetration=probes[0].penetration;
            result.inverted_vertical=probes[0].direction==1;
        } else {result.horizontal_penetration=probes[0].penetration;result.horizontal_direction=probes[0].direction;}
        angle=probes[0].angle;
        result.selected_word=probes[0].descriptor;
        result.selected_high=static_cast<std::uint8_t>(probes[0].descriptor>>8U);
    }
    for (std::size_t i=1;i<probes.size();++i) {
        const auto& probe=probes[i];
        if (probe.penetration==0xa0U) {
            if ((probe.descriptor&0x01ffU)!=0 && result.selected_word==0) result.selected_word=probe.descriptor;
            continue;
        }
        if (nonnegative_difference(probe.penetration,support)) {
            support=probe.penetration;
            result.leading_support=i<2 && probe.penetration<0x80U;
            if ((probe.descriptor&1U)!=0 || result.selected_word==0) result.selected_word=probe.descriptor;
            if (nonnegative_difference(probe.angle,angle)) result.selected_high=static_cast<std::uint8_t>(probe.descriptor>>8U);
            angle=probe.angle;
        }
        if(probe.penetration<0x80U)result.any_nonnegative_probe=true;
        if (probe.direction<2 && probe.penetration<0x80U && nonnegative_difference(probe.penetration,result.penetration)) {
            result.penetration=probe.penetration;
            result.inverted_vertical=(probe.descriptor&0x8000U)!=0;
            result.angle=static_cast<std::int16_t>(probe.angle<128 ? static_cast<int>(probe.angle) : static_cast<int>(probe.angle)-256);
        }
    }
    for(const auto& probe:probes) {
        if(probe.direction>=2 && probe.penetration<128 && nonnegative_difference(probe.penetration,result.horizontal_penetration)) {
            result.horizontal_penetration=probe.penetration;result.horizontal_direction=probe.direction;
        }
    }
    result.supported=support<0x80U;
    result.angle=static_cast<std::int16_t>(angle<128 ? static_cast<int>(angle) : static_cast<int>(angle)-256);
    result.boundary_marker=result.penetration==127;
    result.tile_flags=byte(content.flags,tile(result.selected_word));
    return result;
}

void resolve_vertical_contact(RiderContactState& rider,ContactMotion& motion,
                              const VerticalContactSummary& summary,const ContactContext& context,
                              std::span<const std::uint8_t> shifts,
                              std::span<const std::uint8_t> multipliers,
                              std::span<const std::uint8_t> landing_matrices,unsigned horizontal, unsigned pose_index,bool reflected) {
    require(context.phase<=1 && context.mode<=1,"unsupported vertical contact phase/mode");
    require(rider.unsupported_count<=9 && summary.penetration<128,"unsupported vertical contact state");

    // $81:9185-91D1 dispatches on the selected tile's flag pair (flag & $FE)
    // before the auxiliary and support tests. Pair 24 clears the unsupported
    // count $0F33 and duration $0FBF as whole words, after $81:8F9A has
    // snapshotted the incoming count. Pairs 8 and 16 set $1349, read only at
    // $81:9685 on a path ($81:966F-9690) that continued contact enters only
    // for a magnitude of 31 or more, which the response has already taken
    // ($81:9286); no capture executes it, so for pair 16 it is inert. Pair 8
    // also changes the correction ($81:92D9, $81:96FF, $81:97E6) and pair 26
    // can clear probe penetrations $28/$2C; both stay unrecovered. Every other
    // pair takes no branch here (R-0047).
    const auto flag_pair=static_cast<unsigned>(summary.tile_flags&0xfeU);
    require(flag_pair!=26,"vertical contact reaches unrecovered tile flag pair 26");
    auto incoming=rider;
    if(flag_pair==24) {incoming.unsupported_count=0; incoming.unsupported_duration=0;}
    auto next=incoming; auto moved=motion;
    next.previous_unsupported_count=rider.unsupported_count;
    next.selected_word=summary.selected_word;
    next.selected_high=summary.selected_high;
    next.recontact=false;
    next.angle_unspecified=true;
    if(!summary.any_nonnegative_probe)next.auxiliary_flag=0;
    if(summary.boundary_marker)next.auxiliary_flag=1;
    if(next.auxiliary_flag==1) {
        next.unsupported_duration=static_cast<std::uint16_t>((incoming.unsupported_duration&0xff00U)|static_cast<std::uint8_t>(incoming.unsupported_duration+1U));
        next.unsupported_count=std::min<std::uint16_t>(9,static_cast<std::uint16_t>(incoming.unsupported_count+1U));
        rider=next;return;
    }
    if (!summary.supported) {
        next.unsupported_count=std::min<std::uint16_t>(9,static_cast<std::uint16_t>(incoming.unsupported_count+1U));
        next.unsupported_duration=static_cast<std::uint16_t>(incoming.unsupported_duration+1U);
        next.angle_unspecified=true;
        next.auxiliary_flag=0;
    } else {
        const auto magnitude=static_cast<unsigned>(std::abs(static_cast<int>(summary.angle)));
        require(magnitude<shifts.size(),"vertical response angle outside recovered coefficients");
        require(flag_pair!=8,"vertical contact reaches unrecovered tile flag pair 8");
        // $81:924E–9275 removes motion into the inverted contact face.
        if(signed_word(moved.velocity_y)<0 && (summary.selected_high&0x80U) &&
           ((summary.selected_high&0x40U)?signed_word(moved.velocity_x)<0:signed_word(moved.velocity_x)>=0))moved.velocity_x=0;
        next.surface_angle=static_cast<std::uint16_t>(summary.angle);
        next.angle_unspecified=magnitude==31;
        next.unsupported_count=0; next.unsupported_duration=0;
        if(magnitude>=31) {
            moved.velocity_x=arithmetic_shift(moved.velocity_x,2);
            next.unsupported_count=std::min<std::uint16_t>(9,static_cast<std::uint16_t>(incoming.unsupported_count+1U));
            next.unsupported_duration=static_cast<std::uint16_t>(incoming.unsupported_duration+1U);
        } else if (incoming.unsupported_count>=9 || (summary.leading_support && incoming.unsupported_count>=2)) {

            if(magnitude<28) {
            // R-0025: signed displacement quadrant and coarse-angle sentinel.
            const auto dx=std::abs(signed_word(static_cast<std::uint16_t>(motion.x-incoming.previous_uncorrected_x)));
            const auto half_dy=std::abs(signed_word(static_cast<std::uint16_t>(motion.y-incoming.previous_uncorrected_y)))/2;
            // $81:984D-98A8 uses bounded subtraction, not division. A zero
            // subtrahend still terminates at the angle endpoint (4 or 31).
            const int magnitude_angle=dx>=half_dy ?
                (half_dy==0 ? 4 : std::max(4,16-4*(dx/half_dy))) :
                (dx==0 ? 31 : std::min(31,16+4*(half_dy/dx)));
            const int coarse=signed_word(static_cast<std::uint16_t>(motion.x-incoming.previous_uncorrected_x))<0 ? -magnitude_angle : magnitude_angle;
            require((context.cartridge_options&8U)==0, "unrecovered landing option");
            // M4-13 authenticates $132B == 0 throughout the domain; player
            // selection alone does not replace the matrix ($81:94A8-94C3).
            next.recontact=true;
            // $81:931C–9358 uses the reflected low six pose bits, not the
            // surface target. All comparisons below are signed original words.
            int pose_direction=static_cast<int>(pose_index&63U);
            if(reflected && pose_direction)pose_direction=64-pose_direction;
            const int surface_direction=summary.angle<0 ? 62+summary.angle : summary.angle;
            const bool negative_dx=signed_word(static_cast<std::uint16_t>(motion.x-incoming.previous_uncorrected_x))<0;
            const bool negative_dy=signed_word(static_cast<std::uint16_t>(motion.y-incoming.previous_uncorrected_y))<0;
            int orientation_angle=(negative_dx!=negative_dy)?-coarse:coarse;
            orientation_angle=(!negative_dx && !negative_dy) ?
                std::max(orientation_angle,static_cast<int>(summary.angle)) :
                std::min(orientation_angle,static_cast<int>(summary.angle));
            const int coarse_direction=orientation_angle<0 ? 62+orientation_angle : orientation_angle;
            const int direction_difference=(coarse_direction-surface_direction+62)%62;
            bool force_long_airtime_matrix=false;
            if(direction_difference!=0 && direction_difference!=31) {
                const int displacement=signed_word(motion.previous_x_displacement);
                int response;
                if(direction_difference>31) {
                    response=(displacement<3 && pose_direction<32)?-1:
                        (displacement<1?1:std::max(-2,-1-static_cast<int>(motion.previous_x_displacement>>4U)));
                } else {
                    response=(displacement<3 && pose_direction>32)?1:
                        (displacement<1?1:std::min(2,static_cast<int>(motion.previous_x_displacement>>4U)+1));
                }
                if(summary.selected_high&0x80U)response=0;
                moved.response_a=static_cast<std::uint16_t>(response);
                if(!summary.leading_support) {
                    if(response) {
                        const auto impulse=static_cast<std::uint16_t>((motion.previous_x_displacement>>2U)+1U);
                        moved.orientation_impulse=response<0?static_cast<std::uint16_t>(0U-impulse):impulse;
                    }
                    if(context.mode==0 && summary.angle<30 && summary.angle>=-30 && incoming.unsupported_duration>=120 && pose_direction>=32) {
                        moved.response_a=static_cast<std::uint16_t>(response<0?1:-1);
                        force_long_airtime_matrix=true;
                    } else moved.response_a=0;
                }
            } else {
                auto decay=[](std::uint16_t value) {return static_cast<std::uint16_t>(value+(signed_word(value)<0?1:(value?-1:0)));};
                moved.response_a=decay(moved.response_a);moved.response_b=decay(moved.response_b);
            }
            const auto angle_difference=std::abs(coarse-static_cast<int>(summary.angle));
            if(angle_difference>5 || summary.leading_support || force_long_airtime_matrix) {
                require(landing_matrices.size()==1512,"landing coefficient matrices are missing");
                unsigned bucket=angle_difference?static_cast<unsigned>(std::min(3,(angle_difference-1)/5)):0U;
                if(summary.leading_support)bucket=bucket>1?bucket-1:1;
                unsigned matrix=(bucket<=1 && !force_long_airtime_matrix)?1U:2U;
                int angle=(summary.selected_high&0x80U)?-summary.angle:summary.angle;
                const auto velocity=signed_word(moved.velocity_x);
                if((velocity>0 && horizontal==0) || (velocity<0 && horizontal==2)) {
                    angle=std::clamp(angle+(velocity>0?-5:5),-31,31);matrix=2;
                }
                const auto angle_index=static_cast<unsigned>(angle<0?31-angle:angle);
                const auto offset=matrix*504U+angle_index*8U;
                auto multiply=[&](std::uint16_t value,unsigned coefficient) {
                    const auto low=byte(landing_matrices,offset+coefficient*2U);
                    const auto high=byte(landing_matrices,offset+coefficient*2U+1U);
                    const auto raw=high?high:low;
                    const auto signed_coefficient=raw<128?static_cast<int>(raw):static_cast<int>(raw)-256;
                    const int product=signed_word(value)*signed_coefficient;
                    if(high)return static_cast<std::uint16_t>(product);
                    // $0566 is the middle/high product word. ASL follows
                    // selection, so negative products round down before doubling.
                    const int upper=product>=0?product/256:-((-product+255)/256);
                    return static_cast<std::uint16_t>(upper*2);
                };
                const auto vx=moved.velocity_x,vy=moved.velocity_y;
                moved.velocity_y=static_cast<std::uint16_t>(multiply(vy,0)+multiply(vx,1));
                moved.velocity_x=static_cast<std::uint16_t>(multiply(vy,2)+multiply(vx,3));
            }
            } else next.recontact=true;
        } else {
            if (!summary.leading_support && context.phase==0) {moved.response_a=0; moved.response_b=0;}
            const auto shifted=arithmetic_shift(motion.velocity_x,byte(shifts,magnitude));
            const auto product=static_cast<std::uint16_t>(static_cast<unsigned>(shifted)*byte(multipliers,magnitude));
            if(!summary.leading_support && magnitude<26) {
                moved.velocity_y=summary.angle<0 ? static_cast<std::uint16_t>(1U-product) : product;
                if(summary.selected_high&0x80U)moved.velocity_y=static_cast<std::uint16_t>(0U-moved.velocity_y);
            }
            const int contribution=summary.angle<0 ? -static_cast<int>(magnitude/2U) : static_cast<int>(magnitude/2U);
            if(!summary.leading_support && magnitude<28 && !(summary.selected_high&0x80U))moved.velocity_x=static_cast<std::uint16_t>(static_cast<int>(motion.velocity_x)+contribution);
        }
        // $81:92FE–9309 sends a full steep landing straight to correction.
        // The vertical-to-horizontal conversion belongs only to continued contact.
        if(magnitude==28 && !summary.leading_support && incoming.unsupported_count<9) {
            const auto shifted=arithmetic_shift(moved.velocity_y,byte(shifts,magnitude));
            auto velocity=static_cast<std::uint16_t>(static_cast<unsigned>(shifted)*byte(multipliers,magnitude));
            if(summary.angle<0)velocity=static_cast<std::uint16_t>(1U-velocity);
            const auto reduced=static_cast<std::uint16_t>(velocity+(signed_word(velocity)<0?10:-10));
            if(signed_word(reduced)>=0)velocity=reduced;
            moved.velocity_x=(summary.selected_high&0x80U)?static_cast<std::uint16_t>(0U-velocity):velocity;
        }
    }
    next.previous_uncorrected_x=motion.x;
    auto horizontal_penetration=summary.horizontal_penetration;
    if(summary.penetration>=horizontal_penetration || (summary.tile_flags&0xfeU)==8) {
        if((summary.tile_flags&0xfeU)!=8)horizontal_penetration=0;
        next.previous_uncorrected_y=motion.y;
        moved.y=static_cast<std::uint16_t>(motion.y+(summary.inverted_vertical?summary.penetration:-static_cast<int>(summary.penetration)));
    }
    if(summary.horizontal_direction>=3)moved.x=static_cast<std::uint16_t>(motion.x+(summary.horizontal_direction==3?horizontal_penetration:-static_cast<int>(horizontal_penetration)));
    rider=next; motion=moved;
}
} // namespace unirally
