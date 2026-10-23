# Noni (old) — Feature & Design Analysis for the `new/` Rewrite

Analysis date: 2026-10-23.
Basis: full read of `old/` (source `old/src/`, `old/includes/`, `old/noni.cpp`, `old/Makefile`,
`old/config.json`) plus the author's own verified guides `old/sys.txt` (marked
"LAST VERIFIED AGAINST CODE: 2026-09-26") and `old/PROGRESS.md`. Claims below were
spot-checked directly against the code where it mattered (search, LSP stacking, UI,
buffer/undo, clipboard, JSON); anything only known from `sys.txt` is labeled accordingly.

Notes on task assumptions:
- The prompt assumed "libgit2": the codebase does **not** use libgit2 — it shells out to
  the `git` CLI via `popen` (`old/includes/utils/git.hpp`, `old/src/utils/git.cpp`; no
  `#include <git2.h>` anywhere; Makefile has no `-lgit2`).
- `no libgit2` was already documented in `PROGRESS.md` T0.

## 0. Source code layout (old/)

Modules in `src/: commands, components, configs, editor, extensions, help, lsp, scm,
sidebar, syntax, terminal, ui, utils, workspace` (~26,000 lines total across src/includes).
~183 cpp + ~70 hpp files (the codebase follows a hard 200-line-per-file rule, see
`old/.cursor/rules/file-size.mdc`, which produced many machine-file "_part2/_part3" splits).
No tests exist (they were removed, `PROGRESS.md` T0).

---

# 1. Overview

## 1.1 (High-level purpose)

One-sentence product: a keyboard-first, ncursesw TUI code editor with vim-style modal
editing, LSP integration (clangd/pylsp/…), tree-sitter syntax highlighting (auto-download +
dlopen of grammars), git status/diff gutters (via `git` CLI, not libgit2), an embedded
PTY terminal, surface config via `config.json` keybindings, VS Code-style workbench
commands, per-workspace session persistence and crash recovery.

## 1.2 Build system

- Handwritten `old/Makefile` (no CMake, no pkg-config, no third-party vendored code).
- Compiler: `g++`, `-std=c++23` (glibc versions: `__cpp_lib_expected=202211`,
  `__cpp_lib_flat_map=202511`, `__cpp_lib_print=202406`, `__cpp_lib_move_only_function=202110`
  — recorded in `old/PROGRESS.md` line 13-14).
- Flags: `-Wall -Wextra -std=c++23 -g -MMD -MP -Iincludes -DNCURSES_WIDECHAR=1`, `-O2` default.
- Link: `-lncursesw -pthread -lutil -ltree-sitter -ldl` (`old/Makefile` `LDFLAGS`, line 5).
  No libgit2.
- Targets: `all` (default), `debug` (`-O0`), `lto` (`-O2 -flto=auto`, verified by author),
  `run`, `clean`. No test target.
- Source globs: `$(wildcard *.cpp) $(shell find src -type f -name '*.cpp')`.
- Object/dep files land in `build/` (gitignored). Header dependency tracking via `-MMD -MP`
  + `-include $(OBJ:.o=.d)`.
- Old repo also had `.clangd` (`-std=c++23`) for clangd analyze; no compile_commands.

## 1.3 Dependencies

| lib | used for | where |
|---|---|---|
| ncursesw | TUI, wide chars, colors, define_key, esc delay | `old/src/ui/ui_init.cpp`, components include `<ncurses.h>` |
| pthread | std::thread (logger writer) + Background job pool | `old/src/utils/logger.cpp`, `old/src/utils/async.cpp` |
| libutil | `forkpty` for integrated terminal | `old/src/terminal/pty_session.cpp` (`<pty.h>`/`<util.h>`) |
| tree-sitter | highlight parser (system lib) | `old/src/syntax/tree_highlighter.cpp` includes `<tree_sitter/api.h>` |
| dl | `dlopen` of per-language grammar `.so` | same file |
| (processes, not libs) | `git` CLI via `popen`, `wl-copy/xclip/xsel` clipboard tools, `curl`/`git clone` for grammar + LSP installs | `old/src/utils/git.cpp`, `old/src/utils/clipboard.cpp`, `old/src/syntax/grammar_installer.cpp`, `old/src/lsp/lsp_installer_backends.cpp` |

## 1.4 Module map (one-line each)

Under `old/src/` unless noted (from `old/sys.txt` PART Q + direct spot checks):

| Module | Purpose |
|---|---|
| `../noni.cpp` | `main`: Logger::init → Args::get_file_path(argv[1]) → UI(path)::run → Logger shutdown |
| `ui/` | TUI shell: ncurses bootstrap, main poll() loop, layout/resize, dirty-bitmap rendering, Focus routing, key dispatch, all workbench `register_action`s, ex commands (`ui_ex.cpp`), session save/restore, SCM UI glue, config loading, LSP selection UI |
| `components/` | Concrete ncurses widgets: Editor (vim views/motions operators), Sidebar FileExplorer, StatusBar, Header, TabBar, LineNumber gutter, CommandLine, ConfirmPrompt, InputPrompt, FilePicker, BufferPicker, LspPicker, CompletionPicker, MessagesPanel, SearchPanel, TerminalPanel, HoverPopup |
| `editor/` | Non-ncurses engine: Buffer (doc + undo/redo + syntax + diagnostics + mtime), BufferManager (unique_ptr<Buffer> + tab list), EditorTab (WindowLayout + mode), Window (view: cursor/scroll/selection), WindowLayout (binary split tree), Motion namespace (header-only), Selection, Marks, Registers, Jumps, BufferSearch, ExCommands |
| `lsp/` | LSP client over JSON-RPC Content-Length framing + process layer + language-server auto-installers (pip/npm/curl/go). Hpp: lsp_models.hpp, completion.hpp, diagnostics.hpp (header-only data types) |
| `scm/` | git status/diff: ScmGit (porcelain parser via git CLI), ScmDiff (unified diff parser), ScmService (async cache + Background refresh queue) |
| `syntax/` | Syntax highlighting: `Syntax` (fallback hand lexer by language), `TreeHighlighter` (dlopen grammar + tree-sitter parse), `GrammarInstaller` (curl/git + cc to build .so), grammar download helpers |
| `terminal/` | Integrated terminal: PtySession (forkpty), VtParser (ANSI/VT/xterm parser), TerminalScreen (grid model + scrollback), TerminalSession glue |
| `configs/` | `mini_json` (own tiny JSON parser/stringify), `config.cpp` (AppConfig load), `keybinding*.cpp` (chord engine: parse_key, from_raw, when clauses, pending prefix) |
| `commands/` | `CommandRegistry` (command_id → fn) + CommandId constants + InputContext (`command.hpp`) |
| `extensions/` | Extension API (EditorEvents bus, ExtensionContext, ExtensionManager) and one builtin demo extension (`noni.hello`) |
| `help/` | `HelpDocs::build` pages for `:help` |
| `sidebar/` | DirScanner (async directory tree over Background pool) |
| `workspace/` | Workspace root detection (git root → marker walk), SessionStore (`session.json`), RecoveryStore (crash snapshots), RecentFiles (MRU) |
| `utils/` | Logger (async file writer), Messages ring, FS (atomic write `FS::write_file`), `Git` (popen wrapper with shell_quote), Clipboard (wl-copy/xclip/xsel + OSC52 base64), Fuzzy + FileIndex, TextSearch (async project grep), TextMetrics (UTF-8/UTF-16/display columns), Background worker pool, EventWake (eventfd/pipe), FsWatcher (inotify, non-blocking), str helpers, args |

Headers mirror `src/` in `old/includes/` (mostly `#pragma once`, several header-only).

Other top-level files in `old/`: `config.json` (+ `.bak`), `sys.txt` (architecture doc),
`PROGRESS.md`, `simpletodo.txt`, `keybinds.txt` (NOT loaded by the app — pure notes),
`keymap-catalog.md` (nvim/vscode cross-check notes, partially stale),
`.cursor/rules/` (author conventions incl. the 200-line rule), `.clangd`.

## 1.5 Runtime (from `old/sys.txt` PART I/AB, code-verified by author 2026-09-26)

Main loop `UI::run` (`old/src/ui/ui_loop.cpp`) is poll()-driven on a single thread:
fds = stdin + EventWake + FsWatcher inotify + PTY master + all LSP stdouts. All LSP/PTY
pumping happens on main after poll — there are **no** reader threads for them (only
Logger has a thread; Background holds a 2-worker job pool). Dirty-flag rendering paints
only changed ncurses `WINDOW`s.

---

# 2. Feature inventory

Status legend: **Complete** / **Partial** / **Stub** / **Broken-looking** / **Unknown**.

## 2.1 Editing

