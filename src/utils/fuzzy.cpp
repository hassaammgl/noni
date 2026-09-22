#include "fuzzy_detail.hpp"

using namespace fuzzy_detail;

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

