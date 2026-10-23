#include <lsp/lsp_edits.hpp>
#include <lsp/lsp_service.hpp>
#include <utils/logger.hpp>
#include <utils/text_metrics.hpp>

#include <algorithm>
#include <filesystem>
#include <format>

namespace
{
    void convert_edits_utf16_to_bytes(Buffer &buf, std::vector<LspTextEdit> &edits)
    {
        const auto &lines = buf.lines();
        for (auto &e : edits)
        {
            // If utf16_pending was set, start/end.column still hold UTF-16 units.
            e.start.column = static_cast<int>(TextMetrics::utf16_to_byte(
                (e.start.line >= 0 && e.start.line < static_cast<int>(lines.size()))
                    ? lines[static_cast<std::size_t>(e.start.line)]
                    : std::string_view{},
                e.start.column));
            e.end.column = static_cast<int>(TextMetrics::utf16_to_byte(
                (e.end.line >= 0 && e.end.line < static_cast<int>(lines.size()))
                    ? lines[static_cast<std::size_t>(e.end.line)]
                    : std::string_view{},
                e.end.column));
        }
    }
}

int LspEdits::apply_workspace_edit(
    BufferManager &buffers,
    LspWorkspaceEdit edit,
    const std::function<void(Buffer &)> &on_opened)
{
    int touched = 0;
    for (auto &[uri, edits] : edit.changes)
    {
        if (edits.empty())
            continue;

        const fs::path path = LspService::uri_to_path(uri);
        if (path.empty())
        {
            Logger::warning(std::format("WorkspaceEdit: bad uri {}", uri));
            continue;
        }

        Buffer *buf = buffers.find_buffer_by_path(path);
        if (!buf)
        {
            buffers.open_file(path);
            buf = buffers.find_buffer_by_path(path);
            if (buf && on_opened)
                on_opened(*buf);
        }
        if (!buf)
        {
            Logger::warning(std::format("WorkspaceEdit: could not open {}", path.string()));
            continue;
        }

        if (edit.utf16_pending)
            convert_edits_utf16_to_bytes(*buf, edits);

        std::sort(edits.begin(), edits.end(), [](const LspTextEdit &a, const LspTextEdit &b) {
            if (a.start.line != b.start.line)
                return a.start.line > b.start.line;
            return a.start.column > b.start.column;
        });

        buf->begin_edit(0, 0);
        for (const auto &e : edits)
        {
            Cursor s = e.start;
            Cursor en = e.end;
            if (en.line < s.line || (en.line == s.line && en.column < s.column))
                std::swap(s, en);
            if (s.line != en.line || s.column != en.column)
                buf->delete_range(s.line, s.column, en.line, en.column);
            if (!e.new_text.empty())
                buf->insert_text(s.line, s.column, e.new_text);
        }
        buf->end_edit(0, 0);
        ++touched;
    }
    return touched;
}
