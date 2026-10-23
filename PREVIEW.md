# PREVIEW.md — what you will see at the end of every phase

Read your phase's section **before** saying GO for that phase. Pre-render phases
(P0–P6) have **no screen yet** — honesty first: your "demo" there is ctest output
and a scripted vim session, not pixels. From P7 onward you get a real editor and
each section shows its screen.

## Universal build commands

```
cmake -B build -DCMAKE_BUILD_TYPE=Debug     # once; then only on CMake changes
cmake --build build                         # after each task
ctest --test-dir build --output-on-failure  # unit tests
ASAN_OPTIONS=detect_leaks=1 ./build/noni    # sanitizer run (P7 onward)
./build/noni [file]                         # app binary (P7 onward)
```

---

## P0 — CMake skeleton

**You will see** (no screen, no GUI):
```
$ cmake -B build -DCMAKE_BUILD_TYPE=Debug
-- The CXX compiler identification is GNU 16.2.1
-- Fetching doctest …
-- Configuring done      $ cmake --build build
   … linking /home/…/new/build/noni
$ ctest --test-dir build
1/1 Test #1: smoke …***Passed
$ ./build/noni --version
noni v2 (CMake skeleton) — UI arrives in P7
```

Try:
1. `./build/noni --version` → prints version line, exit 0.
2. `./build/noni` (no args) → error "UI arrives in P7", exit 2.
3. Deliberate break check: temporarily add `#include <ncurses.h>` into a
   `src/core/` header → build FAILS (layer check works) → revert.
4. `ctest --test-dir build` → smoke test Passed.

Not working yet: everything visible; no editor at all.

## P1 — Core buffer + undo

**You will see**: ctest listing dozens of core tests:
```
$ ctest --test-dir build -R core --output-on-failure
  text: cp_to_byte … PASSED
  buffer: load missing → error … PASSED
  undo: dd,u → cursor restore … PASSED
100% tests passed
```

Try:
1. `ctest -R buffer` → buffer suite green.
2. `ctest -R undo` → undo/redo suites green (interleaving, cap 200, redo-branch cut).
3. Look at `new/tests/buffer_test.cpp` — every ACCEPT bullet in ROADMAP P1 is a
   named test there.

Not working yet: no keys, no screen, motions/operators unimplemented.

## P2 — Motions

**You will see**: `ctest -R motion` with ≈120 named cases.

Try:
1. `ctest -R motion` — green; tests literally named like
   `motions.2fX.cluster`, `motions.w.end.of.buffer`.
2. Grep word tests: `grep -n "3w" new/tests/motion_test.cpp` → count-based cases.
3. `ctest -R view` (P2.7) — page/scroll + scrolloff math.

Not working yet: nothing applies motions to a screen; `d` etc. not yet wired.

## P3 — Operators + text objects / P4 — Registers, macros

Try:
1. `ctest -R operator` — op×motion×count matrix tests pass.
2. `ctest -R textobj` — `iw aw ip ap i" a( i{` objects.
3. `ctest -R register` — numbered/named/blackhole register semantics.
4. `ctest -R macro` — record/replay, recursion guard.
5. `ctest -R dot` — `.` repeat matrix.

Not working yet: still invisible (pure library). P5 makes it alive.

## P5 — Modes + reducer (headless echo of vim)

**You will see**: a scripted-session runner:
```
$ ctest --test-dir build -R session --output-on-failure
[doctest] sessions/insert_test: typed "hello", ABk undo → "hello" restored ✓
[doctest] sessions/visual_test: V then > indented 3 lines ✓
```

Try:
1. `ctest -R reducer` — single-file reducer tests.
2. `ctest -R keys` — the PREVIEW key scripts (below) replayed as state assertions.
3. Inspect `new/tests/sessions/` — each script = exact key strings like
   `"iHello world<Esc>dd.u"`.

Not working yet: no screen; ex commands partially stubbed (P6).

## P6 — Ex commands

Try:
1. `ctest -R ex` — parser table + command tests.
2. `ctest -R ex_s` — `:s///` substitutions incl. `c` flag chooser.
3. `ctest -R session` — scripted session now includes `:w` into a tmpdir
   (`$TMP` fixture), `:q` flows, unsaved-confirm path.

Not working yet: no ncurses. After P6 the whole *brain* exists; P7 gives it eyes.

---

## P7 — Renderer — FIRST SCREEN

