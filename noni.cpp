#include "./includes/ui/ui.hpp"
#include "./includes/utils/args.hpp"

int main(int argc, char *argv[]) {
  fs::path filePath = Args::getFilePath(argc, argv);

  UI u(filePath);
  u.run();
  return 0;
}
