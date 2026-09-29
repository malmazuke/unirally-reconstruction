#pragma once
// Both riders' inputs during the idle demo, $83:E254-E55B.
#include "zoom_zoo_movement.hpp"

namespace unirally {
struct DemoTrickButtons {
    std::array<bool, 2> a{}, x{};
};
DemoTrickButtons update_demo_controllers(ZoomZooState& state, bool pad_pressed);
} // namespace unirally
