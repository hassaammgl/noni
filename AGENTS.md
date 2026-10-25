# AGENTS.md — Noni v2 rewrite (new/)

Pure vim-style ncursesw terminal editor. Rewritten from scratch in `new/`.
Analysis of the old code lives in `FEATURES.md`. Design decisions in
`ARCHITECTURE.md`, phase plan in `ROADMAP.md`, user-visible demo script in
`PREVIEW.md`. Never modify `old/`. Never add features that are not in `ROADMAP.md`.

Scope (revised 2026-10-23, v2): **ONLY git integration is removed** (no git
module/worker/events/signs/keys, and the git binary is never invoked; file scan
uses a built-in skip list + basic `.gitignore` parsing). Everything else stays:
built-in terminal (PTY + VT + `:term` + copy mode) at P14, `:!cmd` shell jobs,
OSC52 clipboard write (async, no subprocesses), LSP at P13 (isolated: nothing
outside P13 depends on it), explorer, picker, grep, syntax, config, extras,
session/recovery/autosave. Syntax parsing is main-thread incremental (visible
range only). Background work = file scan + grep (job pool) + terminal PTY worker
+ `:!` jobs + OSC52 writes.

## Toolchain facts (verified 2026-10-23)

- `g++ (GCC) 16.2.1` — C++23 (portable subset only, see rules below).
- CMake 4.4.4. `pkg-config`: `tree-sitter 0.26.9`, `ncursesw 6.6.20251230`.
- Linux + ncursesw; widechar build (`-DNCURSES_WIDECHAR`, pkg-config `ncursesw`).
- Neovim available locally as a read-only semantic oracle (`nvim --headless -u NONE`).

## Working rules (every later step)

1. **Before each PHASE**: post that phase's PREVIEW section (what you will see,
   exact keys to try, what's not working yet) in 10–15 lines and **wait for "GO"**.
2. **Per task**: implement exactly one ROADMAP task ID, then:
   `cmake --build build` → `ctest --test-dir build --output-on-failure` →
   sanitizer run (`ASAN_OPTIONS=detect_leaks=1 ./build/noni` for app tasks) →
   commit `"<TaskID>: <title>"`.
3. **After each PHASE**: tell me how to run + which keys to try, then wait.
4. Never touch `old/`. No `_partN` files. One cohesive `.hpp`/`.cpp` pair per
   module, no file-size limit — prefer cohesion.
5. If something is unclear, or a C++23/library feature fails on our toolchain:
   **ask, don't guess.**
6. No new third-party deps without approval. Approved: ncursesw, tree-sitter
   (system), doctest (FetchContent), nlohmann/json (FetchContent).
   NO libgit2, NO git CLI anywhere, NO wl-copy/xclip/xsel subprocesses.

## C++23 rules — PORTABLE SUBSET

- **No modules, no `import std`;** plain headers + `#include` only.
- **No coroutines, no `std::generator`;** no eager-pattern machinery — the event
  loop is plain functions.
- **`std::expected` everywhere errors flow** (`core/pos.hpp eresult<T>`);
  **`std::println`/`std::print`** for any stdout output; `std::format` for text.
- `[[nodiscard]]` on all functions returning status/values that must not be
  ignored (eresult, getters with side effects).
- **RAII wrappers for every C-library resource**: ncurses `WINDOW` (custom
  deleter), `FILE*`, raw fds, tree-sitter deallocators
  (`ts_delete_tree`/`ts_query_delete`/parser). No bare handles held in structs.
- No exceptions anywhere (see next section); no `new`/`delete` (RAII/containers
  only); no singletons.

### Exceptions policy — resolved decision: NO exceptions ANYWHERE

- No `throw`/`try`/`catch` in `new/` code (including tests). Modules return
  `std::expected<T, Error>`; invariant violations inside a module are internal
  preconditions — tested with asserts (asserts stay enabled in Debug) and hard
  prechecks that return errors, not crashes.
- `P1.5 with_edit` rethrow case is therefore RESOLVED: callbacks must not throw;
  an early return / error state closes the undo group consistently (test
  `undo.session.error_inside_group`).
- `noexcept` on moves/wrappers where realistic; do not chase `noexcept` on the
  whole API.
- Library-internal throws (e.g. `std::bad_alloc`, nlohmann internal) are treated
  as fatal environment conditions; only the outermost `main()` wrapper catches
  them purely to print context before aborting — no cross-layer catch/resume.

### Differential testing vs Neovim (editing-semantics oracle)

- For EVERY motion / operator / text-object task (P2.7 basics; P3/P4/P5; and
  their polish tasks P15 — J/gJ, visual u/U): add differential tests whose
  expected values are recorded from a real `nvim --headless -u NONE` run
  (read-only reference). Never copy Neovim source code — observe behavior only.
- When observed behavior and our result differ: **Neovim is right**. Match its
  behavior; document any deliberate deviation (rare, e.g. no `:help` docs) in the
  task notes.
- Differential fixtures live in `new/tests/diff/` (input key strings + expected
  positions/text), generated once per task and committed.

## Code conventions (C++23)

- Target `noni`, `-std=c++23`, `-Wall -Wextra -Wpedantic`; Debug adds
  `-fsanitize=address,undefined`.
- Layers are hard boundaries (`core/` includes nothing from `app/ui/…`; `ui/`
  has no editing logic; `ext/`/`syntax/` never include ncurses or app headers).
- Types `PascalCase`, functions/methods `snake_case`; files named after the
  primary type (`buffer.hpp/cpp`); `#pragma once`.
- Project includes quoted (`"core/buffer.hpp"`); system includes `<...>`.
- Buffers/Windows referenced by **id** (`BufferId`, `WindowId`), never by
  pointer, except inside the object that owns them.
- Threads/background: jobs are pure loops over immutable inputs; results are
  posted as `Event`s. Jobs never touch editor state, never call ncurses.
- One `core/…` change ⇒ extend the matching `tests/…` file in the same task.
  Core modules are unit-testable without ncurses.
- Public-API-first: write the header + its doctest skeleton first, then the .cpp.

## Commit / test discipline

- `ctest` is the only permitted merge gate; a phase is done when its "phase
  check" task passes and the PREVIEW script was demoed to the user.
- Commit message: `P<phase>.<task>: <short title>` (e.g. `P3.3: motions f/F/t/T/;/,`).
- If a task turns out bigger than ~100 lines, split it and add task IDs to
  ROADMAP.md (noted in the commit body). Renumbering phases requires scope
  change approval from the user (as done 2026-10-23).

## Definition of done (per task)

- Header + implementation + tests build clean under warnings.
- Every `ACCEPT:` test in its ROADMAP entry passes in `ctest`.
- For motion/operator/text-object tasks: differential fixtures vs
  `nvim --headless -u NONE` pass.
- Edge cases listed in its ROADMAP entry are covered by at least one test or an
  explicit deferred note (`TODO(P<n>): …`).
