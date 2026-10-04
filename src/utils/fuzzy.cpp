#include "fuzzy_detail.hpp"

using namespace fuzzy_detail;

namespace
{
    std::string_view filename_view(std::string_view path)
    {
        const auto slash = path.find_last_of('/');
        if (slash == std::string_view::npos)
            return path;
        return path.substr(slash + 1);
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

        const std::string root_s = root.empty() ? std::string{} : root.string();

        for (const auto &path : files)
        {
            // Score against native path bytes — avoid filename()/relative allocs until hit.
            const std::string &native = path.native();
            const std::string_view name = filename_view(native);
            const int name_score = score(name, query);
            const int path_score = score(native, query);
            const int best = std::max(name_score, path_score);
            if (best < 0)
                continue;

            FuzzyMatch m;
            m.path = path;
            if (!root_s.empty() && native.size() > root_s.size() &&
                native.compare(0, root_s.size(), root_s) == 0 &&
                (native.size() == root_s.size() || native[root_s.size()] == '/'))
            {
                const std::size_t off =
                    root_s.size() + (native.size() > root_s.size() ? 1 : 0);
                m.display.assign(native.data() + off, native.size() - off);
            }
            else
                m.display = native;
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
