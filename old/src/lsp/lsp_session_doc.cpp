#include "lsp_internal.hpp"

void LspSession::did_open(
    const std::string &uri,
    const std::string &language_id,
    int version,
    const std::string &text)
{
    if (state_ == LspSessionState::Running)
    {
        flush_did_open(PendingOpen{uri, language_id, version, text});
        return;
    }
    if (state_ != LspSessionState::Starting)
        return;
    // Queue until initialized — sending early breaks clangd / completion.
    for (auto &p : pending_opens_)
    {
        if (p.uri == uri)
        {
            p.language_id = language_id;
            p.version = version;
            p.text = text;
            return;
        }
    }
    pending_opens_.push_back(PendingOpen{uri, language_id, version, text});
}

void LspSession::did_change_full(const std::string &uri, int version, const std::string &text)
{
    if (state_ != LspSessionState::Running)
        return;
    if (!open_uris_.count(uri))
        return;
    MiniJson::Object td;
    td["uri"] = jstr(uri);
    td["version"] = jnum(static_cast<double>(version));
    MiniJson::Object change;
    change["text"] = jstr(text);
    MiniJson::Object params;
    params["textDocument"] = jobject(std::move(td));
    params["contentChanges"] = jarray({jobject(std::move(change))});
    (void)send_raw(rpc_.make_notification("textDocument/didChange", jobject(std::move(params))));
}

void LspSession::did_change_incremental(
    const std::string &uri,
    int version,
    int start_line,
    int start_utf16,
    int end_line,
    int end_utf16,
    const std::string &text)
{
    if (state_ != LspSessionState::Running)
        return;
    if (!open_uris_.count(uri))
        return;
    MiniJson::Object start;
    start["line"] = jnum(static_cast<double>(start_line));
    start["character"] = jnum(static_cast<double>(start_utf16));
    MiniJson::Object end;
    end["line"] = jnum(static_cast<double>(end_line));
    end["character"] = jnum(static_cast<double>(end_utf16));
    MiniJson::Object range;
    range["start"] = jobject(std::move(start));
    range["end"] = jobject(std::move(end));
    MiniJson::Object change;
    change["range"] = jobject(std::move(range));
    change["text"] = jstr(text);
    MiniJson::Object td;
    td["uri"] = jstr(uri);
    td["version"] = jnum(static_cast<double>(version));
    MiniJson::Object params;
    params["textDocument"] = jobject(std::move(td));
    params["contentChanges"] = jarray({jobject(std::move(change))});
    (void)send_raw(rpc_.make_notification("textDocument/didChange", jobject(std::move(params))));
}

void LspSession::did_save(const std::string &uri)
{
    if (state_ != LspSessionState::Running)
        return;
    MiniJson::Object td;
    td["uri"] = jstr(uri);
    MiniJson::Object params;
    params["textDocument"] = jobject(std::move(td));
    (void)send_raw(rpc_.make_notification("textDocument/didSave", jobject(std::move(params))));
}

void LspSession::did_close(const std::string &uri)
{
    if (state_ == LspSessionState::Idle || state_ == LspSessionState::Stopped)
        return;
    open_uris_.erase(uri);
    MiniJson::Object td;
    td["uri"] = jstr(uri);
    MiniJson::Object params;
    params["textDocument"] = jobject(std::move(td));
    (void)send_raw(rpc_.make_notification("textDocument/didClose", jobject(std::move(params))));
}
