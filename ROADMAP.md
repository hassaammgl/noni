# ROADMAP.md — Noni v2 (new/)

Every task: **ID · title · goal · depends · files · public API (C++23) ·
acceptance test · edge cases · done**. One task ≈ one behavior, ≤ ~100 lines.
"Trace" = manual keystroke check from PREVIEW.md. Phase order is fixed; each phase
ends with a `check` task. Cross-phase ex/window commands are placed in the phase
that owns their machinery (noted inline).

**Scope (revised 2026-10-23, v2):** ONLY git integration is removed — no `git/`
module, no git worker, no `GitStatus/GitDiff` events, no gutter git signs, no
branch/dirty statusline info, no `<space>g*` keys, no `:Git`/`:Gst`, and no
`git check-ignore` (file scan uses a built-in skip list: `.git/`, `build/`,
`node_modules/` + basic `.gitignore` line parsing; the git binary is never invoked).
Everything else stays as planned: built-in terminal (PTY + VT parser + `:term` +
key input + copy mode), `:!cmd` shell jobs, OSC52 clipboard write (worker form,
no subprocesses), explorer, fuzzy picker, grep, syntax, config, extras,
session/recovery/autosave, and LSP at **P13** (isolated: nothing outside P13
depends on it). Syntax highlighting parses incrementally ON THE MAIN THREAD
(visible range only); a syntax worker is optional later. Background work =
file scan + grep (job pool) + terminal PTY worker + `:!` jobs + OSC52 writes.

**Revised 2026-10-25, UI-first:** no long headless stretch. **P2 puts the FIRST
SCREEN up** (minimal editor: type, move, `x`, undo) right after P1; every phase
after P2 lands its features LIVE on the visible editor. Core logic stays
pure and layered (all unit-tested without ncurses); `ui/` only renders.

Conventions for all phases: build = `cmake --build build`; test =
`ctest --test-dir build --output-on-failure`; sanitize =
`ASAN_OPTIONS=detect_leaks=1 ./build/noni` (every Debug build is sanitized).
Differential reference for editing semantics: `nvim --headless -u NONE`
(read-only oracle; see AGENTS.md — Neovim is right when behavior differs).

## Summary

| Phase | Title                      | Tasks | User capability after phase                                   |
| ----- | -------------------------- | ----- | ------------------------------------------------------------- |
| P0    | CMake skeleton             | 3     | empty ui-less binary builds, ctest runs a placeholder         |
| P1    | Core buffer + undo         | 8     | (library only) tested text edits + undo/redo                  |
| P2    | **FIRST SCREEN** minimal editor | 9 | visible editor: type, h/j/k/l/0/$, x, u, resize               |
| P3    | Motions (live)             | 9     | full motion vocabulary usable in the editor                   |
| P4    | Operators + text objects   | 9     | d/c/y/</>/gu/gU/g~/= over motions+objects, live               |
| P5    | Registers/marks/dot/macros | 9     | p/P, `.`, q/@, marks, jumplist — live                         |
| P6    | Modes completion + prompts | 5     | visual/visual-line, cmdline prompt history, full insert rules |
| P7    | Ex commands                | 9     | `:` live: :w :q :e :b :ls :s/// :set :messages                |
| P8    | Windows + tabs             | 8     | :sp :vs ctrl+w, :tabnew gt/gT                                 |
| P9    | Search + substitute        | 7     | / ? n N * # :noh :s/// :%s///g + highlights                   |
| P10   | Picker + explorer + grep   | 8     | <space>ff fuzzy files, <space>fg grep, <space>e explorer      |
| P11   | Config                     | 4     | config.json honored, :set works end-to-end                    |
| P12   | Syntax (tree-sitter)       | 6     | c/cpp/python/rust highlighting, incremental main-thread parse |
| P13   | LSP                        | 11    | completion/hover/def/refs/rename/diag/format, async           |
| P14   | Terminal + `:!` + OSC52    | 7     | :term bottom split, :!cmd async, "+ OSC52 yank                |
| P15   | Extras                     | 6     | ctrl+v block visual, :map/:nnoremap, J/gJ, visual u/U         |
| P16   | Session/recovery/autosave  | 7     | restore across runs, crash recovery prompts                   |

v1 total = **125 tasks** (P0–P16).

---

## P0 — CMake skeleton

### P0.1 Top-level CMakeLists
- goal: project(), C++23, warnings `-Wall -Wextra -Wpedantic`, Debug
  `-fsanitize=address,undefined`, option `NONI_SANITIZERS` (ON in Debug).
- files: `new/CMakeLists.txt`
- api: targets (created over P0.2): `noni_core noni_ext noni_terminal
  noni_syntax noni_lsp noni_app noni_ui noni` + `noni_tests`.
- ACCEPT: `cmake -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build` succeeds
  (no sources yet beyond main stub).
- edges: CMake 4.x requires `cmake_minimum_required(VERSION 3.28…4.0)` compat.
- done: configure+build clean, zero warnings.

### P0.2 Targets + layer include enforcement
- goal: static libs per ARCHITECTURE §1; `target_link_libraries` order
  core ← services ← app ← ui ← noni; include scoping so a wrong include fails
  (ncursesw/tree-sitter attached only to their owning targets).
- depends: P0.1
- files: `new/CMakeLists.txt`, stub headers per target, `tests/layer_lint.sh`
- ACCEPT: configure+build with all layer stubs links clean; `ctest` runs the
  layer lint green; the `noni` exe links the stub chain end-to-end.
- edges: TOOLCHAIN-VERIFIED DEVIATION — this box also has `ncurses.h` on the
  default `/usr/include` path, so a wrong `#include` in `core/` still compiles;
  enforcement is therefore (a) link direction (ncursesw/tree-sitter targets in
  the owning chain only) + (b) the `layer_lint` ctest grep gate, both running
  at every merge gate.
- done: target graph exists; `noni` links empty main.

### P0.3 doctest + main stub + ctest smoke
- goal: FetchContent doctest; `tests/smoke_test.cpp` asserting 1==1; `main.cpp`
  builds, links app+ui, `noni --version` prints and exits 0.
