#pragma once

#include <ncurses.h>
#include <components/buffer_picker.hpp>
#include <components/commandline.hpp>
#include <components/completion_picker.hpp>
#include <components/confirm_prompt.hpp>
#include <components/editor.hpp>
#include <components/file_picker.hpp>
#include <components/header.hpp>
#include <components/input_prompt.hpp>
#include <components/line_number.hpp>
#include <components/lsp_picker.hpp>
#include <components/messages_panel.hpp>
#include <components/search_panel.hpp>
#include <components/sidebar.hpp>
#include <components/statusbar.hpp>
#include <components/tab_bar.hpp>
#include <components/terminal_panel.hpp>
#include <configs/config.hpp>
#include <configs/keybindings.hpp>
#include <editor/buffer_manager.hpp>
#include <editor/editor_core.hpp>
#include <commands/command.hpp>
#include <commands/command_registry.hpp>
#include <extensions/extension_manager.hpp>
#include <lsp/lsp_models.hpp>
#include <ui/ui_types.hpp>
#include <utils/fs_watcher.hpp>
#include <workspace/recovery.hpp>
#include <workspace/session.hpp>
#include <filesystem>
#include <chrono>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;

class UI
{
private:
    Header header;
    TabBar tab_bar;
    Sidebar sidebar;
    SearchPanel search_panel;
    LineNumber line_number;
    Editor editor;
    MessagesPanel messages_panel;
    FilePicker file_picker;
    BufferPicker buffer_picker;
    LspPicker lsp_picker;
    CompletionPicker completion_picker;
    Statusbar statusbar;
    CommandLine command_line;
    ConfirmPrompt confirm_prompt;
    InputPrompt input_prompt;
    TerminalPanel terminal;
    EditorCore core;
    CommandRegistry commands;
    ExtensionManager extensions;
    AppConfig config;
    KeybindingEngine keys;
    FsWatcher fs_watcher_;
    std::uint64_t messages_echo_seen_ = 0;
    bool fs_explorer_dirty_ = false;
    std::chrono::steady_clock::time_point fs_explorer_refresh_at_{};
    LspPickerKind lsp_picker_kind_ = LspPickerKind::Locations;

    int height;
    int width;

    int sidebar_width = 25;
    int line_number_width = 5;
    int terminal_height = 12;
    bool running = true;
    bool sidebar_visible = true;
    bool terminal_visible = false;
    SideView side_view = SideView::Explorer;
    ConfirmIntent confirm_intent = ConfirmIntent::None;
    PromptIntent prompt_intent = PromptIntent::None;
    fs::path pending_delete_path;

#include <ui/ui_private.hpp>

    std::uint64_t scm_gen_seen_ = 0;
    std::uint64_t scm_diff_gen_seen_ = 0;
    fs::path scm_diff_path_;
    int scm_diff_lines_ = -1;
    std::optional<ScmFileDiff> scm_diff_cache_;
    fs::path scm_diff_cache_path_;
    std::uint64_t scm_diff_cache_gen_ = 0;

    std::vector<RecoveryEntry> pending_recovery_;
    std::chrono::steady_clock::time_point last_recovery_tick_{};
    bool session_restored_ = false;

public:
    Focus focus = Focus::Editor;

    UI(const fs::path file_path);
    ~UI();

    void run();
    void request_quit();

    void ex_quit(bool bang);
    void ex_write(bool bang, const std::string &path);
    void ex_write_quit(bool bang, const std::string &path);
    void ex_edit(bool bang, const std::string &path);
    void ex_bnext();
    void ex_bprevious();
    void ex_bdelete(bool bang);
    void ex_messages();
    void ex_help(const std::string &topic);
    void ex_logs(const std::string &which);
    void ex_lsp();
    void open_lsp_install_picker();
    void ex_sidebar(const std::string &arg);
    void ex_find();
    void ex_buffers();
    void ex_workspace(const std::string &arg);
    void ex_terminal(const std::string &arg);
    void ex_search();
    void ex_undo();
    void ex_redo();

    CommandLine &get_command_line();
    MessagesPanel &get_messages_panel();
    Editor &get_editor();
    BufferManager &get_buffers();

    Dimentions get_editor_dim() const;
};
