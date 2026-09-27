#include "speed_limits.hpp"
#include <array>
#include <iostream>
#include <stdexcept>

void require(bool value) {
    if (!value) throw std::runtime_error("speed limit expectation failed");
}
int main() {
    try {
        std::array<std::uint8_t, 9> masks{};
        masks.fill(3);
        std::array<std::uint8_t, 18> decrements{};
        for (std::size_t index = 0; index < 9; ++index) decrements[index * 2] = 4;
        const unirally::SpeedDecayContent content{masks, decrements};
        unirally::SpeedLimitContext context;
        context.opponent = context.ai_enabled = true;
        context.pose_byte = 64;
        context.adjustment_limit = 96;
        context.player_base_cap = 448;
        context.update_counter = 3;
        context.friction_mode = 2;
        std::uint16_t vx = 600, vy = 1000;
        unirally::SpeedModifiers state{128, 10, 0};
        unirally::limit_rider_speed(vx, vy, state, context, content);
        require(vx == 512 && vy == 832 && state.boost == 124 && state.vertical_boost == 9);
        vx = 65536 - 600;
        vy = 65536 - 1000;
        state = {128, 10, 0};
        unirally::limit_rider_speed(vx, vy, state, context, content);
        require(vx == 65536 - 512 && vy == 65536 - 832);
        context.opponent = false;
        context.player_progress = 65535;
        context.opponent_progress = 0;
        vx = 600;
        state = {};
        unirally::limit_rider_speed(vx, vy, state, context, content);
        require(state.progress_adjustment == 1 && vx == 448);
        vx = 600;
        unirally::limit_rider_speed(vx, vy, state, context, content);
        require(state.progress_adjustment == 2 && vx == 449);
        context.start_override = true;
        vx = 600;
        state = {128, 10, 12};
        unirally::limit_rider_speed(vx, vy, state, context, content);
        require(vx == 488 && state.progress_adjustment == 12 && state.boost == 124);
        context.start_override = false;
        context.friction_mode = 1;
        vx = 65535;
        state = {};
        unirally::limit_rider_speed(vx, vy, state, context, content);
        require(vx == 65535); // The original negative-one asymmetry.
        // BOWL and HUNTER's stunt event cap falling at 768 whatever the extra, and rising not at
        // all ($82:A84F-A86A); the horizontal cap and the vertical boost's decay are unchanged.
        for (const std::uint8_t track : {std::uint8_t{2}, std::uint8_t{42}}) {
            context.track = track;
            vx = 600;
            vy = 1000;
            state = {128, 10, 12};
            unirally::limit_rider_speed(vx, vy, state, context, content);
            require(vx == 517 && vy == 768 && state.vertical_boost == 9);
            vy = 65536 - 1000;
            state = {128, 10, 12};
            unirally::limit_rider_speed(vx, vy, state, context, content);
            require(vy == 65536 - 1000 && state.vertical_boost == 9);
        }
        context.track = 0;
        vx = 600;
        vy = 65536 - 1000;
        state = {128, 10, 12};
        unirally::limit_rider_speed(vx, vy, state, context, content);
        require(vy == 65536 - 838);
        vx = 600;
        state = {128, 10, 12};
        bool rejected = false;
        try { unirally::limit_rider_speed(vx, vy, state, context, {masks, {}}); }
        catch (const std::invalid_argument&) { rejected = true; }
        require(rejected && vx == 600 && state.progress_adjustment == 12);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
