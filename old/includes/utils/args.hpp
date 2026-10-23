#pragma once

#include <filesystem>

namespace fs = std::filesystem;

class Args
{
public:
    static fs::path get_file_path(int argc, char *argv[]);
};