#include <lsp/lsp_service.hpp>
#include <utils/logger.hpp>
#include <utils/text_metrics.hpp>

#include <algorithm>
#include <format>
#include <system_error>
#include <unistd.h>

namespace
{
    MiniJson::Value jstr(std::string s) { return MiniJson::Value{std::move(s)}; }
    MiniJson::Value jnum(double n) { return MiniJson::Value{n}; }
    MiniJson::Value jbool(bool b) { return MiniJson::Value{b}; }
    MiniJson::Value jobject(MiniJson::Object o) { return MiniJson::Value{std::move(o)}; }
    MiniJson::Value jarray(MiniJson::Array a) { return MiniJson::Value{std::move(a)}; }
    MiniJson::Value jnull() { return MiniJson::Value{nullptr}; }

    void advance_pos(int &line, int &col, std::string_view text)
    {
        for (char ch : text)
        {
            if (ch == '\n')
            {
                ++line;
                col = 0;
            }
            else
            {
                ++col;
            }
        }
    }

    int utf16_on_line(const std::vector<std::string> &lines, int line, int byte_col)
    {
        if (line < 0 || line >= static_cast<int>(lines.size()))
            return 0;
        return TextMetrics::byte_to_utf16(
            lines[static_cast<std::size_t>(line)],
            static_cast<std::size_t>(std::max(0, byte_col)));
    }

    DiagnosticSeverity severity_from_json(const MiniJson::Value *v)
    {
        const int s = v && v->is_number() ? v->as_int(1) : 1;
        switch (s)
        {
        case 1:
            return DiagnosticSeverity::Error;
        case 2:
            return DiagnosticSeverity::Warning;
        case 3:
            return DiagnosticSeverity::Information;
        case 4:
            return DiagnosticSeverity::Hint;
        default:
            return DiagnosticSeverity::Error;
        }
    }

    CompletionItem parse_completion_item(const MiniJson::Value &item, Buffer *buf)
    {
        CompletionItem out;
        out.label = item.get_string("label", "");
        out.detail = item.get_string("detail", "");
        out.insert_text = item.get_string("insertText", "");
        if (const MiniJson::Value *k = item.get("kind"); k && k->is_number())
            out.kind = k->as_int(0);
        if (const MiniJson::Value *doc = item.get("documentation"); doc)
        {
            if (doc->is_string())
                out.documentation = doc->as_string();
            else if (doc->is_object())
                out.documentation = doc->get_string("value", "");
        }

        if (const MiniJson::Value *te = item.get("textEdit"); te && te->is_object())
        {
            out.new_text = te->get_string("newText", out.label);
            if (const MiniJson::Value *range = te->get("range"); range && range->is_object() && buf)
            {
                const MiniJson::Value *start = range->get("start");
                const MiniJson::Value *end = range->get("end");
                if (start && end)
                {
                    out.has_text_edit = true;
                    out.edit_start = LspService::lsp_pos_to_cursor(
                        buf->lines(),
                        start->get_int("line", 0),
                        start->get_int("character", 0));
                    out.edit_end = LspService::lsp_pos_to_cursor(
                        buf->lines(),
                        end->get_int("line", 0),
                        end->get_int("character", 0));
                }
            }
        }
        if (out.new_text.empty())
            out.new_text = !out.insert_text.empty() ? out.insert_text : out.label;
        return out;
    }
}

LspSession::LspSession(LspServerConfig config, fs::path root, LspService *owner)
    : config_(std::move(config)), root_(std::move(root)), owner_(owner)
{
    rpc_.set_handler([this](const MiniJson::Value &msg) { on_message(msg); });
}

LspSession::~LspSession()
{
    stop();
}

