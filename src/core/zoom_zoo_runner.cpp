#include "zoom_zoo_movement.hpp"
#include "content_pack.hpp"
#include <memory>

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
    if(argc!=7)throw std::invalid_argument("usage: zoom_zoo_runner --seed FILE --content-dir DIR --inputs FILE");
    std::filesystem::path seed,content,inputs;
    bool native_start=false,restart=false;
    std::filesystem::path pack_path;
    for(int i=1;i<argc;i+=2) {
        const std::string option=argv[i];
        if(option=="--seed")seed=argv[i+1];
        else if(option=="--restart-from") {seed=argv[i+1];restart=true;}
        else if(option=="--start") {
            if(std::string(argv[i+1])!="classic.crawler.zoom-zoo")throw std::invalid_argument("unknown scenario");
            native_start=true;
        }
        else if(option=="--content-pack")pack_path=argv[i+1];
        else if(option=="--content-dir")content=argv[i+1];
        else if(option=="--inputs")inputs=argv[i+1];
        else throw std::invalid_argument("unknown ZOOM ZOO runner option");
    }
    if((seed.empty()&&!native_start)||(content.empty()&&pack_path.empty())||inputs.empty())throw std::invalid_argument("missing ZOOM ZOO runner option");
    auto state=native_start?unirally::ZoomZooState{}:unirally::deserialize_zoom_zoo(read_bytes(seed));
    if(native_start)state.complete_race=state.sustained=true;
    const auto pack=pack_path.empty()?nullptr:std::make_unique<unirally::ClassicContentPack>(pack_path);
    const auto load=[&](const char* filename) {
        if(!pack)return read_bytes(content/filename);
        auto name=std::string(filename);name.resize(name.size()-4);
        const auto bytes=pack->entry("zoom."+name);
        return std::vector<std::uint8_t>(bytes.begin(),bytes.end());
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
    const unirally::ZoomZooContent data{movement,coefficients,reflection,landing,finish_poses,roll_poses,roll_directions,weights,combinations};
    if(native_start)state=unirally::classic_crawler_zoom_zoo_start(data);
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
        try {unirally::update_zoom_zoo(state,buttons(static_cast<std::uint16_t>(player)),data);}
        catch(const std::exception& e){std::cerr << "frame " << frame << ": " << e.what() << '\n';return 1;}
        emit(state);
    }
    if(!stream.eof())throw std::invalid_argument("malformed ZOOM ZOO controller stream");
    return 0;
} catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
