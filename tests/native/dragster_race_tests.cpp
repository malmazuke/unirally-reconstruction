// ROM-free checks for DRAGSTER on the shared race engine (R-0038): playfield
// geometry, scenario values, state identity and the physical D-pad adapter.
#include "front_end.hpp"
#include "race_camera.hpp"
#include "race_progress.hpp"
#include "zoom_zoo_movement.hpp"
#include <algorithm>
#include <array>
#include <span>
#include <stdexcept>
#include <vector>

static void require(bool value) {if(!value)throw std::runtime_error("DRAGSTER race expectation failed");}
template<class F> static void rejects(F action) {
    bool rejected=false;try{action();}catch(const std::invalid_argument&){rejected=true;}require(rejected);
}

int main() {
    using namespace unirally;
    // $81:A304-A51B: byte 13 = 0 takes the 1,024 x 16 arm ($81:A4C1), 0x40 the
    // 256 x 64 arm ($81:A445); 0x80, 0x20, 0x10 and 0x08 the arms read from the
    // listing (TRACK-BREADTH), and 0x04, which also sets $0FF7 (R-0066). Any
    // value without an arm is rejected.
    std::array<std::uint8_t,14> header{};
    header[3]=0x44;header[5]=0x32;header[7]=0x44;header[9]=0x32; // DRAGSTER cells 68,50
    const auto dragster=track_geometry(header);
    require(dragster.coarse_columns==1024 && dragster.position_mask==0xffff && dragster.screen_shift==0);
    require(dragster.follow_window_low==-24 && dragster.follow_window_high==25);
    require(dragster.visible_left==-49 && dragster.visible_right==256);
    auto zoom_header=header;zoom_header[13]=0x40;
    const auto zoom=track_geometry(zoom_header);
    require(zoom.coarse_columns==256 && zoom.position_mask==0x3fff && zoom.screen_shift==2);
    require(zoom.follow_window_low==-96 && zoom.follow_window_high==100);
    require(zoom.visible_left==-196 && zoom.visible_right==1024);
    struct Arm {std::uint8_t shape;std::uint16_t columns,mask;unsigned shift;std::int16_t low,high,left,right;};
    for(const Arm arm:{Arm{0x80,512,0x7fff,1,-0x30,0x32,-0x62,0x200},Arm{0x20,128,0x1fff,3,-0xc0,0xc8,-0x188,0x800},
                       Arm{0x10,64,0x0fff,4,-0x180,0x190,-0x310,0x1000},Arm{0x08,32,0x07ff,5,-0x300,0x320,-0x620,0x2000}}) {
        auto other=header;other[13]=arm.shape;
        const auto g=track_geometry(other);
        require(g.coarse_columns==arm.columns && g.position_mask==arm.mask && g.screen_shift==arm.shift);
        require(g.follow_window_low==arm.low && g.follow_window_high==arm.high && g.visible_left==arm.left && g.visible_right==arm.right);
        require(g.coarse_columns/4U==(arm.shape==0 ? 256U : arm.shape) && (g.coarse_columns*64U-1U)==g.position_mask);
    }
    // R-0066: the 0x04 arm (track 37), 16 columns of 1,024 rows, the only one whole-height.
    {auto narrow=header;narrow[13]=0x04;const auto g=track_geometry(narrow);
     require(g.coarse_columns==16 && g.position_mask==0x03ff && g.screen_shift==6 && g.whole_height);
     require(g.follow_window_low==-0x600 && g.follow_window_high==0x640 && g.visible_left==-0xc40 && g.visible_right==0x4000);
     require(!zoom.whole_height && !dragster.whole_height);}
    for(unsigned shape:{0x01U,0x02U,0xC0U}) {auto other=header;other[13]=static_cast<std::uint8_t>(shape);rejects([&]{(void)track_geometry(other);});}
    rejects([&]{(void)track_geometry(std::span<const std::uint8_t>(header.data(),13));});

    // Scenario: DRAGSTER initializes 48 frames before ZOOM ZOO on the accepted
    // menu path, races one lap in race mode 0, and settles its result later.
    const auto dragster_scenario=classic_race_scenario(ClassicRaceTrack::Dragster);
    const auto zoom_scenario=classic_race_scenario(ClassicRaceTrack::ZoomZoo);
    require(dragster_scenario.initialization_frame==1328 && dragster_scenario.laps==1 && !dragster_scenario.tour_race);
    require(zoom_scenario.initialization_frame==1376 && zoom_scenario.laps==3 && zoom_scenario.tour_race);
    require(opponent_tier(dragster_scenario,{}).adjustment_limit==96 && opponent_tier(zoom_scenario,{}).adjustment_limit==72);

    ZoomZooContent content{};content.movement.sampling.track=header;
    std::array<std::uint8_t,26> weights{};weights[0]=4;content.reward_weights=weights;
    auto start=classic_crawler_dragster_race_start(content);
    require(start.track==ClassicRaceTrack::Dragster && start.movement.frame==1328);
    require(start.movement.riders[0].motion.x==1088 && start.movement.riders[0].motion.y==800);
    for(const auto& lap:start.race.riders)require(lap.laps_remaining==2);
    require(start.race.camera.x==832 && start.race.camera.y==544);

    // The state identity is the magic: same 742-byte layout, DRAGSTER bounds.
    const auto bytes=serialize_zoom_zoo(start);
    require(bytes.size()==742 && std::equal(dragster_race_state_magic.begin(),dragster_race_state_magic.end(),bytes.begin()));
    const auto restored=deserialize_zoom_zoo(bytes);
    require(restored.track==ClassicRaceTrack::Dragster && serialize_zoom_zoo(restored)==bytes);
    auto as_zoom_zoo=bytes;const std::array<std::uint8_t,8> zoom_magic{'U','R','Z','Z','0','0','0','B'};
    std::copy(zoom_magic.begin(),zoom_magic.end(),as_zoom_zoo.begin());
    rejects([&]{(void)deserialize_zoom_zoo(as_zoom_zoo);}); // Frame 1328 precedes ZOOM ZOO initialization.
    auto truncated=bytes;truncated.pop_back();rejects([&]{(void)deserialize_zoom_zoo(truncated);});
    auto three_laps=bytes;three_laps[423]=3;rejects([&]{(void)deserialize_zoom_zoo(three_laps);});
    auto early=bytes;early[8]=0x2f;rejects([&]{(void)deserialize_zoom_zoo(early);}); // Frame 1327.
    auto legacy=start;legacy.native_initialization=false;rejects([&]{(void)serialize_zoom_zoo(legacy);});

    // TRACK-BREADTH part 3: the other cold-start race tracks. FLAT FUN (13) is a
    // one-run race like DRAGSTER, INFINITY (14) a seven-lap race. Every one of the 45 tracks
    // has a scenario since STUNT-EVENT-RACE (R-0066); no track 45 exists.
    const ClassicRaceTrack flat_fun{13},infinity{14};
    const auto flat=classic_race_scenario(flat_fun),seven=classic_race_scenario(infinity);
    require(flat.initialization_frame==1392 && flat.laps==1 && !flat.tour_race && flat.stable_result_won==226);
    require(seven.initialization_frame==1376 && seven.laps==7 && seven.tour_race && seven.stable_result_won==115);
    require(classic_race_has_scenario(flat_fun) && classic_race_has_scenario(ClassicRaceTrack{2}) &&
            !classic_race_has_scenario(ClassicRaceTrack{45}));
    rejects([&]{(void)classic_race_scenario(ClassicRaceTrack{45});});
    // Its state carries its own identity, URTR13 06 (the 742-byte layout, the
    // special-tile words of R-0047 and R-0051 with $0C73, the checkpoint
    // flags of R-0048 and the HUNTER words of R-0052), and round-trips.
    auto flat_start=classic_race_start(content,flat);
    require(flat_start.track==flat_fun && flat_start.movement.frame==1392);
    const auto flat_bytes=serialize_zoom_zoo(flat_start);
    const std::array<std::uint8_t,8> flat_magic{'U','R','T','R','1','3','0','6'};
    require(flat_bytes.size()==916 && classic_race_state_magic(flat_fun)==flat_magic && std::equal(flat_magic.begin(),flat_magic.end(),flat_bytes.begin()));
    const auto flat_restored=deserialize_zoom_zoo(flat_bytes);
    require(flat_restored.track==flat_fun && serialize_zoom_zoo(flat_restored)==flat_bytes);
    // An identity naming DRAGSTER, ZOOM ZOO or a stunt event (whose state is URTRnn07) is refused.
    for(const auto digits:{std::array<std::uint8_t,2>{'0','0'},std::array<std::uint8_t,2>{'0','2'},std::array<std::uint8_t,2>{'3','7'}}) {
        auto renamed=flat_bytes;renamed[4]=digits[0];renamed[5]=digits[1];
        rejects([&]{(void)deserialize_zoom_zoo(renamed);});
    }

    // One lap: a finished rider has no laps left and its single slot is its total.
    auto finished=start;finished.movement.countdown=0;finished.fade_level=30;
    finished.movement.frame=1328+2000;
    finished.player_announcements.hint_updates=static_cast<std::uint16_t>((2000U+30U)%300U);
    finished.player_announcements.hint_group=static_cast<std::uint16_t>(((2000U+30U)/300U)%8U);
    finished.start_boost={0,0};
    for(unsigned i=0;i<2;++i) {
        finished.race.riders[i].laps_remaining=0;finished.race.riders[i].finished=1;
        finished.race.lap_times[i][0]=static_cast<std::uint16_t>(3300+i);finished.race.total_times[i]=static_cast<std::uint16_t>(3300+i);
    }
    require(serialize_zoom_zoo(deserialize_zoom_zoo(serialize_zoom_zoo(finished)))==serialize_zoom_zoo(finished));
    auto two_slots=finished;two_slots.race.lap_times[0][1]=10;rejects([&]{(void)deserialize_zoom_zoo(serialize_zoom_zoo(two_slots));});

    // A landing clears held rotations on the update a released roll counts its
    // hold, so a supported rider may hold 1 with 0 rotations (original 1623 of
    // fuzz seed 31).
    auto landing_hold=finished;landing_hold.rolls[0].step=0xfffb;landing_hold.rolls[0].held_updates=1;
    landing_hold.rolls[0].prior_orientation=0;landing_hold.rolls[0].pose_base=0x8000;
    landing_hold.movement.riders[0].pose.reflected=true;
    require(serialize_zoom_zoo(deserialize_zoom_zoo(serialize_zoom_zoo(landing_hold)))==serialize_zoom_zoo(landing_hold));
    // It stays ahead while the rider bounces (original 3472 of seed 383), but
    // never beyond the elapsed updates.
    auto airborne_hold=landing_hold;airborne_hold.movement.riders[0].contact.unsupported_count=2;
    airborne_hold.rolls[0].support_count_mirror=2;
    require(serialize_zoom_zoo(deserialize_zoom_zoo(serialize_zoom_zoo(airborne_hold)))==serialize_zoom_zoo(airborne_hold));
    auto impossible_hold=landing_hold;impossible_hold.rolls[0].held_updates=2001;
    rejects([&]{(void)deserialize_zoom_zoo(serialize_zoom_zoo(impossible_hold));});

    // Stable result: winner 226, loser 242 updates of result loading.
    require(classic_race_player_won(finished) && stable_result_updates(finished)==226);
    auto lost=finished;lost.race.total_times={3301,3300};lost.race.lap_times[0][0]=3301;lost.race.lap_times[1][0]=3300;
    require(!classic_race_player_won(lost) && stable_result_updates(lost)==242);
    auto zoom_state=lost;zoom_state.track=ClassicRaceTrack::ZoomZoo;require(stable_result_updates(zoom_state)==115);

    // Race Again keeps the track.
    auto result=finished;result.race.finish_delay=240;result.result_updates=226;
    restart_zoom_zoo(result,content);
    require(result.track==ClassicRaceTrack::Dragster && serialize_zoom_zoo(result)==bytes);

    // A SNES D-pad cannot publish opposing directions (bsnes gamepad latch).
    ControllerButtons pressed{};pressed.up=pressed.down=pressed.left=true;pressed.b=true;
    auto physical=with_physical_dpad(pressed);
    require(!physical.up && !physical.down && physical.left && physical.b);
    pressed={};pressed.left=pressed.right=true;pressed.down=true;
    physical=with_physical_dpad(pressed);
    require(!physical.left && !physical.right && physical.down);

    // The shared race engine applies that rocker itself, so no caller can
    // advance either track with a direction pair the port cannot publish. The
    // paused update samples the controller into the state, so one update shows
    // both axes; DRAGSTER's accepted behaviour is unchanged because its app and
    // runner dropped the same pairs before the engine did.
    auto held=start;held.fade_level=30;held.pause.selection=1;
    auto opposed=held,steered=held,navigated=held;
    ControllerButtons nothing{},both{},left_only{},down_only{};
    both.left=both.right=both.up=both.down=true;left_only.left=true;down_only.down=true;
    update_zoom_zoo(held,nothing,content);
    update_zoom_zoo(opposed,both,content);
    update_zoom_zoo(steered,left_only,content);
    update_zoom_zoo(navigated,down_only,content);
    require(serialize_zoom_zoo(opposed)==serialize_zoom_zoo(held) && opposed.pause.selection==1);
    require(serialize_zoom_zoo(steered)!=serialize_zoom_zoo(held));
    require(navigated.pause.selection==0xffffU);

    // R-0060: from the menus the pause menu's second choice ends the race, left
    // as it was: 0xEA61 (a quit) after the countdown, 0xEA62 (a restart) in it.
    // Any other paused update carries on.
    auto quitting=start;quitting.fade_level=30;quitting.movement.countdown=0;
    quitting.pause.selection=0xffffU;quitting.pause.released=1;quitting.pause.suspended_updates=5;
    ControllerButtons confirm{};confirm.start=true;
    const auto before=serialize_zoom_zoo(quitting);
    auto over=update_race_for_menus(quitting,confirm,content);
    require(over && over->player_total==0xea61 && serialize_zoom_zoo(quitting)==before);
    auto restarting=quitting;restarting.movement.countdown=100;
    over=update_race_for_menus(restarting,confirm,content);
    require(over && over->player_total==0xea62);
    auto waiting=quitting;
    over=update_race_for_menus(waiting,nothing,content);
    require(!over && waiting.pause.suspended_updates==6);

    // R-0079: a two-pad race (2P on DRAGSTER) pauses from either pad ($83:CD05-CD28): pad 2's
    // opening puts the menu in the lower view; pad 1's axis chooses before pad 2's; the release
    // waits for both Starts ($83:CD39-CD4B). The pads publish once the fade has.
    auto local=start; // a local race's pairing and tier (R-0071)
    local.pairing={0,1};local.opponent_tier={0,0,0x60};
    initialize_split_cameras(local);
    auto racing=local;racing.fade_level=30;
    ControllerButtons second_start{};second_start.start=true;
    auto opened=racing;
    update_zoom_zoo(opened,nothing,second_start,content);
    require(opened.pause.selection==1 && opened.pause.lower_view && opened.pause.suspended_updates==1);
    auto upper=racing;
    update_zoom_zoo(upper,confirm,second_start,content);
    require(upper.pause.selection==1 && !upper.pause.lower_view);
    ControllerButtons down1{},up2{};down1.down=true;up2.up=true;
    auto chosen=opened;
    update_zoom_zoo(chosen,down1,up2,content);
    require(chosen.pause.selection==0xffffU);
    chosen=opened;
    update_zoom_zoo(chosen,nothing,up2,content);
    require(chosen.pause.selection==1);
    auto resumed=opened;
    update_zoom_zoo(resumed,nothing,content);
    require(resumed.pause.released==1);
    update_zoom_zoo(resumed,nothing,second_start,content);
    require(resumed.pause.selection==0 && !resumed.pause.lower_view && resumed.pause.released==1);
    update_zoom_zoo(resumed,nothing,second_start,content); // reopened and closed at once
    require(resumed.pause.selection==0 && resumed.pause.released==1 && resumed.pause.suspended_updates==4);
    // $83:F8D5-F912: QUIT (in the countdown, a restart) writes the confirming pad's total, pad
    // 1's when both hold Start.
    auto local_quit=opened;local_quit.pause.selection=0xffffU;local_quit.pause.released=1;
    over=update_race_for_menus(local_quit,nothing,second_start,content);
    require(over && over->opponent_total==0xea62 && over->player_total!=0xea62);
    local_quit=opened;local_quit.pause.selection=0xffffU;local_quit.pause.released=1;
    over=update_race_for_menus(local_quit,confirm,second_start,content);
    require(over && over->player_total==0xea62 && over->opponent_total!=0xea62);
    // The lower view is one more byte, 1, only while it is set; a stray or other byte is refused,
    // and so is a lower view without two pads. (A paused update from the start state, before the
    // countdown runs.)
    auto saved=local;saved.movement.frame+=1;saved.fade_level=1;saved.pause.selection=1;saved.pause.suspended_updates=1;
    saved.pause.lower_view=true;
    const auto open_bytes=serialize_zoom_zoo(saved);
    require(open_bytes.size()==785 && open_bytes.back()==1);
    require(serialize_zoom_zoo(deserialize_zoom_zoo(open_bytes))==open_bytes);
    auto upper_saved=saved;upper_saved.pause.lower_view=false;
    const auto upper_bytes=serialize_zoom_zoo(upper_saved);
    require(upper_bytes.size()==784 && std::equal(upper_bytes.begin(),upper_bytes.end(),open_bytes.begin()));
    auto two=open_bytes;two.back()=2;rejects([&]{(void)deserialize_zoom_zoo(two);});
    auto closed=serialize_zoom_zoo(local);closed.push_back(1);rejects([&]{(void)deserialize_zoom_zoo(closed);});
    auto one_view=saved;one_view.split_screen=false;rejects([&]{(void)serialize_zoom_zoo(one_view);});
    // R-0082: rider 1's tutorial hints ride in the trailer's flag (a league pair's in its last
    // byte, "over"); their count comes back from the clock. A rider 1 who is MIKE has none.
    auto hinted=saved;hinted.opponent_hints.active=true;
    const auto hinted_bytes=serialize_zoom_zoo(hinted);
    const auto hinted_read=deserialize_zoom_zoo(hinted_bytes);
    require(hinted_read.opponent_hints==hinted.opponent_hints && serialize_zoom_zoo(hinted_read)==hinted_bytes);
    auto hinted_mike=hinted;hinted_mike.pairing={1,0};
    rejects([&]{(void)deserialize_zoom_zoo(serialize_zoom_zoo(hinted_mike));});
    auto hinted_pair=hinted;hinted_pair.league_statistics.enabled=true;
    const auto hinted_pair_bytes=serialize_zoom_zoo(hinted_pair);
    require(hinted_pair_bytes[hinted_pair_bytes.size()-2]==0
            && deserialize_zoom_zoo(hinted_pair_bytes).opponent_hints.active);
    // A league pair's wrapper carries the same byte at its end.
    auto pair_saved=saved;pair_saved.league_statistics.enabled=true;
    const auto pair_bytes=serialize_zoom_zoo(pair_saved);
    require(pair_bytes.back()==1 && serialize_zoom_zoo(deserialize_zoom_zoo(pair_bytes))==pair_bytes);
    auto pair_closed=upper_saved;pair_closed.league_statistics.enabled=true;
    require(serialize_zoom_zoo(pair_closed).size()+1==pair_bytes.size());
    auto unpaused=local;unpaused.league_statistics.enabled=true;
    auto stray_unpaused=serialize_zoom_zoo(unpaused);stray_unpaused.push_back(1);
    rejects([&]{(void)deserialize_zoom_zoo(stray_unpaused);});
    // $83:F6FD-F791: a league race against the computer shows only a message after the countdown:
    // Down does not move the choice, so Start resumes rather than quits.
    auto league=start;league.fade_level=30;league.movement.countdown=0;league.league_statistics.enabled=true;
    update_zoom_zoo(league,confirm,content);
    require(league.pause.selection==1);
    update_zoom_zoo(league,down_only,content);
    require(league.pause.selection==1);

    // R-0081: a split race shows its finish display only once both riders have finished. In VS
    // ($77:0750 bit 2) the first finisher's banner driver, 360 updates from its first odd update,
    // then finishes the other rider without a time (0xFFFF); in 2P the race waits. (The finish
    // routine alone, with a finish pose table of one pose that loops.)
    const std::array<std::uint8_t,12> looping_pose{0,0,0xce,0xc7,0xce,0xc7,0,1,0,0x80,0,0};
    auto finish_content=content;finish_content.finish_poses=looping_pose;
    const auto finish_update=[&](ZoomZooState& state){update_finish(state,finish_content);++state.movement.frame;};
    auto player_done=local;player_done.fade_level=30;player_done.movement.countdown=0;
    player_done.player_announcements.hints_active=0;
    player_done.race.riders[0].finished=1;player_done.race.riders[0].laps_remaining=0;
    player_done.race.lap_times[0][0]=player_done.race.total_times[0]=3601;
    auto two_player=player_done;
    for(unsigned update=0;update<600;++update) finish_update(two_player);
    require(!two_player.race.riders[1].finished && !two_player.race.finish_delay);
    require(two_player.race.banners==(std::array<ZoomZooBannerDriver,2>{}));
    auto versus=player_done;versus.versus=true;
    unsigned updates=0;
    while(!versus.race.riders[1].finished && updates<400) {finish_update(versus);++updates;}
    // The player's block runs first, so the opponent's driver starts in the same update.
    require((updates==361 || updates==362) && versus.race.riders[1].finished==forced_finish);
    require(versus.race.banners[0].index>=7 && !versus.race.banners[0].life && versus.race.banners[1].life==359);
    require(!versus.race.finish_delay && versus.race.total_times[1]==no_time && versus.race.riders[1].laps_remaining);
    const auto forced_bytes=serialize_zoom_zoo(versus);
    require(forced_bytes.size()==792 && forced_bytes[7]=='H');
    require(serialize_zoom_zoo(deserialize_zoom_zoo(forced_bytes))==forced_bytes);
    finish_update(versus);
    require(versus.race.finish_delay==1);
    // Without the drivers the forced flag is refused; so are a stray member, two live drivers,
    // and drivers in a race that is not VS.
    rejects([&]{(void)deserialize_zoom_zoo(std::span(forced_bytes).first(784));});
    auto member=forced_bytes;member[784]=6;rejects([&]{(void)deserialize_zoom_zoo(member);});
    auto both_live=forced_bytes;both_live[786]=1;rejects([&]{(void)deserialize_zoom_zoo(both_live);});
    auto stray=player_done;stray.race.banners[0]={8,300};rejects([&]{(void)serialize_zoom_zoo(stray);});
    // A split state's display counts only once both have finished; a league wrapper holds no VS state.
    require(serialize_zoom_zoo(deserialize_zoom_zoo(serialize_zoom_zoo(two_player)))==serialize_zoom_zoo(two_player));
    auto counting_early=two_player;counting_early.race.finish_delay=1;
    rejects([&]{(void)deserialize_zoom_zoo(serialize_zoom_zoo(counting_early));});
    std::vector<std::uint8_t> wrapped{'U','R','L','G','0','0','0','1',
        static_cast<std::uint8_t>(forced_bytes.size()&0xffU),static_cast<std::uint8_t>(forced_bytes.size()>>8U),0,1,1};
    wrapped.insert(wrapped.end(),forced_bytes.begin(),forced_bytes.end());
    wrapped.resize(wrapped.size()+97);
    rejects([&]{(void)deserialize_zoom_zoo(wrapped);});
    // The opponent finishing first forces the player, whose block runs on the next update.
    auto opponent_first=local;opponent_first.fade_level=30;opponent_first.movement.countdown=0;opponent_first.versus=true;
    opponent_first.race.riders[1].finished=1;opponent_first.race.riders[1].laps_remaining=0;
    opponent_first.race.lap_times[1][0]=opponent_first.race.total_times[1]=3601;
    updates=0;
    while(!opponent_first.race.riders[0].finished && updates<400) {finish_update(opponent_first);++updates;}
    require((updates==361 || updates==362) && opponent_first.race.riders[0].finished==forced_finish);
    require(opponent_first.race.banners[0]==ZoomZooBannerDriver{});
    finish_update(opponent_first);
    require(opponent_first.race.banners[0].life==359 && opponent_first.race.finish_delay==1);
}
