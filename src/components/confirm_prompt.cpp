#include <components/confirm_prompt.hpp>
#include <ui/theme.hpp>

void ConfirmPrompt::draw()
{
    if (!window || !active)
        return;

    werase(window);
    leaveok(window, FALSE);
    wbkgd(window, COLOR_PAIR(Theme::InputFocus));
    wattron(window, COLOR_PAIR(Theme::InputFocus));
    mvwhline(window, 0, 0, ' ', width);

    std::string line = message;
    if (line.empty())
        line = "Save changes?";
    line += "  [y]es  [n]o  [c]ancel";

    if (static_cast<int>(line.size()) > width && width > 1)
        line = line.substr(0, static_cast<std::size_t>(width - 1));

    mvwprintw(window, 0, 0, "%s", line.c_str());
    wattroff(window, COLOR_PAIR(Theme::InputFocus));
    wmove(window, 0, 0);
}

void ConfirmPrompt::open(const std::string &msg)
{
    active = true;
    message = msg;
}

void ConfirmPrompt::close()
{
    active = false;
    message.clear();
}

bool ConfirmPrompt::is_active() const
{
    return active;
}

ConfirmChoice ConfirmPrompt::handle_input(int key)
{
    if (!active)
        return ConfirmChoice::Cancel;

    if (key == 27 || key == 3)
        return ConfirmChoice::Cancel;

    if (key == 'y' || key == 'Y')
        return ConfirmChoice::Yes;
    if (key == 'n' || key == 'N')
        return ConfirmChoice::No;
    if (key == 'c' || key == 'C' || key == 'q' || key == 'Q')
        return ConfirmChoice::Cancel;

    return ConfirmChoice::Pending;
}
