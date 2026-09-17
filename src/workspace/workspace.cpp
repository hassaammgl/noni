#include <workspace/workspace.hpp>
#include <scm/scm_git.hpp>

#include <system_error>

fs::path Workspace::detect_root(const fs::path &hint)
{
    const fs::path git_root = ScmGit::find_repository_root(hint);
    if (!git_root.empty())
        return git_root;

    std::error_code ec;
    fs::path cur = hint.empty() ? fs::current_path() : hint;
    if (fs::is_regular_file(cur, ec))
        cur = cur.parent_path();
    cur = fs::weakly_canonical(cur, ec);
    if (ec)
        cur = hint.empty() ? fs::current_path() : hint;

    auto is_project_root = [&](const fs::path &dir) -> bool {
        static const char *markers[] = {
            ".git", ".hg", ".svn", ".noni",
            "CMakeLists.txt", "Makefile", "meson.build", "configure.ac",
            "compile_commands.json", "vcpkg.json", "conanfile.txt", "conanfile.py",
            "Cargo.toml", "go.mod", "build.zig", "Package.swift",
            "pyproject.toml", "setup.py", "setup.cfg", "requirements.txt",
            "Pipfile", "poetry.lock", "tox.ini",
            "pom.xml", "build.gradle", "build.gradle.kts",
            "settings.gradle", "settings.gradle.kts", "build.sbt",
            "project.clj", "deps.edn",
            "package.json", "pnpm-workspace.yaml", "lerna.json",
            ".luarc.json", "selene.toml",
            "composer.json", "Gemfile", "mix.exs", "pubspec.yaml",
        };
        for (const char *m : markers)
        {
            if (fs::exists(dir / m, ec))
                return true;
        }

        for (const auto &entry : fs::directory_iterator(dir, ec))
        {
            if (ec || !entry.is_regular_file(ec))
                continue;
            const std::string ext = entry.path().extension().string();
            if (ext == ".sln" || ext == ".csproj" || ext == ".fsproj" ||
                ext == ".vbproj" || ext == ".rockspec")
                return true;
        }
        return false;
    };

    fs::path walk = cur;
    while (!walk.empty())
    {
        if (is_project_root(walk))
            return walk;
        const fs::path parent = walk.parent_path();
        if (parent == walk)
            break;
        walk = parent;
    }

    return cur.empty() ? fs::current_path() : cur;
}

void Workspace::open(const fs::path &hint)
{
    set_root(detect_root(hint));
}

void Workspace::set_root(fs::path root)
{
    std::error_code ec;
    if (!root.empty() && fs::is_regular_file(root, ec))
        root = root.parent_path();
    if (!root.empty())
    {
        fs::path canon = fs::weakly_canonical(root, ec);
        if (!ec)
            root = std::move(canon);
    }
    root_ = std::move(root);
}

std::string Workspace::display_name() const
{
    if (root_.empty())
        return "(no workspace)";
    return root_.filename().empty() ? root_.string() : root_.filename().string();
}