bool LspSession::start()
{
    if (config_.command.empty())
    {
        state_ = LspSessionState::Failed;
        return false;
    }
    state_ = LspSessionState::Starting;
    if (!process_.start(config_.command, root_.string()))
    {
        state_ = LspSessionState::Failed;
        Logger::error(std::format("LSP failed to start: {}", config_.command[0]));
        return false;
    }

    MiniJson::Object sync;
    sync["didSave"] = jbool(true);
    sync["dynamicRegistration"] = jbool(false);

    MiniJson::Object completion;
    completion["dynamicRegistration"] = jbool(false);
    completion["completionItem"] = jobject({
        {"snippetSupport", jbool(false)},
        {"documentationFormat", jarray({jstr("plaintext"), jstr("markdown")})},
    });
    completion["contextSupport"] = jbool(false);

    MiniJson::Object text_document;
    text_document["synchronization"] = jobject(std::move(sync));
    text_document["completion"] = jobject(std::move(completion));
    text_document["publishDiagnostics"] = jobject({{"relatedInformation", jbool(false)}});

    MiniJson::Object caps;
    caps["textDocument"] = jobject(std::move(text_document));

    MiniJson::Object params;
    params["processId"] = jnum(static_cast<double>(getpid()));
    params["rootUri"] = jstr(LspService::path_to_uri(root_));
    params["capabilities"] = jobject(std::move(caps));
    params["clientInfo"] = jobject({{"name", jstr("noni")}, {"version", jstr("0.1")}});

    initialize_id_ = rpc_.next_id();
    if (!send_raw(rpc_.make_request(initialize_id_, "initialize", jobject(std::move(params)))))
    {
        state_ = LspSessionState::Failed;
        process_.stop();
        return false;
    }
    return true;
}

void LspSession::stop()
{
    if (state_ == LspSessionState::Running || state_ == LspSessionState::Starting)
    {
        const int id = rpc_.next_id();
        (void)send_raw(rpc_.make_request(id, "shutdown", jnull()));
        (void)send_raw(rpc_.make_notification("exit", jnull()));
    }
    process_.stop();
    state_ = LspSessionState::Stopped;
    open_uris_.clear();
    initialized_sent_ = false;
}

void LspSession::pump()
{
    const std::string chunk = process_.take_stdout();
    if (!chunk.empty())
        rpc_.feed(chunk);
    if (!process_.alive() && state_ == LspSessionState::Running)
    {
        Logger::warning(std::format("LSP server exited: {}", config_.language));
        state_ = LspSessionState::Failed;
    }
}

bool LspSession::send_raw(const std::string &framed)
{
    std::lock_guard lock(write_mu_);
    return process_.write_all(framed);
}

void LspSession::send_initialized()
{
    if (initialized_sent_)
        return;
    initialized_sent_ = true;
    (void)send_raw(rpc_.make_notification("initialized", jobject({})));
    state_ = LspSessionState::Running;
    Logger::info(std::format("LSP initialized: {} @ {}", config_.language, root_.string()));
}

void LspSession::on_message(const MiniJson::Value &msg)
{
    if (!msg.is_object())
        return;

    if (const MiniJson::Value *id = msg.get("id"); id && id->is_number())
    {
        const int rid = static_cast<int>(id->as_number());
        if (rid == initialize_id_ && msg.get("result"))
        {
            send_initialized();
            return;
        }
        if (const MiniJson::Value *result = msg.get("result"); result && owner_)
            owner_->on_completion_response(rid, *result);
        return;
    }

    const std::string method = msg.get_string("method", "");
    if (method.empty())
        return;

    if (const MiniJson::Value *id = msg.get("id"); id && !id->is_null())
    {
        if (method == "workspace/configuration")
        {
            (void)send_raw(rpc_.make_response(*id, jarray({})));
            return;
        }
        if (method == "client/registerCapability" || method == "client/unregisterCapability")
        {
            (void)send_raw(rpc_.make_response(*id, jobject({})));
            return;
        }
        if (method == "window/workDoneProgress/create")
        {
            (void)send_raw(rpc_.make_response(*id, jnull()));
            return;
        }
        (void)send_raw(rpc_.make_error(*id, -32601, "Method not implemented"));
        return;
    }

    if (method == "window/logMessage" || method == "window/showMessage")
    {
        if (const MiniJson::Value *p = msg.get("params"); p && p->is_object())
            Logger::debug(std::format("LSP {}: {}", method, p->get_string("message", "")));
        return;
    }
    if (method == "textDocument/publishDiagnostics" && owner_)
    {
        if (const MiniJson::Value *p = msg.get("params"); p && p->is_object())
            owner_->on_publish_diagnostics(*p);
        return;
    }
}

