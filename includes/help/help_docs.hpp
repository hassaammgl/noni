#pragma once

#include <configs/config.hpp>
#include <editor/ex_commands.hpp>

#include <string>
#include <string_view>
#include <vector>

namespace HelpDocs
{
    // Topic ids (lowercase). Empty / "index" = overview.
    std::vector<std::string> topic_ids();

    // Build help pages. `topic` may be a topic id, "all", an ex-command name, or empty.
    // Returns false if unknown topic/command.
    bool build(
        std::string_view topic,
        const AppConfig &config,
        const std::vector<ExCommand> &ex_commands,
        std::vector<std::string> &out_lines,
        std::string &error);
}
