# PREVIEW.md — what you will see at the end of every phase

Read your phase's section **before** saying GO for that phase. Pre-render phases
(P0–P1) have **no screen yet** — your "demo" there is ctest output, not pixels.
From **P2 onward you get a real editor**: each phase lands its features LIVE in
the running app, and each section shows the screen.

Scope note (revised 2026-10-23, v2): git is gone from every mockup (no signs, no
branch, no `:Gst`). Everything else stays: terminal pane (P14), explorer, LSP
diagnostics (P13), `:!`, OSC52 clipboard, sessions.

## Universal build commands

```
make test                                   # root wrapper: configure+build+ctest
make run ARGS=[file]                        # root wrapper: build + run the editor
./build/tests/noni_tests                    # tests binary directly (ASan build)
ASAN_OPTIONS=detect_leaks=1 ./new/build/noni   # sanitizer run (P2 onward)
cmake -S new -B new/build -DCMAKE_BUILD_TYPE=Debug  # if you want raw cmake
```

---

## P0 — CMake skeleton

**You will see** (no screen, no GUI):
```
$ cmake -B build -DCMAKE_BUILD_TYPE=Debug
-- The CXX compiler identification is GNU 16.2.1
-- Fetching doctest …
-- Configuring done      $ cmake --build build
   … linking …/new/build/noni
$ ctest --test-dir build
1/1 Test #1: smoke …***Passed
$ ./build/noni --version
noni v0.1.0 (CMake skeleton)
```

Try:
1. `./build/noni --version` → prints version line, exit 0.
2. `./build/noni` (no args) → info line "editor UI arrives in P2", exit 0
   (real editor launch waits for P2).
3. Deliberate break check: temporarily add `#include <ncurses.h>` into a
   `src/core/` header → build FAILS (layer check works) → revert.
4. `ctest --test-dir build` → smoke test Passed.

Not working yet: everything visible; no editor at all.

## P1 — Core buffer + undo

**You will see**: ctest listing dozens of core tests:
```
$ ctest --test-dir build -R P1 --output-on-failure
  text: cp_to_byte unicode … PASSED
  buffer: load missing → error … PASSED
  undo: dd,u → cursor restore … PASSED
100% tests passed
```

Try:
1. `ctest -R buffer` → buffer suite green.
2. `ctest -R undo` → undo/redo suites green (interleaving, cap 200, redo-branch cut).
3. Look at `new/tests/` — every ACCEPT bullet in ROADMAP P1 is a named test there.

Not working yet: no screen and no keys yet — P2 puts the editor on the glass.

## P2 — FIRST SCREEN — minimal editor

```
┌──────────────────────────────────────────────────────────────┐
│ main.cpp [+]                                     12,7   41%  │ ← statusline
│  1 #include <cstdio>                                         │
│  2                                                           │
│  3 int main() {                                              │
│   4_     printf("hi\n");                                     │ ← cursor
│  5     return 0;                                             │
│        ~                                                     │
│        ~                                                     │
│ :                                                            │ ← cmdline row
└──────────────────────────────────────────────────────────────┘
```

Run: `./new/build/noni main.cpp` (or root `make run ARGS=main.cpp`)

Try (the P2 script):
1. `./new/build/noni main.cpp` → file shown, cursor at line 1 col 1.
2. `h/j/k/l` → cursor moves; `3k` jumps three lines.
3. `0` / `$` → line start/end.
4. `i` → statusline `-- INSERT --`; type text; `<Enter>` copies the previous
   indent; Backspace; `<Esc>` exits.
5. `x` → delete char; `A` → append at end of line.
6. `u` → undo; `ctrl+r` → redo.
7. Resize the terminal → editor reflows (resize edge case).
8. Open a long file: scrolling past the bottom keeps the cursor visible
   (minimal follow; the full page keys land in P3).

Not working yet: `w/b/e/f/t/%/G` wait for P3; operators P4; registers P5; save
(`:w`) and prompts P6/P7. The build is sanitized (ASan on in Debug).

## P3 — Motions (live on screen)