- depends: P0.2
- files: `new/tests/smoke_test.cpp`, `new/src/main.cpp`
- api: `int main(int argc, char** argv)` skeleton in main.cpp (no ncurses yet:
  `noni --version` → stdout, exit 0; no args → info line, exit 0 [deviation:
  bare `make run` must not look like a failure until P2; reverts to launching
  the editor there]); unknown args → error, exit 2).
- ACCEPT: `ctest --test-dir build` runs smoke test; `./build/noni --version` prints.
- edges: FetchContent behind proxy → document manual fallback in commit body.
- done: ctest green; committed `P0.3`.

## P1 — Core buffer + undo (tests only)

### P1.1 Pos/ids/text primitives
- goal: `Pos`, `BufferId/WindowId`; UTF-8 decode/encode; `col↔byte` conversion in a
  line; codepoint width (tabs = `tabwidth`); word char classes (word/WORD/punct).
- files: `core/pos.hpp`, `core/text.hpp/.cpp`, `tests/text_test.cpp`
- api: `std::size_t cp_to_byte(std::string_view line, std::uint32_t col);`
  `std::uint32_t byte_to_cp(...); char32_t next_cp / prev_cp;`
  `CharClass classify(char32_t); bool is_blank;`
  `int cp_display_width(char32_t, int tabwidth, int disp_col);`
- ACCEPT: unit tests: ascii/2-byte/4-byte/invalid sequences; tab width at col 3,9.
- edges: NUL bytes in file (kept raw); over-long lines (no special handling).
- done: tests green.

### P1.2 Buffer storage
- goal: line vector + load/save + `LoadError` state; dirty id tracking.
- files: `core/buffer.hpp/.cpp`, `tests/buffer_test.cpp`
- api: `Buffer(std::vector<std::string> lines);` static
  `eresult<Buffer> load(path); eresult<void> save(path);`
  `bool dirty(); std::uint64_t revision(); std::filesystem::path path();`
- ACCEPT: load missing file → error (never empty doc); round-trip save/load.
- edges: no trailing newline; CRLF normalized to LF on load, LF only saved;
  binary-ish content left untouched.
- done: tests green.

### P1.3 Edit primitives
- goal: codepoint-indexed insert/delete/replace + newline ops, no undo yet.
- depends: P1.1, P1.2
- files: same pair + tests
- api (all take/return `Pos` semantics, clamped):
  `Pos insert_char(Pos, char32_t); std::string insert_text(Pos, sv);`
  `Pos erase_char(Pos); Pos erase_range(TextRange);`
  `Pos insert_newline(Pos); TextRange join(Pos, bool smart);`
- ACCEPT: tests incl. delete to line start, erase across lines, erase to EOF.
- edges: `Pos` past EOL (col == len allowed only as cursor); invalid line → assert.
- done: tests green.

### P1.4 Undo transactions
- goal: grouped edits as one transaction; undo/redo stacks (cap 200); cursor
  before/after inside txn; undo truncates redo on new edit.
- depends: P1.3
- files: `core/undo.hpp/.cpp` (+ buffer merge), `tests/undo_test.cpp`
- api: `class UndoStack { void begin_group(); void end_group(); void
  push(TextChange, Pos cur_before, Pos cur_after); UndoResult undo(); redo(); }`
- ACCEPT: interleaved undo/redo mutations; truncate-on-branch; cap evicts oldest.
- edges: edits before `begin_group` belong to the previous txn; empty txn dropped.
- done: tests green.

### P1.5 Buffer edit sessions
- goal: `with_edit(kind)` wrapper marking the txn boundary + auto-merging typing
  groups (same-revision char runs) — vim-like `u` granularity.
- depends: P1.4
- files: `core/buffer.hpp/.cpp`
- api: `template<class F> void with_edit(EditKind, F&&);` — `F` returns void and
  MUST NOT throw (no exceptions anywhere, see AGENTS.md); an early return or error
  state leaves the undo group consistent. Plus `UndoResult undo(); redo();`
- ACCEPT: `dw` + `u` restores both text and cursor (test).
- edges: nested with_edit (reuse inner group); error inside F → group still closed
  correctly (no exception escape, tested).
- done: tests green.

### P1.6 Buffer revision + snapshot
- goal: monotonic `revision()`; `snapshot_lines()` cheap view for jobs/tests.
- depends: P1.5
- files: `core/buffer.hpp/.cpp`
- api: `std::shared_ptr<const std::vector<std::string>> snapshot_lines() const;`
- ACCEPT: revision bumps on any with_edit; snapshot stable (no dangling after edit).
- edges: snapshot shared_ptr keeps old lines alive (memory note: revisit at
  P14.5 — `:!` scratch buffers hold snapshots of big outputs).
- done: tests green.

### P1.7 BufferSearch (pure)
- goal: literal + ECMAScript-regex find all/next, replace range → new lines (pure
  functions over a line vector; the editor applies them later).
- depends: P1.2
- files: `core/buffer_search.hpp/.cpp`, `tests/buffer_search_test.cpp`
- api: `struct Query { bool regex, case_sensitive, whole_word; };`
  `std::vector<Match> find_all(lines, sv pattern, Query);`
  `eresult<std::vector<std::string>> replace(lines, pattern, sv repl, Query, bool all);`
- ACCEPT: tests: escapes, per-line matches, case, whole-word, bad regex error.
- edges: empty pattern → error; `$`/`^` anchors per line.
- done: tests green. (Search UI keys are P9.)

### P1.8 PHASE CHECK — core green + sanitizer
- goal: entire core compiles warning-free; all P1 tests pass under ASan+UBSan run of
  the test binary; commit tag `P1 done`.
- ACCEPT: `ctest -R P1` 100%; sanitizer run of `noni_tests` clean.
- done: phase reported to user with PREVIEW §P1 demo (test output).

## P2 — FIRST SCREEN — minimal editor

The headless stretch ends here: after this phase the editor is REAL and VISIBLE.
Everything above `ui/` stays layered (core/ logic fully unit-tested without
ncurses); ui/ renders, app/ drives, the reducer stays the ONE edit path.
From here on every phase's features are demonstrated LIVE in the running editor.

Files: `core/editor_state.hpp/.cpp`, `app/reducer.hpp/.cpp`, `app/app.hpp/.cpp`,
`ui/screen.hpp/.cpp`, `ui/view.hpp/.cpp`, `ui/input.hpp/.cpp`,
`ui/statusline.hpp/.cpp`. depends: P1, P0.2.

