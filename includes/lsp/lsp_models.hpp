#pragma once

#include <utils/cursor.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

// UI-independent LSP navigation / edit models (UTF-8 byte Cursors after conversion).

struct LspLocation
{
    std::string uri;
    // Positions use LSP UTF-16 `character` until UI converts against the target Buffer.
    Cursor start{.line = 0, .column = 0};
    Cursor end{.line = 0, .column = 0};
    std::string display; // picker label
};

struct LspTextEdit
{
    Cursor start{.line = 0, .column = 0};
    Cursor end{.line = 0, .column = 0};
    std::string new_text;
};

struct LspWorkspaceEdit
{
    // uri → edits (already converted to byte columns when buffer was known at parse time;
    // for closed files, start/end stay UTF-16 until apply resolves the buffer).
    std::unordered_map<std::string, std::vector<LspTextEdit>> changes;
    bool utf16_pending = false; // true if edits still need UTF-16→byte against buffer lines
};

struct LspSymbol
{
    std::string name;
    std::string detail;
    int kind = 0;
    LspLocation location;
    std::vector<LspSymbol> children;
};

struct LspCodeAction
{
    std::string title;
    std::string kind;
    bool is_preferred = false;
    bool has_edit = false;
    LspWorkspaceEdit edit;
    // Server command (optional) — applied only when no edit, via controlled path later.
    bool has_command = false;
    std::string command;
    std::string command_args_json;
};

struct LspLocationList
{
    std::vector<LspLocation> items;
    int request_id = 0;
    std::uintptr_t buffer_id = 0;
    int doc_version = 0;
    enum class Kind
    {
        Definition,
        Declaration,
        TypeDefinition,
        References,
    } kind = Kind::Definition;
};

struct LspSymbolList
{
    std::vector<LspSymbol> items; // flattened for picker
    int request_id = 0;
    std::uintptr_t buffer_id = 0;
    int doc_version = 0;
    bool workspace = false;
};

struct LspCodeActionList
{
    std::vector<LspCodeAction> items;
    int request_id = 0;
    std::uintptr_t buffer_id = 0;
    int doc_version = 0;
};

struct LspRenameResult
{
    LspWorkspaceEdit edit;
    int request_id = 0;
    std::uintptr_t buffer_id = 0;
    int doc_version = 0;
};

enum class LspPickerKind
{
    Locations,
    Symbols,
    CodeActions,
    InstallServers,
};
