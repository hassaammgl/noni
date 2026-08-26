#include <utils/args.hpp>

fs::path Args::get_file_path(
    int argc,
    char *argv[])
{
    if (argc < 2)
        return {};

    return fs::path(argv[1]);
}