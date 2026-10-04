#include "lsp_internal.hpp"

int LspService::request_hover(Buffer &buffer, int line, int byte_col)
{
    std::lock_guard lock(mu_);
    ready_hover_.reset();
    auto it = documents_.find(reinterpret_cast<std::uintptr_t>(&buffer));
    if (it == documents_.end() || !it->second.open)
        return 0;
    LspSession *session = session_for(it->second.language);
    if (!session || session->state() != LspSessionState::Running || !session->hover_provider())
        return 0;
    const int u16 = utf16_on_line(buffer.lines(), line, byte_col);
    const int id = session->request_position("textDocument/hover", it->second.uri, line, u16);
    if (id <= 0)
        return 0;
    pending_[id] = LspPendingRequest{
        .kind = LspPendingKind::Hover,
        .buffer_id = reinterpret_cast<std::uintptr_t>(&buffer),
        .doc_version = it->second.version,
        .trigger = {.line = line, .column = byte_col},
    };
    return id;
}

int LspService::request_formatting(Buffer &buffer)
{
    std::lock_guard lock(mu_);
    ready_format_.reset();
    auto it = documents_.find(reinterpret_cast<std::uintptr_t>(&buffer));
    if (it == documents_.end() || !it->second.open)
        return 0;
    LspSession *session = session_for(it->second.language);
    if (!session || session->state() != LspSessionState::Running || !session->formatting_provider())
        return 0;
    const int id = session->request_formatting(it->second.uri);
    if (id <= 0)
        return 0;
    pending_[id] = LspPendingRequest{
        .kind = LspPendingKind::Formatting,
        .buffer_id = reinterpret_cast<std::uintptr_t>(&buffer),
        .doc_version = it->second.version,
        .trigger = {},
    };
    return id;
}

bool LspService::session_supports_hover(Buffer &buffer) const
{
    std::lock_guard lock(mu_);
    auto it = documents_.find(reinterpret_cast<std::uintptr_t>(&buffer));
    if (it == documents_.end())
        return false;
    auto *self = const_cast<LspService *>(this);
    LspSession *session = self->session_for(it->second.language);
    return session && session->state() == LspSessionState::Running && session->hover_provider();
}

bool LspService::session_supports_formatting(Buffer &buffer) const
{
    std::lock_guard lock(mu_);
    auto it = documents_.find(reinterpret_cast<std::uintptr_t>(&buffer));
    if (it == documents_.end())
        return false;
    auto *self = const_cast<LspService *>(this);
    LspSession *session = self->session_for(it->second.language);
    return session && session->state() == LspSessionState::Running && session->formatting_provider();
}
