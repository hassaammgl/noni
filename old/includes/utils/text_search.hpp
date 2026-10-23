#pragma once

#include <atomic>
#include <filesystem>
#include <mutex>
#include <string>
#include <vector>

namespace fs = std::filesystem;

struct TextMatch
{
    fs::path path;
    int line = 0;   // 0-based
    int column = 0; // 0-based
    std::string preview;
};

struct TextSearchOptions
{
    bool match_case = false;
    bool whole_word = false;
    bool use_regex = false;
};

class TextSearch
{
public:
    void set_root(const fs::path &root);
    void search_async(const std::string &query, TextSearchOptions opts);
    void cancel();

    bool is_searching() const;
    std::uint64_t version() const;
    std::vector<TextMatch> results() const;
    std::string status() const;

private:
    mutable std::mutex mu;
    fs::path root_path;
    std::vector<TextMatch> matches;
    std::string status_text;
    std::atomic<bool> searching{false};
    std::atomic<std::uint64_t> ver{0};
    std::atomic<std::uint64_t> job_token{0};

    static bool should_skip_dir(const std::string &name);
    static bool looks_binary(const std::string &sample);
    static std::vector<TextMatch> scan(
        const fs::path &root,
        const std::string &query,
        TextSearchOptions opts,
        const std::atomic<std::uint64_t> &token,
        std::uint64_t my_token);
};
