# ROADMAP.md — Noni v2 (new/)

Every task: **ID · title · goal · depends · files · public API (C++23) ·
acceptance test · edge cases · done**. One task ≈ one behavior, ≤ ~100 lines.
"Trace" = manual keystroke check from PREVIEW.md. Phase order is fixed; each phase
ends with a `check` task. Cross-phase ex/window commands are placed in the phase
that owns their machinery (noted inline).

Conventions for all phases: build = `cmake --build build`; test =
`ctest --test-dir build --output-on-failure`; sanitize =
`ASAN_OPTIONS=detect_leaks=1 ./build/noni` (every Debug build is sanitized).

## Summary

| Phase | Title | Tasks | User capability after phase |
|---|---|---|---|
| P0  | CMake skeleton | 3  | empty tui-less binary builds, ctest runs a placeholder |
| P1  | Core buffer + undo | 8  | (library only) tested text edits + undo/redo |
| P2  | Motions | 10 | (tested) full motion vocabulary incl. counts |
| P3  | Operators + text objects | 9  | (tested) d/c/y/</>/gu/gU/g~/= over motions+objects |
| P4  | Registers/marks/dot/macros | 9  | (tested) p/P, ". mac, q/@, marks, jumplist |
| P5  | Modes + reducer | 7  | headless raw vim session replaying a key stream |
| P6  | Ex commands | 8  | `:` prompt, :w :q :e :b :bn :bd :s/// :N :set (no screen) |
| P7  | Renderer | 9  | **first visible editor**: single window, scroll, highlight |
| P8  | Windows + tabs | 8  | :sp :vs ctrl+w, :tabnew gt/gT |
| P9  | Search + substitute | 7  | / ? n N * # :noh :s/// :%s///g + highlights |
| P10 | Picker + explorer + grep | 8  | <space>ff fuzzy files, <space>fg grep, <space>e explorer |
| P11 | Config | 4  | config.json honored, :set works end-to-end |
| P12 | Syntax (tree-sitter) | 6  | c/cpp/python/rust highlighting, async reparse |
| P13 | LSP | 11 | completion/hover/def/refs/rename/diag/format, async |
| P14 | Git | 6  | gutter signs, <space>g s/a/u/x status panel |
| P15 | Terminal + :! | 6  | :term PTY panel, :! cmd, key passthrough |
| P16 | Extras | 5  | ctrl+v block visual, :map/:nnoremap, J polish |
| P17 | Session/recovery/autosave | 7  | restore across runs, crash recovery prompts |

---

## P0 — CMake skeleton

### P0.1 Top-level CMakeLists
- goal: project(), C++23, warnings `-Wall -Wextra -Wpedantic`, Debug
  `-fsanitize=address,undefined`, options `NONI_SANITIZERS` (ON in Debug).
- files: `new/CMakeLists.txt`
- api: targets (created over P0.2): `noni_core noni_lsp noni_git noni_syntax
  noni_terminal noni_app noni_ui noni` + `noni_tests`.
- ACCEPT: `cmake -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build` succeeds
  (no sources yet beyond main stub).
- edges: CMake 4.x requires `cmake_minimum_required(VERSION 3.28…4.0)` compat.
- done: configure+build clean, zero warnings.

### P0.2 Targets + layer include enforcement
- goal: static libs per ARCHITECTURE §1; `target_link_libraries` order
  core ← services ← app ← ui ← noni; add `-I src` include scoping per target so a
  wrong include fails to compile.
- depends: P0.1
- files: `new/CMakeLists.txt`, `new/src/*/.gitkeep`-only stubs
- ACCEPT: intentional `#include <ncurses.h>` in `core/` fails the build (temp check,
  reverted).
- edges: pkg-config `tree-sitter` + `ncursesw` optional at this stage →
  `if(PKG_CONFIG_FOUND)` guards, OFF until P12/P7.
- done: target graph exists; `noni` links empty main.

### P0.3 doctest + main stub + ctest smoke
- goal: FetchContent doctest; `tests/smoke_test.cpp` asserting 1==1; `main.cpp`
  builds, links app+ui, `noni --version` prints and exits 0.
- depends: P0.2
- files: `new/tests/smoke_test.cpp`, `new/src/main.cpp` (moved here last, per tree)
- api: `int main(int argc, char** argv)` skeleton in main.cpp (no ncurses yet:
  `noni --version` → stdout, else error "UI arrives in P7", exit 2).
- ACCEPT: `ctest --test-dir build` runs smoke test; `./build/noni --version` prints.
- edges: FetchContent behind proxy → document manual fallback in commit body.
- done: ctest green; committed `P0.3`.

## P1 — Core buffer + undo (tests only)

### P1.1 Pos/ids/text primitives
- goal: `Pos`, `BufferId/WindowId`; UTF-8 decode/encode; `col↔byte` conversion in a
  line; codepoint width (tabs = `tabwidth`); word char classes (word/WORD/punct).
