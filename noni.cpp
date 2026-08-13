#include "./includes/utils/fs.hpp"
#include "./includes/utils/logger.hpp"
#include "./includes/utils/args.hpp"
#include "./includes/ui/ui.hpp"

#include <fstream>
#include <iostream>
#include <ncurses.h>
#include <nlohmann/json.hpp>
#include <string>

void read_json_config(const std::string &configfile)
{
    std::ifstream file(configfile);

    if (!file.is_open())
    {
        std::cerr
            << "Failed to open config file: "
            << configfile << '\n';

        return;
    }

    nlohmann::json data;

    file >> data;

    std::cout << data.dump(4) << '\n';
}

int main(int argc, char *argv[])
{
    fs::path filePath =
        Args::getFilePath(argc, argv);
    UI u(filePath);
    u.run();
    return 0;
}