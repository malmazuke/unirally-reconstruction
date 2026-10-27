#include "name_check.hpp"

#include <cstddef>
#include <stdexcept>

namespace unirally::front_end_screens {

namespace {

constexpr std::size_t word_count = 71;               // $83:851E: CMP #$0047
constexpr std::uint16_t word_table_address = 0x8537; // $83:8505: LDA $838537,X
constexpr std::uint8_t word_end = 0xff;

// The check's CPU time in master clocks, step by step as the core runs it (R-0101, cycles.py):
// the call and the way back, each position the name is read at, each word compared there, and
// each character a word matches before it differs. A match leaves sooner than the name's end.
constexpr std::uint32_t call_clocks = 484;      // $80:A4B2 to $83:84EE, and back to $80:A4B6
constexpr std::uint32_t position_clocks = 266;  // $83:84EE-8500 and $83:8528
constexpr std::uint32_t word_clocks = 720;      // $83:8501-850D, $83:8779-8799 and $83:851B-8525
constexpr std::uint32_t character_clocks = 174; // $83:877E-878C, a character the word matches
constexpr std::uint32_t match_saving = 116;     // $83:850F-851A against $83:852D-8534

// A PAL frame is 312 lines of 1,364 master clocks, 40 of each line the work RAM's refresh, which
// the CPU waits through. OK's JSL comes after vblank's start by the keyboard's pass (its text and
// OAM uploads and the pad tests); the NMI that interrupts a long check runs its handler.
constexpr std::uint32_t line_clocks = 1364, refresh_clocks = 40, frame_lines = 312;
constexpr std::uint32_t frame_clocks = line_clocks * frame_lines;
constexpr std::uint32_t start_clocks = 31600; // 30,948-32,390 in the captures (R-0101)
constexpr std::uint32_t nmi_clocks = 1082;    // $00:8587 while the keyboard shows
// From the check's return to the frame wait's first read of `$4212` (`$80:FAE3`): the way to the
// loop, or the refusal's print (`$80:A4B8-A4C6`) first.
constexpr std::uint32_t accepted_tail_clocks = 1100, refused_tail_clocks = 26926;

struct CheckCounts {
    std::uint32_t positions{}, words{}, characters{};
    bool refused{};
};

std::span<const std::uint8_t> listed_word(std::span<const std::uint8_t> words, std::size_t index) {
    const auto at = 2 * index;
    const auto address =
        static_cast<unsigned>(words[at]) | (static_cast<unsigned>(words[at + 1]) << 8U);
    if (address < word_table_address + 2 * word_count
        || address - word_table_address >= words.size())
        throw std::invalid_argument("name check word outside its table");
    return words.subspan(address - word_table_address);
}

// $83:8779: the characters the word matches from `position`, up to its 0xFF (a match) or its
// first difference. The name's 0xFF never matches a word's character.
std::uint32_t matched_characters(std::span<const std::uint8_t> word,
                                 std::span<const std::uint8_t> name, std::size_t position,
                                 bool& whole) {
    std::uint32_t matched = 0;
    for (std::size_t at = 0;; ++at) {
        if (at == word.size()) throw std::invalid_argument("name check word without its end");
        if (word[at] == word_end) {
            whole = true;
            return matched;
        }
        if (position + at >= name.size() || word[at] != name[position + at]) return matched;
        ++matched;
    }
}

// $83:84DD-8534: every position from the first, every word at each, until a word matches or the
// name's 0xFF is reached.
CheckCounts count_check(std::span<const std::uint8_t> name, std::span<const std::uint8_t> words) {
    CheckCounts counts;
    for (std::size_t position = 0;; ++position) {
        ++counts.positions;
        if (position == name.size()) return counts;
        for (std::size_t index = 0; index < word_count; ++index) {
            ++counts.words;
            bool whole = false;
            counts.characters +=
                matched_characters(listed_word(words, index), name, position, whole);
            if (whole) {
                counts.refused = true;
                return counts;
            }
        }
    }
}

// The master clocks from vblank's start in OK's frame to the frame wait's first `$4212` read.
std::uint32_t frame_wait_clocks(const CheckCounts& counts) {
    const auto cpu = call_clocks + position_clocks * counts.positions + word_clocks * counts.words
                   + character_clocks * counts.characters - (counts.refused ? match_saving : 0);
    const auto with_refresh = cpu * line_clocks / (line_clocks - refresh_clocks);
    auto end = start_clocks + with_refresh;
    for (auto nmis = 0U; end / frame_clocks > nmis; ++nmis) end += nmi_clocks; // one per vblank
    return end + (counts.refused ? refused_tail_clocks : accepted_tail_clocks);
}

} // namespace

NameCheck check_name(std::span<const std::uint8_t> name, std::span<const std::uint8_t> words) {
    if (words.empty()) return {};
    if (words.size() < 2 * word_count) throw std::invalid_argument("name check word list is short");
    const auto counts = count_check(name, words);
    return {counts.refused, static_cast<std::uint8_t>(frame_wait_clocks(counts) / frame_clocks)};
}

} // namespace unirally::front_end_screens
