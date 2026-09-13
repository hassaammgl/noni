#pragma once

#include <configs/mini_json.hpp>
#include <editor/buffer.hpp>
#include <editor/undo.hpp>
#include <lsp/completion.hpp>
#include <lsp/diagnostics.hpp>
#include <lsp/lsp_jsonrpc.hpp>
#include <lsp/lsp_process.hpp>
#include <syntax/syntax.hpp>

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

    // Returns request id (>0) or 0 if unavailable.
    int request_completion(Buffer &buffer, int line, int byte_col);
    std::optional<CompletionList> take_completion_result();
    void cancel_completion();

    void on_publish_diagnostics(const MiniJson::Value &params);
    void on_completion_response(int id, const MiniJson::Value &result);

    void shutdown_all();
    std::string status_summary() const;

    static std::string path_to_uri(const fs::path &path);
    static std::string join_lines(const std::vector<std::string> &lines);
    static std::string language_id_for(Language lang);

    // Convert LSP UTF-16 position using Buffer lines → byte Cursor.
    static Cursor lsp_pos_to_cursor(const std::vector<std::string> &lines, int line, int character);

private:
    LspSession *session_for(Language lang);
    LspDocumentState *doc_for(Buffer &buffer);
    Buffer *buffer_for_uri(const std::string &uri);

    mutable std::mutex mu_;
    fs::path workspace_root_;
    std::vector<LspServerConfig> configs_;
    std::unordered_map<std::string, std::unique_ptr<LspSession>> sessions_;
    std::unordered_map<std::uintptr_t, LspDocumentState> documents_;

    int pending_completion_id_ = 0;
    std::uintptr_t pending_completion_buffer_ = 0;
    int pending_completion_version_ = 0;
    Cursor pending_completion_trigger_{};
    std::optional<CompletionList> ready_completion_;
};
