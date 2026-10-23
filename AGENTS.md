# AGENTS.md — Noni v2 rewrite (new/)

Pure vim-style ncursesw terminal editor. Rewritten from scratch in `new/`.
Analysis of the old code lives in `FEATURES.md`. Design decisions in
`ARCHITECTURE.md`, phase plan in `ROADMAP.md`, user-visible demo script in
`PREVIEW.md`. Never modify `old/`. Never add features that are not in `ROADMAP.md`.

## Toolchain facts (verified 2026-10-23)

- `g++ (GCC) 16.2.1` — full C++23 (expected, flat_map, print, move_only_function OK).
- CMake 4.4.4. `pkg-config`: `tree-sitter 0.26.9`, `ncursesw 6.6.20251230`.
- Linux + ncursesw; widechar build (`-DNCURSES_WIDECHAR`, pkg-config module `ncursesw`).

## Working rules (every later step)

1. **Before each PHASE**: post that phase's PREVIEW section (what you will see,
   exact keys to try, what's not working yet) in 10–15 lines and **wait for "GO"**.
2. **Per task**: implement exactly one ROADMAP task ID, then:
   `cmake --build build` → `ctest --test-dir build --output-on-failure` →
   sanitizer run (`ASAN_OPTIONS=detect_leaks=1 ./build/noni` for app tasks) →
   commit `"<TaskID>: <title>"`.
3. **After each PHASE**: tell me how to run + which keys to try, then wait.
4. Never touch `old/`. No `_partN` files. One cohesive `.hpp`/`.cpp` pair per module,
   no file-size limit — prefer cohesive files.
5. If something is unclear, or a C++23/library feature fails on our toolchain:
   **ask, don't guess.**
6. No new third-party deps without approval. Approved: ncursesw, tree-sitter (system),
   doctest (FetchContent), nlohmann/json (FetchContent). Git via CLI. No libgit2.

## Code conventions (C++23)

- Target `noni`, `-std=c++23`, `-Wall -Wextra -Wpedantic`; Debug adds
  `-fsanitize=address,undefined`.
- Layers are hard boundaries (`core/` includes nothing from `app/ui/…`; `ui/` has no
  editor logic; `lsp/ git/ syntax/ terminal/` never include ncurses or app headers).
- Types `PascalCase`, functions/methods `snake_case`, enums `CamelCase` values as
  members; files named after the primary type (`buffer.hpp/cpp`), `#pragma once`.
- Project includes are quoted (`"core/buffer.hpp"`), system includes `<...>`.
- No raw `new`/`delete`; `unique_ptr`/value semantics. No singletons.
- No exceptions across module boundaries: modules return
  `std::expected<T, Error>` (`Error{std::string msg; Source loc}` optional code).
  Exceptions allowed only inside main() only as a crash guard.
- Buffers/Windows are referenced by **id** (`BufferId`, `WindowId`), never by pointer,
  except inside the same object that owns them.
- Threads/background: workers are pure loops over immutable inputs; they return
  results by posting `Event`s (see ARCHITECTURE). Workers never touch editor state,
  never call ncurses, never hold editor locks.
- One `core/…` change ⇒ update/extend the matching `tests/…_test.cpp` in the same
  task. Core modules are unit-testable without ncurses.
- Public-API-first: write the header + its doctest skeleton first, then the .cpp.

## Commit / test discipline

- `ctest` is the only permitted merge gate; a phase is done when its "phase check"
  task passes and the PREVIEW script was demoed to the user.
- Commit message: `P<phase>.<task>: <short title>` (e.g. `P2.4: motions f/F/t/T/;/,`).
- If a task turns out bigger than ~100 lines, split it and add task IDs to ROADMAP.md
  (documented in the commit body).

## Definition of done (per task)

- Header + implementation + tests build clean under warnings.
- Every `ACCEPT:` test in its ROADMAP entry passes in `ctest`.
- Edge cases listed in its ROADMAP entry are covered by at least one test or an
  explicit deferred note (`TODO(P17): …`).
