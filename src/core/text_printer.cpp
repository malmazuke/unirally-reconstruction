#include "text_printer.hpp"

#include <stdexcept>
#include <vector>

namespace unirally {

namespace {

// Control codes, dispatched through `$80:C3DC` by 0xFF - code (`$80:C3C6-C3D9`).
constexpr std::uint8_t end_of_text = 0xff, position = 0xfe, number = 0xfd, centre = 0xfc,
                       nothing = 0xfb, attribute = 0xf9, rider_name = 0xf8, track_name = 0xf7,
                       race_time = 0xf1, two_digits = 0xf0, place_object = 0xef, capitals = 0xee,
                       first_control = 0xee;
constexpr std::uint8_t league_slot_name = 0xf3, league_current_name = 0xf6;
constexpr std::uint8_t row_code = 0xf2; // positions F7's name alone, like FC
constexpr std::uint16_t priority_bit = 0x2000;
constexpr std::uint8_t small_glyph = 0x80;
// A small glyph's tile is its entry (less the flag) plus 0x9F, over the tile 0x3C below
// (`$80:C579-C596`). Entries 0x64 and 0x65 take their own paths (`$80:C59B`, `$80:C5B2`).
constexpr unsigned small_tile_base = 0x9f, small_lower_row = 0x3c;

std::uint8_t table_entry(std::span<const std::uint8_t> table, std::uint8_t character) {
    // `$80:C3FE-C40C`: a backquote prints as an underscore, bytes from 0x8C as `?`.
    if (character == 0x60)
        character = 0x5f;
    else if (character >= 0x8c)
        character = 0x3f;
    return table[character];
}

void write(TextMap& map, unsigned at, std::uint16_t word) {
    map.words[at & 1023U] = word;
}

// `$80:C41E-C443`: tile, tile + 1, and tile + 0x50, tile + 0x51 on the row below.
void print_big(TextMap& map, TextCursor& cursor, std::uint8_t entry) {
    const auto tile = static_cast<std::uint16_t>((entry << 1U) | cursor.attribute | priority_bit);
    write(map, cursor.position, tile);
    write(map, cursor.position + 1, static_cast<std::uint16_t>(tile + 1));
    write(map, cursor.position + 32, static_cast<std::uint16_t>(tile + 0x50));
    write(map, cursor.position + 33, static_cast<std::uint16_t>(tile + 0x51));
    cursor.position += 2;
}

// `$80:C56C-C5C3`. The two special entries branch on an equal compare, so the carry is set
// and their `ADC #$009F` adds one more.
void print_small(TextMap& map, TextCursor& cursor, std::uint8_t entry) {
    const unsigned index = entry & 0x7fU;
    if (index == 0x64) {
        const auto tile =
            static_cast<std::uint16_t>((4U + small_tile_base + 1U) | cursor.attribute | 0x6000U);
        write(map, cursor.position, tile);
        write(map, cursor.position + 32, static_cast<std::uint16_t>(tile + small_lower_row));
    } else if (index == 0x65) {
        const auto tile = static_cast<std::uint16_t>((0x3aU + small_tile_base + 1U)
                                                     | cursor.attribute | priority_bit);
        write(map, cursor.position, tile);
        write(map, cursor.position + 32, tile);
    } else {
        const auto tile =
            static_cast<std::uint16_t>((index + small_tile_base) | cursor.attribute | priority_bit);
        write(map, cursor.position, tile);
        write(map, cursor.position + 32, static_cast<std::uint16_t>(tile + small_lower_row));
    }
    cursor.position += 1;
}

// `$80:C4B8-C4FD`: centre what follows on `row`, by its width in tiles up to the next 0xFF or
// 0xFB: two for a big glyph, one for a small one; 0xEF and the byte after it take no width.
void centre_on(TextCursor& cursor, std::span<const std::uint8_t> rest, std::uint8_t row,
               std::span<const std::uint8_t> table) {
    unsigned budget = 33;
    for (std::size_t at = 0; at < rest.size(); ++at) {
        const auto character = rest[at];
        if (character == end_of_text || character == nothing) break;
        if (character == place_object) {
            ++at;
            continue;
        }
        budget -= (table[character] & small_glyph) ? 1U : 2U;
    }
    cursor.position = row * 32U + ((budget & 0xffU) >> 1U);
}

const TextVariables& require(const TextVariables* variables) {
    if (!variables) throw std::invalid_argument("text printer variable code without variables");
    return *variables;
}

// The name of track `track`, up to its 0xFF (`$80:9B55`).
std::span<const std::uint8_t> name_of(std::span<const std::uint8_t> names, unsigned track) {
    std::size_t at = 0;
    for (unsigned k = 0; k < track; ++k) {
        while (at < names.size() && names[at] != end_of_text) ++at;
        ++at;
    }
    std::size_t end = at;
    while (end < names.size() && names[end] != end_of_text) ++end;
    if (end >= names.size()) throw std::invalid_argument("track name is not in the table");
    return names.subspan(at, end - at);
}

// A rider's 16-byte name record (`$80:9B2F`), to its 0xFF.
std::span<const std::uint8_t> record_name(std::span<const std::uint8_t> records, unsigned rider) {
    constexpr std::size_t record = 16;
    if ((rider + 1) * record > records.size())
        throw std::invalid_argument("rider name is not in the records");
    const auto name = records.subspan(rider * record, record);
    std::size_t end = 0;
    while (end < record && name[end] != end_of_text) ++end;
    return name.first(end);
}

// $80:9B79: a league name occupies 32 SRAM bytes; the printer copies at most 31.
std::span<const std::uint8_t> league_name(std::span<const std::uint8_t> records, unsigned slot) {
    constexpr std::size_t record = 32;
    if ((slot + 1) * record > records.size())
        throw std::invalid_argument("league name is not in the records");
    const auto name = records.subspan(slot * record, record);
    std::size_t end = 0;
    while (end < record - 1 && name[end] != end_of_text) ++end;
    return name.first(end);
}

// $80:F8B9: small letters become capitals, digits the big font's (0x16-0x1F). The original
// converts its whole buffer `$00DC-$00FB`, the FC or F2 look-ahead bytes at its start included.
void in_capitals(std::vector<std::uint8_t>& text) {
    for (auto& character : text) {
        if (character >= 'a' && character <= 'z')
            character = static_cast<std::uint8_t>(character - 0x20);
        else if (character >= '0' && character <= '9')
            character = static_cast<std::uint8_t>(character + 0xe6);
    }
}

} // namespace

std::array<std::uint8_t, 6> five_digit_text(std::uint16_t value) {
    std::array<std::uint8_t, 6> text{};
    unsigned rest = value, place = 10000;
    for (std::size_t k = 0; k < 5; ++k, place /= 10) {
        text[k] = static_cast<std::uint8_t>('0' + rest / place);
        rest %= place;
    }
    for (std::size_t k = 0; k < 4 && text[k] == '0'; ++k) text[k] = '_';
    text[5] = end_of_text;
    return text;
}

std::vector<std::uint8_t> race_time_text(std::uint16_t time,
                                         std::span<const std::uint8_t> time_words) {
    constexpr std::uint16_t quit_time = 0xea61, no_time = 0xea60;
    if (time == quit_time || time == no_time) {
        const auto word = name_of(time_words, time == quit_time ? 0 : 1);
        return {word.begin(), word.end()};
    }
    unsigned rest = time, first_minute = '0';
    if (time & 0x8000U) {
        rest -= 30000;
        first_minute = '5';
    }
    const auto digit = [&](unsigned place) {
        const auto value = rest / place;
        rest %= place;
        return static_cast<std::uint8_t>('0' + value);
    };
    const auto minutes = static_cast<std::uint8_t>(first_minute + rest / 6000);
    rest %= 6000;
    const auto tens = digit(1000), seconds = digit(100), tenths = digit(10), hundredths = digit(1);
    return {'_', minutes, ':', tens, seconds, '.', tenths, hundredths};
}

namespace {

std::uint8_t read_stream_byte(std::span<const std::uint8_t> stream, std::size_t& at) {
    if (at >= stream.size()) throw std::invalid_argument("text stream ends without 0xFF");
    return stream[at++];
}

void print_name_operand(TextMap& map, TextCursor& cursor, std::span<const std::uint8_t> stream,
                        std::size_t& at, std::span<const std::uint8_t> character_table,
                        const TextVariables* variables, std::uint8_t control) {
    // $80:C5D3, $80:C628, $80:C675 and $80:C67D; R-0072 observations 3 and 19.
    const auto& values = require(variables);
    std::vector<std::uint8_t> text;
    unsigned name_index = 0;
    if (control == league_slot_name) {
        name_index = read_stream_byte(stream, at);
    } else {
        const auto low = read_stream_byte(stream, at);
        const auto high = read_stream_byte(stream, at);
        const auto address = static_cast<std::uint16_t>(low | (high << 8U));
        name_index = values.word(address);
    }
    if (at < stream.size() && (stream[at] == row_code || stream[at] == centre)) {
        text.push_back(read_stream_byte(stream, at));
        text.push_back(read_stream_byte(stream, at));
    }
    if (control == track_name || control == rider_name) {
        const auto name = control == track_name ? name_of(values.track_names, name_index)
                                                : record_name(values.rider_names, name_index);
        text.insert(text.end(), name.begin(), name.end());
        text.push_back(end_of_text);
        if (at < stream.size() && stream[at] == capitals) {
            ++at;
            in_capitals(text);
        }
    } else {
        const auto name = league_name(values.league_names, name_index & 0xffU);
        text.insert(text.end(), name.begin(), name.end());
        text.push_back(end_of_text);
    }
    print_text(map, cursor, text, character_table, variables);
}

} // namespace

void print_text(TextMap& map, TextCursor& cursor, std::span<const std::uint8_t> stream,
                std::span<const std::uint8_t> character_table, const TextVariables* variables) {
    if (character_table.size() != 256)
        throw std::invalid_argument("text printer table is not 256 bytes");
    std::size_t at = 0;
    const auto next = [&] { return read_stream_byte(stream, at); };
    for (;;) {
        const auto byte = next();
        if (byte < first_control) {
            const auto entry = table_entry(character_table, byte);
            if (entry & small_glyph)
                print_small(map, cursor, entry);
            else
                print_big(map, cursor, entry);
            continue;
        }
        switch (byte) {
        case end_of_text: return;
        case nothing: break;
        case position: {
            const auto column = next();
            const auto row = next();
            cursor.position = row * 32U + column;
            break;
        }
        case centre: {
            const auto row = next();
            centre_on(cursor, stream.subspan(at), row, character_table);
            break;
        }
        case row_code: {
            const auto row = next();
            const auto remaining = stream.subspan(at);
            std::size_t visible = 0;
            for (std::size_t k = 0; k < remaining.size() && remaining[k] != end_of_text; ++k)
                if (remaining[k] != '_') visible = k + 1;
            centre_on(cursor, remaining.first(visible), row, character_table);
            break;
        }
        case attribute: cursor.attribute = static_cast<std::uint16_t>(next() << 10U); break;
        case number:       // $80:C456: five digits
        case two_digits: { // $80:C474: their last two, the tens a blank below 10
            const auto& v = require(variables);
            const auto address = static_cast<std::uint16_t>(next() | (next() << 8U));
            const auto text = five_digit_text(v.word(address));
            constexpr std::size_t tens = 3;
            print_text(map, cursor, std::span(text).subspan(byte == number ? 0 : tens),
                       character_table, variables);
            break;
        }
        case track_name:   // $80:C628
        case rider_name: { // $80:C5D3
            print_name_operand(map, cursor, stream, at, character_table, variables, byte);
            break;
        }
        case league_slot_name:
        case league_current_name: { // $80:C675, $80:C67D, $80:9B79
            print_name_operand(map, cursor, stream, at, character_table, variables, byte);
            break;
        }
        case race_time: { // $80:C6BB
            const auto& v = require(variables);
            const auto address = static_cast<std::uint16_t>(next() | (next() << 8U));
            auto text = race_time_text(v.word(address), v.time_words);
            text.push_back(end_of_text);
            print_text(map, cursor, text, character_table, variables);
            break;
        }
        case place_object: { // $80:C6D5
            const auto& v = require(variables);
            v.place_object(next() & 7U, cursor.position);
            break;
        }
        default: throw std::invalid_argument("text printer control code is not recovered");
        }
    }
}

} // namespace unirally
