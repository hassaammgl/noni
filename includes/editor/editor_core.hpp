#pragma once

#include <editor/buffer_manager.hpp>
#include <editor/buffer_search.hpp>
#include <editor/jumps.hpp>
#include <editor/marks.hpp>
#include <editor/registers.hpp>
#include <editor/window_layout.hpp>
#include <extensions/editor_events.hpp>
#include <lsp/lsp_service.hpp>
#include <scm/scm_service.hpp>
#include <workspace/recent_files.hpp>
#include <workspace/workspace.hpp>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace fs = std::filesystem;

class EditorCore
{
private:
    BufferManager buffers_;
    RegisterFile registers_;
    MarkTable marks_;
    JumpList jumps_;
    BufferSearchState search_;
    ScmService scm_;
    LspService lsp_;
    EditorEvents events_;
    Workspace workspace_;
    RecentFiles recent_;
    int editor_area_w_ = 80;
    int editor_area_h_ = 24;

    void record_jump_from_active();
    void move_active_to_match(const BufferSearchMatch &m, bool record_jump);

public:
    BufferManager &buffers() { return buffers_; }
    const BufferManager &buffers() const { return buffers_; }
    RegisterFile &registers() { return registers_; }
    const RegisterFile &registers() const { return registers_; }
    MarkTable &marks() { return marks_; }
    JumpList &jumps() { return jumps_; }
    BufferSearchState &search() { return search_; }
    const BufferSearchState &search() const { return search_; }
    ScmService &scm() { return scm_; }
    const ScmService &scm() const { return scm_; }
    LspService &lsp() { return lsp_; }
    const LspService &lsp() const { return lsp_; }
    EditorEvents &events() { return events_; }
    const EditorEvents &events() const { return events_; }
    Workspace &workspace() { return workspace_; }
    const Workspace &workspace() const { return workspace_; }
    RecentFiles &recent() { return recent_; }
    const RecentFiles &recent() const { return recent_; }

    void attach_lsp_document(Buffer &buffer);
    std::optional<ScmFileStatus> scm_status_for_path(const fs::path &path) const;
    void set_editor_area(int w, int h);
    int editor_area_w() const { return editor_area_w_; }
    int editor_area_h() const { return editor_area_h_; }
    bool has_tabs() const { return buffers_.has_tabs(); }
    EditorTab &active_tab() { return buffers_.active(); }
    const EditorTab &active_tab() const { return buffers_.active(); }

    Window *active_window();
    const Window *active_window() const;
    Buffer *active_buffer();
    const Buffer *active_buffer() const;
    EditorMode mode() const;
    void set_mode(EditorMode mode);
    std::uintptr_t buffer_id(Buffer *b) const;
    void on_buffer_closed(Buffer *b);
    void notify_buffer_saved(Buffer &b);

    std::string search_start(std::string pattern, SearchDirection dir);
    std::string search_next(bool reverse);
    std::string replace_current(std::string_view replacement);
    std::string replace_all(std::string_view replacement);
    std::string replace_selection(std::string_view replacement);

    bool split_vertical();
    bool split_horizontal();
    bool close_window();
    bool focus_left();
    bool focus_right();
    bool focus_up();
    bool focus_down();
    void resize_left();
    void resize_right();
    void resize_up();
    void resize_down();
};
