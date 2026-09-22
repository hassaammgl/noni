#include "buffer_detail.hpp"

using namespace buffer_detail;

void Buffer::apply_forward(const TextChange &change)
{
    if (!change.deleted.empty())
        raw_delete_text(change.start_line, change.start_col, change.deleted);
    if (!change.inserted.empty())
        (void)raw_insert_text(change.start_line, change.start_col, change.inserted);
}

void Buffer::apply_reverse(const TextChange &change)
{
    TextChange inv = change;
    std::swap(inv.deleted, inv.inserted);
    apply_forward(inv);
}

void Buffer::apply_transaction_forward(const UndoTransaction &txn)
{
    for (const auto &c : txn.changes)
        apply_forward(c);
}

void Buffer::apply_transaction_reverse(const UndoTransaction &txn)
{
    for (auto it = txn.changes.rbegin(); it != txn.changes.rend(); ++it)
        apply_reverse(*it);
}
