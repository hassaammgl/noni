#pragma once

#include <cctype>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

namespace Icons
{
    // Files & folders
    constexpr const wchar_t *folder = L"\uea83";
    constexpr const wchar_t *folder_opened = L"\ueaf7 ";
    constexpr const wchar_t *folder_active = L"\ueaf6";
    constexpr const wchar_t *root_folder = L"\ueb46";
    constexpr const wchar_t *root_folder_opened = L"\ueb45";
    constexpr const wchar_t *new_folder = L"\uea80";
    constexpr const wchar_t *file = L"\uf15c";
    constexpr const wchar_t *file_text = L"\uea7b";
    constexpr const wchar_t *file_code = L"\ueae9";
    constexpr const wchar_t *file_binary = L"\ueae8";
    constexpr const wchar_t *file_media = L"\ueaea";
    constexpr const wchar_t *file_pdf = L"\ueaeb";
    constexpr const wchar_t *file_zip = L"\ueaef";
    constexpr const wchar_t *file_symlink = L"\ueaee";
    constexpr const wchar_t *file_symlink_dir = L"\ueaed";
    constexpr const wchar_t *new_file = L"\uea7f";
    constexpr const wchar_t *files = L"\ueaf0";
    constexpr const wchar_t *go_to_file = L"\uea94";
    constexpr const wchar_t *json = L"\ueb0f";
    constexpr const wchar_t *markdown = L"\ueb1d";
    constexpr const wchar_t *ruby = L"\ueb48";
    constexpr const wchar_t *python = L"\uec39";

    // Editor actions
    constexpr const wchar_t *edit = L"\uea73";
    constexpr const wchar_t *save = L"\ueb4b";
    constexpr const wchar_t *save_as = L"\ueb4a";
    constexpr const wchar_t *save_all = L"\ueb49";
    constexpr const wchar_t *close = L"\uea76";
    constexpr const wchar_t *close_all = L"\ueac1";
    constexpr const wchar_t *trash = L"\uea81";
    constexpr const wchar_t *copy = L"\uebcc";
    constexpr const wchar_t *clippy = L"\ueac0";
    constexpr const wchar_t *redo = L"\uebb0";
    constexpr const wchar_t *discard = L"\ueae2";
    constexpr const wchar_t *refresh = L"\ueb37";
    constexpr const wchar_t *sync = L"\uea77";
    constexpr const wchar_t *add = L"\uea60";
    constexpr const wchar_t *remove = L"\ueb3b";
    constexpr const wchar_t *check = L"\ueab2";
    constexpr const wchar_t *check_all = L"\uebb1";
    constexpr const wchar_t *clear_all = L"\ueabf";
    constexpr const wchar_t *insert = L"\uec11";
    constexpr const wchar_t *surround_with = L"\uec24";
    constexpr const wchar_t *indent = L"\uebf9";
    constexpr const wchar_t *word_wrap = L"\ueb80";
    constexpr const wchar_t *whitespace = L"\ueb7d";
    constexpr const wchar_t *newline = L"\uebea";
    constexpr const wchar_t *selection = L"\ueb85";
    constexpr const wchar_t *pin = L"\ueb2b";
    constexpr const wchar_t *pinned = L"\ueba0";
    constexpr const wchar_t *bookmark = L"\ueaa5";
    constexpr const wchar_t *open_preview = L"\ueb28";
    constexpr const wchar_t *preview = L"\ueb2f";

    // Search & replace
    constexpr const wchar_t *search = L"\uea6d";
    constexpr const wchar_t *search_stop = L"\ueb4e";
    constexpr const wchar_t *search_fuzzy = L"\uec0d";
    constexpr const wchar_t *replace = L"\ueb3d";
    constexpr const wchar_t *replace_all = L"\ueb3c";
    constexpr const wchar_t *case_sensitive = L"\ueab1";
    constexpr const wchar_t *whole_word = L"\ueb7e";
    constexpr const wchar_t *regex = L"\ueb38";
    constexpr const wchar_t *preserve_case = L"\ueb2e";
    constexpr const wchar_t *filter = L"\ueaf1";
    constexpr const wchar_t *exclude = L"\ueae5";

