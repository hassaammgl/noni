#include "app/app.hpp"

#include "ext/ext.hpp"
#include "lsp/lsp.hpp"
#include "syntax/syntax.hpp"
#include "terminal/terminal.hpp"

namespace noni::app {

bool app_ready() {
    return ext::shell_ready() && terminal::pty_ready()
        && syntax::highlight_ready() && lsp::client_ready();
}

} // namespace noni::app
