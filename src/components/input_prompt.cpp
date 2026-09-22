#include <components/input_prompt.hpp>
#include <ui/theme.hpp>
#include <utils/text_metrics.hpp>

#include <algorithm>
#include <string_view>

void InputPrompt::draw()
{
    if (!window || !active)
        return;

    werase(window);
    leaveok(window, FALSE);
    wbkgd(window, COLOR_PAIR(Theme::InputFocus));
    wattron(window, COLOR_PAIR(Theme::InputFocus));
    mvwhline(window, 0, 0, ' ', width);

    std::string line = prefix + input;
    if (static_cast<int>(line.size()) > width && width > 1)
        line = line.substr(0, static_cast<std::size_t>(width - 1));

    mvwprintw(window, 0, 0, "%s", line.c_str());
    wattroff(window, COLOR_PAIR(Theme::InputFocus));
    wmove(window, 0, std::min(width - 1, static_cast<int>(prefix.size() + input.size())));
}

void InputPrompt::open(const std::string &p, const std::string &initial)
{
    active = true;
    prefix = p;
    input = initial;
}

void InputPrompt::close()
{
    active = false;
    prefix.clear();
    input.clear();
}

bool InputPrompt::is_active() const
{
    return active;
}

void InputPrompt::handle_input(int key)
{
    if (key == 27 || key == 3)
    {
        input.clear();
        close();
        return;
    }

    if (key == KEY_BACKSPACE || key == 127 || key == 8)
    {
        if (!input.empty())
            TextMetrics::pop_codepoint(input);
        return;
    }

    if (key >= 32 && key <= 126)
        input += static_cast<char>(key);
}

void InputPrompt::insert_utf8(std::string_view utf8)
{
    if (!active || utf8.empty())
        return;
    input.append(utf8.data(), utf8.size());
}

void InputPrompt::clear_input()
{
    input.clear();
}

const std::string &InputPrompt::get_input() const
{
    return input;
}

const std::string &InputPrompt::get_prefix() const
{
    return prefix;
}
