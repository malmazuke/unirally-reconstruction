#include "content_pack.hpp"
#include "zoom_zoo_movement.hpp"
#include "presentation.hpp"
#include <charconv>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>
namespace {
unirally::ZoomZooState load_state(const char* path) {
    std::ifstream input(path,std::ios::binary);
    if(!input)throw std::invalid_argument("cannot read state");
    const std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(input),{}};
    return unirally::deserialize_zoom_zoo(bytes);
}
unsigned parse_unsigned(std::string_view text,const char* name) {
    unsigned value{};
    const auto parsed=std::from_chars(text.data(),text.data()+text.size(),value);
    if(parsed.ec!=std::errc{} || parsed.ptr!=text.data()+text.size())
        throw std::invalid_argument(std::string("invalid ")+name);
    return value;
}
unirally::ZoomZooState parse_timeline_row(const std::string& line,unsigned& frame) {
    const auto space=line.find(' ');
    if(space==std::string::npos)throw std::invalid_argument("timeline row lacks a state");
    frame=parse_unsigned(std::string_view(line).substr(0,space),"timeline frame");
    const std::string_view hex=std::string_view(line).substr(space+1);
    if(hex.size()%2)throw std::invalid_argument("timeline state has odd hex length");
    std::vector<std::uint8_t> bytes(hex.size()/2);
    for(std::size_t i=0;i<bytes.size();++i) {
        const auto parsed=std::from_chars(hex.data()+2*i,hex.data()+2*i+2,bytes[i],16);
        if(parsed.ec!=std::errc{} || parsed.ptr!=hex.data()+2*i+2)
            throw std::invalid_argument("timeline state is not hexadecimal");
    }
    const auto state=unirally::deserialize_zoom_zoo(bytes);
    if(state.movement.frame!=frame)throw std::invalid_argument("timeline row frame differs from its state");
    return state;
}
void write_frame(const char* path,const unirally::RgbFrame& frame) {
    std::ofstream out(path,std::ios::binary);
    out<<"P6\n256 224\n255\n";
    out.write(reinterpret_cast<const char*>(frame.pixels.data()),static_cast<std::streamsize>(frame.pixels.size()));
    if(!out)throw std::runtime_error("cannot write rendered frame");
}
} // namespace
int main(int argc,char** argv) try {
    // One picture of either track's shared race state, drawn as the app draws it.
    if(argc==4 && std::string_view(argv[2])=="--window-index") {
        // Index-level check (R-0040): for every row of a native timeline, the
        // channel-6 window member the renderer selects, with the tracked
        // history and without it (as a single restored state would), or "-"
        // when channel 6 is disabled for that frame.
        unirally::ClassicContentPack pack(argv[1]);
        std::ifstream input(argv[3]);
        if(!input)throw std::invalid_argument("cannot read timeline");
        unirally::ClassicRaceHistoryTracker history;
        std::string line;
        unsigned frame{};
        if(!std::getline(input,line))throw std::invalid_argument("timeline is empty");
        auto previous=parse_timeline_row(line,frame);
        const auto setup=unirally::classic_race_scenario(previous.track).initialization_frame+6U;
        const auto print=[&](const unirally::ZoomZooState& state) {
            const auto tracked=unirally::classic_window_table_index(state,setup,history.on_screen().opponent_finish_frame);
            const auto alone=unirally::classic_window_table_index(state,setup,std::nullopt);
            std::cout<<state.movement.frame<<' '<<(tracked?std::to_string(*tracked):"-")<<' '<<(alone?std::to_string(*alone):"-")<<'\n';
        };
        print(previous);
        while(std::getline(input,line)) {
            auto state=parse_timeline_row(line,frame);
            history.observe_update(previous,state,pack);
            print(state);
            previous=std::move(state);
        }
        return 0;
    }
    if(argc==6 && std::string_view(argv[2])=="--timeline") {
        // Replays consecutive native states from the timeline's first row so the
        // rider look overlays (R-0036) and the opponent's finish frame (R-0040)
        // follow the race, then draws FRAME.
        unirally::ClassicContentPack pack(argv[1]);
        const auto target=parse_unsigned(argv[4],"frame");
        std::ifstream input(argv[3]);
        if(!input)throw std::invalid_argument("cannot read timeline");
        unirally::ClassicRaceHistoryTracker history;
        std::string line;
        unsigned frame{};
        if(!std::getline(input,line))throw std::invalid_argument("timeline is empty");
        auto previous=parse_timeline_row(line,frame);
        if(!previous.native_initialization || frame!=unirally::classic_race_scenario(previous.track).initialization_frame)
            throw std::invalid_argument("timeline must begin at the native race initialization");
        if(target<=frame)throw std::invalid_argument("frame must follow the timeline's first row");
        const auto content=unirally::classic_race_presentation_content(pack,previous.track);
        while(std::getline(input,line)) {
            const auto previous_frame=frame;
            auto state=parse_timeline_row(line,frame);
            if(frame!=previous_frame+1U)throw std::invalid_argument("timeline rows must be consecutive");
            history.observe_update(previous,state,pack);
            if(frame==target) {
                const auto on_screen=history.on_screen();
                write_frame(argv[5],unirally::render_classic_race(state,content,&previous,&on_screen));
                return 0;
            }
            previous=std::move(state);
        }
        throw std::invalid_argument("frame is beyond the timeline");
    }
    if(argc!=4 && argc!=5)
        throw std::invalid_argument("usage: classic_race_presentation_runner PACK STATE OUT.ppm [PREVIOUS_STATE]\n"
                                    "       classic_race_presentation_runner PACK --timeline NATIVE_TIMELINE FRAME OUT.ppm\n"
                                    "       classic_race_presentation_runner PACK --window-index NATIVE_TIMELINE");
    unirally::ClassicContentPack pack(argv[1]);
    const auto state=load_state(argv[2]);
    // The HUD and riders show the previous update; without PREVIOUS_STATE
    // they are drawn one update ahead. A single state carries no rider look
    // history, so the upper-body overlays are omitted here, and an opponent-won
    // banner is drawn only once both finish times are known.
    const auto previous=argc==5?load_state(argv[4]):state;
    const auto content=unirally::classic_race_presentation_content(pack,state.track);
    write_frame(argv[3],unirally::render_classic_race(state,content,&previous));
    return 0;
} catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
