#pragma once

#include <filesystem>

namespace fs = std::filesystem;

class Args
{
public:
    static fs::path getFilePath(
        int argc,
        char *argv[]);
};