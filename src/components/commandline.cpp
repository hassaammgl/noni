#include <components/commandline.hpp>
#include <ui/theme.hpp>

void CommandLine::draw()
{
    if (!window || !active)
        return;

    werase(window);
    leaveok(window, FALSE);
    wbkgd(window, COLOR_PAIR(Theme::InputFocus));
    wattron(window, COLOR_PAIR(Theme::InputFocus));
    mvwhline(window, 0, 0, ' ', width);
    mvwprintw(window, 0, 0, "%c%s", prompt_, input.c_str());
    wattroff(window, COLOR_PAIR(Theme::InputFocus));
    wmove(window, 0, 1 + static_cast<int>(input.size()));
}

void CommandLine::open(char prompt)
{
    active = true;
    input.clear();
    prompt_ = (prompt == '/' || prompt == '?') ? prompt : ':';
}

void CommandLine::close()
{
    active = false;
}

bool CommandLine::is_active() const
{
    return active;
}

char CommandLine::prompt() const
{
    return prompt_;
}

void CommandLine::handle_input(int key)
{
    if (key == 27)
    {
        input.clear();
        close();
        return;
    }

    if (key == KEY_BACKSPACE || key == 127 || key == 8)
    {
        if (!input.empty())
            input.pop_back();
        return;
    }

    if (key >= 32 && key <= 126)
        input += static_cast<char>(key);
}

void CommandLine::clear_input()
{
    input.clear();
}

const std::string &CommandLine::get_input() const
{
    return input;
}
