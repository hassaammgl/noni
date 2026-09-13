#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>

// Text metrics: byte offset ↔ codepoint ↔ display column.
// Buffer / Cursor.column remain BYTE offsets into UTF-8 std::string.
namespace TextMetrics
{
    inline constexpr int kDefaultTabWidth = 4;

    inline bool is_cont(unsigned char c) { return (c & 0xC0) == 0x80; }

    // Length of UTF-8 sequence starting at lead (1–4). Invalid → 1.
    inline std::size_t seq_len(unsigned char lead)
    {
        if (lead < 0x80)
            return 1;
        if ((lead & 0xE0) == 0xC0)
            return 2;
        if ((lead & 0xF0) == 0xE0)
            return 3;
        if ((lead & 0xF8) == 0xF0)
            return 4;
        return 1;
    }

    // Decode one codepoint at byte offset. Returns {codepoint, byte_length}.
    std::pair<char32_t, std::size_t> decode(std::string_view line, std::size_t byte);

    // Snap byte index onto a codepoint boundary (start of sequence).
    std::size_t snap_byte(std::string_view line, std::size_t byte);

    // Next / previous codepoint start (clamped to [0, line.size()]).
    std::size_t next_cp(std::string_view line, std::size_t byte);
    std::size_t prev_cp(std::string_view line, std::size_t byte);

    // Terminal display width of one codepoint (0, 1, or 2). Tab → -1 (caller expands).
    int codepoint_width(char32_t cp);

    // Display width of a tab at the given display column.
    int tab_width_at(int display_col, int tab_width = kDefaultTabWidth);

    // Byte offset → display column (0-based).
    int byte_to_display(std::string_view line, std::size_t byte, int tab_width = kDefaultTabWidth);

    // Display column → byte offset (snapped to codepoint that contains/starts at col).
    std::size_t display_to_byte(std::string_view line, int display_col, int tab_width = kDefaultTabWidth);

    // Total display width of a line.
    int line_display_width(std::string_view line, int tab_width = kDefaultTabWidth);

    // Count Unicode codepoints in [0, byte).
    int byte_to_codepoint_index(std::string_view line, std::size_t byte);

    // Codepoint index → byte offset.
    std::size_t codepoint_index_to_byte(std::string_view line, int index);
}
