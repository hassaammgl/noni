#include "lsp_internal.hpp"

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
        Messages::warning(std::format(
            "LSP failed to launch `{}` (check logs/lsp.stderr.log)",
            config_.command[0]));
        return false;
    }

    MiniJson::Object sync;
    sync["didSave"] = jbool(true);
    sync["dynamicRegistration"] = jbool(false);
    sync["willSave"] = jbool(false);

    MiniJson::Object hover_cap;
    hover_cap["dynamicRegistration"] = jbool(false);
    hover_cap["contentFormat"] = jarray({jstr("plaintext"), jstr("markdown")});

    MiniJson::Object formatting_cap;
    formatting_cap["dynamicRegistration"] = jbool(false);

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
    text_document["hover"] = jobject(std::move(hover_cap));
    text_document["formatting"] = jobject(std::move(formatting_cap));
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
    pending_opens_.clear();
    initialized_sent_ = false;
    caps_sync_kind_ = 2;
    caps_hover_ = false;
    caps_formatting_ = false;
    exit_announced_ = false;
}

void LspSession::pump()
{
    const std::string chunk = process_.take_stdout();
    if (!chunk.empty())
        rpc_.feed(chunk);
    if (!process_.alive() &&
        (state_ == LspSessionState::Running || state_ == LspSessionState::Starting))
    {
        if (!exit_announced_)
        {
            exit_announced_ = true;
            Logger::error(std::format("LSP server exited: {}", config_.language));
            Messages::error(std::format("LSP crashed: {} (see logs/lsp.stderr.log)", config_.language));
        }
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
    // Spec: no textDocument/* until after initialize + initialized.
    for (const auto &doc : pending_opens_)
        flush_did_open(doc);
    pending_opens_.clear();
}

void LspSession::apply_initialize_result(const MiniJson::Value &result)
{
    if (!result.is_object())
        return;
    const MiniJson::Value *caps = result.get("capabilities");
    if (!caps || !caps->is_object())
        return;
    caps_sync_kind_ = json_sync_kind(caps->get("textDocumentSync"));
    caps_hover_ = json_provider_on(caps->get("hoverProvider"));
    caps_formatting_ = json_provider_on(caps->get("documentFormattingProvider"));
    Logger::info(std::format(
        "LSP caps {}: sync={} hover={} format={}",
        config_.language,
        caps_sync_kind_,
        caps_hover_ ? "yes" : "no",
        caps_formatting_ ? "yes" : "no"));
}

void LspSession::flush_did_open(const PendingOpen &doc)
{
    open_uris_.insert(doc.uri);
    MiniJson::Object td;
    td["uri"] = jstr(doc.uri);
    td["languageId"] = jstr(doc.language_id);
    td["version"] = jnum(static_cast<double>(doc.version));
    td["text"] = jstr(doc.text);
    MiniJson::Object params;
    params["textDocument"] = jobject(std::move(td));
    (void)send_raw(rpc_.make_notification("textDocument/didOpen", jobject(std::move(params))));
}
