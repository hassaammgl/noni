#pragma once

// Cursor positions are line + BYTE offset into the UTF-8 line string.
// Display columns and codepoint indices are derived via TextMetrics.
struct Cursor
{
    int line;
    int column; // byte offset within the line
};
