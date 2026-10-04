#include "ui_impl.hpp"

void UI::register_keymap_actions()
{
    commands.register_command(Commands::Find, [this]() {
        command_line.open('/');
        focus = Focus::Command;
        keys.clear_chord();
        resize();
    });

    commands.register_command(Commands::GotoLine, [this]() {
        command_line.open(':');
        focus = Focus::Command;
        keys.clear_chord();
        resize();
        Messages::info("Go to line: type number, Enter");
    });

    commands.register_command(Commands::HalfPageDown, [this]() {
        if (focus == Focus::Editor && core.buffers().has_tabs())
            editor.cmd_half_page_down();
    });
    commands.register_command(Commands::HalfPageUp, [this]() {
        if (focus == Focus::Editor && core.buffers().has_tabs())
            editor.cmd_half_page_up();
    });
    commands.register_command(Commands::SelectAll, [this]() {
        if (focus == Focus::Editor && core.buffers().has_tabs())
            editor.cmd_select_all();
    });
    commands.register_command(Commands::DeleteLines, [this]() {
        if (focus == Focus::Editor && core.buffers().has_tabs())
            editor.cmd_delete_line();
    });
    commands.register_command(Commands::ClipboardCopy, [this]() {
        if (focus == Focus::Editor && core.buffers().has_tabs())
            editor.cmd_copy_selection_or_line();
    });
    commands.register_command(Commands::ClipboardCut, [this]() {
        if (focus == Focus::Editor && core.buffers().has_tabs())
            editor.cmd_cut_selection_or_line();
    });
    commands.register_command(Commands::CommentLine, [this]() {
        if (focus == Focus::Editor && core.buffers().has_tabs())
            editor.cmd_toggle_comment();
    });
    commands.register_command(Commands::CopyLinesDown, [this]() {
        if (focus == Focus::Editor && core.buffers().has_tabs())
            editor.cmd_copy_lines_down();
    });
    commands.register_command(Commands::MoveLinesDown, [this]() {
        if (focus == Focus::Editor && core.buffers().has_tabs())
            editor.cmd_move_lines_down();
    });
    commands.register_command(Commands::MoveLinesUp, [this]() {
        if (focus == Focus::Editor && core.buffers().has_tabs())
            editor.cmd_move_lines_up();
    });
    commands.register_command(Commands::FormatDocument, [this]() { request_lsp_format(); });
    commands.register_command(Commands::SearchRecent, [this]() { open_file_search(); });
    commands.register_command(Commands::GrepWord, [this]() {
        Messages::info("Grep word under cursor pending (PHASE 5)");
    });
    commands.register_command(Commands::DiagnosticsList, [this]() { open_diagnostics_picker(); });
    commands.register_command(Commands::GitPreviewHunk, [this]() {
        Messages::info("Git hunk preview pending (PHASE 5)");
    });
}
