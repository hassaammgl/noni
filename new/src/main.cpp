// P0.3 — CLI skeleton only. The real App loop arrives in P7.5; the editor UI
// turns on in P7.1. No ncurses here yet by design.
#include <cstdio>
#include <print>
#include <string_view>

#include "core/version.hpp"

int main(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg = argv[i];
        if (arg == "--version") {
            std::println("{}", noni::version());
            return 0;
        }
        std::println(stderr,
                     "noni: unknown argument '{}' — UI arrives in P7", arg);
        return 2;
    }
    // No args, no UI yet: informational success (bare `make run` must not look
    // like a failure until P7 gives the editor a real screen).
    std::println("noni v0.1.0 — editor UI arrives in P7 (`noni --version` works)");
    return 0;
}
