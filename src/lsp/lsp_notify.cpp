#include "lsp_internal.hpp"

void LspService::notify_open(Buffer &buffer)
{
    std::lock_guard lock(mu_);
    const fs::path path = buffer.get_buffer_path();
    if (path.empty() || buffer.has_load_error())
        return;

    const auto id = reinterpret_cast<std::uintptr_t>(&buffer);
    auto existing = documents_.find(id);
    if (existing != documents_.end() && existing->second.open)
        return;

    const Language lang = Syntax::detect_language(path);
    auto &doc = *doc_for(buffer);
    doc.uri = path_to_uri(path);
    doc.language = lang;
    doc.buffer = &buffer;
    // Keep intent even while the binary is still installing.
    if (doc.open)
        return;

    LspSession *session = session_for(lang, path);
    if (!session)
        return;

    doc.version = 1;
    doc.open = true;
    session->did_open(doc.uri, language_id_for(lang), doc.version, join_lines(buffer.lines()));
}

void LspService::notify_change(Buffer &buffer, const TextChange &change)
{
    std::lock_guard lock(mu_);
    auto it = documents_.find(reinterpret_cast<std::uintptr_t>(&buffer));
    if (it == documents_.end() || !it->second.open)
        return;

    auto &doc = it->second;
    LspSession *session = session_for(doc.language);
    if (!session)
        return;

    ++doc.version;
    const auto &lines = buffer.lines();
    const int kind = session->sync_kind();
    if (kind == 0)
        return;
    if (kind == 1)
    {
        session->did_change_full(doc.uri, doc.version, join_lines(lines));
        return;
    }

    int el = change.start_line;
    int ec = change.start_col;
    advance_pos(el, ec, change.deleted);

    const int start_u16 = utf16_on_line(lines, change.start_line, change.start_col);
    if (change.start_line != el)
    {
        session->did_change_full(doc.uri, doc.version, join_lines(lines));
        return;
    }

    int end_u16 = start_u16 + TextMetrics::byte_to_utf16(change.deleted, change.deleted.size());
    if (change.deleted.empty())
        end_u16 = start_u16;

    session->did_change_incremental(
        doc.uri,
        doc.version,
        change.start_line,
        start_u16,
        el,
        end_u16,
        change.inserted);
}

void LspService::notify_reload(Buffer &buffer)
{
    bool need_open = false;
    {
        std::lock_guard lock(mu_);
        auto it = documents_.find(reinterpret_cast<std::uintptr_t>(&buffer));
        if (it == documents_.end() || !it->second.open)
        {
            need_open = true;
        }
        else
        {
            auto &doc = it->second;
            LspSession *session = session_for(doc.language);
            if (!session)
                return;
            ++doc.version;
            session->did_change_full(doc.uri, doc.version, join_lines(buffer.lines()));
            return;
        }
    }
    if (need_open)
        notify_open(buffer);
}

void LspService::notify_save(Buffer &buffer)
{
    std::lock_guard lock(mu_);
    auto it = documents_.find(reinterpret_cast<std::uintptr_t>(&buffer));
    if (it == documents_.end() || !it->second.open)
        return;
    LspSession *session = session_for(it->second.language);
    if (!session)
        return;
    session->did_save(it->second.uri);
}

void LspService::notify_close(Buffer &buffer)
{
    std::lock_guard lock(mu_);
    const auto id = reinterpret_cast<std::uintptr_t>(&buffer);
    auto dit = documents_.find(id);
    if (dit == documents_.end())
        return;
    if (dit->second.open)
    {
        if (LspSession *session = session_for(dit->second.language))
            session->did_close(dit->second.uri);
    }
    for (auto pit = pending_.begin(); pit != pending_.end();)
    {
        if (pit->second.buffer_id == id)
            pit = pending_.erase(pit);
        else
            ++pit;
    }
    ready_completion_.reset();
    buffer.diagnostics() = {};
    documents_.erase(dit);
}