    // Navigation & layout
    constexpr const wchar_t *chevron_down = L"\ueab4";
    constexpr const wchar_t *chevron_left = L"\ueab5";
    constexpr const wchar_t *chevron_right = L"\ueab6";
    constexpr const wchar_t *chevron_up = L"\ueab7";
    constexpr const wchar_t *arrow_down = L"\uea9a";
    constexpr const wchar_t *arrow_left = L"\uea9b";
    constexpr const wchar_t *arrow_right = L"\uea9c";
    constexpr const wchar_t *arrow_up = L"\ueaa1";
    constexpr const wchar_t *triangle_down = L"\ueb6e";
    constexpr const wchar_t *triangle_left = L"\ueb6f";
    constexpr const wchar_t *triangle_right = L"\ueb70";
    constexpr const wchar_t *triangle_up = L"\ueb71";
    constexpr const wchar_t *fold = L"\ueaf5";
    constexpr const wchar_t *fold_down = L"\ueaf3";
    constexpr const wchar_t *fold_up = L"\ueaf4";
    constexpr const wchar_t *unfold = L"\ueb73";
    constexpr const wchar_t *collapse_all = L"\ueac5";
    constexpr const wchar_t *expand_all = L"\ueb95";
    constexpr const wchar_t *list_tree = L"\ueb86";
    constexpr const wchar_t *list_flat = L"\ueb84";
    constexpr const wchar_t *list_unordered = L"\ueb17";
    constexpr const wchar_t *list_ordered = L"\ueb16";
    constexpr const wchar_t *menu = L"\ueb94";
    constexpr const wchar_t *ellipsis = L"\uea7c";
    constexpr const wchar_t *split_horizontal = L"\ueb56";
    constexpr const wchar_t *split_vertical = L"\ueb57";
    constexpr const wchar_t *editor_layout = L"\ueae3";
    constexpr const wchar_t *layout = L"\uebeb";
    constexpr const wchar_t *layout_sidebar_left = L"\uebf3";
    constexpr const wchar_t *layout_sidebar_right = L"\uebf4";
    constexpr const wchar_t *layout_panel = L"\uebf2";
    constexpr const wchar_t *layout_statusbar = L"\uebf5";
    constexpr const wchar_t *window = L"\ueb7f";
    constexpr const wchar_t *multiple_windows = L"\ueb23";
    constexpr const wchar_t *screen_full = L"\ueb4c";
    constexpr const wchar_t *screen_normal = L"\ueb4d";
    constexpr const wchar_t *zoom_in = L"\ueb81";
    constexpr const wchar_t *zoom_out = L"\ueb82";

    // Git & source control
    constexpr const wchar_t *git_branch = L"\uea68";
    constexpr const wchar_t *source_control = L"\uea68";
    constexpr const wchar_t *git_commit = L"\ueafc";
    constexpr const wchar_t *git_compare = L"\ueafd";
    constexpr const wchar_t *git_merge = L"\ueafe";
    constexpr const wchar_t *merge = L"\uebab";
    constexpr const wchar_t *git_pull_request = L"\uea64";
    constexpr const wchar_t *git_pull_request_closed = L"\uebda";
    constexpr const wchar_t *git_pull_request_draft = L"\uebdb";
    constexpr const wchar_t *git_pull_request_create = L"\uebbc";
    constexpr const wchar_t *git_stash = L"\uec26";
    constexpr const wchar_t *git_stash_apply = L"\uec27";
    constexpr const wchar_t *git_stash_pop = L"\uec28";
    constexpr const wchar_t *repo = L"\uea62";
    constexpr const wchar_t *repo_clone = L"\ueb3e";
    constexpr const wchar_t *repo_forked = L"\uea63";
    constexpr const wchar_t *repo_pull = L"\ueb40";
    constexpr const wchar_t *repo_push = L"\ueb41";
    constexpr const wchar_t *repo_fetch = L"\uec1d";
    constexpr const wchar_t *diff = L"\ueae1";
    constexpr const wchar_t *diff_added = L"\ueadc";
    constexpr const wchar_t *diff_modified = L"\ueade";
    constexpr const wchar_t *diff_removed = L"\ueadf";
    constexpr const wchar_t *diff_renamed = L"\ueae0";
    constexpr const wchar_t *diff_ignored = L"\ueadd";
    constexpr const wchar_t *history = L"\uea82";
    constexpr const wchar_t *tag = L"\uea66";
    constexpr const wchar_t *github = L"\uea84";
    constexpr const wchar_t *code_review = L"\uec37";

}
