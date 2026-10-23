#include <utils/text_search.hpp>
#include <utils/async.hpp>
#include <utils/logger.hpp>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <format>
#include <regex>

namespace
{
    char lower(char c)
    {
        return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    bool is_word_char(char c)
    {
        return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
    }

    bool whole_word_at(const std::string &line, std::size_t pos, std::size_t len)
    {
        if (len == 0)
            return false;
        const bool left_ok = (pos == 0) || !is_word_char(line[pos - 1]);
        const bool right_ok = (pos + len >= line.size()) || !is_word_char(line[pos + len]);
        return left_ok && right_ok;
    }
}

std::vector<TextMatch> TextSearch::scan(
    const fs::path &root,
    const std::string &query,
    TextSearchOptions opts,
    const std::atomic<std::uint64_t> &token,
    std::uint64_t my_token)
{
    std::vector<TextMatch> out;
    if (root.empty() || query.empty() || !fs::exists(root))
        return out;

    std::regex re;
    if (opts.use_regex)
    {
        try
        {
            auto flags = std::regex::ECMAScript;
            if (!opts.match_case)
                flags = static_cast<std::regex_constants::syntax_option_type>(
                    flags | std::regex::icase);
            re = std::regex(query, flags);
        }
        catch (...)
        {
            return out;
        }
    }

    std::error_code ec;
    const auto options = fs::directory_options::skip_permission_denied;
    for (auto it = fs::recursive_directory_iterator(root, options, ec);
         it != fs::recursive_directory_iterator();
         it.increment(ec))
    {
        if (token.load(std::memory_order_relaxed) != my_token)
            return out;
        if (ec)
        {
            ec.clear();
            continue;
        }

        const auto &entry = *it;
        if (entry.is_directory(ec))
        {
            if (should_skip_dir(entry.path().filename().string()))
                it.disable_recursion_pending();
            continue;
        }
        if (!entry.is_regular_file(ec))
            continue;

        std::ifstream file(entry.path(), std::ios::in | std::ios::binary);
        if (!file)
            continue;

        std::string content;
        content.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
        if (looks_binary(content))
            continue;

        int line_no = 0;
        std::size_t start = 0;
        while (start <= content.size())
        {
            if (token.load(std::memory_order_relaxed) != my_token)
                return out;

            std::size_t end = content.find('\n', start);
            if (end == std::string::npos)
                end = content.size();
            std::string line = content.substr(start, end - start);
            if (!line.empty() && line.back() == '\r')
                line.pop_back();

            auto add_match = [&](std::size_t col)
            {
                TextMatch m;
                m.path = entry.path();
                m.line = line_no;
                m.column = static_cast<int>(col);
                m.preview = line;
                if (m.preview.size() > 120)
                    m.preview = m.preview.substr(0, 117) + "...";
                out.push_back(std::move(m));
            };

            if (opts.use_regex)
            {
                try
                {
                    for (std::sregex_iterator ri(line.begin(), line.end(), re), rend;
                         ri != rend; ++ri)
                    {
                        if (opts.whole_word &&
                            !whole_word_at(line, static_cast<std::size_t>(ri->position()), ri->length()))
                            continue;
                        add_match(static_cast<std::size_t>(ri->position()));
                        if (out.size() >= 2000)
                            return out;
                    }
                }
                catch (...)
                {
                }
            }
            else
            {
                const std::string hay = opts.match_case ? line : [&]()
                {
                    std::string s = line;
                    for (char &c : s)
                        c = lower(c);
                    return s;
                }();
                const std::string needle = opts.match_case ? query : [&]()
                {
                    std::string s = query;
                    for (char &c : s)
                        c = lower(c);
                    return s;
                }();

                std::size_t pos = 0;
                while ((pos = hay.find(needle, pos)) != std::string::npos)
                {
                    if (!opts.whole_word || whole_word_at(line, pos, needle.size()))
                    {
                        add_match(pos);
                        if (out.size() >= 2000)
                            return out;
                    }
                    pos += std::max<std::size_t>(1, needle.size());
                }
            }

            ++line_no;
            if (end == content.size())
                break;
            start = end + 1;
        }
    }

    return out;
}

