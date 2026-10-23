# ARCHITECTURE.md — Noni v2 (new/)

Short, concrete, stable. Anything not written here gets decided in a ROADMAP task,
not ad-hoc in code.

v1 scope note (revised 2026-10-23, v2): **git integration is removed** (no git
module, worker, events, signs, or CLI use anywhere). Everything else stays:
built-in terminal (`terminal/`: PTY + VT parser + `:term` + copy mode), `:!cmd`
shell jobs, OSC52 clipboard write (async, no subprocesses), LSP at **P13**
(isolated: nothing outside P13 depends on it), syntax highlighting, config,
session/recovery/autosave, extras.

## 1. Layers (strict, compile-enforced by CMake target dependency order)

```
┌──────────────────────────────────────────────────────────────────┐
│ app/   App: owns ALL editor state, event loop, reducer           │
│        one main thread. Knows core + services + ui.              │
├───────────────────────────────┬──────────────────────────────────┤
│ ext/                          │ syntax/                          │
│ `:!cmd` shell jobs            │ tree-sitter highlight            │
│ (short-lived worker jobs)     │ parses ON THE MAIN THREAD        │
│                               │ (visible range; optional worker  │
│ Background work in v1:        │ comes later)                     │
│ file scan + grep (`app/jobs`) │                                  │
│ + `:!` shell jobs             │ terminal/  PTY + VT grid +       │
│ + OSC52 clip writes           │ copy mode; dedicated PTY worker  │
│                               │                                  │
│                               │ lsp/  P13 (isolated: nothing     │
│                               │ outside P13 depends on it)       │
├───────────────────────────────┴──────────────────────────────────┤
│ core/  Buffer + undo, text primitives, motions, operators,       │  pure logic,
│        registers, marks, jumps, search, ex parser, config        │  no I/O, no
│        model, layout model, keymap data                          │  threads, 100%
├──────────────────────────────────────────────────────────────────┤   unit-tested
│ ui/    ncurses only: draw views, read keys, popups               │
└──────────────────────────────────────────────────────────────────┘
```

Rules:
- `core/` includes nothing above it. Fully unit-testable, no ncurses, no threads.
- `ui/` has zero editing logic: it reads state, draws, reads keys, returns keys.
- `ext/ syntax/ terminal/ lsp/` never include `app/`, `ui/`, or `<ncurses.h>`.
- `syntax/` runs on the main thread: incremental reparse of the visible range only
  (`ts_tree_edit` + parse of visible lines). It must be deterministic and fast
  (<a few ms for a screenful). An optional off-main worker may be introduced later
  WITHOUT changing the interface (snapshot in → token spans out).
- The main thread owns `App` (all mutable editor state). Background execution is
  limited to: a shared job pool (file scan, grep, `:!` jobs, OSC52 writes) and
  the dedicated terminal PTY worker (one thread, owns the master fd).

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
      register.hpp/.cpp         # registers (+ OSC52 backend hook)
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
      events.hpp                # Event variant (FileScanDone/GrepDone/Shell...)
      jobs.hpp/.cpp             # job pool: file scan, grep, `:!` shell jobs
      reducer.hpp/.cpp          # keys -> Command -> apply (ONE place)
      commands.hpp              # Command variant definitions
      app.hpp/.cpp              # App: loop, owns state, fans out notifications
      config.hpp/.cpp           # config.json load (nlohmann/json)
      notify.hpp/.cpp           # buffer-change fan-out (ONE explicit path)
    ui/                         # ncurses renderer (static lib)
      screen.hpp/.cpp           # init/teardown, size, color pairs
      view.hpp/.cpp             # draw one window's text area
      gutter.hpp/.cpp           # line numbers + (optional LSP) diag signs
      statusline.hpp/.cpp       # statusline + cmdline + messages
      tabbar.hpp/.cpp           # tab bar
      popup.hpp/.cpp            # completion/hover/picker popups
      explorer_view.hpp/.cpp    # explorer pane rendering
      input.hpp/.cpp            # get_wch -> Key
    ext/                        # :! shell jobs (static lib)
      shell_job.hpp/.cpp        # spawn, stream output, timeout kill, % expansion
      clipboard.hpp/.cpp        # OSC52 encode + async write (on the job pool)
    terminal/                   # built-in terminal (static lib)
      pty.hpp/.cpp              # forkpty + master fd RAII
      vt.hpp/.cpp               # VT grid parser (CSI/SGR/erase/scroll/OSC)
      worker.hpp/.cpp           # dedicated PTY worker loop
    syntax/                     # tree-sitter (static lib, MAIN-thread use)
      highlight.hpp/.cpp        # incremental visible-range parse
      fallback_lexer.hpp/.cpp   # regex-lite fallback for unknown languages
    lsp/                        # P13; jsonrpc + client (static lib)
      jsonrpc.hpp/.cpp          # Content-Length framing
      lsp_client.hpp/.cpp       # process, mailbox, request/reply (worker-side)
      lsp_types.hpp             # Position/TextEdit/Diagnostic/... types
  tests/                        # doctest, core tests (+ nvim differential tests)
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
                          // ExRun, SplitWindow, PickFile, ShellJob, ...
    std::uint32_t count = 0;                  // pending count, 1 = default
    Arg     arg;                              // std::variant below
};
using Arg = std::variant<std::monostate, char32_t,        // r, f, t, "r
                         Motion, TextObject, Operator,    // composed moves/ops
                         std::string,                     // ex line, file name
                         Reg, Mark>;                      // register/mark refs
