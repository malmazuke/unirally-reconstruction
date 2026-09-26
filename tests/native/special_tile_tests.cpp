// ROM-free checks for the special tiles on the shared race engine (R-0047,
// R-0051): mud (flag pair 14), the corkscrew (pair 10), the loop (pair 26),
// contact's pairs 8 and 26, their per-update counters and the other tracks'
// state layout that carries them.
#include "vertical_contact.hpp"
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
        require(special.drive_step==4 && special.mud_velocity==0xeb);
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
            require(u.drive_step==0 && u.mud_velocity==0 && t.mud_cooldown==4);
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
        // The rider look reads the corkscrew's skipped contact back from the words each update
        // leaves: the reset counts the hold down, and each step sets it to 8 again. The exit
        // step and the updates after it run the contact.
        RiderMovementState rider{};SpecialTileRider tiles{};SurfaceTransition surface{};
        ReflectionTransition transition{};
        rider.contact.selected_word=0x0a02;rider.motion.y=100;
        require(!special_tiles_skipped_contact(tiles));
        for(unsigned update=0;update<0x34;++update) {
            SpecialTileUpdate special{};
            update_special_tile_counters(tiles,transition,rider.contact.selected_high);
            update_corkscrew_tile(rider,tiles,surface,transition,special,heights);
            require(special_tiles_skipped_contact(tiles)==(special.contact_skip!=0));
            require(special.contact_skip==(update<0x30?1:0));
        }
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

    // Layout: any track but DRAGSTER and ZOOM ZOO is URTRnn06, 916 bytes: the
    // special-tile words (12 per rider), $0E7B and $0C73 after the shared 742,
    // then 60 checkpoint flags and the HUNTER effects' 31 words.
    {
        std::array<std::uint8_t,14> header{};header[3]=0x44;header[5]=0x32;header[7]=0x44;header[9]=0x32;header[13]=0x40;
        ZoomZooContent content{};content.movement.sampling.track=header;
        std::array<std::uint8_t,26> weights{};weights[0]=4;content.reward_weights=weights;
        auto state=classic_race_start(content,classic_race_scenario(ClassicRaceTrack{11}));
        state.special_tiles[1]={4,4,1,0x12,1,8,0,1};state.drive_target_latch=1;
        const auto bytes=serialize_zoom_zoo(state);
        const std::array<std::uint8_t,8> magic{'U','R','T','R','1','1','0','6'};
        require(bytes.size()==916 && std::equal(magic.begin(),magic.end(),bytes.begin()));
        require(bytes[766]==4 && bytes[770]==1 && bytes[772]==0x12 && bytes[776]==8 && bytes[780]==1 && bytes[790]==1);
        // R-0051: the loop's three words and flag pair 8's counter follow each rider's eight.
        auto looping=state;looping.special_tiles[0].loop_direction=1;looping.special_tiles[0].loop_step=9;
        looping.special_tiles[0].loop_cooldown=3;looping.special_tiles[1].slow_counter=7;looping.special_tiles[1].loop_step=0xfffe;
        const auto looping_bytes=serialize_zoom_zoo(looping);
        require(looping_bytes[758]==1 && looping_bytes[760]==9 && looping_bytes[762]==3 && looping_bytes[788]==7 &&
                looping_bytes[784]==0xfe && looping_bytes[785]==0xff && deserialize_zoom_zoo(looping_bytes).special_tiles==looping.special_tiles);
        const auto restored=deserialize_zoom_zoo(bytes);
        require(restored.special_tiles==state.special_tiles && restored.drive_target_latch==1 && serialize_zoom_zoo(restored)==bytes);
        // Out-of-domain words are refused.
        for(const unsigned at:{742U,752U,758U,760U,762U,764U,772U,780U,790U}) {auto bad=bytes;bad[at]=0x40;rejects([&]{(void)deserialize_zoom_zoo(bad);});}
        auto short_state=bytes;short_state.resize(794);rejects([&]{(void)deserialize_zoom_zoo(short_state);});
        auto old_version=bytes;old_version[7]='5';rejects([&]{(void)deserialize_zoom_zoo(old_version);});
        auto old_width=bytes;old_width.resize(854);old_width[7]='5';rejects([&]{(void)deserialize_zoom_zoo(old_width);});
        // R-0052: HUNTER words on a track outside the HUNTER tour are refused.
        auto stray=bytes;stray[854]=1;rejects([&]{(void)deserialize_zoom_zoo(stray);});
        auto stray_zoom=classic_crawler_zoom_zoo_start(content);stray_zoom.hunter.latched=1;
        rejects([&]{(void)serialize_zoom_zoo(stray_zoom);});
        // LOCKED-TOURS: the opponent's turnaround counter travels at 792, 0-30.
        auto turning=state;turning.opponent_turnaround=12;
        const auto turning_bytes=serialize_zoom_zoo(turning);
        require(turning_bytes[792]==12 && deserialize_zoom_zoo(turning_bytes).opponent_turnaround==12);
        auto bad_turn=turning_bytes;bad_turn[792]=31;rejects([&]{(void)deserialize_zoom_zoo(bad_turn);});
        // R-0048: the last 60 checkpoint flags travel with it, $FF or 0 only.
        auto seven=state;seven.race.checkpoint_seen[35]=0;
        const auto seven_bytes=serialize_zoom_zoo(seven);
        require(seven_bytes[794+15]==0 && deserialize_zoom_zoo(seven_bytes).race.checkpoint_seen[35]==0);
        auto bad_flag=seven_bytes;bad_flag[818]=0x40;rejects([&]{(void)deserialize_zoom_zoo(bad_flag);});
        // DRAGSTER and ZOOM ZOO keep 742 bytes until a special-tile word is live,
        // then take the extended layout under URDG0004 / URZZ000E.
        auto dragster=classic_crawler_dragster_race_start(content);
        require(serialize_zoom_zoo(dragster).size()==742);
        dragster.special_tiles[0].physics_hold=1;
        const auto live=serialize_zoom_zoo(dragster);
        const std::array<std::uint8_t,8> dragster_live{'U','R','D','G','0','0','0','4'};
        require(live.size()==794 && std::equal(dragster_live.begin(),dragster_live.end(),live.begin()));
        require(deserialize_zoom_zoo(live).special_tiles==dragster.special_tiles && serialize_zoom_zoo(deserialize_zoom_zoo(live))==live);
        auto zoom=classic_crawler_zoom_zoo_start(content);
        require(serialize_zoom_zoo(zoom).size()==742);
        zoom.special_tiles[1].corkscrew_step=5;zoom.special_tiles[1].corkscrew_latch=1;
        const auto zoom_live=serialize_zoom_zoo(zoom);
        const std::array<std::uint8_t,8> zoom_magic{'U','R','Z','Z','0','0','0','E'};
        require(zoom_live.size()==794 && std::equal(zoom_magic.begin(),zoom_magic.end(),zoom_live.begin()));
        require(deserialize_zoom_zoo(zoom_live).track==ClassicRaceTrack::ZoomZoo && serialize_zoom_zoo(deserialize_zoom_zoo(zoom_live))==zoom_live);
        // The opponent's turnaround alone also makes the state extended (LOCKED-TOURS).
        auto turning_zoom=classic_crawler_zoom_zoo_start(content);turning_zoom.opponent_turnaround=5;
        const auto turning_zoom_bytes=serialize_zoom_zoo(turning_zoom);
        require(turning_zoom_bytes.size()==794 && turning_zoom_bytes[792]==5 &&
                serialize_zoom_zoo(deserialize_zoom_zoo(turning_zoom_bytes))==turning_zoom_bytes);
        // The extended layout with every word zero is not canonical and is refused.
        auto idle=zoom_live;for(unsigned at=742;at<794;++at)idle[at]=0;
        rejects([&]{(void)deserialize_zoom_zoo(idle);});
    }
    // The loop (flag pair 26, R-0051), entered against a mirrored descriptor by
    // a reflected rider. Offsets here are the step number itself.
    {
        std::array<std::uint8_t,34> offsets{};
        for(unsigned k=0;k<17;++k)offsets[2*k]=static_cast<std::uint8_t>(k);
        RiderMovementState rider{};SpecialTileRider tiles{};SurfaceTransition surface{};ReflectionTransition transition{};
        rider.contact.selected_word=0x4a2a;rider.pose.reflected=true;rider.motion.x=1000;
        rider.motion.velocity_x=0x40;rider.motion.velocity_y=0x100;
        SpecialTileUpdate entry{};
        update_loop_tile(rider,tiles,surface,transition,entry,offsets);
        require(tiles.loop_step==1 && tiles.loop_cooldown==3 && tiles.loop_direction==0 && transition.pose_override==0x610);
        require(rider.motion.x==990 && rider.motion.velocity_x==0 && rider.motion.velocity_y==0x100 && entry.contact_skip==0);
        std::uint16_t x=990;
        for(std::uint16_t step=1;step<16;++step) {
            SpecialTileUpdate u{};SurfaceTransition s{};rider.motion.response_b=3;
            update_loop_tile(rider,tiles,s,transition,u,offsets);
            x=static_cast<std::uint16_t>(x-step);
            require(tiles.loop_step==step+1 && transition.pose_override==0x610+step && rider.motion.x==x);
            require(rider.motion.velocity_y==0x1ce && rider.motion.velocity_x==0 && rider.motion.response_b==0);
            require(s.mode==1 && tiles.reflection_lock==6 && rider.pose.reflected);
            // Step 8, the top, restores contact and gravity for that update.
            require(u.contact_skip==(step==8?0:1) && tiles.corkscrew_float==(step==8?0:1) && s.tile_pose_enabled==(step==8?0:1));
            // The rider look reads the skipped contact back from the words the update leaves.
            require(special_tiles_skipped_contact(tiles)==(u.contact_skip!=0));
        }
        SpecialTileUpdate last{};
        update_loop_tile(rider,tiles,surface,transition,last,offsets);
        require(tiles.loop_step==0 && tiles.loop_cooldown==3 && transition.pose_override==0x61f);
        // The cooldown: two updates later the pose, the float and the angle
        // sentinel end and the rider is posed rolling and upright.
        rider.contact.angle_unspecified=true;tiles.corkscrew_float=1;
        update_loop_cooldown(rider,tiles,transition);
        require(tiles.loop_cooldown==2 && transition.pose_override==0x61f);
        update_loop_cooldown(rider,tiles,transition);
        require(tiles.loop_cooldown==1 && transition.pose_override==0 && !tiles.corkscrew_float && !rider.contact.angle_unspecified);
        require(rider.pose.orientation==0x24 && rider.pose.rolling && rider.pose.rolling_level==2);
        update_loop_cooldown(rider,tiles,transition);update_loop_cooldown(rider,tiles,transition);
        require(tiles.loop_cooldown==0 && tiles.loop_step==0);
        // Leaving at step 9 turns the rider back the loop's way; mid-loop and
        // unreflected it is posed at $14.
        RiderMovementState mid{};SpecialTileRider t9{};ReflectionTransition tr9{};
        t9.loop_step=9;t9.loop_cooldown=2;t9.loop_direction=1;
        update_loop_cooldown(mid,t9,tr9);
        require(mid.pose.reflected && mid.pose.orientation==0x14);
        // Entering the other way: x forward, direction 1, reflected cleared.
        RiderMovementState right{};SpecialTileRider rt{};SurfaceTransition rs{};ReflectionTransition rtr{};SpecialTileUpdate ru{};
        right.contact.selected_word=0x0a2a;right.motion.x=100;
        update_loop_tile(right,rt,rs,rtr,ru,offsets);update_loop_tile(right,rt,rs,rtr,ru,offsets);
        require(rt.loop_direction==1 && right.motion.x==111 && !right.pose.reflected && rt.loop_step==2);
        // Refusals: rising, leading support or another tile's pose; a refused
        // entry counts $FFFE back up to 0 and holds the loop off meanwhile.
        for(unsigned c=0;c<3;++c) {
            RiderMovementState r{};SpecialTileRider t{};SurfaceTransition sf{};ReflectionTransition tr{};SpecialTileUpdate u{};
            r.contact.selected_word=0x0a2a;
            if(c==0)r.motion.velocity_y=0xffff;
            if(c==1)sf.leading_support=1;
            if(c==2)tr.pose_override=0x605;
            update_loop_tile(r,t,sf,tr,u,offsets);
            require(t.loop_step==0xfffe && t.loop_cooldown==0);
            update_loop_tile(r,t,sf,tr,u,offsets);require(t.loop_step==0xfffe);
            update_loop_cooldown(r,t,tr);require(t.loop_step==0xffff);
            update_loop_cooldown(r,t,tr);require(t.loop_step==0);
        }
        // Facing the wrong way: only the cooldown is set.
        RiderMovementState wrong{};SpecialTileRider wt{};SurfaceTransition ws{};ReflectionTransition wtr{};SpecialTileUpdate wu{};
        wrong.contact.selected_word=0x4a2a;wrong.motion.x=50;
        update_loop_tile(wrong,wt,ws,wtr,wu,offsets);
        require(wt.loop_step==0 && wt.loop_cooldown==3 && wrong.motion.x==50 && wtr.pose_override==0);
        rejects([&]{update_loop_tile(wrong,wt,ws,wtr,wu,std::span<const std::uint8_t>{});});
    }

    // Contact, flag pair 26: at the loop's top (step 9) both penetrations are
    // cleared, so the rider is not corrected and no boundary is taken.
    {
        std::array<std::uint8_t,9> shifts{},multipliers{};
        VerticalContactSummary summary{};summary.supported=true;summary.any_nonnegative_probe=true;
        summary.angle=0;summary.tile_flags=0x1b;summary.penetration=5;summary.horizontal_penetration=3;summary.horizontal_direction=3;
        RiderContactState rider{};ContactMotion motion{};motion.x=400;motion.y=300;
        resolve_vertical_contact(rider,motion,summary,{0,false,0,0xc200,true},shifts,multipliers);
        require(motion.x==400 && motion.y==300);
        rider={};motion={};motion.x=400;motion.y=300;
        resolve_vertical_contact(rider,motion,summary,{0,false,0,0xc200,false},shifts,multipliers);
        require(motion.x==400 && motion.y==295);
        summary.penetration=127;summary.boundary_marker=true;rider={};
        resolve_vertical_contact(rider,motion,summary,{0,false,0,0xc200,true},shifts,multipliers);
        require(rider.auxiliary_flag==0 && rider.unsupported_count==0);
    }

    // Contact, flag pair 8: a re-contact goes straight to the correction with
    // the counters cleared (no landing matrix is read), and under surface mode
    // continued contact keeps velocity y.
    {
        std::array<std::uint8_t,9> shifts{},multipliers{};multipliers[4]=3;
        VerticalContactSummary summary{};summary.supported=true;summary.any_nonnegative_probe=true;
        summary.tile_flags=8;summary.angle=4;summary.penetration=2;
        RiderContactState rider{};rider.unsupported_count=9;rider.unsupported_duration=40;
        ContactMotion motion{};motion.velocity_x=0x80;motion.velocity_y=0x55;motion.y=100;
        resolve_vertical_contact(rider,motion,summary,{0,false,0,0xc200},shifts,multipliers);
        require(rider.recontact && rider.unsupported_count==0 && rider.unsupported_duration==0);
        require(motion.velocity_x==0x80 && motion.velocity_y==0x55 && motion.y==98);
        RiderContactState flat{};ContactMotion kept{};kept.velocity_x=0x10;kept.velocity_y=0x21;
        resolve_vertical_contact(flat,kept,summary,{0,false,1,0xc200},shifts,multipliers);
        require(kept.velocity_y==0x21);
        RiderContactState other{};ContactMotion stored{};stored.velocity_x=0x10;stored.velocity_y=0x21;
        resolve_vertical_contact(other,stored,summary,{0,false,0,0xc200},shifts,multipliers);
        require(stored.velocity_y==0x30);
    }
    return 0;
}
