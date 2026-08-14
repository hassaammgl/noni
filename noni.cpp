#include "./includes/utils/fs.hpp"
#include "./includes/utils/logger.hpp"
#include "./includes/utils/args.hpp"
#include "./includes/ui/ui.hpp"
#include "./includes/sidebar/dirscanner.hpp"

#include <fstream>

int main(int argc, char *argv[])
{
    FS fs;
    fs::path filePath =
        Args::getFilePath(argc, argv);
    // UI u(filePath);
    fs::path projpath = fs.currentPath();
    DirScanner d(projpath);
    d.scanDirs();
    // u.run();
    return 0;
}