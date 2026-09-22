#include "ui_impl.hpp"

fs::path UI::project_root() const
{
    if (core.workspace().has_root())
        return core.workspace().root();
    fs::path root = sidebar.get_project_path();
    if (!root.empty())
        return root;
    if (core.buffers().has_tabs())
    {
        const auto path = core.buffers().active().buffer().get_buffer_path();
        if (!path.empty())
            return path.parent_path().empty() ? fs::current_path() : path.parent_path();
    }
    return fs::current_path();
}

fs::path UI::find_workspace_root(const fs::path &hint) const
{
    return Workspace::detect_root(hint);
}

void UI::note_opened_file(const fs::path &path)
{
    if (path.empty())
        return;
    core.recent().touch(path);
}

void UI::remap_buffer_path(const fs::path &from, const fs::path &to)
{
    if (from.empty() || to.empty())
        return;
    if (Buffer *b = core.buffers().find_buffer_by_path(from))
        b->set_save_path(to);
}

void UI::close_buffers_under(const fs::path &path)
{
    if (path.empty() || !core.buffers().has_tabs())
        return;
    // Close matching tabs from the end to keep indices stable enough.
    for (int guard = 0; guard < 64 && core.buffers().has_tabs(); ++guard)
    {
        bool closed_any = false;
        const auto &tabs = core.buffers().get_tabs();
        for (int i = static_cast<int>(tabs.size()) - 1; i >= 0; --i)
        {
            const auto p = tabs[static_cast<std::size_t>(i)].buffer().get_buffer_path();
            if (p.empty())
                continue;
            std::error_code ec;
            const bool match = (p == path) ||
                               (fs::is_directory(path, ec) &&
                                p.string().rfind(path.string(), 0) == 0);
            if (!match)
                continue;
            core.buffers().switch_to(i);
            Buffer *b = &core.buffers().active().buffer();
            core.lsp().notify_close(*b);
            core.on_buffer_closed(b);
            core.buffers().close_active(true);
            closed_any = true;
            break;
        }
        if (!closed_any)
            break;
    }
    if (!core.buffers().has_tabs())
        core.buffers().open_untitled();
    sync_active_tab();
}

void UI::apply_workspace_root(const fs::path &hint, bool announce)
{
    core.workspace().open(hint);
    const fs::path root = core.workspace().root();
    sidebar.set_project_path(root);
    search_panel.set_root(root);
    file_picker.warm(root);
    file_picker.reindex();
    refresh_scm(root);
    terminal.set_cwd(root.string());
    sync_fs_watches();
    if (announce)
        Messages::info(std::format("Workspace: {}", root.string()));
}

void UI::sync_fs_watches()
{
    if (!fs_watcher_.start())
        return;

    fs_watcher_.set_workspace(project_root());

    // Track open buffer files for external edit detection.
    if (!core.buffers().has_tabs())
        return;
    for (const auto &tab : core.buffers().get_tabs())
    {
        const fs::path path = tab.buffer().get_buffer_path();
        if (!path.empty())
            fs_watcher_.watch(path);
    }
}

void UI::poll_fs_events()
{
    const auto events = fs_watcher_.poll();
    if (events.empty() && !fs_explorer_dirty_)
        return;

    bool explorer_touch = fs_explorer_dirty_;
    for (const auto &ev : events)
    {
        Logger::debug(std::format(
            "FsWatcher event: {} ({})",
            ev.path.string(),
            ev.is_dir ? "dir" : "file"));

        // Workspace tree changes → debounce explorer + file index refresh.
        const fs::path root = project_root();
        if (!root.empty())
        {
            std::error_code ec;
            const fs::path abs = fs::weakly_canonical(ev.path, ec);
            const std::string rs = root.string();
            const std::string ps = ec ? ev.path.string() : abs.string();
            if (ps == rs || (ps.size() > rs.size() && ps.compare(0, rs.size(), rs) == 0 &&
                             (ps[rs.size()] == '/')))
                explorer_touch = true;
        }

        if (ev.is_dir)
        {
            if (ev.kind == FsEventKind::Created || ev.kind == FsEventKind::Moved)
                fs_watcher_.watch(ev.path);
            continue;
        }

        Buffer *buf = core.buffers().find_buffer_by_path(ev.path);
        if (!buf)
            continue;
        if (!buf->disk_changed())
            continue;

        if (buf->is_dirty())
        {
            if (!buf->external_change_notified())
            {
                buf->mark_external_change_notified();
                Messages::warning(std::format(
                    "{} changed on disk (buffer dirty — :e! to reload)",
                    ev.path.filename().string()));
            }
            continue;
        }

        // Clean buffer: safe auto-reload.
        buf->load();
        buf->clear_external_change_flag();
        Messages::info(std::format("Reloaded {}", ev.path.filename().string()));
        if (core.active_buffer() == buf)
            sync_active_tab();
    }

    if (explorer_touch)
    {
        fs_explorer_dirty_ = true;
        const auto now = std::chrono::steady_clock::now();
        if (fs_explorer_refresh_at_.time_since_epoch().count() == 0)
            fs_explorer_refresh_at_ = now + std::chrono::milliseconds(250);
    }

    if (fs_explorer_dirty_ && std::chrono::steady_clock::now() >= fs_explorer_refresh_at_)
    {
        fs_explorer_dirty_ = false;
        fs_explorer_refresh_at_ = {};
        sidebar.refresh();
        file_picker.reindex();
        Logger::debug("FsWatcher: explorer refreshed");
    }
}

void UI::sync_messages_echo()
{
    const auto [seq, text] = Messages::echo_snapshot();
    if (seq == messages_echo_seen_)
        return;
    messages_echo_seen_ = seq;
    if (!text.empty())
        statusbar.set_echo(text);
}
