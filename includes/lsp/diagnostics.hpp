#pragma once

#include <utils/cursor.hpp>

#include <cstdint>
#include <string>
#include <vector>

enum class DiagnosticSeverity : std::uint8_t
{
    Error = 1,
    Warning = 2,
    Information = 3,
    Hint = 4,
};

// Logical editor ranges: Cursor.column is UTF-8 byte offset (not UTF-16, not display).
struct Diagnostic
{
    DiagnosticSeverity severity = DiagnosticSeverity::Error;
    std::string message;
    std::string source;
    std::string code;
    Cursor start{.line = 0, .column = 0};
    Cursor end{.line = 0, .column = 0};
};

struct DiagnosticSnapshot
{
    std::vector<Diagnostic> items;
    int lsp_version = -1; // -1 = server omitted version
    std::uint64_t generation = 0;
};
