#pragma once

#include <editor/buffer.hpp>
#include <syntax/syntax.hpp>
#include <utils/cursor.hpp>

#include <cstdint>
#include <string>
#include <vector>

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
