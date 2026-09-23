#pragma once

#include <ui/ui.hpp>
#include <utils/async.hpp>
#include <ui/theme.hpp>
#include <editor/ex_commands.hpp>
#include <editor/buffer_search.hpp>
#include <commands/command.hpp>
#include <scm/scm_git.hpp>
#include <lsp/lsp_service.hpp>
#include <extensions/builtin_hello.hpp>
#include <help/help_docs.hpp>
#include <lsp/lsp_edits.hpp>
#include <workspace/workspace.hpp>
#include <workspace/session.hpp>
#include <workspace/recovery.hpp>
#include <utils/fs_watcher.hpp>
#include <syntax/grammar_installer.hpp>
#include <lsp/lsp_installer.hpp>
#include <utils/logger.hpp>
#include <utils/messages.hpp>
#include <utils/str.hpp>
#include <utils/text_metrics.hpp>
#include <utils/fs.hpp>
#include <algorithm>
#include <cstdint>
#include <csignal>
#include <chrono>
#include <fstream>
#include <locale>
#include <cstdlib>
#include <format>
#include <map>
#include <regex>
#include <termios.h>
#include <unistd.h>
#include <vector>

inline bool ui_is_enter(int ch)
{
    return ch == '\n' || ch == '\r' || ch == KEY_ENTER;
}

inline InputContext ui_input_context_for(Focus focus, EditorMode mode, bool search_in_text_field)
{
    switch (focus)
    {
    case Focus::Command:
        return InputContext::CommandLine;
    case Focus::Prompt:
        return InputContext::PromptInput;
    case Focus::FileSearch:
    case Focus::BufferSearch:
    case Focus::LspPicker:
    case Focus::Completion:
        return InputContext::Picker;
    case Focus::Terminal:
        return InputContext::Terminal;
    case Focus::Confirm:
        return InputContext::Confirm;
    case Focus::Messages:
        return InputContext::Messages;
    case Focus::Sidebar:
        return InputContext::Sidebar;
    case Focus::Search:
        return search_in_text_field ? InputContext::SearchText : InputContext::SearchResults;
    case Focus::Editor:
    default:
        if (mode == EditorMode::Insert)
            return InputContext::EditorInsert;
        if (mode == EditorMode::Visual || mode == EditorMode::VisualLine)
            return InputContext::EditorVisual;
        return InputContext::EditorNormal;
    }
}
