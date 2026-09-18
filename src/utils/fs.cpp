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


bool FS::create_file(const fs::path &path)
{
    try
    {
        if (path.has_parent_path())
        {
            fs::create_directories(path.parent_path());
        }
        std::ofstream file;
        file.exceptions(std::ofstream::failbit | std::ofstream::badbit);
        file.open(path);
        file.close();
        Logger::info("File Created succesfully");

        return true;
    }
    catch (const std::exception &e)
    {
        Logger::error(std::format("General Error: {}", e.what()));

        return false;
    }
}

bool FS::write_file(
    const fs::path &path,
    const std::vector<std::string> &content)
{
    try
    {
        if (path.empty())
        {
            Logger::error("write_file: empty path");
            return false;
        }

        const fs::path parent = path.has_parent_path() ? path.parent_path() : fs::path(".");
        if (!parent.empty() && parent != ".")
            fs::create_directories(parent);

        // Unique temp beside the destination so rename can be atomic on the same filesystem.
        std::string pattern = (parent / (path.filename().string() + ".noni.XXXXXX")).string();
        std::vector<char> tmpl(pattern.begin(), pattern.end());
        tmpl.push_back('\0');

        const int fd = ::mkstemp(tmpl.data());
        if (fd < 0)
        {
            Logger::error(std::format(
                "write_file: mkstemp failed for {}: {}",
                path.string(),
                std::strerror(errno)));
            return false;
        }

        const fs::path temp_path(tmpl.data());
        bool rename_ok = false;

        // Ensure temp cleanup on all failure paths.
        struct TempGuard
        {
            fs::path path;
            bool keep = false;
            ~TempGuard()
            {
                if (!keep && !path.empty())
                {
                    std::error_code ec;
                    fs::remove(path, ec);
                }
            }
        } guard{temp_path, false};

        // New files: mkstemp creates 0600; honor the usual umask-derived default.
        std::error_code xec;
        if (!(fs::exists(path, xec) && fs::is_regular_file(path, xec)))
        {
            const mode_t mask = ::umask(0);
            ::umask(mask);
            (void)::fchmod(fd, static_cast<mode_t>(0666 & ~mask));
        }

        {
            // Close mkstemp fd; rewrite via ofstream for line-oriented write.
            ::close(fd);

            std::ofstream file(temp_path, std::ios::out | std::ios::trunc | std::ios::binary);
            if (!file)
            {
                Logger::error(std::format("write_file: could not open temp {}", temp_path.string()));
                return false;
            }

            for (const auto &line : content)
                file << line << '\n';

            file.flush();
            if (!file)
            {
                Logger::error(std::format("write_file: write failed for temp {}", temp_path.string()));
                return false;
            }

            // Best-effort durability of file data before rename.
            file.close();
            const int sync_fd = ::open(temp_path.c_str(), O_RDONLY);
            if (sync_fd >= 0)
            {
                (void)::fsync(sync_fd);
                ::close(sync_fd);
            }
        }

        // Preserve destination mode bits when replacing an existing file (best-effort).
        std::error_code pec;
        if (fs::exists(path, pec) && fs::is_regular_file(path, pec))
        {
            const auto st = fs::status(path, pec);
            if (!pec)
                fs::permissions(temp_path, st.permissions(), pec);
        }

        std::error_code rec;
        fs::rename(temp_path, path, rec);
        if (rec)
        {
            Logger::error(std::format(
                "write_file: rename {} → {} failed: {}",
                temp_path.string(),
                path.string(),
                rec.message()));
            return false;
        }

        guard.keep = true; // temp now is the destination
        rename_ok = true;

        // Best-effort directory fsync so the rename itself is durable.
        const int dir_fd = ::open(parent.c_str(), O_RDONLY | O_DIRECTORY);
        if (dir_fd >= 0)
        {
            (void)::fsync(dir_fd);
            ::close(dir_fd);
        }

        Logger::info(std::format("File written atomically: {}", path.string()));
        return rename_ok;
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