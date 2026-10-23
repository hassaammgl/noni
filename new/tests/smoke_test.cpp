// No exceptions anywhere (AGENTS.md), tests included: doctest runs in its
// no-exceptions mode with all asserts (returns from test case on failure).
#define DOCTEST_CONFIG_NO_EXCEPTIONS_BUT_WITH_ALL_ASSERTS
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "app/app.hpp"
#include "core/version.hpp"
#include "ext/ext.hpp"
#include "lsp/lsp.hpp"
#include "syntax/syntax.hpp"
#include "terminal/terminal.hpp"
#include "ui/ui.hpp"

TEST_CASE("smoke: doctest runs in the sanitized noni_tests binary") {
    CHECK(1 == 1);
}

TEST_CASE("smoke: every layer target is linked and answers") {
    using namespace noni;
    REQUIRE(std::string_view(version()).size() > 0);
    CHECK(ext::shell_ready());
    CHECK(terminal::pty_ready());
    CHECK(syntax::highlight_ready());
    CHECK(lsp::client_ready());
    CHECK(app::app_ready());
    CHECK(ui::screen_ready());
}