void LspSession::did_open(
    const std::string &uri,
    const std::string &language_id,
    int version,
    const std::string &text)
{
    if (state_ != LspSessionState::Running && state_ != LspSessionState::Starting)
        return;
    open_uris_.insert(uri);
    MiniJson::Object doc;
    doc["uri"] = jstr(uri);
    doc["languageId"] = jstr(language_id);
    doc["version"] = jnum(static_cast<double>(version));
    doc["text"] = jstr(text);
    MiniJson::Object params;
    params["textDocument"] = jobject(std::move(doc));
    (void)send_raw(rpc_.make_notification("textDocument/didOpen", jobject(std::move(params))));
}

void LspSession::did_change_full(const std::string &uri, int version, const std::string &text)
{
    if (state_ != LspSessionState::Running)
        return;
    if (!open_uris_.count(uri))
        return;
    MiniJson::Object td;
    td["uri"] = jstr(uri);
    td["version"] = jnum(static_cast<double>(version));
    MiniJson::Object change;
    change["text"] = jstr(text);
    MiniJson::Object params;
    params["textDocument"] = jobject(std::move(td));
    params["contentChanges"] = jarray({jobject(std::move(change))});
    (void)send_raw(rpc_.make_notification("textDocument/didChange", jobject(std::move(params))));
}

void LspSession::did_change_incremental(
    const std::string &uri,
    int version,
    int start_line,
    int start_utf16,
    int end_line,
    int end_utf16,
    const std::string &text)
{
    if (state_ != LspSessionState::Running)
        return;
    if (!open_uris_.count(uri))
        return;
    MiniJson::Object start;
    start["line"] = jnum(static_cast<double>(start_line));
    start["character"] = jnum(static_cast<double>(start_utf16));
    MiniJson::Object end;
    end["line"] = jnum(static_cast<double>(end_line));
    end["character"] = jnum(static_cast<double>(end_utf16));
    MiniJson::Object range;
    range["start"] = jobject(std::move(start));
    range["end"] = jobject(std::move(end));
    MiniJson::Object change;
    change["range"] = jobject(std::move(range));
    change["text"] = jstr(text);
    MiniJson::Object td;
    td["uri"] = jstr(uri);
    td["version"] = jnum(static_cast<double>(version));
    MiniJson::Object params;
    params["textDocument"] = jobject(std::move(td));
    params["contentChanges"] = jarray({jobject(std::move(change))});
    (void)send_raw(rpc_.make_notification("textDocument/didChange", jobject(std::move(params))));
}

void LspSession::did_save(const std::string &uri)
{
    if (state_ != LspSessionState::Running)
        return;
    MiniJson::Object td;
    td["uri"] = jstr(uri);
    MiniJson::Object params;
    params["textDocument"] = jobject(std::move(td));
    (void)send_raw(rpc_.make_notification("textDocument/didSave", jobject(std::move(params))));
}

void LspSession::did_close(const std::string &uri)
{
    if (state_ == LspSessionState::Idle || state_ == LspSessionState::Stopped)
        return;
    open_uris_.erase(uri);
    MiniJson::Object td;
    td["uri"] = jstr(uri);
    MiniJson::Object params;
    params["textDocument"] = jobject(std::move(td));
    (void)send_raw(rpc_.make_notification("textDocument/didClose", jobject(std::move(params))));
}

int LspSession::request_completion(const std::string &uri, int line, int character)
{
    if (state_ != LspSessionState::Running)
        return 0;
    if (!open_uris_.count(uri))
        return 0;
    const int id = rpc_.next_id();
    MiniJson::Object pos;
    pos["line"] = jnum(static_cast<double>(line));
    pos["character"] = jnum(static_cast<double>(character));
    MiniJson::Object td;
    td["uri"] = jstr(uri);
    MiniJson::Object params;
    params["textDocument"] = jobject(std::move(td));
    params["position"] = jobject(std::move(pos));
    if (!send_raw(rpc_.make_request(id, "textDocument/completion", jobject(std::move(params)))))
        return 0;
    return id;
}

bool LspSession::is_document_open(const std::string &uri) const
{
    return open_uris_.count(uri) > 0;
}

// --- LspService ---

