#pragma once

#include <string>

// Stable command identity — independent of physical keys.
using CommandId = std::string;

namespace Commands
{
    // Existing workbench / noni IDs preserved for config.json compatibility.
    inline constexpr const char *ModeNormal = "noni.mode.normal";
    inline constexpr const char *FilesSave = "workbench.action.files.save";
    inline constexpr const char *CloseActiveEditor = "workbench.action.closeActiveEditor";
    inline constexpr const char *NextEditor = "workbench.action.nextEditor";
    inline constexpr const char *PreviousEditor = "workbench.action.previousEditor";
    inline constexpr const char *ToggleSidebarFocus = "noni.focus.toggleSidebar";
    inline constexpr const char *FocusSidebar = "noni.focus.sidebar";
    inline constexpr const char *FocusEditor = "noni.focus.editor";
    inline constexpr const char *ToggleSidebar = "workbench.action.toggleSidebarVisibility";
    inline constexpr const char *CloseSidebar = "workbench.action.closeSidebar";
    inline constexpr const char *ViewExplorer = "workbench.view.explorer";
    inline constexpr const char *ViewSearch = "workbench.view.search";
    inline constexpr const char *FindInFiles = "workbench.action.findInFiles";
    inline constexpr const char *CommandOpen = "noni.command.open";
    inline constexpr const char *Quit = "workbench.action.quit";
    inline constexpr const char *QuickOpen = "workbench.action.quickOpen";
    inline constexpr const char *SearchFiles = "noni.search.files";
    inline constexpr const char *TerminalToggle = "workbench.action.terminal.toggle";
    inline constexpr const char *TerminalFocus = "workbench.action.terminal.focus";
    inline constexpr const char *ClipboardPaste = "editor.action.clipboardPasteAction";
    inline constexpr const char *Undo = "editor.action.undo";
    inline constexpr const char *Redo = "editor.action.redo";

    // P8 splits
    inline constexpr const char *SplitVertical = "workbench.action.splitEditorRight";
    inline constexpr const char *SplitHorizontal = "workbench.action.splitEditorDown";
    inline constexpr const char *CloseWindow = "workbench.action.closeActiveEditorGroup";
    inline constexpr const char *FocusLeft = "workbench.action.focusLeftGroup";
    inline constexpr const char *FocusRight = "workbench.action.focusRightGroup";
    inline constexpr const char *FocusUp = "workbench.action.focusAboveGroup";
    inline constexpr const char *FocusDown = "workbench.action.focusBelowGroup";
    inline constexpr const char *ResizeLeft = "workbench.action.decreaseViewWidth";
    inline constexpr const char *ResizeRight = "workbench.action.increaseViewWidth";
    inline constexpr const char *ResizeUp = "workbench.action.decreaseViewHeight";
    inline constexpr const char *ResizeDown = "workbench.action.increaseViewHeight";

    // P9 search
    inline constexpr const char *SearchNext = "editor.action.nextMatchFindAction";
    inline constexpr const char *SearchPrevious = "editor.action.previousMatchFindAction";
    inline constexpr const char *SearchReplace = "editor.action.replaceOne";
    inline constexpr const char *SearchReplaceAll = "editor.action.replaceAll";

    // P12 SCM
    inline constexpr const char *ScmRefresh = "git.refresh";
    inline constexpr const char *ScmShowStatus = "git.showStatus";

    // P19 Advanced Git
    inline constexpr const char *ScmStage = "git.stage";
    inline constexpr const char *ScmUnstage = "git.unstage";
    inline constexpr const char *ScmDiscard = "git.discard";
    inline constexpr const char *ScmShowDiff = "git.showDiff";
    inline constexpr const char *ScmRefreshDiff = "git.refreshDiff";

    // P13 LSP
    inline constexpr const char *LspRestart = "lsp.restart";
    inline constexpr const char *LspShowStatus = "lsp.showStatus";
    inline constexpr const char *LspInstall = "lsp.installServers";

    // P14 completion
    inline constexpr const char *TriggerSuggest = "editor.action.triggerSuggest";

    // P15 terminal
    inline constexpr const char *TerminalClear = "workbench.action.terminal.clear";
    inline constexpr const char *TerminalScrollUp = "workbench.action.terminal.scrollUp";
    inline constexpr const char *TerminalScrollDown = "workbench.action.terminal.scrollDown";
    inline constexpr const char *TerminalKill = "workbench.action.terminal.kill";

    // P17 workspace / navigation
    inline constexpr const char *OpenWorkspace = "workbench.action.openWorkspace";
    inline constexpr const char *ShowAllEditors = "workbench.action.showAllEditors";
    inline constexpr const char *RevealInExplorer = "noni.explorer.revealActive";

    // P18 Advanced LSP
    inline constexpr const char *GotoDefinition = "editor.action.revealDefinition";
    inline constexpr const char *GotoDeclaration = "editor.action.revealDeclaration";
    inline constexpr const char *GotoTypeDefinition = "editor.action.goToTypeDefinition";
    inline constexpr const char *FindReferences = "editor.action.goToReferences";
    inline constexpr const char *DocumentSymbols = "workbench.action.gotoSymbol";
    inline constexpr const char *WorkspaceSymbols = "workbench.action.showAllSymbols";
    inline constexpr const char *RenameSymbol = "editor.action.rename";
    inline constexpr const char *CodeAction = "editor.action.quickFix";

    // P20 session
    inline constexpr const char *SessionSave = "workbench.action.saveSession";
    inline constexpr const char *SessionRestore = "workbench.action.restoreSession";
}

enum class InputContext
{
    EditorNormal,
    EditorInsert,
    EditorVisual,
    Sidebar,
    SearchText,     // typing in search query/replace
    SearchResults,  // navigating results (leader OK)
    CommandLine,
    PromptInput,
    Picker,
    Terminal,
    Confirm,
    Messages,
};

// Whether the resolver may consume Space as a leader chord prefix.
inline bool context_allows_leader(InputContext ctx)
{
    switch (ctx)
    {
    case InputContext::EditorNormal:
    case InputContext::EditorVisual:
    case InputContext::Sidebar:
    case InputContext::SearchResults:
        return true;
    default:
        return false;
    }
}

// Whether unresolved keys should fall through to a modal/literal handler.
inline bool context_is_literal(InputContext ctx)
{
    switch (ctx)
    {
    case InputContext::CommandLine:
    case InputContext::PromptInput:
    case InputContext::Picker:
    case InputContext::Terminal:
    case InputContext::Confirm:
    case InputContext::SearchText:
    case InputContext::EditorInsert:
        return true;
    default:
        return false;
    }
}

enum class ResolveStatus
{
    Unmatched, // no binding; pass to component/editor
    Prefix,    // chord in progress; consume key
    Matched,   // full binding → execute command_id
};

struct ResolveResult
{
    ResolveStatus status = ResolveStatus::Unmatched;
    CommandId command_id;
};