### P2.1 Screen init/teardown + Key reader
- goal: ncursesw init (noecho/cbreak/escdelay 25/keypad), colors init, `Key`
  reader from get_wch (Esc vs CSI resolve; UTF-8; Alt as ESC-prefix).
- ACCEPT: temporary debug line shows key codes; removed after P2.4.
- edges: terminal resize (KEY_RESIZE → App::on_resize); Ctrl+J vs Enter;
  Unicode input; bracketed-paste sequences decoded as data (markers stripped —
  small edge fixture test).
- depends: P1, P0.2.

### P2.2 EditorState skeleton + reducer shell v0
- goal: `EditorState{buffers, windows, mode, msgs, cmdline,…}` + single
  `reducer(EditorState&, Command) -> void` — ONE edit path per ARCHITECTURE §4;
  in-memory key feeder for tests. v0 commands: Motion/InsertChar/EraseChar/
  EnterInsert/LeaveInsert/Undo/Redo/DeleteChar.
- files: `app/reducer.hpp/.cpp`, `core/editor_state.hpp/.cpp`, tests.
- ACCEPT: a 40-key scripted session ends in asserted state (text/mode/cursor).
- edges: all commands no-op safely when `BufferId` invalid.

### P2.3 View draw + gutter numbers
- goal: draw buffer lines (single window) with horizontal scroll (long-line
  clip), tabs expanded, nonprinting markers, current-line highlight + line
  numbers; `screen_row ↔ buffer_line` mapping API (pure part unit-tested).
- ACCEPT: scripted render test + manual visual check.
- depends: P2.1, P2.2.

### P2.4 Statusline + message row + layout fill
- goal: statusline (mode flag, filename, `[+]`, Ln,Col), bottom message row,
  cmdline prompt row reserved; single-window layout fill (1-row statusline,
  1-row message/cmdline, 1-row tab-bar placeholder colored at P8).
- ACCEPT: PREVIEW walkthrough; mode flag flips with i/Esc.

### P2.5 Main loop + followed cursor
- goal: `App::run` polls stdin (+ wake fd skeleton), drains the queue, runs the
  reducer, renders dirty state only; logical cursor via wmove; view follows the
  cursor minimally (scrolloff honored) until the full scroll set lands (P3.6).
- ACCEPT: typing session via the headless feeder + manual demo.
- depends: P2.3, P2.4.