```
┌──────────────────────────────────────────────────────────────┐
│ main.cpp [+                                     12,7   41%] │ ← statusline
│  1 #include <cstdio>                                         │
│  2                                                           │
│  3 int main() {                                              │
│   4_   printf("hi\n");                                       │ ← cursor
│  5     return 0;                                             │
│        ~                                                     │
│        ~                                                     │
│ 23 lines, 128 chars                                          │
│ :                                                            │ ← cmdline row
└──────────────────────────────────────────────────────────────┘
```

Build/run: `cmake --build build && ./build/noni main.cpp`

Try (from PREVIEW §P7 script):
1. `./build/noni main.cpp` → file shows, cursor line 1 col 1.
2. `5G` → cursor to line 5, gutter highlights it.
3. `j` `k` `0` `$` `w` `b` → move; `5j` drops five.
4. `i` → statusline `-- INSERT --`; type `hello` → chars appear; `<Esc>`.
5. `u` → hello gone; `ctrl+r` → back.
6. `:q` → quit; `:q!` after editing saves-nothing exits without prompt.
7. `ctrl+d` / `ctrl+u` / `ctrl+f` / `ctrl+b` → page scrolls; `zz` centers.
8. `:set relativenumber` — NOT working yet (P11 flush; P7 gutters use
   hardcoded absolute; P7 has `number` on only).

Not working yet: windows/splits/tabs, search UI, explorer, syntax colors (plain
text), config file, LSP, git, terminal, sessions. (Motions/operators/modes from
P2–P6 ARE fully wired here.)

## P8 — Windows + tabs

```
┌ main.cpp [+ ────────────────┐┌ buffer.hpp ─────────────────┐
│  1 #include …               ││  1 #pragma once             │
│  2                          ││  2                          │
│ ~                           ││  3 struct Buffer {          │
└ main.cpp … 12,7 41% ────────┘└ buffer.hpp 1,1 Top ──────── ┘
┌── main.cpp [+][2 tabs]────── main.cpp buffer ────────────────┐
│ :                                                            │
└──────────────────────────────────────────────────────────────┘
```

Run: `cmake --build build && ./build/noni main.cpp`, then keys:

1. `:sp` → two horizontal panes; cursor jumps to the lower pane.
2. `:vs buffer.hpp` (P10 picker not needed; type path) → vertical split.
3. `ctrl+w j` then `ctrl+w k` → focus moves.
4. `ctrl+w =` → equal sizes; `ctrl+w 10+` → top pane taller.
5. `ctrl+w c` → close pane (others grow); `ctrl+w o` → only current.
6. `:tabnew` → empty tab; tab bar `main.cpp +| [No Name]`; `gt`/`gT` switch.
7. `:bd` in a 2-window showing same file → both windows handled (collapse).
8. `:q` closes pane; last pane+tab asks to save if `[+]`.

Not working yet: search UI, explorer, syntax, LSP, git, terminal, sessions.

## P9 — Search + substitute

Statusline prefix `-- Search --`-like prompts; matches inverted.

1. `/pri` `<Enter>` → cursor on first match; matches highlighted; `n` next.
2. `*` on `printf` → search whole word; `#` backward.
3. `:noh` → highlights cleared until next search.
4. `:s/int/char/` → first occurrence on current line replaced, message
   "1 substitution on 1 line".
5. `:%s/#include/#require/g` → all lines, message "N substitutions on M lines".
6. Visual-select lines + `:s/foo/bar/g` → range auto `'<,'>` pre-filled.
7. `/qmark` `:s/Qmark/Q/gc` → y/n/a confirmation prompt on each hit.
8. No match → beep + message "Pattern not found: qmark" (msg row).

Not working yet: explorer, syntax, LSP, git, terminal, sessions; project-wide
grep arrives with P10 (`<space>fg`) — P9 search covers the active buffer only.

## P10 — Picker + explorer

```
┌ nanod .noni/new root ────────┐┌ main.cpp …
│ src/                         ││  1 …
│   core/                      ││
│   app/                       ││
│ CMakeLists.txt               ││
│ README.md                    ││
└ explorer ────────────────────┘└ …
         ┌──────────────────────────┐
         │ main.cpp                 │
         │ new/CMakeLists.txt       │
         │ ROADMAP.md               │
         │ 3/128                    │
         └──────────────────────────┘ (ctrl+j/k nav, <Enter> open)
```

1. `<space>ff` → popup; type `redmd` → ROADMAP.md on top (fuzzy); Enter → opens.
2. `<space>ff` `road` `<Enter>` → opens ROADMAP.md in split if current buffer is
   unloaded? opens in active window (split option documented in help text later).
3. `<space>e` → left explorer column; `j/k` move, `h` collapse dir, `l` expand.
4. `<space>fg printf` → grep popup over the project; Enter on a hit opens that
   file at the matching line, match highlighted.
