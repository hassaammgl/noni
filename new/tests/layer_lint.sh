#!/usr/bin/env bash
# P0.2 layer lint — runs as a ctest at the merge gate (AGENTS: ctest is the gate).
# Hard rules today: ncurses headers/calls exist ONLY in ui/;
# tree-sitter usage exists ONLY in syntax/.
# (Compile-time include scoping is best-effort only: this toolchain puts
# ncurses.h on the default /usr/include path, so a wrong include would still
# compile — see ROADMAP P0.2 edge note.)
set -eu

fail=0
out="$(grep -rn -E '<curses\.h>|<ncurses|<panel\.h>|<menu\.h>|<form\.h>' src/ || true)"
if [ -n "$out" ]; then
    bad="$(printf '%s\n' "$out" | grep -v '^src/ui/' || true)"
    if [ -n "$bad" ]; then
        echo "ERROR: ncurses usage outside src/ui/:" >&2
        printf '%s\n' "$bad" >&2
        fail=1
    fi
fi

out="$(grep -rn -E 'tree_sitter|tree-sitter' src/ || true)"
if [ -n "$out" ]; then
    bad="$(printf '%s\n' "$out" | grep -v '^src/syntax/' || true)"
    if [ -n "$bad" ]; then
        echo "ERROR: tree-sitter usage outside src/syntax/:" >&2
        printf '%s\n' "$bad" >&2
        fail=1
    fi
fi

exit "$fail"
