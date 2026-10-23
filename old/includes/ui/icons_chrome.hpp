#pragma once

#include <cctype>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

namespace Icons
{
    // Debug & run
    constexpr const wchar_t *debug = L"\uead8";
    constexpr const wchar_t *debug_alt = L"\ueb91";
    constexpr const wchar_t *debug_start = L"\uead3";
    constexpr const wchar_t *debug_stop = L"\uead7";
    constexpr const wchar_t *debug_pause = L"\uead1";
    constexpr const wchar_t *debug_continue = L"\ueacf";
    constexpr const wchar_t *debug_restart = L"\uead2";
    constexpr const wchar_t *debug_disconnect = L"\uead0";
    constexpr const wchar_t *debug_step_into = L"\uead4";
    constexpr const wchar_t *debug_step_out = L"\uead5";
    constexpr const wchar_t *debug_step_over = L"\uead6";
    constexpr const wchar_t *debug_step_back = L"\ueb8f";
    constexpr const wchar_t *debug_console = L"\ueb9b";
    constexpr const wchar_t *debug_breakpoint = L"\uea71";
    constexpr const wchar_t *debug_stackframe = L"\ueb8b";
    constexpr const wchar_t *bug = L"\ueaaf";
    constexpr const wchar_t *play = L"\ueb2c";
    constexpr const wchar_t *run = L"\ueb2c";
    constexpr const wchar_t *run_all = L"\ueb9e";
    constexpr const wchar_t *run_errors = L"\uebde";
    constexpr const wchar_t *beaker = L"\uea79";
    constexpr const wchar_t *coverage = L"\uec2e";

    // Terminal & output
    constexpr const wchar_t *terminal = L"\uea85";
    constexpr const wchar_t *terminal_bash = L"\uebca";
    constexpr const wchar_t *terminal_linux = L"\uebc6";
    constexpr const wchar_t *output = L"\ueb9d";
    constexpr const wchar_t *console = L"\uea85";
    constexpr const wchar_t *server = L"\ueb50";
    constexpr const wchar_t *database = L"\ueace";
    constexpr const wchar_t *cloud = L"\uebaa";
    constexpr const wchar_t *remote = L"\ueb3a";
    constexpr const wchar_t *package_icon = L"\ueb29";
    constexpr const wchar_t *extensions = L"\ueae6";
    constexpr const wchar_t *library = L"\ueb9c";
    constexpr const wchar_t *notebook = L"\uebaf";

    // Status & diagnostics
    constexpr const wchar_t *error = L"\uea87";
    constexpr const wchar_t *error_small = L"\uebfb";
    constexpr const wchar_t *warning = L"\uea6c";
    constexpr const wchar_t *info = L"\uea74";
    constexpr const wchar_t *question = L"\ueb32";
    constexpr const wchar_t *lightbulb = L"\uea61";
    constexpr const wchar_t *lightbulb_autofix = L"\ueb13";
    constexpr const wchar_t *pass = L"\ueba4";
    constexpr const wchar_t *pass_filled = L"\uebb3";
    constexpr const wchar_t *loading = L"\ueb19";
    constexpr const wchar_t *bell = L"\ueaa2";
    constexpr const wchar_t *bell_dot = L"\ueb9a";
    constexpr const wchar_t *lock = L"\uea75";
    constexpr const wchar_t *unlock = L"\ueb74";
    constexpr const wchar_t *eye = L"\uea70";
    constexpr const wchar_t *eye_closed = L"\ueae7";
    constexpr const wchar_t *verified = L"\ueb77";
    constexpr const wchar_t *unverified = L"\ueb76";

    // Settings & tools
    constexpr const wchar_t *gear = L"\ueaf8";
    constexpr const wchar_t *settings = L"\ueb52";
    constexpr const wchar_t *settings_gear = L"\ueb51";
    constexpr const wchar_t *tools = L"\ueb6d";
    constexpr const wchar_t *wrench = L"\ueb65";
    constexpr const wchar_t *keyboard = L"\uea65";
    constexpr const wchar_t *color_mode = L"\ueac6";
    constexpr const wchar_t *symbol_color = L"\ueb5c";
    constexpr const wchar_t *inspect = L"\uebd1";
    constexpr const wchar_t *references = L"\ueb36";
    constexpr const wchar_t *telescope = L"\ueb68";
    constexpr const wchar_t *home = L"\ueb06";
    constexpr const wchar_t *project = L"\ueb30";
    constexpr const wchar_t *book = L"\ueaa4";
    constexpr const wchar_t *comment = L"\uea6b";
    constexpr const wchar_t *comment_discussion = L"\ueac7";
    constexpr const wchar_t *link = L"\ueb15";
    constexpr const wchar_t *link_external = L"\ueb14";
    constexpr const wchar_t *share = L"\uec25";
    constexpr const wchar_t *export_icon = L"\uebac";
    constexpr const wchar_t *attach = L"\uec34";
    constexpr const wchar_t *key = L"\ueb11";
    constexpr const wchar_t *shield = L"\ueb53";
    constexpr const wchar_t *flame = L"\ueaf2";
    constexpr const wchar_t *zap = L"\uea86";
    constexpr const wchar_t *rocket = L"\ueb44";
    constexpr const wchar_t *sparkle = L"\uec10";
    constexpr const wchar_t *copilot = L"\uec1e";
    constexpr const wchar_t *robot = L"\uec20";
    constexpr const wchar_t *wand = L"\uebcf";
    constexpr const wchar_t *mcp = L"\uec47";

    // Code symbols (outline / breadcrumbs)
    constexpr const wchar_t *symbol_file = L"\ueb60";
    constexpr const wchar_t *symbol_module = L"\uea8b";
    constexpr const wchar_t *symbol_namespace = L"\uea8b";
    constexpr const wchar_t *symbol_package = L"\uea8b";
    constexpr const wchar_t *symbol_class = L"\ueb5b";
    constexpr const wchar_t *symbol_method = L"\uea8c";
    constexpr const wchar_t *symbol_function = L"\uea8c";
    constexpr const wchar_t *symbol_variable = L"\uea88";
    constexpr const wchar_t *symbol_field = L"\ueb5f";
    constexpr const wchar_t *symbol_property = L"\ueb65";
    constexpr const wchar_t *symbol_enum = L"\uea95";
    constexpr const wchar_t *symbol_enum_member = L"\ueb5e";
    constexpr const wchar_t *symbol_interface = L"\ueb61";
    constexpr const wchar_t *symbol_struct = L"\uea91";
    constexpr const wchar_t *symbol_event = L"\uea86";
    constexpr const wchar_t *symbol_operator = L"\ueb64";
    constexpr const wchar_t *symbol_keyword = L"\ueb62";
    constexpr const wchar_t *symbol_snippet = L"\ueb66";
    constexpr const wchar_t *symbol_string = L"\ueb8d";
    constexpr const wchar_t *symbol_number = L"\uea90";
    constexpr const wchar_t *symbol_boolean = L"\uea8f";
    constexpr const wchar_t *symbol_array = L"\uea8a";
    constexpr const wchar_t *symbol_constant = L"\ueb5d";
    constexpr const wchar_t *symbol_key = L"\uea93";
    constexpr const wchar_t *symbol_parameter = L"\uea92";
    constexpr const wchar_t *symbol_misc = L"\ueb63";
    constexpr const wchar_t *bracket = L"\ueb0f";
    constexpr const wchar_t *bracket_error = L"\uebe6";
    constexpr const wchar_t *bracket_dot = L"\uebe5";
    constexpr const wchar_t *type_hierarchy = L"\uebb9";
}
