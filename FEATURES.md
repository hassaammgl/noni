# Noni (old) — Feature & Design Analysis for the `new/` Rewrite

Analysis date: 2026-10-23.
Basis: full sweep of `old/` (source `old/src/`, `old/includes/`, `old/noni.cpp`,
`old/Makefile`, `old/config.json`, `old/src/editor/*`, `old/src/lsp/*`, `old/src/ui/*`
spot-reads) plus the author's own code-verified guide `old/sys.txt` (marked
"LAST VERIFIED AGAINST CODE: 2026-09-26") and `old/PROGRESS.md`. Claims checked directly
in code where they matter are cited with file/function; anything known only from
`sys.txt` is labeled "(sys)". No `old/` file was modified.

Task-assumption corrections:
- The prompt said "libgit2" — the codebase does **not** link or use libgit2. Git features
  run the `git` CLI via `popen` (`old/src/utils/git.cpp`). `old/PROGRESS.md` T0 documents
  the deliberate drop of `-lgit2`; no `#include <git2.h>` exists.
- There are **no tests** (removed in T0, `old/PROGRESS.md`; Makefile has no test target).

---

# 1. Overview

## 1.1 Product

A keyboard-first, single-threaded-UI ncursesw TUI code editor with: vim-style modal
editing, LSP integration (clangd/pylsp/…), tree-sitter highlighting with auto-install of
grammars, git status/diff gutters (git CLI), an embedded PTY terminal, `config.json`
keybindings for workbench-style commands, per-workspace session persistence, crash
recovery, autosave, and an in-app extension event bus (one builtin demo extension).

Entry: `old/noni.cpp` `main` → `Logger::init` → `Args::get_file_path(argv[1])` →
`UI(path)::run` → shutdown.

## 1.2 Build system — `old/Makefile` (handwritten; no CMake/pkg-config)

- `g++ -std=c++23 -Wall -Wextra -g -MMD -MP -Iincludes -DNCURSES_WIDECHAR=1 -O2`
  (libstdc++ feature-check macros recorded in `old/PROGRESS.md`).
- Link: `-lncursesw -pthread -lutil -ltree-sitter -ldl` (no libgit2).
- Sources: all root `*.cpp` + `find src -name '*.cpp'`; objects/deps under `build/`.
- Targets: `all` (default), `debug` (`-O0`), `lto` (`-O2 -flto=auto`; built OK per
  `old/PROGRESS.md` T7.8), `run`, `clean`.
- ~250 implementation files, ~26,000 lines in `old/src/` (+ mirrored `old/includes/`),
  consequence of a hard 200-line-per-file rule (`old/.cursor/rules/file-size.mdc`)
  which produced mechanical `_part2/_part3/detail` file splits.

## 1.3 Dependencies

| dependency | used for |
|---|---|
| ncursesw | entire TUI (wide chars, colors, `define_key`, `set_escdelay`) — `old/src/ui/ui_init.cpp` |
| pthread | Logger writer thread, Background worker pool (`old/src/utils/logger.cpp`, `old/src/utils/async.cpp`) |
| libutil | `forkpty` for the terminal panel (`old/src/terminal/pty_session.cpp`) |
| tree-sitter | highlight parsing (`old/src/syntax/tree_highlighter.cpp`) |
| dl | `dlopen` of grammar `.so` files (same file) |
| subprocesses (git CLI, wl-copy/xclip/xsel, curl, tar, cc/git-clone, pip/npm/go) | SCM, clipboard, grammar & LSP installers |

## 1.4 Module map (one line each, under `old/src/` + `old/includes/` mirror)

| module | purpose |
|---|---|
| `root noni.cpp` | `main`, CLI arg (exactly `argv[1]` only) |
| `ui/` (16 cpp, ~5.3k lines) | ncurses bootstrap, poll() main loop, layout/resize, dirty-render, `Focus` routing, key dispatch/when-context, all workbench `register_command`s, ex commands, session management, LSP result routing, SCM UI glue, config load, recovery prompts |
| `components/` (~28 cpp, ~5.7k lines) | ncurses widgets: Editor (vim view), Sidebar, TabBar, Header, LineNumber gutter, CommandLine, Confirm/Input prompts, File/Buffer/Lsp/Completion pickers, SearchPanel, MessagesPanel, TerminalPanel, HoverPopup |
| `editor/` (~2.7k lines) | non-ncurses engine: Buffer (doc+undo+syntax+diagnostics+mtime), BufferManager, EditorTab (mode+WindowLayout), Window (cursor/scroll/selection), WindowLayout (split tree), Motion (header-only), Selection, Marks, Registers, Jumps, BufferSearch, ExCommands |
| `lsp/` (~3.1k lines) | LSP client: JSON-RPC framing, process (fork/exec pipes), session/state machine, requests (completion/def/refs/rename/symbols/codeAction/format), diagnostics decode, auto-installer (pip/npm/curl/go) |
| `scm/` (~800 lines) | `ScmGit` (porcelain/diff via git CLI), `ScmDiff` (unified `-U0` parser), `ScmService` (async cache on Background pool) |
| `syntax/` (~2.5k lines) | `Syntax` fallback lexer per language; `TreeHighlighter` (dlopen grammar + tree-sitter, embedded queries); `GrammarInstaller` (download + cc → .so) |
| `terminal/` (~1.2k lines) | `PtySession` (forkpty), `VtParser` (CSI/SGR/OSC), `TerminalScreen` (grid + scrollback), `TerminalSession` glue |
| `configs/` (~1.3k lines) | `mini_json` (own JSON parser), `config.cpp` (AppConfig), `keybindings*.cpp` (chord engine: parse/raw-mapping/when/pending) |
| `commands/` | `CommandRegistry` (CommandId → fn) + `InputContext`/`ResolveStatus` (`includes/commands/command.hpp`) |
| `extensions/` | EditorEvents bus, ExtensionContext, ExtensionManager, builtin demo `noni.hello` |
| `help/` | `:help` pages (`HelpDocs::build`) |
| `sidebar/` | `DirScanner` (async tree scan on Background) |
| `workspace/` | root detection, `SessionStore` (`session.json`), `RecoveryStore` (crash drafts), `RecentFiles` |
| `utils/` | Logger, Messages, FS (atomic write), Git (popen), Clipboard (wl-copy/xclip/xsel/OSC52), Fuzzy+FileIndex, TextSearch (async grep), TextMetrics (UTF-8/UTF-16/display), Background pool, EventWake, FsWatcher (inotify), str helpers, args |
| `old/config.json(.bak)` | shipped 83-key config; `keybinds.txt` = notes only (never loaded) |

Files kept out of the app: `old/keymap-catalog.md` (nvim/vscode cross-check notes —
partially stale), `old/sys.txt` (architecture doc), `old/PROGRESS.md` (work queue),
`old/simpletodo.txt` (split tracker), `old/.cursor/rules/*` (author rules), `old/.clangd`.

## 1.5 Runtime shape

Single main thread (`old/src/ui/ui_loop.cpp`): `poll()` on stdin + EventWake + inotify +
PTY master + every LSP stdout; LSP/PTY output is pumped on main (no reader threads —
T7.1); only Logger has its own thread; 2 Background workers run dir scanning, file
indexing, project grep, SCM refresh, grammar/LSP installs. Rendering paints only dirty
ncurses windows (`old/src/ui/ui_render*.cpp`).

---

# 2. Feature inventory

Legend: **Complete / Partial / Stub / Broken-looking / Unknown** — with one line of
evidence each.

## 2.1 Editing

