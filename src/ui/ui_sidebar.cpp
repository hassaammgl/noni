#include "ui_impl.hpp"

std::string UI::when_context() const
{
    std::string ctx;
    if (focus == Focus::Editor)
        ctx += "editorFocus ";
    if (focus == Focus::Sidebar)
        ctx += "sidebarFocus ";
    if (focus == Focus::Search)
        ctx += "searchFocus ";
    if (focus == Focus::Command)
        ctx += "commandFocus ";
    if (focus == Focus::Messages)
        ctx += "messagesFocus ";
    if (focus == Focus::FileSearch)
        ctx += "fileSearchFocus ";
    if (focus == Focus::BufferSearch)
        ctx += "bufferSearchFocus ";
    if (focus == Focus::LspPicker)
        ctx += "lspPickerFocus ";
    if (focus == Focus::Completion)
        ctx += "completionFocus ";
    if (focus == Focus::Confirm)
        ctx += "confirmFocus ";
    if (focus == Focus::Prompt)
        ctx += "promptFocus ";
    if (focus == Focus::Terminal)
        ctx += "terminalFocus ";
    if (editor.get_mode() == EditorMode::Normal)
        ctx += "normalMode ";
    if (editor.get_mode() == EditorMode::Insert)
        ctx += "insertMode ";
    if (editor.get_mode() == EditorMode::Visual || editor.get_mode() == EditorMode::VisualLine)
        ctx += "visualMode ";
    if (sidebar_visible)
        ctx += "sidebarVisible ";
    else
        ctx += "sidebarHidden ";
    if (terminal_visible)
        ctx += "terminalVisible ";
    else
        ctx += "terminalHidden ";
    return StrUtils::trim(ctx);
}

void UI::set_sidebar_visible(bool visible)
{
    if (sidebar_visible == visible)
        return;

    sidebar_visible = visible;
    if (!sidebar_visible && (focus == Focus::Sidebar || focus == Focus::Search))
    {
        focus = Focus::Editor;
        editor.enter_normal_mode();
    }
    keys.clear_chord();
    resize();
}

void UI::toggle_sidebar()
{
    set_sidebar_visible(!sidebar_visible);
}

void UI::open_sidebar(bool focus_sidebar)
{
    open_explorer_view(focus_sidebar);
}

int UI::effective_sidebar_width() const
{
    if (!sidebar_visible)
        return 0;
    if (side_view == SideView::Search)
        return std::max(sidebar_width, 36);
    return sidebar_width;
}

void UI::open_explorer_view(bool focus_explorer)
{
    sidebar_visible = true;
    side_view = SideView::Explorer;
    search_panel.close();
    if (focus_explorer)
    {
        focus = Focus::Sidebar;
        editor.enter_normal_mode();
    }
    keys.clear_chord();
    resize();
}

void UI::open_search_view(bool focus_search)
{
    sidebar_visible = true;
    side_view = SideView::Search;
    search_panel.set_root(project_root());
    search_panel.open();
    if (focus_search)
    {
        focus = Focus::Search;
        editor.enter_normal_mode();
    }
    keys.clear_chord();
    resize();
}


void UI::close_sidebar()
{
    set_sidebar_visible(false);
}
