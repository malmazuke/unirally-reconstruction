#include "race_camera.hpp"
#include "zoom_zoo_pack.hpp"
#include <algorithm>
#include <cctype>
#include <memory>
#include <optional>
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

// One timeline row, the state after its update. STUNT-HUD: with `palettes`, also the player's
// contact palette ("frame palette"), which NEON's picture follows and the state does not carry.
void emit(const unirally::ZoomZooState& state, std::ostream* palettes, std::ostream* split_state) {
    std::cout << state.movement.frame << ' ';
    for (auto byte : unirally::serialize_zoom_zoo(state))
        std::cout << std::hex << std::setw(2) << std::setfill('0') << unsigned(byte);
    std::cout << std::dec << '\n';
    if (palettes)
        *palettes << state.movement.frame << ' ' << unsigned(state.player_contact_palette) << '\n';
    if (split_state) {
        const auto& camera = state.race.second_camera;
        *split_state << state.movement.frame;
        for (const auto value : {camera.x, camera.y, camera.velocity_x, camera.velocity_y,
                                 camera.lookahead, camera.screen_xy, state.demo.elapsed})
            *split_state << ' ' << value;
        *split_state << ' ' << state.demo.exit_requested << '\n';
    }
}

// The runner's options: one origin (a seed, a restart seed or a native start) and one content
// source (a pack or a loose directory), the controller stream, and an optional track override.
struct Options {
    std::filesystem::path seed, content, inputs, pack_path, track_override;
    // STUNT-HUD: where to write the player's contact palette by update (R-0068).
    std::filesystem::path contact_palettes;
    std::filesystem::path split_state;
    bool native_start = false, restart = false;
    bool split_screen = false;
    unirally::ClassicRaceTrack race_track = unirally::ClassicRaceTrack::ZoomZoo;
    // RACE-RIDERS-OPPONENTS: another rider or opponent than the scenario's MIKE against its
    // usual opponent, and the tutorial hints' start ($77:1116's rider bit), on a --start race.
    std::optional<unsigned> rider, opponent;
    bool tutorial_hints = true, pairing_given = false;
    // STUNT-EVENT-RACE: the rider's best medal on the tour (0-3), which sets a stunt event's
    // qualifying score, on a --start race.
    std::optional<unsigned> best_medal;
};

unsigned small_number(const std::string& text) {
    if (text.empty() || text.size() > 2 || !std::all_of(text.begin(), text.end(), [](char c) {
            return std::isdigit(static_cast<unsigned char>(c));
        }))
        throw std::invalid_argument(
            "a rider, an opponent, a hints flag or a medal is a small number");
    return static_cast<unsigned>(std::stoi(text));
}

// classic.crawler.dragster, classic.crawler.zoom-zoo, or classic.track.NN for any race track
// with a recovered scenario (TRACK-BREADTH part 3).
unirally::ClassicRaceTrack scenario_track(const std::string& scenario) {
    if (scenario == "classic.crawler.dragster") return unirally::ClassicRaceTrack::Dragster;
    if (scenario.size() == 16 && scenario.starts_with("classic.track.")
        && std::isdigit(static_cast<unsigned char>(scenario[14]))
        && std::isdigit(static_cast<unsigned char>(scenario[15]))) {
        const unirally::ClassicRaceTrack track{
            static_cast<std::uint8_t>((scenario[14] - '0') * 10 + (scenario[15] - '0'))};
        if (!unirally::classic_race_has_scenario(track))
            throw std::invalid_argument("track has no recovered scenario");
        return track;
    }
    if (scenario != "classic.crawler.zoom-zoo") throw std::invalid_argument("unknown scenario");
    return unirally::ClassicRaceTrack::ZoomZoo;
}

