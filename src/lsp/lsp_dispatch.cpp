#include "lsp_internal.hpp"

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
    case LspPendingKind::Hover:
    {
        LspHoverResult hr;
        hr.request_id = id;
        hr.buffer_id = pending.buffer_id;
        hr.contents = parse_hover_contents(result);
        ready_hover_ = std::move(hr);
        break;
    }
    case LspPendingKind::Formatting:
    {
        LspFormatResult fr;
        fr.request_id = id;
        fr.buffer_id = pending.buffer_id;
        fr.doc_version = pending.doc_version;
        fr.utf16_pending = true;
        if (result.is_array())
        {
            for (const auto &te : result.as_array())
                fr.edits.push_back(parse_text_edit_utf16(te));
        }
        ready_format_ = std::move(fr);
        break;
    }
    }
}
