#include "text_printer.hpp"

#include <stdexcept>

namespace unirally {

namespace {

// Control codes, dispatched through `$80:C3DC` by 0xFF - code (`$80:C3C6-C3D9`).
constexpr std::uint8_t end_of_text = 0xff, position = 0xfe, centre = 0xfc, nothing = 0xfb,
                       attribute = 0xf9, skip_pair = 0xef, first_control = 0xee;
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
// and their `ADC #$9F` adds one more.
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
        if (character == skip_pair) {
            ++at;
            continue;
        }
        budget -= (table[character] & small_glyph) ? 1U : 2U;
    }
    cursor.position = row * 32U + ((budget & 0xffU) >> 1U);
}

} // namespace

void print_text(TextMap& map, TextCursor& cursor, std::span<const std::uint8_t> stream,
                std::span<const std::uint8_t> character_table) {
    if (character_table.size() != 256)
        throw std::invalid_argument("text printer table is not 256 bytes");
    std::size_t at = 0;
    const auto next = [&] {
        if (at >= stream.size()) throw std::invalid_argument("text stream ends without 0xFF");
        return stream[at++];
    };
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
        case attribute: cursor.attribute = static_cast<std::uint16_t>(next() << 10U); break;
        default: throw std::invalid_argument("text printer control code is not recovered");
        }
    }
}

} // namespace unirally
