#pragma once

#include <configs/mini_json.hpp>
#include <editor/buffer.hpp>
#include <editor/undo.hpp>
#include <lsp/completion.hpp>
#include <lsp/diagnostics.hpp>
#include <lsp/lsp_models.hpp>
#include <lsp/lsp_session.hpp>
#include <syntax/syntax.hpp>
#include <utils/cursor.hpp>

#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

class LspService
{
public:
    void set_workspace_root(const fs::path &root);
    fs::path workspace_root() const;

    void set_server_configs(std::vector<LspServerConfig> configs);

    void pump();

    void notify_open(Buffer &buffer);
    void notify_change(Buffer &buffer, const TextChange &change);
    void notify_reload(Buffer &buffer);
    void notify_save(Buffer &buffer);
    void notify_close(Buffer &buffer);

    int request_completion(Buffer &buffer, int line, int byte_col);
    std::optional<CompletionList> take_completion_result();
    void cancel_completion();

    int request_definition(Buffer &buffer, int line, int byte_col);
    int request_declaration(Buffer &buffer, int line, int byte_col);
    int request_type_definition(Buffer &buffer, int line, int byte_col);
    int request_references(Buffer &buffer, int line, int byte_col);
    int request_document_symbols(Buffer &buffer);
    int request_workspace_symbols(Buffer &buffer, const std::string &query);
    int request_rename(Buffer &buffer, int line, int byte_col, const std::string &new_name);
    int request_code_actions(Buffer &buffer, Cursor start, Cursor end);

    std::optional<LspLocationList> take_location_result();
    std::optional<LspSymbolList> take_symbol_result();
    std::optional<LspRenameResult> take_rename_result();
    std::optional<LspCodeActionList> take_code_action_result();

    void cancel_pending();

    void on_publish_diagnostics(const MiniJson::Value &params);
    void on_response(int id, const MiniJson::Value &result);
    void on_workspace_apply_edit(const MiniJson::Value &params);

    std::optional<LspWorkspaceEdit> take_server_apply_edit();

    void shutdown_all();
    std::string status_summary() const;

    static std::string path_to_uri(const fs::path &path);
    static fs::path uri_to_path(const std::string &uri);
    static std::string join_lines(const std::vector<std::string> &lines);
    static std::string language_id_for(Language lang);

    static Cursor lsp_pos_to_cursor(const std::vector<std::string> &lines, int line, int character);

    Buffer *buffer_for_uri(const std::string &uri);

private:
    LspSession *session_for(Language lang, const fs::path &file_hint = {});
    LspDocumentState *doc_for(Buffer &buffer);
    int begin_position_request(Buffer &buffer, int line, int byte_col, LspPendingKind kind, const char *method);
    void clear_ready();

    mutable std::mutex mu_;
    fs::path workspace_root_;
    std::vector<LspServerConfig> configs_;
    std::unordered_map<std::string, std::unique_ptr<LspSession>> sessions_;
    std::unordered_map<std::uintptr_t, LspDocumentState> documents_;
    std::uint64_t install_gen_ = 0;

    std::unordered_map<int, LspPendingRequest> pending_;

    std::optional<CompletionList> ready_completion_;
    std::optional<LspLocationList> ready_locations_;
    std::optional<LspSymbolList> ready_symbols_;
    std::optional<LspRenameResult> ready_rename_;
    std::optional<LspCodeActionList> ready_actions_;
    std::optional<LspWorkspaceEdit> ready_server_apply_;
};