```

- Pipeline: key → (keymap tables) → `Command` → `reducer(App&, Command)`.
- During multi-key sequences (operators, `f<char>`, `q`), the editor holds a
  small `Pending` state inside the mode handler; when complete it emits one Command.
- Ex commands parse to `ExCmd` values and are applied by the same reducer.
- `:!cmd` becomes a `ShellJob` Command: reducer asks `ext/` to spawn; output
  arrives as events into a scratch buffer.

## 5. Event queue + threading rules (app/event_queue.hpp, jobs.hpp)

```cpp
struct Event {                 // std::variant payload + routing id
    std::uint64_t req;         // request id for stale-result filtering
    std::variant<FileScanDone, GrepDone, ShellOutput, ShellExit, TermOutput,
                 TermExit, ClipDone, LspReply, LspDiag, JobError> data;
};
class EventQueue {  /* MPSC: job threads push, App drains */ };
```

- **Main thread** owns `App` (all editor state). It: polls stdin + wake fd,
  reads keys → reducer; drains `EventQueue` between keys; ticks autosave/recovery
  timers each loop.
- **Jobs** (file-scan, grep, `:!` short-lived child processes, OSC52 tty writes)
  run on a shared 2-thread job pool in `app/jobs`. The terminal PTY worker
  (`terminal/worker`) is a separate dedicated thread that owns the master fd.
  They own their fds during a job and emit results ONLY via
  `EventQueue::post()`.
- NO blocking calls on the main thread. Job pool threads never touch editor state,
  never call ncurses, never hold editor locks.
- No pointer-to-editor-state ever crosses a thread. Jobs receive copies /
  immutable snapshots (`std::shared_ptr<const BufferSnapshot>`); `:!` streams
  output chunks as `ShellOutput` events and reports exit status via `ShellExit`.
- Every async op gets `req = next_request_id++`; stale events (req mismatch,
  buffer revision mismatch) are dropped in the reducer.
- Wake: eventfd (or pipe); job threads also write one byte after posting so poll wakes.

## 6. Buffer change notification (ONE explicit path)

Buffer internals do NOT notify anyone. After the reducer applies an edit it calls:

```cpp
// app/notify.hpp — the only place fan-out happens
void App::post_buffer_change(BufferId id, const ChangeInfo& info);
```

`ChangeInfo { first_line, old_count, new_count, revision }`. Subscribers (registered
in `App` ctor, plain `move_only_function` list):
1. syntax → incremental visible-range reparse ON the main thread (no job);
2. LSP → queue didChange for that BufferId (P13 subscriber; inert when no server);
3. recovery → mark buffer dirty for next snapshot tick.

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
- `<space>` chords are plain nested tables; unfilled chords are cancelled by Esc.
- v1 leader chords: `<space>ff` file picker, `<space>fg` grep, `<space>e`
  explorer, `<space>fr` recent files. LSP-only chords (`<space>gd/gr/rn`) come
  with P13 and are inert while no server is attached.

## 8. "Rules for future code" checklist

Before adding any module, verify:
- [ ] Lives in the right layer; layer includes checked (CMake enforces order).
- [ ] No `_partN`; one `.hpp/.cpp` pair; public header first + doctest first.
- [ ] All state mutations go through the reducer — no direct pokes from ui/widgets.
- [ ] References buffers/windows by id; owners hold pointers privately.
- [ ] Any background I/O: job-pool task + Event + req id; main thread never blocks.
- [ ] Any reaction to edits: subscriber in `post_buffer_change` only.
- [ ] New keys: one row in the mode table (+ tests for counts/macros interplay;
      differential tests vs `nvim --headless` for motion/operator/text-object keys).
- [ ] New ex command: one row in `ex_engine` table (name, bang, range, fn).
- [ ] No exceptions anywhere (see AGENTS.md); error paths return `eresult`.
- [ ] No raw new/delete, no singletons; C-library resources wrapped in RAII
      (WINDOW deleter, FILE, fds, tree-sitter deallocators).
- [ ] `ctest` green, sanitizers clean on the demo path.
