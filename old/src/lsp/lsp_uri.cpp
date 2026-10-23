#include "lsp_internal.hpp"

Cursor LspService::lsp_pos_to_cursor(const std::vector<std::string> &lines, int line, int character)
{
    if (lines.empty())
        return {.line = 0, .column = 0};
    line = std::clamp(line, 0, static_cast<int>(lines.size()) - 1);
    const auto &row = lines[static_cast<std::size_t>(line)];
    const int byte = static_cast<int>(TextMetrics::utf16_to_byte(row, character));
    return {.line = line, .column = byte};
}

std::string LspService::path_to_uri(const fs::path &path)
{
    std::error_code ec;
    fs::path abs = fs::weakly_canonical(path, ec);
    if (ec)
        abs = fs::absolute(path, ec);
    if (ec)
        abs = path;
    std::string s = abs.generic_string();
    if (!s.empty() && s[0] != '/')
        return "file:///" + s;
    return "file://" + s;
}

fs::path LspService::uri_to_path(const std::string &uri)
{
    if (uri.empty())
        return {};
    std::string s = uri;
    if (s.rfind("file://", 0) == 0)
        s = s.substr(7);
    // file:///path → /path; file://localhost/path → skip host
    if (s.rfind("localhost", 0) == 0)
        s = s.substr(9);
    // Percent-decode minimal (%20 etc.)
    std::string out;
    out.reserve(s.size());
    for (std::size_t i = 0; i < s.size(); ++i)
    {
        if (s[i] == '%' && i + 2 < s.size())
        {
            auto hex = [](char c) -> int {
                if (c >= '0' && c <= '9')
                    return c - '0';
                if (c >= 'a' && c <= 'f')
                    return c - 'a' + 10;
                if (c >= 'A' && c <= 'F')
                    return c - 'A' + 10;
                return -1;
            };
            const int hi = hex(s[i + 1]);
            const int lo = hex(s[i + 2]);
            if (hi >= 0 && lo >= 0)
            {
                out.push_back(static_cast<char>((hi << 4) | lo));
                i += 2;
                continue;
            }
        }
        out.push_back(s[i]);
    }
    return fs::path(out);
}

std::string LspService::join_lines(const std::vector<std::string> &lines)
{
    std::string out;
    for (std::size_t i = 0; i < lines.size(); ++i)
    {
        out += lines[i];
        if (i + 1 < lines.size())
            out.push_back('\n');
    }
    return out;
}

std::string LspService::language_id_for(Language lang)
{
    return Syntax::language_definition(lang).name;
}