Options parse_options(int argc, char** argv) {
    if (argc < 7 || argc % 2 == 0)
        throw std::invalid_argument(
            "usage: zoom_zoo_runner --seed FILE --content-dir DIR --inputs FILE");
    Options options;
    std::set<std::string> seen;
    for (int i = 1; i < argc; i += 2) {
        const std::string option = argv[i];
        if (!seen.insert(option).second)
            throw std::invalid_argument("repeated ZOOM ZOO runner option: " + option);
        if (option == "--seed")
            options.seed = argv[i + 1];
        else if (option == "--restart-from") {
            options.seed = argv[i + 1];
            options.restart = true;
        } else if (option == "--start") {
            options.race_track = scenario_track(argv[i + 1]);
            options.native_start = true;
        } else if (option == "--content-pack")
            options.pack_path = argv[i + 1];
        else if (option == "--content-dir")
            options.content = argv[i + 1];
        else if (option == "--inputs")
            options.inputs = argv[i + 1];
        // TRACK-BREADTH laboratory experiment: another track's decoded data, tile columns and
        // tile flags (track-data.bin, tile-tables.bin, tile-flags.bin, from
        // `tools/unirally_lab/content/tracks.py`) in place of the pack's, on the scenario
        // `--start` names.
        else if (option == "--track-override")
            options.track_override = argv[i + 1];
        else if (option == "--rider")
            options.rider = small_number(argv[i + 1]);
        else if (option == "--opponent")
            options.opponent = small_number(argv[i + 1]);
        else if (option == "--tutorial-hints") {
            const auto hints = small_number(argv[i + 1]);
            if (hints > 1) throw std::invalid_argument("--tutorial-hints is 0 or 1");
            options.tutorial_hints = hints == 1;
        } else if (option == "--contact-palettes") {
            options.contact_palettes = argv[i + 1];
        } else if (option == "--split-state") {
            options.split_state = argv[i + 1];
        } else if (option == "--best-medal") {
            options.best_medal = small_number(argv[i + 1]);
            if (*options.best_medal > 3) throw std::invalid_argument("--best-medal is 0 to 3");
        } else if (option == "--split-screen") {
            options.split_screen = small_number(argv[i + 1]) == 1;
        } else
            throw std::invalid_argument("unknown ZOOM ZOO runner option");
    }
    if (seen.count("--seed") + seen.count("--restart-from") + seen.count("--start") != 1
        || seen.count("--content-pack") + seen.count("--content-dir") != 1)
        throw std::invalid_argument("ZOOM ZOO runner needs one of --seed/--restart-from/--start "
                                    "and one of --content-pack/--content-dir");
    options.pairing_given = options.rider || options.opponent || !options.tutorial_hints;
    if ((options.pairing_given || options.best_medal) && !options.native_start)
        throw std::invalid_argument(
            "--rider, --opponent, --tutorial-hints and --best-medal need --start");
    if ((options.seed.empty() && !options.native_start)
        || (options.content.empty() && options.pack_path.empty()) || options.inputs.empty())
        throw std::invalid_argument("missing ZOOM ZOO runner option");
    return options;
}

// The loose content directory of the historical M4-12 to M4-15 cases, read only without a
// pack. It owns the bytes the content's spans point into.
struct LooseContent {
    std::vector<std::uint8_t> track, poses, templates, columns, flags, progress, slopes;
    std::vector<std::uint8_t> displacement, idle, reward, reward_class, masks, decrements;
    std::vector<std::uint8_t> coefficients, reflection, landing, finish_poses, roll_poses;
    std::vector<std::uint8_t> roll_directions, weights, combinations;

    unirally::ZoomZooContent content() const {
        const unirally::MovementContent movement{{track, poses, templates},
                                                 {columns, flags},
                                                 progress,
                                                 slopes,
                                                 displacement,
                                                 idle,
                                                 reward,
                                                 reward_class,
                                                 {masks, decrements}};
        return {
            movement, coefficients, reflection, landing, finish_poses, roll_poses, roll_directions,
            weights,  combinations, {},         {},      {},           {},         {},
            {},       {},           {}};
    }
};

LooseContent load_loose_content(const std::filesystem::path& directory,
                                const unirally::ZoomZooState& state, bool pack) {
    const auto load = [&](const char* filename) {
        return pack ? std::vector<std::uint8_t>{} : read_bytes(directory / filename);
    };
    LooseContent c;
    c.track = load("track-data.bin");
    c.poses = load("collision-poses.bin");
    c.templates = load("collision-templates.bin");
    c.columns = load("tile-tables.bin");
    c.flags = load("tile-flags.bin");
    c.progress = load("progress-transitions.bin");
    c.slopes = load("pose-slopes.bin");
    c.displacement = load("displacement-table.bin");
    c.idle = load("idle-pose-table.bin");
    c.reward = load((state.complete_race ? "race-finish-reward-values.bin"
                     : state.sustained   ? "sustained-reward-values.bin"
                                         : "rotation-reward.bin"));
    c.reward_class = load((state.complete_race ? "race-finish-reward-classes.bin"
                           : state.sustained   ? "sustained-reward-classes.bin"
                                               : "rotation-class.bin"));
    c.masks = load("speed-masks.bin");
    c.decrements = load("speed-decrements.bin");
    c.coefficients = load((state.sustained ? "sustained-slope-coefficients.bin"
                                           : "reflected-vertical-slope-coefficients.bin"));
    c.reflection = load("reflection-pose-table.bin");
    c.landing = load("landing-response-matrices.bin");
    c.finish_poses =
        state.complete_race ? load("race-finish-poses.bin") : std::vector<std::uint8_t>{};
    c.roll_poses = pack ? load("roll-pose-table.bin") : std::vector<std::uint8_t>{};
    c.roll_directions = pack ? load("roll-direction-table.bin") : std::vector<std::uint8_t>{};
    c.weights = pack ? load("roll-reward-weights.bin") : std::vector<std::uint8_t>{};
    c.combinations = pack ? load("trick-combinations.bin") : std::vector<std::uint8_t>{};
    return c;
}

