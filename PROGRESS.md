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
| T3 | todo | KEYLOG, Alt, Ctrl+arrows, mappings, nl/nonl, chord UI | |

## T4 — KEY OWNERSHIP

| id | status | note | commit |
|----|--------|------|--------|
| T4 | todo | vim prefixes editor.cpp; search/sidebar/term; jk; when flags | |

## T5 — CONFIG REWRITE

| id | status | note | commit |
|----|--------|------|--------|
| T5 | todo | config.json = PART 2-A/C; backup .bak; register ids | |

## T6 — MISSING FEATURES

| id | status | note | commit |
|----|--------|------|--------|
| T6.1 | todo | clipboard write chain + OSC52 | |
| T6.2 | todo | comment toggle gcc/gc/ctrl+/ | |
| T6.3 | todo | hover K popup | |
| T6.4 | todo | [d ]d diagnostic jump | |
| T6.5 | todo | LSP format + formatOnSave | |
| T6.6 | todo | move/copy line | |
| T6.7 | todo | autosave | |
| T6.8 | todo | pickers + placeholder ids | |

## T7 — PERFORMANCE

| id | status | note | commit |
|----|--------|------|--------|
| T7.1 | todo | poll() event loop | |
| T7.2 | todo | dirty-flag render | |
| T7.3 | todo | Buffer/Window/Editor hot paths | |
| T7.4 | todo | syntax visible-range cache | |
| T7.5 | todo | keybinding resolve | |
| T7.6 | todo | LSP JSON/handoff | |
| T7.7 | todo | search/fuzzy alloc | |
| T7.8 | todo | LTO try | |

## T8 — FINAL

| id | status | note | commit |
|----|--------|------|--------|
| T8 | todo | sys.txt re-verify + LAST VERIFIED + PART AD + T/U | |

## UNVERIFIED

- T1.4: 8-bit meta (`key >= 128 && key < KEY_MIN` → ESC+(key&0x7f)). Split ESC+key path verified by code (first getch 27 now reaches PTY). Combined-meta path UNVERIFIED on this terminal.
- T1.7: get_wch + UTF-8 insert compiled; runtime é/Urdu/emoji in TUI UNVERIFIED (no interactive run).

## Pause

User asked to stop after T1.1 (2026-09-21); resumed 2026-09-22 (T1.2+).

## Existing warnings

Clean `-Wall -Wextra -std=c++23 -O2` build (code path: `make` after `make clean`; `build/bin/app` linked). One warning, not a runtime-bug class — left for T3/T4 (touches KeybindingEngine):

- `src/configs/keybindings.cpp:348` `any_leader_prefix` set but not used (`-Wunused-but-set-variable`). Sets on leader prefix match; never read. `any_prefix` is what drives Prefix vs Unmatched.

Header-dep check: `-MMD` `.d` files generated (e.g. `build/src/ui/ui.d` lists `includes/ui/ui.hpp`). `touch includes/utils/logger.hpp && make` rebuilt dependents. Do not pipe `make` to `head` (SIGPIPE mid-compile).
