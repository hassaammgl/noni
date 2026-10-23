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

bool FS::append_file(const fs::path &path, const std::string &content)
{
    try
    {
        if (path.has_parent_path())
        {
            fs::create_directories(
                path.parent_path());
        }

        std::ofstream file(
            path,
            std::ios::app);

        if (!file)
        {
            Logger::error(std::format("Could not open file: {}", path.string()));

            return false;
        }

        file << content;

        file.close();

        Logger::info(std::format("File appended: {}", path.string()));
        return true;
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

std::optional<std::string> FS::read_file(const fs::path &path)
{
    const FsReadResult result = read_file_detailed(path);
    if (!result.ok)
        return std::nullopt;
    return result.content;
}

FsReadResult FS::read_file_detailed(const fs::path &path)
{
    FsReadResult result;
    try
    {
        if (path.empty())
        {
            result.error = "empty path";
            Logger::error("read_file: empty path");
            return result;
        }

        std::error_code ec;
        if (!fs::exists(path, ec) || ec)
        {
            result.error = ec ? ec.message() : "file does not exist";
            Logger::error(std::format("File does not exist: {}", path.string()));
            return result;
        }

        if (!fs::is_regular_file(path, ec) || ec)
        {
            result.error = "path is not a regular file";
            Logger::error(std::format("Path is not a file: {}", path.string()));
            return result;
        }

        std::ifstream file(path, std::ios::in | std::ios::binary);
        if (!file)
        {
            result.error = std::strerror(errno);
            if (!result.error.empty() && result.error == "Success")
                result.error = "could not open file";
            Logger::error(std::format("Could not open file: {}", path.string()));
            return result;
        }

        result.content.assign(
            std::istreambuf_iterator<char>(file),
            std::istreambuf_iterator<char>());

        if (file.bad())
        {
            result.content.clear();
            result.error = "read failed";
            Logger::error(std::format("Read failed: {}", path.string()));
            return result;
        }

        result.ok = true;
        return result;
    }
    catch (const fs::filesystem_error &e)
    {
        result.error = e.what();
        Logger::error(std::format("File System Error: {}", e.what()));
        return result;
    }
    catch (const std::exception &e)
    {
        result.error = e.what();
        Logger::error(std::format("General Error: {}", e.what()));
        return result;
    }
}

bool FS::delete_file(const fs::path &path)
{
    try
    {
        if (!this->exists(path))
        {
            Logger::error(std::format("File does not exists on path: {}", path.string()));
            return false;
        }
        else
        {
            fs::remove(path);
            Logger::info(std::format("File deleted: {}", path.string()));
            return true;
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

bool FS::rename_file(const fs::path &oldfile, const fs::path &newpath)
{
    try
    {
        if (!fs::exists(oldfile))
        {
            Logger::error("FS Error [rename_file]: Source path does not exist.");
            return false;
        }

        fs::rename(oldfile, newpath);
        Logger::info(std::format("File renamed: {} -> {}", oldfile.string(), newpath.string()));
        return true;
    }
    catch (const fs::filesystem_error &e)
    {
        Logger::error(std::format("FS Error [rename_file]: {}", e.what()));
        return false;
    }
    catch (const std::exception &e)
    {
        Logger::error(std::format("General Error [rename_file]: {}", e.what()));
        return false;
    }
}