| # | Feature | Lives in | Status | Evidence |
|---|---|---|---|---|
| 1 | Line-based buffer, UTF-8 text, revision + `content_id` dirty tracking | `includes/editor/buffer.hpp:29-44`, `src/editor/buffer.cpp` | Complete | `content_id` vs `saved_content_id` |
| 2 | File load with real LoadError state (never silently blanked) | `buffer.hpp:19-23 BufferLoadState`, `buffer.cpp load()` | Complete | save() refuses on `LoadError` (`buffer_io.cpp:69-76`) |
| 3 | Atomic save (temp file → rename) | `src/utils/fs.cpp` (`FS::write_file`, `.noni.XXXXXX` temp at `fs.cpp:55`) | Complete | rename-based |
| 4 | External-change detection (disk mtime captured on load/save) | `buffer.hpp:54-56`, `disk_changed()` | Complete | flag consumed by autosave (sys T6.7) |
| 5 | Save / save-as (`:w path`, untitled→named) | `src/editor/buffer_io.cpp:61-120`, `src/ui/ui_ex.cpp ex_write` | Complete | save_as retargets path |
| 6 | Undo/redo — per-Buffer transaction stack, `kMaxUndo=256`, txn stores cursor before/after | `buffer.hpp:40-44`, `buffer_io.cpp:5-59 undo()/redo()` | Complete | stacks + `apply_transaction_reverse/forward` |
| 7 | Begin/end edit transaction grouping (typing, arrows split txns) | `buffer.hpp:144-146`, `src/components/editor_insert.cpp` `split_insert_edit` | Complete | save()/recovery close any open txn (`buffer_io.cpp:78-79`) |
| 8 | Insert/delete char/text, newline, delete range | `src/editor/buffer_insert.cpp`, `buffer_delete.cpp` | Complete | used by editor ops |
| 9 | Registers (unnamed + a–z + `"`), pending register via `"` prefix | `includes/editor/registers.hpp` (header-only), `editor.cpp` pending_register | Complete | `RegisterFile` |
| 10 | Marks a–z, set `m`, jump `'x` | `includes/editor/marks.hpp`, `editor_normal.cpp:113-118` | Complete | MarkTable invalidate_buffer |
| 11 | Jump list (push on big moves, Ctrl+O back / Tab forward) | `includes/editor/jumps.hpp`, `src/components/editor_cursor.cpp:41-56` pushes on >1-line moves | Complete | cap 100 (sys PART R) |
| 12 | Comment toggle `gcc` / Visual `gc` / `ctrl+/` | `includes/editor/comment_tokens.hpp`, `src/components/editor_cmds_edit.cpp`, `ui_commands_keymap.cpp` | Complete | verified T6.2 (`PROGRESS.md`) |
| 13 | Move/copy lines: `alt+j/k/up/down`, `ctrl+e` | `ui_commands_keymap.cpp` (copyLinesDown / moveLines/Up/Down) | Complete | T6.6 verified |
| 14 | Read/write clipboard: wl-copy → xclip → xsel, OSC52 base64 fallback on stdout; read wl-paste/xclip/xsel | `src/utils/clipboard.cpp` (`Clipboard::read/write/write_osc52`) | Complete | but **blocking subprocesses on main thread** → see §5 |
| 15 | Clipboard sync yank/copy/cut/paste when `clipboard.sync` | `src/components/editor_yank.cpp:20-24`, `editor_edit.cpp:44-57`, `Editor::clipboard_sync_` (`includes/components/editor.hpp:34`) | Complete | |
| 16 | Paste `p/P` char/linewise from registers | `src/components/editor_ops.cpp:65-129 paste_register` | Complete | handles Line vs Character registers |
| 17 | Paste from system clipboard (`ctrl+v`, KEY_SIC in insert) | `editor_ops.cpp` / `editor_edit.cpp` | Complete | blocking (see §5) |
| 18 | Autosave `afterDelay` (default 1000ms; skips untitled + external-mtime conflict; never formats) | `ui_features.cpp` tick + `config.hpp AutosaveConfig` | Complete | T6.7 verified |
| 19 | Format-on-save via LSP (2s deadline) + manual format | `src/ui/ui_tabs.cpp:85-110` | Partial | **spin-waits on UI thread up to 2 s** → §5 issue 2 |
| 20 | UTF-8 typing: `get_wch` codepoints → `insert_focused_utf8` routing | `src/ui/ui_input.cpp:22-62`, `editor_insert.cpp:126-137 insert_utf8` | Complete | T1.7 verified |
| 21 | Insert-mode exit `jk` (deletes the pending `j`; old `jj` removed in T4) | `src/components/editor_insert.cpp:8-27,108-117 pending_j` | Complete | |
| 22 | Recovery snapshot apply | `buffer_io.cpp:122-146 apply_recovered_content` | Complete | keeps dirty so snapshots never auto-overwrite files |
| 23 | No auto-indent / smart indent on Enter or joins | `buffer_insert.cpp raw_insert_newline`, `editor_insert.cpp:93-100` | **Stub** | no indentation computation anywhere (grep: no indent logic in insert path) |

## 2.2 Vim modes

| # | Feature | Lives in | Status | Evidence |
|---|---|---|---|---|
| 1 | Modes Normal / Insert / Visual / VisualLine | `includes/editor/editor_tab.hpp:12-18`, `src/components/editor_mode.cpp`, dispatch in `Editor::handle_input` | Complete | |
| 2 | Motions h/j/k/l/arrows/w/b/e/0/^/$/G | `includes/editor/motion.hpp` (+ `motion_word*.hpp`, `motion_line.hpp`, `motion_basic.hpp`, `motion_word_back/end.hpp`), `src/components/editor_normal.cpp:14-131`, `editor_impl.hpp:133-190 motion_from_key` | Partial | **no `f/F/t/T` find-char, no `r/R` replace, no `%` match, no counts** (`3w`) — motion namespace only has word/line/file motions |
| 3 | Operators `d`/`c`/`y` + motion (`dw`, `ciw`, …), `dd/cc/yy` linewise | `src/components/editor_normal_prefix.cpp` (pending_op) + `editor_ops.cpp` | Partial | path exists; only Delete/Change/Yank ops (`PendingOperator` enum in `editor.hpp:19-24`) — no `gu/gU`, `>`, `<` |
| 4 | Insert entry: i/a/I/A/o/O | `editor_normal_edit.cpp` → `enter_insert_mode` | Complete | |
| 5 | Visual selection model (anchor on Window, char/line kinds, normalize to byte ranges) | `includes/editor/selection.hpp`, `includes/editor/window.hpp:62-116` | Complete(-ish) | semantics described in §5 issue 1 |
| 6 | Visual ops d/x/c/s/y/p/P + motions + scroll + `gg` + `gc` | `src/components/editor_visual.cpp` (all of it, 102 lines) | Partial | **no `o`/`O` cursor-flip, no `<`/`>` indent, no `S` surround** |
| 7 | `x` delete, `s/S/C`, `p/P` | `src/components/editor_normal_edit.cpp`, `editor_ops.cpp` | Complete | |
| 8 | `u`/`ctrl+r` undo/redo; `:u`/`:redo` | `editor_normal_edit.cpp`, `ex_commands.cpp` | Complete | also `ctrl+z`/`ctrl+y` config (see §5 issue 4) |
| 9 | `"/register` prefix, `m`/`'` marks, pending g/bracket prefixes | `editor_normal_prefix.cpp` + pending flags `editor.hpp:22-32` | Complete | |
| 10 | `:` → CommandLine; `/` `?` search entry; `n/N` | `editor_normal.cpp:76-103,119-122`, `src/ui/ui_ex.cpp:4-31 execute_command` | Complete | `/`+`?` route through CommandLine prompt, not ex |
| 11 | `K` LSP hover, `gd/gD/gy/gr/gi` LSP jumps, `g t/g T` tabs, `g g` file-start, `g c` comment | `editor_normal_prefix.cpp`, callbacks `editor.hpp:40-48` | Complete | wired in `ui.cpp` `Editor::bind` |
| 12 | `[d`/`]d` diagnostic jump with wrap | `editor_normal_prefix.cpp` (`pending_bracket_`) | Complete | T6.4 verified |
| 13 | Insert mode: Backspace/Delete/Tab/Enter/arrows/Page keys; Backspace at col 0 merges lines | `editor_insert.cpp:29-124` | Partial | merge loses indent handling (§5 issue 5) |
| 14 | Insert mode redo `ctrl+r` hardcoded | `editor_insert.cpp` (raw 18 per sys PART T3) | Complete | |
| 15 | Terminal-mode key pass-through & DECCKM app-cursor | `src/components/terminal_panel_part2.cpp:102+`, `ui_input.cpp` Terminal branch | Complete | |
| 16 | Mouse | — | **Stub** | no mousemask/KEY_MOUSE anywhere (grep) |

