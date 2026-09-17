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
    // 256 x 64 arm ($81:A445). Other arms are unreached and rejected.
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
    for(unsigned shape:{0x80U,0x20U,0x10U,0x08U,0x01U}) {auto other=header;other[13]=static_cast<std::uint8_t>(shape);rejects([&]{(void)track_geometry(other);});}
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
}
