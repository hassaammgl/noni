#include <utils/fuzzy.hpp>
#include <utils/async.hpp>
#include <utils/logger.hpp>

#include <algorithm>
#include <cctype>
#include <format>

namespace
{
    char lower(char c)
    {
        return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    bool is_skipped_name(const std::string &name)
    {
        static const char *skip[] = {
            ".git", ".hg", ".svn", ".jj",
            "node_modules", "target", "build", "dist", "out",
            ".cache", ".idea", ".vscode", ".cursor",
            "__pycache__", ".next", ".nuxt", "vendor",
            "CMakeFiles", ".tox", ".venv", "venv",
        };
        for (const char *s : skip)
        {
            if (name == s)
                return true;
        }
        return false;
    }
}

namespace Fuzzy
{
    int score(std::string_view text, std::string_view query)
    {
        if (query.empty())
            return 1;

        int score_value = 0;
        std::size_t ti = 0;
        int consecutive = 0;
        bool first_hit = true;

        for (std::size_t qi = 0; qi < query.size(); ++qi)
        {
            const char qc = lower(query[qi]);
            bool found = false;

            while (ti < text.size())
            {
                const char tc = lower(text[ti]);
                if (tc == qc)
                {
                    int bonus = 1;
                    if (ti == 0 || text[ti - 1] == '/' || text[ti - 1] == '_' ||
                        text[ti - 1] == '-' || text[ti - 1] == '.')
                        bonus += 8;
                    if (consecutive > 0)
                        bonus += 4 * consecutive;
                    if (first_hit && ti < 4)
                        bonus += 3;

                    score_value += bonus;
                    ++consecutive;
                    ++ti;
                    found = true;
                    first_hit = false;
                    break;
                }
                consecutive = 0;
                ++ti;
            }

            if (!found)
                return -1;
        }

        score_value += std::max(0, 40 - static_cast<int>(text.size()) / 2);
        return score_value;
    }

    std::vector<FuzzyMatch> filter(
        const std::vector<fs::path> &files,
        const fs::path &root,
        std::string_view query,
        std::size_t limit)
    {
        std::vector<FuzzyMatch> out;
        out.reserve(std::min(files.size(), limit * 2));

        for (const auto &path : files)
        {
            std::string relative = path.string();
            if (!root.empty())
            {
                std::error_code ec;
                auto rel = fs::relative(path, root, ec);
                if (!ec)
                    relative = rel.string();
            }

            const std::string name = path.filename().string();
            const int name_score = score(name, query);
            const int path_score = score(relative, query);
            const int best = std::max(name_score, path_score);
            if (best < 0)
                continue;

            FuzzyMatch m;
            m.path = path;
            m.display = relative;
            m.score = best + (name_score > 0 ? 5 : 0);
            out.push_back(std::move(m));
        }

        std::sort(out.begin(), out.end(), [](const FuzzyMatch &a, const FuzzyMatch &b) {
            if (a.score != b.score)
                return a.score > b.score;
            return a.display.size() < b.display.size();
        });

        if (out.size() > limit)
            out.resize(limit);
        return out;
    }
}

bool FileIndex::should_skip_dir(const fs::path &name)
{
    return is_skipped_name(name.string());
}

std::vector<fs::path> FileIndex::scan_files(const fs::path &root)
{
    std::vector<fs::path> found;
    if (root.empty() || !fs::exists(root))
        return found;

    std::error_code ec;
    const auto options = fs::directory_options::skip_permission_denied;
    for (auto it = fs::recursive_directory_iterator(root, options, ec);
         it != fs::recursive_directory_iterator();
         it.increment(ec))
    {
        if (ec)
        {
            ec.clear();
            continue;
        }

        const auto &entry = *it;
        if (entry.is_directory(ec))
        {
            if (should_skip_dir(entry.path().filename()))
                it.disable_recursion_pending();
            continue;
        }

        if (!entry.is_regular_file(ec))
            continue;

        found.push_back(entry.path());
    }

    std::sort(found.begin(), found.end());
    return found;
}

void FileIndex::set_root(const fs::path &root)
{
    std::lock_guard lock(mu);
    if (root_path == root)
        return;
    root_path = root;
    entries.clear();
    ver.fetch_add(1, std::memory_order_relaxed);
}

void FileIndex::rebuild()
{
    fs::path root;
    {
        std::lock_guard lock(mu);
        root = root_path;
    }

    auto found = scan_files(root);

    {
        std::lock_guard lock(mu);
        if (root_path != root)
            return;
        entries = std::move(found);
    }
    indexing.store(false, std::memory_order_release);
    ver.fetch_add(1, std::memory_order_relaxed);
    Logger::info(std::format("FileIndex: {} files under {}", file_count(), root.string()));
}

void FileIndex::rebuild_async()
{
    fs::path root;
    {
        std::lock_guard lock(mu);
        root = root_path;
    }
    if (root.empty())
        return;

    const std::uint64_t token = Background::instance().next_token();
    job_token.store(token, std::memory_order_relaxed);
    indexing.store(true, std::memory_order_release);

    Background::instance().post([this, root, token]() {
        if (job_token.load(std::memory_order_relaxed) != token)
            return;

        auto found = scan_files(root);

        if (job_token.load(std::memory_order_relaxed) != token)
            return;

        {
            std::lock_guard lock(mu);
            if (root_path != root)
                return;
            entries = std::move(found);
        }
        indexing.store(false, std::memory_order_release);
        ver.fetch_add(1, std::memory_order_relaxed);
        Logger::info(std::format(
            "FileIndex(async): {} files under {}",
            file_count(),
            root.string()));
    });
}

bool FileIndex::is_indexing() const
{
    return indexing.load(std::memory_order_acquire);
}

std::uint64_t FileIndex::version() const
{
    return ver.load(std::memory_order_acquire);
}

fs::path FileIndex::root() const
{
    std::lock_guard lock(mu);
    return root_path;
}

std::vector<fs::path> FileIndex::files() const
{
    std::lock_guard lock(mu);
    return entries;
}

std::size_t FileIndex::file_count() const
{
    std::lock_guard lock(mu);
    return entries.size();
}