- files: `core/pos.hpp`, `core/text.hpp/.cpp`, `tests/text_test.cpp`
- api: `std::size_t cp_to_byte(std::string_view line, std::uint32_t col);`
  `std::uint32_t byte_to_cp(...); U32 next_cp / prev_cp;`
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
- ACCEPT: interleaved undo/redomutations; truncate-on-branch; cap evicts oldest.
- edges: edits before `begin_group` belong to the previous txn; empty txn dropped.
- done: tests green.

### P1.5 Buffer edit sessions
- goal: `with_undo(kind)` wrapper that marks the txn boundary + auto-merges typing
  groups (same-revision char runs) — vim-like `u` granularity.
- depends: P1.4
- files: `core/buffer.hpp/.cpp`
- api: `template<class F> auto with_edit(EditKind, F&&);`
  `UndoResult undo(); UndoResult redo();`
- ACCEPT: `dw` + `u` restores both text and cursor (test).
- edges: nested with_edit (reuse inner group); exception inside F → rethrow, group
  still consistent.
- done: tests green.

### P1.6 Buffer revision + snapshot
- goal: monotonic `revision()`; `snapshot_lines()` cheap view for workers/LSP.
- depends: P1.5
- files: `core/buffer.hpp/.cpp`
- api: `std::shared_ptr<const std::vector<std::string>> snapshot_lines() const;`
- ACCEPT: revision bumps on any with_edit; snapshot stable (no dangling after edit).
- edges: snapshot shared_ptr keeps old lines alive (memory note: review in P15.1).
- done: tests green.

### P1.7 BufferSearch (pure)
- goal: literal + ECMAScript-regex find all/next, replace range → new lines (pure
  functions over a line vector; the editor applies them later).
- depends: P1.2
- files: `core/buffer_search.hpp/.cpp`, `tests/buffer_search_test.cpp`
- api: `struct Query { bool regex, case_sensitive, whole_word; };`
  `std::vector<Match> find_all(lines, sv pattern, Query);`
  `eresult<std::vector<std::string>> replace(lines, pattern, sv repl, Query, bool all);`
- ACCEPT: tests: escapes, multiline none (per line), case, whole-word, bad regex err.
- edges: empty pattern → error; `$`/`^` anchors per line.
- done: tests green. (Search UI keys are P9.)

### P1.8 PHASE CHECK — core green + sanitizer
- goal: entire core compiles warning-free; all P1 tests pass under ASan+UBSan run of
  the test binary; commit tag `P1 done`.
- ACCEPT: `ctest -R core` 100%; sanitizer run of `noni_tests` clean.
- done: phase reported to user with PREVIEW §P1 demo (test output).

## P2 — Motions (pure, still no screen)

All motion tasks share files `core/motion.hpp/.cpp` + `tests/motion_test.cpp`,
api style: `struct Motion { MotionId id; args...; Pos apply(const TextView, Pos, uint32_t count); };`
`TextView` = lines + tabwidth + view scroll (for zz). depends: P1.

### P2.1 Framework + h j k l 0 $
- goal: MotionContext, clamping (inclusive last char in Normal), apply(count).
- ACCEPT: h at col0 stays; 5j stops at last line; $ on empty line.
- edges: movement past end; count 0 → 1.

### P2.2 Word motions w W
- goal: vim `w`/`W` incl. end-of-line skip, empty-line boundary.
- ACCEPT: "foo.bar baz" cases; w on trailing blank line stops.

### P2.3 Word motions b B e E
- goal: backward + end motions; e on last char of file.
- ACCEPT: tests from vim docs table (word/WORD/punct mixes).

### P2.4 f F t T ; ,
- goal: find-char with count; `;`/`,` repeat with count; per-line persist; last
  find state (module-level per window? Keep in Window: `last_find`).
- ACCEPT: `2fx`, `3;` chains;,left `t` backwards.
- edges: no match → no move, error beep flag (no message yet).

### P2.5 ^ $ gg G + counts
- goal: first-nonblank, goto line via count (`123G`), file start/end.
- ACCEPT: `$` on very wide tab; `3G` on 2-line file → last line.

### P2.6 { } sentence/paragraph
- goal: blank-line paragraph moves; `{`/`}` on empty buffer.
- ACCEPT: paragraph tests; no infinite loops.

### P2.7 Page + scroll: ctrl+d/u/f/b, H M L, zz zt zb
- goal: view scrolls (updates TextView scroll), H/M/L pick rows in view.
- depends: none for logic, but touches view state — keep pure over TextView.
- files: same + `core/view.hpp/.cpp` (view scroll state, scrolloff calc).
- ACCEPT: ctrl+f at EOF clamps; zz keeps line centered.
- edges: view shorter than screen; scrolloff > half view.

### P2.8 % bracket match
- goal: match `(){}[]` forward/backward, skip in comments/strings = none for now
  (plain scan, note limitation), count% jumps.
- ACCEPT: nested + unmatched → no move.

### P2.9 Cursor column memory
- goal: `preferred_col` (j/k through short/long lines restore column) — Window
  field + apply in vertical motions (P2.1 update).
- ACCEPT: long→short→long keeps column.