## 2.3 Navigation

| # | Feature | Lives in | Status | Evidence |
|---|---|---|---|---|
| 1 | Splits (vertical/horizontal), focus traversal, resize ±, close group | `includes/editor/window_layout.hpp` (header-only binary tree), `src/editor/window_layout*.cpp`, commands `workbench.action.splitEditorRight/Down`, `focus*Group`, `increase/decreaseView*` in `src/ui/ui_commands_nav.cpp` | Complete | rects computed per draw (`editor_draw.cpp:90-120`) |
| 2 | Tab strip UI + per-tab mode | `src/components/tab_bar.cpp`, `src/ui/ui_tabs.cpp sync_active_tab` | Complete | |
| 3 | Buffer lifecycle (open file/untitled, switch, next/prev, close with save-confirm) | `src/editor/buffer_manager*.cpp`, `src/ui/ui_tabs.cpp` | Complete | BufferManager owns `unique_ptr<Buffer>` + tabs; destroy only when unreferenced (sys) |
| 4 | Goto line `ctrl+g` → `:NNN` | `src/ui/ui_commands_keymap.cpp:12-19 GotoLine`, ex parser `src/editor/ex_commands.cpp:36-72` | **Broken-looking** | command opens `:` + hint, but parse **skips leading digits** ("range tokens ... future use", `ex_commands.cpp:40-46` → same for `%`, `.`, `$`) and then early-returns silently on empty name (`ex_commands.cpp:121-122`) — typing a number does **nothing** |
| 5 | FilePicker fuzzy-open (`ctrl+p`, `space f f`, `space space`) | `src/components/file_picker(.cpp,_part2.cpp)`, `src/utils/fuzzy(_index|_detail).cpp FileIndex` | Complete | async rebuild + version guard |
| 6 | BufferPicker (`space f b`, `:buffers`) | `src/components/buffer_picker(.cpp,_part2.cpp)` | Complete | |
| 7 | Recent-files picker (`space f r`) | `src/workspace/recent_files.cpp`, `ui_commands_keymap.cpp` | Complete | |
| 8 | LspPicker (locations/symbols/actions/install-catalog) | `src/components/lsp_picker(.cpp,_part2.cpp)`, `src/ui/ui_lsp.cpp` | Complete | |
| 9 | Sidebar explorer tree: lazy dirs, expand/collapse, open file, reveal active | `src/components/sidebar*.cpp` (7 files), `src/sidebar/dirscanner*.cpp` | Complete | `reveal_path` in `sidebar_nav.cpp:54-101` |
| 10 | Sidebar CRUD: add file/folder, rename, delete (confirm), cut/copy/paste, refresh | `sidebar_fs.cpp`, `sidebar_clip.cpp`, `sidebar_input.cpp`, `sidebar_nav.cpp`, `src/ui/ui_sidebar_delete.cpp`, `ui_sidebar_prompt.cpp` | Complete | uses FS helpers |
| 11 | Workspace root detect (git root → marker walk), `:workspace`/openWorkspace | `src/workspace/workspace.cpp`, `src/ui/ui_workspace.cpp`, ex `:workspace` | Complete | marker list at `workspace.cpp` (sys G6) |

## 2.4 Search / Replace

| # | Feature | Lives in | Status | Evidence |
|---|---|---|---|---|
| 1 | In-buffer search `/` `?` + match set + `n/N` + highlight current match | `src/editor/buffer_search*.cpp` (`BufferSearch::find_all` regex ECMAScript / literal / case / whole-word, `buffer_search.cpp:37-118`), core in `src/editor/editor_core.cpp search_start/next` (`editor_core.cpp:46+`), highlight `editor_draw_overlay.cpp:85-127` | Complete | |
| 2 | Search UI search-panel side view with Query/Replace fields, F6/F7/F8 (case/whole-word/regex), Tab cycle, Results list with open-match/ReplaceAll | `src/components/search_panel(.cpp,_part2,_part3).cpp`, `src/ui/ui_search.cpp`, focus routing in `ui_input.cpp` | Complete | SearchText vs SearchResults contexts (T1.3) |
| 3 | Live search while typing in Query field | `search_panel.cpp` run_search on each printed key | Complete | |
| 4 | Project-wide async grep (skip VCS/vendor/binary dirs, background pool, cancel token) | `src/utils/text_search*.cpp` (`TextSearch::search_async`), skip list `text_search.cpp:7-30` | Complete | |
| 5 | `replaceOne` / `replaceAll` in active buffer | `src/ui/ui_commands_nav.cpp:56-77`, `src/editor/editor_core.cpp replace_*` | Partial | single-buffer only; no multi-file replace UI |
| 6 | `space f w` grep-word-under-cursor | `src/ui/ui_commands_keymap.cpp:63` | **Stub** | only `Messages::info("PHASE 5 …")` (sys PART AD) |

## 2.5 Syntax highlighting

