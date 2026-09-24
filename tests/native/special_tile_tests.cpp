// ROM-free checks for the special tiles on the shared race engine (R-0047):
// mud (flag pair 14), the corkscrew (pair 10), their per-update counters and
// the other tracks' state layout that carries them.
#include "zoom_zoo_movement.hpp"
#include <array>
#include <stdexcept>
#include <string>
#include <vector>

static void require_at(bool value,int line) {
    if(!value)throw std::runtime_error("special tile expectation failed at line "+std::to_string(line));
}
#define require(value) require_at((value),__LINE__)
template<class F> static void rejects_at(F action,int line) {
    bool rejected=false;try{action();}catch(const std::invalid_argument&){rejected=true;}require_at(rejected,line);
}
#define rejects(action) rejects_at((action),__LINE__)

int main() {
    using namespace unirally;

    // Mud, entering: velocity x halves by an arithmetic shift, vertical motion
    // stops, both counters hold 4 and the tile brakes by 5.
    {
        RiderMovementState rider{};SpecialTileRider tiles{};SurfaceTransition surface{};SpecialTileUpdate special{};
        rider.motion.velocity_x=0x1e0;rider.motion.velocity_y=0x13;
        update_mud_tile(rider,tiles,surface,special);
        require(rider.motion.velocity_x==0xf0-5 && rider.motion.velocity_y==0);
        require(tiles.mud_cooldown==4 && tiles.mud_exit_pending==4 && surface.tile_mode==1);
        require(special.mud_drive_step==4 && special.mud_velocity==0xeb);
        // Staying on it: no second halving.
        SpecialTileUpdate again{};rider.motion.velocity_y=7;
        update_mud_tile(rider,tiles,surface,again);
        require(rider.motion.velocity_x==0xeb-5 && rider.motion.velocity_y==7);
        // Negative velocity halves toward minus infinity and brakes upward.
        RiderMovementState left{};SpecialTileRider fresh{};SpecialTileUpdate l{};
        left.motion.velocity_x=static_cast<std::uint16_t>(-0x101);
        update_mud_tile(left,fresh,surface,l);
        require(left.motion.velocity_x==static_cast<std::uint16_t>(-0x81+5) && l.mud_velocity==left.motion.velocity_x);
        // Within 48 of zero ($FFD0-$002F) the brake and the drive step stay off.
        for(const std::uint16_t v:{std::uint16_t{0x5f},std::uint16_t{0},static_cast<std::uint16_t>(-0x60)}) {
            RiderMovementState slow{};SpecialTileRider t{};SpecialTileUpdate u{};slow.motion.velocity_x=v;
            update_mud_tile(slow,t,surface,u);
            require(u.mud_drive_step==0 && u.mud_velocity==0 && t.mud_cooldown==4);
        }
    }

    // The counters: mud cools down, then clears its exit word; a corkscrew
    // latch counts up from an ejection; the physics hold counts down.
    {
        SpecialTileRider tiles{};ReflectionTransition transition{};
        tiles.mud_cooldown=1;tiles.mud_exit_pending=4;tiles.physics_hold=2;
        tiles.corkscrew_latch=0xfffc;tiles.corkscrew_step=0xffff;
        update_special_tile_counters(tiles,transition,0);
        require(tiles.mud_cooldown==0 && tiles.mud_exit_pending==4 && tiles.physics_hold==1);
        require(tiles.corkscrew_latch==0xfffd && tiles.corkscrew_step==0xffff);
        update_special_tile_counters(tiles,transition,0);
        require(tiles.mud_exit_pending==0 && tiles.physics_hold==0 && tiles.corkscrew_latch==0xfffe);
        // With no latch the step ends, and so do a corkscrew pose and the
        // float, unless the rider is on an inverted contact.
        SpecialTileRider after{};after.corkscrew_step=0x31;after.corkscrew_float=1;
        transition.pose_override=0x605;
        update_special_tile_counters(after,transition,0x80);
        require(after.corkscrew_step==0 && transition.pose_override==0 && after.corkscrew_float==1);
        update_special_tile_counters(after,transition,0);
        require(after.corkscrew_float==0);
        // A latch of 1 (the previous update was in the corkscrew) keeps them.
        SpecialTileRider riding{};riding.corkscrew_latch=1;riding.corkscrew_step=7;
        transition.pose_override=0x603;
        update_special_tile_counters(riding,transition,0);
        require(riding.corkscrew_step==7 && transition.pose_override==0x603 && riding.corkscrew_latch==0);
    }

    // The corkscrew. Heights: the first table for a rider not rolling.
    std::array<std::uint8_t,96> heights{};
    for(unsigned i=0;i<96;++i)heights[i]=static_cast<std::uint8_t>(i<48?i+1:0xff-(i-48));
    {
        // Entry, unreflected, facing left (bit 14 clear), standing still.
        RiderMovementState rider{};SpecialTileRider tiles{};SurfaceTransition surface{};
        ReflectionTransition transition{};SpecialTileUpdate special{};
        rider.contact.selected_word=0x0a02;rider.motion.y=100;rider.motion.response_b=2;
        update_corkscrew_tile(rider,tiles,surface,transition,special,heights);
        require(tiles.corkscrew_latch==1 && tiles.corkscrew_step==1 && tiles.raised_priority==0);
        require(rider.motion.velocity_x==0xfe32 && rider.motion.velocity_y==0 && rider.motion.response_b==0);
        require(rider.launch_override==80 && tiles.physics_hold==8 && tiles.corkscrew_float==1);
        require(special.contact_skip==1 && special.corkscrew_stepped && surface.tile_pose_enabled==1);
        require(transition.pose_override==0x600 && rider.motion.y==101);
        // Step 1 toggles the object priority; the pose wraps at $10.
        SpecialTileUpdate next{};
        update_corkscrew_tile(rider,tiles,surface,transition,next,heights);
        require(tiles.corkscrew_step==2 && tiles.raised_priority==1 && transition.pose_override==0x601 && rider.motion.y==103);
        tiles.corkscrew_step=0x2f;
        update_corkscrew_tile(rider,tiles,surface,transition,next,heights);
        require(tiles.corkscrew_step==0x30 && tiles.raised_priority==0 && transition.pose_override==0x608);
        // A rolling rider reads the second table, sign-extended.
        rider.pose.rolling=true;rider.motion.y=100;tiles.corkscrew_step=4;
        update_corkscrew_tile(rider,tiles,surface,transition,next,heights);
        require(rider.motion.y==static_cast<std::uint16_t>(100-5));
        // Step $30: out facing the other way, upside down, reflection locked.
        rider.pose.rolling=false;tiles.corkscrew_step=0x30;SpecialTileUpdate out{};
        update_corkscrew_tile(rider,tiles,surface,transition,out,heights);
        require(tiles.corkscrew_step==0x31 && rider.pose.reflected && rider.pose.orientation==0x2b);
        require(rider.contact.selected_high==0x80 && tiles.reflection_lock==6 && surface.mode==1);
        require(transition.pose_override==0 && surface.tile_pose_enabled==0 && !out.corkscrew_stepped);
        // Step $31, now reflected on a descriptor facing right and not moving
        // left, only keeps the launch override.
        SpecialTileUpdate hold{};rider.launch_override=0;
        rider.contact.selected_word=0x4a02;rider.motion.velocity_x=0x10;
        update_corkscrew_tile(rider,tiles,surface,transition,hold,heights);
        require(tiles.corkscrew_step==0x31 && rider.launch_override==80 && !hold.corkscrew_stepped);
    }
    {
        // Ejection: moving the wrong way (right while unreflected) boosts the
        // rider by $C0 along the descriptor's facing and latches -4.
        RiderMovementState rider{};SpecialTileRider tiles{};SurfaceTransition surface{};
        ReflectionTransition transition{};SpecialTileUpdate special{};
        rider.contact.selected_word=0x0a02;rider.motion.velocity_x=0x40;rider.motion.velocity_y=0x55;
        update_corkscrew_tile(rider,tiles,surface,transition,special,heights);
        require(tiles.corkscrew_latch==0xfffc && tiles.corkscrew_step==0xffff && !special.corkscrew_stepped);
        require(rider.motion.velocity_x==0x40+0xc0 && rider.motion.velocity_y==0 && rider.launch_override==80);
        require(surface.tile_pose==1 && surface.tile_pose_enabled==1);
        // While the latch is negative every entry ejects again.
        tiles.corkscrew_latch=0xfffe;rider.motion.velocity_x=0;
        update_corkscrew_tile(rider,tiles,surface,transition,special,heights);
        require(tiles.corkscrew_latch==0xfffc);
        // Mid-reflection, inverted, or after three unsupported updates: eject.
        for(int reason=0;reason<3;++reason) {
            RiderMovementState r{};SpecialTileRider t{};ReflectionTransition tr{};SpecialTileUpdate u{};
            r.contact.selected_word=reason==1?0x8a02:0x0a02;
            if(reason==0)tr.step=3;
            if(reason==2)r.contact.previous_unsupported_count=3;
            update_corkscrew_tile(r,t,surface,tr,u,heights);
            require(t.corkscrew_latch==0xfffc);
        }
        // A pose override before the first step defers entry without ejecting.
        RiderMovementState r{};SpecialTileRider t{};ReflectionTransition tr{};SpecialTileUpdate u{};
        r.contact.selected_word=0x0a02;tr.pose_override=0x625;
        update_corkscrew_tile(r,t,surface,tr,u,heights);
        require(t.corkscrew_latch==1 && t.corkscrew_step==0 && r.launch_override==0);
        // Missing heights are refused rather than read.
        rejects([&]{SpecialTileRider t2{};RiderMovementState r2{};ReflectionTransition tr2{};r2.contact.selected_word=0x0a02;
                    update_corkscrew_tile(r2,t2,surface,tr2,u,std::span<const std::uint8_t>{});});
    }

    // Layout: any track but DRAGSTER and ZOOM ZOO is URTRnn02, 776 bytes, the
    // special-tile words after the shared 742; DRAGSTER keeps 742 and refuses them.
    {
        std::array<std::uint8_t,14> header{};header[3]=0x44;header[5]=0x32;header[7]=0x44;header[9]=0x32;header[13]=0x40;
        ZoomZooContent content{};content.movement.sampling.track=header;
        std::array<std::uint8_t,26> weights{};weights[0]=4;content.reward_weights=weights;
        auto state=classic_race_start(content,classic_race_scenario(ClassicRaceTrack{11}));
        state.special_tiles[1]={4,4,1,0x12,1,8,0,1};state.drive_target_latch=1;
        const auto bytes=serialize_zoom_zoo(state);
        const std::array<std::uint8_t,8> magic{'U','R','T','R','1','1','0','2'};
        require(bytes.size()==776 && std::equal(magic.begin(),magic.end(),bytes.begin()));
        require(bytes[758]==4 && bytes[762]==1 && bytes[764]==0x12 && bytes[768]==8 && bytes[772]==1 && bytes[774]==1);
        const auto restored=deserialize_zoom_zoo(bytes);
        require(restored.special_tiles==state.special_tiles && restored.drive_target_latch==1 && serialize_zoom_zoo(restored)==bytes);
        // Out-of-domain words are refused.
        for(const unsigned at:{742U,752U,764U,772U,774U}) {auto bad=bytes;bad[at]=0x40;rejects([&]{(void)deserialize_zoom_zoo(bad);});}
        auto short_state=bytes;short_state.resize(742);rejects([&]{(void)deserialize_zoom_zoo(short_state);});
        auto old_version=bytes;old_version[7]='1';rejects([&]{(void)deserialize_zoom_zoo(old_version);});
        auto dragster=classic_crawler_dragster_race_start(content);
        require(serialize_zoom_zoo(dragster).size()==742);
        dragster.special_tiles[0].physics_hold=1;
        rejects([&]{(void)serialize_zoom_zoo(dragster);});
    }
    return 0;
}
