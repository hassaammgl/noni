# Noni master progress

Branch: `noni-master` (created from `master` @ `d9b7479`).
Continue here on new chats: this file + `.cursor/rules/noni-master.mdc`.

Statuses: `todo` | `doing` | `done` | `blocked`

## Decisions

- Dirty working tree at start (LSP installer, config, grammar, UI, etc.) **kept** — not discarded (R7). Branch carries prior WIP; PART 0 commit is setup files only.
- `g++` 16.2.1 → use `-std=c++23` (T0).
- No libgit2 C API in source (`#include <git2>` none) → drop `-lgit2` (T0).
- Repo-root `Makefile` was gitignored (`gitignore:45`); T0 stops ignoring it and tracks the handwritten Makefile.
- C++23 libstdc++ on this machine: `__cpp_lib_expected=202211`, `__cpp_lib_print=202406`, `__cpp_lib_flat_map=202511`, `__cpp_lib_move_only_function=202110`.
- T3 `nonl()`: Enter = KEY_ENTER or CR(13); Ctrl+J = LF(10); Ctrl+H = 8 vs Backspace 127/KEY_BACKSPACE. Ctrl+M is the same byte as Enter on CR terminals. No Prefix chord timeout.
- T4 ownership: engine_owns_token; vim prefixes editor-owned; Insert `jk`; Ctrl when=editorFocus except window ctrl+arrows (normalMode).
- T5: ctrl+h/j/k/l NOT bound for window focus — after `nonl()` bytes 8/10 are distinct from Bs/Enter, but binding them would fight Insert ctrl+h and vim muscle memory; ctrl+arrows cover focus.
- T6: `formatOnSave` default true; `clipboard.sync` default true; `autosave.mode=afterDelay` delayMs=1000.

## PART 0 — bootstrap

| id | status | note | commit |
|----|--------|------|--------|
| P0 | done | rules + PROGRESS.md + branch | 3fc8da7 |

## T0 — BASELINE

| id | status | note | commit |
|----|--------|------|--------|
| T0 | done | Makefile c++23, -MMD -MP, -O2 -g, debug, drop tests + -lgit2; track Makefile | e3e2855 |

Warnings (fill after first build).

## T1 — ROOT-CAUSE TRIAGE

| id | status | note | commit |
|----|--------|------|--------|
| T1.1 | done | cwd→exe→~/.config/noni; parse fail no partial; statusbar + "loaded N bindings, M commands, unbound: …" (ui.cpp ctor, config.cpp load). Symptom: cwd-only load + empty defaults unbound all chords. | |
| T1.2 | done | CONFIRMED steal, fix T4. Engine first (ui.cpp:3517-3523): Prefix/Matched never reach editor. config.json: `g t/g g/g T/g d/g D/g y/g r` + `u` (editorFocus&&normalMode). resolve_pending (keybindings.cpp:354-412) has no editor-modal skip (`any_leader_prefix` unused). `g`=Prefix → editor.pending_g never sets; `gi`/`dgg`/`ygg` break (second key Unmatched, not re-injected). `u`=Matched engine undo. `m`/`'`/`"` not in config.json → editor tak pahunchte hain. | |
| T1.3 | done | Query/Replace always used SearchResults (ui.cpp Search branch) so Space = leader Prefix, never inserted (search_panel.cpp:267 printable). Fix: in_text_field() → SearchText; Results still SearchResults. Code path: command.hpp context_is_literal Space unmatched; build pass. | |
| T1.4 | done | Esc 27 UI leave (ui.cpp Terminal) + empty-when `escape` would steal; case 27 in panel unreachable. Leave now 29 Ctrl+] (ctrl+t still toggle). F1–F12 were dropped (default only 1–26/32–126). Forward xterm seqs. 8-bit meta UNVERIFIED. | |
| T1.5 | done | stale objects — T0 -MMD -MP + -include .d (confirm: ui.d lists headers) | e3e2855 |
| T1.6 | done | initialize result stored (sync_kind, hover, formatting). didChange: 0 skip / 1 full / 2 incremental. rootMarkers walk from file path (session_for file_hint). Crash Starting|Running → Messages::error once. Client now advertises hover+formatting. | |
| T1.7 | done | getch() ASCII-only; widgets `key>=32 && key<=126` dropped UTF-8 bytes. Fix: get_wch; cp>126 → UTF-8 insert_focused_utf8; KeyToken.codepoint; Backspace pop_codepoint. -DNCURSES_WIDECHAR=1. | |
| T1.8 | done | Insert still `jj` Esc not `jk` (editor.cpp:1640, T4). Engine Prefix no timeout (T3). empty-when `escape` matches all contexts (terminal special-cased T1.4). when_context concatenates focus+mode every key (not stale). any_leader_prefix unused leftover (T4). | |