| # | Feature | Lives in | Status | Evidence |
|---|---|---|---|---|
| 1 | Fallback lexer per language (Plain/MD/C/C++/Python/Rust/Go/JS/TS/JSON/Shell/Lua/Java/C#) | `src/syntax/syntax.cpp` (+ `syntax_generic/_kw/_kw_cxx/_kw_more/_md/_scan_*.cpp`) | Complete | `Syntax::detect_language` maps ext→Language (sys Y) |
| 2 | Tree-sitter highlighting with embedded per-language queries | `src/syntax/tree_highlighter*.cpp`, `ts_hl_detail(.hpp|.cpp)`, `ts_queries.hpp` | Partial | queries for C/C++, Python, JS(+TS), Rust, Bash, JSON, Lua, Markdown; **Go/Java: grammar installed but no query → lexer only (`sys` Y); C#: nothing** |
| 3 | Incremental re-parse (ts_tree_edit per change) + viewport-limited sync | `tree_highlighter.cpp notify_edit/sync` (~357-738), `Buffer::sync_syntax` (`buffer.cpp:156`), paint calls sync with visible rows (`editor_draw.cpp:20`) | Complete | dirty_from tracking + pending edits |
| 4 | Grammar lookup chain `/usr/lib/tree_sitter` → `~/.local/share/noni/tree-sitter` → `<ws>/.noni/tree-sitter` → user_dir → `/tmp` | `src/syntax/grammar_installer.cpp find_grammar` (~619-648) | Complete | sys Y |
| 5 | Grammar auto-install: curl tarball or git clone fallback + `cc` shared build, retry 60 s, logs to `logs/grammar-install.log` | `grammar_installer.cpp` + `grammar_download/_compile/_tools/_request/_state.cpp`, trigger `set_auto_install(config)` | Complete | runs on Background pool |
| 6 | Theme: 90 color pairs, init_color customization when terminal supports | `src/ui/theme.cpp:159-297` (`make_color` 8/256/custom), `includes/ui/theme.hpp` | Complete | fallback chain |
| 7 | `syntaxAutoInstall` flag (default true) | `config.cpp` bool-only check | Complete | disabled install = silent no-grammar |

## 2.6 LSP

| # | Feature | Lives in | Status | Evidence |
|---|---|---|---|---|
| 1 | JSON-RPC framing (Content-Length), request/notification/response demux | `src/lsp/lsp_jsonrpc.cpp`, `lsp_session_msg.cpp` | Complete | unknown server requests → `-32601` (`lsp_session_msg.cpp:78`) |
| 2 | Process layer: fork/exec pipes, stderr → `logs/lsp.stderr.log`, non-blocking stdout pumped on main, SIGTERM→1 s wait→SIGKILL | `src/lsp/lsp_process.cpp` (start 20-106, stop 108-146, write_all 153-172) | Complete-as-designed | **write_all is a blocking loop on the UI thread** (§5 issue 2) |
| 3 | Session state machine Idle/Starting/Running/Failed/Stopped, restart on crash, pending didOpen while Starting | `src/lsp/lsp_service*.cpp`, `lsp_session*.cpp` | Complete | (sys X; T1.6) |
| 4 | didOpen/didChange (0 none / 1 full / 2 incremental UTF-16)/didSave/didClose, `open_uris_` registry | `lsp_session_doc.cpp`, `lsp_service.cpp did_change_incremental` | Complete | multi-line change degrades to full resend (sys X; noted gap) |
| 5 | Completion (ctrl+space) with CompletionList → CompletionPicker → `apply_completion` | `lsp_session_req.cpp request_completion`, `lsp_result.cpp`, `src/ui/ui_completion.cpp`, `editor.cpp apply_completion` | Complete | |
| 6 | Definition/Declaration/TypeDefinition/References/Symbols/Rename | `lsp_session_req.cpp`, parse in `lsp_parse_item.cpp`/`lsp_parse_edit.cpp`, routing `src/ui/ui_lsp.cpp` | Complete | version-gated pending map (drop stale responses) |
| 7 | Code actions (quickFix) — **edit-only** | `ui_lsp.cpp:184` + `includes/lsp/lsp_models.hpp:53` | Partial/Stub | actions with only `command` are logged, never executed |
| 8 | WorkspaceEdit application (textDocument/edit only; CreateFile/RenameFile/DeleteFile skipped) | `src/lsp/lsp_edits.cpp`, `lsp_models.hpp` | Partial | documentChanges limited to TextDocumentEdit (sys X) |
| 9 | Diagnostics publish → version-gated Buffer snapshot → gutter + strike/underline overlay + `[d ]d` + `space x x` picker | `lsp_dispatch_diag.cpp`, `includes/lsp/diagnostics.hpp`, `buffer.hpp:116-124 set_diagnostics`, `line_number.cpp`, `editor_draw_overlay.cpp:129+` | Complete | |
| 10 | Hover `K` → HoverPopup | `editor_normal.cpp:123-126`, `src/components/hover_popup.cpp` | Complete | T6.3 |
| 11 | Formatting request + apply (+ formatOnSave) | `lsp_service.cpp` request_formatting/take_format_result, `ui_tabs.cpp:85-110` | Partial | 2 s blocking poll on main (§5 issue 2) |
| 12 | `$ /cancelRequest` never sent; no pending-request timeout | `lsp_service.cpp cancel_pending` (`sys` X) | Partial | cancel only clears local map |
| 13 | Server auto-installer: pylsp (pip venv), tsserver (npm/bun prefix), clangd (curl GitHub zip 19.1.2), gopls (go), rust-analyzer (curl zip); dirs `~/.local/share/noni/lsp` → `<ws>/.noni/lsp` → `/tmp/noni-lsp-<uid>`; `:lsp` picker + `lsp.restart/showStatus`, status bar log | `src/lsp/lsp_installer*.cpp` (+ `_detail.hpp`), `src/components/lsp_picker.cpp`, `src/ui/ui_lsp.cpp` | Partial | 5-server catalog; servers without `lsp.servers[].language` install but never auto-spawn (sys K) |
| 14 | rootMarkers: walk file dir upward, else workspace | `lsp_service.cpp session_for` (~950-1008) | Complete | C/Cpp fallback `"cpp"`/`"c"` |

## 2.7 Git (SCM) — via git CLI (no libgit2)

| # | Feature | Lives in | Status | Evidence |
|---|---|---|---|---|
| 1 | Repo detection (`<root>/.git` exists) + popen wrapper with shell-quoting | `src/utils/git.cpp` (`shell_quote` `git.cpp:3-16`, `run/run_ok`), `includes/utils/git.hpp` | Complete | |
| 2 | Status porcelain parse (branch, file states) | `src/scm/scm_git(.cpp,_part2.cpp)`, `includes/scm/scm_types.hpp` | Complete for stated scope | no upstream/HEAD-tracking features |
| 3 | Diff → gutter marks (added/modified/deleted) | `src/scm/scm_diff.cpp parse_unified` (`-U0`), `scm_service.cpp request_file_diff`, cache `ui.hpp:91-95 scm_diff_cache_`, draw `line_number.cpp:69-80` + `Theme::Gutter*` | Complete | file-level only |
| 4 | Async refresh on Background + generation counters, diff cache invalidation by path/lines/gen | `src/scm/scm_service(.cpp,_part2.cpp)`, `ui.cpp sync_scm_ui/diff` | Complete | |
| 5 | Commands: showStatus / showDiff / refresh(+Diff) / stage / unstage / discard (confirm) | `src/ui/ui_commands_nav.cpp:85-112`, handlers `ui_scm.cpp scm_stage_active` etc. | Partial | **stage/unstage/discard run git synchronously on the main thread**, then request_refresh (`sys` G5) — see §5 issue 11 |
| 6 | Hunk preview / hunk-level stage | `ui_commands_keymap.cpp:66-68` | **Stub** | `space g p git.previewHunk` → `Messages::info("PHASE 5 …")` only; hunk nav listed as PHASE 5 in `sys` AD |
| 7 | Branch badge (Header) + SCM badge (statusbar) | `src/components/header.cpp`, `statusbar.cpp set_scm_badge` | Complete | |

## 2.8 Terminal panel

| # | Feature | Lives in | Status | Evidence |
|---|---|---|---|---|
| 1 | forkpty shell session (config `terminal.shell` → `$SHELL` → `/bin/sh`) | `src/terminal/pty_session(.cpp,_part2.cpp)` | Complete | |
| 2 | VT parser: CSI dispatch, SGR color/attrs, OSC, DECCKM app-cursor keys | `src/terminal/vt_parser(.cpp,_csi.cpp,_sgr.cpp)`, `TerminalScreen::application_cursor_keys` | Partial | no test coverage; sys treats as verified-by-code only |
| 3 | Screen grid + scrollback ring (default 5000, min 0), scroll keys | `src/terminal/terminal_screen(.cpp,_part2,_part3).cpp`, `src/components/terminal_panel_part2.cpp` | Complete | |
| 4 | Panel draw + focus/leave semantics (Esc → PTY, Ctrl+] raw 29 / `ctrl+t` leave; resolved engine keys dispatched) | `src/components/terminal_panel(.cpp,_part2.cpp)`, `src/ui/ui_input.cpp` Terminal branch | Complete | T1.4 verified design |
| 5 | Key forwarding: enter/bs/del/arrows (SS3 vs CSI by DECCKM)/home/end/tab/PgUp-PgDn/F1-12 (xterm SS3/CSI)/ctrl bytes/printable/8-bit meta (UNVERIFIED path) | `terminal_panel_part2.cpp:102+`, `sys` T3 Terminal table | Complete | 8-bit-meta path flagged unverified by author |
| 6 | Terminal commands: toggle/clear/kill/focus/scrollUp/scrollDown | `src/ui/ui_commands.cpp:125-152` | Complete | no respawn-on-exit UI (Unknown UX beyond panel dying) |
| 7 | Mouse | — | **Stub** | none (grep) |
| 8 | Sixel / image protocols | — | **Stub** | no support |

## 2.9 UI / Layout

