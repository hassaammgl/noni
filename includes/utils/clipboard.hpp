#pragma once

#include <optional>
#include <string>

namespace Clipboard
{
    // Read system clipboard (wl-paste → xclip → xsel). Empty / missing → nullopt.
    std::optional<std::string> read();
}
