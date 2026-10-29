#pragma once
// P4.1 — operator composition: the `Pending` FSM behind
//     [count]operator [count]motion | text-object | the operator again
// plus the '"x' register prefix. Pure core: the FSM only BUILDS an `Edit`
// value; applying it to the buffer is the reducer's job (P4.2 delete/yank,
// P4.3 change, P4.6/P4.7/P4.8 shift/case/indent).
//
// API (ROADMAP P4.1):  std::optional<Edit> step(Pending&, const Key&, const TextView&)
// After the call the caller distinguishes the three outcomes by `p.phase`:
//   - an Edit was returned  -> complete; `p` is reset to Idle
//   - nullopt, phase != Idle -> still collecting keys
//   - nullopt, phase == Idle -> cancelled / failed (vim beeps, nothing happens)
#include <cstdint>
#include <optional>
#include <vector>

#include "core/key.hpp"
#include "core/motion.hpp"
#include "core/pos.hpp"

namespace noni {

enum class OperatorId : std::uint8_t {
    Delete,     // 'd'
    Yank,       // 'y'
    Change,     // 'c'
    ShiftRight, // '>'  (P4.6)
    ShiftLeft,  // '<'  (P4.6)
    AutoIndent, // '='  (P4.8)
    Upper,      // 'gU' (P4.7)
    Lower,      // 'gu' (P4.7)
    ToggleCase, // 'g~' (P4.7)
};

// Range flavor of a composed operator (named apart from undo.hpp's
// EditKind, which classifies committed text changes).
enum class EditScope : std::uint8_t { Charwise, Linewise };

// One composed operator command. Charwise ranges are [start, end) in codepoint
// positions; end.col may be `len(line)` (one past the last char, e.g. `d$`) and
// end.line may be `lines.size()` for a Linewise edit (one past the last line).
struct Edit {
    OperatorId op = OperatorId::Delete;
    EditScope scope = EditScope::Charwise;
    TextRange range{};
    std::uint32_t lines = 0;   // Linewise: number of whole lines affected
    char32_t reg = 0;          // '"x' prefix; 0 = the unnamed register
    bool enter_insert = false; // 'c' variants: switch to Insert after applying
    bool keep_indent = false;  // 'cc'/'S': reuse the first affected line's indent
};

// What the FSM may look at: the text (to resolve the motion) and the cursor
// the operator starts at. `tabwidth` feeds the display-column word motions.
struct TextView {
    const std::vector<std::string>* lines = nullptr;
    Pos cursor{};
    int tabwidth = 8;
};

struct Pending {
    enum class Phase : std::uint8_t {
        Idle,    // nothing armed
        WantReg, // '"' seen: the next key names the register
        Op,      // operator armed: waiting for [count]motion / object / itself
    };

    Phase phase = Phase::Idle;
    OperatorId op = OperatorId::Delete;
    std::uint32_t op_count = 0;     // [count] BEFORE the operator (0 = none)
    std::uint32_t motion_count = 0; // [count] AFTER it (0 = none)
    char32_t reg = 0;               // remembered '"x'
    // Sub-state of Phase::Op.
    MotionId await_char = MotionId::H; // f/F/t/T armed: waiting for the target
    bool want_char = false;
    bool g_armed = false; // 'g' seen: the next 'g' completes 'gg'
};

// Total repetition count of the composed command:
//   (op_count ?: 1) * (motion_count ?: 1)   — vim: 2d3w == 6w, 2dd == 2 lines,
//   2d2d == 4 lines, plain `d` == 1.
[[nodiscard]] std::uint32_t combined_count(const Pending& p);

// Consume one key. `p` is mutated in place; nullptr lines in `tv` cancels.
[[nodiscard]] std::optional<Edit> step(Pending& p, const Key& k,
                                       const TextView& tv);

} // namespace noni
