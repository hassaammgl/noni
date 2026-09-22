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

bool FS::copy_file(const fs::path &from, const fs::path &to)
{
    try
    {
        if (!fs::exists(from))
        {
            Logger::error("FS Error [copy_file]: Source file does not exist.");
            return false;
        }

        const bool copied = fs::copy_file(from, to, fs::copy_options::overwrite_existing);
        if (copied)
            Logger::info(std::format("File copied: {} -> {}", from.string(), to.string()));
        return copied;
    }
    catch (const fs::filesystem_error &e)
    {
        Logger::error(std::format("FS Error [copy_file]: {}", e.what()));
        return false;
    }
    catch (const std::exception &e)
    {
        Logger::error(std::format("General Error [copy_file]: {}", e.what()));
        return false;
    }
}

bool FS::create_directory(const fs::path &path)
{
    try
    {
        fs::create_directories(path);
        if (fs::is_directory(path))
        {
            Logger::info(std::format("Directory ready: {}", path.string()));
            return true;
        }
        return false;
    }
    catch (const fs::filesystem_error &e)
    {
        Logger::error(std::format("FS Error [create_directory]: {}", e.what()));
        return false;
    }
    catch (const std::exception &e)
    {
        Logger::error(std::format("General Error [create_directory]: {}", e.what()));
        return false;
    }
}

bool FS::delete_directory(const fs::path &path)
{
    try
    {
        const auto removed = fs::remove_all(path);
        if (removed > 0)
            Logger::info(std::format("Directory deleted: {} ({} items)", path.string(), removed));
        return removed > 0;
    }
    catch (const fs::filesystem_error &e)
    {
        Logger::error(std::format("FS Error [delete_directory]: {}", e.what()));
        return false;
    }
    catch (const std::exception &e)
    {
        Logger::error(std::format("General Error [delete_directory]: {}", e.what()));
        return false;
    }
}

