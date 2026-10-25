// P2.5 — CLI + ncurses wiring for the real App loop.
//   noni [--version] [file]   file missing => error message, empty buffer
// The `--keys` debug harness is gone (ACCEPT P2.4: debug line removed after
// P2.4); the bottom row is the message/cmdline row reserved for P6.
#include <algorithm>
#include <print>
#include <string>
#include <string_view>

#include "app/app.hpp"
#include "core/version.hpp"
#include "ui/ui.hpp"

namespace {

// NCurses adapters for the app-layer interfaces (main wires ui -> app).
class ScreenKeySource final : public noni::app::KeySource {
public:
    ScreenKeySource(noni::ui::Screen& s) : r_{s.win()} {}
    [[nodiscard]] noni::eresult<noni::Key> next() override {
        return r_.next();
    }

private:
    noni::ui::KeyReader r_;
};

class ScreenRenderer final : public noni::app::Renderer {
public:
    explicit ScreenRenderer(noni::ui::Screen& s) : s_{&s} {}
    [[nodiscard]] int view_rows() const override {
        return std::max(s_->rows() - 3, 0);
    }
    void render(const noni::EditorState& st) override {
        noni::ui::draw_layout(*s_, st);
        place_cursor(st);
    }
    void on_screen_resize() override { s_->on_resize(); }

private:
    void place_cursor(const noni::EditorState& st) {
        const noni::WindowState& w = st.win();
        const noni::Buffer* b = st.buffer(w.buffer);
        if (b == nullptr) {
            return;
        }
        const int row =
            noni::ui::screen_row_of_line(w.cursor.line, w.top_line);
        // Logical cursor on the line's display cell (tabs/wide/marker-aware).
        const noni::ui::LineCells lc =
            noni::ui::LineCells::build(b->lines()[w.cursor.line],
                                       noni::ui::k_tabwidth);
        int col = noni::ui::gutter_width(b->lines().size())
                + lc.disp_of_cp(w.cursor.col)
                - static_cast<int>(w.left_col);
        if (col < 0) {
            col = 0; // mid-cell clip: park at the region start (P3.6 refines)
        }
        wmove(s_->win(), 1 + static_cast<int>(row), col);
        wrefresh(s_->win());
    }
    noni::ui::Screen* s_;
};

} // namespace

int main(int argc, char** argv) {
    std::string path_arg;
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg = argv[i];
        if (arg == "--version") {
            std::println("{}", noni::version());
            return 0;
        }
        if (!path_arg.empty()) {
            std::println(stderr, "noni: multiple file arguments ('{}' after '{}')",
                         arg, path_arg);
            return 2;
        }
        path_arg = std::string{arg};
    }

    auto screen = noni::ui::Screen::open();
    if (!screen) {
        std::println(stderr, "noni: {}", screen.error().msg);
        return 1;
    }
    ScreenKeySource keys{*screen};
    ScreenRenderer renderer{*screen};

    noni::app::App app{keys, renderer};
    app.load_file(path_arg);
    return app.run();
}
