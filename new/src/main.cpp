// P2.1 — CLI frontend. `--keys` is the P2.1 debug harness (bottom-row key-code
// line, removed after P2.4); the real App loop with file rendering arrives at
// P2.5. No args still stays an informational stub until then.
#include <cstdio>
#include <print>
#include <string_view>

#include "app/reducer.hpp"
#include "core/editor_state.hpp"
#include "core/version.hpp"
#include "ui/ui.hpp"

int main(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg = argv[i];
        if (arg == "--version") {
            std::println("{}", noni::version());
            return 0;
        }
        if (arg == "--keys") { // TEMP P2.1..P2.4 demo harness (removed at P2.5)
            auto screen = noni::ui::Screen::open();
            if (!screen) {
                std::println(stderr, "noni: {}", screen.error().msg);
                return 1;
            }
            noni::EditorState st;
            noni::ui::KeyReader reader{screen->win()};
            for (;;) {
                noni::ui::draw_layout(*screen, st);
                const auto k = reader.next();
                if (!k) {
                    std::println(stderr, "noni: {}", k.error().msg);
                    return 1;
                }
                screen->debug_key(*k); // TEMP: bottom-row key codes until P2.5
                if (*k == noni::Key{noni::KeyKind::Char, U'q'}
                    && st.mode == noni::Mode::Normal) {
                    break; // temporary exits; real :q arrives with P6
                }
                noni::app::reducer(st,
                                   noni::app::command_for_key(st, *k));
            }
            return 0;
        }
        std::println(stderr,
                     "noni: unknown argument '{}' — UI arrives in P2", arg);
        return 2;
    }
    // No args: informational success (the screen arrives across P2.1..P2.5).
    std::println("noni v0.1.0 — editor UI arrives in P2 (`--version` works)");
    return 0;
}
