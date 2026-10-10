// ROM-free checks for the HUNTER tour's tag effects (R-0052, $83:CEC9-D600):
// the tag, the dispatch and its announcements, the timers, the skipped
// updates of effects 1 and 5, effect 3's screen flip and the state layout.
#include "zoom_zoo_movement.hpp"
#include <array>
#include <stdexcept>
#include <string>
#include <vector>

static void require_at(bool value,int line) {
    if(!value)throw std::runtime_error("HUNTER effect expectation failed at line "+std::to_string(line));
}
#define require(value) require_at((value),__LINE__)
template<class F> static void rejects_at(F action,int line) {
    bool rejected=false;try{action();}catch(const std::invalid_argument&){rejected=true;}require_at(rejected,line);
}
#define rejects(action) rejects_at((action),__LINE__)

namespace {
using namespace unirally;
std::array<std::uint8_t,14> header{};
std::array<std::uint8_t,26> weights{};
ZoomZooContent content() {
    header[3]=0x44;header[5]=0x32;header[7]=0x44;header[9]=0x32;header[13]=0x40;
    ZoomZooContent c{};c.movement.sampling.track=header;weights[0]=4;c.reward_weights=weights;
    return c;
}
// Both riders at the same place, the player two progress transitions ahead:
// the next effect routine tags them.
ZoomZooState tagged_race(unsigned player_x) {
    auto state=classic_race_start(content(),classic_race_scenario(ClassicRaceTrack{41}));
    auto& riders=state.movement.riders;
    riders[0].motion.x=static_cast<std::uint16_t>(player_x);riders[1].motion.x=static_cast<std::uint16_t>(player_x+4U);
    riders[0].motion.y=riders[1].motion.y=0x300;
    riders[0].progress.transition_count=5;riders[1].progress.transition_count=3;
    auto& q=state.player_announcements.queue;q.read_cursor=10;q.write_cursor=12;q.entries[10]=40;q.entries[11]=41;
    return state;
}
} // namespace