Every motion in this phase is instantly usable in the editor; the same key
sequences run as differential test cases against the `nvim --headless -u NONE`
oracle (see AGENTS.md).

Try (the P3 script):
1. `w` `w` `w` → word hopping; `W` → WORD hopping; `b` back; `e` word end.
2. `f(` → next `(`; `t)` → just before the next `)`; `;` / `,` repeat.
3. `gg` / `G` / `123G` → file start / file end / line 123.
4. `{` / `}` → paragraph hops.
5. `ctrl+d`/`ctrl+u`/`ctrl+f`/`ctrl+b` → page scrolling; `zz` recenter;
   `H M L` → top/middle/low of the view.
6. `%` on a bracket → jump to the matching bracket.
7. Move `j/k` from a long line through short ones → the column is preserved
   (preferred_col).

Not working yet: operators (`d/c/y`) P4; registers P5; visual modes P6.

## P4 — Operators + text objects

1. `dw` → delete a word (to register); cursor to range start; `u` reverses it.
2. `d3w`, `2d3w` → counts compose on the live screen.
3. `cc` → clear line keeping indent; `cw` → correct word-boundary behavior.
4. `daw` / `ciw` / `ci(` → text objects; `da(` works across lines.
5. `3>>` / `<k` → shift lines; `gUiw` / `guw` / `g~~` → case ops live.
6. `=i{` inside a function → indent normalized (heuristic).
7. `yiw` + `p` → yank/paste basics (full register model P5).

Not working yet: registers/named regs are minimal (`"` unnamed) until P5;
visual-selection ops wait for P6.

## P5 — Registers, marks, dot, macros