Cursor LspService::lsp_pos_to_cursor(const std::vector<std::string> &lines, int line, int character)
{
    if (lines.empty())
        return {.line = 0, .column = 0};
    line = std::clamp(line, 0, static_cast<int>(lines.size()) - 1);
    const auto &row = lines[static_cast<std::size_t>(line)];
    const int byte = static_cast<int>(TextMetrics::utf16_to_byte(row, character));
    return {.line = line, .column = byte};
}

void LspService::set_workspace_root(const fs::path &root)
{
    std::lock_guard lock(mu_);
    if (workspace_root_ == root)
        return;
    for (auto &[k, s] : sessions_)
    {
        if (s)
            s->stop();
    }
    sessions_.clear();
    documents_.clear();
    pending_completion_id_ = 0;
    ready_completion_.reset();
    workspace_root_ = root;
}

fs::path LspService::workspace_root() const
{
    std::lock_guard lock(mu_);
    return workspace_root_;
}

void LspService::set_server_configs(std::vector<LspServerConfig> configs)
{
    std::lock_guard lock(mu_);
    configs_ = std::move(configs);
}

void LspService::pump()
{
    std::lock_guard lock(mu_);
    for (auto &[k, s] : sessions_)
    {
        if (s)
            s->pump();
    }
}

std::string LspService::path_to_uri(const fs::path &path)
{
    std::error_code ec;
    fs::path abs = fs::weakly_canonical(path, ec);
    if (ec)
        abs = fs::absolute(path, ec);
    if (ec)
        abs = path;
    std::string s = abs.generic_string();
    if (!s.empty() && s[0] != '/')
        return "file:///" + s;
    return "file://" + s;
}

std::string LspService::join_lines(const std::vector<std::string> &lines)
{
    std::string out;
    for (std::size_t i = 0; i < lines.size(); ++i)
    {
        out += lines[i];
        if (i + 1 < lines.size())
            out.push_back('\n');
    }
    return out;
}

std::string LspService::language_id_for(Language lang)
{
    return Syntax::language_definition(lang).name;
}

LspSession *LspService::session_for(Language lang)
{
    if (lang == Language::Plain || workspace_root_.empty())
        return nullptr;

    const std::string name = language_id_for(lang);
    auto it = sessions_.find(name);
    if (it != sessions_.end() && it->second)
        return it->second.get();

    const LspServerConfig *cfg = nullptr;
    for (const auto &c : configs_)
    {
        if (c.language == name)
        {
            cfg = &c;
            break;
        }
    }
    if (!cfg && (lang == Language::C || lang == Language::Cpp))
    {
        for (const auto &c : configs_)
        {
            if (c.language == "cpp" || c.language == "c")
            {
                cfg = &c;
                break;
            }
        }
    }
    if (!cfg || cfg->command.empty())
        return nullptr;

    auto session = std::make_unique<LspSession>(*cfg, workspace_root_, this);
    if (!session->start())
        return nullptr;
    LspSession *raw = session.get();
    sessions_[name] = std::move(session);
    return raw;
}

LspDocumentState *LspService::doc_for(Buffer &buffer)
{
    const auto id = reinterpret_cast<std::uintptr_t>(&buffer);
    return &documents_[id];
}

Buffer *LspService::buffer_for_uri(const std::string &uri)
{
    for (auto &[id, doc] : documents_)
    {
        (void)id;
        if (doc.open && doc.uri == uri && doc.buffer)
            return doc.buffer;
    }
    return nullptr;
}

void LspService::notify_open(Buffer &buffer)
{
    std::lock_guard lock(mu_);
    const fs::path path = buffer.get_buffer_path();
    if (path.empty() || buffer.has_load_error())
        return;

    const auto id = reinterpret_cast<std::uintptr_t>(&buffer);
    auto existing = documents_.find(id);
    if (existing != documents_.end() && existing->second.open)
        return;

    const Language lang = Syntax::detect_language(path);
    LspSession *session = session_for(lang);
    if (!session)
        return;

    auto &doc = *doc_for(buffer);
    doc.uri = path_to_uri(path);
    doc.language = lang;
    doc.version = 1;
    doc.open = true;
    doc.buffer = &buffer;
    session->did_open(doc.uri, language_id_for(lang), doc.version, join_lines(buffer.lines()));
}

