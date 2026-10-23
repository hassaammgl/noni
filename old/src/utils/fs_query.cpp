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

std::vector<fs::path> FS::list_directory(const fs::path &path)
{
    std::vector<fs::path> items;

    try
    {
        if (!fs::exists(path))
        {
            Logger::error("FS Error [list_directory]: Path does not exist.");
            return items;
        }

        if (!fs::is_directory(path))
        {
            Logger::error("FS Error [list_directory]: Path is not a directory.");
            return items;
        }

        auto options = fs::directory_options::skip_permission_denied;

        for (const auto &entry : fs::directory_iterator(path, options))
        {
            items.push_back(entry.path());
        }
    }
    catch (const fs::filesystem_error &e)
    {
        Logger::error(std::format("FS Error [list_directory]: {}", e.what()));
    }
    catch (const std::exception &e)
    {
        Logger::error(std::format("General Error [list_directory]: {}", e.what()));
    }

    return items;
}

bool FS::exists(const fs::path &path) const
{
    try
    {
        return fs::exists(path);
    }
    catch (const fs::filesystem_error &e)
    {
        Logger::error(std::format("File System Error: {}", e.what()));

        return false;
    }
    catch (const std::exception &e)
    {
        Logger::error(std::format("General Error: {}", e.what()));

        return false;
    }
}

bool FS::is_file(const fs::path &path) const
{
    try
    {
        if (fs::is_regular_file(path))
        {
            return true;
        }
        else
        {
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

bool FS::is_directory(const fs::path &path) const
{
    try
    {
        if (fs::is_directory(path))
        {
            return true;
        }
        else
        {
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

uintmax_t FS::file_size(const fs::path &path) const
{
    try
    {
        if (this->exists(path) && !this->is_directory(path))
        {
            return fs::file_size(path);
        }
        else
        {
            Logger::error(std::format("Path is directory or something else: {}", path.string()));
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

