#pragma once
#include <utils/fs.hpp>
#include <utils/str.hpp>
#include <vector>
#include <string>

class Buffer
{
private:
    fs::path bufferPath;
    std::vector<std::string> content;
    FS fs;

public:
    Buffer() = default;
    ~Buffer() = default;
    // set path
    void setBufferPath(const fs::path &projectPath);
    // get path
    fs::path getBufferPath();
    // read buffer
    std::vector<std::string> readBuffer();
    // write buffer
    bool writeBuffer(std::vector<std::string> &);
    // delete buffer
    bool deleteBuffer(const fs::path &bufferPath);
};