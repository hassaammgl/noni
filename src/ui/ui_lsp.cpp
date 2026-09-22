#include "ui_impl.hpp"

void UI::goto_lsp_location(const LspLocation &loc)
{
    const fs::path path = LspService::uri_to_path(loc.uri);
    if (path.empty())
    {
        Messages::warning("LSP location has no path");
        return;
    }

    editor.record_jump_from_here();
    core.buffers().open_file(path);
    sync_active_tab();
    note_opened_file(path);

    Buffer &buf = editor.get_buffer();
    const Cursor byte_pos = LspService::lsp_pos_to_cursor(
        buf.lines(), loc.start.line, loc.start.column);
    editor.set_cursor_position(byte_pos.line, byte_pos.column);
    editor.enter_normal_mode();
    focus = Focus::Editor;
    Messages::info(std::format("→ {}:{}", path.filename().string(), byte_pos.line + 1));
}

void UI::apply_lsp_workspace_edit(LspWorkspaceEdit edit)
{
    const int n = LspEdits::apply_workspace_edit(
        core.buffers(),
        std::move(edit),
        [this](Buffer &b) { core.attach_lsp_document(b); });
    sync_active_tab();
    if (n > 0)
        Messages::info(std::format("Applied workspace edit ({} buffer(s))", n));
    else
        Messages::warning("Workspace edit applied nothing");
}

void UI::open_lsp_locations(std::string title, std::vector<LspLocation> locs)
{
    if (locs.empty())
    {
        Messages::info("No locations");
        return;
    }
    if (locs.size() == 1)
    {
        goto_lsp_location(locs.front());
        return;
    }
    lsp_picker_kind_ = LspPickerKind::Locations;
    lsp_picker.open_locations(std::move(title), std::move(locs));
    focus = Focus::LspPicker;
    resize();
}

void UI::open_lsp_symbols(std::string title, std::vector<LspSymbol> syms)
{
    if (syms.empty())
    {
        Messages::info("No symbols");
        return;
    }
    lsp_picker_kind_ = LspPickerKind::Symbols;
    lsp_picker.open_symbols(std::move(title), std::move(syms));
    focus = Focus::LspPicker;
    resize();
}

void UI::open_lsp_actions(std::vector<LspCodeAction> actions)
{
    if (actions.empty())
    {
        Messages::info("No code actions");
        return;
    }
    lsp_picker_kind_ = LspPickerKind::CodeActions;
    lsp_picker.open_actions("code actions", std::move(actions));
    focus = Focus::LspPicker;
    resize();
}

void UI::open_lsp_install_picker()
{
    std::vector<LspCodeAction> items;
    for (const auto &e : LspInstaller::catalog())
    {
        const auto st = LspInstaller::status(e.binary);
        const fs::path path = LspInstaller::resolve(e.binary);
        LspCodeAction a;
        a.title = std::format(
            "[{}] {}  · {} ({})",
            LspInstaller::status_label(st),
            e.binary,
            e.via,
            e.package);
        if (!path.empty())
            a.title += std::format("  → {}", path.string());
        a.kind = "install";
        a.has_command = true;
        a.command = e.binary;
        a.is_preferred = st != LspInstaller::Status::Ready;
        items.push_back(std::move(a));
    }
    if (items.empty())
    {
        Messages::info("No LSP install recipes");
        return;
    }
    lsp_picker_kind_ = LspPickerKind::InstallServers;
    lsp_picker.open_actions("LSP servers (Enter = install)", std::move(items));
    focus = Focus::LspPicker;
    resize();
}

void UI::ex_lsp()
{
    open_lsp_install_picker();
}

void UI::close_lsp_picker(bool accept)
{
    if (!lsp_picker.is_active())
    {
        focus = Focus::Editor;
        return;
    }

    if (!accept)
    {
        lsp_picker.close();
        focus = Focus::Editor;
        resize();
        return;
    }

    if (lsp_picker_kind_ == LspPickerKind::Locations)
    {
        LspLocation loc;
        if (lsp_picker.take_location(loc))
            goto_lsp_location(loc);
        else
            lsp_picker.close();
        focus = Focus::Editor;
        resize();
    }
    else if (lsp_picker_kind_ == LspPickerKind::Symbols)
    {
        LspSymbol sym;
        if (lsp_picker.take_symbol(sym))
            goto_lsp_location(sym.location);
        else
            lsp_picker.close();
        focus = Focus::Editor;
        resize();
    }
    else if (lsp_picker_kind_ == LspPickerKind::InstallServers)
    {
        LspCodeAction act;
        if (lsp_picker.take_action(act) && act.has_command && !act.command.empty())
        {
            if (!LspInstaller::resolve(act.command).empty())
                Messages::info(std::format("LSP `{}` already installed", act.command));
            else
                LspInstaller::request(act.command, true);
        }
        else
        {
            lsp_picker.close();
        }
        focus = Focus::Editor;
        resize();
    }
    else
    {
        LspCodeAction act;
        if (lsp_picker.take_action(act))
        {
            if (act.has_edit)
                apply_lsp_workspace_edit(std::move(act.edit));
            else if (act.has_command)
                Messages::warning(std::format(
                    "Code action command not executed: {}", act.command));
            else
                Messages::info(act.title);
        }
        else
        {
            lsp_picker.close();
        }
        focus = Focus::Editor;
        resize();
    }
}
