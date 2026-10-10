// ROM-free checks for a stunt event's race picture (R-0068): the score field's cells and the
// qualifying score, the countdown's windows, `stunt` in the left field, the score field's place
// in the one-player NMI's upload order, the clock that stops at 0:00.0, no direction arrow, and
// the finish drivers armed by the finish display; NEON's lighting, colour math and contact
// (track 42). Every state and table here is synthetic.
#include "presentation.hpp"
#include "vertical_contact.hpp"
#include "zoom_zoo_movement.hpp"

#include <array>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>

namespace {

using namespace unirally;

void require(bool value, const char* what) {
    if (!value) throw std::runtime_error(std::string("stunt HUD expectation failed: ") + what);
}
template <class F> void rejects(F action, const char* what) {
    bool rejected = false;
    try {
        action();
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected, what);
}

std::string text(const std::array<char, 3>& cells) { return {cells.begin(), cells.end()}; }

const ClassicRaceTrack bowl{2};

// $81:C374-C3CA and $81:F29C-F2FD: the hundreds cell is written from 100, the tens from 10; a
// cell not written keeps what it held. $81:CD8D-CE0F: the qualifying score moves left below 100.
void score_cells() {
    const std::array<char, 3> setup{' ', ' ', '0'};
    require(text(stunt_score_cells(0, setup)) == "  0", "zero keeps the setup's cells");
    require(text(stunt_score_cells(4, setup)) == "  4", "one digit");
    require(text(stunt_score_cells(49, setup)) == " 49", "two digits");
    require(text(stunt_score_cells(121, setup)) == "121", "three digits");
    require(text(stunt_score_cells(105, setup)) == "105", "a zero tens digit is written from 100");
    require(text(stunt_score_cells(7, {'9', '8', '6'})) == "987", "unwritten cells keep theirs");
    require(text(stunt_score_cells(1234, setup)) == "c34", "a hundreds count past 9 is a letter");
    rejects([] { (void)stunt_score_cells(3600, {' ', ' ', '0'}); }, "past the letters");
    require(stunt_qualifying_text(68) == "68 ", "a qualifying score below 100");
    require(stunt_qualifying_text(245) == "245", "a qualifying score from 100");
    require(stunt_qualifying_text(5) == "05 ", "the tens digit is always written");
    require(stunt_qualifying_text(100) == "100", "zero tens and units");
    // R-0095, $81:E705-E82E: a split race's cells start at column 2, each written cell moving the
    // next right; nothing is written before the first score.
    const std::array<char, 3> blank{' ', ' ', ' '};
    require(text(split_stunt_score_cells(8, blank)) == "8  ", "one digit from column 2");
    require(text(split_stunt_score_cells(24, split_stunt_score_cells(8, blank))) == "24 ",
            "two digits over one");
    require(text(split_stunt_score_cells(105, blank)) == "105", "three digits");
    require(text(split_stunt_score_cells(7, {'1', '2', '3'})) == "723", "past the digits, theirs");
}

// $83:E5E2, $83:E634, $83:E686, $83:E6E8: no transition member, each digit from its phase's first
// update, GO from 100 on the $0300 parity. A race keeps its waits.
void countdown_windows() {
    const unsigned transition = 6;
    for (const bool parity : {false, true}) {
        const unsigned go = parity ? 3U : 4U;
        require(classic_countdown_window(270, parity, transition, true) == 0U, "phase 1 at 270");
        require(classic_countdown_window(221, parity, transition, true) == 0U, "phase 1 at 221");
        require(classic_countdown_window(220, parity, transition, true) == 1U, "phase 2 at 220");
        require(classic_countdown_window(161, parity, transition, true) == 1U, "phase 2 at 161");
        require(classic_countdown_window(160, parity, transition, true) == 2U, "phase 3 at 160");
        require(classic_countdown_window(101, parity, transition, true) == 2U, "phase 3 at 101");
        require(classic_countdown_window(100, parity, transition, true) == go, "GO at 100");
        require(classic_countdown_window(1, parity, transition, true) == go, "GO at 1");
        require(!classic_countdown_window(0, parity, transition, true), "nothing at 0");
        require(classic_countdown_window(270, parity, transition) == transition, "a race waits");
        require(classic_countdown_window(100, parity, transition) == transition, "until 70");
    }
}

ZoomZooState stunt_state() {
    ZoomZooState state{};
    state.track = bowl;
    state.stunt.qualifying_score = 68;
    state.race.riders[0].laps_remaining = 1;
    state.race.riders[1].laps_remaining = 1;
    state.movement.timer = {0, 4, 5, 0, 0};
    return state;
}

void left_field_and_text() {
    const auto scenario = classic_race_scenario(bowl);
    require(scenario.stunt_event, "track 2 is a stunt event");
    auto state = stunt_state();
    const auto left = classic_hud_left_field(state, scenario);
    require(left.text == "stunt" && left.column == 2, "stunt from column 2");
    state.race.riders[0].finished = 1; // the clock's end finishes without crossing the line
    require(classic_hud_left_field(state, scenario).text == "stunt", "stunt after the finish");
    state.player_announcements.queue.feature_total = 49;
    const auto hud = classic_race_hud_text(state, scenario, std::nullopt);
    require(hud.score_field == " 49/68 ", "the score field without the queue's history");
    state.stunt.clock_stopped = 1;
    state.movement.timer = {0, 5, 9, 9, 0}; // the stopping tick wraps the lower digits
    require(classic_race_hud_text(state, scenario, std::nullopt).clock == "0:00:0",
            "a stopped clock shows 0:00.0");
    require(classic_race_hud_text(stunt_state(), classic_race_scenario(ClassicRaceTrack{3}),
                                  std::nullopt)
                .score_field.empty(),
            "a race has no score field");
}

// $81:C357-C3D0 asks for the score field on the update the points are paid, which is the update
// the trick's caption is taken; the NMI writes the score first ($81:F28B) and the caption a
// picture later ($81:F30C).
void upload_order() {
    ClassicRaceHudClock queue;
    auto idle = stunt_state();
    idle.player_announcements.queue.cooldown = 1;
    queue.observe_update(idle, idle);
    queue.observe_update(idle, idle); // the clock digits are written
    require(queue.published().clock == std::optional<std::string>("0:45:0"), "the clock");
    auto paid = idle;
    paid.player_announcements.queue.read_cursor = 1;
    paid.player_announcements.queue.entries[1] = 18;
    paid.player_announcements.queue.feature_total = 4;
    queue.observe_update(idle, paid);
    queue.observe_update(paid, paid);
    require(text(queue.published().score_cells) == "  4" && queue.published().caption_event == 0,
            "the score before the caption");
    queue.observe_update(paid, paid);
    require(queue.published().caption_event == 18, "the caption a picture later");
    // The clock's stopping tick sets `$034D` without new digits: the NMI spends the update
    // rewriting 0:00:0, so a caption taken with it waits a picture.
    auto last_tenth = paid;
    last_tenth.movement.timer = {0, 0, 0, 0, 0};
    queue.observe_update(paid, last_tenth);
    queue.observe_update(last_tenth, last_tenth);
    require(queue.published().clock == std::optional<std::string>("0:00:0"), "0:00.0 written");
    auto stopped = last_tenth;
    stopped.stunt.clock_stopped = 1;
    stopped.movement.timer = {0, 5, 9, 9, 0};
    stopped.player_announcements.queue.read_cursor = 2;
    stopped.player_announcements.queue.entries[2] = 19;
    queue.observe_update(last_tenth, stopped);
    queue.observe_update(stopped, stopped);
    require(queue.published().clock == std::optional<std::string>("0:00:0") &&
                queue.published().caption_event == 18,
            "the stopping tick spends the update and keeps 0:00:0");
    queue.observe_update(stopped, stopped);
    require(queue.published().caption_event == 19, "then the caption");
    queue.observe_update(stopped, stopped);
    require(queue.published().clock == std::optional<std::string>("0:00:0"), "and it stays");
}

// $82:98B7-98C8: a stunt event blanks the arrow word every update.
void no_arrow() {
    auto behind = stunt_state();
    behind.movement.riders[1].progress.transition_count = 40;
    behind.movement.riders[0].progress.marker_word = 0x4000;
    require(!classic_race_arrow(behind, behind, 0), "no arrow in a stunt event");
    behind.track = ClassicRaceTrack{3};
    require(classic_race_arrow(behind, behind, 0).has_value(), "a race's player behind has one");
}

// $83:E898-E8DD then $83:E8E0: both drivers are armed on the update that sets `$0FE9`, not by
// the riders' finishes at the clock's end; the player's runs first.
void finish_drivers() {
    ClassicWindowPointer pointer;
    auto state = stunt_state();
    state.movement.frame = 3700; // long after the countdown, whose word is 0
    const auto run = [&](ZoomZooState next) {
        next.movement.frame = state.movement.frame + 1U;
        pointer.observe_update(state, next, 5);
        state = next;
    };
    run(state);
    auto finished = state;
    finished.race.riders[0].finished = 1;
    finished.race.riders[1].finished = 1;
    for (int k = 0; k < 4; ++k) run(finished);
    require(!pointer.published(), "a finish alone arms nothing");
    auto display = finished;
    display.stunt.finish_display = 1;
    run(display); // $0FE9 set
    run(display); // the player's driver first runs: member 7, or 8 on an odd `$0300`
    require(!pointer.published(), "published for the next picture");
    run(display);
    const auto member = pointer.published();
    require(member == 7U || member == 8U, "the banner from the finish display");
}

// $83:D1EE-D247: a quarter of the distance, at least one, in eight-bit arithmetic; $83:D24A-D271:
// red 15, blue 30, the level's low five bits as green. The levels here are made up.
void neon_lighting() {
    const std::array<std::uint8_t, 8> levels{0, 8, 40, 100, 3, 60, 31, 0xC0};
    require(neon_green_step(0, 2, levels) == 10, "up by a quarter");
    require(neon_green_step(38, 2, levels) == 39, "at least one");
    require(neon_green_step(40, 2, levels) == 40, "at the level");
    require(neon_green_step(20, 4, levels) == 16, "down by a quarter");
    require(neon_green_step(4, 4, levels) == 3, "down at least one, not past the level");
    require(neon_green_step(0, 3, levels) == 25, "a level of 100");
    // 0xC0 reads as below 20 in the signed test: down by (20 - 0xC0) & 0xFF >> 2 = 21, to 0xFF,
    // which is not past the level in eight bits (0xC0 - 0xFF is negative).
    require(neon_green_step(20, 7, levels) == 0xFF, "the eighth byte, eight-bit arithmetic");
    rejects([&] { (void)neon_green_step(0, 8, levels); }, "past the table");
    require(neon_colour(0) == 0x780F, "red 15 and blue 30");
    require(neon_colour(0x13) == (0x780F | (0x13 << 5)), "green");
    require(neon_colour(0x3F) == neon_colour(0x1F), "five bits of green");
    require(subtract_colour(0x7FFF, 0x780F) == (16U | (31U << 5) | (1U << 10)), "channel by channel");
    require(subtract_colour(0x0001, 0x7FFF) == 0, "at least 0");
    auto neon = classic_race_scenario(ClassicRaceTrack{42});
    require(neon.neon_lighting && neon.stunt_event, "track 42 is NEON");
    require(!classic_race_scenario(bowl).neon_lighting, "BOWL is not");
}

// $81:8BD9-8BE0 and $81:8CDD-8CEE: an empty cell of palette 7 is a probe of penetration 0x7F
// whose angle is the tile-column table's byte 1 + `$12D1` (X, $81:8BA0): byte 1 in a race, byte 2
// on NEON, negated after a mirrored tile. $81:900E-9025: the first probe is selected only when
// its angle less 0xE0 is not negative in eight bits. The column table here is made up.
void neon_contact() {
    std::array<std::uint8_t, 64> columns{};
    for (unsigned i = 0; i < columns.size(); i += 2) columns[i] = 0xA0;
    columns[2] = 0x30; // tile 0, column 1's height: the angle NEON's X reads
    const std::array<std::uint8_t, 2> flags{};
    const FlatContactContent content{columns, flags};
    CollisionPoints points{};
    TrackSamples samples{};
    samples[0] = 0x1C00; // empty, palette 7
    const auto race = summarize_vertical_contact(content, points, samples, 0, 0);
    require(race.selected_word == 0x1C00 && race.boundary_marker, "a race's angle 0 is selected");
    columns[1] = 0x90; // byte 1: a race's angle, were it not 0
    const auto race_steep = summarize_vertical_contact(content, points, samples, 0, 0);
    require(race_steep.selected_word == 0 && race_steep.supported, "a race reads byte 1");
    columns[1] = 0;
    const auto lit = summarize_vertical_contact(content, points, samples, 0, 0, true);
    require(lit.selected_word == 0x1C00 && lit.selected_high == 0x1C && lit.angle == 0x30,
            "angle 0x30 is selected");
    samples[1] = 0x4002; // tile 1, mirrored: processed before the first probe
    const auto mirrored = summarize_vertical_contact(content, points, samples, 0, 0, true);
    // Not selected, the first probe leaves the word to the tiled empty probe after it ($81:9063).
    require(mirrored.selected_word == 0x4002 && mirrored.selected_high == 0, "angle 0xD0 is not");
    columns[2] = 0xA0;
    samples[1] = 0;
    const auto steep = summarize_vertical_contact(content, points, samples, 0, 0, true);
    require(steep.selected_word == 0 && steep.supported, "angle 0xA0 is not, the support stays");
}

} // namespace

int main() {
    try {
        score_cells();
        countdown_windows();
        left_field_and_text();
        upload_order();
        no_arrow();
        finish_drivers();
        neon_lighting();
        neon_contact();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