1. `dd` then `p` → unnamed register; `"1p`/`"0p` → numbered history.
2. `"ayiw` then `"ap` → named register; `A`-append variant.
3. `mx`, `'x`, `` `x `` → set and jump to marks.
4. `ctrl+o` / `ctrl+i` → jump list round-trips.
5. `dw…dw…dw….` → dot repeat replays the last edit.
6. `qa…edits…q` then `3@a` → macro replay; `@@` last macro.
7. `git` the buffers: registers survive across operations as vim-model tests assert.

Not working yet: `+`/`*` register write-through OSC52 lands in P13/P14; visual
modes P6.

## P6 — Modes completion + prompt machinery

```
┌──────────────────────────────────────────────────────────────┐
│ main.cpp [+] -- VISUAL LINE --                    9,1   Top  │
│ > 9   selected line (whole lines highlighted)                │
│ > 10  second selected line                                   │
└──────────────────────────────────────────────────────────────┘
```

Try (the P6 script):
1. `V` j j `>` → visual-line selection follows motion keys; indent block once.
2. `o` swaps selection ends; `d` deletes the selection; `y` yanks it.
3. `o`/`O` in Insert mode → open below/above with indent.
4. `ctrl+w` / `ctrl+u` in Insert mode → word/line erase back.
5. `:` on the cmdline row → prompt appears; type a line, `<Esc>` (parse comes P7);
   history arrows cycle after first use.
6. `/pat` → `SearchRun` emitted; actual match highlighting lands in P9.

Not working yet: `:` doesn't parse execute anything until P7; search
highlighting P9.

## P7 — Ex commands

Try:
1. `ctest -R ex` — parser table + command tests.
2. `:w` → FILE SAVED (statusline `[+]` disappears); `:e!` reloads from disk.
3. `:w other.cpp` → save-as retargets the buffer filename.
4. `:ls` → buffer list printed in the message area; `:2b` switches buffers.
5. `:3` / `:$` → goto line/EOF.
6. `:set relativenumber` → gutter flips LIVE (config FILE is still P11).
7. `:s/old/new/g` → substitute with counts in the message row.
8. `:messages` → history in a scratch buffer (dropdown for `:!out` in P14.5).
9. `:q` on modified text → confirm prompt; `:q!` bypasses.

Not working yet: windows/splits (P8), search UI (P9), picker/explorer (P10).

---

## P8 — Windows + tabs

```
┌ main.cpp [+] ──── 12,7 41% ──┐┌ buffer.hpp ───── 1,1 Top ────┐
│  1 #include <cstdio>         ││  1 #pragma once              │
│  2                           ││  2                           │
│  3 int main() {              ││  3 struct Buffer {           │
│ ~                            ││ ~                            │
└──────────────────────────────┘└──────────────────────────────┘
   main.cpp [+] [2 tabs]                          5,1   Top
:▁
```

Run: `cmake --build build && ./build/noni main.cpp`, then keys:

1. `:sp` → two horizontal panes; cursor jumps into the lower pane.
2. `:vs buffer.hpp` → vertical split right; statuslines show both files.
3. `ctrl+w j` / `ctrl+w k` → focus moves down/up.
4. `ctrl+w =` → equal sizes; `ctrl+w 10+` → the current pane grows.
5. `ctrl+w c` → close pane (the others grow); `ctrl+w o` → only current pane.
6. `:tabnew` → new empty tab; tab bar row shows `main.cpp [+] | [No Name]`;
   `gt` / `gT` switch; `2gt` jumps two tabs ahead.
7. `:bd` when both panes show the same file → both panes collapse correctly.
8. `:q` on the last pane of the last tab, modified file → save-confirm prompt.

Not working yet: search UI, explorer, syntax, config file, `:!`, clipboard,
sessions.

## P9 — Search + substitute

1. `/pri` `<Enter>` → cursor lands on the first match; all matches inverted;
   `n` goes to the next.
2. `*` standing on `printf` → searches the whole word; `#` goes backward.
3. `:noh` → highlights cleared until the next search.
4. `:s/int/char/` → first occurrence on the cursor line replaced; message
   "1 substitution on 1 line".
5. `:%s/printf/write/g` → all occurrences; message "N substitutions on M lines".
6. Visual-select lines then `:s/foo/bar/g` → range `'<,'>` pre-filled.
7. `:s/Qmark/Q/gc` → `y/n/a/q/l` confirmation prompt per match.
8. Miss `/qqq` → "Pattern not found: qqq" message; cursor unchanged.

Not working yet: explorer, syntax, config file, `:!`, clipboard, sessions.
Project-wide grep arrives with P10 (`<space>fg`).

## P10 — Picker + explorer + grep

```
┌ explorer ────────────────────┐┌ main.cpp [+] ── 12,7 41% ───┐
│ src/                         ││  1 …                         │
│   core/                      ││  2 …                         │
│   app/                       ││  3 …                         │
│ CMakeLists.txt               ││ ~                            │
│ ROADMAP.md                   ││                              │
└──────────────────────────────┘└──────────────────────────────┘
         ┌───────────────────────────────┐
         │ main.cpp                      │
         │ new/CMakeLists.txt            │
         │ ROADMAP.md                    │
         │ match 1/3   (j/k/Enter/Esc)   │
         └───────────────────────────────┘
```

1. `<space>ff` → popup; type `redmap` → ROADMAP.md ranks first; `<Enter>` opens.
2. `<space>ff` + `Esc` → popup closes, buffer unchanged.
3. `<space>e` → explorer pane: `j/k` move, `l` expand dir, `h` collapse, `-` up.
4. `<Enter>` on `src/core/buffer.cpp` → opens; explorer stays open.
5. `o` in explorer → opens the selected file in a split.
6. `<space>fg printf` → grep popup (`file:line:col preview`); Enter opens at
   the match, match highlighted; Esc closes.
7. `:e new/ROAD<Tab>` → cmdline completion popup; `<Enter>` opens.
8. `R` in the explorer → rescan; cursor position kept.

Not working yet: syntax colors, config file, `:!`, clipboard, sessions.

## P11 — Config

`~/.config/noni/config.json` example:
```json
{ "tabwidth": 4, "expandtab": true, "number": true, "relativenumber": true,
  "scrolloff": 5,
  "lsp": { "servers": [ { "language": "cpp", "command": ["clangd"],
                          "rootMarkers": ["compile_commands.json"] } ] } }
```

1. Set `"number": false` → run → gutter shows plain `~`-lanes? numbers hidden.
2. `:set relativenumber!` → toggles instantly (config file NOT rewritten — v1
   config is read-only at startup; `:set` changes are runtime-only).
3. `:set ts=2` → tab rendering narrows immediately.
4. Broken JSON → message "config: parse error …" + defaults; editor still runs.
5. `:set unknownkey` → error message, no crash.

Not working yet: syntax colors, `:!`, clipboard, sessions, terminal, LSP; the
`lsp` config section is parsed but unused until P13.

## P12 — Syntax (tree-sitter)

```
│  1 #include <cstdio>        ← include line dim purple
│  3 int main() {             ← int = keyword; main = function
│   4_     printf("hi\n");    ← string green; call yellow
│  5     return 0;            ← number orange
```

1. `./build/noni src/core/buffer.cpp` → C++ colors visible.
2. `./build/noni tests/x.py` → python colors.
3. `./build/noni README.md` → fallback-lexer colors.
4. Type `/*` in a `.cpp` → comment color appears within a frame (main-thread
   incremental parse, visible range only).
5. Build a 10k-line fixture file, scroll → stays smooth (no worker stalls).
6. Open an unknown `.weird` file → plain text, no crash.

Not working yet: `:!`, clipboard, terminal, LSP (P13), sessions.

## P13 — LSP

Requires a `lsp.servers[]` entry in config.json (P11) and the server binary
(e.g. clangd). The editor stays fully usable when none is configured.

```
│  2 #include <unistd.h>                   ← problem underline
│ 14   wtite(fd, buf, n);   ⌐W             ← gutter diagnostic sign
│ -- INSERT --    main.cpp [+]    14,8 22%   ✖1 ⚠2  cpp ● clangd
┌ completion popup ────────────┐
│ write     (function)         │
│ writev …                     │
└──────────────────────────────┘
```

1. Open a `.cpp` with the config server → statusline shows `clangd ●` when ready.
2. Type `wr` → popup lists `write/writev`; `<c-n>`/`<cr>` inserts (textEdit-safe).
3. `K` on a function → hover popup; move → dismissed.
4. `gd` → jump to definition; `ctrl+o` back; `<space>gr` → references picker.
5. `<space>rn` → cmdline asks the new name; rename applied project-wide; `u`
   undoes it in one step.
6. Break a call (`wtite(…`) → gutter `W`; `[d`/`]d` jump between diagnostics;
   `:Diag` list.
7. `:Format` → LSP formatting applied async, one undo group.

Not working yet: terminal (next), extras (P15), sessions (P16).

## P14 — Terminal + `:!` + OSC52

```
┌ main.cpp [+] ─────────────────────────────── 15,9 47% ──────┐
│  3m int main() {                                            │
│ 14m   write(fd, buf, n);                                    │
│ 15m+  return 1;                                             │
├─ $SHELL ─────────────────────────────────────────── running ┤
│ $ ls                                                        │
│ buffer.cpp  buffer.hpp  main.cpp                            │
│ $ ▁          ← :term pane; i focuses, Esc leaves            │
└─────────────────────────────────────────────────────────────┘
```

1. `:term` → bottom pane running your `$SHELL`, labeled in its statusline;
   `:term` height from config (`terminal.height`, default 40%).
2. `i` → focus the terminal (keys go to the child); type `ls`, `make` etc.
3. `Esc` → focus returns to the editor; `ctrl+w j/k` still moves between panes.
4. Copy mode: `Esc` then `k/j/ctrl+b/ctrl+f` over scrollback history; `y` on
   the selection → system clipboard via OSC52.
5. `:!ls` → scratch buffer `:!out` opens (unmodifiable, in `:ls`);
   "shell done (exit 0)" message line.
6. `:!cc %` in a `.cpp` → `%` expands to the file path before running.
7. `:!sleep 60` → killed at the 30 s timeout, `:!sleep` buffer marked killed;
   the editor NEVER froze (kept typing in a split while waiting).
8. In an OSC52 terminal (kitty/wezterm/iTerm): `"+yy` → clipboard gains the
   line without any subprocess; `"+p` pastes it back in-session; a huge yank
   (over cap) → "OSC52 skipped (too large)" message, register still holds text.

Not working yet: extras (:map, block visual), sessions. No git anywhere; no
wl-copy/xclip/xsel subprocesses anywhere.

## P15 — Extras

1. `ctrl+v` → block visual: `jjllx` deletes a column; `A;` `<Esc>` appends `;`
   per line; `I//` `<Esc>` comments a block.
2. `:map ,w :w<cr>` → `,w` saves; `:map` lists it; `:nunmap ,w` removes (later
   `,w` does nothing).
3. `3J` → three lines join with single spaces; `gJ` → no space inserted.
4. Visual-select `V` + `U` → uppercase; `u` lowercase; `~` toggles; `u`-undo
   restores in one step.
5. `z h` / `4z l` → horizontal side scroll moves; `zz`/`zt`/`zb` recenter.

## P16 — Session / recovery / autosave (final)

1. Edit a file and wait ≥ autosave delay → disk updated without `:w`
   (statusline `+` disappears, msg line notes autosave).
2. Edit, then `kill -9` the process → restart `./build/noni` → prompt:
   `Recovery draft for main.cpp — restore? [y/N]` → `y` → content back.
3. Set up 2 tabs + a split, `:qa` → session saved; `./build/noni --restore`
   → windows/tabs/cursors restored exactly.
4. Corrupt a save mid-write (fixture) → old file content intact (atomic
   temp+fsync+rename).
5. `<space>fr` → recent-files popup; order = last opened first.

---

# FINAL v1 MOCKUP (end of P16; NO git anywhere)

```
┌─ main.cpp [+] [2 tabs] ──────────────────────────────────────┐
│ src/      │  1m #include <unistd.h>       │ buffer.hpp       │
│   core/   │  3m │            /* TODO */   │  1m #pragma once │
│   app/    │ 14m   wtite(fd, buf, n);  ⌐W  │  2m              │
│ CMake…    │ 15m+  return 1;               │  3m class Buffer{│
│ ROAD…     ├─ $SHELL ─────────────────────────────────────────┤
│ ROAD…     │ $ ls                                           │
│           │ buffer.cpp  buffer.hpp  main.cpp               │
├───────────┴──────────────────────────────────────────────────┤
│ main.cpp [+] ✖1 ⚠2  cpp ● clangd    15,9 47%    buffer.hpp   │
├──────────────────────────────────────────────────────────────┤
│ :▁                                                           │
└──────────────────────────────────────────────────────────────┘
Fields: [tabbar] [explorer pane] [text areas + LSP diagnostic signs]
        [terminal pane (:term)] [per-pane statuslines] [cmdline + message row]
```

Final acceptance script:
1. `<space>ff con` → config.json (fixture) opens; syntax colors on.
2. `<space>fg GrepDone` → hits list; Enter opens at first hit.
3. `<space>e` + navigate tree; `o` split-opens a file.
4. `:sp buffer.hpp` + `:tabnew` → layout as mockup; `gt` cycles.
5. `/write` highlights; `:%s#/new/renamed/#new/gc` confirm flow; `u` once
   reverts everything.
6. `gd` → definition jump; break a call (`wtite(`) → gutter `W` sign +
   `:Diag` list as in the mockup; `[d`/`]d` cycle.
7. `:term` → bottom pane; `i`, run `ls`, `Esc` back; copy-mode `y` copies to
   system clipboard.
8. `:!cc %` → `%` expands; `:!sleep 60` timeout-killed at 30 s, editor live.
9. `"+yy` (OSC52 terminal) → paste into terminal with ctrl+shift+v → line there.
10. Edit + wait → autosave; `kill -9` → restart → recovery prompt restores.
11. `:qa` → `./build/noni --restore` → everything back.

**Done.** If any listed key fails its expected result in the demo, the phase is
not accepted — bugs go back into the matching ROADMAP task before moving on.
