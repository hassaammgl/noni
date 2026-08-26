#include <ui/ui.hpp>
#include <utils/args.hpp>

int main(int argc, char *argv[]) {
  fs::path file_path = Args::get_file_path(argc, argv);

  UI u(file_path);
  u.run();
  return 0;
}
