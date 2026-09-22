#include <utils/fs.hpp>
#include <utils/logger.hpp>

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <format>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

fs::path FS::current_path() const
{
    try
    {
        return fs::current_path();
    }
    catch (const fs::filesystem_error &e)
    {
        Logger::error(std::format("File System error: {}", e.what()));
        return {};
    }
}

bool FS::change_current_path(const fs::path &path)
{
    try
    {
        if (fs::exists(path))
        {
            fs::current_path(path);
            return true;
        }
        else
        {
            Logger::error(std::format("Error: Path does not exist or is not a directory: {}", path.string()));
            return false;
        }
    }
    catch (const fs::filesystem_error &e)
    {
        Logger::error(std::format("File System error: {} | Path: {}", e.what(), e.path1().string()));
        return false;
    }
    catch (const std::exception &e)
    {
        Logger::error(std::format("General Error: {}", e.what()));
        return false;
    }
}

fs::path FS::absolute(const fs::path &path) const
{
    try
    {
        return fs::absolute(path);
    }
    catch (const fs::filesystem_error &e)
    {
        Logger::error(std::format("File System error (absolute): {} | Path: {}", e.what(), e.path1().string()));
        return {};
    }
    catch (const std::exception &e)
    {
        Logger::error(std::format("General Error (absolute): {}", e.what()));
        return {};
    }
}

fs::path FS::canonical(const fs::path &path) const
{
    try
    {
        if (fs::exists(path))
        {
            return fs::canonical(path);
        }
        else
        {
            Logger::error(std::format("Canonical Error: Path does not exist on disk: {}", path.string()));
            return {};
        }
    }
    catch (const fs::filesystem_error &e)
    {
        Logger::error(std::format("File System error (canonical): {} | Path: {}", e.what(), e.path1().string()));
        return {};
    }
    catch (const std::exception &e)
    {
        Logger::error(std::format("General Error (canonical): {}", e.what()));
        return {};
    }
}

fs::path FS::weakly_canonical(const fs::path &path) const
{
    try
    {
        return fs::weakly_canonical(path);
    }
    catch (const fs::filesystem_error &e)
    {
        Logger::error(std::format("File System error (weakly_canonical): {} | Path: {}", e.what(), e.path1().string()));
        return {};
    }
    catch (const std::exception &e)
    {
        Logger::error(std::format("General Error (weakly_canonical): {}", e.what()));
        return {};
    }
}
