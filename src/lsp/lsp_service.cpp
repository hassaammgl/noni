#include "lsp_internal.hpp"

void LspService::set_workspace_root(const fs::path &root)
{
    LspInstaller::set_workspace_root(root);
    std::lock_guard lock(mu_);
    if (workspace_root_ == root)
        return;
    for (auto &[k, s] : sessions_)
    {
        if (s)
            s->stop();
    }
    sessions_.clear();
    documents_.clear();
    pending_.clear();
    clear_ready();
    workspace_root_ = root;
}

fs::path LspService::workspace_root() const
{
    std::lock_guard lock(mu_);
    return workspace_root_;
}

void LspService::set_server_configs(std::vector<LspServerConfig> configs)
{
    std::lock_guard lock(mu_);
    configs_ = std::move(configs);
}

void LspService::pump()
{
    const std::uint64_t gen = LspInstaller::generation();
    std::vector<Buffer *> retry;
    {
        std::lock_guard lock(mu_);
        if (gen != install_gen_)
        {
            install_gen_ = gen;
            for (auto &[id, doc] : documents_)
            {
                (void)id;
                if (!doc.open && doc.buffer)
                    retry.push_back(doc.buffer);
            }
        }
        for (auto &[k, s] : sessions_)
        {
            if (s)
                s->pump();
        }
    }
    for (Buffer *b : retry)
    {
        if (b)
            notify_open(*b);
    }
}


LspSession *LspService::session_for(Language lang, const fs::path &file_hint)
{
    if (lang == Language::Plain || workspace_root_.empty())
        return nullptr;

    const std::string name = language_id_for(lang);
    auto it = sessions_.find(name);
    if (it != sessions_.end() && it->second)
    {
        const auto st = it->second->state();
        if (st == LspSessionState::Failed || st == LspSessionState::Stopped)
        {
            it->second->stop();
            sessions_.erase(it);
        }
        else
        {
            return it->second.get();
        }
    }

    const LspServerConfig *cfg = nullptr;
    for (const auto &c : configs_)
    {
        if (c.language == name)
        {
            cfg = &c;
            break;
        }
    }
    if (!cfg && (lang == Language::C || lang == Language::Cpp))
    {
        for (const auto &c : configs_)
        {
            if (c.language == "cpp" || c.language == "c")
            {
                cfg = &c;
                break;
            }
        }
    }
    if (!cfg || cfg->command.empty())
        return nullptr;

    LspServerConfig resolved = *cfg;
    const fs::path bin = LspInstaller::resolve(resolved.command[0]);
    if (bin.empty())
    {
        LspInstaller::request(resolved.command[0]);
        return nullptr;
    }
    resolved.command[0] = bin.string();

    fs::path root = workspace_root_;
    const fs::path walk_from = file_hint.empty() ? workspace_root_ : file_hint;
    const fs::path marked = walk_root_markers(walk_from, resolved.root_markers);
    if (!marked.empty())
        root = marked;

    auto session = std::make_unique<LspSession>(std::move(resolved), root, this);
    if (!session->start())
        return nullptr;
    LspSession *raw = session.get();
    sessions_[name] = std::move(session);
    return raw;
}

LspDocumentState *LspService::doc_for(Buffer &buffer)
{
    const auto id = reinterpret_cast<std::uintptr_t>(&buffer);
    return &documents_[id];
}

Buffer *LspService::buffer_for_uri(const std::string &uri)
{
    for (auto &[id, doc] : documents_)
    {
        (void)id;
        if (doc.open && doc.uri == uri && doc.buffer)
            return doc.buffer;
    }
    return nullptr;
}

void LspService::shutdown_all()
{
    std::lock_guard lock(mu_);
    for (auto &[k, s] : sessions_)
    {
        if (s)
            s->stop();
    }
    sessions_.clear();
    documents_.clear();
    pending_.clear();
    clear_ready();
}

std::string LspService::status_summary() const
{
    std::lock_guard lock(mu_);
    if (workspace_root_.empty())
        return "LSP: no workspace";
    if (sessions_.empty())
        return std::format("LSP: idle @ {}", workspace_root_.string());
    std::string out = "LSP:";
    for (const auto &[name, s] : sessions_)
    {
        if (!s)
            continue;
        const char *st = "?";
        switch (s->state())
        {
        case LspSessionState::Starting:
            st = "starting";
            break;
        case LspSessionState::Running:
            st = "running";
            break;
        case LspSessionState::Failed:
            st = "failed";
            break;
        case LspSessionState::Stopped:
            st = "stopped";
            break;
        default:
            st = "idle";
            break;
        }
        out += std::format(" {}[{}]", name, st);
    }
    out += std::format(" · {} docs", documents_.size());
    return out;
}
