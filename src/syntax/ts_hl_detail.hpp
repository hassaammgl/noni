#pragma once
#include "ts_queries.hpp"
#include <syntax/syntax.hpp>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace ts_hl_detail
{
    using LangFn = const struct TSLanguage *(*)();

    struct LangSpec
    {
        const char *so_name;
        const char *symbol;
        const char *query;
    };

    const LangSpec *spec_for(Language lang);
    TokenKind capture_to_kind(std::string_view name);
    void add_span(
        std::vector<std::vector<SyntaxToken>> &lines,
        uint32_t start_row,
        uint32_t start_col,
        uint32_t end_row,
        uint32_t end_col,
        TokenKind kind);
    std::string join_source(const std::vector<std::string> &lines);
}
