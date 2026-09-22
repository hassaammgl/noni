#include "ui_impl.hpp"

void UI::handle_overlay_input(int ch)
{
    if (focus == Focus::FileSearch)
    {
        if (ch == 27 || ch == 3)
        {
            close_file_search(false);
            return;
        }
        if (ch == '\n' || ch == KEY_ENTER)
        {
            close_file_search(true);
            return;
        }
        file_picker.handle_input(ch);
        return;
    }

    if (focus == Focus::BufferSearch)
    {
        if (ch == 27 || ch == 3)
        {
            close_buffer_search(false);
            return;
        }
        if (ch == '\n' || ch == KEY_ENTER)
        {
            close_buffer_search(true);
            return;
        }
        buffer_picker.handle_input(ch);
        return;
    }

    if (focus == Focus::LspPicker)
    {
        if (ch == 27 || ch == 3)
        {
            close_lsp_picker(false);
            return;
        }
        if (ch == '\n' || ch == KEY_ENTER)
        {
            close_lsp_picker(true);
            return;
        }
        lsp_picker.handle_input(ch);
        return;
    }

    if (focus == Focus::Completion)
    {
        if (ch == 27 || ch == 3)
        {
            close_completion(false);
            return;
        }
        if (ch == '\n' || ch == KEY_ENTER || ch == '\t')
        {
            close_completion(true);
            return;
        }
        completion_picker.handle_input(ch);
        return;
    }

    if (focus == Focus::Terminal)
    {
        // Leave: Ctrl+] (29). Esc goes to the PTY (vim/less). ctrl+t toggle is config.
        if (ch == 29)
        {
            return_to_normal();
            return;
        }
        // Esc must not hit the empty-when "escape" → noni.mode.normal binding.
        if (ch != 27 && dispatch_resolved(ch, InputContext::Terminal))
            return;
        terminal.handle_input(ch);
        return;
    }

    if (focus == Focus::Search)
    {
        const InputContext ctx = ui_input_context_for(
            Focus::Search, editor.get_mode(), search_panel.in_text_field());
        if (dispatch_resolved(ch, ctx))
            return;

        const SearchPanelAction action = search_panel.handle_input(ch);
        if (action == SearchPanelAction::FocusEditor)
        {
            return_to_normal();
            return;
        }
        if (action == SearchPanelAction::OpenMatch)
        {
            const TextMatch m = search_panel.selected_match();
            if (!m.path.empty())
            {
                core.buffers().open_file(m.path);
                sync_active_tab();
                editor.set_cursor_position(m.line, m.column);
                editor.enter_normal_mode();
                focus = Focus::Editor;
                keys.clear_chord();
            }
            return;
        }
        if (action == SearchPanelAction::ReplaceAll)
        {
            apply_replace_all();
            return;
        }
        return;
    }

    if (focus == Focus::Confirm)
    {
        const ConfirmChoice choice = confirm_prompt.handle_input(ch);
        if (choice != ConfirmChoice::Pending)
            resolve_save_confirm(choice);
        return;
    }

    if (focus == Focus::Prompt)
    {
        if (ch == '\n' || ch == KEY_ENTER)
        {
            resolve_sidebar_prompt();
            return;
        }
        if (ch == 27 || ch == 3)
        {
            (void)dispatch_resolved(ch, InputContext::PromptInput);
            if (input_prompt.is_active())
            {
                input_prompt.handle_input(ch);
            }
            if (!input_prompt.is_active())
            {
                prompt_intent = PromptIntent::None;
                focus = Focus::Sidebar;
                resize();
                Messages::info("Cancelled");
            }
            return;
        }
        input_prompt.handle_input(ch);
        if (!input_prompt.is_active())
        {
            prompt_intent = PromptIntent::None;
            focus = Focus::Sidebar;
            resize();
            Messages::info("Cancelled");
        }
        return;
    }
}
