#pragma once

#include <ncurses.h>
#include <components/commandline.hpp>
#include <components/completion_picker.hpp>
#include <components/confirm_prompt.hpp>
#include <components/editor.hpp>
#include <components/file_picker.hpp>
#include <components/header.hpp>
#include <components/input_prompt.hpp>
#include <components/line_number.hpp>
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
#include <filesystem>
#include <cstdint>

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
};

enum class PromptIntent
{
    None,
    AddFile,
    AddFolder,
    Rename,
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
    CompletionPicker completion_picker;
    Statusbar statusbar;
    CommandLine command_line;
    ConfirmPrompt confirm_prompt;
    InputPrompt input_prompt;
    TerminalPanel terminal;
    EditorCore core;
    CommandRegistry commands;
    AppConfig config;
    KeybindingEngine keys;

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
    void trigger_completion();
    void close_completion(bool accept);
    void poll_completion_result();
    void sync_scm_ui();
    void refresh_scm(const fs::path &hint);
    fs::path project_root() const;
    fs::path find_workspace_root(const fs::path &hint) const;

    std::uint64_t scm_gen_seen_ = 0;

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
    void ex_sidebar(const std::string &arg);
    void ex_find();
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
