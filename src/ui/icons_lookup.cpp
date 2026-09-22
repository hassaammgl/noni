#include <ui/icons.hpp>
#include "icons_lookup.hpp"

namespace Icons
{
    const wchar_t *for_file(const fs::path &path)
    {
        const std::string name = lower_ascii(path.filename().string());
        const std::string ext = lower_ascii(path.extension().string());
        if (const wchar_t *p = for_file_part1(name, ext)) return p;
        if (const wchar_t *p = for_file_part2(name, ext)) return p;
        if (const wchar_t *p = for_file_part3(name, ext)) return p;
        if (const wchar_t *p = for_file_part4(name, ext)) return p;
        if (const wchar_t *p = for_file_part5(name, ext)) return p;
        return file;
    }
}
