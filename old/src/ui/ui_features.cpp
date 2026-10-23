#include "ui_impl.hpp"

#include <thread>

bool UI::write_buffer_to_disk()
{
    if (editor.get_buffer().get_buffer_path().empty())
    {
        Messages::error("E32: No file name (use :w <path>)");
        return false;
    }
    if (editor.get_buffer().has_load_error())
    {
        Messages::error(std::format(
            "E13: Cannot save — file was not loaded ({})",
            editor.get_buffer().get_load_error()));
        return false;
    }
    if (!editor.get_buffer().save())
    {
        Messages::error("E212: Can't open file for writing");
        return false;
    }
    statusbar.set_filename(core.buffers().active().display_name());
    Messages::info(std::format("\"{}\" written", core.buffers().active().display_name()));
    clear_recovery_for_buffer(editor.get_buffer());
    core.scm().request_refresh();
    core.lsp().notify_save(editor.get_buffer());
    core.notify_buffer_saved(editor.get_buffer());
    dirty_clock_armed_ = false;
    return true;
}

void UI::apply_lsp_format_edits(std::vector<LspTextEdit> edits, bool utf16_pending)
{
    Buffer *b = core.active_buffer();
    if (!b || edits.empty())
        return;
    LspWorkspaceEdit we;
    we.utf16_pending = utf16_pending;
    we.changes[LspService::path_to_uri(b->get_buffer_path())] = std::move(edits);
    apply_lsp_workspace_edit(std::move(we));
}

void UI::request_lsp_format()
{
    Buffer *b = core.active_buffer();
    if (!b)
        return;
    if (!core.lsp().session_supports_formatting(*b))
    {
        Messages::info("Format unavailable (no LSP formatting)");
        return;
    }
    if (core.lsp().request_formatting(*b) <= 0)
        Messages::info("Format request failed");
    else
        Messages::info("Formatting…");
}

void UI::request_lsp_hover()
{
    Buffer *b = core.active_buffer();
    if (!b)
        return;
    if (!core.lsp().session_supports_hover(*b))
    {
        Messages::info("Hover unavailable");
        return;
    }
    const Cursor c = editor.get_cursor();
    if (core.lsp().request_hover(*b, c.line, c.column) <= 0)
        Messages::info("Hover request failed");
}

void UI::open_diagnostics_picker()
{
    Buffer *b = core.active_buffer();
    if (!b)
        return;
    std::vector<LspLocation> locs;
    for (const auto &d : b->diagnostics().items)
    {
        LspLocation loc;
        loc.uri = LspService::path_to_uri(b->get_buffer_path());
        loc.start = d.start;
        loc.end = d.end;
        loc.display = d.message;
        if (!d.source.empty())
            loc.display = d.source + ": " + loc.display;
        locs.push_back(std::move(loc));
    }
    if (locs.empty())
    {
        Messages::info("No diagnostics");
        return;
    }
    open_lsp_locations("diagnostics", std::move(locs));
    lsp_picker_kind_ = LspPickerKind::Diagnostics;
}

void UI::tick_autosave()
{
    if (config.autosave.mode != "afterDelay")
        return;
    Buffer *b = core.active_buffer();
    if (!b || b->get_buffer_path().empty() || !b->is_dirty())
    {
        dirty_clock_armed_ = false;
        return;
    }
    if (b->disk_changed())
    {
        if (!b->external_change_notified())
        {
            Messages::warning("Autosave skipped: file changed on disk");
            b->mark_external_change_notified();
        }
        return;
    }
    const auto now = std::chrono::steady_clock::now();
    if (!dirty_clock_armed_)
    {
        dirty_since_ = now;
        dirty_clock_armed_ = true;
        return;
    }
    if (now - dirty_since_ < std::chrono::milliseconds(config.autosave.delay_ms))
        return;
    // Autosave never runs formatOnSave.
    (void)write_buffer_to_disk();
}
