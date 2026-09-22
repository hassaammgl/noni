#include "ui_impl.hpp"

void UI::refresh_scm(const fs::path &hint)
{
    const fs::path root = Workspace::detect_root(hint.empty() ? project_root() : hint);
    if (!core.workspace().has_root() || core.workspace().root() != root)
        core.workspace().set_root(root);
    core.scm().set_workspace_root(root);
    core.scm().request_refresh();
    scm_gen_seen_ = 0;
    scm_diff_path_.clear();
    scm_diff_lines_ = -1;
    scm_diff_gen_seen_ = 0;
    core.lsp().set_workspace_root(root);
    GrammarInstaller::set_workspace_root(root);
}

void UI::sync_scm_diff()
{
    Buffer *b = core.active_buffer();
    if (!b)
        return;
    const fs::path path = b->get_buffer_path();
    if (path.empty())
        return;

    const int lines = static_cast<int>(b->lines().size());
    if (path != scm_diff_path_ || lines != scm_diff_lines_ ||
        core.scm().diff_generation() != scm_diff_gen_seen_)
    {
        // Request when path/lines changed; generation bump means cache updated.
        if (path != scm_diff_path_ || lines != scm_diff_lines_)
        {
            scm_diff_path_ = path;
            scm_diff_lines_ = lines;
            core.scm().request_file_diff(path, lines);
        }
        scm_diff_gen_seen_ = core.scm().diff_generation();
    }
}

void UI::scm_stage_active()
{
    Buffer *b = core.active_buffer();
    if (!b || b->get_buffer_path().empty())
    {
        Messages::warning("No file to stage");
        return;
    }
    std::string err;
    if (!core.scm().stage(b->get_buffer_path(), &err))
    {
        Messages::error(err.empty() ? "Stage failed" : err);
        return;
    }
    scm_diff_path_.clear();
    sync_scm_diff();
    Messages::info(std::format("Staged {}", b->get_buffer_path().filename().string()));
}

void UI::scm_unstage_active()
{
    Buffer *b = core.active_buffer();
    if (!b || b->get_buffer_path().empty())
    {
        Messages::warning("No file to unstage");
        return;
    }
    std::string err;
    if (!core.scm().unstage(b->get_buffer_path(), &err))
    {
        Messages::error(err.empty() ? "Unstage failed" : err);
        return;
    }
    scm_diff_path_.clear();
    sync_scm_diff();
    Messages::info(std::format("Unstaged {}", b->get_buffer_path().filename().string()));
}

void UI::scm_discard_active_confirm()
{
    Buffer *b = core.active_buffer();
    if (!b || b->get_buffer_path().empty())
    {
        Messages::warning("No file to discard");
        return;
    }
    if (b->is_dirty())
    {
        Messages::error("Buffer has unsaved changes — save or discard buffer edits first");
        return;
    }
    if (command_line.is_active())
        command_line.close();
    if (input_prompt.is_active())
        input_prompt.close();
    confirm_intent = ConfirmIntent::DiscardGit;
    confirm_prompt.open(std::format(
        "Discard worktree changes to {}? [y/n/esc]",
        b->get_buffer_path().filename().string()));
    focus = Focus::Confirm;
    resize();
}

void UI::resolve_discard_confirm(ConfirmChoice choice)
{
    confirm_prompt.close();
    confirm_intent = ConfirmIntent::None;
    focus = Focus::Editor;
    resize();

    if (choice != ConfirmChoice::Yes)
    {
        Messages::info("Discard cancelled");
        return;
    }

    Buffer *b = core.active_buffer();
    if (!b || b->get_buffer_path().empty())
        return;
    if (b->is_dirty())
    {
        Messages::error("Buffer became dirty — discard aborted");
        return;
    }

    const fs::path path = b->get_buffer_path();
    std::string err;
    if (!core.scm().discard_worktree(path, &err))
    {
        Messages::error(err.empty() ? "Discard failed" : err);
        return;
    }

    // Reload from disk (P0-safe load); preserve undo? load() clears history — OK for discard.
    b->load();
    core.attach_lsp_document(*b);
    scm_diff_path_.clear();
    sync_active_tab();
    sync_scm_diff();
    Messages::info(std::format("Discarded changes: {}", path.filename().string()));
}

void UI::scm_show_diff_summary()
{
    sync_scm_diff();
    Buffer *b = core.active_buffer();
    if (!b || b->get_buffer_path().empty())
    {
        Messages::info("No file");
        return;
    }
    auto diff = core.scm().file_diff(b->get_buffer_path());
    if (!diff)
    {
        Messages::info("Diff not ready yet — try again");
        core.scm().request_file_diff(b->get_buffer_path(), static_cast<int>(b->lines().size()));
        return;
    }
    if (!diff->ok)
    {
        Messages::warning(diff->error.empty() ? "Diff unavailable" : diff->error);
        return;
    }
    int added = 0, modified = 0, deleted = 0;
    for (const auto &[line, ch] : diff->lines)
    {
        (void)line;
        switch (ch)
        {
        case ScmLineChange::Added:
            ++added;
            break;
        case ScmLineChange::Modified:
            ++modified;
            break;
        case ScmLineChange::Deleted:
            ++deleted;
            break;
        default:
            break;
        }
    }
    Messages::info(std::format(
        "Diff {}: +{} ~{} -{} ({} hunks){}",
        b->get_buffer_path().filename().string(),
        added,
        modified,
        deleted,
        diff->hunks.size(),
        diff->is_untracked ? " [untracked]" : ""));
}
