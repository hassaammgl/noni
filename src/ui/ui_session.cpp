#include "ui_impl.hpp"

SessionState UI::capture_session_state() const
{
    SessionState st;
    st.workspace_root = core.workspace().root().string();
    st.active_index = core.buffers().get_active_index();
    st.sidebar_visible = sidebar_visible;
    st.terminal_visible = terminal_visible;
    for (const auto &tab : core.buffers().get_tabs())
    {
        const fs::path p = tab.buffer().get_buffer_path();
        if (p.empty())
            continue;
        SessionTabState t;
        t.path = p.string();
        t.cursor_line = tab.cursor().line;
        t.cursor_column = tab.cursor().column;
        t.scroll_y = tab.scroll_y();
        t.scroll_x = tab.scroll_x();
        st.tabs.push_back(std::move(t));
    }
    for (const auto &r : core.recent().list())
        st.recent.push_back(r.string());
    return st;
}

void UI::save_session()
{
    const fs::path root = core.workspace().root();
    if (root.empty())
        return;
    std::string err;
    if (!SessionStore::save(root, capture_session_state(), &err))
        Logger::warning(err.empty() ? "session save failed" : err);
}

bool UI::restore_session_if_available(bool allow_replace_tabs)
{
    const fs::path root = core.workspace().root();
    if (root.empty())
        return false;

    std::string err;
    auto st = SessionStore::load(root, &err);
    if (!st)
        return false;

    for (const auto &r : st->recent)
    {
        if (!r.empty())
            core.recent().touch(r);
    }

    sidebar_visible = st->sidebar_visible;
    terminal_visible = st->terminal_visible;

    if (!allow_replace_tabs || st->tabs.empty())
        return false;

    // Close current tabs (untitled only expected at startup).
    while (core.buffers().has_tabs())
    {
        if (!core.buffers().close_active(true))
            break;
        if (!core.buffers().has_tabs())
            break;
    }

    int opened = 0;
    for (const auto &t : st->tabs)
    {
        std::error_code ec;
        if (t.path.empty() || !fs::exists(t.path, ec))
            continue;
        core.buffers().open_file(t.path);
        note_opened_file(t.path);
        EditorTab &tab = core.buffers().active();
        tab.cursor().line = std::max(0, t.cursor_line);
        tab.cursor().column = std::max(0, t.cursor_column);
        tab.scroll_y() = std::max(0, t.scroll_y);
        tab.scroll_x() = std::max(0, t.scroll_x);
        ++opened;
    }

    if (opened == 0)
    {
        core.buffers().open_untitled();
        return false;
    }

    const int idx = std::clamp(st->active_index, 0, static_cast<int>(core.buffers().size()) - 1);
    core.buffers().switch_to(idx);
    session_restored_ = true;
    Logger::info(std::format("Session restored ({} tabs)", opened));
    return true;
}
