#include "ui_impl.hpp"

bool UI::dispatch_resolved(int key, InputContext ctx)
{
    const ResolveResult result = keys.resolve(key, when_context(), ctx);
    if (result.status == ResolveStatus::Matched)
    {
        if (!commands.execute(result.command_id))
            Logger::warning(std::format("unhandled command: {}", result.command_id));
        return true;
    }
    return result.status == ResolveStatus::Prefix;
}

void UI::insert_focused_utf8(std::string_view utf8)
{
    switch (focus)
    {
    case Focus::Editor:
        editor.insert_utf8(utf8);
        break;
    case Focus::Command:
        command_line.insert_utf8(utf8);
        break;
    case Focus::Prompt:
        input_prompt.insert_utf8(utf8);
        break;
    case Focus::Search:
        search_panel.insert_utf8(utf8);
        break;
    case Focus::FileSearch:
        file_picker.insert_utf8(utf8);
        break;
    case Focus::BufferSearch:
        buffer_picker.insert_utf8(utf8);
        break;
    case Focus::LspPicker:
        lsp_picker.insert_utf8(utf8);
        break;
    case Focus::Terminal:
        terminal.insert_utf8(utf8);
        break;
    default:
        break;
    }
}

void UI::handle_inputs()
{
    wint_t wch = 0;
    const int rc = get_wch(&wch);
    if (rc == ERR)
        return;

    int ch = 0;
    if (rc == KEY_CODE_YES)
    {
        ch = static_cast<int>(wch);
    }
    else
    {
        const auto cp = static_cast<char32_t>(wch);
        if (cp >= 32 && cp != 127 && cp > 126)
        {
            char buf[8];
            const int n = TextMetrics::encode_utf8(cp, buf);
            insert_focused_utf8(std::string_view(buf, static_cast<std::size_t>(n)));
            return;
        }
        ch = static_cast<int>(wch);
    }

    if (ch == KEY_RESIZE)
    {
        resize();
        return;
    }

    if (focus == Focus::Command)
    {
        if (ch == 27 || ch == 3)
        {
            (void)dispatch_resolved(ch, InputContext::CommandLine);
            return;
        }
        if (ch == '\n' || ch == KEY_ENTER)
        {
            execute_command();
            return;
        }
        command_line.handle_input(ch);
        if (!command_line.is_active())
            return_to_normal();
        return;
    }

    if (focus == Focus::Messages)
    {
        if (ch == 27 || ch == 3)
        {
            (void)dispatch_resolved(ch, InputContext::Messages);
            return;
        }
        messages_panel.handle_input(ch);
        if (!messages_panel.is_active())
            return_to_normal();
        return;
    }

    if (focus == Focus::FileSearch || focus == Focus::BufferSearch ||
        focus == Focus::LspPicker || focus == Focus::Completion ||
        focus == Focus::Terminal || focus == Focus::Search ||
        focus == Focus::Confirm || focus == Focus::Prompt)
    {
        handle_overlay_input(ch);
        return;
    }

    if (focus == Focus::Sidebar)
    {
        handle_sidebar_input(ch);
        return;
    }

    {
        const InputContext ctx = ui_input_context_for(focus, editor.get_mode(), false);
        if (dispatch_resolved(ch, ctx))
            return;
        if (focus == Focus::Editor)
            editor.handle_input(ch);
    }
}