| # | Feature | Lives in | Status | Evidence |
|---|---|---|---|---|
| 1 | Screen composition: Header, TabBar, Sidebar/Search side, LineNumber gutter, editor area (splits), bottom CommandLine/Confirm/Input row, Terminal band, Statusbar | `src/ui/ui_layout.cpp` resize + `ui_init.cpp` | Complete | layout numbers in `ui.cpp` (sys C/AA) |
| 2 | `Focus` enum routing with per-widget `handle_input` hard-coded dispatch; overlay Esc/Enter shortcuts | `src/ui/ui_input.cpp` (big switch), `ui_types.hpp` | Complete | 12 focus kinds (`sys` T2) |
| 3 | Dirty-bitmap render: per-component `mark_dirty`, painted-state mirror, `force_paint_all` on resize, single `doupdate` | `src/ui/ui_render.cpp` + `_dirty.cpp` + `_paint.cpp`, mirrors `ui.hpp:100-115` | Complete | T7.2; widgets must call mark_dirty themselves (late bug fix sys AA) |
| 4 | Config-driven engine keybindings + when-flags recalculated from focus+mode each key | `src/configs/keybindings*.cpp`, `when_context` in `ui_sidebar.cpp` | Complete | `sys` J/T |
| 5 | Chord prefix display + no-timeout pending semantics | `statusbar.cpp set_chord`, `keybindings.cpp resolve_pending` | Complete | deliberate (sys T3) |
| 6 | HoverPopup (floating LSP hover widget) | `src/components/hover_popup.cpp` | Complete | |
| 7 | Messages ring + `:messages` panel + statusbar echo TTL | `src/utils/messages.cpp`, `src/components/messages_panel.cpp`, `statusbar.cpp set_echo` | Complete | |
| 8 | `:logs` tail (noni/lsp/install/grammar) | `ui_ex.cpp ex_logs` | Complete | E149 on unknown key |
| 9 | `:help` pages + topic list | `src/help/help_docs.cpp` + `help_pages_core/_more.cpp` | Complete | incl. `:commands` page |
| 10 | Command palette-ish? — none; `:h ex` page lists ex commands | — | **Stub** | no ctrl+shift+s command palette; workbench.command-aliases only in help |
| 11 | Session save/restore (`session.json`, tabs+cursor+scroll+visibilities; untitled skipped), `space s s/s w` | `src/workspace/session.cpp`, `src/ui/ui_session.cpp`, commands `ui_commands_nav.cpp:149-156` | Complete | |
| 12 | Crash recovery snapshots (every ≥2 s while dirty; startup prompts) | `src/workspace/recovery.cpp`, `src/ui/ui_recovery.cpp`, tick in render | Complete | |
| 13 | RecentFiles MRU + `space f r` | `src/workspace/recent_files.cpp`, `ui_commands_keymap.cpp:61-62` | Complete | |
| 14 | FsWatcher (inotify, non-blocking, drain on main, debounced sidebar refresh) | `src/utils/fs_watcher(.cpp,_part2.cpp)` | Complete | no watcher thread |
| 15 | DirScanner + sidebar tree (async on Background, version snapshot swap) | `src/sidebar/dirscanner*.cpp` | Complete | |
| 16 | File icons (Codicon-ish static glyph tables per extension) | `src/ui/icons_file_part1..5.cpp`, `icons_lookup.cpp`, header-only `includes/ui/icons*.hpp` | Complete | ~large static data |
| 17 | Window split separators drawing | `editor_draw.cpp:94-113` | Complete | adjacent-rect detection |
| 18 | INVALID DIMENSIONS guard | `ui.cpp` resize | Complete | |
| 19 | `UIComponent` WINDOW wrapper (create/resize/clear/dtor) | `src/ui/UIComponent.cpp` | Complete | |
| 20 | Lazy `window_layout` header-only split tree | `includes/editor/window_layout.hpp` | Complete | |

## 2.10 Config

| # | Feature | Lives in | Status | Evidence |
|---|---|---|---|---|
| 1 | config.json load order `$PWD` → exe dir → `~/.config/noni`; first existing regular file wins; parse fail → defaults + statusbar echo; never writes back | `src/configs/config.cpp`, `src/ui/ui_init.cpp load_config` | Complete | (sys S, T1.1) |
| 2 | JSON parser (rolled by hand) | `src/configs/mini_json(.cpp,_parse.cpp)` | Partial | **no `\uXXXX` decoding** (`mini_json_parse.cpp:119-138` copies the escape's raw char; cases like `\"` handled but `\u` not decoded) and **no numeric exponent** (`parse_number):3-5` handles only optional `-` and one fraction) — matters for LSP payloads |
| 3 | Keybinding engine: chord tokenizer, when-clause `&&`/`||` tokens, first-match-wins, leader (Space) rules, pending prefix w/o timeout, engine ownership of ctrl/alt/F/Esc/Space in editor modal contexts | `src/configs/keybindings(_part2,_raw,_ext).cpp` + `keybindings_detail.hpp engine_owns_token` | Complete | T4/T5 verified |
| 4 | Startup diagnostic of unbound/unknown bindings (statusbar + logs) | `src/commands/command_registry.cpp diagnose` + UI ctor | Complete | |
| 5 | Esc semantics: empty-when `escape→noni.mode.normal`; Terminal special-cased to let Esc reach PTY (leave via Ctrl+] or `ctrl+t`) | `config.json`, `src/ui/ui_input.cpp` (Terminal branch handles ch==27 & raw 29 before resolve) | Complete | T1.4 verified |

## 2.11 Other

| # | Feature | Lives in | Status | Evidence |
|---|---|---|---|---|
| 1 | Background job pool (2 workers, stale-token invalidation) | `src/utils/async.cpp`, per-module next-token guards | Complete | |
| 2 | EventWake (eventfd/pipe) wakeups | `src/utils/event_wake.cpp` | Complete | |
| 3 | Logger (writer thread, 2-line format, truncate on init) | `src/utils/logger.cpp` (init truncate at `logger.cpp:124`) | Complete | |
| 4 | `NONI_KEYLOG=1` raw-key tracing | `ui_input.cpp:57` | Complete | |
| 5 | Atomic FS write / path helpers | `src/utils/fs*.cpp` | Complete | |
| 6 | Fuzzy matcher + FileIndex (async rebuild + version) | `src/utils/fuzzy(.cpp,_index.cpp)` | Complete | T7.7 perf done |
| 7 | TextMetrics (UTF-8 decode, display widths incl. tabs, UTF-16 ↔ byte) | `src/utils/text_metrics(.cpp,_part2.cpp)` | Complete | used everywhere incl. LSP |
| 8 | Fd-based `poll` main loop | `src/ui/ui_loop.cpp` | Complete | T7.1 |
| 9 | Extension API (EditorEvents bus, register command/keybinding, per-extension config) | `src/extensions/*` | Partial | one demo extension `noni.hello` (`extension.hello.ping` unbound) |
| 10 | CLI args | `src/utils/args.cpp` | Complete | only `argv[1]` used |
| 11 | `Author note files` (sys.txt/PROGRESS.md/simpletodo.txt/keymap-catalog.md/keybinds.txt) — documentation only, never loaded | — | Complete-as-notes | `sys` H: keybinds.txt is not loaded by the app |
| 12 | `workbench.view.explorer/.search`; `workbench.action.openWorkspace` | `src/ui/ui_commands.cpp:73-81 etc.` | Complete | side/terminal/workspace commands |

---

# 3. Keybindings (full table)

Two layers, `sys` PART J: engine chords come **only** from `config.json`
(`AppConfig::defaults().keybindings = {}` — empty in code); everything else is
hard-coded in components. When-flags are:
`editorFocus`, `sidebarFocus`, `searchFocus`, `commandFocus`, `messagesFocus`,
`fileSearchFocus`, `bufferSearchFocus`, `lspPickerFocus`, `completionFocus`,
`confirmFocus`, `promptFocus`, `terminalFocus`, `normalMode`, `insertMode`,
`visualMode`, `sidebar(Hidden)Visible`, `terminal(Hidden)Visible` — computed in
`UI::when_context()` (`src/ui/ui_sidebar.cpp`) and matched in
`KeybindingEngine::when_matches` (`keybindings.cpp:303-342`).

