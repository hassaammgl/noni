#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace Clipboard
{
    // Read system clipboard (wl-paste → xclip → xsel). Empty / missing → nullopt.
    std::optional<std::string> read();

    // Write: wl-copy → xclip -selection clipboard → xsel -ib (fork/exec + pipe).
    // If tools missing: OSC52 to stdout. Returns true if any path succeeded.
    // `hint_out` gets a short user-facing reason when false / OSC52 fallback.
    bool write(std::string_view text, std::string *hint_out = nullptr);

    // Encode + emit OSC52 (always attempts stdout).
    bool write_osc52(std::string_view text);
}
