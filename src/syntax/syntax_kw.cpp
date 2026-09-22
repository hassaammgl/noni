#include "syntax_lex.hpp"

namespace syntax_lex
{
    bool is_control(Language lang, const std::string &word)
    {
        static const std::unordered_set<std::string> ctrl = {
            "if", "else", "elif", "elseif", "for", "while", "do", "switch", "case",
            "default", "break", "continue", "return", "goto", "try", "catch", "throw",
            "finally", "except", "raise", "yield", "await", "match", "loop", "when",
            "fi", "done", "esac", "then", "until", "select",
        };
        (void)lang;
        return ctrl.count(word) > 0;
    }

    const std::unordered_set<std::string> *keywords_for(Language lang)
    {
        switch (lang)
        {
        case Language::Cpp:
            return &cpp_keywords();
        case Language::C:
            return &c_keywords();
        case Language::Python:
            return &python_keywords();
        case Language::Rust:
            return &rust_keywords();
        case Language::JavaScript:
        case Language::TypeScript:
            return &js_keywords();
        case Language::Go:
            return &go_keywords();
        case Language::Java:
            return &java_keywords();
        case Language::CSharp:
            return &csharp_keywords();
        case Language::Lua:
            return &lua_keywords();
        case Language::Shell:
            return &shell_keywords();
        default:
            return nullptr;
        }
    }

    bool uses_c_comments(Language lang)
    {
        switch (lang)
        {
        case Language::Cpp:
        case Language::C:
        case Language::Rust:
        case Language::Go:
        case Language::JavaScript:
        case Language::TypeScript:
        case Language::Java:
        case Language::CSharp:
        case Language::JSON:
            return true;
        default:
            return false;
        }
    }

    bool uses_hash_comments(Language lang)
    {
        return lang == Language::Python || lang == Language::Shell;
    }

    bool uses_lua_comments(Language lang)
    {
        return lang == Language::Lua;
    }
}
