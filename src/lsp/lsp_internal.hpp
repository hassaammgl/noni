#pragma once

#include <lsp/lsp_service.hpp>
#include <lsp/lsp_installer.hpp>
#include <lsp/completion.hpp>
#include <lsp/diagnostics.hpp>
#include <lsp/lsp_models.hpp>
#include <utils/logger.hpp>
#include <utils/messages.hpp>
#include <utils/text_metrics.hpp>

#include <algorithm>
#include <format>
#include <system_error>
#include <unistd.h>

namespace lsp_detail
{
    inline MiniJson::Value jstr(std::string s) { return MiniJson::Value{std::move(s)}; }
    inline MiniJson::Value jnum(double n) { return MiniJson::Value{n}; }
    inline MiniJson::Value jbool(bool b) { return MiniJson::Value{b}; }
    inline MiniJson::Value jobject(MiniJson::Object o) { return MiniJson::Value{std::move(o)}; }
    inline MiniJson::Value jarray(MiniJson::Array a) { return MiniJson::Value{std::move(a)}; }
    inline MiniJson::Value jnull() { return MiniJson::Value{nullptr}; }

    inline void advance_pos(int &line, int &col, std::string_view text)
    {
        for (char ch : text)
        {
            if (ch == '\n')
            {
                ++line;
                col = 0;
            }
            else
            {
                ++col;
            }
        }
    }

    inline int utf16_on_line(const std::vector<std::string> &lines, int line, int byte_col)
    {
        if (line < 0 || line >= static_cast<int>(lines.size()))
            return 0;
        return TextMetrics::byte_to_utf16(
            lines[static_cast<std::size_t>(line)],
            static_cast<std::size_t>(std::max(0, byte_col)));
    }

    inline bool json_provider_on(const MiniJson::Value *v)
    {
        if (!v)
            return false;
        if (v->is_bool())
            return v->as_bool(false);
        return v->is_object();
    }

    inline int json_sync_kind(const MiniJson::Value *sync)
    {
        if (!sync)
            return 2;
        if (sync->is_number())
            return static_cast<int>(sync->as_number(2));
        if (sync->is_object())
        {
            if (const MiniJson::Value *ch = sync->get("change"); ch && ch->is_number())
                return static_cast<int>(ch->as_number(2));
        }
        return 2;
    }

    inline fs::path walk_root_markers(const fs::path &hint, const std::vector<std::string> &markers)
    {
        if (markers.empty() || hint.empty())
            return {};
        std::error_code ec;
        fs::path cur = hint;
        if (fs::is_regular_file(cur, ec))
            cur = cur.parent_path();
        while (!cur.empty())
        {
            for (const auto &m : markers)
            {
                if (m.empty())
                    continue;
                if (fs::exists(cur / m, ec) && !ec)
                    return cur;
            }
            const fs::path parent = cur.parent_path();
            if (parent == cur)
                break;
            cur = parent;
        }
        return {};
    }

    inline DiagnosticSeverity severity_from_json(const MiniJson::Value *v)
    {
        const int s = v && v->is_number() ? v->as_int(1) : 1;
        switch (s)
        {
        case 1:
            return DiagnosticSeverity::Error;
        case 2:
            return DiagnosticSeverity::Warning;
        case 3:
            return DiagnosticSeverity::Information;
        case 4:
            return DiagnosticSeverity::Hint;
        default:
            return DiagnosticSeverity::Error;
        }
    }

    CompletionItem parse_completion_item(const MiniJson::Value &item, Buffer *buf);
    LspLocation parse_location(const MiniJson::Value &v, Buffer *hint_buf);
    void collect_locations(const MiniJson::Value &result, Buffer *hint, std::vector<LspLocation> &out);
    LspTextEdit parse_text_edit_utf16(const MiniJson::Value &te);
    LspWorkspaceEdit parse_workspace_edit(const MiniJson::Value &edit);
    void flatten_document_symbol(
        const MiniJson::Value &sym,
        const std::string &uri,
        Buffer *buf,
        std::vector<LspSymbol> &out,
        const std::string &prefix);
    LspCodeAction parse_code_action(const MiniJson::Value &v);
}

using namespace lsp_detail;
