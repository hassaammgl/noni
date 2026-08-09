#include "./includes/utils/fs.hpp"
#include <fstream>
#include <iostream>
#include <ncurses.h>
#include <nlohmann/json_fwd.hpp>
#include <string>

#include <nlohmann/json.hpp>

void read_json_config(std::string configfile)
{
  std::ifstream file(configfile);
  if (!file.is_open())
  {
    std::cerr << "Failed to open file\n";
  }

  nlohmann::json data;
  file >> data;
  std::cout << data.dump(4);
}

int main()
{
  FS filesystem;
  fs::path filepath = "./test/dogs/ninjas/test.txt";
  fs::path filepath2 = "./test/dogs/ninjas";
  std::cout << "is dir: " << filesystem.is_directory(filepath) << std::endl;
  std::cout << "is file: " << filesystem.is_file(filepath) << std::endl;
  std::cout << "is dir: " << filesystem.is_directory(filepath2) << std::endl;
  std::cout << "is file: " << filesystem.is_file(filepath2) << std::endl;
  std::cout << "current path: " << filesystem.currentPath() << std::endl;
  std::cout << "current path: " << filesystem.changeCurrentPath(filepath2) << std::endl;
  std::cout << "current path: " << filesystem.currentPath() << std::endl;
  std::cout << "absolute path: " << filesystem.absolute(filepath) << std::endl;
  std::cout << "cononical path: " << filesystem.canonical(filepath) << std::endl;
  std::cout << "weak cononical path: " << filesystem.weaklyCanonical(filepath) << std::endl;

  // read_json_config("config.json");
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);

  mvprintw(5, 10, "Hello, ncurses!");
  mvprintw(7, 10, "Press any key to exit.");

  refresh();

  getch();

  endwin();

  return 0;
}
