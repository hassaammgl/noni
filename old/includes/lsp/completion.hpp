#pragma once

#include <utils/cursor.hpp>

#include <cstdint>
#include <string>
#include <vector>

struct CompletionItem
{
    std::string label;
    std::string detail;
    std::string documentation;
    std::string insert_text;
    int kind = 0;

    // If has_text_edit, use logical byte range replacement.
    bool has_text_edit = false;
    Cursor edit_start{.line = 0, .column = 0};
    Cursor edit_end{.line = 0, .column = 0};
    std::string new_text;
};

struct CompletionList
{
    bool incomplete = false;
    std::vector<CompletionItem> items;
    int request_id = 0;
    std::uintptr_t buffer_id = 0;
    int doc_version = 0;
    Cursor trigger{.line = 0, .column = 0};
};
