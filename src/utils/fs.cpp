#include "utils/fs.hpp"
#include "utils/logger.hpp"
#include <fstream>
#include <iostream>

Logger logger;

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
        logger.info("File Created succesfully");
        return true;
    }
    catch (const fs::filesystem_error &e)
    {
        std::cerr << "File System error: " << e.what() << '\n';
        std::cerr << "Path: " << e.path1() << '\n';
        logger.error("Error while creating file...");
        return false;
    }
    catch (const std::ios_base::failure &e)
    {
        std::cerr << "FILE I/O Error: " << e.what() << '\n';
        logger.error("Error while creating file...");
        return false;
    }
    catch (const std::exception &e)
    {
        std::cerr << "General Error: " << e.what() << '\n';
        logger.error("Error while creating file...");
        return false;
    }
}

bool FS::write_file(const fs::path &path, const std::string &content)
{
    try
    {
        if (path.has_parent_path())
        {
            fs::create_directories(
                path.parent_path());
        }

        std::ofstream file(path);

        if (!file)
        {
            std::cerr
                << "Could not open file: "
                << path << '\n';

            return false;
        }

        file << content;

        file.close();

        return true;
    }
    catch (const fs::filesystem_error &e)
    {
        std::cerr
            << "File System Error: "
            << e.what() << '\n';

        return false;
    }
    catch (const std::exception &e)
    {
        std::cerr
            << "General Error: "
            << e.what() << '\n';

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
            std::cerr
                << "Could not open file: "
                << path << '\n';

            return false;
        }

        file << content;

        file.close();

        return true;
    }
    catch (const fs::filesystem_error &e)
    {
        std::cerr
            << "File System Error: "
            << e.what() << '\n';

        return false;
    }
    catch (const std::exception &e)
    {
        std::cerr
            << "General Error: "
            << e.what() << '\n';

        return false;
    }
}

std::optional<std::string> FS::read_file(const fs::path &path)
{
    try
    {
        if (!fs::exists(path))
        {
            std::cerr
                << "File does not exist: "
                << path << '\n';

            return std::nullopt;
        }

        if (!fs::is_regular_file(path))
        {
            std::cerr
                << "Path is not a file: "
                << path << '\n';

            return std::nullopt;
        }

        std::ifstream file(path);

        if (!file)
        {
            std::cerr
                << "Could not open file: "
                << path << '\n';

            return std::nullopt;
        }

        std::string content{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};

        file.close();

        return content;
    }
    catch (const fs::filesystem_error &e)
    {
        std::cerr
            << "File System Error: "
            << e.what() << '\n';

        return std::nullopt;
    }
    catch (const std::exception &e)
    {
        std::cerr
            << "General Error: "
            << e.what() << '\n';

        return std::nullopt;
    }
}

bool FS::delete_file(const fs::path &path)
{
    try
    {
        if (!this->exists(path))
        {
            std::cerr << "File does not exists on path: " << path << "\n";
            return false;
        }
        else
        {
            fs::remove(path);
            return true;
        }
    }
    catch (const fs::filesystem_error &e)
    {
        std::cerr << "File System error: " << e.what() << '\n';
        std::cerr << "Path: " << e.path1() << '\n';
        return false;
    }
    catch (const std::exception &e)
    {
        std::cerr << "General Error: " << e.what() << '\n';
        return false;
    }
}

bool FS::rename_file(const fs::path &oldfile, const fs::path &newpath)
{
    try
    {
        if (!fs::exists(oldfile))
        {
            std::cerr << "FS Error [rename_file]: Source path does not exist.\n";
            return false;
        }

        fs::rename(oldfile, newpath);
        return true;
    }
    catch (const fs::filesystem_error &e)
    {
        std::cerr << "FS Error [rename_file]: " << e.what() << '\n';
        return false;
    }
    catch (const std::exception &e)
    {
        std::cerr << "General Error [rename_file]: " << e.what() << '\n';
        return false;
    }
}

bool FS::copy_file(const fs::path &from, const fs::path &to)
{
    try
    {
        if (!fs::exists(from))
        {
            std::cerr << "FS Error [copy_file]: Source file does not exist.\n";
            return false;
        }

        return fs::copy_file(from, to, fs::copy_options::overwrite_existing);
    }
    catch (const fs::filesystem_error &e)
    {
        std::cerr << "FS Error [copy_file]: " << e.what() << '\n';
        return false;
    }
    catch (const std::exception &e)
    {
        std::cerr << "General Error [copy_file]: " << e.what() << '\n';
        return false;
    }
}