## T2 — SPLIT PASS (200 lines)

File list after `wc -l` (T2 start / HEAD). Split = pure move. No commit until you ask. All listed files now ≤200. T3 blocked until you confirm `make` is clean.

| file | lines | status | commit |
|------|------:|--------|--------|
| src/ui/ui.cpp | 3532 | done | |
| src/components/editor.cpp | 1803 | done | |
| src/lsp/lsp_service.cpp | 1653 | done | |
| includes/ui/icons.hpp | 1370 | done | |
| src/editor/buffer.cpp | 849 | done | |
| src/components/sidebar.cpp | 775 | done | |
| src/syntax/tree_highlighter.cpp | 770 | done | |
| src/syntax/grammar_installer.cpp | 739 | done | |
| src/syntax/syntax.cpp | 735 | done | |
| src/utils/fs.cpp | 648 | done | |
| src/help/help_docs.cpp | 496 | done | |
| src/editor/ex_commands.cpp | 488 | done | |
| includes/editor/motion.hpp | 456 | done | |
| src/configs/keybindings.cpp | 448 | done | |
| src/terminal/terminal_screen.cpp | 410 | done | |
| src/terminal/vt_parser.cpp | 403 | done | |
| src/configs/mini_json.cpp | 376 | done | |
| src/components/search_panel.cpp | 354 | done | |
| includes/editor/window_layout.hpp | 338 | done | |
| src/scm/scm_git.cpp | 336 | done | |
| src/lsp/lsp_installer.cpp | 335 | done | |
| src/components/terminal_panel.cpp | 335 | done | |
| src/utils/text_search.cpp | 305 | done | |
| src/ui/theme.cpp | 298 | done | |
| src/scm/scm_service.cpp | 294 | done | |
| src/components/file_picker.cpp | 288 | done | |
| src/editor/buffer_search.cpp | 277 | done | |
| src/utils/fuzzy.cpp | 266 | done | |
| includes/ui/ui.hpp | 264 | done | |
| src/sidebar/dirscanner.cpp | 262 | done | |
| src/terminal/pty_session.cpp | 260 | done | |
| src/utils/fs_watcher.cpp | 252 | done | |
| src/components/lsp_picker.cpp | 252 | done | |
| includes/editor/editor_core.hpp | 243 | done | |
| src/utils/text_metrics.cpp | 230 | done | |
| src/utils/str.cpp | 224 | done | |
| src/editor/buffer_manager.cpp | 224 | done | |
| includes/lsp/lsp_service.hpp | 223 | done | |
| src/components/buffer_picker.cpp | 214 | done | |
| src/editor/editor_core.cpp | 209 | done | |
| src/lsp/lsp_process.cpp | 204 | done | |

## T3 — INPUT LAYER

| id | status | note | commit |
|----|--------|------|--------|
| T3 | done | nonl(): Enter=KEY_ENTER or CR; Ctrl+J=LF; Ctrl+H=8 vs Bs=127. KEYLOG, Esc-peek Alt (skip Terminal), define_key CSI 1;3/1;5/1;7, from_raw ctrl+]/ctrl+/, f1–f12+shift+F, punctuation names, statusbar chord_ (no Prefix timeout). | |

## T4 — KEY OWNERSHIP

| id | status | note | commit |
|----|--------|------|--------|
| T4 | done | engine_owns_token: EditorNormal/Visual me sirf ctrl/alt/F/Space/Esc. Vim g/u/m/'/"/d/c/y editor pe. Insert jk. : + gd/gD/gy/gr/gi callbacks. config: hatao g*/u/:/tab steal; ctrl when=editorFocus; ctrl+z; ctrl+arrows normalMode. Terminal DECCKM. SearchText/sidebar/term leave pehle se OK. | |

## T5 — CONFIG REWRITE

| id | status | note | commit |
|----|--------|------|--------|
| T5 | done | config.json = PART 2-A/C (63 bindings); config.json.bak; register stubs in ui_commands_keymap.cpp; selectAll/deleteLine/halfPage wired. ctrl+h/j/k/l skip (PROGRESS). | |