void LspService::notify_change(Buffer &buffer, const TextChange &change)
{
    std::lock_guard lock(mu_);
    auto it = documents_.find(reinterpret_cast<std::uintptr_t>(&buffer));
    if (it == documents_.end() || !it->second.open)
        return;

    auto &doc = it->second;
    LspSession *session = session_for(doc.language);
    if (!session)
        return;

    ++doc.version;
    const auto &lines = buffer.lines();
    int el = change.start_line;
    int ec = change.start_col;
    advance_pos(el, ec, change.deleted);

    const int start_u16 = utf16_on_line(lines, change.start_line, change.start_col);
    if (change.start_line != el)
    {
        session->did_change_full(doc.uri, doc.version, join_lines(lines));
        return;
    }

    int end_u16 = start_u16 + TextMetrics::byte_to_utf16(change.deleted, change.deleted.size());
    if (change.deleted.empty())
        end_u16 = start_u16;

    session->did_change_incremental(
        doc.uri,
        doc.version,
        change.start_line,
        start_u16,
        el,
        end_u16,
        change.inserted);
}

void LspService::notify_reload(Buffer &buffer)
{
    bool need_open = false;
    {
        std::lock_guard lock(mu_);
        auto it = documents_.find(reinterpret_cast<std::uintptr_t>(&buffer));
        if (it == documents_.end() || !it->second.open)
        {
            need_open = true;
        }
        else
        {
            auto &doc = it->second;
            LspSession *session = session_for(doc.language);
            if (!session)
                return;
            ++doc.version;
            session->did_change_full(doc.uri, doc.version, join_lines(buffer.lines()));
            return;
        }
    }
    if (need_open)
        notify_open(buffer);
}

void LspService::notify_save(Buffer &buffer)
{
    std::lock_guard lock(mu_);
    auto it = documents_.find(reinterpret_cast<std::uintptr_t>(&buffer));
    if (it == documents_.end() || !it->second.open)
        return;
    LspSession *session = session_for(it->second.language);
    if (!session)
        return;
    session->did_save(it->second.uri);
}

void LspService::notify_close(Buffer &buffer)
{
    std::lock_guard lock(mu_);
    const auto id = reinterpret_cast<std::uintptr_t>(&buffer);
    auto it = documents_.find(id);
    if (it == documents_.end())
        return;
    if (it->second.open)
    {
        if (LspSession *session = session_for(it->second.language))
            session->did_close(it->second.uri);
    }
    if (pending_completion_buffer_ == id)
    {
        pending_completion_id_ = 0;
        ready_completion_.reset();
    }
    buffer.diagnostics() = {};
    documents_.erase(it);
}

int LspService::request_completion(Buffer &buffer, int line, int byte_col)
{
    std::lock_guard lock(mu_);
    auto it = documents_.find(reinterpret_cast<std::uintptr_t>(&buffer));
    if (it == documents_.end() || !it->second.open)
        return 0;
    LspSession *session = session_for(it->second.language);
    if (!session || session->state() != LspSessionState::Running)
        return 0;

    const int u16 = utf16_on_line(buffer.lines(), line, byte_col);
    const int id = session->request_completion(it->second.uri, line, u16);
    if (id <= 0)
        return 0;

    pending_completion_id_ = id;
    pending_completion_buffer_ = reinterpret_cast<std::uintptr_t>(&buffer);
    pending_completion_version_ = it->second.version;
    pending_completion_trigger_ = {.line = line, .column = byte_col};
    ready_completion_.reset();
    return id;
}

std::optional<CompletionList> LspService::take_completion_result()
{
    std::lock_guard lock(mu_);
    if (!ready_completion_)
        return std::nullopt;
    auto out = std::move(*ready_completion_);
    ready_completion_.reset();
    return out;
}

void LspService::cancel_completion()
{
    std::lock_guard lock(mu_);
    pending_completion_id_ = 0;
    ready_completion_.reset();
}

