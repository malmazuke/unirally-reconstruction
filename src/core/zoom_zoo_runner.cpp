#include "zoom_zoo_pack.hpp"
#include <cctype>
#include <memory>
#include <set>

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
std::vector<std::uint8_t> read_bytes(const std::filesystem::path& path) {
    std::ifstream input(path,std::ios::binary);
    if (!input) throw std::runtime_error("cannot open native movement input: " + path.string());
    return {std::istreambuf_iterator<char>(input),std::istreambuf_iterator<char>()};
}

unirally::ControllerButtons buttons(std::uint16_t mask) {
    unirally::ControllerButtons value{};
    value.b=mask&(1U<<0); value.y=mask&(1U<<1); value.select=mask&(1U<<2);
    value.start=mask&(1U<<3); value.up=mask&(1U<<4); value.down=mask&(1U<<5);
    value.left=mask&(1U<<6); value.right=mask&(1U<<7); value.a=mask&(1U<<8);
    value.x=mask&(1U<<9); value.left_shoulder=mask&(1U<<10);
    value.right_shoulder=mask&(1U<<11);
    return value;
}

void emit(const unirally::ZoomZooState& state) {
    std::cout << state.movement.frame << ' ';
    for(auto byte:unirally::serialize_zoom_zoo(state))std::cout << std::hex << std::setw(2) << std::setfill('0') << unsigned(byte);
    std::cout << std::dec << '\n';
}
}
int main(int argc,char** argv) try {
    if(argc!=7 && argc!=9)throw std::invalid_argument("usage: zoom_zoo_runner --seed FILE --content-dir DIR --inputs FILE");
    std::filesystem::path seed,content,inputs;
    bool native_start=false,restart=false;
    unirally::ClassicRaceTrack race_track=unirally::ClassicRaceTrack::ZoomZoo;
    std::filesystem::path pack_path,track_override;
    std::set<std::string> seen;
    for(int i=1;i<argc;i+=2) {
        const std::string option=argv[i];
        if(!seen.insert(option).second)throw std::invalid_argument("repeated ZOOM ZOO runner option: "+option);
        if(option=="--seed")seed=argv[i+1];
        else if(option=="--restart-from") {seed=argv[i+1];restart=true;}
        else if(option=="--start") {
            const std::string scenario=argv[i+1];
            // classic.crawler.dragster, classic.crawler.zoom-zoo, or classic.track.NN
            // for any race track with a recovered scenario (TRACK-BREADTH part 3).
            if(scenario=="classic.crawler.dragster")race_track=unirally::ClassicRaceTrack::Dragster;
            else if(scenario.size()==16 && scenario.starts_with("classic.track.") &&
                    std::isdigit(static_cast<unsigned char>(scenario[14])) && std::isdigit(static_cast<unsigned char>(scenario[15]))) {
                race_track=unirally::ClassicRaceTrack{static_cast<std::uint8_t>((scenario[14]-'0')*10+(scenario[15]-'0'))};
                if(!unirally::classic_race_has_scenario(race_track))throw std::invalid_argument("track has no recovered scenario");
            }
            else if(scenario!="classic.crawler.zoom-zoo")throw std::invalid_argument("unknown scenario");
            native_start=true;
        }
        else if(option=="--content-pack")pack_path=argv[i+1];
        else if(option=="--content-dir")content=argv[i+1];
        else if(option=="--inputs")inputs=argv[i+1];
        // TRACK-BREADTH laboratory experiment: another track's decoded data,
        // tile columns and tile flags (track-data.bin, tile-tables.bin,
        // tile-flags.bin, from `tools/unirally_lab/content/tracks.py`) in
        // place of the pack's, on the scenario `--start` names.
        else if(option=="--track-override")track_override=argv[i+1];
        else throw std::invalid_argument("unknown ZOOM ZOO runner option");
    }
    // One origin (a seed, a restart seed or a native start) and one content
    // source (a pack or a loose directory).
    if(seen.count("--seed")+seen.count("--restart-from")+seen.count("--start")!=1 ||
       seen.count("--content-pack")+seen.count("--content-dir")!=1)
        throw std::invalid_argument("ZOOM ZOO runner needs one of --seed/--restart-from/--start and one of --content-pack/--content-dir");
    if((seed.empty()&&!native_start)||(content.empty()&&pack_path.empty())||inputs.empty())throw std::invalid_argument("missing ZOOM ZOO runner option");
    auto state=native_start?unirally::ZoomZooState{}:unirally::deserialize_zoom_zoo(read_bytes(seed));
    if(native_start)state.complete_race=state.sustained=true;
    const auto pack=pack_path.empty()?nullptr:std::make_unique<unirally::ClassicContentPack>(pack_path);
    // A pack binds content through the shared accessors, by track. The loose
    // content directory remains for the historical M4-12 to M4-15 cases and
    // is read only without a pack.
    const auto load=[&](const char* filename) {
        return pack?std::vector<std::uint8_t>{}:read_bytes(content/filename);
    };
    const auto track=load("track-data.bin");
    const auto poses=load("collision-poses.bin");
    const auto templates=load("collision-templates.bin");
    const auto columns=load("tile-tables.bin");
    const auto flags=load("tile-flags.bin");
    const auto progress=load("progress-transitions.bin");
    const auto slopes=load("pose-slopes.bin");
    const auto displacement=load("displacement-table.bin");
    const auto idle=load("idle-pose-table.bin");
    const auto reward=load((state.complete_race?"race-finish-reward-values.bin":state.sustained?"sustained-reward-values.bin":"rotation-reward.bin"));
    const auto reward_class=load((state.complete_race?"race-finish-reward-classes.bin":state.sustained?"sustained-reward-classes.bin":"rotation-class.bin"));
    const auto masks=load("speed-masks.bin");
    const auto decrements=load("speed-decrements.bin");
    const auto coefficients=load((state.sustained?"sustained-slope-coefficients.bin":"reflected-vertical-slope-coefficients.bin"));
    const auto reflection=load("reflection-pose-table.bin");
    const unirally::MovementContent movement{{track,poses,templates},{columns,flags},progress,slopes,displacement,idle,reward,reward_class,{masks,decrements}};
    const auto landing=load("landing-response-matrices.bin");
    const auto finish_poses=state.complete_race?load("race-finish-poses.bin"):std::vector<std::uint8_t>{};
    const auto roll_poses=pack?load("roll-pose-table.bin"):std::vector<std::uint8_t>{};
    const auto roll_directions=pack?load("roll-direction-table.bin"):std::vector<std::uint8_t>{};
    const auto weights=pack?load("roll-reward-weights.bin"):std::vector<std::uint8_t>{};
    const auto combinations=pack?load("trick-combinations.bin"):std::vector<std::uint8_t>{};
    const unirally::ZoomZooContent zoom_zoo_data{movement,coefficients,reflection,landing,finish_poses,roll_poses,roll_directions,weights,combinations,{},{},{}};
    if(!native_start)race_track=state.track;
    if(race_track!=unirally::ClassicRaceTrack::ZoomZoo && !pack)throw std::invalid_argument("a race on any track but ZOOM ZOO requires the content pack");
    if(pack && !(native_start || (state.complete_race && state.sustained)))
        throw std::invalid_argument("a content pack binds the complete-race content; earlier seeds use --content-dir");
    if(!track_override.empty() && !(pack && native_start))throw std::invalid_argument("--track-override needs --content-pack and --start");
    const auto override_track=track_override.empty()?std::vector<std::uint8_t>{}:read_bytes(track_override/"track-data.bin");
    const auto override_columns=track_override.empty()?std::vector<std::uint8_t>{}:read_bytes(track_override/"tile-tables.bin");
    const auto override_flags=track_override.empty()?std::vector<std::uint8_t>{}:read_bytes(track_override/"tile-flags.bin");
    auto data=!pack?zoom_zoo_data
        :unirally::classic_race_content(*pack,race_track);
    if(!track_override.empty()) {
        data.movement.sampling.track=override_track;
        data.movement.flat_contact={override_columns,override_flags};
    }
    if(native_start)state=unirally::classic_race_start(data,unirally::classic_race_scenario(race_track));
    if(restart)unirally::restart_zoom_zoo(state,data);
    unirally::validate_zoom_zoo_content_state(state,data);
    std::ifstream stream(inputs);
    if(!stream)throw std::runtime_error("cannot open ZOOM ZOO controller stream");
    emit(state);
    std::string line;
    while(std::getline(stream,line)) {
        unsigned frame,player,opponent;std::string trailing;
        std::istringstream row(line);
        if(!(row>>frame>>player>>opponent) || (row>>trailing))
            throw std::invalid_argument("malformed ZOOM ZOO controller row");
        if(frame!=state.movement.frame+1 || player>4095 || opponent!=0)throw std::invalid_argument("invalid ZOOM ZOO controller row");
        // The controller stream is what a device reports; update_zoom_zoo
        // applies the rocker the original's controller port applies.
        const auto pressed=buttons(static_cast<std::uint16_t>(player));
        try {unirally::update_zoom_zoo(state,pressed,data);}
        catch(const std::exception& e){std::cerr << "frame " << frame << ": " << e.what() << '\n';return 1;}
        emit(state);
    }
    if(!stream.eof())throw std::invalid_argument("malformed ZOOM ZOO controller stream");
    return 0;
} catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
