#pragma once

#include <configs/mini_json.hpp>

#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

// Content-Length JSON-RPC 2.0 framing for LSP.
class LspJsonRpc
{
public:
    using MessageHandler = std::function<void(const MiniJson::Value &msg)>;

    void set_handler(MessageHandler handler) { handler_ = std::move(handler); }

    // Feed raw stdout bytes; invokes handler for complete messages.
    void feed(const std::string &chunk);

    std::string make_request(int id, const std::string &method, const MiniJson::Value &params);
    std::string make_notification(const std::string &method, const MiniJson::Value &params);
    std::string make_response(const MiniJson::Value &id, const MiniJson::Value &result);
    std::string make_error(const MiniJson::Value &id, int code, const std::string &message);

    int next_id();

private:
    bool try_parse_one();

    MessageHandler handler_;
    std::string buffer_;
    std::mutex id_mu_;
    int next_id_ = 1;
};
