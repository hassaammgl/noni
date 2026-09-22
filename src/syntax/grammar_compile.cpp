#include "grammar_state.hpp"
#include <utils/logger.hpp>
#include <utils/messages.hpp>
#include <format>
#include <vector>
namespace grammar_install_state
{
    // Compile flags aligned with nvim-treesitter shell_command_selectors (unix path).
    bool compile_parser(const fs::path &src_dir, const fs::path &out_so)
    {
        const fs::path parser = src_dir / "parser.c";
        if (!fs::exists(parser))
        {
            Logger::error(std::format("grammar-install: missing {}", parser.string()));
            return false;
        }

        std::vector<fs::path> sources{parser};
        bool cxx = false;
        if (fs::exists(src_dir / "scanner.c"))
            sources.push_back(src_dir / "scanner.c");
        if (fs::exists(src_dir / "scanner.cc"))
        {
            sources.push_back(src_dir / "scanner.cc");
            cxx = true;
        }
        if (fs::exists(src_dir / "scanner.cpp"))
        {
            sources.push_back(src_dir / "scanner.cpp");
            cxx = true;
        }

        const char *cc = nullptr;
        if (cxx)
        {
            if (exe_ok("c++"))
                cc = "c++";
            else if (exe_ok("g++"))
                cc = "g++";
            else if (exe_ok("clang++"))
                cc = "clang++";
        }
        else
        {
            if (exe_ok("cc"))
                cc = "cc";
            else if (exe_ok("gcc"))
                cc = "gcc";
            else if (exe_ok("clang"))
                cc = "clang";
        }
        if (!cc)
        {
            Logger::error("grammar-install: no C/C++ compiler found (cc/gcc/clang)");
            return false;
        }

        fs::create_directories(out_so.parent_path());
        const fs::path tmp = out_so.string() + ".tmp";

        // nvim: -o parser.so -I./src files -Os -shared -fPIC [-lstdc++]
        // Use c11 for C-only grammars; C++ scanners need a C++ standard.
        std::string cmd = cc;
        cmd += " -o \"";
        cmd += tmp.string();
        cmd += "\" -I\"";
        cmd += src_dir.string();
        cmd += "\"";
        for (const auto &s : sources)
        {
            cmd += " \"";
            cmd += s.string();
            cmd += "\"";
        }
        cmd += cxx ? " -Os -std=c++14 -shared -fPIC -lstdc++"
                   : " -Os -std=c11 -shared -fPIC";

        if (run_cmd(cmd) != 0)
        {
            fs::remove(tmp);
            return false;
        }

        std::error_code ec;
        fs::rename(tmp, out_so, ec);
        if (ec)
        {
            fs::remove(out_so, ec);
            fs::rename(tmp, out_so, ec);
        }
        return fs::exists(out_so);
    }

    bool install_one(const InstallSpec &spec)
    {
        const fs::path root = GrammarInstaller::user_dir();
        const fs::path cache_folder = root / "src";
        const std::string project = repo_basename(spec.git_url);
        const fs::path repo_dir = cache_folder / project;
        const fs::path out_so = root / spec.so_name;

        Messages::info(std::format(
            "Grammar {} → {}",
            spec.so_name,
            root.string()));

        std::error_code ec;
        fs::create_directories(cache_folder, ec);
        if (ec)
        {
            Logger::error(std::format(
                "grammar-install: cannot write {} ({}) — fix ownership "
                "(e.g. sudo chown -R \"$USER\" ~/.local/share/noni)",
                cache_folder.string(),
                ec.message()));
            Messages::error(std::format(
                "Grammar dir not writable: {}",
                root.string()));
            return false;
        }

        const bool have_src =
            fs::exists(repo_dir / spec.src_rel / "parser.c") ||
            fs::exists(repo_dir / "src" / "parser.c");

        if (!have_src)
        {
            Messages::info(std::format(
                "Downloading {} → {}",
                project,
                repo_dir.string()));
            // nvim order: curl+tar first, then git.
            if (!download_tarball(spec, cache_folder, repo_dir) &&
                !download_git(spec, cache_folder, repo_dir))
            {
                Logger::error(std::format(
                    "grammar-install: download failed for {} (need curl+tar or git)",
                    spec.so_name));
                Messages::warning(std::format(
                    "Download failed: {} (see logs/grammar-install.log)",
                    project));
                return false;
            }
            Messages::info(std::format("Downloaded {}", project));
        }
        else
        {
            Messages::info(std::format("Using cached src {}", repo_dir.string()));
        }

        fs::path src_dir = repo_dir / spec.src_rel;
        if (!fs::exists(src_dir / "parser.c"))
            src_dir = repo_dir / "src";

        Messages::info(std::format("Compiling {} → {}", spec.so_name, out_so.string()));
        if (!compile_parser(src_dir, out_so))
        {
            Logger::error(std::format("grammar-install: compile failed for {}", spec.so_name));
            Messages::warning(std::format(
                "Compile failed: {} (see logs/grammar-install.log)",
                spec.so_name));
            return false;
        }

        Logger::info(std::format("grammar-install: ready {}", out_so.string()));
        return true;
    }

    void set_status(const std::string &so, GrammarInstaller::Status st)
    {
        std::lock_guard lock(g_mu);
        g_status[so] = st;
    }
}
