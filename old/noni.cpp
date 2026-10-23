#include <ui/ui.hpp>
#include <utils/args.hpp>
#include <utils/logger.hpp>
#include <format>
#include <iostream>

int main(int argc, char *argv[])
{
  Logger::init();
  Logger::info(std::format("noni started (argc={})", argc));

  fs::path file_path = Args::get_file_path(argc, argv);

  {
    UI u(file_path);
    u.run();
  }

  Logger::info("noni exiting");
  Logger::shutdown();

  return 0;
}
