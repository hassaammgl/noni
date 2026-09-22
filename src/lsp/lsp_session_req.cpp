#include "lsp_internal.hpp"

int LspSession::request_completion(const std::string &uri, int line, int character)
{
    if (state_ != LspSessionState::Running)
        return 0;
    if (!open_uris_.count(uri))
        return 0;
    const int id = rpc_.next_id();
    MiniJson::Object pos;
    pos["line"] = jnum(static_cast<double>(line));
    pos["character"] = jnum(static_cast<double>(character));
    MiniJson::Object td;
    td["uri"] = jstr(uri);
    MiniJson::Object params;
    params["textDocument"] = jobject(std::move(td));
    params["position"] = jobject(std::move(pos));
    if (!send_raw(rpc_.make_request(id, "textDocument/completion", jobject(std::move(params)))))
        return 0;
    return id;
}

int LspSession::request_position(const std::string &method, const std::string &uri, int line, int character)
{
    if (state_ != LspSessionState::Running || !open_uris_.count(uri))
        return 0;
    const int id = rpc_.next_id();
    MiniJson::Object params;
    params["textDocument"] = jobject({{"uri", jstr(uri)}});
    params["position"] = jobject({
        {"line", jnum(static_cast<double>(line))},
        {"character", jnum(static_cast<double>(character))},
    });
    if (!send_raw(rpc_.make_request(id, method, jobject(std::move(params)))))
        return 0;
    return id;
}

int LspSession::request_references(const std::string &uri, int line, int character)
{
    if (state_ != LspSessionState::Running || !open_uris_.count(uri))
        return 0;
    const int id = rpc_.next_id();
    MiniJson::Object params;
    params["textDocument"] = jobject({{"uri", jstr(uri)}});
    params["position"] = jobject({
        {"line", jnum(static_cast<double>(line))},
        {"character", jnum(static_cast<double>(character))},
    });
    params["context"] = jobject({{"includeDeclaration", jbool(true)}});
    if (!send_raw(rpc_.make_request(id, "textDocument/references", jobject(std::move(params)))))
        return 0;
    return id;
}

int LspSession::request_document_symbol(const std::string &uri)
{
    if (state_ != LspSessionState::Running || !open_uris_.count(uri))
        return 0;
    const int id = rpc_.next_id();
    MiniJson::Object params;
    params["textDocument"] = jobject({{"uri", jstr(uri)}});
    if (!send_raw(rpc_.make_request(id, "textDocument/documentSymbol", jobject(std::move(params)))))
        return 0;
    return id;
}

int LspSession::request_workspace_symbol(const std::string &query)
{
    if (state_ != LspSessionState::Running)
        return 0;
    const int id = rpc_.next_id();
    MiniJson::Object params;
    params["query"] = jstr(query);
    if (!send_raw(rpc_.make_request(id, "workspace/symbol", jobject(std::move(params)))))
        return 0;
    return id;
}

int LspSession::request_rename(
    const std::string &uri,
    int line,
    int character,
    const std::string &new_name)
{
    if (state_ != LspSessionState::Running || !open_uris_.count(uri))
        return 0;
    const int id = rpc_.next_id();
    MiniJson::Object params;
    params["textDocument"] = jobject({{"uri", jstr(uri)}});
    params["position"] = jobject({
        {"line", jnum(static_cast<double>(line))},
        {"character", jnum(static_cast<double>(character))},
    });
    params["newName"] = jstr(new_name);
    if (!send_raw(rpc_.make_request(id, "textDocument/rename", jobject(std::move(params)))))
        return 0;
    return id;
}

int LspSession::request_code_action(
    const std::string &uri,
    int start_line,
    int start_character,
    int end_line,
    int end_character)
{
    if (state_ != LspSessionState::Running || !open_uris_.count(uri))
        return 0;
    const int id = rpc_.next_id();
    MiniJson::Object params;
    params["textDocument"] = jobject({{"uri", jstr(uri)}});
    params["range"] = jobject({
        {"start",
         jobject({
             {"line", jnum(static_cast<double>(start_line))},
             {"character", jnum(static_cast<double>(start_character))},
         })},
        {"end",
         jobject({
             {"line", jnum(static_cast<double>(end_line))},
             {"character", jnum(static_cast<double>(end_character))},
         })},
    });
    params["context"] = jobject({{"diagnostics", jarray({})}});
    if (!send_raw(rpc_.make_request(id, "textDocument/codeAction", jobject(std::move(params)))))
        return 0;
    return id;
}

bool LspSession::is_document_open(const std::string &uri) const
{
    return open_uris_.count(uri) > 0;
}

// --- LspService ---
