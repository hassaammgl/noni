#include "ui_impl.hpp"

void UI::poll_lsp_results()
{
    if (auto edit = core.lsp().take_server_apply_edit())
        apply_lsp_workspace_edit(std::move(*edit));

    if (auto locs = core.lsp().take_location_result())
    {
        const char *title = "locations";
        switch (locs->kind)
        {
        case LspLocationList::Kind::Declaration:
            title = "declarations";
            break;
        case LspLocationList::Kind::TypeDefinition:
            title = "type definitions";
            break;
        case LspLocationList::Kind::References:
            title = "references";
            break;
        default:
            title = "definitions";
            break;
        }
        open_lsp_locations(title, std::move(locs->items));
    }

    if (auto syms = core.lsp().take_symbol_result())
    {
        open_lsp_symbols(
            syms->workspace ? "workspace symbols" : "document symbols",
            std::move(syms->items));
    }

    if (auto ren = core.lsp().take_rename_result())
    {
        if (ren->edit.changes.empty())
            Messages::warning("Rename produced no edits");
        else
            apply_lsp_workspace_edit(std::move(ren->edit));
    }

    if (auto acts = core.lsp().take_code_action_result())
        open_lsp_actions(std::move(acts->items));
}

void UI::request_lsp_definition()
{
    Buffer *b = core.active_buffer();
    if (!b)
        return;
    const Cursor c = editor.get_cursor();
    if (core.lsp().request_definition(*b, c.line, c.column) <= 0)
        Messages::info("Definition unavailable (no LSP)");
    else
        Messages::info("Finding definition…");
}

void UI::request_lsp_declaration()
{
    Buffer *b = core.active_buffer();
    if (!b)
        return;
    const Cursor c = editor.get_cursor();
    if (core.lsp().request_declaration(*b, c.line, c.column) <= 0)
        Messages::info("Declaration unavailable (no LSP)");
    else
        Messages::info("Finding declaration…");
}

void UI::request_lsp_type_definition()
{
    Buffer *b = core.active_buffer();
    if (!b)
        return;
    const Cursor c = editor.get_cursor();
    if (core.lsp().request_type_definition(*b, c.line, c.column) <= 0)
        Messages::info("Type definition unavailable (no LSP)");
    else
        Messages::info("Finding type definition…");
}

void UI::request_lsp_references()
{
    Buffer *b = core.active_buffer();
    if (!b)
        return;
    const Cursor c = editor.get_cursor();
    if (core.lsp().request_references(*b, c.line, c.column) <= 0)
        Messages::info("References unavailable (no LSP)");
    else
        Messages::info("Finding references…");
}

void UI::request_lsp_document_symbols()
{
    Buffer *b = core.active_buffer();
    if (!b)
        return;
    if (core.lsp().request_document_symbols(*b) <= 0)
        Messages::info("Document symbols unavailable (no LSP)");
    else
        Messages::info("Loading symbols…");
}

void UI::request_lsp_workspace_symbols_prompt()
{
    if (command_line.is_active())
        command_line.close();
    if (lsp_picker.is_active())
        lsp_picker.close();
    keys.clear_chord();
    prompt_intent = PromptIntent::WorkspaceSymbolQuery;
    input_prompt.open("Workspace symbol: ");
    focus = Focus::Prompt;
    resize();
}

void UI::request_lsp_rename_prompt()
{
    if (command_line.is_active())
        command_line.close();
    if (lsp_picker.is_active())
        lsp_picker.close();
    keys.clear_chord();
    prompt_intent = PromptIntent::RenameSymbol;
    input_prompt.open("Rename to: ");
    focus = Focus::Prompt;
    resize();
}

void UI::request_lsp_code_actions()
{
    Buffer *b = core.active_buffer();
    if (!b)
        return;
    const Cursor c = editor.get_cursor();
    if (core.lsp().request_code_actions(*b, c, c) <= 0)
        Messages::info("Code actions unavailable (no LSP)");
    else
        Messages::info("Loading code actions…");
}