## 3.1 `config.json` → commands (63 entries; file: `old/config.json`)
"leader" below = when string
`editorFocus && normalMode || editorFocus && visualMode || sidebarFocus || searchFocus`.

| key | command | when |
|---|---|---|
| escape | noni.mode.normal | (empty — all) |
| ctrl+s | workbench.action.files.save | editorFocus |
| ctrl+p | workbench.action.quickOpen | editorFocus |
| ctrl+b | workbench.action.toggleSidebarVisibility | editorFocus |
| ctrl+t | workbench.action.terminal.toggle | editorFocus |
| ctrl+z | editor.action.undo | editorFocus |
| ctrl+y | editor.action.redo | editorFocus |
| ctrl+f | actions.find | editorFocus |
| ctrl+g | workbench.action.gotoLine | editorFocus |
| ctrl+space | editor.action.triggerSuggest | editorFocus |
| f2 | editor.action.rename | editorFocus |
| f12 | editor.action.revealDefinition | editorFocus |
| ctrl+/ | editor.action.commentLine | editorFocus |
| ctrl+a | editor.action.selectAll | editorFocus |
| ctrl+c | editor.action.clipboardCopyAction | editorFocus |
| ctrl+x | editor.action.clipboardCutAction | editorFocus |
| ctrl+v | editor.action.clipboardPasteAction | editorFocus |
| ctrl+e | editor.action.copyLinesDownAction | editorFocus |
| alt+j / alt+down | editor.action.moveLinesDownAction | editorFocus |
| alt+k / alt+up | editor.action.moveLinesUpAction | editorFocus |
| ctrl+d | editor.action.deleteLines | editorFocus && insertMode |
| ctrl+d | editor.action.halfPageDown | editorFocus && normalMode |
| ctrl+u | editor.action.halfPageUp | editorFocus && normalMode |
| ctrl+left/right/up/down | workbench.action.focusLeft/Right/Above/BelowGroup | editorFocus && normalMode |
| space space / space f f | workbench.action.quickOpen | leader |
| space f g | workbench.action.findInFiles | leader |
| space f w | noni.search.grepWord (**stub**) | leader |
| space f b | workbench.action.showAllEditors | leader |
| space f r | noni.search.recent | leader |
| space e | workbench.action.toggleSidebarVisibility | leader |
| space o | noni.focus.sidebar | leader |
| space b n / space b b | workbench.action.next/previousEditor | leader |
| space b d | workbench.action.closeActiveEditor | leader |
| space w | workbench.action.files.save | leader |
| space q q | workbench.action.quit | leader |
| space r n | editor.action.rename | leader |
| space c a | editor.action.quickFix | leader |
| space c f | editor.action.formatDocument | leader |
| space l d | editor.action.revealDefinition | leader |
| space l r | editor.action.goToReferences | leader |
| space l s | workbench.action.gotoSymbol | leader |
| space l w | workbench.action.showAllSymbols | leader |
| space l R | lsp.restart | leader |
| space l S | lsp.showStatus | leader |
| space x x | noni.diagnostics.list | leader |
| space g s | git.showStatus | leader |
| space g a | git.stage | leader |
| space g u | git.unstage | leader |
| space g x | git.discard | leader |
| space g d | git.showDiff | leader |
| space g f | git.refreshDiff | leader |
| space g R | git.refresh | leader |
| space g p | git.previewHunk (**stub**) | leader |
| space t t | workbench.action.terminal.toggle | leader |
| space s s | workbench.action.restoreSession | leader |
| space s w | workbench.action.saveSession | leader |

Engine plumbing: `src/configs/keybindings.cpp/_part2/_raw/_ext.cpp` +
`includes/commands/command.hpp` (`context_is_literal`, `context_allows_leader`) +
`engine_owns_token` (`keybindings_detail.hpp:17`, T4). from_raw mapping table
(`27=Esc`, `13=Enter` via `nonl`, `10=ctrl+j`, `8=ctrl+h`, `127=Backspace`,
F1-24, ctrl bytes 1-26, raw 28/29/31) lives in `keybindings_raw.cpp`.

## 3.2 Hard-coded vim/panel keys (`.cpp`)

Normal mode — `src/components/editor_normal.cpp`,
`editor_normal_prefix.cpp`, `editor_normal_edit.cpp`:

| key(s) | action |
|---|---|
| h j k l w b e 0 ^ $ G arrows | motions |
| d c y (+motion or doubled) | delete/change/yank operators (`PendingOperator`) |
| dd/cc/yy | linewise |
| " r | register prefix |
| m x / ' x | mark set / jump |
| g g / g t / g T | buffer top / next tab / prev tab |
| g d g D g y g r g i | LSP definition/declaration/typedef/references/last-insert |
| g c | comment toggle (also `gcc`) |
| [ d / ] d | prev/next diagnostic (wrap) |
| v / V | visual char/line |
| i a I A o O | insert |
| x / s / S / C | delete-char/subst/change-line/change-eol |
| p / P | paste after/before |
| u / ctrl+r | undo / redo |
| / ? | search forward/backward (opens CommandLine) |
| n / N | next/prev match |
| : | open CommandLine |
| K | hover |
| ctrl+u / ctrl+d / PgUp / PgDn | half/full page |
| Tab / ctrl+I | jump forward |
| ctrl+O | jump back |

Visual mode — `src/components/editor_visual.cpp`:
`d x` delete, `c s` change, `y` yank, `p P` paste (leaves visual), any motion key
extends selection, `v` `V` toggle kind, `g g` jump to top, `g c` comment, ctrl+u/d
and PgUp/PgDn scroll, `esc` leaves. **No `o`/`O`, no `<` `>`.**

Insert mode — `src/components/editor_insert.cpp`
`jk` exit (pending_j), `esc` exit, arrows/Page keys move (split txn), Backspace
(merge on col 0), Delete, Tab `\t`, Enter, `ctrl+r` redo, `ctrl+d` deleteLines
(engine; when-gated by config), KEY_SIC clipboard paste, printable insert (`j`
arms pending_j).

/widgets — each component's `handle_input` (files listed in §2 per widget):
CommandLine (`commandline.cpp`, Enter/Esc/Backspace/printable),
FilePicker, BufferPicker, LspPicker, CompletionPicker (Enter/Tab accept,
Esc cancel, arrows wrap, PgUp/Dn),
SearchPanel (`search_panel*.cpp`: F6/F7/F8 toggles, Tab/Shift-Tab field cycle,
Enter per field, `r` ReplaceAll in results, `q`/Esc leave),
MessagesPanel (j/k/PgUp/PgDn/g/G),
ConfirmPrompt (y/n/c/Esc), InputPrompt (Enter/Esc/Backspace/printable),
Sidebar (`sidebar.cpp:293-420` + `sidebar_nav.cpp`: h/l/Enter open/expand,
j/k/arrows/PgUp/Dn/g/G, `a`/`A` add, `r` rename, `d` delete, `x c p` clipboard,
`R` refresh, `q` back),
Terminal (`terminal_panel_part2.cpp` + `ui_input.cpp` Terminal branch:
Esc→PTY, 29 Ctrl+] leave, Ctrl+t/`ctrl+t` config toggle, arrows etc.),
HoverPopup (passive).

---

# 4. Config options (`old/config.json` + defaults in `src/configs/config.cpp/hpp`)

