#include "lsp_internal.hpp"

namespace lsp_detail
{
    CompletionItem parse_completion_item(const MiniJson::Value &item, Buffer *buf)
    {
        CompletionItem out;
        out.label = item.get_string("label", "");
        out.detail = item.get_string("detail", "");
        out.insert_text = item.get_string("insertText", "");
        if (const MiniJson::Value *k = item.get("kind"); k && k->is_number())
            out.kind = k->as_int(0);
        if (const MiniJson::Value *doc = item.get("documentation"); doc)
        {
            if (doc->is_string())
                out.documentation = doc->as_string();
            else if (doc->is_object())
                out.documentation = doc->get_string("value", "");
        }

        if (const MiniJson::Value *te = item.get("textEdit"); te && te->is_object())
        {
            out.new_text = te->get_string("newText", out.label);
            if (const MiniJson::Value *range = te->get("range"); range && range->is_object() && buf)
            {
                const MiniJson::Value *start = range->get("start");
                const MiniJson::Value *end = range->get("end");
                if (start && end)
                {
                    out.has_text_edit = true;
                    out.edit_start = LspService::lsp_pos_to_cursor(
                        buf->lines(),
                        start->get_int("line", 0),
                        start->get_int("character", 0));
                    out.edit_end = LspService::lsp_pos_to_cursor(
                        buf->lines(),
                        end->get_int("line", 0),
                        end->get_int("character", 0));
                }
            }
        }
        if (out.new_text.empty())
            out.new_text = !out.insert_text.empty() ? out.insert_text : out.label;
        return out;
    }

    LspLocation parse_location(const MiniJson::Value &v, Buffer * /*hint_buf*/)
    {
        LspLocation loc;
        if (!v.is_object())
            return loc;

        auto set_range = [&](const MiniJson::Value *range) {
            if (!range || !range->is_object())
                return;
            const MiniJson::Value *s = range->get("start");
            const MiniJson::Value *e = range->get("end");
            if (!s || !e)
                return;
            // Store UTF-16 characters; convert at navigation against the target Buffer.
            loc.start = {.line = s->get_int("line", 0), .column = s->get_int("character", 0)};
            loc.end = {.line = e->get_int("line", 0), .column = e->get_int("character", 0)};
        };

        if (v.get("targetUri"))
        {
            loc.uri = v.get_string("targetUri", "");
            const MiniJson::Value *range = v.get("targetSelectionRange");
            if (!range)
                range = v.get("targetRange");
            set_range(range);
        }
        else
        {
            loc.uri = v.get_string("uri", "");
            set_range(v.get("range"));
        }

        const fs::path path = LspService::uri_to_path(loc.uri);
        loc.display = std::format(
            "{}:{}:{}",
            path.empty() ? loc.uri : path.filename().string(),
            loc.start.line + 1,
            loc.start.column + 1);
        return loc;
    }

    void collect_locations(const MiniJson::Value &result, Buffer *hint, std::vector<LspLocation> &out)
    {
        if (result.is_null())
            return;
        if (result.is_array())
        {
            for (const auto &item : result.as_array())
            {
                auto loc = parse_location(item, hint);
                if (!loc.uri.empty())
                    out.push_back(std::move(loc));
            }
            return;
        }
        if (result.is_object())
        {
            auto loc = parse_location(result, hint);
            if (!loc.uri.empty())
                out.push_back(std::move(loc));
        }
    }

    LspTextEdit parse_text_edit_utf16(const MiniJson::Value &te)
    {
        LspTextEdit out;
        out.new_text = te.get_string("newText", "");
        if (const MiniJson::Value *range = te.get("range"); range && range->is_object())
        {
            if (const MiniJson::Value *s = range->get("start"))
            {
                out.start.line = s->get_int("line", 0);
                out.start.column = s->get_int("character", 0);
            }
            if (const MiniJson::Value *e = range->get("end"))
            {
                out.end.line = e->get_int("line", 0);
                out.end.column = e->get_int("character", 0);
            }
        }
        return out;
    }
}