### P2.10 PHASE CHECK
- goal: motion test table sweeping (idempotent, count>line fallbacks), phase report.

## P3 — Operators + text objects

Files: `core/operator.hpp/.cpp`, `core/textobj.hpp/.cpp`, tests both. depends: P2.

### P3.1 Operator parse state
- goal: `Pending` FSM: `[count]op` → wait motion/textobject/`op` (linewise) →
  build `Edit{op, start, end, kind}`; register prefix `"x` recorded before/with.
- files: `core/operator.hpp/.cpp` (+ scope into P5 later); tests.
- api: `std::optional<Edit> step(Pending&, Key, const TextView);`
  `std::optional<Edit> step_visual(Pending&, sel);`
- ACCEPT: `2d3w` = 6 words; `dd` = line; `cc` keeps indent (uses P3.6 later: keep
  indent via join+insert, test here).
- edges: `d` then Esc cancels; same op same `d` twice → linewise.

### P3.2 d y (delete/yank)
- goal: apply Edit: text→register (Reg kind char/line), cursor to range start
  (line ops → first nonblank of line).
- depends: P3.1 (+P4 registers API stubbed as unnamed first: Reg snapshot).
- ACCEPT: `dw` at EOL; `y$` cursor stays; deleted line yank = linewise.

### P3.3 c (change)
- goal: delete + enter Insert (mode transition handled in P5; here produce
  Edit + post_action=EnterInsert).
- ACCEPT: `cc`/`cw` (→ `cw` behaves vim-accurate: win on word... vim special-cases
  `cw` to not move past EOL — TEST).

### P3.4 Text objects iw aw ip ap
- goal: selection ranges; `aw` includes trailing space/newline rules.
- ACCEPT: "foo bar ," samples; `ap` merges trailing blank lines vim-style.
- depends: P2 word motions.

