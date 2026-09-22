#include "lsp_internal.hpp"

void LspService::on_publish_diagnostics(const MiniJson::Value &params)
{
    // Called from LspSession::on_message during pump() — mu_ already held.
    const std::string uri = params.get_string("uri", "");
    if (uri.empty())
        return;
    Buffer *buf = buffer_for_uri(uri);
    if (!buf)
        return;

    int version = -1;
    if (const MiniJson::Value *v = params.get("version"); v && v->is_number())
        version = v->as_int(-1);

    auto dit = documents_.find(reinterpret_cast<std::uintptr_t>(buf));
    if (dit != documents_.end() && version >= 0 && version < dit->second.version)
        return;

    DiagnosticSnapshot snap;
    snap.lsp_version = version;
    if (const MiniJson::Value *arr = params.get("diagnostics"); arr && arr->is_array())
    {
        for (const auto &d : arr->as_array())
        {
            if (!d.is_object())
                continue;
            Diagnostic item;
            item.severity = severity_from_json(d.get("severity"));
            item.message = d.get_string("message", "");
            item.source = d.get_string("source", "");
            if (const MiniJson::Value *code = d.get("code"); code)
            {
                if (code->is_string())
                    item.code = code->as_string();
                else if (code->is_number())
                    item.code = std::to_string(code->as_int(0));
            }
            if (const MiniJson::Value *range = d.get("range"); range && range->is_object())
            {
                const MiniJson::Value *start = range->get("start");
                const MiniJson::Value *end = range->get("end");
                if (start && end)
                {
                    item.start = lsp_pos_to_cursor(
                        buf->lines(), start->get_int("line", 0), start->get_int("character", 0));
                    item.end = lsp_pos_to_cursor(
                        buf->lines(), end->get_int("line", 0), end->get_int("character", 0));
                }
            }
            snap.items.push_back(std::move(item));
        }
    }
    buf->set_diagnostics(std::move(snap));
}

void LspService::on_workspace_apply_edit(const MiniJson::Value &params)
{
    if (const MiniJson::Value *edit = params.get("edit"); edit && edit->is_object())
        ready_server_apply_ = parse_workspace_edit(*edit);
}

void LspService::on_response(int id, const MiniJson::Value &result)
{
    // Called from LspSession::on_message during pump() — mu_ already held.
    auto pit = pending_.find(id);
    if (pit == pending_.end())
        return;
    const LspPendingRequest pending = pit->second;
    pending_.erase(pit);

    auto dit = documents_.find(pending.buffer_id);
    Buffer *buf = nullptr;
    if (dit != documents_.end() && dit->second.open)
    {
        if (dit->second.version != pending.doc_version &&
            pending.kind != LspPendingKind::WorkspaceSymbol)
            return;
        buf = dit->second.buffer;
    }

    switch (pending.kind)
    {
    case LspPendingKind::Completion:
    {
        CompletionList list;
        list.request_id = id;
        list.buffer_id = pending.buffer_id;
        list.doc_version = pending.doc_version;
        list.trigger = pending.trigger;
        const MiniJson::Array *items = nullptr;
        if (result.is_array())
            items = &result.as_array();
        else if (result.is_object())
        {
            list.incomplete =
                result.get("isIncomplete") && result.get("isIncomplete")->as_bool(false);
            if (const MiniJson::Value *arr = result.get("items"); arr && arr->is_array())
                items = &arr->as_array();
        }
        if (items && buf)
        {
            list.items.reserve(items->size());
            for (const auto &it : *items)
            {
                if (it.is_object())
                    list.items.push_back(parse_completion_item(it, buf));
            }
        }
        ready_completion_ = std::move(list);
        break;
    }
    case LspPendingKind::Definition:
    case LspPendingKind::Declaration:
    case LspPendingKind::TypeDefinition:
    case LspPendingKind::References:
    {
        LspLocationList list;
        list.request_id = id;
        list.buffer_id = pending.buffer_id;
        list.doc_version = pending.doc_version;
        if (pending.kind == LspPendingKind::Declaration)
            list.kind = LspLocationList::Kind::Declaration;
        else if (pending.kind == LspPendingKind::TypeDefinition)
            list.kind = LspLocationList::Kind::TypeDefinition;
        else if (pending.kind == LspPendingKind::References)
            list.kind = LspLocationList::Kind::References;
        else
            list.kind = LspLocationList::Kind::Definition;
        collect_locations(result, buf, list.items);
        ready_locations_ = std::move(list);
        break;
    }
    case LspPendingKind::DocumentSymbol:
    case LspPendingKind::WorkspaceSymbol:
    {
        LspSymbolList list;
        list.request_id = id;
        list.buffer_id = pending.buffer_id;
        list.doc_version = pending.doc_version;
        list.workspace = (pending.kind == LspPendingKind::WorkspaceSymbol);
        const std::string uri = (buf && dit != documents_.end()) ? dit->second.uri : "";
        if (result.is_array())
        {
            for (const auto &sym : result.as_array())
                flatten_document_symbol(sym, uri, buf, list.items, "");
        }
        ready_symbols_ = std::move(list);
        break;
    }
    case LspPendingKind::Rename:
    {
        LspRenameResult rr;
        rr.request_id = id;
        rr.buffer_id = pending.buffer_id;
        rr.doc_version = pending.doc_version;
        if (result.is_object())
            rr.edit = parse_workspace_edit(result);
        ready_rename_ = std::move(rr);
        break;
    }
    case LspPendingKind::CodeAction:
    {
        LspCodeActionList list;
        list.request_id = id;
        list.buffer_id = pending.buffer_id;
        list.doc_version = pending.doc_version;
        if (result.is_array())
        {
            for (const auto &a : result.as_array())
            {
                auto action = parse_code_action(a);
                if (!action.title.empty())
                    list.items.push_back(std::move(action));
            }
        }
        ready_actions_ = std::move(list);
        break;
    }
    }
}
