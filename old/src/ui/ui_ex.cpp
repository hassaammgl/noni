#include "ui_impl.hpp"

void UI::execute_command()
{
    const char prompt = command_line.prompt();
    const std::string line = StrUtils::trim(command_line.get_input());
    command_line.close();
    focus = Focus::Editor;
    editor.enter_normal_mode();
    resize();

    if (prompt == '/' || prompt == '?')
    {
        if (line.empty())
        {
            Messages::info("Empty pattern");
            return;
        }
        const SearchDirection dir =
            (prompt == '?') ? SearchDirection::Backward : SearchDirection::Forward;
        const std::string err = core.search_start(line, dir);
        if (!err.empty())
            Messages::info(err);
        else
            Messages::info(std::format(
                "{} matches",
                core.search().matches().size()));
        return;
    }

    if (line.empty())
        return;

    ExCommands::instance().execute(*this, line);
}

void UI::ex_quit(bool bang)
{
    if (core.buffers().size() > 1)
    {
        close_active_tab(bang);
        return;
    }

    if (!bang && core.buffers().active().buffer().is_dirty())
    {
        open_save_confirm(ConfirmIntent::Quit);
        return;
    }
    request_quit();
}

void UI::ex_write(bool bang, const std::string &path)
{
    (void)bang;
    if (!path.empty())
    {
        if (!editor.get_buffer().save_as(path))
        {
            Messages::error("E212: Can't open file for writing");
            return;
        }
        statusbar.set_filename(core.buffers().active().display_name());
        Messages::info(std::format("\"{}\" written", core.buffers().active().display_name()));
        clear_recovery_for_buffer(editor.get_buffer());
        core.scm().request_refresh();
        core.lsp().notify_save(editor.get_buffer());
        core.notify_buffer_saved(editor.get_buffer());
        return;
    }

    if (editor.get_buffer().get_buffer_path().empty())
    {
        Messages::error("E32: No file name");
        return;
    }

    if (editor.get_buffer().has_load_error())
    {
        Messages::error(std::format(
            "E13: Cannot save — file was not loaded ({})",
            editor.get_buffer().get_load_error()));
        return;
    }

    if (editor.get_buffer().save())
    {
        statusbar.set_filename(core.buffers().active().display_name());
        Messages::info(std::format("\"{}\" written", core.buffers().active().display_name()));
        clear_recovery_for_buffer(editor.get_buffer());
        core.scm().request_refresh();
        core.lsp().notify_save(editor.get_buffer());
        core.notify_buffer_saved(editor.get_buffer());
    }
    else
    {
        Messages::error("E212: Can't open file for writing");
    }
}

void UI::ex_write_quit(bool bang, const std::string &path)
{
    if (!path.empty())
    {
        if (!editor.get_buffer().save_as(path))
        {
            Messages::error("E212: Can't open file for writing");
            return;
        }
    }
    else if (editor.get_buffer().get_buffer_path().empty())
    {
        if (!bang)
        {
            Messages::error("E32: No file name");
            return;
        }
    }
    else if (editor.get_buffer().has_load_error())
    {
        Messages::error(std::format(
            "E13: Cannot save — file was not loaded ({})",
            editor.get_buffer().get_load_error()));
        return;
    }
    else if (!editor.get_buffer().save())
    {
        Messages::error("E212: Can't open file for writing");
        return;
    }

    clear_recovery_for_buffer(editor.get_buffer());
    if (core.buffers().size() > 1)
        close_active_tab(true);
    else
        request_quit();
}

void UI::ex_edit(bool bang, const std::string &path)
{
    if (path.empty())
    {
        if (!bang && editor.get_buffer().is_dirty())
        {
            Messages::error("E37: No write since last change (add ! to override)");
            return;
        }
        editor.get_buffer().load();
        editor.set_cursor_position(0, 0);
        if (editor.get_buffer().has_load_error())
        {
            Messages::error(std::format(
                "Failed to reload {}: {}",
                editor.get_buffer().get_buffer_path().string(),
                editor.get_buffer().get_load_error()));
        }
        else
        {
            Messages::info("Buffer reloaded");
        }
        return;
    }

    if (!bang && editor.get_buffer().is_dirty() && core.buffers().size() == 1)
    {
        // Opening another file in a new tab is fine; only warn when replacing sole dirty buf.
    }

    core.buffers().open_file(path);
    note_opened_file(path);
    sync_active_tab();
    editor.enter_normal_mode();
    refresh_scm(fs::path(path));
    focus = Focus::Editor;
    Messages::info(std::format("\"{}\"", core.buffers().active().display_name()));
}
