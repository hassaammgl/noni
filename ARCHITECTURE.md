# ARCHITECTURE.md — Noni v2 (new/)

Short, concrete, stable. Anything not written here gets decided in a ROADMAP task,
not ad-hoc in code.

## 1. Layers (strict, compile-enforced by CMake target dependency order)

```
┌────────────────────────────────────────────────────────────┐
│ app/   App: owns ALL editor state, event loop, reducer     │
│        one main thread. Knows core + all services + ui.    │
├───────────────┬───────────────┬─────────────┬──────────────┤
│ lsp/          │ git/          │ syntax/     │ terminal/    │  background
│ jsonrpc client│ git CLI worker│ tree-sitter │ PTY + VT     │  services
│ own IO worker │ own worker    │ own worker  │ own worker   │
├───────────────┴───────────────┴─────────────┴──────────────┤
│ core/  Buffer + undo, text primitives, motions, operators, │  pure logic,
│        registers, marks, jumps, search, ex parser, config  │  no I/O, no
│        model, layout model, keymap data                    │  threads, 100%
├────────────────────────────────────────────────────────────┤   unit-tested
│ ui/    ncurses only: draw views, read keys, popups         │
└────────────────────────────────────────────────────────────┘
```

Rules:
- `core/` includes nothing above it. Fully unit-testable, no ncurses, no threads.
- `ui/` has zero editing logic: it reads state, draws, reads keys, returns keys.
- `lsp/ git/ syntax/ terminal/` never include `app/`, `ui/`, or `<ncurses.h>`;
  they talk to the app only through `EventQueue` + plain request structs.
- The main thread owns `App` (all mutable editor state). Everything else is workers.

## 2. Directory tree (`new/`)

```
new/
  CMakeLists.txt
  src/
    core/                       # pure logic (static lib)
      pos.hpp                   # Pos{line,col}, BufferId/WindowId, Error
      text.hpp/.cpp             # UTF-8/codepoint helpers, word classes
      buffer.hpp/.cpp           # lines + edits (no undo)
      undo.hpp/.cpp             # transactions, undo/redo stack
      buffer_search.hpp/.cpp    # literal + regex find/replace
      motion.hpp/.cpp           # Motion enum + resolution
      operator.hpp/.cpp         # operator + text-object engine
      register.hpp/.cpp         # registers, paste kinds
      marks.hpp/.cpp            # marks + jump list
      macro.hpp/.cpp            # recorded key sequences
      ex_parse.hpp/.cpp         # ex command parser (pure)
      ex_engine.hpp/.cpp        # ex command table (data)
      settings.hpp/.cpp         # :set model (tabwidth, number, ...)
      keymap.hpp/.cpp           # mode keymap tables (plain data)
      layout.hpp/.cpp           # window split tree + tab list (model only)
      editor_state.hpp/.cpp     # EditorState struct the reducer works on
    app/                        # orchestrator (static lib)
      event_queue.hpp/.cpp      # MPSC queue + wake
      events.hpp                # Event variant (LspRow/GitRow/TermRow/...)
      workers.hpp/.cpp          # worker threads (own their fds/loops)
      reducer.hpp/.cpp          # keys -> Command -> apply (ONE place)
      commands.hpp              # Command variant definitions
      app.hpp/.cpp              # App: loop, owns state, fans out notifications
      config.hpp/.cpp           # config.json load (nlohmann/json)
      notify.hpp/.cpp           # buffer-change fan-out (ONE explicit path)
    ui/                         # ncurses renderer (static lib)
      screen.hpp/.cpp           # init/teardown, size, color pairs
      view.hpp/.cpp             # draw one window's text area
      gutter.hpp/.cpp           # line numbers + git/diagnostic signs
      statusline.hpp/.cpp       # statusline + cmdline + messages
      tabbar.hpp/.cpp           # tab bar
      popup.hpp/.cpp            # completion/hover/picker popups
      explorer_view.hpp/.cpp    # explorer pane rendering
      input.hpp/.cpp            # get_wch -> Key
    lsp/    jsonrpc.hpp/.cpp  lsp_client.hpp/.cpp  lsp_types.hpp
    git/    git_worker.hpp/.cpp  git_types.hpp
    syntax/ highlight.hpp/.cpp  fallback_lexer.hpp/.cpp  queries/ (embedded .c)
    terminal/ pty.hpp/.cpp  vt.hpp/.cpp  terminal_worker.hpp/.cpp
  tests/                        # doctest, core tests (+ snippet-run tests)
  main.cpp                      # only file that touches app+ui together
```

## 3. Fundamental types (core/pos.hpp, text.hpp)

```cpp
using BufferId = std::uint32_t;   // 0 = invalid; assigned by app
using WindowId = std::uint32_t;
struct Pos   { std::uint32_t line = 0, col = 0; };   // col = CODEPOINT index
struct Error { std::string msg; };
template<class T> using eresult = std::expected<T, Error>;
```

- Buffer text = `std::vector<std::string>` (UTF-8 bytes); positions are
  `(line, codepoint index)`. Never store cursors as raw byte offsets outside Buffer.