5. `Enter` on `src/core/buffer.cpp` in explorer → opens; explorer stays.
6. `-` in explorer → up one dir; `R` → rescan.
7. `:e new/ROAD<Tab>` → cmdline completion popup; Enter opens.
8. `:e /etc/hosts` → opens absolute file; `:ls` lists both.

Not working yet: syntax colors, LSP, git, terminal, sessions; explorer
CRUD (add/rename/delete) NOT in v1 (files created via OS/`:e` only).

## P11 — Config

`~/.config/noni/config.json` example:
```json
{ "tabwidth": 4, "expandtab": true, "number": true, "relativenumber": true,
  "scrolloff": 5,
  "lsp": { "servers": [ { "language": "cpp", "command": ["clangd"],
                           "rootMarkers": ["compile_commands.json", ".git"] } ] },
  "terminal": { "shell": "/bin/bash", "height": 12 } }
```

1. Put `false` for `"number"`, run → gadget shows plain (no numbers).
2. `:set relativenumber!` → relative toggles instantly, also saved? NOT
   persisted (v1 config is read-only at start + runtime `:set`).
3. `:set ts=2` → tab rendering narrows immediately.
4. Malformed JSON → message "config: parse error …" + defaults; editor still runs.
5. `:set unknownkey` → error message, no crash.

Not working yet: syntax, LSP actual use of "lsp" section (P13 wires it), terminal
(P15), clipboard provider (P15).

## P12 — Syntax (tree-sitter)

```
│  1 #include <cstdio>        ← include = dim purple
│  3 int main() {             ← int = keyword blue; main = function yellow
│   4_  printf("hi\n");       ← string = green; call = yellow
│  5   return 0;              ← number = orange
```

1. `./build/noni src/core/buffer.cpp` → C++ colors visible.
2. `./build/noni tests/x.py` → python colors.
3. `./build/noni README.md` → headings/code spans colored (fallback lexer).
4. Type `/*` in a `.cpp` → comment color appears ~immediately (async parse,
   no lag on small file).
5. Big-file scroll (e.g. `openFeatures-old` generate a 10k-line file) →
   scroll stays smooth (parse off-thread, visible-range only).
6. Unknown extension `.weird` → plain text, no crash.

Not working yet: LSP, git, terminal, sessions; Go/Java grammars (plain look
via fallback).

## P13 — LSP

```
│  2 #include <unistd.h>                   ← problem underline
│ 14   wtite(fd, buf, n);   /*wtite*/  ⌐W → gutter sign W
│ …
│ -- INSERT --          main.cpp[+]        14,8   22%    ✖1 ⚠2
┌ completion popup ────────┐
│ write     (function)     │  ← doc line under
│ writev …                 │
└──────────────────────────┘
```

Backend: clangd (system) if present; PREVIEW gives a fake python mock server
script for stable demos.

1. Open a `.cpp` in a folder with `compile_commands.json` or `#include` root →
   statusline shows `clangd ●` when ready (`:LspStatus` line in msg row).
2. Type `wr` → after idle popup lists `write/writev`; `<c-n>`, `<cr>` inserts.
3. `K` on a function → hover popup with signature.
4. `gd` → jumps to definition (file may switch in same window);
   `ctrl+o` comes back.
5. `<space>gr` → references picker; Enter opens the chosen file/line.
6. `<space>rn` → cmdline asks new name; Enter renames across project; `u`
   multi-file undo works.
7. Insert typo (`wtite(…)`) → gutter `W` sign + underline appears;
   `[d` jumps between diags; `:Diag` list.
8. `:Format` → clang-format via LSP applied asynchronously; undo in one group.

Not working yet: git gutter, terminal, sessions; auto-format-on-save (P17
default OFF; command exists).

## P14 — Git

```
│  1 #include <cstdio>                                        │
│  2+int main() {        ← gutter '+' new line                │
│  3~  return 1;         ← gutter '~' modified                │
│ main.cpp [+][main]                     3,1   Top    ⁺3 ~1   │
│ :Git status …
┌────────── :Gst pane ────────────────────────────────────────┐
│ Changes not staged for commit:                              │
│   ~ modified:   main.cpp                                    │
│ Untracked files:                                            │
│   ? build/…                                                 │
└─────────────────────────────────────────────────────────────┘
```

1. `./build/noni` inside this repo → branch `master` in statusline.
2. Edit a tracked file, trigger rewrite → gutter `+` on added lines.
3. `<space>gs` (map from `:Gst`) → status pane; `j/k` move; `Enter` opens file.
4. `space` (in pane) on file toggles staged status (`git add`/`git reset`).
5. `-` on a file → discard with confirm prompt (`y`/`n`).
6. `:Git add new/ROADMAP.md` directly works too.
7. Undo restores edit; gutter sign disappears (debounced worker).

