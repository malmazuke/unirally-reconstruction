#include "content_pack.hpp"
#include "movement.hpp"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
std::vector<std::uint8_t> read_bytes(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("cannot open native movement input: " + path.string());
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

unirally::ControllerButtons buttons(std::uint16_t mask) {
    unirally::ControllerButtons value{};
    value.b = mask & (1U << 0);
    value.y = mask & (1U << 1);
    value.select = mask & (1U << 2);
    value.start = mask & (1U << 3);
    value.up = mask & (1U << 4);
    value.down = mask & (1U << 5);
    value.left = mask & (1U << 6);
    value.right = mask & (1U << 7);
    value.a = mask & (1U << 8);
    value.x = mask & (1U << 9);
    value.left_shoulder = mask & (1U << 10);
    value.right_shoulder = mask & (1U << 11);
    return value;
}

void emit(const unirally::MovementState& state) {
    const auto& player = state.riders[0];
    std::cout << state.frame << ' ' << unsigned(state.player_input.low_image) << ' '
              << unsigned(state.player_input.high_image) << ' '
              << unsigned(state.player_input.vertical) << ' '
              << unsigned(state.player_input.horizontal) << ' ' << player.motion.x << ' '
              << static_cast<std::int16_t>(player.motion.previous_x_displacement) << ' '
              << static_cast<std::int16_t>(player.throttle) << ' '
              << static_cast<std::int16_t>(player.motion.velocity_x) << ' ' << state.timer.minutes
              << ' ' << state.timer.tens_seconds << ' ' << state.timer.seconds << ' '
              << state.timer.tenths << ' ' << state.timer.subframe << ' ';
    for (const auto byte : unirally::serialize_movement_state(state)) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << unsigned(byte);
    }
    std::cout << std::dec << '\n';
}
}

int main(int argc, char** argv) try {
    std::filesystem::path seed, content, pack_path, inputs;
    std::string start_state;
    for (int index = 1; index < argc; index += 2) {
        if (index + 1 >= argc)
            throw std::invalid_argument("movement runner requires option values");
        const std::string option = argv[index];
        if (option == "--seed")
            seed = argv[index + 1];
        else if (option == "--content-dir")
            content = argv[index + 1];
        else if (option == "--content-pack")
            pack_path = argv[index + 1];
        else if (option == "--start-state")
            start_state = argv[index + 1];
        else if (option == "--inputs")
            inputs = argv[index + 1];
        else
            throw std::invalid_argument("unknown movement runner option: " + option);
    }
    if (inputs.empty() || (seed.empty() == start_state.empty())
        || (content.empty() == pack_path.empty())) {
        throw std::invalid_argument("movement runner requires exactly one seed/start state, one "
                                    "content directory/pack, and inputs");
    }
    auto state = seed.empty() ? unirally::classic_crawler_dragster_start()
                              : unirally::deserialize_movement_state(read_bytes(seed));
    if (!start_state.empty() && start_state != "classic.crawler.dragster.race-start.v1")
        throw std::invalid_argument("unsupported semantic start-state ID");
    std::unique_ptr<unirally::ClassicContentPack> pack;
    std::vector<std::uint8_t> masks, decrements, track, poses, templates, transitions, columns,
        flags, slopes, displacement, idle_pose, reward, reward_class;
    if (!pack_path.empty()) pack = std::make_unique<unirally::ClassicContentPack>(pack_path);
    const auto load = [&](const char* filename,
                          const char* logical_id) -> std::vector<std::uint8_t> {
        (void)logical_id;
        if (pack) return {};
        return read_bytes(content / filename);
    };
    masks = load("speed-masks.bin", "physics.speed.masks");
    decrements = load("speed-decrements.bin", "physics.speed.decrements");
    track = load("track-data.bin", "physics.track.dragster.data");
    poses = load("collision-poses.bin", "physics.rider.collision-poses");
    templates = load("collision-templates.bin", "physics.rider.collision-templates");
    transitions = load("progress-transitions.bin", "physics.track.progress-transitions");
    columns = load("tile-tables.bin", "physics.track.dragster.tile-columns");
    flags = load("tile-flags.bin", "physics.track.dragster.tile-flags");
    slopes = load("pose-slopes.bin", "physics.rider.pose-slopes");
    displacement = load("displacement-table.bin", "physics.rider.displacement-table");
    idle_pose = load("idle-pose-table.bin", "physics.rider.idle-pose-table");
    reward = load("rotation-reward.bin", "physics.reward.rotation-value");
    reward_class = load("rotation-class.bin", "physics.reward.rotation-class");
    const auto bytes = [&](const std::vector<std::uint8_t>& loose,
                           const char* logical_id) -> std::span<const std::uint8_t> {
        return pack ? pack->entry(logical_id) : std::span<const std::uint8_t>(loose);
    };
    const unirally::MovementContent movement_content{
        {bytes(track, "physics.track.dragster.data"), bytes(poses, "physics.rider.collision-poses"),
         bytes(templates, "physics.rider.collision-templates")},
        {bytes(columns, "physics.track.dragster.tile-columns"),
         bytes(flags, "physics.track.dragster.tile-flags")},
        bytes(transitions, "physics.track.progress-transitions"),
        bytes(slopes, "physics.rider.pose-slopes"),
        bytes(displacement, "physics.rider.displacement-table"),
        bytes(idle_pose, "physics.rider.idle-pose-table"),
        bytes(reward, "physics.reward.rotation-value"),
        bytes(reward_class, "physics.reward.rotation-class"),
        {bytes(masks, "physics.speed.masks"), bytes(decrements, "physics.speed.decrements")}};
    std::ifstream stream(inputs);
    if (!stream) throw std::runtime_error("cannot open controller input stream");
    std::cout << "unirally-movement-v1\n";
    emit(state);
    std::uint32_t frame{};
    unsigned player_mask{}, opponent_mask{};
    while (stream >> frame >> player_mask >> opponent_mask) {
        if (frame != state.frame + 1 || player_mask > 0xffffU || opponent_mask > 0xffffU) {
            throw std::invalid_argument("controller input frames or masks are invalid");
        }
        if (opponent_mask != 0)
            throw std::invalid_argument("controller port 1 is outside the recovered domain");
        unirally::update_movement(state, buttons(static_cast<std::uint16_t>(player_mask)),
                                  movement_content);
        emit(state);
    }
    if (!stream.eof()) throw std::invalid_argument("malformed controller input stream");
    return 0;
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
}