## T6 — MISSING FEATURES

| id | status | note | commit |
|----|--------|------|--------|
| T6.1 | done | Clipboard::write wl-copy→xclip→xsel→OSC52; yank/copy/cut sync when clipboard.sync | |
| T6.2 | done | gcc / Visual gc / ctrl+/ comment toggle (lang tokens) | |
| T6.3 | done | K → textDocument/hover → HoverPopup | |
| T6.4 | done | [d ]d diagnostic jump wrap + echo | |
| T6.5 | done | formatting request + apply; formatOnSave (2s timeout) | |
| T6.6 | done | alt+j/k/up/down move; ctrl+e copy line down | |
| T6.7 | done | autosave.afterDelay (default 1000ms); skip untitled/external mtime; no format | |
| T6.8 | done | Space x x diagnostics picker; f r recent; f w / g p placeholders | |

## T7 — PERFORMANCE

| id | status | note | commit |
|----|--------|------|--------|
| T7.1 | done | poll() main loop: stdin+wake+inotify+PTY+LSP fds; timeout=timers; needs_render_; no PTY/LSP reader threads | |
| T7.2 | done | per-component dirty_; paint_if_dirty + wnoutrefresh only dirty; resize force_paint_all | |
| T7.3 | done | Buffer line display cache + hot-line prefix; motions/draw use cache; layout rects_cache reuse | |
| T7.4 | done | syntax sync_visible: lexer limited to viewport; skip redraw sync when clean | |
| T7.5 | done | keybinding first-token index; pending.reserve(8) | |
| T7.6 | done | LSP/PTY out_buf main-thread (no mutex); JSON-RPC frame without format double-alloc | |
| T7.7 | done | fuzzy filter scores native path/filename views; display string only on hit | |
| T7.8 | done | `make lto` OK (user 2026-09-26): `-O2 -flto=auto` linked `build/bin/app`. Default `make` stays `-O2 -g`. | |

## T8 — FINAL

| id | status | note | commit |
|----|--------|------|--------|
| T8 | doing | sys.txt re-verify + LAST VERIFIED + PART AD + T/U | |

## UNVERIFIED

- T1.4: 8-bit meta (`key >= 128 && key < KEY_MIN` → ESC+(key&0x7f)). Split ESC+key path verified by code (first getch 27 now reaches PTY). Combined-meta path UNVERIFIED on this terminal.
- T1.7: get_wch + UTF-8 insert compiled; runtime é/Urdu/emoji in TUI UNVERIFIED (no interactive run).
- T3: Alt Esc-peek, Ctrl+arrows define_key, KEYLOG, chord statusbar — code path only; interactive TUI UNVERIFIED (no `make` this turn).
- T4: ownership + jk + DECCKM — code path only; interactive gi/gd/jk/ctrl+arrows UNVERIFIED.
- T5: new keybindings load + diagnose unbound/unknown — needs user `make` + run.
- T6: clipboard tools/OSC52, hover popup, formatOnSave, autosave, move/copy lines — interactive UNVERIFIED (no `make` this turn).
- T7.1: poll() loop — code path only; idle CPU / key latency UNVERIFIED (user `make` + run).
- T7.2/T7.3: dirty paint + metrics cache — code path only; interactive UNVERIFIED.
- T7.4–T7.7: syntax/keybinds/LSP/fuzzy — code path only; UNVERIFIED until user `make`.
- T7.8: LTO verified by user `make clean lto` (linked). Fixed post-build: Focus::Hover switch + unused `row` in page_up/half_page_up.
- Bugfix: Sidebar/SearchPanel frozen after T7.2 — mutators never `mark_dirty()` so `paint_if_dirty` skipped selection/expand redraw. Fixed in sidebar_* + search_panel_part2; sys.txt PART AA.

## Pause

User asked to stop after T1.1 (2026-09-21); resumed 2026-09-22 (T1.2+).

## Existing warnings

Clean `-Wall -Wextra -std=c++23 -O2` build (user runs `make`). T4 removed unused `any_leader_prefix`.

Header-dep check: `-MMD` `.d` files generated (e.g. `build/src/ui/ui.d` lists `includes/ui/ui.hpp`). `touch includes/utils/logger.hpp && make` rebuilt dependents. Do not pipe `make` to `head` (SIGPIPE mid-compile).
