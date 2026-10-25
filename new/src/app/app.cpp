// P2.5 — see app.hpp.
#include "app/app.hpp"

#include <unistd.h>

#include "ext/ext.hpp"
#include "lsp/lsp.hpp"
#include "syntax/syntax.hpp"
#include "terminal/terminal.hpp"

namespace noni::app {

bool app_ready() {
    return ext::shell_ready() && terminal::pty_ready()
        && syntax::highlight_ready() && lsp::client_ready();
}

void App::load_file(const std::filesystem::path& path) {
    if (path.empty()) {
        return; // bare `noni`: the default empty buffer stays
    }
    const auto id = st_.open_file(path);
    if (id) {
        st_.win().buffer = *id;
    } else {
        st_.msg = "noni: " + path.string() + ": " + id.error().msg;
        st_.mark_dirty();
    }
}

void App::on_resize() {
    renderer_.on_screen_resize();
    // Re-clamp cursor + viewport to the new terminal size.
    resize_clamp(st_, renderer_.view_rows());
    render_pending_ = true;
}

void App::render_if_dirty() {
    if (!render_pending_ && !st_.needs_render()) {
        return;
    }
    follow_cursor(st_, renderer_.view_rows());
    renderer_.render(st_);
    st_.clear_render();
    render_pending_ = false;
}

int App::run() {
    for (;;) {
        render_if_dirty();
        const auto k = keys_.next(); // blocks (poll target: INPUT SEQUENCE)
        if (!k) {
            return 1; // hard input failure (stdin closed / tty gone)
        }
        if (k->kind == KeyKind::Resize) {
            on_resize();
            continue;
        }
        // TODO(P6/P16): drain EventQueue between keys; wake_fd_ wakes poll()
        // when job threads post events. Until then the loop is key-driven.
        if (k->kind == KeyKind::Char && k->cp == U'q'
            && st_.mode == Mode::Normal) {
            quit_ = true; // TEMP quit until :q (P6)
            return 0;
        }
        reducer(st_, command_for_key(st_, *k));
    }
}

} // namespace noni::app
