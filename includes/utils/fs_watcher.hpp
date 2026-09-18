#pragma once

#include <cstdint>
#include <filesystem>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

enum class FsEventKind
{
    Modified,
    Created,
    Deleted,
    Moved,
    Other,
};

struct FsEvent
{
    FsEventKind kind = FsEventKind::Other;
    fs::path path;
    bool is_dir = false;
};

// Minimal POSIX inotify watcher. Non-blocking; drain from the UI loop.
// No ncurses. Not a full recursive project indexer.
class FsWatcher
{
public:
    FsWatcher() = default;
    ~FsWatcher();

    FsWatcher(const FsWatcher &) = delete;
    FsWatcher &operator=(const FsWatcher &) = delete;

    bool start();
    void stop();
    bool running() const { return fd_ >= 0; }

    // Watch a directory (non-recursive) or a single file.
    bool watch(const fs::path &path);
    void unwatch(const fs::path &path);
    void clear();

    // Replace workspace directory watch (keeps per-file watches).
    void set_workspace(const fs::path &root);

    // Non-blocking drain of pending events (may coalesce).
    std::vector<FsEvent> poll();

private:
    int fd_ = -1;
    std::mutex mu_;
    std::unordered_map<int, fs::path> wd_to_path_;
    std::unordered_map<std::string, int> path_to_wd_;
    fs::path workspace_;

    bool add_watch_unlocked(const fs::path &path, uint32_t mask);
    void remove_wd_unlocked(int wd);
    static bool should_ignore(const fs::path &path);
};
