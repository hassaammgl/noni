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
#include <commands/command_registry.hpp>
#include <extensions/extension_manager.hpp>
#include <lsp/lsp_models.hpp>
#include <utils/fs_watcher.hpp>
#include <workspace/recovery.hpp>
#include <workspace/session.hpp>
#include <filesystem>
#include <chrono>
#include <cstdint>
#include <optional>
#include <vector>

namespace fs = std::filesystem;

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

    void init();
    void load_config();
    void register_actions();
    void resize();
    void render();
    void handle_inputs();
    void update_statusbar_mode();
    void update_cursor_visibility();
    void execute_command();
    void sync_active_tab();
    bool close_active_tab(bool force);
    void return_to_normal();
    void open_save_confirm(ConfirmIntent intent);
    void resolve_save_confirm(ConfirmChoice choice);
    bool save_active_buffer();
    void finish_close_or_quit(ConfirmIntent intent, bool force);
    void open_sidebar_prompt(PromptIntent intent);
    void resolve_sidebar_prompt();
    void open_delete_confirm();
    void resolve_delete_confirm(ConfirmChoice choice);
    void toggle_terminal();
    void open_terminal(bool focus_terminal = true);
    void close_terminal();
    void open_search_view(bool focus_search = true);
    void open_explorer_view(bool focus_explorer = true);
    void apply_replace_all();
    int effective_sidebar_width() const;
    std::string when_context() const;
    void set_sidebar_visible(bool visible);
    void toggle_sidebar();
    void open_sidebar(bool focus_sidebar = true);
    void close_sidebar();
    void open_file_search();
    void close_file_search(bool open_selected);
    void open_buffer_search();
    void close_buffer_search(bool open_selected);
    void apply_workspace_root(const fs::path &hint, bool announce = true);
    void open_workspace_prompt();
    void remap_buffer_path(const fs::path &from, const fs::path &to);
    void close_buffers_under(const fs::path &path);
    void note_opened_file(const fs::path &path);
    void trigger_completion();
    void close_completion(bool accept);
    void poll_completion_result();
    void sync_scm_ui();
    void refresh_scm(const fs::path &hint);
    void sync_scm_diff();
    void scm_stage_active();
    void scm_unstage_active();
    void scm_discard_active_confirm();
    void scm_show_diff_summary();
    void resolve_discard_confirm(ConfirmChoice choice);
    void sync_fs_watches();
    void poll_fs_events();
    void sync_messages_echo();

    SessionState capture_session_state() const;
    void save_session();
    bool restore_session_if_available(bool allow_replace_tabs);
    void queue_recovery_prompts();
    void open_recovery_confirm();
    void resolve_recovery_confirm(ConfirmChoice choice);
    void tick_recovery_snapshots();
    void clear_recovery_for_buffer(const Buffer &buffer);

    void goto_lsp_location(const LspLocation &loc);
    void apply_lsp_workspace_edit(LspWorkspaceEdit edit);
    void open_lsp_locations(std::string title, std::vector<LspLocation> locs);
    void open_lsp_symbols(std::string title, std::vector<LspSymbol> syms);
    void open_lsp_actions(std::vector<LspCodeAction> actions);
    void close_lsp_picker(bool accept);
    void poll_lsp_results();
    void request_lsp_definition();
    void request_lsp_declaration();
    void request_lsp_type_definition();
    void request_lsp_references();
    void request_lsp_document_symbols();
    void request_lsp_workspace_symbols_prompt();
    void request_lsp_rename_prompt();
    void request_lsp_code_actions();
    fs::path project_root() const;
    // Prefer Workspace::detect_root; kept for call sites.
    fs::path find_workspace_root(const fs::path &hint) const;

    std::uint64_t scm_gen_seen_ = 0;
    std::uint64_t scm_diff_gen_seen_ = 0;
    fs::path scm_diff_path_;
    int scm_diff_lines_ = -1;
    // Cached gutter diff — avoid copying ScmFileDiff every frame.
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
