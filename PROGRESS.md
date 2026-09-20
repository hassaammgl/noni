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
| T1.2 | todo | engine vs editor prefix (confirm; full fix T4) | |
| T1.3 | todo | Search panel Space swallow | |
| T1.4 | todo | Terminal Esc/Alt/F-keys | |
| T1.5 | done | stale objects — T0 -MMD -MP + -include .d (confirm: ui.d lists headers) | e3e2855 |
| T1.6 | todo | LSP capabilities + rootMarkers + crash message | |
| T1.7 | todo | UTF-8 getch vs get_wch | |
| T1.8 | todo | other shared root causes | |

## T2 — SPLIT PASS (200 lines)

File list after `wc -l` (T2 start). One file = one commit. T3 blocked until this is done.

| file | lines | status | commit |
|------|------:|--------|--------|
| | | todo | |

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

(none yet)

## Pause

User asked to stop after T1.1 (2026-09-21). Next: T1.2 engine vs editor prefix.

## Existing warnings

Clean `-Wall -Wextra -std=c++23 -O2` build (code path: `make` after `make clean`; `build/bin/app` linked). One warning, not a runtime-bug class — left for T3/T4 (touches KeybindingEngine):

- `src/configs/keybindings.cpp:348` `any_leader_prefix` set but not used (`-Wunused-but-set-variable`). Sets on leader prefix match; never read. `any_prefix` is what drives Prefix vs Unmatched.

Header-dep check: `-MMD` `.d` files generated (e.g. `build/src/ui/ui.d` lists `includes/ui/ui.hpp`). `touch includes/utils/logger.hpp && make` rebuilt dependents. Do not pipe `make` to `head` (SIGPIPE mid-compile).