| key | type | default (code) | shipped (old/config.json) | effect |
|---|---|---|---|---|
| appName | string | `"noni"` (config.hpp) | `"noni"` | stored only; no reader found (grep) |
| sidebarWidth | int | 25 | 25 | explorer column width (search side forces min 36) |
| lineNumberWidth | int | 5 (code) | 7 | gutter column count |
| escDelayMs | int | 25 | 25 | `set_escdelay` (Esc vs CSI) |
| syntaxAutoInstall | bool | true (bool-only check) | true | background grammar install |
| keybindings | array | `{}` (empty) | 63 entries | engine chord→command table |
| lsp.autoInstall | bool | true | true | missing servers auto-install |
| lsp.servers[].language | string | — | `"cpp"`, `"python"` | maps to `Syntax` language names (+ C/C++ fallback) |
| lsp.servers[].command | string[] | — | `["clangd"]`, `["pylsp"]` | server argv; argv[0] path-resolved |
| lsp.servers[].rootMarkers | string[] | — | e.g. `.git`, `compile_commands.json` | root-walk triggers |
| terminal.shell | string | `""` → `$SHELL` → `/bin/sh` | `""` | PTY shell |
| terminal.height | int | 12 | 12 | terminal rows (clamped 5…half screen) |
| terminal.scrollback | int | 5000 | 5000 | screen scrollback lines |
| extensions.* | object-map | `{}` | `noni.hello: {greet: "Welcome back"}` | ExtensionContext config lookup |
| formatOnSave | bool | true (T6 default) | true | LSP format (≤2 s wait) on save |
| clipboard.sync | bool | true (ClipboardConfig) | true | yank/copy/cut/paste → system clipboard |
| autosave.mode | `"off"` \| `"afterDelay"` | `"afterDelay"` | `"afterDelay"` | autosave behavior |
| autosave.delayMs | int | 1000 | 1000 | autosave delay |

Not in config but hardcoded data dirs: `~/.local/share/noni/{lsp,tree-sitter}`,
`<workspace>/.noni/{session.json,recovery/,tree-sitter/,lsp/}`, `/tmp/noni-*-<uid>`
fallbacks; `XDG_DATA_HOME` is ignored (sys H/S).

---

# 5. Known problems (root causes; DESIGN vs BUG verdict)

Sources: the two examples you cited ("Visual mode selection galat hai",
"LSP freeze karta hai") plus issues auto-derived from `old/PROGRESS.md` UNVERIFIED
list, `old/sys.txt` PART AD/UNVERIFIED, and direct code reads in this pass.

1. **Visual mode selection misbehaves** — **DESIGN** (+2 concrete gaps).
   - Selection is anchored on the `Window`, normalized to byte ranges by
     `normalize_character_range` (`includes/editor/selection.hpp:70-108`); when the
     "high" cursor sits at EOL, the normalized range is extended across the newline
     into the next line (`selection.hpp:97-101`) — that merge-on-edit asymmetry is the
     observable "selection feels wrong" case.
   - There is **no `o`/`O` cursor swap in Visual mode** (`editor_visual.cpp` handles
     `gg`/ops only) and the effective visual endpoint logic makes it hard to extend
     backward comfortably — missing-feature coupling of keyboard behavior and
     mode-implementation (deferred in design; both are v1-fixable).
2. **LSP can freeze the UI** — **DESIGN** (architecture-level, with several concrete spots).
   - `formatOnSave` spin-waits on the UI thread (`src/ui/ui_tabs.cpp:85-110`) —
     up to 2 s stall per save when a server is slow.
   - `LspProcess::write_all` does a **blocking pipe `write()` from the main thread**
     (`lsp_process.cpp:153-172`): if a server stops reading, the UI hard-freezes on
     every `didChange`/`didOpen`.
   - `Clipboard::write/read` spawn subprocesses with blocking `waitpid`/popen on the
     main thread (`clipboard.cpp:74-92 exec_write, run_capture:25-39`) — yank/copy/cut/paste
     can stall seconds when clipboard tools are slow/absent (falls back to OSC52 only after
     all three binaries fail).
   - `LspProcess::stop()` sleeps up to 1 s in a main-thread loop
     (`lsp_process.cpp:114-133`).
   - Pending LSP requests have no timeout and no `$ /cancelRequest`
     (`lsp_service.cpp cancel_pending`, `sys` X).
   Verdict: a **DESIGN** problem (all external interaction marshalled on the UI thread
   with no async-write path), not a simple bug.
3. **`:NNN` / `ctrl+g` goto-line is a silent no-op** — **BUG**.
   `Ctrl+g` opens `:` and tells you to "type number, Enter" (`ui_commands_keymap.cpp:12-19`),
   but the ex parser skips leading digits/%/`./$` as "range tokens for future use"
   (`ex_commands.cpp:40-46`) and then early-returns when `parsed.name` is empty
   (`ex_commands.cpp:121-122`) — no command ever runs.
4. **`ctrl+y` redo overlaps/steals keys** — **BUG (keymap design)**.
   In `old/config.json` `ctrl+y` is bound to `editor.action.redo` with
   `when="editorFocus"` only — i.e. active in Insert mode too, and duplicated with the
   already-hardcoded `ctrl+r` redo and `u`/`:redo`; `ctrl+y` is also the standard vim
   scroll-line-up key. `old/keymap-catalog.md` (OPEN DECISIONS section) calls this an
   open question; the config when-clause can't disambiguate Insert-vs-Normal because
   the binding lacks a `normalMode` gate.
5. **No auto-indent / broken backspace-join at col 0** — **BUG** (missing feature).
   Insert-mode Backspace at column 0 simply drops the newline
   (`editor_insert.cpp:77-82` → `buffer.delete_char_before(line,0)`); there is no
   leading-indent handling on join and **no autoindent on Enter** either
   (`buffer_insert.cpp raw_insert_newline` copies nothing; `editor_insert.cpp:93-100`
   computes no indent). Notes called this "backspace after space at indent" +
   "object-init indent"; realistic UX gap, not coupling.
6. **`Insert` `jk` only for ASCII `j`** — minor **BUG**.
   `pending_j` is armed only in the `key >= 32 && key <= 126` printable path
   (`editor_insert.cpp:108-117`) — typing via the UTF-8 path never arms/chains the
   `jk` chord (safe because `\uXXXX`-free, but surprising for users who exit via
   non-ASCII text). Low severity.
7. **`space f w` grep-word and `space g p` preview-hunk are placeholders** — **Stub**.
   `ui_commands_keymap.cpp:63` and `:67` only `Messages::info("PHASE 5 …")`
   (matches `sys` PART AD inventory).
8. **Code actions with only a `command` are dropped** — **Stub**.
   `ui_lsp.cpp:184` logs "Code action command not executed"; no command-runner exists.
9. **LSP didChange multi-line change loses incremental fidelity** — **Partial**.
   Incremental path sends UTF-16 one-line diff; multi-line deletes fall back to a
   full resend (`lsp_service.cpp did_change_incremental`, sys X) — perf gap, correct
   fix is UTF-16 range math in TextMetrics.
10. **JSON lexer uses C-style comments** — **Partial** (tolerated JSONC vs strict JSON).
    `old/sys.txt` PART Y fallback section states `uses_c_comments` includes JSON —
    `highlight_generic` mis-tokenizes strict JSON containing `//`-style text.
11. **git stage/unstage/discard block the UI thread** — **DESIGN**.
    `scm_service.cpp` runs those git commands synchronously on main, then
    `request_refresh` (sys G5). Same root cause family as #2.
12. **Visual `<` `>` indent ops missing** — **Stub** (gap), confirmed by reading
    `editor_visual.cpp` switch (no case for `<`/`>`).
13. **Docs drift** — `old/keymap-catalog.md` describes an older keymap (it still
    mentions `ctrl+k …` chords, `jj` insert-exit, `space ;`, `space l n/o/a`,
    `space m r/s` session keys) that the T5 config rewrite removed; `old/config.json`
    and `old/sys.txt` PART T are authoritative. **DESIGN**-process issue (docs out of
    sync, no single source of truth).
14. **Mouse unsupported** — **Stub** (grep: no mousemask/KEY_MOUSE/KEY_RESIZE-mouse
    handling).
15. **Items from your notes that could NOT be tied to code** — **Unknown**:
    any notes about "font fallback in popups" (font fallback is terminal/ncurses
    environment; Noni just draws wide chars in `HoverPopup`), and anything outside the
    source tree (e.g. async term records, plugin marketplace) — no matching code found
    (`SEARCH: src/ for plugin/marketplace/ssh/debugger returned nothing`).

