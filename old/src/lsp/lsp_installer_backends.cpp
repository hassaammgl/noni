#include "lsp_installer_detail.hpp"

#include <utils/messages.hpp>

#include <cstring>
#include <format>

namespace lsp_install_detail
{
    namespace
    {
        bool install_pip_venv(const Spec &spec, const fs::path &root)
        {
            if (!exe_ok("python3"))
                return false;
            const fs::path venv = root / "pkg" / spec.binary;
            const fs::path py = venv / "bin" / "python";
            const fs::path pip = venv / "bin" / "pip";
            const fs::path out = venv / "bin" / spec.binary;

            if (!fs::exists(py))
            {
                if (run_cmd(std::format("python3 -m venv \"{}\"", venv.string())) != 0)
                    return false;
            }
            if (run_cmd(std::format(
                    "\"{}\" install -U pip \"{}\"",
                    pip.string(),
                    spec.package)) != 0)
                return false;
            if (!fs::exists(out))
                return false;
            return link_bin(out, root / "bin" / spec.binary);
        }

        bool install_npm_prefix(const Spec &spec, const fs::path &root)
        {
            const fs::path pkg = root / "pkg" / spec.binary;
            std::error_code ec;
            fs::create_directories(pkg, ec);

            std::string pkgs = std::format("\"{}\"", spec.package);
            if (spec.extra && *spec.extra)
                pkgs += std::format(" \"{}\"", spec.extra);

            int rc = 1;
            if (exe_ok("npm"))
            {
                rc = run_cmd(std::format(
                    "npm install --prefix \"{}\" {}",
                    pkg.string(),
                    pkgs));
            }
            if (rc != 0 && exe_ok("bun"))
            {
                rc = run_cmd(std::format(
                    "cd \"{}\" && bun init -y >/dev/null 2>&1; bun add {}",
                    pkg.string(),
                    pkgs));
            }
            if (rc != 0)
                return false;

            const fs::path bin = find_named_binary(pkg / "node_modules", spec.binary);
            if (bin.empty())
                return false;
            return link_bin(bin, root / "bin" / spec.binary);
        }

        bool install_curl_zip(const Spec &spec, const fs::path &root)
        {
            if (!exe_ok("curl"))
                return false;
            const fs::path pkg = root / "pkg" / spec.binary;
            const fs::path zip = root / "pkg" / (std::string(spec.binary) + ".zip");
            std::error_code ec;
            fs::remove_all(pkg, ec);
            fs::create_directories(pkg, ec);

            if (std::strcmp(spec.binary, "rust-analyzer") == 0)
            {
                const std::string url = std::format(
                    "https://github.com/{}/releases/download/{}/"
                    "rust-analyzer-x86_64-unknown-linux-gnu.gz",
                    spec.package,
                    spec.extra);
                const fs::path gz = root / "pkg" / "rust-analyzer.gz";
                if (run_cmd(std::format(
                        "curl --fail --location --silent --show-error --max-time 180 "
                        "-L \"{}\" --output \"{}\"",
                        url,
                        gz.string())) != 0)
                    return false;
                const fs::path out = pkg / "rust-analyzer";
                if (run_cmd(std::format(
                        "gzip -dc \"{}\" > \"{}\" && chmod +x \"{}\"",
                        gz.string(),
                        out.string(),
                        out.string())) != 0)
                    return false;
                return link_bin(out, root / "bin" / spec.binary);
            }

            if (std::strcmp(spec.binary, "clangd") != 0)
                return false;

            const std::string url = std::format(
                "https://github.com/{}/releases/download/{}/clangd-linux-{}.zip",
                spec.package,
                spec.extra,
                spec.extra);
            if (run_cmd(std::format(
                    "curl --fail --location --silent --show-error --max-time 300 "
                    "-L \"{}\" --output \"{}\"",
                    url,
                    zip.string())) != 0)
                return false;

            if (exe_ok("unzip"))
            {
                if (run_cmd(std::format(
                        "unzip -o -q \"{}\" -d \"{}\"", zip.string(), pkg.string())) != 0)
                    return false;
            }
            else if (exe_ok("bsdtar"))
            {
                if (run_cmd(std::format(
                        "bsdtar -xf \"{}\" -C \"{}\"", zip.string(), pkg.string())) != 0)
                    return false;
            }
            else
                return false;

            const fs::path found = find_named_binary(pkg, spec.binary);
            if (found.empty())
                return false;
            return link_bin(found, root / "bin" / spec.binary);
        }

        bool install_go(const Spec &spec, const fs::path &root)
        {
            if (!exe_ok("go"))
                return false;
            const fs::path bin = root / "bin";
            std::error_code ec;
            fs::create_directories(bin, ec);
            const std::string ver = (spec.extra && *spec.extra) ? spec.extra : "latest";
            return run_cmd(std::format(
                       "GOBIN=\"{}\" go install {}@{}",
                       bin.string(),
                       spec.package,
                       ver)) == 0 &&
                   fs::exists(bin / spec.binary);
        }
    }

    bool install_one(const Spec &spec)
    {
        const fs::path root = LspInstaller::user_dir();
        std::error_code ec;
        fs::create_directories(root / "bin", ec);
        fs::create_directories(root / "pkg", ec);

        Messages::info(std::format("Installing LSP `{}` → {} …", spec.binary, root.string()));
        bool ok = false;
        switch (spec.method)
        {
        case Method::PipVenv:
            ok = install_pip_venv(spec, root);
            break;
        case Method::NpmPrefix:
            ok = install_npm_prefix(spec, root);
            break;
        case Method::CurlGithubZip:
            ok = install_curl_zip(spec, root);
            break;
        case Method::GoInstall:
            ok = install_go(spec, root);
            break;
        }

        if (ok)
            Messages::info(std::format("LSP ready: {}", (root / "bin" / spec.binary).string()));
        else
            Messages::warning(std::format(
                "LSP install failed: {} (retry 120s; log: logs/lsp-install.log)",
                spec.binary));
        return ok;
    }
}