bool FS::createDirectory(const fs::path &path)
{
    try
    {
        return fs::create_directories(path);
    }
    catch (const fs::filesystem_error &e)
    {
        std::cerr << "FS Error [createDirectory]: " << e.what() << '\n';
        return false;
    }
    catch (const std::exception &e)
    {
        std::cerr << "General Error [createDirectory]: " << e.what() << '\n';
        return false;
    }
}

bool FS::deleteDirectory(const fs::path &path)
{
    try
    {
        return fs::remove_all(path) > 0;
    }
    catch (const fs::filesystem_error &e)
    {
        std::cerr << "FS Error [deleteDirectory]: " << e.what() << '\n';
        return false;
    }
    catch (const std::exception &e)
    {
        std::cerr << "General Error [deleteDirectory]: " << e.what() << '\n';
        return false;
    }
}

std::vector<fs::path> FS::listDirectory(const fs::path &path)
{
    std::vector<fs::path> items;

    try
    {
        if (!fs::exists(path))
        {
            std::cerr << "FS Error [listDirectory]: Path does not exist.\n";
            return items;
        }

        if (!fs::is_directory(path))
        {
            std::cerr << "FS Error [listDirectory]: Path is not a directory.\n";
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
        std::cerr << "FS Error [listDirectory]: " << e.what() << '\n';
    }
    catch (const std::exception &e)
    {
        std::cerr << "General Error [listDirectory]: " << e.what() << '\n';
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
        std::cerr
            << "File System Error: "
            << e.what() << '\n';

        return false;
    }
    catch (const std::exception &e)
    {
        std::cerr
            << "General Error: "
            << e.what() << '\n';

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
        std::cerr << "File System error: " << e.what() << '\n';
        std::cerr << "Path: " << e.path1() << '\n';
        return false;
    }
    catch (const std::exception &e)
    {
        std::cerr << "General Error: " << e.what() << '\n';
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
        std::cerr << "File System error: " << e.what() << '\n';
        std::cerr << "Path: " << e.path1() << '\n';
        return false;
    }
    catch (const std::exception &e)
    {
        std::cerr << "General Error: " << e.what() << '\n';
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
            std::cerr << "Path is directory or something else: " << path << "\n";
            return false;
        }
    }
    catch (const fs::filesystem_error &e)
    {
        std::cerr << "File System error: " << e.what() << '\n';
        std::cerr << "Path: " << e.path1() << '\n';
        return false;
    }
    catch (const std::exception &e)
    {
        std::cerr << "General Error: " << e.what() << '\n';
        return false;
    }
}

fs::path FS::currentPath() const
{
    try
    {
        return fs::current_path();
    }
    catch (const fs::filesystem_error &e)
    {
        std::cerr << "File System error: " << e.what() << '\n';
        return {};
    }
}

bool FS::changeCurrentPath(const fs::path &path)
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
            std::cerr << "Error: Path does not exist or is not a directory: " << path << '\n';
            return false;
        }
    }
    catch (const fs::filesystem_error &e)
    {
        std::cerr << "File System error: " << e.what() << '\n';
        std::cerr << "Path: " << e.path1() << '\n';
        return false;
    }
    catch (const std::exception &e)
    {
        std::cerr << "General Error: " << e.what() << '\n';
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
        std::cerr << "File System error (absolute): " << e.what() << '\n';
        std::cerr << "Path: " << e.path1() << '\n';
        return {};
    }
    catch (const std::exception &e)
    {
        std::cerr << "General Error (absolute): " << e.what() << '\n';
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
            std::cerr << "Canonical Error: Path does not exist on disk: " << path << '\n';
            return {};
        }
    }
    catch (const fs::filesystem_error &e)
    {
        std::cerr << "File System error (canonical): " << e.what() << '\n';
        std::cerr << "Path: " << e.path1() << '\n';
        return {};
    }
    catch (const std::exception &e)
    {
        std::cerr << "General Error (canonical): " << e.what() << '\n';
        return {};
    }
}

fs::path FS::weaklyCanonical(const fs::path &path) const
{
    try
    {
        return fs::weakly_canonical(path);
    }
    catch (const fs::filesystem_error &e)
    {
        std::cerr << "File System error (weaklyCanonical): " << e.what() << '\n';
        std::cerr << "Path: " << e.path1() << '\n';
        return {};
    }
    catch (const std::exception &e)
    {
        std::cerr << "General Error (weaklyCanonical): " << e.what() << '\n';
        return {};
    }
}