---

# 6. Design smells

1. **UI god-class**: `old/includes/ui/ui.hpp` `UI` owns 16 widgets + core/config/keys/
   extension manager/fs watcher + layout ints + painted-state mirrors
   (`ui.hpp:45-115`) + ex/session/SCM/LSP glue. Almost no isolation boundary.
2. **Editor-module ↔ UI callback ping-pong**: `Editor` carries 11 raw
   `std::function` slots set by UI (`includes/components/editor.hpp:37-53`) —
   UI modules reach into details while components reach back into UI strings.
3. **Buffer aggregates too much**: one class holds content + undo/redo + syntax
   engine + diagnostics snapshot + FS + mtime + display-metric caches + listener
   hooks (`includes/editor/buffer.hpp`) — hard to split conceptually in a rewrite
   without first factoring policy from data.
4. **`EditorCore` acts as a mini-service locator**: it binds Buffer change-listener →
   LSP + SCM + events (`editor_core.hpp attach_lsp_document`) and holds
   search/replaces/registers/marks/jumps — a bag of interrelated members with no clear
   ownership direction (reached from UI, Editor, widgets).
5. **MiniJson is a hand-rolled dependency** (no `\uXXXX`, no exponent, no comments) —
   LSP correctness hinges on it (`src/configs/mini_json*.cpp`).
6. **Panel-specific key handling duplicated** `statusbar.cpp`/`ui_input.cpp`/editor
   floor space: e.g. both `ui_input.cpp` and `search_panel.cpp` define focus-level
   Esc/E-sc handling; `Commandline::open('/')` used for both `actions.find` and `/`.
7. **Buffers/registers keyed by pointer-as-id** (`std::uintptr_t` cast of
   `Buffer*` for LSP docs & marks: `editor_core.cpp`, `search_args` etc.) — fragile
   identity scheme.
8. **Two-layer keybinding evaluation** (config engine + hardcoded component switches)
   has *implicit priority*: engine literal contexts (`keybindings_detail.hpp
   engine_owns_token`, `command.hpp context_is_literal`) decide what even reaches a
   widget, while vim chords like insert `jk` (`pending_j`, `editor_insert.cpp:8-27`)
   and prefix pairs (`g`, `m`, `'`, `"`, `[`) live entirely inside the editor and are
   invisible to the engine. Any new widget must learn which layer eats which key.
9. **Buffer::record_change fan-out has no ordering** (`buffer_insert.cpp:5-60
   record_change` → `apply_forward` → TreeHighlighter invalidation →
   `change_listener_` LSP) — subscribers are implicit; a rewrite should formalize the
   notification graph.
10. **`Selection`/cursor state lives on Window but LSP positions are derived ALL
    over the UI path** (`ui_lsp.cpp`, `ui.cpp` when applying edits) — position math
    mixing byte/UTF-16/display columns (`text_metrics.cpp`, `Buffer::display_to_byte_cached`)
    duplicated rather than encapsulated.
11. **Threading**: only Logger thread + `Background` pool; SCM/ScmService/Process
    read path single-threaded — fine, but `ui.cpp` mixes "pump LSP", "pump PTY",
    "pump fs", "tick autosave", "tick recovery", "sync scm" all inside `render`
    (`sys` Z-step 12-18 spaghetti). Hard to test, hard to reason about in the rewrite.
12. **200-line rule fragments files 5×** (e.g. `lsp/lsp_service*.cpp` six files,
    `ui/ui_commands*` five, `search_panel` three, `terminal_screen` three, `fs_watcher`
    two, `editor_*` twelve, `sidebar` seven, `icons_file` five) — everything that
    should be one readable module is scattered across `_partN` pairs with
    `-detail.hpp` headers.

---

# 7. Must-keep vs can-drop (SUGGESTION)

## 7.1 Keep for v1 rewrite (essential)

1. Buffer model + undo/redo transactions + load-error state + atomic save + external-
   change detection (`editor/buffer*.cpp|hpp`, `buffer_io.cpp`, `utils/fs.cpp`).
2. Modal editing (Normal/Insert/Visual/VisualLine) with the existing motion/operator
   surface, registers/marks/jumps — extend gracefully rather than reinvent
   (`components/editor*.cpp`, `includes/editor/*`).
3. Declarative keybinding engine (config.json chords + when-flags + engine-owned
   syntax) — but consolidate the layering rules into one place
   (`configs/keybindings*`, `commands/command.hpp`).
4. Command registry with stable string IDs (vs-code-like identifiers carry over well
   to config docs/`keymap-catalog` habits) (`commands/*`).
5. Search: in-buffer `/ ? n N` + project search side panel + `replaceAll` single
   buffer (`editor/buffer_search*`, `ui/ui_search.cpp`, `components/search_panel*`).
6. LSP core (sessions, completion, LSP navigation, rename, diagnostics, hover,
   formatting) **with the freeze fixes of §5-2 first** (`lsp/*`).
7. Tree-sitter highlighting + fallback lexer + per-language embedded queries +
   GrammarInstaller (nice-to-have but already built; low risk to keep)
   (`syntax/*`).
8. Git: status + branch + diff-gutter + stage/unstage/discard, **backgrounded**
   (`scm/*`, `utils/git.cpp`); no libgit needed.
9. Integrated terminal (PTY + VT parser + screen + scrollback) — keep as one panel
   (`terminal/*`, `components/terminal_panel*`).
10. UI: dirty-render pipeline + `UIComponent` base (`ui/ui_render*.cpp`,
    `UIComponent.*`) plus Focus routing table; header/tab-bar/status-bar/gutter
    widgets (`components/{header,tab_bar,statusbar,line_number}.cpp`).
11. Session persistence + crash recovery + autosave + recent files
    (`workspace/*`).
12. Config loader semantics (search order + no-write-back + config echo)
    (`configs/config.cpp`).
13. Fuzzy FilePicker/BufferPicker/Recent files (`utils/fuzzy*`, `components/*picker*`).
14. Message/log surfaces (`utils/messages.cpp`, `logger.cpp`, `:messages`/`:logs`).

## 7.2 Defer to later milestones (can drop for v1)

- LSP auto-installer backends (pip/npm/curl/go) — maintainable later, not core
  (`lsp/lsp_installer*`).
- Extension API + demo extension (`extensions/*`) until the core is stable.
- `:help` page system (nice-to-have; static markdown helper suffices)
  (`help/*`).
- hover_popup styling beyond plain text (`components/hover_popup.cpp`).
- Window split resize/close/`openWorkspace` extras beyond basics
  (`ui_commands_nav.cpp` viewWidth/Height group).
- LSP auto-installer configs from `lsp.servers[]` docs overlay — defer the
  `language`-gap CSharp/Java/Go etc. (`syntax/grammar_installer.cpp`).
- `space f w` grepWord and `space g p` previewHunk (already stubs).
- Icons/per-file-glyph datasets (keep a compact map; drop the 5 giant static tables
  `ui/icons_file_part1..5.cpp`).
- Anything GUI/marketplace/SSH/debugger (none exists — the prompt's libgit
  assumption was already removed).

## 7.3 Files I could not fully analyze (from `old/`)

- Deep behavioral edge cases of `src/terminal/vt_parser_csi.cpp` dispatch (sys
  describes its purpose; full per-opcode matrix not read line-by-line).
- `src/ui/ui_recovery.cpp` prompt-ordering nuance (sys Z-step 18; code skimmed only).
- `icons_file_part1..5.cpp` (static data tables + `icons_lookup.cpp` — data content,
  low value to re-verify).
- `extension/builtin_hello greeting` interaction with `extensions.noni.hello.greet`
  (behavior inferred from code+config only).
- `workspace/recovery.cpp` `id = hash(path)` specifics (sys AC) — code briefly read.