- One window shows one buffer at one cursor. Cursor state lives in the Window
  (vim semantics), undo lives in the Buffer.

## 4. Command type (app/commands.hpp)

Every user action becomes a `Command` value; the reducer applies it. No UI code
mutates state directly.

```cpp
struct Command {
    Action  action;       // enum: Move, InsertChar, DeleteOp, YankOp, EnterInsert,
                          // ExRun, SplitWindow, PickFile, SendTerminal, ...
    std::uint32_t count = 0;                  // pending count, 1 = default
    Arg     arg;                              // std::variant below
};
using Arg = std::variant<std::monostate, char32_t,        // r, f, t, "r
                         Motion, TextObject, Operator,    // composed moves/ops
                         std::string,                     // ex line, file name
                         Reg, Mark>;                      // register/mark refs
```

- Pipeline: key → (keymap tables) → `Command` (with counts/operators already
  composed where the key sequence determines them) → `reducer(App&, Command)`.
- During multi-key sequences (operators, `f<char>`, `q`), the editor holds a
  small `Pending` state inside the mode handler; when complete it emits one Command.
- Ex commands parse to `ExCmd` values and are applied by the same reducer.

## 5. Event queue + threading rules (app/event_queue.hpp, workers.hpp)

```cpp
struct Event {                 // std::variant payload + routing id
    std::uint64_t req;         // request id for stale-result filtering
    std::variant<LspReply, LspDiag, GitStatus, GitDiff, GrepDone,
                 FileScanDone, TermOutput, TermExit, ClipDone, JobError> data;
};
class EventQueue {  /* MPSC: workers push, App drains */ };
```

- **Main thread** owns `App` (all editor state). It: polls stdin + wake fd,
  reads keys → reducer; drains `EventQueue` between keys; ticks autosave/recovery
  timers each loop.
- **Workers** (one per service: LSP, git, grep/file-scan, terminal PTY, clipboard)
  own their fds and their own `poll()` loop. They accept work via per-worker
  mailbox (mutex), emit results ONLY via `EventQueue::post()`.
- NO blocking calls on the main thread. No exceptions: no `pthread_join` on stop,
  no subprocess `waitpid` loops, no pipe writes that can block (workers handle EAGAIN).
- No pointer-to-editor-state ever crosses a thread. Workers receive copies /
  immutable snapshots (`std::shared_ptr<const BufferSnapshot>`).
- Every async op gets `req = next_request_id++`; stale events (req mismatch,
  buffer revision mismatch) are dropped in the reducer.
- Wake: eventfd (or pipe); workers also write one byte after posting so poll wakes.

## 6. Buffer change notification (ONE explicit path)

Buffer internals do NOT notify anyone. After the reducer applies an edit it calls:

```cpp
// app/notify.hpp — the only place fan-out happens
void App::post_buffer_change(BufferId id, const ChangeInfo& info);
```

`ChangeInfo { first_line, old_count, new_count, revision }`. Subscribers (registered
in `App` ctor, plain `move_only_function` list):
1. syntax → post reparse job (visible range) to syntax worker;
2. LSP → queue didChange for that BufferId (if a session exists);
3. git → invalidate gutter diff for that file (debounced re-run);
4. recovery → mark buffer dirty for next snapshot tick.

Rule for future code: **any new feature reacting to edits must be added as a
subscriber here, never by hooking Buffer internals.**

## 7. Keymap table (core/keymap.hpp — plain data, ONE place)

```cpp
struct Binding { Key key; Action action; Arg arg; };
constexpr inline std::array kNormalBindings = {   // FULL table, one place
    Binding{Key{'h'}, Action::Move, Motion{MotionId::Left}}, ...
};
constexpr inline std::array kInsertBindings  = { ... };
constexpr inline std::array kVisualBindings  = { ... };  // char/line/block merged
constexpr inline std::array kLeaderBindings  = {         // <space> IDE chords
    Binding{Key{'f'}, ... seq ff -> Action::PickFile, ...}, ...
};
```

- No when-clauses, no JSON keybinding layer. Mode + small per-mode state decides
  which table is active. ctrl+w window keys handled by `WindowKeymap` within Normal.
- Counts and operator+motion composition live in the Normal/Visual dispatcher
  (`Pending` state), not in every binding row.
- `<space>` chords are plain nested tables; unfilled chords are cancelled by Esc/Space.

## 8. "Rules for future code" checklist

Before adding any module, verify:
- [ ] Lives in the right layer; layer includes checked (CMake enforces order).
- [ ] No `_partN`; one `.hpp/.cpp` pair; public header first + doctest first.
- [ ] All state mutations go through the reducer — no direct pokes from ui/widgets.
- [ ] References buffers/windows by id; owners hold pointers privately.
- [ ] Any background I/O: worker + Event + req id; main thread never blocks.
- [ ] Any reaction to edits: subscriber in `post_buffer_change` only.
- [ ] New keys: one row in the mode table (+ tests for counts/macros interplay).
- [ ] New ex command: one row in `ex_engine` table (name, bang, range, fn).
- [ ] No raw new/delete, no singletons, no exceptions across layers.
- [ ] `ctest` green, sanitizers clean on the demo path.
