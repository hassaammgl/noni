#include <utils/text_search.hpp>
#include <utils/async.hpp>
#include <utils/logger.hpp>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <format>
#include <regex>

void TextSearch::search_async(const std::string &query, TextSearchOptions opts)
{
    fs::path root;
    {
        std::lock_guard lock(mu);
        root = root_path;
        matches.clear();
        status_text = query.empty() ? "Type to search" : "Searching…";
    }
    ver.fetch_add(1, std::memory_order_relaxed);

    if (query.empty() || root.empty())
    {
        searching.store(false, std::memory_order_release);
        return;
    }

    const std::uint64_t token = Background::instance().next_token();
    job_token.store(token, std::memory_order_relaxed);
    searching.store(true, std::memory_order_release);

    Background::instance().post([this, root, query, opts, token]()
                                {
        if (job_token.load(std::memory_order_relaxed) != token)
            return;

        auto found = scan(root, query, opts, job_token, token);

        if (job_token.load(std::memory_order_relaxed) != token)
            return;

        {
            std::lock_guard lock(mu);
            matches = std::move(found);
            std::size_t files = 0;
            fs::path prev;
            for (const auto &m : matches)
            {
                if (m.path != prev)
                {
                    ++files;
                    prev = m.path;
                }
            }
            status_text = std::format("{} results in {} files", matches.size(), files);
        }
        searching.store(false, std::memory_order_release);
        ver.fetch_add(1, std::memory_order_relaxed);
        Logger::info(std::format("TextSearch: {} hits for '{}'", results().size(), query)); });
}
