#include "content_pack.hpp"
#include "zoom_zoo_movement.hpp"
#include "presentation.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
int main(int argc,char** argv) try {
    if(argc!=4 && argc!=5)
        throw std::invalid_argument("usage: zoom_zoo_presentation_runner PACK STATE OUT.ppm [PREVIOUS_STATE]");
    unirally::ClassicContentPack pack(argv[1]);
    const auto load=[](const char* path) {
        std::ifstream input(path,std::ios::binary);
        if(!input)throw std::invalid_argument("cannot read state");
        const std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(input),{}};
        return unirally::deserialize_zoom_zoo(bytes);
    };
    const auto state=load(argv[2]);
    // The HUD and riders show the previous update; without PREVIOUS_STATE
    // they are drawn one update ahead.
    const auto previous=argc==5?load(argv[4]):state;
    const auto frame=unirally::render_zoom_zoo(state,pack,&previous);
    std::ofstream out(argv[3],std::ios::binary);
    out<<"P6\n256 224\n255\n";
    out.write(reinterpret_cast<const char*>(frame.pixels.data()),frame.pixels.size());
    if(!out)throw std::runtime_error("cannot write rendered frame");
    return 0;
} catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
