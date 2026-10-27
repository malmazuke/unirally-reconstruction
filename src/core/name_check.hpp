#pragma once

#include <cstdint>
#include <span>

namespace unirally::front_end_screens {

// What the keyboard's OK finds when it checks a name against the word list (`$83:84D9`, R-0101).
struct NameCheck {
    bool refused{}; // a listed word appears somewhere in the name
    // Frames the check and what follows it run past OK's frame before the keyboard's frame wait
    // (`$80:FADF`): 0 for a short name, 1 from eight characters, 2 for the longest league names.
    std::uint8_t frames_late{};
};

// `$83:84D9`: the name (its characters, without the 0xFF that ends it) against the word list
// (`front-end.name-check-words`: 71 little-endian pointers into bank `$83` from `$83:8537`, then
// the words, each ended by 0xFF). An empty list (a pack before v41) refuses nothing and takes no
// time.
NameCheck check_name(std::span<const std::uint8_t> name, std::span<const std::uint8_t> words);

} // namespace unirally::front_end_screens
