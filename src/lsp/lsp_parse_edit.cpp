#include "lsp_internal.hpp"

namespace lsp_detail
{
    LspWorkspaceEdit parse_workspace_edit(const MiniJson::Value &edit)
    {
        LspWorkspaceEdit out;
        out.utf16_pending = true;
        if (!edit.is_object())
            return out;

        if (const MiniJson::Value *changes = edit.get("changes"); changes && changes->is_object())
        {
            for (const auto &[uri, arr] : changes->as_object())
            {
                if (!arr.is_array())
                    continue;
                std::vector<LspTextEdit> edits;
                for (const auto &te : arr.as_array())
                {
                    if (te.is_object())
                        edits.push_back(parse_text_edit_utf16(te));
                }
                if (!edits.empty())
                    out.changes[uri] = std::move(edits);
            }
        }

        if (const MiniJson::Value *docs = edit.get("documentChanges"); docs && docs->is_array())
        {
            for (const auto &dc : docs->as_array())
            {
                if (!dc.is_object())
                    continue;
                // TextDocumentEdit
                std::string uri;
                if (const MiniJson::Value *td = dc.get("textDocument"); td && td->is_object())
                    uri = td->get_string("uri", "");
                if (uri.empty())
                    continue;
                if (const MiniJson::Value *edits = dc.get("edits"); edits && edits->is_array())
                {
                    auto &dest = out.changes[uri];
                    for (const auto &te : edits->as_array())
                    {
                        if (te.is_object())
                            dest.push_back(parse_text_edit_utf16(te));
                    }
                }
            }
        }
        return out;
    }

    void flatten_document_symbol(
        const MiniJson::Value &sym,
        const std::string &uri,
        Buffer *buf,
        std::vector<LspSymbol> &out,
        const std::string &prefix)
    {
        if (!sym.is_object())
            return;
        LspSymbol item;
        item.name = sym.get_string("name", "");
        item.detail = sym.get_string("detail", "");
        if (const MiniJson::Value *k = sym.get("kind"); k && k->is_number())
            item.kind = k->as_int(0);

        // DocumentSymbol uses range/selectionRange; SymbolInformation uses location.
        if (const MiniJson::Value *loc = sym.get("location"); loc && loc->is_object())
        {
            item.location = parse_location(*loc, buf);
        }
        else
        {
            item.location.uri = uri;
            const MiniJson::Value *range = sym.get("selectionRange");
            if (!range)
                range = sym.get("range");
            if (range && range->is_object())
            {
                const MiniJson::Value *s = range->get("start");
                const MiniJson::Value *e = range->get("end");
                if (s && e)
                {
                    item.location.start = {
                        .line = s->get_int("line", 0),
                        .column = s->get_int("character", 0)};
                    item.location.end = {
                        .line = e->get_int("line", 0),
                        .column = e->get_int("character", 0)};
                }
            }
        }

        const std::string full = prefix.empty() ? item.name : prefix + " / " + item.name;
        item.location.display = std::format(
            "{}  {}:{}",
            full,
            item.location.start.line + 1,
            item.location.start.column + 1);
        out.push_back(item);

        if (const MiniJson::Value *children = sym.get("children"); children && children->is_array())
        {
            for (const auto &ch : children->as_array())
                flatten_document_symbol(ch, uri, buf, out, full);
        }
    }

    LspCodeAction parse_code_action(const MiniJson::Value &v)
    {
        LspCodeAction out;
        if (!v.is_object())
            return out;

        // Command-only (legacy)
        if (!v.get("title") && v.get("command") && v.get("command")->is_string())
        {
            out.title = v.get_string("command", "");
            out.has_command = true;
            out.command = out.title;
            return out;
        }

        out.title = v.get_string("title", "");
        out.kind = v.get_string("kind", "");
        out.is_preferred = v.get("isPreferred") && v.get("isPreferred")->as_bool(false);
        if (const MiniJson::Value *edit = v.get("edit"); edit && edit->is_object())
        {
            out.has_edit = true;
            out.edit = parse_workspace_edit(*edit);
        }
        if (const MiniJson::Value *cmd = v.get("command"); cmd && cmd->is_object())
        {
            out.has_command = true;
            out.command = cmd->get_string("command", "");
        }
        return out;
    }
}
