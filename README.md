# Noni

A small, fast, **Vim-style terminal text editor** written in modern C++23.
Noni is an editor, not an IDE: it does modal editing, search, syntax highlighting
and file navigation, and leaves git, terminals and build tools to the tools that
already do them well (tmux, lazygit, your shell).

## Features

- Modal editing: Normal, Insert, Visual, Visual-Line, Visual-Block, Command-line
- Counts, operators and motions that compose: `3dw`, `c2f)`, `>ip`, `gUiw`
- Text objects: `iw aw ip ap i" a" i( a( i{ a{ i[ a[`
- Registers, marks, jump list, `.` repeat, macros (`q` / `@`), undo/redo
- Windows and tabs: `:sp`, `:vs`, `Ctrl-w` commands, `:tabnew`, `gt`/`gT`
- Search and replace: `/ ? n N * #`, `:s///`, `:%s///gc`
- Ex commands: `:w :q :wq :e :b :bn :bd :ls :set :<number> :!cmd`
- Fuzzy file picker (`<space>ff`), project grep (`<space>fg`), simple explorer (`<space>e`)
- Syntax highlighting via tree-sitter (C, C++, Python, Rust, ...)
- Autosave, crash recovery, session restore
- One small `config.json`

## Not included (on purpose)

Git integration, embedded terminal, plugin system, mouse support, GUI.
LSP is planned as an optional late addition.

## Build

Requirements: GCC 14+ (or any compiler with C++23 `<print>` and `<expected>`),
CMake 3.28+, ncursesw, tree-sitter.

    cmake -B build -DCMAKE_BUILD_TYPE=Debug
    cmake --build build
    ctest --test-dir build --output-on-failure
    ./build/noni [file]

Debug builds enable AddressSanitizer and UBSan.

## Quick start

| Keys | Action |
|---|---|
| `i` / `Esc` | Insert mode / back to Normal |
| `h j k l`, `w b e`, `0 $`, `gg G` | Move |
| `dd`, `dw`, `ciw`, `yy`, `p` | Delete, change, yank, paste |
| `u` / `Ctrl-r` | Undo / redo |
| `/text` `n` `N` | Search |
| `:w` `:q` `:wq` | Save / quit |
| `:sp` `:vs` `Ctrl-w h/j/k/l` | Split windows and move |
| `<space>ff` / `<space>fg` / `<space>e` | Find file / grep / explorer |

## Configuration

`~/.config/noni/config.json` (falls back to the current directory, then defaults):

    { "tabwidth": 4, "expandtab": true, "number": true,
      "relativenumber": true, "scrolloff": 5 }

Runtime changes with `:set`, e.g. `:set relativenumber`.

## Project layout

    src/core/   pure editing logic: buffer, undo, motions, operators, registers, ex parser
    src/app/    event loop, reducer, config, background workers
    src/ui/     ncurses rendering only
    src/syntax/ tree-sitter highlighting
    tests/      doctest unit tests and scripted key-session tests

Rules: `core/` never includes ncurses or threads; all state changes go through one
reducer; the main thread never blocks. See `ARCHITECTURE.md`.

## Development

Work is organised in small tasks (`ROADMAP.md`), each with acceptance tests.
`PREVIEW.md` shows what each phase looks like. Vim behaviour is checked against
`nvim --headless` in differential tests; Neovim's source is used as a reference
only, never copied.

## License

TBD