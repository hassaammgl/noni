#include "grammar_state.hpp"
#include <utils/logger.hpp>
#include <array>
#include <cctype>
#include <cstdlib>
#include <format>
namespace grammar_install_state
{
    // nvim-treesitter: prefer curl+tar of GitHub/GitLab archive; git is fallback.
    bool download_tarball(const InstallSpec &spec, const fs::path &cache_folder, const fs::path &repo_dir)
    {
        if (!exe_ok("curl") || !exe_ok("tar"))
            return false;

        const std::string project = repo_basename(spec.git_url);
        std::string url = spec.git_url;
        if (url.size() > 4 && url.substr(url.size() - 4) == ".git")
            url = url.substr(0, url.size() - 4);

        const bool github = url.find("github.com") != std::string::npos;
        const bool gitlab = url.find("gitlab.com") != std::string::npos;
        if (!github && !gitlab)
            return false;

        // Try preferred revision, then common defaults (repos migrated master→main).
        const std::array<const char *, 3> revisions = {
            spec.revision,
            "master",
            "main",
        };

        const fs::path tar_path = cache_folder / (project + ".tar.gz");
        const fs::path tmp_dir = cache_folder / (project + "-tmp");

        for (const char *rev : revisions)
        {
            if (!rev || !*rev)
                continue;

            std::error_code ec;
            fs::remove_all(tmp_dir, ec);
            fs::remove(tar_path, ec);
            fs::remove_all(repo_dir, ec);
            fs::create_directories(tmp_dir, ec);

            std::string archive_url;
            if (github)
                archive_url = std::format("{}/archive/{}.tar.gz", url, rev);
            else
                archive_url = std::format(
                    "{}/-/archive/{}/{}-{}.tar.gz",
                    url,
                    rev,
                    project,
                    rev);

            const std::string curl_cmd = std::format(
                "curl --fail --location --silent --show-error --max-time 120 "
                "-L \"{}\" --output \"{}\"",
                archive_url,
                tar_path.string());
            if (run_cmd(curl_cmd) != 0 || !fs::exists(tar_path))
                continue;

            const std::string tar_cmd = std::format(
                "tar -xzf \"{}\" -C \"{}\"",
                tar_path.string(),
                tmp_dir.string());
            if (run_cmd(tar_cmd) != 0)
                continue;

            // Extracted dir is typically "<repo>-<rev>" (tags like v0.1 → strip leading v like nvim).
            std::string folder_rev = rev;
            if (github && folder_rev.size() >= 2 && folder_rev[0] == 'v' &&
                std::isdigit(static_cast<unsigned char>(folder_rev[1])))
                folder_rev = folder_rev.substr(1);

            const fs::path extracted = tmp_dir / (project + "-" + folder_rev);
            fs::path chosen = extracted;
            if (!fs::exists(chosen))
            {
                // Pick the single top-level directory tar produced.
                chosen.clear();
                for (const auto &entry : fs::directory_iterator(tmp_dir, ec))
                {
                    if (entry.is_directory(ec))
                    {
                        chosen = entry.path();
                        break;
                    }
                }
            }

            if (chosen.empty() || !fs::exists(chosen))
                continue;

            fs::rename(chosen, repo_dir, ec);
            if (ec)
            {
                fs::remove_all(repo_dir, ec);
                fs::rename(chosen, repo_dir, ec);
            }

            fs::remove(tar_path, ec);
            fs::remove_all(tmp_dir, ec);

            if (fs::exists(repo_dir / spec.src_rel / "parser.c") ||
                fs::exists(repo_dir / "src" / "parser.c"))
            {
                Logger::info(std::format(
                    "grammar-install: downloaded {} @ {} (curl+tar)",
                    project,
                    rev));
                return true;
            }
        }

        std::error_code ec;
        fs::remove(tar_path, ec);
        fs::remove_all(tmp_dir, ec);
        fs::remove_all(repo_dir, ec);
        return false;
    }

    bool download_git(const InstallSpec &spec, const fs::path &cache_folder, const fs::path &repo_dir)
    {
        if (!exe_ok("git"))
            return false;

        // Avoid corrupting an active git session (same guard as nvim-treesitter).
        static const char *git_env[] = {
            "GIT_DIR",
            "GIT_INDEX_FILE",
            "GIT_WORK_TREE",
            "GIT_PREFIX",
            "GIT_OBJECT_DIRECTORY",
            nullptr,
        };
        for (const char **e = git_env; *e; ++e)
        {
            if (std::getenv(*e))
            {
                Logger::warning(
                    "grammar-install: skipping git clone inside active git session env");
                return false;
            }
        }

        std::error_code ec;
        fs::remove_all(repo_dir, ec);
        fs::create_directories(cache_folder, ec);

        // nvim classic: git clone --filter=blob:none then checkout revision.
        const std::string clone_cmd = std::format(
            "git -C \"{}\" clone --filter=blob:none \"{}\" \"{}\"",
            cache_folder.string(),
            spec.git_url,
            repo_dir.filename().string());
        if (run_cmd(clone_cmd) != 0 || !fs::exists(repo_dir))
        {
            // Fallback shallow clone (older git without filter).
            const std::string shallow = std::format(
                "git clone --depth 1 --single-branch --branch \"{}\" \"{}\" \"{}\"",
                spec.revision && *spec.revision ? spec.revision : "master",
                spec.git_url,
                repo_dir.string());
            if (run_cmd(shallow) != 0)
                return false;
            return fs::exists(repo_dir);
        }

        if (spec.revision && *spec.revision)
        {
            const std::string co = std::format(
                "git -C \"{}\" checkout \"{}\"",
                repo_dir.string(),
                spec.revision);
            (void)run_cmd(co); // best-effort; tip of default branch still usable
        }

        Logger::info(std::format(
            "grammar-install: cloned {} (git)",
            repo_basename(spec.git_url)));
        return true;
    }
}
