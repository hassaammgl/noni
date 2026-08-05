#include <fstream>
#include <iostream>
#include <ncurses.h>
#include <nlohmann/json_fwd.hpp>
#include <string>

#include <nlohmann/json.hpp>

void read_json_config(std::string configfile) {
  std::ifstream file(configfile);
  if (!file.is_open()) {
    std::cerr << "Failed to open file\n";
  }
  nlohmann::json data;
  file >> data;
  std::cout << data.dump(4);
}

int main() {

  read_json_config("config.json");
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
