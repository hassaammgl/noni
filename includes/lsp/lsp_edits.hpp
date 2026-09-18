#pragma once

#include <editor/buffer.hpp>
#include <editor/buffer_manager.hpp>
#include <lsp/lsp_models.hpp>

#include <functional>
#include <string>

namespace LspEdits
{
    // Apply WorkspaceEdit across open buffers (opens missing files).
    // Each buffer gets one undo transaction. Edits applied high→low.
    // Returns number of buffers touched.
    int apply_workspace_edit(
        BufferManager &buffers,
        LspWorkspaceEdit edit,
        const std::function<void(Buffer &)> &on_opened = {});
}