void LspService::on_publish_diagnostics(const MiniJson::Value &params)
{
    // Called from LspSession::on_message during pump() — mu_ already held.
    const std::string uri = params.get_string("uri", "");
    if (uri.empty())
        return;
    Buffer *buf = buffer_for_uri(uri);
    if (!buf)
        return;

    int version = -1;
    if (const MiniJson::Value *v = params.get("version"); v && v->is_number())
        version = v->as_int(-1);

    // Drop stale diagnostics when we know versions.
    auto dit = documents_.find(reinterpret_cast<std::uintptr_t>(buf));
    if (dit != documents_.end() && version >= 0 && version < dit->second.version)
        return;

    DiagnosticSnapshot snap;
    snap.lsp_version = version;
    if (const MiniJson::Value *arr = params.get("diagnostics"); arr && arr->is_array())
    {
        for (const auto &d : arr->as_array())
        {
            if (!d.is_object())
                continue;
            Diagnostic item;
            item.severity = severity_from_json(d.get("severity"));
            item.message = d.get_string("message", "");
            item.source = d.get_string("source", "");
            if (const MiniJson::Value *code = d.get("code"); code)
            {
                if (code->is_string())
                    item.code = code->as_string();
                else if (code->is_number())
                    item.code = std::to_string(code->as_int(0));
            }
            if (const MiniJson::Value *range = d.get("range"); range && range->is_object())
            {
                const MiniJson::Value *start = range->get("start");
                const MiniJson::Value *end = range->get("end");
                if (start && end)
                {
                    item.start = lsp_pos_to_cursor(
                        buf->lines(), start->get_int("line", 0), start->get_int("character", 0));
                    item.end = lsp_pos_to_cursor(
                        buf->lines(), end->get_int("line", 0), end->get_int("character", 0));
                }
            }
            snap.items.push_back(std::move(item));
        }
    }
    buf->set_diagnostics(std::move(snap));
}

void LspService::on_completion_response(int id, const MiniJson::Value &result)
{
    // Called from LspSession::on_message during pump() — mu_ already held.
    if (id != pending_completion_id_ || pending_completion_id_ <= 0)
        return; // stale

    Buffer *buf = nullptr;
    auto dit = documents_.find(pending_completion_buffer_);
    if (dit == documents_.end() || !dit->second.open || !dit->second.buffer)
    {
        pending_completion_id_ = 0;
        return;
    }
    if (dit->second.version != pending_completion_version_)
    {
        pending_completion_id_ = 0;
        return; // document moved on
    }
    buf = dit->second.buffer;

    CompletionList list;
    list.request_id = id;
    list.buffer_id = pending_completion_buffer_;
    list.doc_version = pending_completion_version_;
    list.trigger = pending_completion_trigger_;

    const MiniJson::Array *items = nullptr;
    if (result.is_array())
    {
        items = &result.as_array();
    }
    else if (result.is_object())
    {
        list.incomplete = result.get("isIncomplete") && result.get("isIncomplete")->as_bool(false);
        if (const MiniJson::Value *arr = result.get("items"); arr && arr->is_array())
            items = &arr->as_array();
    }
    if (items)
    {
        list.items.reserve(items->size());
        for (const auto &it : *items)
        {
            if (!it.is_object())
                continue;
            list.items.push_back(parse_completion_item(it, buf));
        }
    }

    pending_completion_id_ = 0;
    ready_completion_ = std::move(list);
}

void LspService::shutdown_all()
{
    std::lock_guard lock(mu_);
    for (auto &[k, s] : sessions_)
    {
        if (s)
            s->stop();
    }
    sessions_.clear();
    documents_.clear();
    pending_completion_id_ = 0;
    ready_completion_.reset();
}

std::string LspService::status_summary() const
{
    std::lock_guard lock(mu_);
    if (workspace_root_.empty())
        return "LSP: no workspace";
    if (sessions_.empty())
        return std::format("LSP: idle @ {}", workspace_root_.string());
    std::string out = "LSP:";
    for (const auto &[name, s] : sessions_)
    {
        if (!s)
            continue;
        const char *st = "?";
        switch (s->state())
        {
        case LspSessionState::Starting:
            st = "starting";
            break;
        case LspSessionState::Running:
            st = "running";
            break;
        case LspSessionState::Failed:
            st = "failed";
            break;
        case LspSessionState::Stopped:
            st = "stopped";
            break;
        default:
            st = "idle";
            break;
        }
        out += std::format(" {}[{}]", name, st);
    }
    out += std::format(" · {} docs", documents_.size());
    return out;
}
