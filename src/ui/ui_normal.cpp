#include "ui_impl.hpp"

void UI::return_to_normal()
{
    keys.clear_chord();

    if (command_line.is_active())
    {
        command_line.clear_input();
        command_line.close();
    }

    if (messages_panel.is_active())
        messages_panel.close();

    if (file_picker.is_active())
        file_picker.close();

    if (buffer_picker.is_active())
        buffer_picker.close();

    if (lsp_picker.is_active())
        lsp_picker.close();

    if (completion_picker.is_active())
        completion_picker.close();
    core.lsp().cancel_pending();

    if (confirm_prompt.is_active())
    {
        confirm_prompt.close();
        confirm_intent = ConfirmIntent::None;
        pending_delete_path.clear();
    }

    if (input_prompt.is_active())
    {
        input_prompt.close();
        prompt_intent = PromptIntent::None;
    }

    terminal.set_focused(false);
    focus = Focus::Editor;
    editor.enter_normal_mode();
    resize();
}
