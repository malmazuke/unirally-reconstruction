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

struct Options {
    std::filesystem::path seed, content, pack_path, inputs;
    std::string start_state;
};

// One seed or start state, one content directory or pack, and the controller stream.
Options parse_options(int argc, char** argv) {
    Options options;
    for (int index = 1; index < argc; index += 2) {
        if (index + 1 >= argc)
            throw std::invalid_argument("movement runner requires option values");
        const std::string option = argv[index];
        if (option == "--seed")
            options.seed = argv[index + 1];
        else if (option == "--content-dir")
            options.content = argv[index + 1];
        else if (option == "--content-pack")
            options.pack_path = argv[index + 1];
        else if (option == "--start-state")
            options.start_state = argv[index + 1];
        else if (option == "--inputs")
            options.inputs = argv[index + 1];
        else
            throw std::invalid_argument("unknown movement runner option: " + option);
    }
    if (options.inputs.empty() || (options.seed.empty() == options.start_state.empty())
        || (options.content.empty() == options.pack_path.empty())) {
        throw std::invalid_argument("movement runner requires exactly one seed/start state, one "
                                    "content directory/pack, and inputs");
    }
    return options;
}

// The movement content, from the pack or a loose directory; it owns the loose bytes the
// content's spans point into.
struct BoundContent {
    std::unique_ptr<unirally::ClassicContentPack> pack;
    std::vector<std::uint8_t> masks, decrements, track, poses, templates, transitions, columns,
        flags, slopes, displacement, idle_pose, reward, reward_class;

    std::span<const std::uint8_t> bytes(const std::vector<std::uint8_t>& loose,
                                        const char* logical_id) const {
        return pack ? pack->entry(logical_id) : std::span<const std::uint8_t>(loose);
    }
    unirally::MovementContent movement() const {
        return {
            {bytes(track, "physics.track.dragster.data"),
             bytes(poses, "physics.rider.collision-poses"),
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
    }
};

void bind_content(BoundContent& c, const Options& options) {
    if (!options.pack_path.empty())
        c.pack = std::make_unique<unirally::ClassicContentPack>(options.pack_path);
    const auto load = [&](const char* filename) -> std::vector<std::uint8_t> {
        if (c.pack) return {};
        return read_bytes(options.content / filename);
    };
    c.masks = load("speed-masks.bin");
    c.decrements = load("speed-decrements.bin");
    c.track = load("track-data.bin");
    c.poses = load("collision-poses.bin");
    c.templates = load("collision-templates.bin");
    c.transitions = load("progress-transitions.bin");
    c.columns = load("tile-tables.bin");
    c.flags = load("tile-flags.bin");
    c.slopes = load("pose-slopes.bin");
    c.displacement = load("displacement-table.bin");
    c.idle_pose = load("idle-pose-table.bin");
    c.reward = load("rotation-reward.bin");
    c.reward_class = load("rotation-class.bin");
}

// The controller stream ("frame player opponent"), each state printed after its update;
// port 1 (the opponent) must stay released.
int run_controller_stream(unirally::MovementState& state, const unirally::MovementContent& content,
                          const std::filesystem::path& inputs) {
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
        unirally::update_movement(state, buttons(static_cast<std::uint16_t>(player_mask)), content);
        emit(state);
    }
    if (!stream.eof()) throw std::invalid_argument("malformed controller input stream");
    return 0;
}

} // namespace

int main(int argc, char** argv) try {
    const auto options = parse_options(argc, argv);
    auto state = options.seed.empty()
                   ? unirally::classic_crawler_dragster_start()
                   : unirally::deserialize_movement_state(read_bytes(options.seed));
    if (!options.start_state.empty()
        && options.start_state != "classic.crawler.dragster.race-start.v1")
        throw std::invalid_argument("unsupported semantic start-state ID");
    BoundContent content;
    bind_content(content, options);
    const auto movement = content.movement();
    return run_controller_stream(state, movement, options.inputs);
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
}
