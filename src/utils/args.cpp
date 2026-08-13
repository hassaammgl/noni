#include "utils/args.hpp"

fs::path Args::getFilePath(
    int argc,
    char *argv[])
{
    if (argc < 2)
        return {};

    return fs::path(argv[1]);
}