#pragma once

#include <configs/mini_json.hpp>
#include <lsp/lsp_jsonrpc.hpp>
#include <lsp/lsp_process.hpp>
#include <lsp/lsp_types.hpp>

#include <atomic>
#include <filesystem>
#include <mutex>
#include <string>
#include <unordered_set>
#include <vector>

namespace fs = std::filesystem;

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

    int sync_kind() const { return caps_sync_kind_; }
    bool hover_provider() const { return caps_hover_; }
    bool formatting_provider() const { return caps_formatting_; }

private:
    void on_message(const MiniJson::Value &msg);
    void send_initialized();
    void apply_initialize_result(const MiniJson::Value &result);

    LspServerConfig config_;
    fs::path root_;
    LspService *owner_ = nullptr;
    LspProcess process_;
    LspJsonRpc rpc_;
    std::atomic<LspSessionState> state_{LspSessionState::Idle};
    std::mutex write_mu_;
    std::unordered_set<std::string> open_uris_;
    struct PendingOpen
    {
        std::string uri;
        std::string language_id;
        int version = 1;
        std::string text;
    };
    std::vector<PendingOpen> pending_opens_;
    int initialize_id_ = 0;
    bool initialized_sent_ = false;
    int caps_sync_kind_ = 2;
    bool caps_hover_ = false;
    bool caps_formatting_ = false;
    bool exit_announced_ = false;

    void flush_did_open(const PendingOpen &doc);
};