Not working yet: diff preview popup (hunk preview = out of v1 scope), terminal,
sessions.

## P15 — Terminal + :!

```
│  1 int main() {                     (editor pane)           │
├───────── terminal (bash) ─────────────────────────────────── ┤
│ /home/ratx86/Projects/cpp/noni $ make -C build               │
│ …                                            (↕ scrollback)  │
│ /home/ratx86/Projects/cpp/noni $ █                           │
└ main.cpp [+][2 tabs] ───────────── 12,7 41%  terminal:bash ──┘
```

1. `:term` → bottom split with running shell; statusline shows `term`.
2. Type `ls` in it (via `i` to focus) → normal shell behavior.
3. `Esc` then move with `ctrl+w k` back into editor; coding works.
4. `<Enter>` focused? nonfocused → in terminal `i` switches to term input.
5. Scrollback: in copy mode (`Esc` from term focus? per help line) `ctrl+b/f`
   then `Enter` to paste-free? v1: clipboard copy via `y` in copy mode (OSC52 →
   system terminal clipboard).
6. `:!ls` → output in a new scratch buffer `:!out`; `<Esc>` closes? leaves —
   `:bd` to close (consistent, not magic).
7. `:!cc %` in a `.cpp` → compiles, `%` expands.

Not working yet: sessions/recovery/autosave.

## P16 — Extras

1. `ctrl+v` → block visual: `jjllx` deletes a column; `A;` appends `;` per line
   then `<Esc>`; `I//` comments a block of lines.
2. `:map ,w :w<cr>` → comma-w saves; `:map` lists; `:nunmap ,w` removes.
3. `gJ` joins without space; `3J` joins three lines.
4. Visual `u`/`U`/`~`.
5. `z h`/`z l` horizontal scroll adjust; `z=`? no (spell is out of scope).

## P17 — Session / recovery / autosave (final)

1. Edit a file, wait ≥ delay → disk updated without `:w` (statusline flash).
2. Kill -9 the process mid-edit; restart `./build/noni` → prompt:
   `Recovery draft for main.cpp — restore? [y/N]` → `y` → content back.
3. `:qa` → session saved; `./build/noni --restore` → windows/tabs/cursors
   restored.
4. Crash during write (simulate) → old file content intact (atomic write).
5. `<space>fr` → recent files popup.

---

# FINAL FULL EDITOR MOCKUP (after P17; P12–P15 visuals shown together)

```
┌─ main.cpp [+][3 tabs] ───────────────────────────────────────┐
│ src/      │  2m #include <unistd.h>       │ buffer.hpp        │
│   core/   │  3m │             /* TODO */  │  1m #pragma once  │
│   app/    │ 14m   wtite(fd, buf, n);   ⌐W │  2m               │
│ CMake…    │ 15m+  return 1;            ⁺ │  3m class Buffer {│
│ ROAD…     │ 16m}                       ⁺                   │
│           │ 17m  (—or plain "~")                           │
│           │                                                │
├───────────┴──────────────────────────────────────────────────┤
│ /home/ratx86/… $ cmake --build build        (↕ scrollback)    │
│ … $ █                                                         │
├───────────────────────────────────────────────────────────────┤
│ main.cpp  [+][cpp ●clangd][✖1 ⚠2][main ⁺3 ~1] 15,9 47%        │
└───────────────────────────────────────────────────────────────┘
Fields: [tabbar] [explorer] [gutter: nums+git signs+diag signs]
        [pane+statusline each] [cmdline+messages row above status]
```

Key trials (final acceptance script):
1. Open project root → explorer left, buffers pane right, brace highlights.
2. `<space>ff mancpp` → main.cpp; LSP attaches; gutter git signs appear.
3. Make a typo again → W sign; fix via completion popup.
4. `:sp buffer.hpp`, `:vs new/ROADMAP.md` → 3 panes + tab ok.
5. `:term` → bottom; `ctest` run inside; `y` a failing test name; paste in
   editor with `"+p` (OSC52).
6. `<space>ga` stage a file; `:Gst` toggle; discard with confirm.
7. `:%s/new/renamed/gc` → confirm flow; undo once reverts all.
8. `:qa` clean exit; `./build/noni --restore` restores layout; `kill -9` +
   restart → recovery prompt works.

**Done.** If any listed key fails its expected result in the demo phase, phase is
not accepted — bugs go back into the matching ROADMAP task before moving on.
