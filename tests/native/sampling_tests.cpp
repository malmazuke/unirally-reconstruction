#include "track_sampling.hpp"
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <vector>

static void require(bool condition) {
    if (!condition) throw std::runtime_error("sampling expectation failed");
}
static void set_word(std::vector<std::uint8_t>& data, std::size_t offset, unsigned value) {
    data.at(offset) = static_cast<std::uint8_t>(value);
    data.at(offset + 1) = static_cast<std::uint8_t>(value >> 8);
}

int main(int argc, char** argv) {
    if (argc != 2) return 3;
    try {
        const std::string_view name{argv[1]};
        if (name == "pose") {
            // Authored record, including both bytes of the selector and
            // a byte-overflow template offset. No original content.
            const std::vector<std::uint8_t> poses{10,20,30,40,1,2,1,2};
            std::vector<std::uint8_t> templates(4144);
            for (unsigned i = 0; i < 16; ++i) templates.at(4128 + i) = static_cast<std::uint8_t>(i);
            templates[4128] = 250;
            const unirally::SamplingContent content{{}, poses, templates};
            const auto normal = unirally::collision_points(content, 0, false);
            const auto reflected = unirally::collision_points(content, 0, true);
            require(normal[0].x == 18 && normal[0].y == 20);
            require(normal[1].x == 38 && normal[1].y == 40);
            require(normal[2].x == 3 && normal[2].y == 3);
            require(normal[9].x == 23 && normal[9].y == 17);
            require(reflected[0].x == 45 && reflected[0].y == 20);
            require(reflected[2].x == 60 && reflected[2].y == 3);
        } else if (name == "grid") {
            std::vector<std::uint8_t> track(0x8090);
            for (unsigned block = 0; block < 4; ++block) {
                set_word(track, 15 + block * 2, block);
                for (unsigned cell = 0; cell < 16; ++cell) set_word(track, 0x800F + block * 32 + cell * 2, block * 100 + cell);
            }
            unirally::CollisionPoints points{};
            points[0] = {8,8}; points[1] = {16,8}; points[2] = {8,16}; points[3] = {16,16};
            const auto values = unirally::sample_track({track, {}, {}}, points, 48, 48, 2);
            require(values[0] == 15 && values[1] == 112 && values[2] == 203 && values[3] == 300);
            // 192 - 64 wraps to 0x80, so BMI chooses the left quadrant.
            // Ordinary unsigned comparison would select block 1, returning 100.
            points[0] = {192,0};
            const auto wrapped = unirally::sample_track({track, {}, {}}, points, 0, 0, 2);
            require(wrapped[0] == 0);
            // $81:8A60-8A99: in the last column (1 of 2) the right-hand
            // neighbours are column 0 of the same rows: block 1 on the left,
            // block 0 on the right, blocks 3 and 2 below.
            points[0] = {8,8}; points[1] = {72,8}; points[2] = {8,72}; points[3] = {72,72};
            const auto last = unirally::sample_track({track, {}, {}}, points, 64, 0, 2);
            require(last[0] / 100 == 1 && last[1] / 100 == 0 && last[2] / 100 == 3 && last[3] / 100 == 2);
            // $81:8A2C-8A3B: a negative y samples coarse cell (0, 0) whatever x is.
            const auto negative = unirally::sample_track({track, {}, {}}, points, 64, 0xFFC0, 2);
            require(negative[0] / 100 == 0 && negative[1] / 100 == 1 && negative[2] / 100 == 2 && negative[3] / 100 == 3);
        } else if (name == "bounds") {
            bool missing = false, edge = false;
            try { (void)unirally::collision_points({{}, {}, {}}, 0, false); }
            catch (const std::out_of_range&) { missing = true; }
            // A coarse width of zero has no playfield.
            try { (void)unirally::sample_track({{}, {}, {}}, {}, 64, 0, 0); }
            catch (const std::out_of_range&) { edge = true; }
            require(missing && edge);
        } else return 3;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n'; return 1;
    }
}
