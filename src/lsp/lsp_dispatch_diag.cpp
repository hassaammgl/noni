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
