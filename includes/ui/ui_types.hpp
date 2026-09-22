#pragma once

struct Dimentions
{
    int height;
    int width;
};

enum class Focus
{
    Editor,
    Sidebar,
    Command,
    Messages,
    FileSearch,
    BufferSearch,
    LspPicker,
    Confirm,
    Prompt,
    Terminal,
    Search,
    Completion,
};

enum class SideView
{
    Explorer,
    Search,
};

enum class ConfirmIntent
{
    None,
    CloseTab,
    Quit,
    DeletePath,
    DiscardGit,
    RecoverBuffer,
};

enum class PromptIntent
{
    None,
    AddFile,
    AddFolder,
    Rename,
    OpenWorkspace,
    RenameSymbol,
    WorkspaceSymbolQuery,
};
