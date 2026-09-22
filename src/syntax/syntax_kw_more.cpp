#include "syntax_lex.hpp"

namespace syntax_lex
{
    const std::unordered_set<std::string> &js_keywords()
    {
        static const std::unordered_set<std::string> k = {
            "await", "break", "case", "catch", "class", "const", "continue", "debugger",
            "default", "delete", "do", "else", "enum", "export", "extends", "false",
            "finally", "for", "function", "if", "implements", "import", "in",
            "instanceof", "interface", "let", "new", "null", "package", "private",
            "protected", "public", "return", "static", "super", "switch", "this",
            "throw", "true", "try", "typeof", "var", "void", "while", "with", "yield",
            "async", "of", "from", "as", "type", "namespace",
        };
        return k;
    }

    const std::unordered_set<std::string> &go_keywords()
    {
        static const std::unordered_set<std::string> k = {
            "break", "case", "chan", "const", "continue", "default", "defer", "else",
            "fallthrough", "for", "func", "go", "goto", "if", "import", "interface",
            "map", "package", "range", "return", "select", "struct", "switch", "type",
            "var", "true", "false", "nil",
        };
        return k;
    }

    const std::unordered_set<std::string> &java_keywords()
    {
        static const std::unordered_set<std::string> k = {
            "abstract", "assert", "boolean", "break", "byte", "case", "catch", "char",
            "class", "const", "continue", "default", "do", "double", "else", "enum",
            "extends", "final", "finally", "float", "for", "goto", "if", "implements",
            "import", "instanceof", "int", "interface", "long", "native", "new",
            "package", "private", "protected", "public", "return", "short", "static",
            "strictfp", "super", "switch", "synchronized", "this", "throw", "throws",
            "transient", "try", "void", "volatile", "while", "true", "false", "null",
            "var", "record", "sealed", "permits", "yield",
        };
        return k;
    }

    const std::unordered_set<std::string> &csharp_keywords()
    {
        static const std::unordered_set<std::string> k = {
            "abstract", "as", "base", "bool", "break", "byte", "case", "catch", "char",
            "checked", "class", "const", "continue", "decimal", "default", "delegate",
            "do", "double", "else", "enum", "event", "explicit", "extern", "false",
            "finally", "fixed", "float", "for", "foreach", "goto", "if", "implicit",
            "in", "int", "interface", "internal", "is", "lock", "long", "namespace",
            "new", "null", "object", "operator", "out", "override", "params", "private",
            "protected", "public", "readonly", "ref", "return", "sbyte", "sealed",
            "short", "sizeof", "stackalloc", "static", "string", "struct", "switch",
            "this", "throw", "true", "try", "typeof", "uint", "ulong", "unchecked",
            "unsafe", "ushort", "using", "virtual", "void", "volatile", "while",
            "async", "await", "var", "record", "init", "required",
        };
        return k;
    }

    const std::unordered_set<std::string> &lua_keywords()
    {
        static const std::unordered_set<std::string> k = {
            "and", "break", "do", "else", "elseif", "end", "false", "for", "function",
            "goto", "if", "in", "local", "nil", "not", "or", "repeat", "return",
            "then", "true", "until", "while",
        };
        return k;
    }

    const std::unordered_set<std::string> &shell_keywords()
    {
        static const std::unordered_set<std::string> k = {
            "if", "then", "else", "elif", "fi", "case", "esac", "for", "select",
            "while", "until", "do", "done", "in", "function", "time", "coproc",
            "return", "exit", "break", "continue", "export", "local", "readonly",
            "declare", "typeset", "unset", "shift", "source",
        };
        return k;
    }
}
