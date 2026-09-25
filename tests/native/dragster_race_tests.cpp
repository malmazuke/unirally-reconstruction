// ROM-free checks for DRAGSTER on the shared race engine (R-0038): playfield
// geometry, scenario values, state identity and the physical D-pad adapter.
#include "zoom_zoo_movement.hpp"
#include <array>
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
    // listing (TRACK-BREADTH). 0x04 also sets $0FF7 and is rejected, as is any
    // value without an arm.
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
    for(unsigned shape:{0x04U,0x01U,0x02U,0xC0U}) {auto other=header;other[13]=static_cast<std::uint8_t>(shape);rejects([&]{(void)track_geometry(other);});}
    rejects([&]{(void)track_geometry(std::span<const std::uint8_t>(header.data(),13));});

    // Scenario: DRAGSTER initializes 48 frames before ZOOM ZOO on the accepted
    // menu path, races one lap in race mode 0, and settles its result later.
    const auto dragster_scenario=classic_race_scenario(ClassicRaceTrack::Dragster);
    const auto zoom_scenario=classic_race_scenario(ClassicRaceTrack::ZoomZoo);
    require(dragster_scenario.initialization_frame==1328 && dragster_scenario.laps==1 && !dragster_scenario.tour_race);
    require(zoom_scenario.initialization_frame==1376 && zoom_scenario.laps==3 && zoom_scenario.tour_race);
    require(race_adjustment_limit(dragster_scenario)==96 && race_adjustment_limit(zoom_scenario)==72);

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
    // one-run race like DRAGSTER, INFINITY (14) a seven-lap race; the stunt
    // events (2) and the tracks no cold start reaches (37) have no scenario.
    const ClassicRaceTrack flat_fun{13},infinity{14};
    const auto flat=classic_race_scenario(flat_fun),seven=classic_race_scenario(infinity);
    require(flat.initialization_frame==1392 && flat.laps==1 && !flat.tour_race && flat.stable_result_won==226);
    require(seven.initialization_frame==1376 && seven.laps==7 && seven.tour_race && seven.stable_result_won==115);
    require(classic_race_has_scenario(flat_fun) && !classic_race_has_scenario(ClassicRaceTrack{2}) &&
            !classic_race_has_scenario(ClassicRaceTrack{37}));
    rejects([&]{(void)classic_race_scenario(ClassicRaceTrack{2});});
    // Its state carries its own identity, URTR13 06 (the 742-byte layout, the
    // special-tile words of R-0047 and R-0051 with $0C73, the checkpoint
    // flags of R-0048 and the HUNTER words of R-0052), and round-trips.
    auto flat_start=classic_race_start(content,flat);
    require(flat_start.track==flat_fun && flat_start.movement.frame==1392);
    const auto flat_bytes=serialize_zoom_zoo(flat_start);
    const std::array<std::uint8_t,8> flat_magic{'U','R','T','R','1','3','0','6'};
    require(flat_bytes.size()==912 && classic_race_state_magic(flat_fun)==flat_magic && std::equal(flat_magic.begin(),flat_magic.end(),flat_bytes.begin()));
    const auto flat_restored=deserialize_zoom_zoo(flat_bytes);
    require(flat_restored.track==flat_fun && serialize_zoom_zoo(flat_restored)==flat_bytes);
    // An identity naming DRAGSTER, ZOOM ZOO or a track without a scenario is refused.
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
}