// Which content can drive which state: another track needs the pack; the pack binds the
// complete-race content; a track override needs the pack and a native start.
void check_content_choice(const Options& options, const unirally::ZoomZooState& state, bool pack,
                          unirally::ClassicRaceTrack race_track) {
    if (race_track != unirally::ClassicRaceTrack::ZoomZoo && !pack)
        throw std::invalid_argument("a race on any track but ZOOM ZOO requires the content pack");
    if (pack && !(options.native_start || (state.complete_race && state.sustained)))
        throw std::invalid_argument(
            "a content pack binds the complete-race content; earlier seeds use --content-dir");
    if (!options.track_override.empty() && !(pack && options.native_start))
        throw std::invalid_argument("--track-override needs --content-pack and --start");
}

// The controller stream, one row per update ("frame player opponent"), each state printed
// after its update. The stream is what a device reports; update_zoom_zoo applies the rocker
// the original's controller port applies.
int run_controller_stream(unirally::ZoomZooState& state, const unirally::ZoomZooContent& data,
                          const std::filesystem::path& inputs, std::ostream* palettes,
                          std::ostream* split_state) {
    std::ifstream stream(inputs);
    if (!stream) throw std::runtime_error("cannot open ZOOM ZOO controller stream");
    emit(state, palettes, split_state);
    std::string line;
    while (std::getline(stream, line)) {
        unsigned frame, player, opponent;
        std::string trailing;
        std::istringstream row(line);
        if (!(row >> frame >> player >> opponent) || (row >> trailing))
            throw std::invalid_argument("malformed ZOOM ZOO controller row");
        if (frame != state.movement.frame + 1 || player > 4095 || opponent != 0)
            throw std::invalid_argument("invalid ZOOM ZOO controller row");
        const auto pressed = buttons(static_cast<std::uint16_t>(player));
        try {
            unirally::update_zoom_zoo(state, pressed, data);
        } catch (const std::exception& e) {
            std::cerr << "frame " << frame << ": " << e.what() << '\n';
            return 1;
        }
        emit(state, palettes, split_state);
    }
    if (!stream.eof()) throw std::invalid_argument("malformed ZOOM ZOO controller stream");
    return 0;
}

} // namespace

int main(int argc, char** argv) try {
    const auto options = parse_options(argc, argv);
    auto state = options.native_start ? unirally::ZoomZooState{}
                                      : unirally::deserialize_zoom_zoo(read_bytes(options.seed));
    if (options.native_start) state.complete_race = state.sustained = true;
    const auto pack = options.pack_path.empty()
                        ? nullptr
                        : std::make_unique<unirally::ClassicContentPack>(options.pack_path);
    const auto loose = load_loose_content(options.content, state, pack != nullptr);
    const auto race_track = options.native_start ? options.race_track : state.track;
    check_content_choice(options, state, pack != nullptr, race_track);
    const auto& override_dir = options.track_override;
    const auto override_track = override_dir.empty() ? std::vector<std::uint8_t>{}
                                                     : read_bytes(override_dir / "track-data.bin");
    const auto override_columns = override_dir.empty()
                                    ? std::vector<std::uint8_t>{}
                                    : read_bytes(override_dir / "tile-tables.bin");
    const auto override_flags = override_dir.empty() ? std::vector<std::uint8_t>{}
                                                     : read_bytes(override_dir / "tile-flags.bin");
    auto data = !pack ? loose.content() : unirally::classic_race_content(*pack, race_track);
    if (!override_dir.empty()) {
        data.movement.sampling.track = override_track;
        data.movement.flat_contact = {override_columns, override_flags};
    }
    if (options.native_start) {
        auto scenario = unirally::classic_race_scenario(race_track);
        if (options.pairing_given) {
            auto pairing = scenario.pairing;
            if (options.rider) pairing.rider = static_cast<std::uint8_t>(*options.rider);
            if (options.opponent) pairing.opponent = static_cast<std::uint8_t>(*options.opponent);
            scenario = unirally::classic_race_scenario(race_track, pairing, options.tutorial_hints);
        }
        if (options.best_medal)
            scenario.best_medal = static_cast<std::uint8_t>(*options.best_medal);
        state = unirally::classic_race_start(data, scenario);
        if (options.split_screen) {
            unirally::initialize_split_cameras(state);
            state.demo_ai = true;
            state.opponent_hints.active = true;
            state.pairing = {4, 14};
            state.opponent_tier.ai_level = 0;
        }
    }
    if (options.restart) unirally::restart_zoom_zoo(state, data);
    unirally::validate_zoom_zoo_content_state(state, data);
    std::ofstream palettes;
    if (!options.contact_palettes.empty()) {
        palettes.open(options.contact_palettes);
        if (!palettes) throw std::runtime_error("cannot write the contact palettes");
    }
    std::ofstream split_state;
    if (!options.split_state.empty()) {
        split_state.open(options.split_state);
        if (!split_state) throw std::runtime_error("cannot write split state");
    }
    return run_controller_stream(state, data, options.inputs,
                                 palettes.is_open() ? &palettes : nullptr,
                                 split_state.is_open() ? &split_state : nullptr);
} catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
}
