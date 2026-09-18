#include <workspace/session.hpp>

#include <configs/mini_json.hpp>
#include <utils/fs.hpp>
#include <utils/logger.hpp>

#include <system_error>

namespace
{
    MiniJson::Value str_val(std::string s)
    {
        return MiniJson::Value{std::move(s)};
    }

    MiniJson::Value num_val(int n)
    {
        return MiniJson::Value{static_cast<double>(n)};
    }

    MiniJson::Value bool_val(bool b)
    {
        return MiniJson::Value{b};
    }
}

namespace SessionStore
{
    fs::path noni_dir(const fs::path &workspace_root)
    {
        if (workspace_root.empty())
            return {};
        return workspace_root / ".noni";
    }

    fs::path session_file(const fs::path &workspace_root)
    {
        const fs::path dir = noni_dir(workspace_root);
        if (dir.empty())
            return {};
        return dir / "session.json";
    }

    bool save(const fs::path &workspace_root, const SessionState &state, std::string *error)
    {
        const fs::path path = session_file(workspace_root);
        if (path.empty())
        {
            if (error)
                *error = "no workspace root";
            return false;
        }

        std::error_code ec;
        fs::create_directories(path.parent_path(), ec);
        if (ec)
        {
            if (error)
                *error = ec.message();
            return false;
        }

        MiniJson::Object root;
        root["version"] = num_val(state.version);
        root["workspace"] = str_val(state.workspace_root.empty()
                                        ? workspace_root.string()
                                        : state.workspace_root);
        root["activeIndex"] = num_val(state.active_index);
        root["sidebarVisible"] = bool_val(state.sidebar_visible);
        root["terminalVisible"] = bool_val(state.terminal_visible);

        MiniJson::Array tabs;
        tabs.reserve(state.tabs.size());
        for (const auto &t : state.tabs)
        {
            if (t.path.empty())
                continue;
            MiniJson::Object tab;
            tab["path"] = str_val(t.path);
            tab["cursorLine"] = num_val(t.cursor_line);
            tab["cursorColumn"] = num_val(t.cursor_column);
            tab["scrollY"] = num_val(t.scroll_y);
            tab["scrollX"] = num_val(t.scroll_x);
            tabs.push_back(MiniJson::Value{std::move(tab)});
        }
        root["tabs"] = MiniJson::Value{std::move(tabs)};

        MiniJson::Array recent;
        recent.reserve(state.recent.size());
        for (const auto &r : state.recent)
        {
            if (!r.empty())
                recent.push_back(str_val(r));
        }
        root["recent"] = MiniJson::Value{std::move(recent)};

        FS fs;
        const std::string text = MiniJson::stringify(MiniJson::Value{std::move(root)});
        if (!fs.write_file(path, {text}))
        {
            if (error)
                *error = "failed to write session.json";
            Logger::warning("SessionStore: write failed: " + path.string());
            return false;
        }
        return true;
    }

    std::optional<SessionState> load(const fs::path &workspace_root, std::string *error)
    {
        const fs::path path = session_file(workspace_root);
        if (path.empty())
        {
            if (error)
                *error = "no workspace root";
            return std::nullopt;
        }

        FS fs;
        if (!fs.exists(path))
            return std::nullopt;

        auto raw = fs.read_file(path);
        if (!raw)
        {
            if (error)
                *error = "failed to read session.json";
            return std::nullopt;
        }

        try
        {
            MiniJson::Value root = MiniJson::parse(*raw);
            if (!root.is_object())
            {
                if (error)
                    *error = "session.json is not an object";
                return std::nullopt;
            }

            SessionState st;
            st.version = root.get_int("version", 1);
            st.workspace_root = root.get_string("workspace", workspace_root.string());
            st.active_index = root.get_int("activeIndex", 0);
            if (const MiniJson::Value *v = root.get("sidebarVisible"); v && v->is_bool())
                st.sidebar_visible = v->as_bool(true);
            if (const MiniJson::Value *v = root.get("terminalVisible"); v && v->is_bool())
                st.terminal_visible = v->as_bool(false);

            if (const MiniJson::Value *tabs = root.get("tabs"); tabs && tabs->is_array())
            {
                for (const auto &el : tabs->as_array())
                {
                    if (!el.is_object())
                        continue;
                    SessionTabState t;
                    t.path = el.get_string("path");
                    if (t.path.empty())
                        continue;
                    t.cursor_line = el.get_int("cursorLine", 0);
                    t.cursor_column = el.get_int("cursorColumn", 0);
                    t.scroll_y = el.get_int("scrollY", 0);
                    t.scroll_x = el.get_int("scrollX", 0);
                    st.tabs.push_back(std::move(t));
                }
            }

            if (const MiniJson::Value *recent = root.get("recent"); recent && recent->is_array())
            {
                for (const auto &el : recent->as_array())
                {
                    if (el.is_string() && !el.as_string().empty())
                        st.recent.push_back(el.as_string());
                }
            }

            return st;
        }
        catch (const std::exception &ex)
        {
            if (error)
                *error = ex.what();
            Logger::warning(std::string("SessionStore: parse failed: ") + ex.what());
            return std::nullopt;
        }
    }
}
