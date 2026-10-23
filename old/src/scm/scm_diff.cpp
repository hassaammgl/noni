#include <scm/scm_diff.hpp>

#include <cctype>
#include <cstdlib>
#include <sstream>

namespace
{
    bool parse_hunk_header(const std::string &line, ScmHunk &h)
    {
        // @@ -l,s +l,s @@ or @@ -l +l @@
        if (line.size() < 5 || line[0] != '@' || line[1] != '@')
            return false;
        const auto minus = line.find('-');
        const auto plus = line.find('+');
        if (minus == std::string::npos || plus == std::string::npos || plus < minus)
            return false;

        auto read_pair = [&](std::size_t start, int &out_start, int &out_count) -> std::size_t {
            while (start < line.size() && !std::isdigit(static_cast<unsigned char>(line[start])) &&
                   line[start] != '-')
                ++start;
            char *end = nullptr;
            out_start = static_cast<int>(std::strtol(line.c_str() + start, &end, 10));
            out_count = 1;
            if (end && *end == ',')
                out_count = static_cast<int>(std::strtol(end + 1, &end, 10));
            return end ? static_cast<std::size_t>(end - line.c_str()) : line.size();
        };

        read_pair(minus + 1, h.old_start, h.old_count);
        read_pair(plus + 1, h.new_start, h.new_count);
        if (h.old_start < 0)
            h.old_start = 0;
        if (h.new_start < 0)
            h.new_start = 0;
        return true;
    }
}

namespace ScmDiff
{
    ScmFileDiff untracked_all_added(fs::path path, int new_line_count)
    {
        ScmFileDiff d;
        d.path = std::move(path);
        d.is_untracked = true;
        d.ok = true;
        for (int i = 0; i < new_line_count; ++i)
            d.lines[i] = ScmLineChange::Added;
        return d;
    }

    ScmFileDiff parse_unified(const std::string &diff_text, fs::path path, int new_line_count)
    {
        ScmFileDiff d;
        d.path = std::move(path);
        if (diff_text.empty())
        {
            d.ok = true;
            return d;
        }

        std::istringstream in(diff_text);
        std::string line;
        while (std::getline(in, line))
        {
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            ScmHunk hunk;
            if (!parse_hunk_header(line, hunk))
                continue;
            d.hunks.push_back(hunk);

            // Map hunk onto new-file (buffer) lines using -U0 semantics.
            if (hunk.old_count == 0 && hunk.new_count > 0)
            {
                // Pure add
                for (int i = 0; i < hunk.new_count; ++i)
                {
                    const int idx = hunk.new_start - 1 + i;
                    if (idx >= 0 && (new_line_count <= 0 || idx < new_line_count))
                        d.lines[idx] = ScmLineChange::Added;
                }
            }
            else if (hunk.new_count == 0 && hunk.old_count > 0)
            {
                // Pure delete — mark the line after which content was removed.
                int idx = hunk.new_start - 1; // in new file, insertion point
                if (idx < 0)
                    idx = 0;
                if (new_line_count > 0 && idx >= new_line_count)
                    idx = new_line_count - 1;
                if (idx >= 0)
                {
                    auto &slot = d.lines[idx];
                    if (slot == ScmLineChange::None)
                        slot = ScmLineChange::Deleted;
                }
            }
            else if (hunk.new_count > 0)
            {
                // Modified (replace)
                for (int i = 0; i < hunk.new_count; ++i)
                {
                    const int idx = hunk.new_start - 1 + i;
                    if (idx >= 0 && (new_line_count <= 0 || idx < new_line_count))
                    {
                        auto &slot = d.lines[idx];
                        if (slot != ScmLineChange::Added)
                            slot = ScmLineChange::Modified;
                    }
                }
            }
        }

        d.ok = true;
        return d;
    }
}
