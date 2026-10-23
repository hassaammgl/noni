#include "lsp_internal.hpp"

int LspService::request_completion(Buffer &buffer, int line, int byte_col)
{
    std::lock_guard lock(mu_);
    auto it = documents_.find(reinterpret_cast<std::uintptr_t>(&buffer));
    if (it == documents_.end() || !it->second.open)
        return 0;
    LspSession *session = session_for(it->second.language);
    if (!session)
        return 0;
    if (session->state() == LspSessionState::Starting)
        return -1; // still handshaking — UI can show "starting"
    if (session->state() != LspSessionState::Running)
        return 0;

    const int u16 = utf16_on_line(buffer.lines(), line, byte_col);
    const int id = session->request_completion(it->second.uri, line, u16);
    if (id <= 0)
        return 0;

    pending_[id] = LspPendingRequest{
        .kind = LspPendingKind::Completion,
        .buffer_id = reinterpret_cast<std::uintptr_t>(&buffer),
        .doc_version = it->second.version,
        .trigger = {.line = line, .column = byte_col},
    };
    ready_completion_.reset();
    return id;
}


int LspService::begin_position_request(
    Buffer &buffer,
    int line,
    int byte_col,
    LspPendingKind kind,
    const char *method)
{
    auto it = documents_.find(reinterpret_cast<std::uintptr_t>(&buffer));
    if (it == documents_.end() || !it->second.open)
        return 0;
    LspSession *session = session_for(it->second.language);
    if (!session || session->state() != LspSessionState::Running)
        return 0;

    const int u16 = utf16_on_line(buffer.lines(), line, byte_col);
    int id = 0;
    if (kind == LspPendingKind::References)
        id = session->request_references(it->second.uri, line, u16);
    else
        id = session->request_position(method, it->second.uri, line, u16);
    if (id <= 0)
        return 0;

    pending_[id] = LspPendingRequest{
        .kind = kind,
        .buffer_id = reinterpret_cast<std::uintptr_t>(&buffer),
        .doc_version = it->second.version,
        .trigger = {.line = line, .column = byte_col},
    };
    return id;
}

int LspService::request_definition(Buffer &buffer, int line, int byte_col)
{
    std::lock_guard lock(mu_);
    ready_locations_.reset();
    return begin_position_request(
        buffer, line, byte_col, LspPendingKind::Definition, "textDocument/definition");
}

int LspService::request_declaration(Buffer &buffer, int line, int byte_col)
{
    std::lock_guard lock(mu_);
    ready_locations_.reset();
    return begin_position_request(
        buffer, line, byte_col, LspPendingKind::Declaration, "textDocument/declaration");
}

int LspService::request_type_definition(Buffer &buffer, int line, int byte_col)
{
    std::lock_guard lock(mu_);
    ready_locations_.reset();
    return begin_position_request(
        buffer, line, byte_col, LspPendingKind::TypeDefinition, "textDocument/typeDefinition");
}

int LspService::request_references(Buffer &buffer, int line, int byte_col)
{
    std::lock_guard lock(mu_);
    ready_locations_.reset();
    return begin_position_request(
        buffer, line, byte_col, LspPendingKind::References, "textDocument/references");
}

int LspService::request_document_symbols(Buffer &buffer)
{
    std::lock_guard lock(mu_);
    ready_symbols_.reset();
    auto it = documents_.find(reinterpret_cast<std::uintptr_t>(&buffer));
    if (it == documents_.end() || !it->second.open)
        return 0;
    LspSession *session = session_for(it->second.language);
    if (!session || session->state() != LspSessionState::Running)
        return 0;
    const int id = session->request_document_symbol(it->second.uri);
    if (id <= 0)
        return 0;
    pending_[id] = LspPendingRequest{
        .kind = LspPendingKind::DocumentSymbol,
        .buffer_id = reinterpret_cast<std::uintptr_t>(&buffer),
        .doc_version = it->second.version,
    };
    return id;
}

int LspService::request_workspace_symbols(Buffer &buffer, const std::string &query)
{
    std::lock_guard lock(mu_);
    ready_symbols_.reset();
    auto it = documents_.find(reinterpret_cast<std::uintptr_t>(&buffer));
    if (it == documents_.end() || !it->second.open)
        return 0;
    LspSession *session = session_for(it->second.language);
    if (!session || session->state() != LspSessionState::Running)
        return 0;
    const int id = session->request_workspace_symbol(query);
    if (id <= 0)
        return 0;
    pending_[id] = LspPendingRequest{
        .kind = LspPendingKind::WorkspaceSymbol,
        .buffer_id = reinterpret_cast<std::uintptr_t>(&buffer),
        .doc_version = it->second.version,
    };
    return id;
}

int LspService::request_rename(Buffer &buffer, int line, int byte_col, const std::string &new_name)
{
    std::lock_guard lock(mu_);
    ready_rename_.reset();
    auto it = documents_.find(reinterpret_cast<std::uintptr_t>(&buffer));
    if (it == documents_.end() || !it->second.open)
        return 0;
    LspSession *session = session_for(it->second.language);
    if (!session || session->state() != LspSessionState::Running)
        return 0;
    const int u16 = utf16_on_line(buffer.lines(), line, byte_col);
    const int id = session->request_rename(it->second.uri, line, u16, new_name);
    if (id <= 0)
        return 0;
    pending_[id] = LspPendingRequest{
        .kind = LspPendingKind::Rename,
        .buffer_id = reinterpret_cast<std::uintptr_t>(&buffer),
        .doc_version = it->second.version,
        .trigger = {.line = line, .column = byte_col},
    };
    return id;
}

int LspService::request_code_actions(Buffer &buffer, Cursor start, Cursor end)
{
    std::lock_guard lock(mu_);
    ready_actions_.reset();
    auto it = documents_.find(reinterpret_cast<std::uintptr_t>(&buffer));
    if (it == documents_.end() || !it->second.open)
        return 0;
    LspSession *session = session_for(it->second.language);
    if (!session || session->state() != LspSessionState::Running)
        return 0;
    const int s16 = utf16_on_line(buffer.lines(), start.line, start.column);
    const int e16 = utf16_on_line(buffer.lines(), end.line, end.column);
    const int id = session->request_code_action(
        it->second.uri, start.line, s16, end.line, e16);
    if (id <= 0)
        return 0;
    pending_[id] = LspPendingRequest{
        .kind = LspPendingKind::CodeAction,
        .buffer_id = reinterpret_cast<std::uintptr_t>(&buffer),
        .doc_version = it->second.version,
        .trigger = start,
    };
    return id;
}
