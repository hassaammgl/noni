#include "ui_impl.hpp"

void UI::trigger_completion()
{
    if (!core.buffers().has_tabs())
        return;
    if (completion_picker.is_active())
        close_completion(false);

    Buffer &buf = editor.get_buffer();
    const Cursor c = editor.get_cursor();
    core.lsp().cancel_completion();
    const int id = core.lsp().request_completion(buf, c.line, c.column);
    if (id < 0)
    {
        Messages::info("LSP still starting — try Ctrl+Space again in a second");
        return;
    }
    if (id == 0)
    {
        Messages::info("Completion unavailable (no LSP for this buffer)");
        return;
    }
    Messages::info("Completing…");
}

void UI::close_completion(bool accept)
{
    CompletionItem item;
    const bool took = accept && completion_picker.take_selection(item);
    if (!took)
        completion_picker.close();
    core.lsp().cancel_completion();
    focus = Focus::Editor;
    keys.clear_chord();
    resize();

    if (took)
    {
        if (editor.apply_completion(item))
            Messages::info(std::format("Inserted {}", item.label));
        else
            Messages::warning("Failed to insert completion");
    }
}

void UI::poll_completion_result()
{
    auto result = core.lsp().take_completion_result();
    if (!result)
        return;

    if (result->items.empty())
    {
        Messages::info("No completions");
        return;
    }

    if (file_picker.is_active())
        file_picker.close();
    if (command_line.is_active())
    {
        command_line.clear_input();
        command_line.close();
    }

    completion_picker.open(std::move(*result));
    focus = Focus::Completion;
    keys.clear_chord();
    resize();
}
