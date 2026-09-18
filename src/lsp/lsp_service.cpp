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

    LspLocation parse_location(const MiniJson::Value &v, Buffer * /*hint_buf*/)
    {
        LspLocation loc;
        if (!v.is_object())
            return loc;

        auto set_range = [&](const MiniJson::Value *range) {
            if (!range || !range->is_object())
                return;
            const MiniJson::Value *s = range->get("start");
            const MiniJson::Value *e = range->get("end");
            if (!s || !e)
                return;
            // Store UTF-16 characters; convert at navigation against the target Buffer.
            loc.start = {.line = s->get_int("line", 0), .column = s->get_int("character", 0)};
            loc.end = {.line = e->get_int("line", 0), .column = e->get_int("character", 0)};
        };

        if (v.get("targetUri"))
        {
            loc.uri = v.get_string("targetUri", "");
            const MiniJson::Value *range = v.get("targetSelectionRange");
            if (!range)
                range = v.get("targetRange");
            set_range(range);
        }
        else
        {
            loc.uri = v.get_string("uri", "");
            set_range(v.get("range"));
        }

        const fs::path path = LspService::uri_to_path(loc.uri);
        loc.display = std::format(
            "{}:{}:{}",
            path.empty() ? loc.uri : path.filename().string(),
            loc.start.line + 1,
            loc.start.column + 1);
        return loc;
    }

    void collect_locations(const MiniJson::Value &result, Buffer *hint, std::vector<LspLocation> &out)
    {
        if (result.is_null())
            return;
        if (result.is_array())
        {
            for (const auto &item : result.as_array())
            {
                auto loc = parse_location(item, hint);
                if (!loc.uri.empty())
                    out.push_back(std::move(loc));
            }
            return;
        }
        if (result.is_object())
        {
            auto loc = parse_location(result, hint);
            if (!loc.uri.empty())
                out.push_back(std::move(loc));
        }
    }

    LspTextEdit parse_text_edit_utf16(const MiniJson::Value &te)
    {
        LspTextEdit out;
        out.new_text = te.get_string("newText", "");
        if (const MiniJson::Value *range = te.get("range"); range && range->is_object())
        {
            if (const MiniJson::Value *s = range->get("start"))
            {
                out.start.line = s->get_int("line", 0);
                out.start.column = s->get_int("character", 0);
            }
            if (const MiniJson::Value *e = range->get("end"))
            {
                out.end.line = e->get_int("line", 0);
                out.end.column = e->get_int("character", 0);
            }
        }
        return out;
    }

    LspWorkspaceEdit parse_workspace_edit(const MiniJson::Value &edit)
    {
        LspWorkspaceEdit out;
        out.utf16_pending = true;
        if (!edit.is_object())
            return out;

        if (const MiniJson::Value *changes = edit.get("changes"); changes && changes->is_object())
        {
            for (const auto &[uri, arr] : changes->as_object())
            {
                if (!arr.is_array())
                    continue;
                std::vector<LspTextEdit> edits;
                for (const auto &te : arr.as_array())
                {
                    if (te.is_object())
                        edits.push_back(parse_text_edit_utf16(te));
                }
                if (!edits.empty())
                    out.changes[uri] = std::move(edits);
            }
        }

        if (const MiniJson::Value *docs = edit.get("documentChanges"); docs && docs->is_array())
        {
            for (const auto &dc : docs->as_array())
            {
                if (!dc.is_object())
                    continue;
                // TextDocumentEdit
                std::string uri;
                if (const MiniJson::Value *td = dc.get("textDocument"); td && td->is_object())
                    uri = td->get_string("uri", "");
                if (uri.empty())
                    continue;
                if (const MiniJson::Value *edits = dc.get("edits"); edits && edits->is_array())
                {
                    auto &dest = out.changes[uri];
                    for (const auto &te : edits->as_array())
                    {
                        if (te.is_object())
                            dest.push_back(parse_text_edit_utf16(te));
                    }
                }
            }
        }
        return out;
    }

    void flatten_document_symbol(
        const MiniJson::Value &sym,
        const std::string &uri,
        Buffer *buf,
        std::vector<LspSymbol> &out,
        const std::string &prefix)
    {
        if (!sym.is_object())
            return;
        LspSymbol item;
        item.name = sym.get_string("name", "");
        item.detail = sym.get_string("detail", "");
        if (const MiniJson::Value *k = sym.get("kind"); k && k->is_number())
            item.kind = k->as_int(0);

        // DocumentSymbol uses range/selectionRange; SymbolInformation uses location.
        if (const MiniJson::Value *loc = sym.get("location"); loc && loc->is_object())
        {
            item.location = parse_location(*loc, buf);
        }
        else
        {
            item.location.uri = uri;
            const MiniJson::Value *range = sym.get("selectionRange");
            if (!range)
                range = sym.get("range");
            if (range && range->is_object())
            {
                const MiniJson::Value *s = range->get("start");
                const MiniJson::Value *e = range->get("end");
                if (s && e)
                {
                    item.location.start = {
                        .line = s->get_int("line", 0),
                        .column = s->get_int("character", 0)};
                    item.location.end = {
                        .line = e->get_int("line", 0),
                        .column = e->get_int("character", 0)};
                }
            }
        }

        const std::string full = prefix.empty() ? item.name : prefix + " / " + item.name;
        item.location.display = std::format(
            "{}  {}:{}",
            full,
            item.location.start.line + 1,
            item.location.start.column + 1);
        out.push_back(item);

        if (const MiniJson::Value *children = sym.get("children"); children && children->is_array())
        {
            for (const auto &ch : children->as_array())
                flatten_document_symbol(ch, uri, buf, out, full);
        }
    }

    LspCodeAction parse_code_action(const MiniJson::Value &v)
    {
        LspCodeAction out;
        if (!v.is_object())
            return out;

        // Command-only (legacy)
        if (!v.get("title") && v.get("command") && v.get("command")->is_string())
        {
            out.title = v.get_string("command", "");
            out.has_command = true;
            out.command = out.title;
            return out;
        }

        out.title = v.get_string("title", "");
        out.kind = v.get_string("kind", "");
        out.is_preferred = v.get("isPreferred") && v.get("isPreferred")->as_bool(false);
        if (const MiniJson::Value *edit = v.get("edit"); edit && edit->is_object())
        {
            out.has_edit = true;
            out.edit = parse_workspace_edit(*edit);
        }
        if (const MiniJson::Value *cmd = v.get("command"); cmd && cmd->is_object())
        {
            out.has_command = true;
            out.command = cmd->get_string("command", "");
        }
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

    MiniJson::Object def_cap;
    def_cap["dynamicRegistration"] = jbool(false);
    def_cap["linkSupport"] = jbool(true);

    MiniJson::Object symbol_cap;
    symbol_cap["dynamicRegistration"] = jbool(false);
    symbol_cap["hierarchicalDocumentSymbolSupport"] = jbool(true);

    MiniJson::Object code_action_cap;
    code_action_cap["dynamicRegistration"] = jbool(false);
    code_action_cap["codeActionLiteralSupport"] = jobject({
        {"codeActionKind",
         jobject({{"valueSet",
                   jarray({jstr(""), jstr("quickfix"), jstr("refactor"), jstr("source")})}})},
    });

    MiniJson::Object rename_cap;
    rename_cap["dynamicRegistration"] = jbool(false);
    rename_cap["prepareSupport"] = jbool(false);

    MiniJson::Object text_document;
    text_document["synchronization"] = jobject(std::move(sync));
    text_document["completion"] = jobject(std::move(completion));
    text_document["publishDiagnostics"] = jobject({{"relatedInformation", jbool(false)}});
    text_document["definition"] = jobject(def_cap);
    text_document["declaration"] = jobject(def_cap);
    text_document["typeDefinition"] = jobject(def_cap);
    text_document["references"] = jobject({{"dynamicRegistration", jbool(false)}});
    text_document["documentSymbol"] = jobject(std::move(symbol_cap));
    text_document["rename"] = jobject(std::move(rename_cap));
    text_document["codeAction"] = jobject(std::move(code_action_cap));

    MiniJson::Object workspace;
    workspace["applyEdit"] = jbool(true);
    workspace["workspaceEdit"] = jobject({{"documentChanges", jbool(true)}});
    workspace["symbol"] = jobject({{"dynamicRegistration", jbool(false)}});

    MiniJson::Object caps;
    caps["textDocument"] = jobject(std::move(text_document));
    caps["workspace"] = jobject(std::move(workspace));

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
        if (rid == initialize_id_)
        {
            if (msg.get("result"))
            {
                send_initialized();
            }
            else if (const MiniJson::Value *err = msg.get("error"))
            {
                Logger::error(std::format(
                    "LSP initialize failed for {}: {}",
                    config_.language,
                    err->get_string("message", "unknown error")));
                state_ = LspSessionState::Failed;
                process_.stop();
            }
            return;
        }
        if (const MiniJson::Value *result = msg.get("result"); result && owner_)
            owner_->on_response(rid, *result);
        else if (msg.get("error") && owner_)
            owner_->on_response(rid, jnull());
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
        if (method == "workspace/applyEdit" && owner_)
        {
            if (const MiniJson::Value *p = msg.get("params"); p && p->is_object())
            {
                owner_->on_workspace_apply_edit(*p);
                (void)send_raw(rpc_.make_response(
                    *id,
                    jobject({{"applied", jbool(true)}})));
            }
            else
            {
                (void)send_raw(rpc_.make_response(
                    *id,
                    jobject({{"applied", jbool(false)}})));
            }
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

int LspSession::request_position(const std::string &method, const std::string &uri, int line, int character)
{
    if (state_ != LspSessionState::Running || !open_uris_.count(uri))
        return 0;
    const int id = rpc_.next_id();
    MiniJson::Object params;
    params["textDocument"] = jobject({{"uri", jstr(uri)}});
    params["position"] = jobject({
        {"line", jnum(static_cast<double>(line))},
        {"character", jnum(static_cast<double>(character))},
    });
    if (!send_raw(rpc_.make_request(id, method, jobject(std::move(params)))))
        return 0;
    return id;
}

int LspSession::request_references(const std::string &uri, int line, int character)
{
    if (state_ != LspSessionState::Running || !open_uris_.count(uri))
        return 0;
    const int id = rpc_.next_id();
    MiniJson::Object params;
    params["textDocument"] = jobject({{"uri", jstr(uri)}});
    params["position"] = jobject({
        {"line", jnum(static_cast<double>(line))},
        {"character", jnum(static_cast<double>(character))},
    });
    params["context"] = jobject({{"includeDeclaration", jbool(true)}});
    if (!send_raw(rpc_.make_request(id, "textDocument/references", jobject(std::move(params)))))
        return 0;
    return id;
}

int LspSession::request_document_symbol(const std::string &uri)
{
    if (state_ != LspSessionState::Running || !open_uris_.count(uri))
        return 0;
    const int id = rpc_.next_id();
    MiniJson::Object params;
    params["textDocument"] = jobject({{"uri", jstr(uri)}});
    if (!send_raw(rpc_.make_request(id, "textDocument/documentSymbol", jobject(std::move(params)))))
        return 0;
    return id;
}

int LspSession::request_workspace_symbol(const std::string &query)
{
    if (state_ != LspSessionState::Running)
        return 0;
    const int id = rpc_.next_id();
    MiniJson::Object params;
    params["query"] = jstr(query);
    if (!send_raw(rpc_.make_request(id, "workspace/symbol", jobject(std::move(params)))))
        return 0;
    return id;
}

int LspSession::request_rename(
    const std::string &uri,
    int line,
    int character,
    const std::string &new_name)
{
    if (state_ != LspSessionState::Running || !open_uris_.count(uri))
        return 0;
    const int id = rpc_.next_id();
    MiniJson::Object params;
    params["textDocument"] = jobject({{"uri", jstr(uri)}});
    params["position"] = jobject({
        {"line", jnum(static_cast<double>(line))},
        {"character", jnum(static_cast<double>(character))},
    });
    params["newName"] = jstr(new_name);
    if (!send_raw(rpc_.make_request(id, "textDocument/rename", jobject(std::move(params)))))
        return 0;
    return id;
}

int LspSession::request_code_action(
    const std::string &uri,
    int start_line,
    int start_character,
    int end_line,
    int end_character)
{
    if (state_ != LspSessionState::Running || !open_uris_.count(uri))
        return 0;
    const int id = rpc_.next_id();
    MiniJson::Object params;
    params["textDocument"] = jobject({{"uri", jstr(uri)}});
    params["range"] = jobject({
        {"start",
         jobject({
             {"line", jnum(static_cast<double>(start_line))},
             {"character", jnum(static_cast<double>(start_character))},
         })},
        {"end",
         jobject({
             {"line", jnum(static_cast<double>(end_line))},
             {"character", jnum(static_cast<double>(end_character))},
         })},
    });
    params["context"] = jobject({{"diagnostics", jarray({})}});
    if (!send_raw(rpc_.make_request(id, "textDocument/codeAction", jobject(std::move(params)))))
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
    pending_.clear();
    clear_ready();
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

fs::path LspService::uri_to_path(const std::string &uri)
{
    if (uri.empty())
        return {};
    std::string s = uri;
    if (s.rfind("file://", 0) == 0)
        s = s.substr(7);
    // file:///path → /path; file://localhost/path → skip host
    if (s.rfind("localhost", 0) == 0)
        s = s.substr(9);
    // Percent-decode minimal (%20 etc.)
    std::string out;
    out.reserve(s.size());
    for (std::size_t i = 0; i < s.size(); ++i)
    {
        if (s[i] == '%' && i + 2 < s.size())
        {
            auto hex = [](char c) -> int {
                if (c >= '0' && c <= '9')
                    return c - '0';
                if (c >= 'a' && c <= 'f')
                    return c - 'a' + 10;
                if (c >= 'A' && c <= 'F')
                    return c - 'A' + 10;
                return -1;
            };
            const int hi = hex(s[i + 1]);
            const int lo = hex(s[i + 2]);
            if (hi >= 0 && lo >= 0)
            {
                out.push_back(static_cast<char>((hi << 4) | lo));
                i += 2;
                continue;
            }
        }
        out.push_back(s[i]);
    }
    return fs::path(out);
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
    auto dit = documents_.find(id);
    if (dit == documents_.end())
        return;
    if (dit->second.open)
    {
        if (LspSession *session = session_for(dit->second.language))
            session->did_close(dit->second.uri);
    }
    for (auto pit = pending_.begin(); pit != pending_.end();)
    {
        if (pit->second.buffer_id == id)
            pit = pending_.erase(pit);
        else
            ++pit;
    }
    ready_completion_.reset();
    buffer.diagnostics() = {};
    documents_.erase(dit);
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

    pending_[id] = LspPendingRequest{
        .kind = LspPendingKind::Completion,
        .buffer_id = reinterpret_cast<std::uintptr_t>(&buffer),
        .doc_version = it->second.version,
        .trigger = {.line = line, .column = byte_col},
    };
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
    cancel_pending();
}

void LspService::clear_ready()
{
    ready_completion_.reset();
    ready_locations_.reset();
    ready_symbols_.reset();
    ready_rename_.reset();
    ready_actions_.reset();
    ready_server_apply_.reset();
}

void LspService::cancel_pending()
{
    std::lock_guard lock(mu_);
    pending_.clear();
    clear_ready();
}

int LspService::begin_position_request(
    Buffer &buffer,
    int line,
    int byte_col,
    LspPendingKind kind,
    const char *method)
{
    auto it = documents_.find(reinterpret_cast<std::uintptr_t>(&buffer));
    if (it == documents_.end() || !it->second.open)
        return 0;
    LspSession *session = session_for(it->second.language);
    if (!session || session->state() != LspSessionState::Running)
        return 0;

    const int u16 = utf16_on_line(buffer.lines(), line, byte_col);
    int id = 0;
    if (kind == LspPendingKind::References)
        id = session->request_references(it->second.uri, line, u16);
    else
        id = session->request_position(method, it->second.uri, line, u16);
    if (id <= 0)
        return 0;

    pending_[id] = LspPendingRequest{
        .kind = kind,
        .buffer_id = reinterpret_cast<std::uintptr_t>(&buffer),
        .doc_version = it->second.version,
        .trigger = {.line = line, .column = byte_col},
    };
    return id;
}

int LspService::request_definition(Buffer &buffer, int line, int byte_col)
{
    std::lock_guard lock(mu_);
    ready_locations_.reset();
    return begin_position_request(
        buffer, line, byte_col, LspPendingKind::Definition, "textDocument/definition");
}

int LspService::request_declaration(Buffer &buffer, int line, int byte_col)
{
    std::lock_guard lock(mu_);
    ready_locations_.reset();
    return begin_position_request(
        buffer, line, byte_col, LspPendingKind::Declaration, "textDocument/declaration");
}

int LspService::request_type_definition(Buffer &buffer, int line, int byte_col)
{
    std::lock_guard lock(mu_);
    ready_locations_.reset();
    return begin_position_request(
        buffer, line, byte_col, LspPendingKind::TypeDefinition, "textDocument/typeDefinition");
}

int LspService::request_references(Buffer &buffer, int line, int byte_col)
{
    std::lock_guard lock(mu_);
    ready_locations_.reset();
    return begin_position_request(
        buffer, line, byte_col, LspPendingKind::References, "textDocument/references");
}

int LspService::request_document_symbols(Buffer &buffer)
{
    std::lock_guard lock(mu_);
    ready_symbols_.reset();
    auto it = documents_.find(reinterpret_cast<std::uintptr_t>(&buffer));
    if (it == documents_.end() || !it->second.open)
        return 0;
    LspSession *session = session_for(it->second.language);
    if (!session || session->state() != LspSessionState::Running)
        return 0;
    const int id = session->request_document_symbol(it->second.uri);
    if (id <= 0)
        return 0;
    pending_[id] = LspPendingRequest{
        .kind = LspPendingKind::DocumentSymbol,
        .buffer_id = reinterpret_cast<std::uintptr_t>(&buffer),
        .doc_version = it->second.version,
    };
    return id;
}

int LspService::request_workspace_symbols(Buffer &buffer, const std::string &query)
{
    std::lock_guard lock(mu_);
    ready_symbols_.reset();
    auto it = documents_.find(reinterpret_cast<std::uintptr_t>(&buffer));
    if (it == documents_.end() || !it->second.open)
        return 0;
    LspSession *session = session_for(it->second.language);
    if (!session || session->state() != LspSessionState::Running)
        return 0;
    const int id = session->request_workspace_symbol(query);
    if (id <= 0)
        return 0;
    pending_[id] = LspPendingRequest{
        .kind = LspPendingKind::WorkspaceSymbol,
        .buffer_id = reinterpret_cast<std::uintptr_t>(&buffer),
        .doc_version = it->second.version,
    };
    return id;
}

int LspService::request_rename(Buffer &buffer, int line, int byte_col, const std::string &new_name)
{
    std::lock_guard lock(mu_);
    ready_rename_.reset();
    auto it = documents_.find(reinterpret_cast<std::uintptr_t>(&buffer));
    if (it == documents_.end() || !it->second.open)
        return 0;
    LspSession *session = session_for(it->second.language);
    if (!session || session->state() != LspSessionState::Running)
        return 0;
    const int u16 = utf16_on_line(buffer.lines(), line, byte_col);
    const int id = session->request_rename(it->second.uri, line, u16, new_name);
    if (id <= 0)
        return 0;
    pending_[id] = LspPendingRequest{
        .kind = LspPendingKind::Rename,
        .buffer_id = reinterpret_cast<std::uintptr_t>(&buffer),
        .doc_version = it->second.version,
        .trigger = {.line = line, .column = byte_col},
    };
    return id;
}

int LspService::request_code_actions(Buffer &buffer, Cursor start, Cursor end)
{
    std::lock_guard lock(mu_);
    ready_actions_.reset();
    auto it = documents_.find(reinterpret_cast<std::uintptr_t>(&buffer));
    if (it == documents_.end() || !it->second.open)
        return 0;
    LspSession *session = session_for(it->second.language);
    if (!session || session->state() != LspSessionState::Running)
        return 0;
    const int s16 = utf16_on_line(buffer.lines(), start.line, start.column);
    const int e16 = utf16_on_line(buffer.lines(), end.line, end.column);
    const int id = session->request_code_action(
        it->second.uri, start.line, s16, end.line, e16);
    if (id <= 0)
        return 0;
    pending_[id] = LspPendingRequest{
        .kind = LspPendingKind::CodeAction,
        .buffer_id = reinterpret_cast<std::uintptr_t>(&buffer),
        .doc_version = it->second.version,
        .trigger = start,
    };
    return id;
}

std::optional<LspLocationList> LspService::take_location_result()
{
    std::lock_guard lock(mu_);
    if (!ready_locations_)
        return std::nullopt;
    auto out = std::move(*ready_locations_);
    ready_locations_.reset();
    return out;
}

std::optional<LspSymbolList> LspService::take_symbol_result()
{
    std::lock_guard lock(mu_);
    if (!ready_symbols_)
        return std::nullopt;
    auto out = std::move(*ready_symbols_);
    ready_symbols_.reset();
    return out;
}

std::optional<LspRenameResult> LspService::take_rename_result()
{
    std::lock_guard lock(mu_);
    if (!ready_rename_)
        return std::nullopt;
    auto out = std::move(*ready_rename_);
    ready_rename_.reset();
    return out;
}

std::optional<LspCodeActionList> LspService::take_code_action_result()
{
    std::lock_guard lock(mu_);
    if (!ready_actions_)
        return std::nullopt;
    auto out = std::move(*ready_actions_);
    ready_actions_.reset();
    return out;
}

std::optional<LspWorkspaceEdit> LspService::take_server_apply_edit()
{
    std::lock_guard lock(mu_);
    if (!ready_server_apply_)
        return std::nullopt;
    auto out = std::move(*ready_server_apply_);
    ready_server_apply_.reset();
    return out;
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

void LspService::on_workspace_apply_edit(const MiniJson::Value &params)
{
    if (const MiniJson::Value *edit = params.get("edit"); edit && edit->is_object())
        ready_server_apply_ = parse_workspace_edit(*edit);
}

void LspService::on_response(int id, const MiniJson::Value &result)
{
    // Called from LspSession::on_message during pump() — mu_ already held.
    auto pit = pending_.find(id);
    if (pit == pending_.end())
        return;
    const LspPendingRequest pending = pit->second;
    pending_.erase(pit);

    auto dit = documents_.find(pending.buffer_id);
    Buffer *buf = nullptr;
    if (dit != documents_.end() && dit->second.open)
    {
        if (dit->second.version != pending.doc_version &&
            pending.kind != LspPendingKind::WorkspaceSymbol)
            return;
        buf = dit->second.buffer;
    }

    switch (pending.kind)
    {
    case LspPendingKind::Completion:
    {
        CompletionList list;
        list.request_id = id;
        list.buffer_id = pending.buffer_id;
        list.doc_version = pending.doc_version;
        list.trigger = pending.trigger;
        const MiniJson::Array *items = nullptr;
        if (result.is_array())
            items = &result.as_array();
        else if (result.is_object())
        {
            list.incomplete =
                result.get("isIncomplete") && result.get("isIncomplete")->as_bool(false);
            if (const MiniJson::Value *arr = result.get("items"); arr && arr->is_array())
                items = &arr->as_array();
        }
        if (items && buf)
        {
            list.items.reserve(items->size());
            for (const auto &it : *items)
            {
                if (it.is_object())
                    list.items.push_back(parse_completion_item(it, buf));
            }
        }
        ready_completion_ = std::move(list);
        break;
    }
    case LspPendingKind::Definition:
    case LspPendingKind::Declaration:
    case LspPendingKind::TypeDefinition:
    case LspPendingKind::References:
    {
        LspLocationList list;
        list.request_id = id;
        list.buffer_id = pending.buffer_id;
        list.doc_version = pending.doc_version;
        if (pending.kind == LspPendingKind::Declaration)
            list.kind = LspLocationList::Kind::Declaration;
        else if (pending.kind == LspPendingKind::TypeDefinition)
            list.kind = LspLocationList::Kind::TypeDefinition;
        else if (pending.kind == LspPendingKind::References)
            list.kind = LspLocationList::Kind::References;
        else
            list.kind = LspLocationList::Kind::Definition;
        collect_locations(result, buf, list.items);
        ready_locations_ = std::move(list);
        break;
    }
    case LspPendingKind::DocumentSymbol:
    case LspPendingKind::WorkspaceSymbol:
    {
        LspSymbolList list;
        list.request_id = id;
        list.buffer_id = pending.buffer_id;
        list.doc_version = pending.doc_version;
        list.workspace = (pending.kind == LspPendingKind::WorkspaceSymbol);
        const std::string uri = (buf && dit != documents_.end()) ? dit->second.uri : "";
        if (result.is_array())
        {
            for (const auto &sym : result.as_array())
                flatten_document_symbol(sym, uri, buf, list.items, "");
        }
        ready_symbols_ = std::move(list);
        break;
    }
    case LspPendingKind::Rename:
    {
        LspRenameResult rr;
        rr.request_id = id;
        rr.buffer_id = pending.buffer_id;
        rr.doc_version = pending.doc_version;
        if (result.is_object())
            rr.edit = parse_workspace_edit(result);
        ready_rename_ = std::move(rr);
        break;
    }
    case LspPendingKind::CodeAction:
    {
        LspCodeActionList list;
        list.request_id = id;
        list.buffer_id = pending.buffer_id;
        list.doc_version = pending.doc_version;
        if (result.is_array())
        {
            for (const auto &a : result.as_array())
            {
                auto action = parse_code_action(a);
                if (!action.title.empty())
                    list.items.push_back(std::move(action));
            }
        }
        ready_actions_ = std::move(list);
        break;
    }
    }
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
    pending_.clear();
    clear_ready();
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
