#include <lsp/lsp_jsonrpc.hpp>
#include <utils/logger.hpp>

#include <cctype>
#include <format>

namespace
{
    MiniJson::Value jstr(std::string s)
    {
        return MiniJson::Value{std::move(s)};
    }

    MiniJson::Value jnum(double n)
    {
        return MiniJson::Value{n};
    }

    MiniJson::Value jobject(MiniJson::Object o)
    {
        return MiniJson::Value{std::move(o)};
    }

    std::string frame(const std::string &body)
    {
        return std::format("Content-Length: {}\r\n\r\n{}", body.size(), body);
    }
}

void LspJsonRpc::feed(const std::string &chunk)
{
    if (chunk.empty())
        return;
    buffer_ += chunk;
    while (try_parse_one())
    {
    }
}

bool LspJsonRpc::try_parse_one()
{
    // Headers end with \r\n\r\n
    const auto header_end = buffer_.find("\r\n\r\n");
    if (header_end == std::string::npos)
        return false;

    const std::string headers = buffer_.substr(0, header_end);
    std::size_t content_length = 0;
    bool found = false;
    std::size_t line_start = 0;
    while (line_start < headers.size())
    {
        auto line_end = headers.find("\r\n", line_start);
        if (line_end == std::string::npos)
            line_end = headers.size();
        std::string line = headers.substr(line_start, line_end - line_start);
        // Case-insensitive Content-Length
        std::string lower = line;
        for (char &c : lower)
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (lower.rfind("content-length:", 0) == 0)
        {
            const auto colon = line.find(':');
            if (colon != std::string::npos)
            {
                try
                {
                    content_length = static_cast<std::size_t>(std::stoul(line.substr(colon + 1)));
                    found = true;
                }
                catch (...)
                {
                    buffer_.erase(0, header_end + 4);
                    return true;
                }
            }
        }
        line_start = (line_end == headers.size()) ? line_end : line_end + 2;
    }

    if (!found)
    {
        buffer_.erase(0, header_end + 4);
        return true;
    }

    const std::size_t body_start = header_end + 4;
    if (buffer_.size() < body_start + content_length)
        return false;

    const std::string body = buffer_.substr(body_start, content_length);
    buffer_.erase(0, body_start + content_length);

    try
    {
        MiniJson::Value msg = MiniJson::parse(body);
        if (handler_)
            handler_(msg);
    }
    catch (const std::exception &e)
    {
        Logger::warning(std::format("LSP: malformed JSON-RPC body: {}", e.what()));
    }
    return true;
}

int LspJsonRpc::next_id()
{
    std::lock_guard lock(id_mu_);
    return next_id_++;
}

std::string LspJsonRpc::make_request(int id, const std::string &method, const MiniJson::Value &params)
{
    MiniJson::Object o;
    o["jsonrpc"] = jstr("2.0");
    o["id"] = jnum(static_cast<double>(id));
    o["method"] = jstr(method);
    o["params"] = params;
    return frame(MiniJson::stringify(jobject(std::move(o))));
}

std::string LspJsonRpc::make_notification(const std::string &method, const MiniJson::Value &params)
{
    MiniJson::Object o;
    o["jsonrpc"] = jstr("2.0");
    o["method"] = jstr(method);
    o["params"] = params;
    return frame(MiniJson::stringify(jobject(std::move(o))));
}

std::string LspJsonRpc::make_response(const MiniJson::Value &id, const MiniJson::Value &result)
{
    MiniJson::Object o;
    o["jsonrpc"] = jstr("2.0");
    o["id"] = id;
    o["result"] = result;
    return frame(MiniJson::stringify(jobject(std::move(o))));
}

std::string LspJsonRpc::make_error(const MiniJson::Value &id, int code, const std::string &message)
{
    MiniJson::Object err;
    err["code"] = jnum(static_cast<double>(code));
    err["message"] = jstr(message);
    MiniJson::Object o;
    o["jsonrpc"] = jstr("2.0");
    o["id"] = id;
    o["error"] = jobject(std::move(err));
    return frame(MiniJson::stringify(jobject(std::move(o))));
}
