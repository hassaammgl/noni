#include <utils/clipboard.hpp>
#include <utils/logger.hpp>

#include <array>
#include <cstdio>
#include <format>
#include <memory>

namespace
{
    std::optional<std::string> run_capture(const char *cmd)
    {
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
        if (!pipe)
            return std::nullopt;

        std::string out;
        char buf[4096];
        while (std::fgets(buf, sizeof(buf), pipe.get()) != nullptr)
            out += buf;

        // Also drain any remaining without newline
        // (fgets already covers text; binary paste rare for editors)

        if (out.empty())
            return std::nullopt;
        return out;
    }
}

std::optional<std::string> Clipboard::read()
{
    static constexpr std::array<const char *, 3> cmds = {
        "wl-paste -n 2>/dev/null",
        "xclip -selection clipboard -o 2>/dev/null",
        "xsel --clipboard --output 2>/dev/null",
    };

    for (const char *cmd : cmds)
    {
        if (auto text = run_capture(cmd))
        {
            Logger::debug(std::format("clipboard: read {} bytes via {}", text->size(), cmd));
            return text;
        }
    }

    Logger::debug("clipboard: no clipboard tool returned text");
    return std::nullopt;
}
