#include "./includes/utils/fs.hpp"
#include "./includes/utils/logger.hpp"
#include "./includes/ui/ui.hpp"

#include <fstream>
#include <iostream>
#include <ncurses.h>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

void read_json_config(const std::string &configfile)
{
    std::ifstream file(configfile);

    if (!file.is_open())
    {
        std::cerr
            << "Failed to open config file: "
            << configfile << '\n';

        return;
    }

    nlohmann::json data;

    file >> data;

    std::cout << data.dump(4) << '\n';
}
void test_create_file(FS &filesystem)
{
    std::cout << "\n--- create_file ---\n";

    fs::path path = "./test/dogs/ninjas/test.txt";

    bool result = filesystem.create_file(path);

    std::cout
        << "Create file result: "
        << result
        << '\n';
}
void test_write_file(FS &filesystem)
{
    std::cout << "\n--- write_file ---\n";

    fs::path path = "./test/dogs/ninjas/test.txt";

    bool result = filesystem.write_file(
        path,
        "Hello from C++!\n"
        "This is my test file.\n"
        "I am learning C++ filesystem.\n");

    std::cout
        << "Write result: "
        << result
        << '\n';
}
void test_append_file(FS &filesystem)
{
    std::cout << "\n--- append_file ---\n";

    fs::path path = "./test/dogs/ninjas/test.txt";

    bool result = filesystem.append_file(
        path,
        "This line was appended.\n");

    std::cout
        << "Append result: "
        << result
        << '\n';
}
void test_read_file(FS &filesystem)
{
    std::cout << "\n--- read_file ---\n";

    fs::path path = "./test/dogs/ninjas/test.txt";

    auto content = filesystem.read_file(path);

    if (content)
    {
        std::cout << "File content:\n";
        std::cout << *content;
    }
    else
    {
        std::cout << "Failed to read file.\n";
    }
}
void test_file_info(FS &filesystem)
{
    std::cout << "\n--- file information ---\n";

    fs::path file = "./test/dogs/ninjas/test.txt";
    fs::path directory = "./test/dogs/ninjas";

    std::cout
        << "Exists: "
        << filesystem.exists(file)
        << '\n';

    std::cout
        << "Is file: "
        << filesystem.is_file(file)
        << '\n';

    std::cout
        << "Is directory: "
        << filesystem.is_directory(directory)
        << '\n';

    std::cout
        << "File size: "
        << filesystem.file_size(file)
        << " bytes\n";
}
void test_directory(FS &filesystem)
{
    std::cout << "\n--- directory operations ---\n";

    fs::path directory =
        "./test/example_directory";

    bool created =
        filesystem.createDirectory(directory);

    std::cout
        << "Directory created: "
        << created
        << '\n';

    std::cout << "Directory exists: "
              << filesystem.exists(directory)
              << '\n';
}
void test_list_directory(FS &filesystem)
{
    std::cout << "\n--- list directory ---\n";

    fs::path directory =
        "./test/dogs/ninjas";

    std::vector<fs::path> items =
        filesystem.listDirectory(directory);

    for (const auto &item : items)
    {
        std::cout << item << '\n';
    }
}
void test_copy_file(FS &filesystem)
{
    std::cout << "\n--- copy_file ---\n";

    fs::path source =
        "./test/dogs/ninjas/test.txt";

    fs::path destination =
        "./test/dogs/ninjas/test_copy.txt";

    bool result =
        filesystem.copy_file(
            source,
            destination);

    std::cout
        << "Copy result: "
        << result
        << '\n';
}
void test_rename_file(FS &filesystem)
{
    std::cout << "\n--- rename_file ---\n";

    fs::path oldpath =
        "./test/dogs/ninjas/test_copy.txt";

    fs::path newpath =
        "./test/dogs/ninjas/renamed.txt";

    bool result =
        filesystem.rename_file(
            oldpath,
            newpath);

    std::cout
        << "Rename result: "
        << result
        << '\n';
}
void test_paths(FS &filesystem)
{
    std::cout << "\n--- path operations ---\n";

    fs::path path =
        "./test/dogs/ninjas/test.txt";

    std::cout
        << "Current path: "
        << filesystem.currentPath()
        << '\n';

    std::cout
        << "Absolute path: "
        << filesystem.absolute(path)
        << '\n';

    std::cout
        << "Canonical path: "
        << filesystem.canonical(path)
        << '\n';

    std::cout
        << "Weak canonical path: "
        << filesystem.weaklyCanonical(path)
        << '\n';
}
void test_delete_file(FS &filesystem)
{
    std::cout << "\n--- delete_file ---\n";

    fs::path path =
        "./test/dogs/ninjas/renamed.txt";

    bool result =
        filesystem.delete_file(path);

    std::cout
        << "Delete result: "
        << result
        << '\n';
}
void test_delete_directory(FS &filesystem)
{
    std::cout << "\n--- deleteDirectory ---\n";

    fs::path path =
        "./test/example_directory";

    bool result =
        filesystem.deleteDirectory(path);

    std::cout
        << "Delete directory result: "
        << result
        << '\n';
}

int main()
{
    FS filesystem;
    UI u;

    std::cout << "========== FS TEST ==========\n";
    test_create_file(filesystem);
    test_write_file(filesystem);
    test_append_file(filesystem);
    test_read_file(filesystem);
    test_file_info(filesystem);
    test_directory(filesystem);
    test_list_directory(filesystem);
    test_copy_file(filesystem);
    test_rename_file(filesystem);
    test_paths(filesystem);
    test_delete_file(filesystem);
    test_delete_directory(filesystem);
    std::cout
        << "\n========== JSON TEST ==========\n";
    read_json_config("config.json");

    std::cout
        << "\nPress ENTER to start ncurses...\n";

    std::cin.get();
    u.init();
    mvprintw(5, 10, "Hello, ncurses!");
    mvprintw(7, 10, "Press any key to exit.");
    refresh();
    getch();
    endwin();

    return 0;
}