### P2.6 Insert mode v0
- goal: insert chars incl. UTF-8 stream; Enter inserts a newline (copying the
  previous line's indent); Backspace (incl. at leading indent); Esc exits.
  Full rules (o/O, ctrl+w/u, smart options) are P6.1.
- ACCEPT: typing-flow feeder tests; bracketed paste inserted as data.
- depends: P2.2, P1.5 (typing runs merge into one undo group).

### P2.7 Normal mode v0 + first motions
- goal: `core/motion.hpp` framework (MotionContext, clamping at line ends,
  `apply(count)`) with h j k l 0 $ first; digit-count prefix handled by the v0
  dispatcher; `x` delete-char (plain unnamed register — full model is P5.1),
  `u`/`ctrl+r` undo/redo (P1 engine), `i`/`A` enter insert.
- ACCEPT: h at col0 stays; `5j` stops at last line; `$` on an empty line;
  `3j` via count prefix (feeder test; live on screen too).
- depends: P2.5, P1 (undo engine).

### P2.8 Viewport edge cases
- goal: empty buffer (`~` lanes), lines longer than the screen (horizontal
  scroll correct with tabs/wide chars), lines ending exactly at the right edge,
  half-visible last line viewport math.
- ACCEPT: unit tests over the mapping API; visual check.
- depends: P2.3.

### P2.9 PHASE CHECK — first visible demo
- goal: user demo per PREVIEW §P2; ASan+UBSan run of the REAL app.

## P3 — Motions (live on the screen)

All motion tasks extend `core/motion.hpp/.cpp` (framework from P2.7) +
`tests/motion_test.cpp`, api style:
`struct Motion { MotionId id; args...; Pos apply(const TextView, Pos, count); };`
`TextView` = lines + tabwidth + view scroll (for zz). Every task ships
**differential cases vs `nvim --headless -u NONE`** (AGENTS.md) and is
immediately usable in the running editor. depends: P2.

### P3.1 Word motions w W
- goal: vim `w`/`W` incl. end-of-line skip, empty-line boundary.
- ACCEPT: "foo.bar baz" cases (differential vs nvim); w on trailing blank stops.

### P3.2 Word motions b B e E
- goal: backward + end motions; e on last char of file.
- ACCEPT: tests from vim docs table (word/WORD/punct mixes), differential suite.

### P3.3 f F t T ; ,
- goal: find-char with count; `;`/`,` repeat with count; per-line persist; last
  find state stored in Window (`last_find`).
- ACCEPT: `2fx`, `3;` chains; backward `t`; no match → no move + error flag.
- edges: no match (no cursor change, error state for msg later).

### P3.4 ^ gg G + counts
- goal: `^` first-nonblank (distinct from `0`), goto line via count (`123G`),
  file start/end.
- ACCEPT: `$` on line with tabs; `3G` on 2-line file → last line (differential).

### P3.5 { } paragraph
- goal: blank-line paragraph moves; `{`/`}` on empty buffer no-op.
- ACCEPT: paragraph tests; no infinite loops (differential).

### P3.6 Page + scroll: ctrl+d/u/f/b, H M L, zz zt zb
- goal: view scrolls (updates TextView scroll), H/M/L pick rows in view;
  replaces P2.5's minimal cursor-following.
- files: same + `core/view.hpp/.cpp` (view scroll state, scrolloff calc).
- ACCEPT: ctrl+f at EOF clamps; zz keeps line centered.
- edges: view shorter than screen; scrolloff > half view.

### P3.7 % bracket match
- goal: match `(){}[]` forward/backward, plain scan (comments/strings NOT skipped —
  documented limitation), count% jumps.
- ACCEPT: nested + unmatched → no move (differential).

### P3.8 Cursor column memory
- goal: `preferred_col` (j/k through short/long lines restore column) — Window
  field + apply in the vertical motions.
- ACCEPT: long→short→long keeps column.

### P3.9 PHASE CHECK
- goal: motion test sweep (idempotency, count>line fallbacks), differential
  suite green, live demo per PREVIEW §P3.

## P4 — Operators + text objects

Files: `core/operator.hpp/.cpp`, `core/textobj.hpp/.cpp`, tests both. depends: P3.
Differential oracle: `nvim --headless -u NONE` for every op×motion×object case;
every operator is live in the editor the moment its task lands.

### P4.1 Operator parse state (Pending FSM) — wire into the v0 dispatcher
- goal: `Pending` FSM: `[count]op` → wait motion/textobject/doubled-op (linewise) →
  build `Edit{op, start, end, kind}`; register prefix `"x` recorded before/with.
  Extends P2.7's dispatcher — from now on `[count]Operator` composes live.
- files: `core/operator.hpp/.cpp`; tests.
- api: `std::optional<Edit> step(Pending&, Key, const TextView);`
  `std::optional<Edit> step_visual(Pending&, sel);`
- ACCEPT: `2d3w` = 6 words; `dd` = line; `cc` keeps indent (indent preserved via
  P4.6 shift machinery, tested here).
- edges: `d` then Esc cancels; same op repeated → linewise.

### P4.2 d y (delete/yank)
- goal: apply Edit: text→register (Reg kind char/line), cursor to range start
  (line ops → first nonblank of line).
- depends: P4.1 (unnamed register first full model is P5.1)
- ACCEPT: `dw` at EOL; `y$` cursor stays; deleted line yank = linewise (live).

### P4.3 c (change)
- goal: delete + enter Insert; mode transition delivered as
  `Edit.post_action = EnterInsert` (handled by the reducer here).
- ACCEPT: `cc`/`cw`; `cw` vim-accuracy: cursor on word start does NOT move past
  EOL (vim special case) — differential TEST.

### P4.4 Text objects iw aw ip ap
- goal: selection ranges; `aw` includes trailing space/newline rules.
- ACCEPT: "foo bar ," samples; `ap` merges trailing blank lines vim-style
  (differential).
- depends: P3.1-P3.2 word motions.

### P4.5 Text objects quotes + brackets
- goal: i" a" i' a' i( a( i{ a{ i[ a[ (stand on delimiter or inside), multiline ok.
- ACCEPT: `da(` across lines; unmatched → error (no edit, count consumed).

### P4.6 > < (shift)
- goal: indent lines by `shiftwidth` (tabs or spaces per `expandtab`), with
  counts and `.` repeat; cursor to first nonblank of first line.
- ACCEPT: here Normal `3>>`; visual-line `>N` arrives in P6.2.
- edges: empty lines unaffected; mixed tabs/spaces.

### P4.7 gu gU g~
- goal: case ops over ranges (`guu`, `gUU`, `g~~` doubled forms too), with motion.
- ACCEPT: accented UTF-8 via wide-char towupper/towlower; ASCII-only fallback
  documented if the locale is C (edge case test).
- edges: on nonletters no-op.

### P4.8 = (auto-indent range)
- goal: re-indent lines from context — bracket depth + previous-line indent,
  c-family heuristic (balanced braces) only; keep tab/space style of the buffer
  (`expandtab` decides).
- ACCEPT: `=i{` inside a function body normalizes indent (differential
  approximation; v1 = heuristic, exact nvim indentexpr NOT promised).
- edges: empty lines; mixed tabs/spaces.

### P4.9 PHASE CHECK
- goal: regression matrix op×motion×object×count (generated cases) + one nvim
  differential script per operator; live demo per PREVIEW §P4.

## P5 — Registers/marks/jumps/dot/macros

Files: `core/register.hpp/.cpp`, `core/marks.hpp/.cpp`, `core/macro.hpp/.cpp`,
tests. depends: P4.

### P5.1 Register model
- goal: unnamed `"`; numbered 0-9 (yank/delete history); named a-z (append via
  A-z); small delete `-`; blackhole `_`; `+`/`*` aliases writing through OSC52
  (P13.2 — model stores content; no subprocess clipboard ever). Upgrades the
  plain unnamed register P2.7 used for `x`.
- api: `RegSet::get(char) -> RegView; void set(char, RegView, bool append);`
- ACCEPT: `dd` rotates `"1`; `cw` fills `"-`; `"ayy` names tmp; `"Ayy` appends.
- edges: delete of 1 char (non-line) goes to `"-` only, NOT into `"1`.

### P5.2 paste p P
- goal: char/linewise from RegView (count multiplier = paste N times), cursor
  placement rules; visual-mode paste replaces selection (wired in P6.2).
- ACCEPT: `3p` linewise pastes 3 copies; p on empty buffer.

### P5.3 Marks a-z (`'x` vs `` `x ``)
- goal: set (`mx`), jump-line (`'x`) vs exact (`` `x ``); invalidated on line ops
  (line index shift).
- ACCEPT: mark survives insert above (moves down); dies if its line deleted.

### P5.4 Jump list + ctrl+o / ctrl+i
- goal: cap 50, dedupe consecutive entries, push only on big jumps (>2 lines, G,
  mark, search-hit).
- ACCEPT: ctrl+o/ctrl+i chain round-trips; each big move adds exactly one entry.

### P5.5 Dot repeat `.`
- goal: record last Edit (op+motion+count+register) + simple insert sequences;
  `.` replays at current cursor.
- depends: P4.
- ACCEPT: `dw.` on 3 words; `ciwfoo<Esc>.` changes second word to foo; insert
  text repeat (differential).
- edges: `.` after undo replays the edit that predates the undo attempt; visual
  edits replayable.

### P5.6 Macros q a…q / @a
- goal: record raw key sequence into a register (`q` toggle, aborted recording
  = empty), play `@a` count N, `@@` repeats last, recursion depth 1 max.
- ACCEPT: record+play edits + moves; error inside macro aborts (error state) —
  no dialogs.
- edges: `q` inside playback → ignored (depth limit), not recorded.

### P5.7 r ~ J basics
- goal: replace-char (count), toggle case over motion, `J` joins with single
  space (with count). (`x` delete-char already lives since P2.7.)
- ACCEPT: `3rx` past EOL clamps; `J` basic here; `J` polish + `gJ` in P15.3.
- depends: P1, P2.

### P5.8 R (Replace mode) as an edit engine
- goal: overwrite chars in place; Backspace restores original char; Esc closes
  the undo group.
- ACCEPT: replace past EOL appends; newline in Replace mode switches to
  insert-line semantics (differential).
- depends: P1.5 (transaction grouping). Binding ships in P6.2.

### P5.9 PHASE CHECK
- goal: macro + dot + undo interaction matrix test; differential script; live
  demo per PREVIEW §P5.

## P6 — Modes completion + prompt machinery

(`EditorState` + reducer shell arrived in P2.2; v0 Normal dispatcher P2.7;
operator FSM P4.1. This phase completes the mode set and the cmdline prompt.
Everything below is live on screen immediately.)

### P6.1 Insert mode completing
- goal: auto-indent on Enter with `++` rules, `o`/`O` open lines, smart
  Backspace (indent), ctrl+w (delete word back), ctrl+u (line); Esc exits
  (jk/jj only if configured later).
- ACCEPT: typing-flow tests; `ctrl+w` edge at col 0; `o` inherits indent.
- depends: P2.6.

### P6.2 Visual + VisualLine (+ R binding)
- goal: selection anchor on Window; `o`/`O` swap ends; ops: d x c s y > <
  gu gU g~ = ; `p` replaces selection; R (Replace mode, engine P5.8) bound here.
  (visual `u`/`U`/`~` arrives in P15.4.)
- ACCEPT: motion updates selection; `Vd` deletes lines; `V>` indents block; `vp`
  replaces with char/line register content correctly.
- depends: P2.7, P4.

### P6.3 Cmdline / search prompt state
- goal: typed string + history ring (per prompt char); Enter emits
  `ExRun{line}` / `SearchRun{pat, dir}` Commands as typed; visible in the cmdline
  row (P2.4).
- depends: P2.4.
- ACCEPT: history cycles up/down; empty Enter is a no-op.
- (Actual `:` parsing is P7; `/ ? n N` behavior is P9.)

### P6.4 Event queue + job pool skeleton
- goal: MPSC `EventQueue` + 2-thread job pool; fake job posts 1000 ordered
  events; App drain helper for reducer tests. (GROUNDWORK for P10 scan/grep,
  P14 terminal/`:!`/OSC52.)
- files: `app/event_queue.hpp/.cpp`, `app/jobs.hpp/.cpp`, `app/events.hpp`.
- ACCEPT: multithreaded test (≤2 job threads): drain preserves per-producer order.
- edges: wake fd absent (pure queue usable without an OS event).

### P6.5 PHASE CHECK — scripted vim session, live
- goal: run the PREVIEW §P6 script both as feeder tests AND live; assert final
  state; sanitizer run.

## P7 — Ex commands (prompt row already live from P2.4/P6.3)

Files: `core/ex_parse.hpp/.cpp`, `core/ex_engine.hpp/.cpp`, reducer glue in
`app/*.cpp`. Input entry via P6.3.

### P7.1 Ex parser
- goal: range grammar `%`, `.`, `$`, `N`, `N,M`, `.,+3`; bang; args; command-name
  abbreviation (longest unique prefix).
- ACCEPT: parser table tests incl. `:'<,'>s/x/y/g`.
- edges: trailing count after flags (`:s/x/y/g 3` — reserved, error for now);
  stray spaces.

### P7.2 File commands :w :wq :q :q! :qa :e :enew
- goal: file IO via Buffer::save/load; unsaved-confirm flow (message +
  `q!`/`wq` overrides); `:e` reloads from disk keeping the cursor.
- ACCEPT: tests with temp dirs; `:w other.cpp` retargets the buffer filename
  (vim behavior: save-as under the new name).
- depends: P1, P2.

### P7.3 Buffer commands :ls :b name/:bN :bn :bp :bd
- goal: `:ls` listing string (format test); switch by number/partial-name;
  `:bd` unloads the buffer (windows showing it collapse; refuse without bang
  when modified).
- ACCEPT: list format test; window collapse to nearest remaining window.
- (Window wiring finalized in P8.)

### P7.4 :<number> + :$ goto
- goal: pure range command → move cursor (col = first nonblank), clear pending.
- ACCEPT: `:3` on a 2-line buffer → error message state, no crash.

### P7.5 :set options
- goal: settings table (`tabwidth, number, relativenumber, scrolloff, expandtab,
  shiftwidth, lsp [optional placeholder]`) with `key=value`, `nokey`, boolean
  toggles; invalid → error entry, no crash.
- ACCEPT: set/get tests; unknown key error; `:set ts?` prints value.
- files: `core/settings.hpp/.cpp` (single cohesive pair), reducer hook.

### P7.6 :s/// (single line & range)
- goal: regex replace on `:'<,'>` or current line via P1.7; flags `g i I c E`;
  cursor to last substitution; empty pattern reuses last search text (P9 fills).
- ACCEPT: tests; `c` flag pauses per match (injected chooser in tests).
- depends: P1.7, P7.1.

### P7.7 :noh (and the highlight model)
- goal: `Highlight` state (matches, kinds) + `:noh` clears until next search.
- ACCEPT: model state toggles tested.
- (The match-filling itself is P9.)

### P7.8 Messages ring + :messages (scratch buffers land here)
- goal: `Messages` ring + statusbar echo (visible since P2.4) + `:messages`
  opens a scratch buffer (vim-like; scratch = unmodifiable, listed in `:ls`;
  reused later by `:!out` P14.5).
- ACCEPT: demo (echo appears; `:messages` shows history in a scratch buffer).

### P7.9 PHASE CHECK
- goal: ex e2e suite: parse→apply for every table row; message texts asserted;
  live demo per PREVIEW §P7.

## P8 — Windows + tabs

Files: `core/layout.hpp/.cpp` (model), reducer + ui glue.

### P8.1 Layout split tree
- goal: binary-tree model, focus-WindowId, split (`:sp`, `:vs`) with minimum
  sizes; close collapses; `:new` (horizontal split, empty buffer).
- api: `Layout::split(WindowId, Dir); close(WindowId); equalize();`
- ACCEPT: model tests (sizes sum to 100%, min clamp, 2-pane collapse).
- depends: P7.3. (:ls/:bd rows)

### P8.2 ctrl+w prefix keys
- goal: h/j/k/l/w focus, `c` close, `o` only, `=` equalize, `+`/`-`/`<`/`>`
  resize, `s`/`v` split single-window flows unchanged.
- ACCEPT: headless state asserts + UI check.

### P8.3 Windows render (multiple)
- goal: draw each window rect (P7.2 parametrized by rect), active/inactive
  statuslines (dimmed), separator lines, cursor only in the focused window.
- ACCEPT: PREVIEW demo (`:sp` → two panes, typing goes to focused).

### P8.4 :tabnew gt gT :tabs + tabbar
- goal: tabs array in EditorState (each with own Layout); `:tabnew` (and
  `:tabe f`); `gt`/`gT` with prefix count (`2gt`); tab bar row rendering.
- ACCEPT: model tests + UI demo.

### P8.5 :sp :vs :new :only — ex hookup + focus routing
- goal: ex table rows call Layout; focus follows the newly created window
  (vim rule).
- depends: P8.1, P7.
- ACCEPT: ex tests.

### P8.6 :b switching updates the ACTIVE window only
- goal: `:b N` shows the buffer in the active window; other windows untouched;
  closing a buffer gracefully collapses panes showing it.
- ACCEPT: state tests.

### P8.7 Per-window scrolling
- goal: view state per Window; ctrl+d/u/f/b (and `e`/`y` scroll) affect the
  focused window only; horizontal scroll persists per window.
- ACCEPT: demo + state assert.

### P8.8 PHASE CHECK
- goal: demo: 2 splits + 2 tabs, scripted PREVIEW §P8.

## P9 — Search + substitute (UI wiring)

### P9.1 / ? prompt flow
- goal: typed pattern → P1.7 search (regex per Query settings), highlight all
  matches, cursor to first, direction persists for n/N.
- ACCEPT: PREVIEW script; no-match message; cursor position kept when nothing found.
- depends: P7.7 (highlight model), P6.3 (prompt).

### P9.2 n N + last-search state
- goal: counts on n/N; `;`/`,` (P3.3) keep their own last-find state; wrap-around
  message shown once per wrap.
- ACCEPT: wrap test (differential).

### P9.3 * and #
- goal: word under cursor → search (visual-selection text if in Visual), strict
  word boundaries.
- ACCEPT: tests + demo.

### P9.4 Highlight rendering + :noh
- goal: matches drawn inverted in the view; `:noh` clears until the next search.
- ACCEPT: demo.

### P9.5 :s/// confirm UI
- goal: wire P7.6 to the interactive `c` chooser (`y naql` in cmdline); Visual
  mode `:s` pre-fills `'<,'>`.
- ACCEPT: demo; `a`/`q` abort semantics.

### P9.6 :%s///g + result messages
- goal: range `%` applied; message "N substitutions on M lines".
- ACCEPT: messages + counts tested.

### P9.7 PHASE CHECK
- goal: PREVIEW §P9 script.

## P10 — Fuzzy picker + explorer + grep

### P10.1 File-scan worker
- goal: job-pool worker walks cwd; built-in skip list (`.git/`, `build/`,
  `node_modules/`) + basic `.gitignore` line parsing (glob subset, built-in —
  the git binary is never invoked); posts `FileScanDone` (batched); debounced
  re-scan on request.
- files: `app/jobs.*`.
- ACCEPT: fake-tree fixture: correct list incl. ignore rules honored.
- depends: P6.4.

### P10.2 Generic list popup UI
- goal: centered popup (list, filter-as-you-type, arrows/ctrl+j/k, Enter, Esc);
  reused later by buffer picker and (optional) LSP pickers.
- ACCEPT: PREVIEW demo with tmp files.

### P10.3 <space>ff fuzzy file open
- goal: fuzzy scorer (subsequence match + compactness penalty); Enter opens the
  file in the active window; `Esc` closes. Splits/tabs stay via `:sp`/`:tabe` +
  completion (P10.7) — the picker itself has no split modes in v1.
- ACCEPT: PREVIEW demo; fixture ranking tests (prefix hits rank first).

### P10.4 <space>fg project grep
- goal: typed pattern → job-pool grep over the P10.1 file list (batch by size),
  results in the popup (`file:line:col  preview`), Enter opens at the match,
  cycling within one file, Esc closes; no-match message.
- files: `app/jobs.*`, `ui/popup.*`, reducer additions.
- api: `Job grep_project(sv pattern) -> req;` event
  `GrepDone{hits: vector<{path, line, col, text}>}`.
- ACCEPT: fake-tree tests; binary/oversize files skipped; open-at-match correct.
- edges: huge repos → batched arrival, popup shows count-so-far.
- depends: P10.1, P10.2, P9 highlights.

### P10.5 Explorer buffer (:E / <space>e)
- goal: netrw-style scratch buffer listing the cwd tree (dirs suffixed `/`);
  Enter opens the file, `-` goes up one dir, `o` opens in a split; movement keys
  j/k/g/G.
- ACCEPT: demo + state tests (explorer buffers are flagged unmodifiable).
- depends: P10.1 (uses scan events), P8 (split-open).

### P10.6 Explorer refresh + reveal
- goal: `R` rescans; `gf` in the explorer reveals the active buffer's file and
  moves the cursor there; per-tree cursor memory across re-scans.
- ACCEPT: demo.

### P10.7 :e file completion
- goal: cmdline `:e <partial>` shows a completion popup of file matches;
  Tab completes (cmdline hook).
- ACCEPT: demo.

### P10.8 PHASE CHECK
- goal: PREVIEW §P10 script incl. `<space>fg` grep demo.

## P11 — Config (nlohmann/json)

### P11.1 Settings merge from config.json
- goal: load `~/.config/noni/config.json` → cwd → exe dir (first found wins);
  known keys only (see schema in ARCHITECTURE/P11 fixture in PREVIEW); unknown
  keys warn once, never fail; missing file = defaults; malformed JSON → defaults
  + error message.
- files: `app/config.hpp/.cpp`.
- ACCEPT: fixture configs tested; malformed JSON → message + defaults.
- depends: P6.5.

### P11.2 Apply + live :set effect
- goal: config values flow into the same Settings the `:set` engine owns; edits
  via `:set` change rendering instantly (no restart).
- ACCEPT: demo (`:set relativenumber` flips the gutter immediately).

### P11.3 lsp config section (P13 hookup)
- goal: parse `lsp.servers[]` placeholders (language/command/rootMarkers) and
  stash them; no behavior in v1 — P16 consumes this if built.
- ACCEPT: parse tests.

### P11.4 PHASE CHECK — config e2e
- goal: all above shown in PREVIEW §P11.

## P12 — Syntax highlighting (tree-sitter)

### P12.1 Tree-sitter skeleton
- goal: pkg-config link; parser per language c/cpp/python/rust; embedded minimal
  highlight queries (keywords/strings/comments/numbers/functions/types).
- files: `syntax/*` (queries embedded as strings, no runtime files).
- ACCEPT: fixture file → token spans asserted.
- depends: P0.2 + language detection (P12.4 supplies the extension map).

### P12.2 Incremental reparse — MAIN THREAD
- goal: on `post_buffer_change`, reparse ONLY the affected lines/visible range on
  the main thread via `ts_tree_edit` + partial parse; debounce by animation frame
  (e.g. min 16 ms between parses); deterministic, no worker in v1.
- files: `syntax/highlight.*`.
- ACCEPT: edit→visible-range highlight updates within one frame in tests
  (fixture timings); long-document parse cost proportional to visible range.
- edges: edits above the viewport (span tracking), multi-line string truncation.

### P12.3 Renderer colors
- goal: token → color-pair table (theme), fallback plain; graceful when a grammar
  is missing (plain text, no crash).
- ACCEPT: PREVIEW visual.

### P12.4 Language detection + extension map
- goal: filename→language table; unknown → plain text via fallback lexer (P12.5).
- ACCEPT: tests.

### P12.5 Fallback lexer (minimal)
- goal: regex-lite fallback (keywords/strings/comments/numbers) for extensions
  without grammars.
- ACCEPT: fixture asserts.

### P12.6 PHASE CHECK
- goal: PREVIEW §P12 (highlight visible + smooth scroll).

## P13 — LSP

Isolated by design: nothing outside P13 depends on this phase — the editor works
fully when no server is configured or the binary is missing. Adds the `noni_lsp`
target and `LspReply/LspDiag` event types. All I/O worker-side (jsonrpc worker
owns pipes) per ARCHITECTURE §5 rules; config placeholders parsed in P11.3.

### P13.1 JSON-RPC framing + protocol types
- goal: Content-Length framing; serialize/parse via nlohmann/json; typed structs
  for Position/Range/TextEdit/Diagnostic/CompletionItem/Hover.
- ACCEPT: encode/decode tests incl. Unicode offsets (UTF-16 columns!).
- depends: P11.3 (json lib + config placeholders).

### P13.2 LspProcess (pipes + worker)
- goal: spawn from `lsp.servers[].command`; worker loop reads stdout → decode →
  post `LspReply/LspDiag`; mailbox accepts sends (never blocking main).
- ACCEPT: fake server script (python) request/response test.
- depends: P13.1, P6.4.

### P13.3 Session lifecycle
- goal: per-BufferId doc state; initialize (roots via config rootMarkers);
  didOpen/Change (revision throttled)/Save/Close; restart on crash; status
  messages.
- ACCEPT: fake-server lifecycle test incl. pending-change flush.
- depends: P13.2, notify path (`post_buffer_change` subscriber).

### P13.4 didChange sync (full + incremental)
- goal: incremental ranges via UTF-16 mapping (`text.hpp` helpers); full fallback
  on mismatch detection.
- ACCEPT: round-trip tests (server echo).

### P13.5 Completion UI
- goal: trigger on insert-mode idle 150 ms + trigger chars, and manual;
  popup (P10.2), `<c-n>/<c-p>/<cr>`, detail preview.
- ACCEPT: fake server; popup nav tests; insertion applies `textEdit` ranges.

### P13.6 Hover (K)
- goal: popup at word; plain text + limited markdown (code spans); dismiss on move.
- ACCEPT: demo with fake server.

### P13.7 Goto def/decl/impl + references
- goal: `gd`/`gD`/`gy` jump when a single hit (jumplist push); multi-hit opens
  the list popup; `<space>gr` references list with open-on-Enter.
- ACCEPT: fake server tests + demo.

### P13.8 Rename
- goal: `<space>rn` prompt via cmdline; apply WorkspaceEdit; unsaved-buffers
  prompt; multi-file undo works.
- ACCEPT: fake server; undo restores all files.

### P13.9 Diagnostics
- goal: `LspDiag` events → per-buffer diagnostics (version-gated); gutter signs
  E/W, current-highlight, `:Diag` list, `[d ]d`; stale-version results dropped.
- ACCEPT: fake server tests + demo.

### P13.10 Formatting
- goal: `:Format` (+ optional format-on-save, default OFF); async apply in one
  undo group; conflict check against disk changes.
- ACCEPT: fake server; rapid saves never freeze (latency test).
- depends: P13.2 (pipes worker), P6.4 (job pool); all I/O async.

### P13.11 PHASE CHECK
- goal: real clangd demo (PREVIEW §P13) + sanitizer run.

## P14 — Terminal + shell command + clipboard

Files: `terminal/pty.hpp/.cpp`, `terminal/vt.hpp/.cpp`, `terminal/worker.hpp/.cpp`,
`ext/shell_job.hpp/.cpp`, `core/register.hpp` (OSC52 hook), new `Event` variants
`TermOutput/TermExit/ClipDone`. The PTY worker owns the terminal fd; editor state
is never touched from that thread.

### P14.1 PTY spawn + TermOutput stream
- goal: `forkpty` child running the configured shell (`terminal.shell`, default
  `$SHELL`); dedicated worker owns the master fd, reads chunks, posts `TermOutput`;
  resize propagation (SIGWINCH → TIOCSWINSZ); scrollback cap (5000 lines,
  configurable); child exit → `TermExit`.
- ACCEPT: echo test with a fake program; cap enforced; exit detected.
- depends: P6.4 (jobs/events), P2.5 (main loop wake).

### P14.2 VT grid parser (subset)
- goal: grid of cells (codepoint + style); parse subset — CSI cursor moves, SGR
  colors/attrs, erase ops (char/line/screen), scroll region, OSC title; DECCKM
  application-cursor mode flag; NO images/sixel.
- files: `terminal/vt.hpp/.cpp`, golden-sequence tests.
- ACCEPT: golden escape-sequence fixtures → asserted grids; malformed input
  never crashes.
- depends: P14.1.

### P14.3 :term + pane
- goal: `:term` opens a terminal pane as a bottom split (height via
  `terminal.height`, default 40%); `i` enters terminal focus mode, `Esc` leaves
  back to the editor; ctrl+w window keys still work; closing the pane ends the
  child (prompt on running job).
- ACCEPT: PREVIEW demo (bash, `ls`, `exit`); focus routing tests.
- depends: P14.2, P8 (splits).

### P14.4 Key input + copy mode
- goal: focused terminal gets raw keys (arrows/F1-12 per DECCKM state, bracketed
  paste forwarded); `Esc` opens copy mode over scrollback: `k/j/ctrl+b/ctrl+f`
  navigate, `y` copies selection into `+` (OSC52), `q`/`Esc` leaves.
- ACCEPT: key routing table tests (DECCKM on/off) + PREVIEW demo.
- depends: P14.3, P2.1 (Key reader), P14.6.

### P14.5 :!cmd — async shell jobs
- goal: `:!cmd` (and `:!cmd %` with `%`→current file expansion) runs as a pooled
  job; output streams into a scratch buffer (`:!out`), unmodifiable, listed in
  `:ls`; exit status shown in the message line; jobs killed on timeout
  (default 30 s, configurable) with the child process reaped; stdin is closed
  for the child.
- api: `Job shell_job(sv cmd, fs::path cwd) -> req;` events `ShellOutput{chunk}`,
  `ShellExit{code, timed_out}`.
- ACCEPT: tests: fast command output complete; `%` expansion; timeout kill
  fixture (`:!sleep 60` killed at 30 s, UI never frozen); nonzero exit → message.
- edges: huge output → chunked arrival; binary output sanitized.
- depends: P6.4 (jobs), P7.8 (scratch buffers).

### P14.6 OSC52 clipboard write (async, "+ / * registers)
- goal: writing the `+` (or `*`, aliased) register schedules an OSC52 escape
  (base64) write to the controlling terminal ON THE JOB POOL (never on the main
  thread) — NO subprocess clipboard tools ever; the stored text also lands in
  the register so `"+p` works within the session; no OSC52 read-back in v1
  (`"+p` of foreign content is message "unsupported"); completion posts
  `ClipDone` (status message only).
- api: `void osc52_write(sv utf8) -> req;` event `ClipDone{ok|skipped}`.
- ACCEPT: unit test: escape sequence bytes format; async order test; demo in a
  supporting terminal.
- edges: huge yank → config cap (default 100 KiB, larger → `ClipDone{skipped}`
  + message, register still holds the text). Implicit yanks (no register
  prefix) do NOT emit OSC52 (bandwidth) — documented.

### P14.7 PHASE CHECK
- goal: PREVIEW §P14 script; sanitizer run.

## P15 — Extras (vim polish)

### P15.1 VisualBlock (ctrl+v)
- goal: block selection on the Window; ops `d x y c I A` (insert per line),
  block paste; visual replace `r`.
- ACCEPT: golden tests + demo.

### P15.2 :map/:nnoremap/:iunmap
- goal: simple rebind table over key sequences (no when-clauses); `:map` lists;
  recursion → error, never loops.
- ACCEPT: tests + demo (`:map <C-x> :w<cr>`).

### P15.3 J polish + gJ
- goal: `J` joins with one space, smart about trailing whitespace; `gJ` no
  space; `[count]J`; wide-char spacing rule.
- ACCEPT: tests (differential).

### P15.4 Visual u/U/~
- goal: lowercase/uppercase/toggle over the selection, undo as one group.
- ACCEPT: tests.

### P15.5 zz zt zb + horizontal z-correct
- goal: `zz`/`zt`/`zb` final semantics (with scrolloff), side scroll `z h/l`
  (3 l default), `z<cr>` recenter horizontally… minimal cumulative fixes.
- ACCEPT: visuals + unit tests of scroll offsets.

### P15.6 PHASE CHECK
- goal: PREVIEW §P15 script.

## P16 — Session / recovery / autosave

### P16.1 Autosave afterDelay
- goal: tick in the main loop writes dirty file buffers (never scratch/explorer);
  conflict re-check (disk mtime) before write; a conflict skips saving and shows
  a message (no popup).
- ACCEPT: mtime-conflict test.

### P16.2 Crash recovery snapshots
- goal: debounced snapshot per dirty buffer into `<ws>/.noni/recovery/<buf-hash>`;
  removed on save/close; startup: detect + list prompt `restore? [y/N]`.
- ACCEPT: kill -9 fixture then start → prompt restores content.

### P16.3 Session state
- goal: on `:qa` (any exit path) persist open files/cursors/window
  layout/tabs/recent files to `~/.local/state/noni/session.json` (XDG).
  Restoring is explicit: `noni --restore` or `:Sload`; default startup does NOT
  restore (vim-natural).
- ACCEPT: fixture session roundtrip (widgets→json→state).

### P16.4 Recent files
- goal: MRU list persisted; `<space>fr` opens the list popup; opened files update
  MRU order.
- ACCEPT: test.

### P16.5 Startup flags
- goal: `noni [file...]` opens files (cursor restoration per P16.3 if requested);
  `noni --restore` loads the session; `noni --version`.
- ACCEPT: manual + flag unit test.

### P16.6 Fsync + atomic save everywhere
- goal: temp file + fsync + rename for buffer save, recovery snapshots, session
  writes (same helper).
- ACCEPT: crash-mid-write fixture leaves old content intact.

### P16.7 PHASE CHECK + final acceptance
- goal: PREVIEW final v1 script (all sections), full warning + sanitizer pass,
  `v1.0-alpha` tag pushed.

