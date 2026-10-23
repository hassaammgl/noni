#include "ui_impl.hpp"

void UI::register_nav_actions()
{
    commands.register_command("editor.action.undo", [this]() {
        if (focus != Focus::Editor || !core.buffers().has_tabs())
            return;
        if (!editor.undo())
            Messages::info("Nothing to undo");
    });

    commands.register_command("editor.action.goToFileStart", [this]() {
        if (focus != Focus::Editor || !core.buffers().has_tabs())
            return;
        editor.set_cursor_position(0, 0);
    });

    commands.register_command("editor.action.redo", [this]() {
        if (focus != Focus::Editor || !core.buffers().has_tabs())
            return;
        if (!editor.redo())
            Messages::info("Nothing to redo");
    });

    commands.register_command(Commands::SplitVertical, [this]() {
        if (core.split_vertical())
            Messages::info("Vertical split");
    });
    commands.register_command(Commands::SplitHorizontal, [this]() {
        if (core.split_horizontal())
            Messages::info("Horizontal split");
    });
    commands.register_command(Commands::CloseWindow, [this]() {
        if (!core.close_window())
            Messages::info("No split to close");
    });
    commands.register_command(Commands::FocusLeft, [this]() { core.focus_left(); });
    commands.register_command(Commands::FocusRight, [this]() { core.focus_right(); });
    commands.register_command(Commands::FocusUp, [this]() { core.focus_up(); });
    commands.register_command(Commands::FocusDown, [this]() { core.focus_down(); });
    commands.register_command(Commands::ResizeLeft, [this]() { core.resize_left(); });
    commands.register_command(Commands::ResizeRight, [this]() { core.resize_right(); });
    commands.register_command(Commands::ResizeUp, [this]() { core.resize_up(); });
    commands.register_command(Commands::ResizeDown, [this]() { core.resize_down(); });

    commands.register_command(Commands::SearchNext, [this]() {
        const std::string err = core.search_next(false);
        if (!err.empty())
            Messages::info(err);
    });
    commands.register_command(Commands::SearchPrevious, [this]() {
        const std::string err = core.search_next(true);
        if (!err.empty())
            Messages::info(err);
    });
    commands.register_command(Commands::SearchReplace, [this]() {
        // Replace current with empty unless replace text from panel; buffer search uses "".
        // Prefer panel replace text when search panel has one; else no-op message.
        const std::string &r = search_panel.get_replace();
        const std::string err = core.replace_current(r);
        if (!err.empty())
            Messages::warning(err);
        else
            Messages::info("Replaced match");
    });
    commands.register_command(Commands::SearchReplaceAll, [this]() {
        if (focus == Focus::Search)
        {
            apply_replace_all();
            return;
        }
        const std::string &r = search_panel.get_replace();
        const std::string err = core.replace_all(r);
        if (!err.empty())
            Messages::warning(err);
        else
            Messages::info("Replaced all matches in buffer");
    });

    commands.register_command(Commands::ScmRefresh, [this]() {
        refresh_scm(project_root());
        Messages::info("SCM refresh requested");
    });

    commands.register_command(Commands::ScmShowStatus, [this]() {
        sync_scm_ui();
        const auto snap = core.scm().snapshot();
        if (!snap.is_repo)
        {
            Messages::info("Not a git repository");
            return;
        }
        std::string msg = std::format("Git {} · {} files", snap.branch, snap.files.size());
        if (snap.ahead >= 0 || snap.behind >= 0)
            msg += std::format(" · ↑{} ↓{}", std::max(0, snap.ahead), std::max(0, snap.behind));
        if (Buffer *b = core.active_buffer())
        {
            if (auto st = core.scm().status_for(b->get_buffer_path()))
                msg += std::format(" · [{}{}]", st->xy[0], st->xy[1]);
            else
                msg += " · [clean]";
        }
        Messages::info(msg);
    });

    commands.register_command(Commands::ScmStage, [this]() { scm_stage_active(); });
    commands.register_command(Commands::ScmUnstage, [this]() { scm_unstage_active(); });
    commands.register_command(Commands::ScmDiscard, [this]() { scm_discard_active_confirm(); });
    commands.register_command(Commands::ScmShowDiff, [this]() { scm_show_diff_summary(); });
    commands.register_command(Commands::ScmRefreshDiff, [this]() {
        scm_diff_path_.clear();
        scm_diff_lines_ = -1;
        sync_scm_diff();
        Messages::info("Diff refresh requested");
    });

    commands.register_command(Commands::LspShowStatus, [this]() {
        core.lsp().pump();
        Messages::info(core.lsp().status_summary());
    });

    commands.register_command(Commands::LspInstall, [this]() {
        open_lsp_install_picker();
    });

    commands.register_command(Commands::LspRestart, [this]() {
        core.lsp().shutdown_all();
        core.lsp().set_workspace_root(project_root());
        if (Buffer *b = core.active_buffer())
            core.attach_lsp_document(*b);
        Messages::info("LSP restarted");
    });

    commands.register_command(Commands::TriggerSuggest, [this]() {
        trigger_completion();
    });

    commands.register_command(Commands::GotoDefinition, [this]() { request_lsp_definition(); });
    commands.register_command(Commands::GotoDeclaration, [this]() { request_lsp_declaration(); });
    commands.register_command(Commands::GotoTypeDefinition, [this]() { request_lsp_type_definition(); });
    commands.register_command(Commands::FindReferences, [this]() { request_lsp_references(); });
    commands.register_command(Commands::DocumentSymbols, [this]() { request_lsp_document_symbols(); });
    commands.register_command(Commands::WorkspaceSymbols, [this]() {
        request_lsp_workspace_symbols_prompt();
    });
    commands.register_command(Commands::RenameSymbol, [this]() { request_lsp_rename_prompt(); });
    commands.register_command(Commands::CodeAction, [this]() { request_lsp_code_actions(); });

    commands.register_command(Commands::SessionSave, [this]() {
        save_session();
        Messages::info("Session saved");
    });
    commands.register_command(Commands::SessionRestore, [this]() {
        if (restore_session_if_available(true))
        {
            sync_active_tab();
            Messages::info("Session restored");
        }
        else
            Messages::warning("No session to restore");
    });

    commands.register_command("extension.showStatus", [this]() {
        Messages::info(extensions.status_summary());
    });
}
