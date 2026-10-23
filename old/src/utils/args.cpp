#include <utils/args.hpp>
#include <utils/logger.hpp>
#include <format>

fs::path Args::get_file_path(
    int argc,
    char *argv[])
{
    if (argc < 2)
    {
        Logger::debug("No CLI file argument provided");
        return {};
    }

    fs::path path(argv[1]);
    Logger::info(std::format("CLI file argument: {}", path.string()));
    return path;
}