### P3.5 Text objects quotes + brackets
- goal: i" a" i' a' i( a( i{ a{ i[ a[ (dot-in or on-bracket), multiline ok.
- ACCEPT: `da(` across lines; unmatched → error (no edit, count consumed).

### P3.6 > < (shift)
- goal: indent lines by `shiftwidth` (default tabwidth 4 → use tab chars), with
  counts and repeat `.`; cursor to first nonblank of first line.
- ACCEPT: visual-line >N later; here Normal `3>>`.
- edges: empty lines unaffected; tabs mix.

### P3.7 gu gU g~
- goal: case ops over ranges (use `guu`, `gUU`, `g~~` doubled shortcut too).
- ACCEPT: accented UTF-8 via wide-char towupper/towlower; ASCII-only fallback
  documented if the locale is C (edge case test).
- edges: on nonletters no-op.

### P3.8 = (auto-indent range)
- goal: re-indent lines from context (bracket depth + prev line indent); v1
  heuristic only (c-family aware via balanced braces).
- ACCEPT: `=i{` inside function body normalizes indent.
- edges: mixed tabs/spaces → expandtabs=false keeps tabs.

### P3.9 PHASE CHECK
- goal: churn = table of op×motions×objects × counts regression test set.

## P4 — Registers/marks/jumps/dot/macros

Files: `core/register.hpp/.cpp`, `core/marks.hpp/.cpp`, `core/macro.hpp/.cpp`,
tests. depends: P3.

### P4.1 Register model
- goal: unnamed, numbered 0-9 (yank/delete history), named a-z (append via A-z),
  small delete `-`, blackhole `_`, read-only system `+`/`*` (post-and-forget for
  now, worker later P15/P13; store content only).
- api: `RegSet::get(char)->RegView; void set(char, RegView, bool append);`
- ACCEPT: dd→"1 rotate; cw→"-; "ayy/tmp; "Ayy append.
- edges: count prefix multiplies (`3"ayy` = 3×? actually repeat-reg = P4.5).

### P4.2 paste p P
- goal: char/linewise from RegView (with count multiplier), cursor placement rules,
  paste-then-auto at EOL; visual paste handled in P5 visual (acceptance there).
- ACCEPT: `3p` linewise; p on empty buffer.

### P4.3 Marks a-z (and 'x vs `x)
- goal: set (`mx`), jump-line (`'x`) vs exact (`` `x ``), invalidated on line ops
  (line index shift).
- ACCEPT: mark survives insert above (moves), dies if its line deleted.

### P4.4 Jump list + ctrl+o / ctrl+i
- goal: cap 50, dedupe consecutive, push on big jumps only (>2 lines or mark).
- ACCEPT: jump chain round trip; insert-mode i at last pos restores.

### P4.5 Dot repeat `.`
- goal: record last Edit (op+motion+count+register) + simple insert sequences;
  `.` replays with current cursor.
- depends: P3.
- ACCEPT: `dw.` on 3 words; `ciwfoo<esc>.` second word→foo; insert text repeat.
- edges: `.` after undo doesn't retrigger; visual edits replayable.

### P4.6 Macros q a…q
- goal: record normal-mode key sequence to register (store RAW keys, to be replays
  as keys) incl. aborted q (no-op), recursion guard level 1, play `@a` count N, `@@`.
- ACCEPT: record+play edits + moves; error inside macro aborts (beep) — no dialogs.
- edges: q registered while playing (1 level), not deeper.

### P4.7 r, x, ~, J basics
- goal: replace-char (with count), delete-char (yank), toggle case over motion,
  join with smart space handling (J + gJ later P16).
- ACCEPT: `3rx` past EOL clamps; J removes comments? (no — plain join with space).
- depends: P1, P2.

### P4.8 R (Replace mode) DFS
- goal: overwrite chars in place; Backspace restores original; Esc exits extending
  undo group correctly.
- ACCEPT: replace past EOL appends; newline in replace switches to insert-line.
- depends: P5 modes skeleton? — keep as pure edit op now; binding in P5. (decided:
  implement as Edit; key binding in P5.4.)

### P4.9 PHASE CHECK
- goal: "macro + repeat + undo" interaction matrix test; report.

## P5 — Modes + the reducer (still no ncurses!)

### P5.1 EditorState + reducer shell
- goal: `EditorState{tabs, windows, buffers, mode, cmdline, msgs,…}` + single
  `reducer(EditorState&, Command) -> void`; Simple in-memory key feed.
- files: `app/reducer.hpp/.cpp`, `core/editor_state.hpp/.cpp`, tests.
- ACCEPT: a 40-key scripted session ends in asserted state (text/mode/windows).
- edges: all commands no-op safely when `BufferId` invalid.

### P5.2 Normal mode dispatcher
- goal: pending FSM (counts, operators, registers, `g`/`z`/ctrl+w prefixes) →
  Command; binds P2/P3/P4 pieces; status text stub for UI later.
- ACCEPT: key sequence tables from PREVIEW §P5 replay identically as tests.
- depends: P2..P4, P5.1.

### P5.3 Insert mode
- goal: insert chars incl. UTF-8 input stream, auto-indent on Enter (copy prev
  indent, `++` rules: `o`/`O` too), smart Backspace (indent), ctrl+w (delete word
  back), ctrl+u (line), Exit via Esc (jk/jj only if option set later).
- ACCEPT: typing flow tests; `ctrl+w` edge at col 0.
- depends: P5.2.

### P5.4 Visual + VisualLine (+ Replace bind)
- goal: modes with selection anchor on Window; o/O swap ends; ops: d x c s y > <
  gu gU g~ = r? no; p paste (replace sel); U/u visual case? (defer P16).
- ACCEPT: motion updates selection; `Vd` deletes lines; `V>` indent block.
- depends: P5.2, P3.

### P5.5 Cmdline / Search prompt mode (UI-less state)
- goal: typed string state + history ring; on Enter emit ExRun/SearchRun Command.
- depends: P5.1.
- ACCEPT: history cycles; empty Enter no-op.
- (actual : parsing is P6; /? n N is P9.)

### P5.6 Event queue skeleton
- goal: MPSC `EventQueue` + a fake worker pushing events → App::Pump_assert_state
  drain in reducer tests.
- files: `app/event_queue.hpp/.cpp`, `app/events.hpp`.
- ACCEPT: multithreaded test (<=2 threads) posting 1000 events; drain preserves
  order per worker.
- edges: wake fd absent (pure queue usable without os event).

### P5.7 PHASE CHECK — scripted vim session
- goal: run PREVIEW §P5 script through the headless feeder; asserts final state.

## P6 — Ex commands (no screen yet)

Files: `core/ex_parse.hpp/.cpp`, `core/ex_engine.hpp/.cpp`, reducer glue in
`app/*.cpp`. Input entry via P5.5.

### P6.1 Ex parser
- goal: range grammar `%`, `.`, `$`, `N`, `N,M`, `.,+3`, bang, args, `cmd!`, cmd
  abbreviations (match longest uniq prefix).
- ACCEPT: parser table tests (incl. `:'<,'>s/x/y/g`).
- edges: ranges with count after cmd (`:s/x/y/g 3`); spaces.

### P6.2 File commands :w :wq :q :q! :qa :e :enew
- goal: file io via Buffer::save/load; confirm-unsaved flow (msg + `q!` override);
  `:e` reload from disk (keep cursor).
- ACCEPT: tests with temp dirs; `:w other.cpp` writes buffer copy? (no: retarget,
  vim sets buffer filename).
- depends: P1, P5.

### P6.3 Buffer list commands :ls :b name/:bN :bn :bp :bd
- goal: listing string for UI later; switch by num/partial-name; :bd closes buffer
  (auto-splice windows showing it; refuse w/o bang when modified).
- ACCEPT: list string format test; :bd unloads; windows collapse to nearest.
- (window wiring robust in P8.)

### P6.4 :<number> + :$ goto
- goal: pure range command → move cursor (col=first nonblank), clear pending.
- ACCEPT: `:3` on 2-line buffer → error msg state, no crash.

### P6.5 :set options
- goal: settings table (`tabwidth, number, relativenumber, scrolloff, expandtab,
  shiftwidth, clipboard, lsp, cmdheight…`) with parser `key=value`, `nokey`,
  booleans; invalid → error entry.
- ACCEPT: set/get tests; unknown key error; `:set ts?` prints.
- files: `core/settings.hpp/.cpp` (decided: full file), reducer hook.

### P6.6 :s/// (single line & range)
- goal: regex replace on `:'<,'>` or current line using P1.7; flags g i c I E;
  counts; cursor to last substitution; empty pattern uses last search (P9 filler).
- ACCEPT: tests; `c` flag pauses per match (test with injected chooser).
- depends: P1.7, P6.1.

### P6.7 :noh (and highlight model)
- goal: highlight state + clear (search itself in P9).
- ACCEPT: model state toggles in tests.

### P6.8 PHASE CHECK
- goal: ex e2e suite: parse→apply for every table row; msg texts asserted.

## P7 — Renderer (FIRST SCREEN)

Files: `ui/*`, wires `App::run()` in `main.cpp`. depends: P5 reducer, P6.

### P7.1 Screen init/teardown + Key reader
- goal: ncursesw init (noecho/cbreak/escdelay 25/keypad), colors start, `Key` reader
  from get_wch (esc vs csi resolve; bracketed paste support arrives in P15).
- ACCEPT: manual demo: keys echo to statusbar "key=…" line (removed after P7.4).
- edges: terminal resize (KEY_RESIZE → App::on_resize); Ctrl+J vs Enter; utf8 input.

### P7.2 View draw (single window)
- goal: draw buffer lines with horizontal scroll (long-line clip), tabs expanded,
  nonprinting, cursor cell, completion of `screen row↔line` mapping in TextView.
- ACCEPT: scripted UI test via write-to-buffer + manual visual check (PREVIEW §P7).
- depends: P7.1.

### P7.3 Gutter (numbers + relative)
- goal: line numbers/current highlight/relative via `scrolloff`-independent state;
  width from `len(lines)`; foldcolumn placeholder blank.
- ACCEPT: visual check; relative numbering correctness tests (pure part).

### P7.4 Statusline + messages + cmdline
- goal: per-window statusline (mode flag, filename+, %/Ln,Col), bottom cmdline
  prompt row (: / ?), messages area (echo with 3s expiry).
- ACCEPT: PREVIEW walkthrough; mode shown changes with i/V/:.

### P7.5 Main loop + cursors
- goal: App::run with poll(stdin+wake), drain events, reducer, render dirty;
  cursor placed at logical pos (curses move), scrolloff honored.
- ACCEPT: typing session via headless fixture then manual demo.
- depends: P7.2-4, P5.6.

### P7.6 Persistence of last position? (no title/tab bar yet)
- goal: undo restored cursor placement; blockvisibility. minimal; fold into P7.5 if
  small — else skip (task may be dropped before P7.9 if P7.5 covers it).
- ACCEPT: covered by P7.5 test.

### P7.7 Window frame borders/statusline fixup for N=1
- goal: single window fills screen except tabbar (tab bar itself lands in P8 — keep
  a 1-row placeholder now), statusline 1 row, cmdline 1 row.
- ACCEPT: PREVIEW.

### P7.8 Message/log plumbing (no :help)
- goal: `Messages` ring + statusbar echo + `:messages` dumps into a new scratch
  buffer (vim-like).
- ACCEPT: demo (echo + window shows lines).

### P7.9 PHASE CHECK — demo run
- goal: user demo per PREVIEW §P7; smoke test under sanitizers (`noni file.txt`).
- depends: all P7.

## P8 — Windows + tabs

Files: `core/layout.hpp/.cpp` (model), reducer + ui glue.

### P8.1 Layout split tree
- goal: binary tree model, focus id, split (`:sp`, `:vs`) with min sizes; close
  collapses; `:new` (horizontal, empty buffer).
- api: `Layout::split(WindowId, Dir); close(WindowId); equalize();`
- ACCEPT: model tests (sizes sum, min clamp, collapse of 2-panes).
- depends: P6.3.

### P8.2 ctrl+w prefix keys
- goal: h/j/k/l/w (focus), c (close), o (only), = (equalize), +/-/</> resize,
  s/v splits, single window map unchanged.
- ACCEPT: headless state asserts + UI check.

### P8.3 Windows render (multiple)
- goal: draw each window rect (P7.2 parametrized), active/inactive statusline
  (dimmed), separator lines, cursor into focused window only.
- ACCEPT: PREVIEW demo (:sp → two panes; type in focused).

### P8.4 :tabnew gt gT :tabs + tabbar
- goal: tabs array in EditorState (each with own Layout), `:tabnew` (+ `:tabe f`),
  gt/gT + `2gt`, count on :tabnew? (no), tabbar row rendering.
- ACCEPT: model tests + UI demo.

### P8.5 :sp :vs :new :only — ex hookup + focus routing
- goal: ex table rows call Layout; ensure focus follows new window (vim rule).
- depends: P8.1, P6.
- ACCEPT: ex tests.

### P8.6 :b switching updates active window only
- goal: `:b N` shows buffer in active window, other windows untouched; closing
  buffer hides panes gracefully.
- ACCEPT: state tests.

### P8.7 Scroll per window (independent views)
- goal: view state per Window; ctrl+d/u/f/b/e/y affect focused window only;
  horizontal scroll persists per window.
- ACCEPT: demo + state assert.

### P8.8 PHASE CHECK
- goal: demo: 2 splits + 2 tabs scripted PREVIEW §P8.

## P9 — Search + substitute (UI wiring)

### P9.1 / ? prompt flow
- goal: typed pattern → BufferSearch (literal unless `\v`? keep plain regex per
  P1.7), highlight all matches, cursor to first, direction persists (`n`/`N`).
- ACCEPT: PREVIEW script; no-match message; offset search stays.
- depends: P6.7, P5.5.

### P9.2 n N + last-search register
- goal: counts, `;`/`,` find-repeat interplay (P2.4 keeps separate state), wrap
  message once per wrap.
- ACCEPT: wrap test.

### P9.3 * and #
- goal: word under cursor → search (visual selection variant too), strict word
  boundary.
- ACCEPT: tests + demo.

### P9.4 Highlights + :noh wiring
- goal: highlight rendering in view layer (reverse video pairs), :noh clears until
  next search.
- ACCEPT: demo.

### P9.5 :s/// with confirm UI + across mode
- goal: wire P6.6 to interactive `c` (y/n/a/q/l) cmdline prompt; Visual-mode `:s`
  pre-filled `'<,'>`.
- ACCEPT: demo; a/q abort correctness.

### P9.6 :%s///g + flash messages ("N substitutions on M lines")
- ACCEPT: messages + counts tested.

### P9.7 PHASE CHECK
- goal: PREVIEW §P9 script.

## P10 — Fuzzy picker + explorer

### P10.1 FileScan worker
- goal: worker walks cwd (gitignore-aware via `git check-ignore` batch), posts
  FileScanDone list; debounce re-scan (P10.6 refresh hook — polling now, fs
  events later if needed).
- files: `app/workers.*`; no third-party.
- ACCEPT: fake-tree fixture list correct incl. skip rules.
- depends: P5.6.

### P10.2 Generic list popup UI
- goal: centered popup (list/hl/filter-as-you-type/enter/esc/ctrl+j/k); reused for
  buffers/lsp/git later.
- ACCEPT: PREVIEW demo with tmp files.

### P10.3 <space>ff fuzzy file open
- goal: fuzzy scorer (subsequence + penalty), open into active window or split on
  ctrl+s? (vim prefers no default splits → `ctrl+x` hint? keep: enter only; ctrl+t? tab later).
- ACCEPT: demo; empty query shows recents? (P17).

### P10.4 <space>fg project grep
- goal: typed pattern → worker grep over scanned files (P10.1 list, batched by
  size), results in list popup (file:line:col + preview), Enter opens at match,
  `:` cycles matches in same file; Esc closes.
- files: `app/workers.*`, popup reuse `ui/popup.*`, reducer additions.
- api: `Job grep_project(sv pattern) -> id;` event `GrepDone{hits{path, line, col, text}}`.
- ACCEPT: fake tree tests; binary/large file skipping; open-at-match correctness.
- edges: no matches → "no matches" msg; huge repos → batched arrival, popup shows
  count-so-far.
- depends: P10.1, P10.2, P9 highlights.

### P10.5 Explorer buffer (:E / <space>e)
- goal: netrw-style buffer listing cwd tree, dirs `/`-suffixed, `Enter` open,
  `-` go up, `o` split-open, `hjkl` move/collapse? minimal: Enter/h.
- ACCEPT: demo + state tests (explorer buffers flagged unmodifiable).
- depends: P10.1 (uses scan events), P8 (split-open).

### P10.6 Explorer refresh + reveal current file
- goal: refresh (`R`), reveal active buffer path (`gf`? use `:Lcd`? minimal: `gf`
  in explorer jumps), cursor memory.
- ACCEPT: demo.

### P10.7 :e file completion
- goal: cmdline `:e <partial>` popup listing matches tab-complete (cmdline hook).
- ACCEPT: demo.

### P10.8 PHASE CHECK
- goal: PREVIEW §P10 script incl. <space>fg grep demo.

## P11 — Config (nlohmann/json)

## P11 — Config (nlohmann/json)

### P11.1 Settings merge from config.json
- goal: load `~/.config/noni/config.json` → cwd → exe dir (first found); schema =
  §3; unknown keys warn (once) not fail; missing file = defaults.
- files: `app/config.hpp/.cpp`.
- ACCEPT: fixture configs tested; malformed JSON → error + defaults + msg.
- depends: P6.5.

### P11.2 Apply + live :set effect
- goal: changes to tabwidth/number/relativenumber/scrolloff/expandtab hit render
  instantly (no restart).
- ACCEPT: demo (:set rnu → gutter flips).

### P11.3 lsp/terminal/clipboard config sections
- goal: parse placeholders for P13/P15 (servers, shell). No behavior yet.
- ACCEPT: parse tests.

### P11.4 PHASE CHECK — config e2e
- goal: all above shown in PREVIEW §P11.

## P12 — Syntax highlighting (tree-sitter)

### P12.1 Tree-sitter skeleton
- goal: pkg-config link; parser per language c/cpp/python/rust; embed minimal
  highlight queries (keywords/strings/comments/numbers/functions/types).
- files: `syntax/*`, `syntax/queries/*.scm` (embedded strings preferable; no files).
- ACCEPT: fixture file token spans asserted.
- depends: P0.2, P11.1? no — depends P0 only + language detect (P7 with extension).

### P12.2 Async reparse worker
- goal: worker receives snapshot + changed span, parses, posts token ranges;
  visible-window priority; debounce 50ms.
- ACCEPT: edit→highlight latency test (fixture).

### P12.3 Renderer colors
- goal: token → color pair table (theme), fallback plain; `:TSOn/Off`? no — always
  on; graceful when grammar missing (plain).
- ACCEPT: PREVIEW visual.

### P12.4 Language detection + extension map
- goal: filename→lang table; unknown → plain text (fallback lexer P12.5).
- ACCEPT: tests.

### P12.5 Fallback lexer (minimal)
- goal: regex-lite fallback (keywords/str/comment/num) for other exts.
- ACCEPT: fixture asserts.

### P12.6 PHASE CHECK
- goal: PREVIEW §P12 (highlight visible + fast scroll).

## P13 — LSP

Files: `lsp/*`, glue in app/notify + reducer. All I/O worker-side (jsonrpc worker
owns pipes).

### P13.1 JSON-RPC framing + protocol types
- goal: Content-Length framing, serialize/parse via nlohmann/json; typed structs
  for Position/Range/TextEdit/Diagnostic/CompletionItem/Hover.
- ACCEPT: fuzz-ish encode/decode tests incl. unicode offsets (UTF-16 cols!).
- depends: P11.3 (json lib present).

### P13.2 LspProcess (pipes + worker)
- goal: spawn from config `lsp.servers[].command`; worker loop: read stdout →
  decode → post LspReply/LspDiag events; mailbox accepts send requests (never
  blocking main).
- ACCEPT: fake server script (python) talks request/response.
- depends: P13.1, P5.6.

### P13.3 Session lifecycle
- goal: per BufferId doc state; initialize (root via config rootMarkers),
  didOpen/Change (rev throttling)/Save/Close; restart on crash; status msgs.
- ACCEPT: fake-server lifecycle test incl. pending-change queue flush.
- depends: P13.2, notify path.

### P13.4 didChange sync (full + incremental)
- goal: incremental ranges via UTF-16 mapping (text.hpp helpers); full fallback on
  mismatch.
- ACCEPT: round-trip tests (server echo).

### P13.5 Completion UI
- goal: trigger on auto (insert mode idle 150ms + trigger chars) and manual
  (`<c-space>` in insert); popup (P10.2), `<c-n>/<c-p>/<cr>`, item detail preview.
- ACCEPT: fake server; popup nav tests; insert applies edits (textEdit ranges!).

### P13.6 Hover (K)
- goal: popup at word; plain text + limited markdown (code spans) render; dismiss
  on move.
- ACCEPT: demo with fake server.

### P13.7 Goto def/decl/impl + references
- goal: jump when 1 hit (jumplist push), else picker; `<space>gr` refs list with
  preview-on-enter.
- ACCEPT: fake server tests + demo.

### P13.8 Rename
- goal: `:Rename new` or `<space>rn` prompt via cmdline; apply WorkspaceEdit +
  fix unsaved buffers prompt.
- ACCEPT: fake server; undo restores all files.

### P13.9 Diagnostics
- goal: LspDiag events → per-buffer diag set (version gated); gutter signs `E/W`,
  hover mark, `:Diag` list, `[d ]d`, underline/highlight of current; stale drop.
- ACCEPT: fake server tests + demo.

### P13.10 Formatting
- goal: `:Format` (+ optional format-on-save setting opt-in OFF default); async
  apply with undo grouping; conflict check.
- ACCEPT: fake server; rapid saves cause no freeze (latency test).

### P13.11 PHASE CHECK
- goal: run real clangd demo PREVIEW §P13 (file present), sanitizer run.

## P14 — Git

### P14.1 Git worker plumbing
- goal: `git` CLI runner in worker (cwd, timeouts, no env leaks); post GitStatus/
  GitDiff events; debounced repo-root detect.
- ACCEPT: fixture repo tests.
- depends: P5.6.

### P14.2 Status + branch
- goal: porcelain parse; status bar branch + dirty count leader panel `:Gst`.
- ACCEPT: fixture repo state asserts.

### P14.3 Diff → gutter signs
- goal: `diff -U0` parse per file; sign set in gutter (`+ ~ -`), refresh debounced
  on buffer change (notify path) + on save.
- ACCEPT: edit→sign latency test; undo→signs revert.

### P14.4 Stage/unstage/discard
- goal: `:Git add/restore -- <file>` wrappers + confirm; executable from status
  pane + `:Gw` current file shortcut? (vim-fugitive: `:Gwrite`, mapped `<space>ga`).
- ACCEPT: fixture repo file staged visible in :ls pane.

### P14.5 Git status pane (fugitive-like)
- goal: `:Gst` opens pane buffer listing status (staged/unstaged), `space` toggle
  stage, `Enter` open file diff? (P14.6), `-` discard? (with confirm).
- ACCEPT: demo (PREVIEW §P14).

### P14.6 PHASE CHECK
- goal: demo repo ops scripted, sanitizers clean.

## P15 — Terminal

### P15.1 PTY spawn + TermOutput stream
- goal: forkpty child (shell from config); terminal worker owns fd, buffers to
  scrollback (cap 5000), posts chunked TermOutput events.
- ACCEPT: echo plumbing to fake main; resize propagation.
- depends: P5.6.

### P15.2 VT grid parser (subset)
- goal: grid of cells (cp, style) incl. CSI cursor moves, SGR colors, erase ops,
  scroll, OSC title; no sixel/images; app-mode cursor keys toggle.
- files: `terminal/vt.hpp/.cpp`.
- ACCEPT: golden-sequence tests (vim-in-term fixture!).
- depends: P15.1.

### P15.3 :term + pane
- goal: `:term` opens bottom split (config height) with grid render; `i` focus
  mode, Esc leaves; ctrl+w window keys work above.
- ACCEPT: demo: run `bash`, `ls`, `exit`.
- depends: P15.2, P8.

### P15.4 Terminal key input + copy mode
- goal: raw key forwarding (arrows per DECCKM, F1-12, paste as bracketed),
  scrollback nav with `k/j/ctrl+f/b` in normal-ish copy mode (ESC from focus).
- ACCEPT: demo (scrollback navigation; clipboard comes in P15.6).

### P15.5 :! cmd + `:!cmd %`
- goal: run via worker, stream output into scratch `:!out` buffer, `%` expand.
- ACCEPT: demo; hangs/timeouts kill process (100ms issues gone).

### P15.6 Clipboard via OSC52 + provider
- goal: yank/paste through `+` register; OSC52 to terminal, OSC52 paste request;
  optional wl-copy provider in config (worker).
- ACCEPT: demo in supported terminal; fallback correctness when absent.

### P15.7 PHASE CHECK
- goal: PREVIEW §P15 script (vim inside vim terminal etc.).

## P16 — Extras (vim polish)

### P16.1 VisualBlock (ctrl+v)
- goal: block selection on Window; ops `d x y c I A` (insert per line), paste of
  block; visual replace (r).
- ACCEPT: golden tests + demo.

### P16.2 :map/:nnoremap/:iunmap
- goal: simple rebind table over key sequences (no when-clauses), `:map` list,
  error on recursion.
- ACCEPT: tests + demo (`:map <C-x> :w<cr>`).

### P16.3 J polish + gJ
- goal: J removes trailing comment line-awareness (simple), `gJ` no space,
  `[count]J`.
- ACCEPT: tests.

### P16.4 Text-visual ops: u/U in visual + case
- goal: visual `u U ~`.
- ACCEPT: tests.

### P16.5 zz polish + ' sidescroll + wrap option? (defer wrap=off only)
- goal: `zcr`/`zs`, horizontal `z.` corrections; wrap stays off (v1 only).
- ACCEPT: visuals.

## P17 — Session / recovery / autosave

### P17.1 Autosave afterDelay
- goal: tick in main loop, writes dirty buffers (never explorer/scratch), conflict
  re-check by mtime before write, skip recovery-conflict pops.
- ACCEPT: mtime-conflict test.

### P17.2 Crash recovery snapshots
- goal: on dirty-change debounce write `.noni/recovery/<buf>` per session; remove
  on save/close; startup: detect + prompt restore-or-open (`y/n` list).
- ACCEPT: kill -9 fixture then start → prompt restores.

### P17.3 Session state
- goal: on exit (any :qa) save open files/paths/cursor/window layout/tabs to
  `~/.local/state/noni/session.json` (XDG); `noni` plain start restores? no — CLI
  `noni --restore` or `:Sload`; default off (vim-natural behavior).
- ACCEPT: fixture session roundtrip.

### P17.4 Recent files
- goal: MRU list persisted; `<space>fr` shows in popup; counts as recent.
- ACCEPT: test.

### P17.5 Startup flags
- goal: `noni [file]` opens; `noni --restore` load session; `noni --version`.
- ACCEPT: manual.

### P17.6 Fsync + atomic save everywhere
- goal: temp file + fsync + rename for buffer save + recovery + session.
- ACCEPT: crash-mid-write fixture still leaves old content.

### P17.7 PHASE CHECK + final acceptance
- goal: PREVIEW final script (all sections), full warning/sanitizer pass, tag
  `v1.0-alpha` tag pushed.
