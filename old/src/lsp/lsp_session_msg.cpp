#include "lsp_internal.hpp"

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
                if (const MiniJson::Value *result = msg.get("result"))
                    apply_initialize_result(*result);
                send_initialized();
            }
            else if (const MiniJson::Value *err = msg.get("error"))
            {
                Logger::error(std::format(
                    "LSP initialize failed for {}: {}",
                    config_.language,
                    err->get_string("message", "unknown error")));
                Messages::error(std::format(
                    "LSP initialize failed: {}",
                    config_.language));
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