int main() {
    std::array<std::uint8_t,64> blink{};
    // Only the HUNTER tour runs the routine.
    {
        auto other=classic_race_start(content(),classic_race_scenario(ClassicRaceTrack{11}));
        auto& riders=other.movement.riders;riders[0].progress.transition_count=5;
        update_hunter_effects(other,blink);
        require(other.hunter==HunterEffects{});
    }
    // NEON (R-0068): HUNTER's stunt event runs the lighting in the tag effects' place, so riders
    // three transitions apart with their boxes together latch nothing; a HUNTER race does.
    for (const unsigned track : {42U, 41U}) {
        ZoomZooState state{};
        state.track = ClassicRaceTrack{static_cast<std::uint8_t>(track)};
        state.hunter_tour = true; // `$131F` on tracks 40 on, NEON's too ($82:D978)
        state.movement.riders[0].progress.transition_count = 4;
        state.movement.riders[1].progress.transition_count = 1;
        update_hunter_effects(state, blink);
        require(classic_race_scenario(state.track).neon_lighting == (track == 42));
        require(track == 42 ? state.hunter == HunterEffects{}
                            : state.hunter.latched && state.hunter.active);
    }
    // No tag until the progress counts have once differed by 2 or more.
    {
        auto even=tagged_race(0x400);
        even.movement.riders[0].progress.transition_count=4;even.movement.riders[1].progress.transition_count=3;
        update_hunter_effects(even,blink);
        require(!even.hunter.latched && !even.hunter.active);
        even.movement.riders[1].progress.transition_count=6; // the player two behind also latches
        update_hunter_effects(even,blink);
        require(even.hunter.latched && even.hunter.active);
    }
    // The tag picks effect x & 7; it is announced at the front of the queue in
    // the same update and names the HUD message.
    for(unsigned k=0;k<8;++k) {
        auto state=tagged_race(0x400+k);
        update_hunter_effects(state,blink);
        const auto& h=state.hunter;
        static constexpr std::array<std::uint8_t,8> event{0x1f,0x1c,0x1e,0x1b,0x20,0x1d,0x21,0x22};
        require(h.latched && h.active && h.effect[k]==2);
        for(unsigned other=0;other<8;++other)if(other!=k)require(h.effect[other]==0);
        const auto& q=state.player_announcements.queue;
        require(q.read_cursor==9 && q.entries[10]==event[k] && q.entries[11]==41);
        require(h.message==(k==1?0:event[k]));
        // Separate riders do not tag.
        auto apart=tagged_race(0x400+k);apart.movement.riders[1].motion.y=0x400;
        update_hunter_effects(apart,blink);
        require(!apart.hunter.active);
    }
    // Effect 0's 500 updates end with the blank announcement $23.
    {
        auto state=tagged_race(0x400);
        for(unsigned update=0;update<499;++update)update_hunter_effects(state,blink);
        require(state.hunter.active && state.hunter.timer[0]==1);
        state.player_announcements.queue.read_cursor=20;state.player_announcements.queue.write_cursor=22;
        update_hunter_effects(state,blink);
        require(!state.hunter.active && state.hunter.effect[0]==0 && state.hunter.timer[0]==0 && state.hunter.message==0x23);
        require(state.player_announcements.queue.entries[20]==0x23 && state.player_announcements.queue.read_cursor==19);
        // A full queue (read at write) takes no front announcement.
        auto full=tagged_race(0x400);full.player_announcements.queue.write_cursor=10;
        update_hunter_effects(full,blink);
        require(full.player_announcements.queue.read_cursor==10 && full.player_announcements.queue.entries[10]==40);
    }
    // Effect 5, slow motion: three updates in four are skipped.
    {
        auto state=tagged_race(0x405);
        std::vector<int> skips;
        for(unsigned update=0;update<8;++update) {
            update_hunter_effects(state,blink);
            skips.push_back(state.hunter.skip_update);state.hunter.skip_update=0;
        }
        require((skips==std::vector<int>{0,1,1,1,0,1,1,1}));
    }
    // Effect 1: freezes of 1, 2 ... 20 skipped updates, then 18 ... 2.
    {
        auto state=tagged_race(0x401);
        unsigned runs=0,skipped=0,updates=0;bool previous=false;
        do {
            update_hunter_effects(state,blink);++updates;
            const bool skip=state.hunter.skip_update!=0;state.hunter.skip_update=0;
            if(skip) {++skipped;if(!previous)++runs;}
            previous=skip;
        } while(state.hunter.effect[1] && updates<2000);
        require(!state.hunter.active && state.hunter.message==0);
        require(runs==29 && skipped==210+90);
    }
    // Effect 3 turns the player's screen position upside down while it is on.
    {
        auto state=tagged_race(0x403);
        state.race.camera.screen_xy=0x6940;
        update_hunter_effects(state,blink);
        require(state.hunter.blink==1 && state.hunter.wave_phase==1 && state.race.camera.screen_xy==0x3740);
        // A nonzero blink byte turns it off for that update (and leaves the position).
        std::array<std::uint8_t,64> off{};off[48]=1;
        update_hunter_effects(state,off);
        require(state.hunter.blink==0 && state.race.camera.screen_xy==0x3740);
    }
    // Effects 4 and 6 hide the track and mosaic the playfield; the table's
    // polarity is reversed over the last 50 (60) updates.
    {
        auto state=tagged_race(0x404);
        update_hunter_effects(state,blink);
        require(state.hunter.hide_track==0); // t=499: blink[49] zero, off
        std::array<std::uint8_t,64> on{};on.fill(1);
        update_hunter_effects(state,on);
        require(state.hunter.hide_track==1);
    }
    // The state carries the words and refuses them off the HUNTER tour.
    {
        auto state=classic_race_start(content(),classic_race_scenario(ClassicRaceTrack{41}));
        auto& h=state.hunter;h.latched=h.active=1;h.effect[6]=2;h.timer[6]=400;h.mosaic=1;h.mosaic_counter=7;
        h.message=0x21;h.caption=12;
        const auto bytes=serialize_zoom_zoo(state);
        require(bytes.size()==916 && deserialize_zoom_zoo(bytes).hunter==state.hunter);
        auto bad=bytes;bad[854+2*2+2*6]=3;rejects([&]{(void)deserialize_zoom_zoo(bad);}); // effect 6 = 3
        rejects([&]{update_hunter_effects(state,std::span<const std::uint8_t>{});});
    }
    // R-0094: while the name "faedine"'s races last, `$131F` is set on any track ($82:D978): the
    // tier and the tag effects run on DRAGSTER as on HUNTER's tracks.
    {
        auto scenario=classic_race_scenario(ClassicRaceTrack::Dragster);
        require(!scenario.hunter_tour);
        scenario.hunter_tour=true;
        auto state=classic_race_start(content(),scenario);
        require((state.hunter_tour && state.opponent_tier==OpponentTier{3,0x40,0x60}));
        auto& riders=state.movement.riders;
        riders[0].motion.x=0x404;riders[1].motion.x=0x408;riders[0].motion.y=riders[1].motion.y=0x300;
        riders[0].progress.transition_count=5;riders[1].progress.transition_count=3;
        auto tagged=state;
        update_hunter_effects(tagged,blink);
        require(tagged.hunter.active==1 && tagged.hunter.effect[4]==2);
        auto& h=state.hunter;h.latched=h.active=1;h.effect[4]=2;h.timer[4]=300;
        // Its state is wrapped (URHF0001): a flag, DRAGSTER's own 742 bytes, then the effects,
        // which DRAGSTER's layout has no room for; it reads back to the same state and tier.
        const auto bytes=serialize_zoom_zoo(state);
        require(bytes.size()==8+1+742+62 && bytes[2]=='H' && bytes[3]=='F' && bytes[8]==1);
        const auto back=deserialize_zoom_zoo(bytes);
        require(back.hunter_tour && back.hunter==state.hunter && back.track==ClassicRaceTrack::Dragster);
        require(back.opponent_tier==state.opponent_tier && serialize_zoom_zoo(back)==bytes);
        // A wrapper's flag is 0 or 1, and says whether the layout lacks the effects.
        auto bad=bytes;bad[8]=2;rejects([&]{(void)deserialize_zoom_zoo(bad);});
        bad=bytes;bad[8]=0;rejects([&]{(void)deserialize_zoom_zoo(bad);});
        // Without the wrapper the effects are refused, as off the HUNTER tour.
        state.hunter_tour=false;
        rejects([&]{(void)serialize_zoom_zoo(state);});
    }
    // Another track's layout holds the effects itself: the wrapper adds only its flag. A HUNTER
    // track sets the flag itself and is never wrapped.
    {
        auto scenario=classic_race_scenario(ClassicRaceTrack{11});
        scenario.hunter_tour=true;
        auto state=classic_race_start(content(),scenario);
        state.hunter.latched=state.hunter.active=1;state.hunter.effect[1]=2;
        const auto bytes=serialize_zoom_zoo(state);
        require(bytes.size()==8+1+916 && bytes[8]==0);
        const auto back=deserialize_zoom_zoo(bytes);
        require(back.hunter_tour && back.hunter==state.hunter && serialize_zoom_zoo(back)==bytes);
        auto hunter=classic_race_start(content(),classic_race_scenario(ClassicRaceTrack{41}));
        auto wrapped=bytes;
        const auto own=serialize_zoom_zoo(hunter);
        require(own.size()==916);
        wrapped.resize(9);wrapped.insert(wrapped.end(),own.begin(),own.end());
        rejects([&]{(void)deserialize_zoom_zoo(wrapped);});
    }
    return 0;
}
