#include "ui_impl.hpp"

void UI::apply_replace_all()
{
    const std::string q = search_panel.get_query();
    const std::string r = search_panel.get_replace();
    if (q.empty())
    {
        Messages::error("Nothing to replace");
        return;
    }

    auto matches = search_panel.results();
    if (matches.empty())
    {
        Messages::error("No matches");
        return;
    }

    const auto opts = search_panel.get_options();

    auto find_open_buffer = [&](const fs::path &path) -> Buffer * {
        return core.buffers().find_buffer_by_path(path);
    };

    // Group by path.
    std::map<fs::path, std::vector<TextMatch>> by_path;
    for (const auto &m : matches)
        by_path[m.path].push_back(m);

    int files_touched = 0;
    int replacements = 0;

    for (auto &[path, file_matches] : by_path)
    {
        std::sort(file_matches.begin(), file_matches.end(), [](const TextMatch &a, const TextMatch &b) {
            if (a.line != b.line)
                return a.line > b.line;
            return a.column > b.column;
        });

        if (Buffer *buf = find_open_buffer(path))
        {
            BufferSearchQuery bq;
            bq.pattern = q;
            bq.options = opts;
            auto buffer_matches = BufferSearch::find_all(buf->lines(), bq);
            if (buffer_matches.empty())
                continue;

            std::sort(buffer_matches.begin(), buffer_matches.end(),
                      [](const BufferSearchMatch &a, const BufferSearchMatch &b) {
                          if (a.start.line != b.start.line)
                              return a.start.line > b.start.line;
                          return a.start.column > b.start.column;
                      });

            Cursor cur{.line = 0, .column = 0};
            if (core.active_buffer() == buf && core.active_window())
                cur = core.active_window()->cursor();

            buf->begin_edit(cur.line, cur.column);
            for (const auto &m : buffer_matches)
            {
                buf->delete_range(m.start.line, m.start.column, m.end.line, m.end.column);
                (void)buf->insert_text(m.start.line, m.start.column, r);
                ++replacements;
            }
            buf->end_edit(cur.line, cur.column);
            ++files_touched;
            continue;
        }

        // File not open: atomic FS write (no Buffer history).
        std::ifstream in(path);
        if (!in)
            continue;
        std::vector<std::string> lines;
        std::string line;
        while (std::getline(in, line))
        {
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            lines.push_back(line);
        }

        bool dirty = false;
        if (opts.use_regex)
        {
            try
            {
                auto flags = std::regex::ECMAScript;
                if (!opts.match_case)
                    flags |= std::regex::icase;
                std::regex re(q, flags);
                for (auto &row : lines)
                {
                    const std::string before = row;
                    row = std::regex_replace(row, re, r);
                    if (row != before)
                    {
                        ++replacements;
                        dirty = true;
                    }
                }
            }
            catch (...)
            {
                continue;
            }
        }
        else
        {
            for (const auto &m : file_matches)
            {
                if (m.line < 0 || m.line >= static_cast<int>(lines.size()))
                    continue;
                std::string &row = lines[static_cast<std::size_t>(m.line)];
                if (m.column < 0 ||
                    m.column + static_cast<int>(q.size()) > static_cast<int>(row.size()))
                    continue;
                std::string span = row.substr(static_cast<std::size_t>(m.column), q.size());
                const bool ok = opts.match_case
                                    ? (span == q)
                                    : (StrUtils::to_lower(span) == StrUtils::to_lower(q));
                if (!ok)
                    continue;
                row.replace(static_cast<std::size_t>(m.column), q.size(), r);
                ++replacements;
                dirty = true;
            }
        }

        if (dirty)
        {
            FS fs;
            if (fs.write_file(path, lines))
                ++files_touched;
        }
    }

    sync_active_tab();
    Messages::info(std::format("Replaced {} matches across {} files", replacements, files_touched));
}