| Feature | Where | Status | Evidence |
|---|---|---|---|
| Buffer: line array, UTF-8 text, revision + content_id dirty tracking | `old/includes/editor/buffer.hpp`, `old/src/editor/buffer.cpp` | Complete | content_id vs saved_content_id in `buffer.hpp:35-38`; save clears |
| Load file (incl. LoadError state) | `buffer.cpp` load(), `BufferLoadState` `buffer.hpp:19-23` | Complete | LoadError never treated as empty ✓ |
| Atomic file writes (temp file + rename) | `old/src/utils/fs.cpp` `FS::write_file` (`mkstemp`-like `.noni.XXXXXX`, `fs.cpp:55`) | Complete | rename-based write |
| mtime external-change detection + notify | `buffer.hpp:54-56` disk_mtime_, disk_changed(), `external_change_notified_` | Complete | flag consumed by UI autosave path (`sys.txt` T6.7) |
| Save / save-as (`:w` to new path) | `buffer_io.cpp:61-120` | Complete | save() rejected when LoadError 🛡 |
| Apply recovery snapshot | `buffer_io.cpp:122-146` | Complete | keeps dirty so we never auto-overwrite files |
| Insert/delete char at/before, insert text, insert newline, delete range | `old/src/editor/buffer_insert.cpp`, `old/src/editor/buffer_delete.cpp`, `buffer.hpp:153-162` | Complete | used by editor component |
| Undo/redo (transaction-based, cursor snapshot inside txn), max 256 entries | `buffer.hpp:40-44 kMaxUndo=256`, `old/src/editor/buffer_io.cpp:1-59` | Complete | author's stated design matches |
| Undo txn semantics via begin_edit/end_edit with cursor handed in | `buffer.hpp:144-145`, Buffer::save() closes any open txn at `buffer_io.cpp:78-79` | Complete | caller owns boundaries |
| Registers (unnamed a-z " etc.) | `old/includes/editor/registers.hpp` header-only | Complete | RegisterFile has named+unnamed |
| Marks a-z | `old/includes/editor/marks.hpp`, set via Normal `m`, jump via `'a` | Complete | MarkTable with invalidate_buffer |
| Jump list | `old/includes/editor/jumps.hpp`, pushed on big moves (`old/src/components/editor_cursor.cpp:50`) | Complete | cap 100 |
| Comment toggle `gcc`, Visual `gc`, `ctrl+/` | `old/src/components/editor_cmds_edit.cpp` + CommentTokens `old/includes/editor/comment_tokens.hpp` | Complete/Partial | author marked working in T6.2; token support per-language — Partial coverage of languages |
| Copy line down `ctrl+e`, move line alt+j/k/up/down | `COMMAND_ID copyLinesDownAction/moveLinesDownAction` in `old/src/ui/ui_commands_keymap.cpp` | Complete | author verified T6.6 |

| Clipboard read/write (wl-copy→xclip→xsel; OSC52 fallback on stdout) | `old/src/utils/clipboard.cpp` | Complete | `Clipboard::write/read/write_osc52`, base64 encoding inline |
| Clipboard sync on yank/copy/cut | `Editor::yank_range` → `Clipboard::write` if `clipboard_sync_` (`old/src/components/editor_yank.cpp:20-24`) shall route through ui path | Complete | flag from config `clipboard.sync` |
| Clipboard paste INSERT (KEY_SIC) + `ctrl+v` | `old/src/components/editor_edit.cpp:44-57 paste_clipboard` | Complete | |
| Autosave afterDelay (default 1000ms; skip untitled; skip external-mtime conflicts) | `old/src/ui/ui_features.cpp`/autosave tick in ui.cpp; config `old/includes/configs/config.hpp AutosaveConfig` | Partial | author verified via T6.7 |
| Format-on-save with LSP formatting, 2s deadline | `old/src/ui/ui_tabs.cpp:85-110` | Partial | blocking poll-loop on UI thread — hidden freeze |
| UTF-8 typing (get_wch, cp>126) | `old/src/ui/ui_input.cpp` `insert_focused_utf8`, `old/src/components/editor_insert.cpp:126-137` | Complete | author T1.7 |
| Retired: `jj` exit (old) → now `jk` via pending_j | `old/src/components/editor_insert.cpp:8-27,108-117` | Complete | matches `sys.txt` T4 |

PTM check note for review: `Buffer::sync_syntax(int lo, int hi)` is called from
`Editor::draw_window_pane (old/src/components/editor_draw.cpp:20)` — syntax parse only for
visible rows (viewport-based, so cheap even for big buffers). Full-buffer highlight is
only forced on load/undo/redo (`buffer_io.cpp` invalidate_all, `TreeHighlighter`).

## 2.2 Vim modes

| Feature | Where | Status | Evidence |
|---|---|---|---|
| Modes Normal/Insert/Visual/VisualLine | `old/includes/editor/editor_tab.hpp:12-18` + `old/src/components/editor_mode.cpp` | Complete | routing in Editor::handle_input |
| Normal motions h j k w b e 0 ^ $ G + arrows | `old/includes/editor/motion.hpp` + `old/src/components/editor_normal.cpp:14-131` | Partial | **no ds//cs elsewhere, no `f/F/t/T` (find char), no `r/R`, no `\|`/count prefixes/pipes, no `%` match-motion** (motion.hpp has only word+line motions) |
| Word motions w/b/e | `old/includes/editor/motion_word*.hpp` (word forward/backward/end) | Complete/Partial | 3 files exist; author marked them working |
| Line motions | `old/includes/editor/motion_line.hpp` | Complete | |
| Operators d c y with motions (e.g. `dw` `ciw`) | `old/src/components/editor_normal_prefix.cpp` (pending_op) | Partial | sys.txt PART T3 covers dd/cc/yy + with-motion |
| dd/cc/yy linewise | same file | Complete | |
| o/O open line + insert | handled in Normal mode | Complete | sys.txt …  |
| x (delete char) | `editor_normal_edit.cpp` | Complete | |
| s / S / C | `editor_normal_edit.cpp` (per sys.txt: substitute/change line/change to eol) | Complete per code map |
| p / P paste (behind/ahead), linewise vs char | `old/src/components/editor_ops.cpp:65-129` | Complete | inserts register then restores cursor (no code-check re-snapshot like beyond) |
| Visual mode (character & line) selection, yank/delete/change below | `old/src/components/editor_visual.cpp` (validate visual intact `if key == 'd'…`) and `editor_visual_state.cpp`, apply via `Window::selected_range` | Partial (see 2.17) | only gg-exchange, no o/O cursor-version, no discard |
| Visual `d`/`x`, `c`/`s`, `y`, `p`/`P` | `editor_visual.cpp:32-66` | Complete (key handling) | uses op machinery |
| Visual scroll keys Ctrl-D/U/PgUp/PgDn | `editor_visual.cpp:82-101` | Complete | |
| Normal `v`/`V` → visual | `editor_normal.cpp:70-75` → `enter_visual_char/line` | Complete | |
| Visual `esc` → normal (clears selection) | `editor_visual_state.cpp:23-29 leave_visual()` | Complete | |
| Insert `jk` exit | `editor_insert.cpp pending_j` logic | Complete | |

| Normal search `/` + `?` prompt | `editor_normal.cpp:76-83` → `on_search_forward_/backward_` (:UI callbacks) → CommandLine | Complete | |
| `n` / `N` next/prev match | `editor_normal.cpp:84-103` → `EditorCore::search_next/prev` | Complete | |
| Navigation via marks (m + ') | `editor_normal.cpp:113-118` pending_m / pending_jump_mark | Complete | MarkTable |
| `K` LSP hover | `editor_normal.cpp:123-126` on_hover_ | Complete | signature from UI |
| `gg` (Normal+Visual), `gt`/`gT` tab change, gd/gD/gy/gr/gi LSP jumps | `old/src/components/editor_normal_prefix.cpp` (pending_g) + `editor_visual.cpp:37-49` | Complete | |
| `dgg`/`ygg`/`cw` etc with `g` prefix; `gi` last insert | `editor_normal_prefix.cpp` + `last_insert_` state in `editor.hpp:26-28` | Complete | |
| Comment toggles `gcc` & Visual `gc` | `editor_normal_prefix.cpp pending_gc_` + `cmd_toggle_comment` | Complete (T6.2 verified) | |
| `Tab`/`Ctrl-I` jump forward (tab sidebar binding removed T5) | normal handle Tab → jumps().forward | Partial | no binding into config engine (documented in `sys.txt` T5), and Normal-mode hard Tab vs nvim MiniPairs-like custom bindings absent |
| `<` `>` indent Visual | **not handled in `handle_visual_input`** (`old/src/components/editor_visual.cpp` switch has no `<`/`>` case) | Stub | Issue: indent missing |

| U undo / Ctrl-R redo / :u / :redo | `editor_normal_edit.cpp` + ExCommands `old/src/editor/ex_commands.cpp` | Complete | `sys.txt` PART V |
| gl LineNumber: relative? `.cursor/rules` + keybinds show `space s s.ln6` was planned for `:set relativenumber` toggle by file — **not in code** | Search old/src grep `.cursor/rules/*` no. Non-app file. | Stub (FilePicker rescan nothing) | `old/keybinds.txt`, `old/.cursor/rules/file-size.mdc`, `old/keymap-catalog.md` — grep of src found nothing |
| Ctrl+O back jump | normal tab — `editor_normal.cpp` case Tab & Ctrl-O raw 15 → jumps().back (sys.txt PART T3) | Complete | in code |

## 2.3 Navigation

| Feature | Where | Status | Evidence |
|---|---|---|---|
| Split windows (right/down), focus left/right/up/down, resize width/height +/-, close group | `old/includes/editor/window_layout.hpp` (WindowLayout binary tree + compute_rects) + `old/src/editor/window_layout*.cpp` + commands `workbench.action.splitEditorRight/Down`, `focusLeft/Right/…Group` in `old/src/ui/ui_commands_nav.cpp` | Complete (set of 25-registered commands) | draw via `rects_cache_` (`old/src/components/editor_draw.cpp:90-120`) |
| Per-tab line numbers gutter, markers, gutter marks, right side? (Right side) — Right side** | **`old/src/components/line_number.cpp` handles gutter only similarly (theme pairs 61-82 — GutterAdded/Modified/Deleted from `old/src/ui/theme.cpp:196-297`)** | Complete | documented in `old/sys.txt` PART |
| Buffer/tabs: new untitled, open file, switch, close, close-all others, next/prev, reorder | `old/src/editor/buffer_manager.cpp` + `old/src/ui/ui_tabs.cpp` close/save; commands in `ui_commands.cpp` | Partial | “close others” is via `closeAsGroup`? (registered command `closeActiveEditorGroup` — exact behavior **Unknown** |

| FilePicker (fuzzy file open, Ctrl+P, `space f f`, `space s s` for FileSearch? Same) | `old/src/components/file_picker.cpp` + `old/src/utils/fuzzy.cpp` FileIndex (background rebuild) | Complete | `sys.txt` verified with dirty flags post T7.7 |
| BufferPicker (`space f b`, `:buffers`) | `old/src/components/buffer_picker.cpp` | Complete | shows all tabs in BufferManager |
| LspPicker (locations/symbols/actions/install) | `old/src/components/lsp_picker.cpp` + `old/src/ui/ui_lsp.cpp` | Complete | enter → proper action |
| CompletionPicker | `old/src/components/completion_picker.cpp` | Complete (per UI code); interact with LSP server pending |
| Goto line (`:LN` via number | **not in code** (`old/keybinds.txt` line for s1 note) | Stub | noted in `old/keybinds.txt` & `old/keymap-catalog.md` (“go to line” never bound in code), search of src for “gotoLine” returns only `workbench.action.gotoLine`→UI (ui_commands_keymap.cpp:12) producing messages…? |
| Sidebar tree expand/collapse via h/l/Enter | `old/src/components/sidebar_nav.cpp` + `sidebar.cpp:293-420` | Complete | |
| Sidebar create/rename/delete + clipboard cut/copy/paste, refresh | `old/src/components/sidebar_fs.cpp`, `sidebar_clip.cpp`, `sidebar_input.cpp`, `ui_sidebar_delete.cpp`, prompts | Complete | uses FS wrappers |
| Fuzzy quickOpen `space space`, `space f f`; unrelated greenfield menu `f w` (grep word) | grep word stub (see below) | See inventory item “search.grepWord stub” | |

## 2.4 Search / Replace

| Feature | Where | Status | Evidence |
|---|---|---|---|
| In-buffer search `/` `?` prompt, match list, incremental n/N going deeper, highlight current | `old/src/editor/buffer_search.cpp` (whole_word + case + regex per `BufferSearchQuery`) + `old/src/components/editor_search.cpp` access via UI search callbacks | Complete | `BufferSearch::find_all` (regex ECMAScript, case-insens option, whole-word check `buffer_search.cpp:37-118`) |
| Search highlight in render | `old/src/components/editor_draw_overlay.cpp:85-127` draw_search_highlight | Complete | current match uses SearchMatchCurrent pair |
| Project-wide async grep with match sampler (skip node_modules, binary dirs) + replaceOne/replaceAll | `old/src/utils/text_search.cpp` + `TextSearch::search_async` + `old/src/ui/ui_search.cpp` + editor action command `old/src/ui/ui_commands_nav.cpp:56-77` | Partial | replaceOne+replaceAll registered but current-match support is per-buffer only; No global “replace all in files” — only per-buffer search panel replace via `EditorCore` |
| Search side panel (Query field, Replace field, regex/case/whole-word toggles F6/F7/F8, Tab cycling) | `old/src/components/search_panel.cpp` (+ part2/3) + Focus Search in `old/src/ui/ui_input.cpp`; F6/7/8 handled in server, key → focus Search UI | Complete | confirmed in ui_input.cpp hot code (Enter in Query → run search; Enter in Replace → ReplaceAll; Results: `r` ReplaceAll) |
| Search result open match in editor; replace-all only for active buffer | `old/src/ui/ui_search.cpp` + `old/src/editor/editor_core.cpp` (EditorCore::replace_all) | Partial | per `sys.txt` PART U replaceAll registered but only wired to SearchPanel Replace field — no multi-file replace |
| Reaction to in-buffer query delete when editing — prune matches | `old/src/editor/buffer_search_part2.cpp` | Complete | |

`space f w` (grep word under cursor) → **Stub** (`old/src/ui/ui_commands_keymap.cpp:63`: only `Messages::info(...)`). Noted in `old/sys.txt` PART AD as PHASE 5.

## 2.5 Syntax highlighting

| Feature | Where | Status | Evidence |
|---|---|---|---|
| Tree-sitter parse + per-language embedded queries | `old/src/syntax/tree_highlighter.cpp` (`kQueryC/Python/JS/Rust/Bash/Json/Lua/Markdown` queries inline; `old/src/syntax/ts_queries.hpp` fallback references) | Partial (languages C, Cpp, Python, Rust, JS/TS, Go?, Lua, Markdown, Bash, Json; Missing: CSharp, Java, Way more? Go/Java fallback only) | per `old/sys.txt` PART Y mapping table; ts_hl_detail.hpp for missing go/java |

| Fallback lexer per-language | `old/src/syntax/syntax.cpp` highlight_line + `syntax_generic.cpp` + `syntax_kw*.cpp` + `syntax_md.cpp` | Complete (Plain/Markdown/Cpp/C/Py/Rust/Go/JS/TS/JSON/Shell/Lua/Java/CSharp) | used when grammar missing |
| Grammar auto-download & build (curl+tar or git clone, cc compile to .so, queue, retry 60s, log to `logs/grammar-install.log`) | `old/src/syntax/grammar_installer.cpp` + `grammar_download.cpp`/`grammar_compile.cpp` + `grammar_request.cpp`/`grammar_state.cpp`/`grammar_tools.cpp` | Partial | complete for 10 languages; CSharp and Plain skipped (`so_for_language` returns nullptr); Build on first open + auto-install flag `syntaxAutoInstall` |
| Search dirs /usr/lib/tree_sitter, ~/.local/share/noni/tree-sitter, <ws>/.noni/tree-sitter, user_dir, /tmp fallback | `grammar_installer.cpp find_grammar` (~line 619+) | Complete | per sys.txt PART Y ordering |
| Theme: 90 color pairs, custom terminal colors from id 16 when can_change_color | `old/src/ui/theme.cpp:159-297` | Complete | uses 256-cube/gray fallback |
| Incremental re-parse on edit (ts_tree_edit + incremental parse; fallback full parse) | `tree_highlighter.cpp notify_edit / sync` (~line 660-739) + `old/src/syntax/tree_highlighter_sync.cpp` | Complete | pending edits queue, dirty_from tracking |

## 2.6 LSP

| Feature | Where | Status | Evidence |
|---|---|---|---|
| JSON-RPC framing (Content-Length), request/notification/response; handle inbound; ignore unknown responses, `-32601` for unknown server requests | `old/src/lsp/lsp_jsonrpc.cpp` + `old/src/lsp/lsp_session_msg.cpp` | Complete | sys.txt PART X; `-32601` seen at `lsp_session_msg.cpp:78` |
| Child process (fork + exec + pipes; stderr → `logs/lsp.stderr.log`; non-blocking stdout pump on main; SIGTERM 1s then SIGKILL) | `old/src/lsp/lsp_process.cpp` | Complete | `stop()` graceful-bounded (`lsp_process.cpp:108-146`); reader thread removed in T7.1 (`sys.txt`) |
| Session lifecycle: initialize/init handshake, pending didOpen while Starting, state Idle/Starting/Running/Failed/Stopped | `old/src/lsp/lsp_session.cpp` + `old/src/lsp/lsp_service.cpp` | Complete | `sys.txt` PART X section code-checked |
| didChange full (0/1) + incremental (2, UTF-16) | `old/src/lsp/lsp_service*.cpp` (`did_change_incremental`), config-driven via initialize result | Complete | honored textDocumentSync, but multi-line delete → full retry fallback |
| didOpen/didSave/didClose (with open_uris_ registry) | `old/src/lsp/lsp_session_doc.cpp` + `lsp_service.cpp` | Complete | |
| Completion (Ctrl+Space), CompletionList data | `old/src/lsp/lsp_session_req.cpp request_completion`, results via `old/src/lsp/lsp_result.cpp` + models `old/includes/lsp/completion.hpp` | Complete | |
| Definition / Declaration / TypeDefinition / References / DocumentSymbol / WorkspaceSymbol / Rename / CodeAction requests | `lsp_session_req.cpp` + `old/src/lsp/lsp_parse_edit.cpp`/`lsp_parse_item.cpp` | Partial | CodeAction handling: only apply when action has `edit`; forward `command`-only actions → **stub** (`old/ui/ui_lsp.cpp:184` messages “Code action command not executed”) |
| Diagnostics (publish → Buffer diagnostics snapshot version-gated) | `old/src/lsp/lsp_dispatch_diag.cpp` + `old/includes/lsp/diagnostics.hpp` + Buffer::set_diagnostics | Complete | |
| Diagnostics gutter marks + hover overlay highlight | `old/src/components/line_number.cpp` draw + `old/src/components/editor_draw_overlay.cpp:129+` | Complete | |

| Diagnostic jump [d / ]d (wrap) | editor prefix pending_bracket_ (`old/src/components/editor_normal_prefix.cpp`) | Complete | T6.4 |
| Hover `K` → HoverPopup component | `on_hover_` + `old/src/components/hover_popup.cpp` | Complete | T6.3 |
| Retry failed sessions with pending_opens_ re-doOpen | `old/src/lsp/lsp_session.cpp session_for` on fail | Complete | state machine per sys.txt T1.6 |
| Restart / status commands (lsp.restart, lsp.showStatus) | `old/src/ui/ui_commands_nav.cpp:117-135` | Complete | |
| Cancellation of in-flight LSP requests (no `$/cancelRequest` sent) | `old/src/lsp/lsp_service.cpp cancel_pending` | Partial | only local pending map does not blow it down |
| 🖼 **Server auto-installer** (pylsp pip venv / tsserver npm-or-bun prefix / clangd curl GitHub zip / gopls go install / rust-analyzer curl zip), install dir `~/.local/share/noni/lsp` → `<ws>/.noni/lsp` → `/tmp/noni-lsp-<uid>`; :lsp install picker | `old/src/lsp/lsp_installer.cpp` + `_backends.cpp` + `_util.cpp` + `lsp_picker.cpp` LSP install catalog | Partial | catalog has 5 servers; language map “without lsp.servers[].language cannot auto-spawn” |
| rootMarkers walk from file path then workspace root | `old/src/lsp/lsp_service.cpp session_for` (T1.6) | Complete | |
| Format-on-save (2s) + manual format | `old/src/ui/ui_tabs.cpp:85-110` + `editor.action.formatDocument` | Partial | 2s freeze on UI thread → see Known Problems |

## 2.7 Git (SCM)

| Feature | Where | Status | Evidence |
|---|---|---|---|
| Repo root detection for status path handling | `old/includes/utils/git.hpp` `Git::is_repo` (checks `<root>/.git` exists) | Complete | `.git` dir presence — shallow |
| git repo resolution for SCM + workspace root markers | `git CLI popen · old/src/utils/git.cpp (popen with shell-quoted args via shell_quote())` | Complete | sequence: `old/src/utils/git.cpp:3-16 shell_quote`, `run_git` → `run`/`run_ok` (streams captured) |
| Status porcelain parse (scm_git + part2) | `old/src/scm/scm_git.cpp`, `scm_git_part2.cpp`, `old/includes/scm/scm_types.hpp` | Partial | no branch graph handling, no upstream |
| Diff parsing for gutter | `old/src/scm/scm_diff.cpp parse_unified` (leading diff -U0) + `scm_types.hpp` | Partial | only -U0-style single-file edits; no renames + copy sources |
| Async refresh service with generation counters | `old/src/scm/scm_service.cpp` + part2 + `Background` pool | Complete | stale token guard on UI (`scm_diff_gen_seen_` ui.hpp:90-92) |
| Commands: stage/unstage/discard worktree file, show status, show diff, refresh, refreshDiff | `old/src/ui/ui_commands_nav.cpp:85-112` + ScmService stage/unstage/discard | Partial | completed for “whole file”; Author marked hunk-level ops as out of scope (“stage/preview hunk” listed as PHASE 5 in `old/sys.txt` PART AD). `space g p` (git.previewHunk) is a **Stub** (`old/src/ui/ui_commands_keymap.cpp:66-67`) |
| Branch badge in Header + statusbar SCM badge | `old/src/components/header.cpp` + `statusbar.cpp` `set_scm_badge` (Nonce via theme ids) + diff cache `ui.hpp:91-95 scm_diff_cache_` | Complete | |

NOTE (no libgit2): the prompt's assumption was wrong. The old repo originally tried libgit2
but dropped it (T0, `old/PROGRESS.md` line 12); all Git features run the `git` CLI via
popen.

## 2.8 Terminal panel

| Feature | Where | Status | Evidence |
|---|---|---|---|
| forkpty session per panel (shell via config `terminal.shell` or `$SHELL` or `/bin/sh`) | `old/src/terminal/pty_session.cpp` | Complete | handle KE_RESIZE etc. passed through |
| VT parser (ANSI/VT100/xterm): CSI streams, SGR color/attributes, cursor movement, scroll regions, DECCKM application cursor keys, OSC (title), wide-char support | `old/src/terminal/vt_parser.cpp` + `vt_parser_csi.cpp` + `vt_parser_sgr.cpp` + `old/includes/terminal/terminal_cell.hpp` | Partial | `sys.txt` note “DECCKM: CSI ?1h/l → TerminalScreen::application_cursor_keys”; no Sixel; no full truecolor test |
| TerminalScreen grid + scrollback ring; scrollback configurable (default 5000) | `old/src/terminal/terminal_screen.cpp` + `-part2/-part3` + `old/includes/terminal/terminal_screen.hpp` | Complete | drain + scroll |
| TerminalPanel draw + input UI | `old/src/components/terminal_panel.cpp` + `_part2` + `sysPARTS` | Complete | toggle/hide via control workbench.action.terminal.toggle etc.) |
| Terminal toggle/clear/kill commands; ScrollUp/Down keys | `old/src/ui/ui_commands.cpp terminal.* + old/src/ui ui_input focus terminal` → leave 29 Ctrl+] or ctrl+t toggle | Complete | |
| PTY pump (read + write) on main thread via poll() | `old/src/terminal/pty_session.cpp pump_readable` + `old/src/ui/ui_loop.cpp` | Complete | no reader thread (T7.1) |
| F-bound keys + Home/End/Insert/Delete/ PageUp/Down/ Tab/Enter/Backspace | `old/src/components/terminal_panel_part2.cpp:102-133` | Complete | via keymap translate |

| Echo term when F1..F12 (xterm seq) | `old/src/ui/ui_input.cpp` Terminal-focus branch | Complete | |
| Mouse support | **not implemented** | Stub | no mousemask/KEY_MOUSE (`grep` confirmed) |
| PTY exit handling, child process restart after exit | `pty_session.cpp` + `TerminalSession` state | Partial | exit → panel stops; no respawn UI (Unknown exact UX) |
| `workbench.action.terminal.*` | `ui_commands.cpp` | Complete | clear/kill/scrollUp/Down/focus/toggle registered |

## 2.9 UI / Layout

| Feature | Where | Status | Evidence |
|---|---|---|---|
| Editor full-screen ncurses layout: header title + `Statusbar` (mode/file/git badge/cursor/echo hints/chord), `TabBar`, `Sidebar` (Explorer tree + SearchPanel side-view), `LineNumber` gutter, Editor area, bottom-row overlays (CommandLine/ConfirmPrompt/InputPrompt), bottom terminal section | `old/src/ui/ui_layout.cpp` UI::resize_resizer + `ui_init.cpp` + components | Complete | `sys.txt` PART C layout numbers |
| Focus enum + hardcoded per-widget input routing (Editor/Sidebar/Terminal/Search/Command/Messages/pickers/Confirm/Prompt) | `old/includes/ui/ui_types.hpp` Focus + `old/src/ui/ui_input.cpp handle_inputs` | Complete | overlay/Hard-coded non-config keys, 12, 3200-3497 |
| Keybinding engine: config.json `keybindings[]` only for engine chords; vim keys/panel keys hardcoded in source; engine owns ctrl/alt/F/Space/Esc in Normal/Visual (`engine_owns_token`) | `old/src/configs/keybindings*.cpp` + `old/src/components/editor*` | Complete | config-only design documented (`old/sys.txt` PART J) |
| Dirty-bitmap render pipeline (only changed WINDOWs repaint; force repaint on focus/visibility/buffer/cursor/SCM deltas; per-widget `mark_dirty()`) | `old/src/ui/ui_render.cpp`, `old/src/ui/ui_render_dirty.cpp`, `old/src/ui/ui_render_paint.cpp`, visible flag via members `ui.hpp:100-115` | Complete | T7.2 |
| Buffer line display metrics caches (`line_disp_width_` + hot prefix) | `old/src/editor/buffer_metrics.cpp` + Buffer fields `buffer.hpp:58-61` | Complete | invalidates on TextChange (`note_metrics_edit`) |
| Chord state “prefix pending” display in status bar, no timeout in editor mode, Esc to cancel | `old/src/components/statusbar.cpp set_chord` + `old/src/configs/keybindings.cpp resolve` pending vector | Complete | |
| !Important — `gotoLine` accepts a number after `:` | `old/src/ui/ui_ex.cpp` + UI::ex_* | Complete | `:123` goes to line 123 (author asks/gocase **Unknown after code check?** — search of `old/src/editor/ex_commands.cpp` colon parse shows numeric-only accepted) |
| HoverPopup floating widget (LSP hover) | `old/src/components/hover_popup.cpp` + `old/includes/components/hover_popup.hpp` | Complete | |
| Git diff gutter (added/modified/deleted marks) | `old/src/components/line_number.cpp` + ScmDiff cache | Complete | |
| Statusbar chord/echo/mode; Header branch | components | Complete | |
| TUI color theme (90 pairs, code + Theme tokens) | `old/src/ui/theme.cpp` 159-297, `old/includes/ui/theme.hpp` | Complete | |

| Windows layout: sidebar width from config; force-open Search side min width 36; bottom = statusbar + optional terminal band (height/center clamped 5..(n-1)/2) | `ui_layout.cpp` + config in `ui.cpp` | Complete | |
| Corner case: INVALID DIMENSIONS print when screen < sb + ln widths | `ui.cpp` resize | Complete (render guard) | |
| Component base class with create/resize/clear (WINDOW wrapper) | `old/includes/ui/UIComponent.hpp` + `old/src/ui/UIComponent.cpp` | Complete | |
| Extension system (add/activate/deactivate, EditorEvents, ExtensionContext with addKeybinding/registerCommand/extension config) | `old/src/extensions/*` | Partial | only one in-tree demo extension `noni.hello` shim; no real-world plugins yet |
| Help overlay pages (`:help`, topic list, command/doc pages) | `old/src/help/help_docs.cpp` + `help_pages_core.cpp` + `help_pages_more.cpp` | Complete | registered for ex and ex-help topics |
| MessagesPanel (`:messages`, `:logs`) | `old/src/components/messages_panel.cpp` + Messages ring in `old/src/utils/messages.cpp` + `old/src/utils/logger.cpp` | Complete | tail budgets (noni/lsp/install/grammar logs) |
| Session save/restore (tabs, cursor, scroll, sidebarVisible, terminalVisible, recent files) | `old/src/workspace/session.cpp` + `old/src/ui/ui_session.cpp` + command in `ui_commands_nav.cpp:149-156` | Complete | `session.json` persisted; untitled tabs skipped |

| Crash recovery snapshots per dirty buffer (id-hash per file path, min 2s granularity) | `old/src/workspace/recovery.cpp` + `old/src/ui/ui_recovery.cpp` prompts at startup | Complete | |
| RecentFiles MRU | `old/src/workspace/recent_files.cpp` + `noni.search.recent` command (`ui_commands_keymap.cpp:61-62`) | Complete | |
| Workspace change (`:workspace` / openWorkspace with picker) | `old/src/ui/ui_commands.cpp:108` + `apply_workspace_root` in `old/src/ui/ui_workspace.cpp` + FSM handler + ScmService reset + FsWatcher set_workspace + Sidebar set_project_path | Complete | |
| FsWatcher (inotify, non-blocking, poll after poll() on main; no watcher thread) | `old/src/utils/fs_watcher.cpp` + `_part2` + `old/includes/utils/fs_watcher.hpp` | Complete | triggers sidebar refresh debounce |
| DirScanner (async tree enumeration on Background pool; expansion state kept in Sidebar) | `old/src/sidebar/dirscanner.cpp` + `_part2` + `old/src/components/sidebar*.cpp` | Complete | cache + seen_version |
| FilePicker FileIndex background rebuild | `old/src/utils/fuzzy.cpp FileIndex` | Complete | version-based swap on the Background thread |
| Multi-buffer pretty header/tab strip + icons (Codicon-mapped per file ext) | `old/src/ui/icons*.cpp/hpp` + `old/src/components/tab_bar.cpp/header.cpp` | Complete | map of icon glyphs — large static data (~5 files in `icons_file*.cpp`), sizeable. |
| Log tailing + logging format (two-line messages: header + source loc) | `old/src/utils/logger.cpp` | Complete | truncates `logs/noni.log` at startup (`logger.cpp:124`) |

## 2.10 Config

| Feature | Where | Status | Evidence |
|---|---|---|---|
| config.json load order: $PWD → exe dir → ~/.config/noni; parse fail → statusbar echo, fall back to defaults; never writes the file back | `old/src/configs/config.cpp` + `old/src/ui/ui_init.cpp load_config` | Complete | `old/sys.txt` PART S |
| JSON parser (mini_json, in-repo, no third-party): objects/arrays/strings/numbers/bools/null; no comments; duplicate keys first-wins; trailing junk error | `old/src/configs/mini_json.cpp` + `old/src/configs/mini_json_parse.cpp` | Partial | **no `\uXXXX` decoding** (`mini_json_parse.cpp:119-138` pushes the backslash char verbatim or raw char), **no exponent** (`parse_number` handles only leading `-` and one `.` fraction) — hard limitation for LSP responses; also double parsing overhead on long LSP payloads |
| Keybindings resolution: chord tokens (whitespace, mod prefixes + keynames), when-clauses (&&/|| on context flags), first-match wins, no-prefix timeout; keybinding→command (engine-owns control keys with editor-mode gates) | `old/src/configs/keybindings.cpp` + `_part2` + `keybindings_detail.hpp` + `keybindings_raw.cpp` + `keybindings_ext.cpp` | Complete | matches `old/sys.txt` PART T |
| Startup “unbound commands” diagnostic logging + “unknown binding” warning | `old/src/commands/command_registry.cpp diagnose` + UI ctor log | Complete | |
| “escape goes to noni.mode.normal” when set (empty when) + special-cased `Esc` in Terminal | `ui_input.cpp` + config.json | Complete | T1.4 |
| Config echo into statusbar on startup (“loaded N bindings…”) | `ui_init.cpp` | Complete | T1.1 |

## 2.11 Other

| Feature | Where | Status | Evidence |
|---|---|---|---|
| Background job pool with token guard (stale result drop) | `old/src/utils/async.cpp` + call sites (DirScanner, FileIndex, TextSearch, ScmService, installers) | Complete | |
| EventWake (eventfd/pipe) wakeups for jobs | `old/src/utils/event_wake.cpp` | Complete | |
| Logger with writer thread, ring — logs to `logs/noni.log` | `old/src/utils/logger.cpp` | Complete | |
| Debug/tracing gate `NONI_KEYLOG=1` env → logged raw key names | `old/src/ui/ui_input.cpp:57` + `old/src/ui/ui_init.cpp` | Complete | |
| `Args` argv parsing (single path only; no flags) | `old/src/utils/args.cpp` | Complete | only argv[1] honored |
| `.clangd` file for tooling | `old/.clangd` (CompileFlags add -std=c++23) | Complete (tooling, not app) | |

| PROGRESS.md work queue + sys.txt doc (good intent) | `old/PROGRESS.md`, `old/sys.txt`, `old/simpletodo.txt` | Complete (docs, not code) | author maintained |

---

# 3. Keybindings (full table)

All chord bindings come from **config.json only** for engine keys; all vim-mode and
panel keys are **hard-coded in source**. `mode` column = the when-window in which the
binding is active (`editorFocus && normalMode || visualMode …` etc.).

## 3.1 config.json → workbench commands (`.json` “Where”) — the engine

File: `old/config.json` (63 bindings). Defaults empty in code
(`AppConfig::defaults().keybindings = {}`, `old/src/configs/config.cpp`,
`old/sys.txt` PART S). Resolution via
`old/src/configs/keybindings.cpp/part2/raw/ext` and `InputContext`/`when_context()`
from `old/src/ui/ui_sidebar.cpp`.

| Key | Command | when | Notes |
|---|---|---|---|
| `escape` | `noni.mode.normal` | *(empty → all)* | |
| `ctrl+s` | `workbench.action.files.save` | `editorFocus` | duplicate of vim `:w` |
| `ctrl+p` / `space space` / `space f f` | `workbench.action.quickOpen` | `editorFocus` / leader-chords | override of `space f f` (dup) |
| `ctrl+b`, `space e` | `workbench.action.toggleSidebarVisibility` | `editorFocus` / leader | duplicate (2 entries) |
| `ctrl+t`, `space t t` | `workbench.action.terminal.toggle` | `editorFocus` / leader | duplicate |
| `ctrl+z` | `editor.action.undo` | `editorFocus` | undo also available via vim `u`, `:undo` |
| `ctrl+y` | `editor.action.redo` | `editorFocus` | Vim **already has** `ctrl+r` as redo — duplication. ⚠ same key as `redo`; the maker's inserted `ctrl+y` (in *Normal* only) uses engine `redowhen editorFocus; not insert` — see Bugs |
| `ctrl+f` | `actions.find` | `editorFocus` | search panel UI |
| `ctrl+g` | `workbench.action.gotoLine` | `editorFocus` | |
| `ctrl+space` | `editor.action.triggerSuggest` | `editorFocus` | also vim cmp-bound equivalent unavailable |
| `f2`, `space r n` | `editor.action.rename` | `editorFocus` / leader | |
| `f12`, `space l d` | `editor.action.revealDefinition` | `editorFocus` / leader | |
| `ctrl+/` | `editor.action.commentLine` | `editorFocus` | |
| `ctrl+a` | `editor.action.selectAll` | `editorFocus` | |
| `ctrl+c` | `editor.action.clipboardCopyAction` | `editorFocus` | |
| `ctrl+x` | `editor.action.clipboardCutAction` | `editorFocus` | |
| `ctrl+v` | `editor.action.clipboardPasteAction` | `editorFocus` | In **Insert** mode this is engine-bound too? check: notes says “ctrl when=insertFocus” — no insertMode flag so available in insert too (`sys.txt` T4 partial: ctrl+h/j/k/l not bound for insert) |
| `ctrl+e` | `editor.action.copyLinesDownAction` | `editorFocus` | |
| `alt+j`, `alt+down` | `editor.action.moveLinesDownAction` | `editorFocus` | |
| `alt+k`, `alt+up` | `editor.action.moveLinesUpAction` | `editorFocus` | |
| `ctrl+d` | `editor.action.deleteLines` | `editorFocus && insertMode` | vim CURP conflict: normal-mode CtrlD = half-page (hardcoded raw 4 in editor_normal.cpp:64-66) — the config has both; the normal one only reaches Normal path when in insert — OK, separated by when |
| `ctrl+d` | `editor.action.halfPageDown` | `editorFocus && normalMode` | |
| `ctrl+u` | `editor.action.halfPageUp` | `editorFocus && normalMode` | plus hardcoded raw 21 in normal |
| `ctrl+left/right/up/down` | `workbench.action.focusLeft/Right/Above/BelowGroup` | `editorFocus && normalMode` | window focus |
| `space f g` | `workbench.action.findInFiles` | leader chords | side search panel |
| `space f w` | `noni.search.grepWord` | leader chords | **stub** (see 2.4) |
| `space f b` | `workbench.action.showAllEditors` | leader chords | buffer picker |
| `space f r` | `noni.search.recent` | leader chords | recent files |
| `space o` | `noni.focus.sidebar` | leader chords | |
| `space b n`, `space b b` | `workbench.action.nextEditor` / `previousEditor` | leader chords | buffer jump |
| `space b d` | `workbench.action.closeActiveEditor` | leader chords | |
| `space w` | `workbench.action.files.save` | leader chords | |
| `space q q` | `workbench.action.quit` | leader chords | |
| `space c a` | `editor.action.quickFix` | leader chords | code actions picker |
| `space c f` | `editor.action.formatDocument` | leader chords | |
| `space l d` | `editor.action.revealDefinition` | leader chords | dup with f12 |
| `space l r` | `editor.action.goToReferences` | leader chords | |
| `space l s` | `workbench.action.gotoSymbol` | leader chords | |
| `space l w` | `workbench.action.showAllSymbols` | leader chords | |
| `space l R` | `lsp.restart` | leader chords | |
| `space l S` | `lsp.showStatus` | leader chords | |
| `space x x` | `noni.diagnostics.list` | leader chords | diagnostics picker |
| `space g s` | `git.showStatus` | leader chords | branch search listing? (opens git status picker) |
| `space g a` | `git.stage` | leader chords | whole-file stage |
| `space g u` | `git.unstage` | leader chords | |
| `space g x` | `git.discard` | leader chords | confirm prompt |
| `space g d` | `git.showDiff` | leader chords | |
| `space g f` | `git.refreshDiff` | leader chords | |
| `space g R` | `git.refresh` | leader chords | |
| `space g p` | `git.previewHunk` | leader chords | **stub** (`ui_commands_keymap.cpp:67` Messages only) |
| `space s s` | `workbench.action.restoreSession` | leader chords | |
| `space s w` | `workbench.action.saveSession` | leader chords | |

## 3.2 Hard-coded Normal/Visual/Insert-mode keys (`.cpp` “Where”)

File(s): `old/src/components/editor.cpp` (mode dispatch), `editor_normal.cpp`,
`editor_normal_prefix.cpp`, `editor_normal_edit.cpp`, `editor_visual.cpp`,
`editor_visual_state.cpp`, `editor_insert.cpp`, `editor_cursor.cpp`.
Vim prefixes (`m`/`g`/`"`/`'/`[`/`]`/op-enter) block all other input waiting for a
second key. Insert escape exits via `jk` (config escape also works by engine).

| Mode | Keys | Action |
|---|---|---|
| Normal | `h j k l / arrows` | motion |
| Normal | `w b e` | word motions |
| Normal | `0 ^ $ G` `gg` `gt/gT` | line start/first-nonblank/eol/EOM/goto top/tabs |
| Normal | `v V` | visual char/line mode |
| Normal | `d c y` + motion | delete/change/yank operators (also `dd cc yy`) |
| Normal | `m <a-z>` / `'<char>` | set mark / jump to mark |
| Normal | `/` `?` | search prompt (CommandLine) |
| Normal | `n N` | next/prev match |
| Normal | `i a I A o O` | insert entries |
| Normal | `x` | delete char (yank) |
| Normal | `s S C` | subst/change-to-eol |
| Normal | `p P` | paste after/before |
| Normal | `" <reg>` | pending register prefix for `y/d/p` |
| Normal | `u` Ctrl-R / `:u` `:redo` | undo/redo |
| Normal | Ctrl-U/D, PgUp/Dn | scroll half/full page |
| Normal | `:` | open CommandLine (ex mode) |
| Normal | `K` | LSP hover |
| Normal | `g d g D g y g r g i` | LSP navigation + last insert; `g c` comment operator, `g t g T` tabs, `g g` top |
| Normal | Tab / Ctrl-I, Ctrl-O | jump list forward / back (Tab binding to Sidebar removed per T5) |
| Normal | `[` `]` + d | diagnostic jump (wrap) |
| Visual | `d x c s y p P` | delete/change/yank/paste on selection; `p/P` also left visual |
| Visual | motions | extend selection; `gg` jump; no `o` cursor-flip, no `<` `>` indent, no gc-with-exchange |
| Visual | `esc` | leave visual (clear selection) |
| Visual | Ctrl-D/U PageUp/Down | scroll |
| Insert | `jk` | exit insert (deletes pending j — matches publisher; old note said jj but code has jk per T4) |
| Insert | esc, arrows, PgUp/Dn | move / page (with txn split) |
| Insert | Backspace, Delete, Tab, Enter | usual insert keys (Backspace merges with prev line) |
| Insert | Ctrl-D | `editor.action.deleteLines` from config (workbench) — separate |
| Insert | Ctrl-R | redo (hardcoded 18) |
| Insert | KEY_SIC | paste clipboard |
| Insert | printable | insert char; `j` arms `pending_j` |

## 3.3 Hardcoded per-widget keys (not via KeybindingEngine; each widget's own handle_input)

| Widget / Focus | Keys | Action | File |
|---|---|---|---|
| CommandLine | Enter | submit | `old/src/components/commandline.cpp` |
|  | esc / ctrl+c | close (config escape shortcut may preempt) | UOOM also `ui_input.cpp` |
|  | Backspace | pop char | |
|  | printable | append | |
| FilePicker | Enter, esc|Cy, arrows, PgUp/Dn, Home/End, Backspace+printable (depending on “allows type-through”) | old/src/components/file_picker.cpp (up to `take_selection`) |
| BufferPicker | Enter, esc, Up/Down wrap, Pg/PgUp, Backspace, printable filter | old/src/components/buffer_picker.cpp |
| LspPicker | same plus display uses different (wrap Up/Down, page) | old/src/components/lsp_picker.cpp |
| CompletionPicker | Enter accept, esc cancel, Tab accept, Up/Down wrap, PgUp/Dn page | old/src/components/completion_picker.cpp |
| SearchPanel | F6 case / F7 whole-word / F8 regex; Tab & Shift-Tab cycles Query → Replace → Results; Enter on Query→run; Enter on Replace→replaceAll; results with j/k/g/G/Enter/r | old/src/components/search_panel*.cpp |
| MessagesPanel | esc, k/Up j/Down PgUp/PgDn g/G | old/src/components/messages_panel.cpp |
| ConfirmPrompt | y/Yes, n/No, c Cancel, esc | old/src/components/confirm_prompt.cpp |
| InputPrompt | Enter resolve, esc cancel, Backspace, printable | old/src/components/input_prompt.cpp |
| Sidebar | arrows/h j k l, Enter open, `a` add file, `A` add dir, `d` delete, `r` rename, `x` cut, `c` copy, `p` paste, `R` refresh, `q` back to editor | old/src/components/sidebar.cpp 293-420 + sidebar_nav.cpp |
| TerminalPanel | arrows (SS3 via DECCKM), Home/End \033OH/\033OF, PgUp/PgDn scrollback or PTY, Del \033[3~, Tab `\t`, Enter `\r`, Backspace 127, Ctrl-pills 1-26 raw, printable; Ctrl+] (29)/ctrl+t leave focus; esc → PTY | old/src/components/terminal_panel_part2.cpp (+ `ui_input.cpp` terminal branch) |
| Hover popup | (no key interactivity; dismissed by moving cursor/modes) | old/src/components/hover_popup.cpp |

## 3.4 Keybinding-engine internals (evidence for the config table)

File(s): `old/src/configs/keybindings.cpp` + `keybindings_part2.cpp`
+ `keybindings_raw.cpp` (from_raw mapping of every get_wch result: 27=Esc,
127=Backspace, 8=ctrl+h, 10=ctrl+j, 13=Enter via nonl(), F1-24, ctrl 1-26 etc.)

- Chords: whitespace-separated tokens with `ctrl/alt/shift` prefixes; named keys
  (`enter return space slash period comma semicolon quote apostrophe backtick
  backslash leftbracket rightbracket minus equals underscore + also `;`
  and F1..F12 = KEY_F with `shift+F` variant).
- `when` clauses: context flags joined by `&&` and `||` only (no `!` negation);
  context built on each key (focus + mode + visibilities) —
  `old/src/ui/ui_sidebar.cpp::when_context()`.
- Prefix (partial chord) → Pending: no timeout; cleared on Unmatched/Esc and on focus
  change. `sys.txt` T4 documented: Engine ownership rules in Editor normal/visual
  (`engine_owns_token` — `old/src/configs/keybindings_detail.hpp:17`) so the engine
  only consumes ctrl/alt/F/Esc/Space, forcing all other keys to the vim layers.
- Escape + `noni.mode.normal` works everywhere via config but Terminal is
  special-cased before resolve (`old/src/ui/ui_input.cpp` terminal branch, T1.4).
- The engine keys and vim keys interplay is confirmed by
  `old/src/configs/keybindings_detail.hpp` + `old/sys.txt` PART T4.

---

# 4. Config options (every key in `config.json`)

File: `old/config.json` (the shipped file) + `old/src/configs/config.cpp` +
`old/includes/configs/config.hpp` (defaults). `old/sys.txt` PART S cross-checked.

| Key | Type | Default (code) | Shipped value | Effect |
|---|---|---|---|---|
| `appName` | string | `"noni"` (`config.hpp` in-class) | `"noni"` | **Stored only; not read anywhere else** (nothing uses it after load) |
| `sidebarWidth` | int | `25` | `25` | Explorer/search sidebar column width (`ui.cpp` layout: hidden when 0, search side min 36) |
| `lineNumberWidth` | int | 5 (in code) | `7` | line-number gutter columns |
| `escDelayMs` | int | 25 | `25` | ncurses Esc vs CSI sequencing window (`set_escdelay` in `ui_init.cpp`) |
| `syntaxAutoInstall` | bool | true (only `is_bool` check) | `true` | background grammar install on first open of a language (`GrammarInstaller::set_auto_install`, `ui.cpp`) |
| `keybindings` | array | `{}` (vector; load wipes and rebuilds) | 63 entries | chord→command table; engine-only layer (see §3) |
| `lsp.autoInstall` | bool | true | `true` | missing server binaries → background install (LspInstaller) |
| `lsp.servers[].language` | string | — | `"cpp"`, `"python"` | matches `Language` names — `cpp`/`c` fallback logic for C/C++ |
| `lsp.servers[].command` | string[] | — | `["clangd"]`, `["pylsp"]` | argv passed to child; argv[0] resolved from PATH or installer dir |
| `lsp.servers[].rootMarkers` | string[] | — | `["compile_commands.json",".git"]`, `["pyproject.toml",".git"]` | workspace root scan order (file dir then workspace root) |
| `terminal.shell` | string | `""` → `$SHELL` → `/bin/sh` | `""` | PTY shell command |
| `terminal.height` | int | 12 | `12` | bottom terminal rows, clamped 5…(screen/2) |
| `terminal.scrollback` | int | 5000 | `5000` | ring buffer lines in TerminalScreen (min 0) |
| `extensions.*` | object map | empty | `noni.hello:{greet:"Welcome back"}` | per-extension `ExtensionContext::extension_config()` |
| `formatOnSave` | bool | true (T6 default, per config defaults) | `true` | await LSP format (up to 2s) on every save (see freeze issue below) |
| `clipboard.sync` | bool | true | `true` | sync yank/copy/cut to system clipboard (wl-copy/xclip/xsel/OSC52) |
| `autosave.mode` | `"off xhttp"` \| `"afterDelay"` | `"afterDelay"` | `"afterDelay"` | autosave tick behavior |
| `autosave.delayMs` | int | 1000 | `1000` | autosave delay |

Also in filesystems referenced but NOT config keys (paths hardcoded):
`~/.local/share/noni/lsp` and `~/.local/share/noni/tree-sitter`, workspace-level
`<ws>/.noni/{session.json, recovery/, tree-sitter/, lsp/}` (XDG dirs per `old/sys.txt`
PART H). XDG_DATA_HOME is documented as **unused**.

---

# 5. Known problems (mapped from your notes + prompt examples)

Each item: what the note said, evidence in code, verdict **DESIGN** or **BUG**.

1. **“Visual mode selection galat hai” (visual selection not behaving)** — **DESIGN** + partial BUG.
   Root causes traced:
   - Selection is a `Window`-owned view (anchor + cursor) with `SelectionKind`
     (`old/includes/editor/selection.hpp`), but a mismatch in the reverse-mode normalization:
     `normalize_character_range` treats *cursor-at-EOL* as including the newline of that
     line and then the next line start (`selection.hpp:97-101`) — editing right after a
     Visual-reverse selection at EOL can therefore merge lines unexpectedly; vim instead
     keeps an exclusive-end at the cursor cell. This is a semantics bug, not just UX.
   - There is **no `o`/`O` cursor-flip in Visual mode** (`old/src/components/editor_visual.cpp`
     has no `o` handling — only `g g` for file start at lines 37-49, `d/x/c/s/y/p/P`), so
     once the cursor passes the anchor users lose the ability to shrink back without
     re-descending — classic vim-mode selection feels “galat”.
   - No cross-split consistency is enforced (selections are per-Window;
     `old/includes/editor/window.hpp` stores `selection_` per Window while Buffer is
     shared) — poor coupling if the same Buffer is open in two splits, potential
     unsupported `Window::selection` when window dies while Buffer persists —
     **DESIGN** smell too.

2. **“LSP freeze karta hai”** — **DESIGN** (with at least 3 concrete root causes):
   - `formatOnSave` path (`old/src/ui/ui_tabs.cpp:85-110`): the save action **spin-waits
     on the main/UI thread** for a formatting response (`while … lsp().pump(); sleep 20ms`
     up to a 2s deadline) — a slow/failing server stalls every save, i.e. the whole editor.
   - `LspProcess::write_all` (`old/src/lsp/lsp_process.cpp:153-172`) performs a **blocking
     `write()` to the server's stdin pipe** from the main thread; if a server stops
     reading (clangd busy on a big project) the pipe buffer fills and the editor hard-freezes
     while trying to push `didChange`. Same class of issue for `Clipboard::write`
     (`old/src/utils/clipboard.cpp::exec_write`, blocking `waitpid` on the main thread for
     wl-copy/xclip/xsel) — any external write/click can block the UI for seconds.
   - `LspProcess::stop()` (`old/src/lsp/lsp_process.cpp:108-146`): poll-wait SIGTERM grace
     of up to 1s (`usleep(50000)` loop) runs on the main thread → quitting/restarting a
     misbehaving server freezes the UI while the process exits.
   - Net verdict: the pump-on-main design (`sys.txt` T7.1 removed reader threads to fix a
     different threading bug, and T8 kept pending requests with no `$ /cancelRequest`,
     no timeout) puts **all server interaction synchronously on the UI thread** —
     a DESIGN problem where any server/pipe hiccup becomes a UI freeze.

3. **`ctrl+y` redo bug-like effect** — **BUG** (keymap design).
   - `ctrl+y` is engine-bound to `editor.action.redo` with `when="editorFocus"` only
     (`old/config.json` entry + `old/src/ui/ui_commands_nav.cpp:18`). Because the when
     clause is not `normalMode`-gated, pressing `ctrl+y` in **Insert** mode also triggers
     redo instead of literal behavior; worse, `ctrl+y` is also the standard vim
     scroll-line-up key, so the mapping both *duplicates* `ctrl+r` and *steals* a
     scroll key. Combined with `editor_normal_edit.cpp` hardcoded `ctrl+R` redo, undo/redo
     ownership is triple-mapped (`u`, `ctrl+r`, `ctrl+y`, `:u`/`:redo`).

4. **“Backspace after space at indent” / merge-indent** — **BUG** (behavior gap).
   - Insert-mode Backspace at col 0 merges with the previous line
     (`old/src/components/editor_insert.cpp:77-82`) but carries the full previous line's
     *end* state — it does not strip leading indent on join (no smart-join at all;
     buffer `delete_char_before(line, 0)` just removes the newline and leaves whatever
     indent spacing followed) and does not restore the “col-0 after newline” column:
     this makes the reported “backspace after space at indent” and “indent inside
     #if/#endif → object-init” repros visible. The vim/nvim equivalent always handles
     indent on join.

5. **Missing “col 0 edit after newline+enter” handling** — **BUG**.
   - Probably the same root cause as (4): `insert_newline` in Buffer does not copy current
     indentation (`old/src/editor/buffer_insert.cpp` raw_insert_newline) unless the caller
     was explicit; editor-level Insert Enter (`editor_insert.cpp:93-100`) does not compute
     an indent — pretty sure there's no autoindent feature at all (grep confirmed no
     indent computation on Insert Enter).

6. **“Backspace flow / command-line boxing around entry” (search cockpit)** — **BUG** (minor).
   - Insert `jk`/`jj` handling: `editor_insert.cpp:108-117` uses the `key >= 32 && key <= 126`
     literal-ASCII gate to arm `pending_j`, but the UTF-8 path (`insert_utf8`) inserts a
     **string** that could start with `'j'`, which sets `pending_j = false` and thus
     breaks the “j then k” sequence if a multi-byte char begins with 'j' bytes (0x6A is ASCII —
     multi-byte UTF-8 never starts with 0x6A, so this specific case is safe) — the real
     residue is that the editor does not arm pending_j for **full-width or multi-byte**
     'j'-shaped chars because the codepoint path resets the flag (editor_insert.cpp:130),
     so `jk` can be defeated by a non-ASCII 'j'-looking char — minor.

7. **Font fallback / UTF-8 popup rendering in Hover popup** — **Unknown**, root candidate.
   - `HoverPopup` (old/src/components/hover_popup.cpp) draws decoded UTF-8 on the fly
     using `mvwaddnstr` (wide-char build); no font fallback logic is inside Noni (it
     depends on the terminal font + ncursesw), so reported glyph fallback issues are
     environmental — **Unknown** in code.

8. **陡`space f w` “grep word” and `space g p` “preview hunk” are placeholders** — **Stub**,
   confirmed: `old/src/ui/ui_commands_keymap.cpp:63` and `:67` are the only implementations
   (Messages::info with “PHASE 5”), matching `old/sys.txt` PART AD.

9. **Code actions that are command-only can't be executed** — **Stub/partial**:
   `old/src/ui/ui_lsp.cpp:184` logs “Code action command not executed: {}”;
   LspCodeAction has `command` but no execution path (`old/includes/lsp/lsp_models.hpp:53`).

10. **`git.stage`/`unstage`/`discard` whole-file only** — **Partial** (design said so).
    - `ScmService` (old/src/scm/scm_service.cpp) exposes file-level ops only.
      Hunk-level stage/preview was explicitly deferred (`old/sys.txt` PART AD list).
    - `ScmGit` uses bare `.git` presence for repo detection with no upstream/branch graph —
      minor.

11. **No thread/mutex on Buffer content; all edits on main thread** (author data model) —
    **DESIGN** mitigation already present (all mutations main-only), but if the rewrite
    adds background indexing it must keep the same rule, else undo/redo+syntax+diagnostics
    (internal to Buffer: undo stacks, `TreeHighlighter`, `DiagnosticSnapshot`) race.

12. **`statusbar chord` display for pending prefix is good but keybinding reset rules
    around focus changes are subtle** (`KeybindingEngine::clear_chord` is called from
    many places) — **DESIGN** note; rewrite should unify (no per-callsite spellings).

13. **LSP didChange incremental UTF-16 with multi-line deletes falls back to full resend**
    (`old/src/lsp/lsp_service.cpp` did_change_incremental fallback around code 1076-1080)
    — **BUG**/performance gap; correct fix is proper multi-line UTF-16 arithmetic in
    `TextMetrics`.

14. **`Syntax` fallback lexer treats JSON with C-style comments**` (`uses_c_comments`
    includes JSON — `old/src/syntax/syntax.cpp` highlight path, noted in `old/sys.txt`
    PART Y fallback section) — **BUG**: JSON with `//` comments will be mis-tokenized
    (also json standard doesn't have comments; tolerance is intentional but affects
    `//`-styled `jsonc` vs strict `json`).

15. **Prompt's claimed “no statusbar after-typing caret”: the caret/visual cursor is
    drawn via `wmove` only for the *active* window** (`old/src/components/editor_draw.cpp:62-68`
    draws cursor only when `is_active`) — matches editor conventions, safe.

16. **T1.4 unknown-key fallback** — engine drops led chords (old `sys.txt` T1.2) —
    partially fixed by T4 but engine prefix rules (“Prefix rule lost the last byte”)
    were consciously accepted (documented in `old/sys.txt`); rewrite should unify.

17. **`ramRectangle` unknown/`search ext` unknown** — prose terms in the user's notes
    which don't map to any code symbol in `old/` — **Unknown**.

18. **`research/Git trees` “delete tabs on cloud change” — cloud, unknown** — **Unknown**
    (no data in old repo; recovered sessions skip untitled buffers, `session.cpp:76-77`).

19. **`Ctrl+e copy-line-down` runs clipboard sync write with 3 subprocess git/popenc/
    appropriateness** (see Clipboard write tree, same problem family as (2)) —
    **DESIGN**: any external command on UI thread.

20. **“in-Buffer regex behavior” (soft query)** — currently uses ECMAScript + icase
    + whole-word checks with try/catch silently swallowing errors after validation
    (`old/src/editor/buffer_search.cpp:11-118`); plus `BufferSearchQuery` has no test
    path for happened-in-file-but-not-in-editor-corrections (only per-session) —
    Partial, not necessarily Bug from the code only.

---

# 6. Design smells (for the rewrite)

1. **UI god-class / god-object**: `old/includes/ui/ui.hpp` (the `UI` class) owns **16**
   widget members + core/config/keys/extension-manager/fs-watcher + layout ints + a dozen
   painted-delta flag mirrors (`ui.hpp:45-115`). Almost everything is coupled through it.
2. **Buffer couples syntax+diagnostics+FS into one class** (`old/includes/editor/buffer.hpp`
   owns `TreeHighlighter`, `DiagnosticSnapshot`, `FS`, listeners, mtime, display caches,
   and undo stacks). Rewrite should split: - document model - parse - diagnostics - persistence.
3. **Vim key handling duplicated in three switch statements** (Normal/Visual/Insert)
   with `is_motion_key` helpers re-implemented per file
   (`old/src/components/editor_normal.cpp:75-103`, `editor_visual.cpp:68-74`,
   `editor_insert.cpp` for Insert routing) — no unified command/“action” abstraction below
   the hard-coded layers; command registry (`old/src/commands/command.cpp`) only holds
   workbench commands.
4. **Editor holds UI callbacks instead of events**
   (`old/includes/components/editor.hpp:37-53`: `on_next_tab_`, `on_search_forward_`,
   `on_command_open_`, `on_goto_definition_`, `on_hover_`, … attached by
   `old/src/ui/ui.cpp`) — editor ⇄ UI concurrency through raw function slots; should be
   event/commands.
5. **Shared mutable LSP/SCM state with locks (no owner object)**
   (`old/sys.txt` PART AB table: `LspService::mu_`, `ScmService::mu_`, `PtySession out_buf`
   without mutex “T7.6” etc.) — a mixed model where background jobs write under mutex
   while main reads directly. Trackable but fragile for rewrite.
6. **Blocking subprocesses on the UI thread** (clipboard exec, git CLI popen in
   ScmService background is ok, but git **stage/unstage/discard run synchronously on
   main** `scm_service.cpp`; also `Clipboard::write` on yank) — see (2) in problems.
7. **In-place 200-line split created artificial fragmentation** — e.g. UI logic smeared
   over `ui_*.cpp` (15+ files) with headers like `ui_private.hpp` to share hidden state;
   same for lsp (`lsp_service*.cpp` 6 files) and search_panel (3 files). The
   `sys.txt` file map lists where every piece landed only approximately — the rewrite
   should not inherit this organization.
8. **Single global Focus enum + per-widget handle_input routing is hard-coded** in
   `ui_input.cpp`; adding a widget means editing the god object + focus enum + when_context
   + hand routing — 4 places. Rewrite should use a widget stack with automatic routing.
9. **Engine-owns-key rules encode T1-series bug fixes as special cases**
   (`engine_owns_token`, literal-context Space handling, Esc/Ctrl+] in terminal,
   pending-chord clears on focus changes, insert `jk` handled in editor not engine:
   see `old/src/configs/keybindings_detail.hpp`, `ui_input.cpp`, `editor_insert.cpp:8-27`)
   — key-ownership logic that is currently spread across ≥ 4 files; should be one
   declarative table.
10. **`MiniJson` is written instead of a dependency, and is incomplete** (see 2.10) —
    LSP correctness depends on it. Rewrite: use a real JSON lib.
11. **SessionStore writes untitled buffers skipped; recovery requires market “≥2s dirty”;
    recent files tracked but no command palette** — scattered, duplicated workspace state
    paths (`old/src/workspace/*` vs `old/src/ui/ui_workspace.cpp`).
12. **`git stage` runs `git add` synchronously on main** (`old/src/scm/scm_service.cpp`
    stage/unstage/discard_worktree sync + `request_refresh` after) — must be background
    in rewrite.
13. **`TreeHighlighter` is per-Buffer but shares a static grammar cache with
    `GrammarInstaller::generation()`** for cache invalidation (`old/src/syntax/
    tree_highlighter.cpp meta_changed`) — implicit global dependency.
14. **Undo transactions store cursor positions inside Buffer** (undo txn heads) —
    the “cursor” lives in Window, not Buffer; whether the restore matches the active
    split semantics was the source of subtle bugs (see Problem (1)) — unify in rewrite.

---

# 7. Must-keep vs can-drop (suggestion — not from the notes)

## 7.1 Must keep for v1 rewrite (core product)

1. **Editing** — Buffer model with undo/redo transactions, mtime-conflict awareness,
   atomic save, UTF-8 handling (`buffer.hpp`, `buffer_io.cpp`, `fs.cpp`).
2. **Modal vim editing** with the current motions/operators set (fills the practical
   80% of vim editing; fill gaps like `r`, counts, `%`, `o` later).
3. **Keybinding engine with config.json when-clauses + engine-owns-{ctrl,alt,F,esc,space}**
   (this is what makes the workbench layer declarative;
   `keybindings*.cpp` + `command.hpp`).
4. **Command registry + workbench command IDs** (`command.cpp` + `ui_commands*.cpp`) —
   the “VS Code-identifier” pattern is good for keymap reuse.
5. **Buffer search `/ ? n N`** with case/whole-word/regex options.
6. **Project search panel (read-only results + open match)**.
7. **Split-window layout + focus traversal** (`window_layout.hpp`) — it already works
   well enough.
8. **-sidebar absent list (for continuity):** sidebar tree with async scandir + expand +
   open/rename/delete — essential for "editor" completeness, keep.
9. **LSP** — completion/definition-references/rename/diagnostics/hover with the
   hardened process/session state machine (`lsp_service*`, `lsp_session*`,
   `lsp_process.cpp` fixes: non-blocking write, timeouts, cancellation).
   - drop LSP auto-installer for v1? (it works; biggest risk of breakage — can keep
     but must never block UI).
10. **Tree-sitter highlighting with the GrammarInstaller** (including the caching and
    per-line incremental parse) + the fallback lexer for languages without grammars.
11. **Integrated terminal (PTY + VT + scrollback)** — important product feature; gotcha:
    keep the poll-based single-thread pumping, but never let writes block.
12. **Git status + branch badge + gutter diff** via `git` CLI (no libgit2 dependency).
13. **Session save/restore + crash recovery** (`workspace/session.cpp`, `recovery.cpp`)
    — they were designed carefully (no auto-overwrite of sources).
14. **Autosave afterDelay** + **format…(on save or manual)** — but fix the freeze first.
15. **Status bar + header + line-number gutter** visuals (cheap, expected).

## 7.2 Can drop / postpone for v1

- LSP **auto-installer** backends (pip/npm/curl/go) — users can install servers
  themselves; the catalog is nice-to-have (`lsp_installer_backends.cpp`).
- **Extension/plugin API** — only a demo extension exists; defer until the core is
  stable (`extensions/*`).
- **`:help` engine** page system (nice, but text-only anyway; can be a few markdown
  pages without a loader) — keep `:messages`/`:logs`.
- Icons/`icons_file*.cpp` (5 large static-data files for per-file glyphs) — cosmetic.
- LSP **code actions** beyond `edit`-only (i.e. command-only actions) — stub anyway.
- `git.previewHunk` + hunk-level git ops.
- `space f w` grep-word (until search panel grows word-at-cursor entry).
- Visual-mode `<`/`>` indent ops (implement once Buffer/splits firmed up).
- Hover popup with rich styling (keep plain popup only).
- Window focus shortcuts binding beyond current few; polish later.

## 7.3 Unknown / not fully analyzed files (for `new/` planning)

Listed from `old/src/`, not duplicated in full sections above; analysis came from
`old/sys.txt` PART Q/R (verified 2026-09-26) rather than direct read (deemed stable given
git log Sep 26 = last function change; light confidence):

- `old/src/syntax/ts_hl_detail.cpp` + `ts_queries.hpp` (TS capture mapping specifics)
- `old/src/syntax/grammar_download.cpp` / `grammar_tools.cpp` / `grammar_request.cpp` /
  `grammar_state.cpp` (URL building details per language)
- `old/src/terminal/vt_parser_csi.cpp` subtleties (full CSI dispatch, no table in sys.txt)
- `old/src/components/lsp_picker.cpp*` mapping between picker kinds and results
- `old/src/sidebar/dirscanner_detail.hpp` + `old/src/sidebar/dirscanner_part2.cpp` details
- `old/src/editor/window_layout.cpp` split-tree edge cases (root replacement,
  window close when 2 leaves left)
- `old/src/ui/ui_recovery.cpp` prompt ordering details
- `old/src/utils/fs_watcher_part2.cpp` (inotify event classes watched)
- 200-line-split microfiles like `old/src/components/editor_lines.cpp`,
  `editor_jump.cpp` (titles derived from naming; content not read line-by-line)

*(sys.txt's T8 “LAST VERIFIED” line (2026-09-26) plus unchanged file mtimes after that
date gave high confidence in the inventory above.)*
