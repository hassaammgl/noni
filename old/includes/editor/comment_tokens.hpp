#pragma once

#include <syntax/syntax.hpp>

#include <string_view>

namespace CommentTokens
{
    // Empty = language does not support line-comment toggle (skip).
    [[nodiscard]] constexpr std::string_view line_prefix(Language lang)
    {
        switch (lang)
        {
        case Language::Cpp:
        case Language::C:
        case Language::Java:
        case Language::JavaScript:
        case Language::TypeScript:
        case Language::Go:
        case Language::Rust:
        case Language::CSharp:
            return "//";
        case Language::Python:
        case Language::Shell:
            return "#";
        case Language::Lua:
            return "--";
        case Language::Markdown:
        case Language::JSON:
        case Language::Plain:
        default:
            return {};
        }
    }
}
