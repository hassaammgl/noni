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

