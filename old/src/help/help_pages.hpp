#pragma once

#include <configs/config.hpp>
#include <editor/ex_commands.hpp>

#include <cctype>
#include <format>
#include <string>
#include <vector>

namespace help_detail
{
    inline void line(std::vector<std::string> &out, std::string s)
    {
        out.push_back(std::move(s));
    }

    inline void blank(std::vector<std::string> &out) { out.emplace_back(""); }

    inline void section(std::vector<std::string> &out, const std::string &title)
    {
        blank(out);
        line(out, std::format("=== {} ===", title));
        blank(out);
    }

    inline void bullet(std::vector<std::string> &out, const std::string &k, const std::string &v)
    {
        line(out, std::format("  {:<28} {}", k, v));
    }

    inline std::string lower(std::string s)
    {
        for (char &c : s)
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return s;
    }

    void page_index(std::vector<std::string> &out);
    void page_modes(std::vector<std::string> &out);
    void page_motions(std::vector<std::string> &out);
    void page_operators(std::vector<std::string> &out);
    void page_search(std::vector<std::string> &out);
    void page_windows(std::vector<std::string> &out);
    void page_buffers(std::vector<std::string> &out);
    void page_workspace(std::vector<std::string> &out);
    void page_sidebar(std::vector<std::string> &out);
    void page_git(std::vector<std::string> &out);
    void page_session(std::vector<std::string> &out);
    void page_lsp(std::vector<std::string> &out);
    void page_terminal(std::vector<std::string> &out);
    void page_extensions(std::vector<std::string> &out);
    void page_config(std::vector<std::string> &out);
    void page_keybindings(std::vector<std::string> &out, const AppConfig &config);
    void page_ex(std::vector<std::string> &out, const std::vector<ExCommand> &cmds);
    void page_overview_extras(std::vector<std::string> &out);
}
