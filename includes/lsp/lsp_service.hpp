#pragma once

#include <configs/mini_json.hpp>
#include <editor/buffer.hpp>
#include <editor/undo.hpp>
#include <lsp/completion.hpp>
#include <lsp/diagnostics.hpp>
#include <lsp/lsp_jsonrpc.hpp>
#include <lsp/lsp_models.hpp>
#include <lsp/lsp_process.hpp>
#include <syntax/syntax.hpp>
#include <utils/cursor.hpp>

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace fs = std::filesystem;

struct LspServerConfig
{
    std::string language;
    std::vector<std::string> command;
    std::vector<std::string> root_markers;
};

enum class LspSessionState
{
    Idle,
    Starting,
    Running,
    Failed,
    Stopped,
};

struct LspDocumentState
{
    std::string uri;
    int version = 0;
    Language language = Language::Plain;
    bool open = false;
    Buffer *buffer = nullptr;
};

enum class LspPendingKind
{
    Completion,
    Definition,
    Declaration,
    TypeDefinition,
    References,
    DocumentSymbol,
    WorkspaceSymbol,
    Rename,
    CodeAction,
};

struct LspPendingRequest
{
    LspPendingKind kind = LspPendingKind::Completion;
    std::uintptr_t buffer_id = 0;
    int doc_version = 0;
    Cursor trigger{};
};

class LspService;

class LspSession
{
public:
    explicit LspSession(LspServerConfig config, fs::path root, LspService *owner);
    ~LspSession();

    LspSession(const LspSession &) = delete;
    LspSession &operator=(const LspSession &) = delete;

    bool start();
    void stop();
    void pump();

    LspSessionState state() const { return state_; }
    const std::string &language() const { return config_.language; }
    const fs::path &root() const { return root_; }

    void did_open(const std::string &uri, const std::string &language_id, int version, const std::string &text);
    void did_change_full(const std::string &uri, int version, const std::string &text);
    void did_change_incremental(
        const std::string &uri,
        int version,
        int start_line,
        int start_utf16,
        int end_line,
        int end_utf16,
        const std::string &text);
    void did_save(const std::string &uri);
    void did_close(const std::string &uri);

    int request_completion(const std::string &uri, int line, int character);
    int request_position(const std::string &method, const std::string &uri, int line, int character);
    int request_references(const std::string &uri, int line, int character);
    int request_document_symbol(const std::string &uri);
    int request_workspace_symbol(const std::string &query);
    int request_rename(const std::string &uri, int line, int character, const std::string &new_name);
    int request_code_action(
        const std::string &uri,
        int start_line,
        int start_character,
        int end_line,
        int end_character);

    bool is_document_open(const std::string &uri) const;
    bool send_raw(const std::string &framed);

private:
    void on_message(const MiniJson::Value &msg);
    void send_initialized();

    LspServerConfig config_;
    fs::path root_;
    LspService *owner_ = nullptr;
    LspProcess process_;
    LspJsonRpc rpc_;
    std::atomic<LspSessionState> state_{LspSessionState::Idle};
    std::mutex write_mu_;
    std::unordered_set<std::string> open_uris_;
    int initialize_id_ = 0;
    bool initialized_sent_ = false;
};

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

    // Last server-initiated applyEdit (UI may apply via LspEdits).
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
    LspSession *session_for(Language lang);
    LspDocumentState *doc_for(Buffer &buffer);
    int begin_position_request(Buffer &buffer, int line, int byte_col, LspPendingKind kind, const char *method);
    void clear_ready();

    mutable std::mutex mu_;
    fs::path workspace_root_;
    std::vector<LspServerConfig> configs_;
    std::unordered_map<std::string, std::unique_ptr<LspSession>> sessions_;
    std::unordered_map<std::uintptr_t, LspDocumentState> documents_;

    std::unordered_map<int, LspPendingRequest> pending_;

    std::optional<CompletionList> ready_completion_;
    std::optional<LspLocationList> ready_locations_;
    std::optional<LspSymbolList> ready_symbols_;
    std::optional<LspRenameResult> ready_rename_;
    std::optional<LspCodeActionList> ready_actions_;
    std::optional<LspWorkspaceEdit> ready_server_apply_;
};
