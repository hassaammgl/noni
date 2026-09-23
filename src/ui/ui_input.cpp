#include "ui_impl.hpp"

bool UI::dispatch_resolved(int key, InputContext ctx)
{
    return dispatch_token(KeybindingEngine::from_raw(key), ctx);
}

bool UI::dispatch_token(KeyToken tok, InputContext ctx)
{
    const ResolveResult result = keys.resolve_token(tok, when_context(), ctx);
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

    if (std::getenv("NONI_KEYLOG"))
    {
        const int raw = static_cast<int>(wch);
        const char *nm = (rc == KEY_CODE_YES) ? keyname(raw) : nullptr;
        Logger::info(std::format(
            "KEYLOG rc={} ch={} name={}",
            rc,
            raw,
            nm ? nm : (rc == OK ? "OK" : "?")));
    }

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

    // Esc then a follow-up key (within escDelayMs) → Alt+key. Terminal keeps raw ESC prefix.
    KeyToken alt_tok{};
    bool have_alt = false;
    if (ch == 27 && rc != KEY_CODE_YES && focus != Focus::Terminal)
    {
        const int esc_ms = config.esc_delay_ms > 0 ? config.esc_delay_ms : 25;
        timeout(esc_ms);
        wint_t nxt = 0;
        const int nrc = get_wch(&nxt);
        timeout(50);
        if (nrc != ERR)
        {
            const int nch = static_cast<int>(nxt);
            alt_tok = KeybindingEngine::from_raw(nch);
            alt_tok.alt = true;
            have_alt = true;
            if (std::getenv("NONI_KEYLOG"))
                Logger::info(std::format("KEYLOG alt-follow ch={}", nch));
        }
    }

    if (have_alt)
    {
        const InputContext ctx = ui_input_context_for(focus, editor.get_mode(), false);
        if (focus == Focus::Editor || focus == Focus::Sidebar)
        {
            if (dispatch_token(alt_tok, ctx))
                return;
            return; // unmatched Alt+key: do not insert the follow-up
        }
        if (dispatch_token(alt_tok, ctx))
            return;
        ch = alt_tok.code;
    }

    if (focus == Focus::Command)
    {
        if (ch == 27 || ch == 3)
        {
            (void)dispatch_resolved(ch, InputContext::CommandLine);
            return;
        }
        if (ui_is_enter(ch))
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
