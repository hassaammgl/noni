#include <workspace/recovery.hpp>

#include <configs/mini_json.hpp>
#include <utils/fs.hpp>
#include <utils/logger.hpp>
#include <utils/str.hpp>

#include <cstdio>
#include <functional>
#include <system_error>

namespace
{
    fs::path canonical_or(const fs::path &p)
    {
        std::error_code ec;
        fs::path abs = fs::weakly_canonical(p, ec);
        if (ec)
            abs = fs::absolute(p, ec);
        if (ec)
            abs = p;
        return abs.lexically_normal();
    }

    MiniJson::Value str_val(std::string s)
    {
        return MiniJson::Value{std::move(s)};
    }
}

namespace RecoveryStore
{
    fs::path recovery_dir(const fs::path &workspace_root)
    {
        if (workspace_root.empty())
            return {};
        return workspace_root / ".noni" / "recovery";
    }

    std::string path_id(const fs::path &path)
    {
        const std::string key = canonical_or(path).string();
        const std::size_t h = std::hash<std::string>{}(key);
        char buf[32];
        std::snprintf(
            buf,
            sizeof(buf),
            "%016llx",
            static_cast<unsigned long long>(h));
        return buf;
    }

    bool write_snapshot(
        const fs::path &workspace_root,
        const fs::path &original_path,
        const std::vector<std::string> &lines,
        std::string *error)
    {
        if (workspace_root.empty() || original_path.empty())
        {
            if (error)
                *error = "missing path";
            return false;
        }

        const fs::path dir = recovery_dir(workspace_root);
        std::error_code ec;
        fs::create_directories(dir, ec);
        if (ec)
        {
            if (error)
                *error = ec.message();
            return false;
        }

        const std::string id = path_id(original_path);
        const fs::path text_path = dir / (id + ".txt");
        const fs::path meta_path = dir / (id + ".meta.json");

        FS fs;
        if (!fs.write_file(text_path, lines))
        {
            if (error)
                *error = "failed to write recovery snapshot";
            return false;
        }

        MiniJson::Object meta;
        meta["id"] = str_val(id);
        meta["path"] = str_val(canonical_or(original_path).string());
        meta["lines"] = MiniJson::Value{static_cast<double>(lines.size())};

        if (!fs.write_file(meta_path, {MiniJson::stringify(MiniJson::Value{std::move(meta)})}))
        {
            if (error)
                *error = "failed to write recovery meta";
            return false;
        }
        return true;
    }

    void clear_snapshot(const fs::path &workspace_root, const fs::path &original_path)
    {
        if (workspace_root.empty() || original_path.empty())
            return;
        const fs::path dir = recovery_dir(workspace_root);
        const std::string id = path_id(original_path);
        std::error_code ec;
        fs::remove(dir / (id + ".txt"), ec);
        fs::remove(dir / (id + ".meta.json"), ec);
    }

    std::vector<RecoveryEntry> list(const fs::path &workspace_root)
    {
        std::vector<RecoveryEntry> out;
        const fs::path dir = recovery_dir(workspace_root);
        if (dir.empty())
            return out;

        std::error_code ec;
        if (!fs::is_directory(dir, ec))
            return out;

        FS fs;
        for (const auto &entry : fs::directory_iterator(dir, ec))
        {
            if (ec || !entry.is_regular_file(ec))
                continue;
            const fs::path p = entry.path();
            if (p.extension() != ".json")
                continue;
            const std::string name = p.filename().string();
            // <id>.meta.json
            if (name.size() < 10 || name.find(".meta.json") == std::string::npos)
                continue;

            auto raw = fs.read_file(p);
            if (!raw)
                continue;
            try
            {
                MiniJson::Value root = MiniJson::parse(*raw);
                if (!root.is_object())
                    continue;
                RecoveryEntry e;
                e.id = root.get_string("id");
                e.original_path = root.get_string("path");
                e.meta_path = p;
                if (e.id.empty())
                {
                    // derive from filename
                    e.id = name.substr(0, name.size() - std::string(".meta.json").size());
                }
                e.text_path = dir / (e.id + ".txt");
                if (e.original_path.empty() || !fs.exists(e.text_path))
                    continue;
                out.push_back(std::move(e));
            }
            catch (...)
            {
                continue;
            }
        }
        return out;
    }

    std::optional<std::vector<std::string>> read_lines(
        const RecoveryEntry &entry,
        std::string *error)
    {
        FS fs;
        auto raw = fs.read_file(entry.text_path);
        if (!raw)
        {
            if (error)
                *error = "recovery text missing";
            return std::nullopt;
        }
        auto lines = StrUtils::split(*raw, '\n');
        if (lines.empty())
            lines.push_back("");
        return lines;
    }
}
