#pragma once

#include <filesystem>
#include <string>
#include <cctype>

namespace fs = std::filesystem;

// Codicons (VS Code icon font). Requires a Nerd Font / Codicons-capable terminal font.
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

    // Language / filetype glyphs (Nerd Font Devicons / Seti / Codicons)
    constexpr const wchar_t *lang_c = L"\ue61e";
    constexpr const wchar_t *lang_cpp = L"\ue61d";
    constexpr const wchar_t *lang_csharp = L"\ue648";
    constexpr const wchar_t *lang_objc = L"\ue61e";
    constexpr const wchar_t *lang_h = L"\uf0fd";
    constexpr const wchar_t *lang_hpp = L"\uf0fd";
    constexpr const wchar_t *lang_js = L"\ue74e";
    constexpr const wchar_t *lang_ts = L"\ue628";
    constexpr const wchar_t *lang_jsx = L"\ue7ba";
    constexpr const wchar_t *lang_tsx = L"\ue7ba";
    constexpr const wchar_t *lang_vue = L"\ue6a0";
    constexpr const wchar_t *lang_svelte = L"\ue697";
    constexpr const wchar_t *lang_angular = L"\ue753";
    constexpr const wchar_t *lang_react = L"\ue7ba";
    constexpr const wchar_t *lang_html = L"\uf13b";
    constexpr const wchar_t *lang_css = L"\ue749";
    constexpr const wchar_t *lang_scss = L"\ue749";
    constexpr const wchar_t *lang_sass = L"\ue74b";
    constexpr const wchar_t *lang_less = L"\ue758";
    constexpr const wchar_t *lang_stylus = L"\ue759";
    constexpr const wchar_t *lang_rust = L"\ue7a8";
    constexpr const wchar_t *lang_go = L"\ue627";
    constexpr const wchar_t *lang_java = L"\ue738";
    constexpr const wchar_t *lang_kotlin = L"\ue634";
    constexpr const wchar_t *lang_scala = L"\ue737";
    constexpr const wchar_t *lang_groovy = L"\ue736";
    constexpr const wchar_t *lang_clojure = L"\ue768";
    constexpr const wchar_t *lang_lua = L"\ue620";
    constexpr const wchar_t *lang_vim = L"\ue62b";
    constexpr const wchar_t *lang_emacs = L"\ue632";
    constexpr const wchar_t *lang_toml = L"\ue6b2";
    constexpr const wchar_t *lang_yaml = L"\ue6a8";
    constexpr const wchar_t *lang_xml = L"\ue619";
    constexpr const wchar_t *lang_sql = L"\ue706";
    constexpr const wchar_t *lang_sh = L"\uf489";
    constexpr const wchar_t *lang_powershell = L"\ue70f";
    constexpr const wchar_t *lang_batch = L"\ue70f";
    constexpr const wchar_t *lang_dockerfile = L"\uf308";
    constexpr const wchar_t *lang_git = L"\ue702";
    constexpr const wchar_t *lang_cmake = L"\ue794";
    constexpr const wchar_t *lang_makefile = L"\ue779";
    constexpr const wchar_t *lang_ninja = L"\ue779";
    constexpr const wchar_t *lang_bazel = L"\ue63a";
    constexpr const wchar_t *lang_gradle = L"\ue70e";
    constexpr const wchar_t *lang_maven = L"\ue741";
    constexpr const wchar_t *lang_image = L"\uf1c5";
    constexpr const wchar_t *lang_font = L"\uf031";
    constexpr const wchar_t *lang_lock = L"\uf023";
    constexpr const wchar_t *lang_php = L"\ue73d";
    constexpr const wchar_t *lang_perl = L"\ue769";
    constexpr const wchar_t *lang_r = L"\ue68a";
    constexpr const wchar_t *lang_matlab = L"\ue61a";
    constexpr const wchar_t *lang_julia = L"\ue624";
    constexpr const wchar_t *lang_haskell = L"\ue61f";
    constexpr const wchar_t *lang_ocaml = L"\ue67a";
    constexpr const wchar_t *lang_fsharp = L"\ue65a";
    constexpr const wchar_t *lang_elixir = L"\ue62d";
    constexpr const wchar_t *lang_erlang = L"\ue65d";
    constexpr const wchar_t *lang_nim = L"\ue677";
    constexpr const wchar_t *lang_crystal = L"\ue62f";
    constexpr const wchar_t *lang_zig = L"\ue6a9";
    constexpr const wchar_t *lang_v = L"\ue6ac";
    constexpr const wchar_t *lang_dart = L"\ue631";
    constexpr const wchar_t *lang_swift = L"\ue755";
    constexpr const wchar_t *lang_fortran = L"\ue65d";
    constexpr const wchar_t *lang_cobol = L"\ue63a";
    constexpr const wchar_t *lang_pascal = L"\ue63a";
    constexpr const wchar_t *lang_assembly = L"\ue637";
    constexpr const wchar_t *lang_wasm = L"\ue6a5";
    constexpr const wchar_t *lang_solidity = L"\ue694";
    constexpr const wchar_t *lang_graphql = L"\ue662";
    constexpr const wchar_t *lang_prisma = L"\ue684";
    constexpr const wchar_t *lang_proto = L"\ue687";
    constexpr const wchar_t *lang_thrift = L"\ue6b1";
    constexpr const wchar_t *lang_terraform = L"\ue69a";
    constexpr const wchar_t *lang_pulumi = L"\ue6b4";
    constexpr const wchar_t *lang_ansible = L"\ue6a0";
    constexpr const wchar_t *lang_nginx = L"\ue776";
    constexpr const wchar_t *lang_apache = L"\ue630";
    constexpr const wchar_t *lang_latex = L"\ue69b";
    constexpr const wchar_t *lang_bibtex = L"\ue69b";
    constexpr const wchar_t *lang_asciidoc = L"\ue678";
    constexpr const wchar_t *lang_org = L"\ue633";
    constexpr const wchar_t *lang_rst = L"\ue679";
    constexpr const wchar_t *lang_textile = L"\ue6b3";
    constexpr const wchar_t *lang_csv = L"\uf1c0";
    constexpr const wchar_t *lang_tsv = L"\uf1c0";
    constexpr const wchar_t *lang_excel = L"\uf1c3";
    constexpr const wchar_t *lang_word = L"\uf1c2";
    constexpr const wchar_t *lang_powerpoint = L"\uf1c4";
    constexpr const wchar_t *lang_audio = L"\uf1c7";
    constexpr const wchar_t *lang_video = L"\uf1c8";
    constexpr const wchar_t *lang_3d = L"\uf1b2";
    constexpr const wchar_t *lang_diff = L"\uf440";
    constexpr const wchar_t *lang_patch = L"\uf440";
    constexpr const wchar_t *lang_ini = L"\ue615";
    constexpr const wchar_t *lang_env = L"\uf462";
    constexpr const wchar_t *lang_editorconfig = L"\ue652";
    constexpr const wchar_t *lang_npm = L"\ue71e";
    constexpr const wchar_t *lang_yarn = L"\ue6a7";
    constexpr const wchar_t *lang_pnpm = L"\ue683";
    constexpr const wchar_t *lang_bun = L"\ue6a4";
    constexpr const wchar_t *lang_node = L"\ue718";
    constexpr const wchar_t *lang_deno = L"\ue6a5";
    constexpr const wchar_t *lang_webpack = L"\ue6a3";
    constexpr const wchar_t *lang_vite = L"\ue6a5";
    constexpr const wchar_t *lang_eslint = L"\ue655";
    constexpr const wchar_t *lang_prettier = L"\ue6b4";
    constexpr const wchar_t *lang_babel = L"\ue639";
    constexpr const wchar_t *lang_jest = L"\ue66e";
    constexpr const wchar_t *lang_cypress = L"\ue65f";
    constexpr const wchar_t *lang_playwright = L"\ue683";
    constexpr const wchar_t *lang_storybook = L"\ue696";
    constexpr const wchar_t *lang_tailwind = L"\ue6a9";
    constexpr const wchar_t *lang_postcss = L"\ue685";
    constexpr const wchar_t *lang_sveltekit = L"\ue697";
    constexpr const wchar_t *lang_next = L"\ue66b";
    constexpr const wchar_t *lang_nuxt = L"\ue676";
    constexpr const wchar_t *lang_astro = L"\ue6a4";
    constexpr const wchar_t *lang_elm = L"\ue62c";
    constexpr const wchar_t *lang_purescript = L"\ue630";
    constexpr const wchar_t *lang_reason = L"\ue68b";
    constexpr const wchar_t *lang_rescript = L"\ue68b";
    constexpr const wchar_t *lang_coffeescript = L"\ue751";
    constexpr const wchar_t *lang_livescript = L"\ue751";
    constexpr const wchar_t *lang_haxe = L"\ue61f";
    constexpr const wchar_t *lang_vala = L"\ue6ac";
    constexpr const wchar_t *lang_d = L"\ue651";
    constexpr const wchar_t *lang_ada = L"\ue63a";
    constexpr const wchar_t *lang_lisp = L"\ue6b0";
    constexpr const wchar_t *lang_scheme = L"\ue6b0";
    constexpr const wchar_t *lang_racket = L"\ue6b0";
    constexpr const wchar_t *lang_prolog = L"\ue6b4";
    constexpr const wchar_t *lang_sml = L"\ue6b0";
    constexpr const wchar_t *lang_tcl = L"\ue6b1";
    constexpr const wchar_t *lang_awk = L"\uf489";
    constexpr const wchar_t *lang_sed = L"\uf489";
    constexpr const wchar_t *lang_regex = L"\ue678";
    constexpr const wchar_t *lang_graphql_config = L"\ue662";
    constexpr const wchar_t *lang_openapi = L"\ue687";
    constexpr const wchar_t *lang_swagger = L"\ue687";
    constexpr const wchar_t *lang_jenkins = L"\ue66d";
    constexpr const wchar_t *lang_github_actions = L"\ue663";
    constexpr const wchar_t *lang_gitlab = L"\uf296";
    constexpr const wchar_t *lang_circleci = L"\ue64a";
    constexpr const wchar_t *lang_travis = L"\ue63a";
    constexpr const wchar_t *lang_azure = L"\ue638";
    constexpr const wchar_t *lang_aws = L"\ue637";
    constexpr const wchar_t *lang_gcp = L"\ue65e";
    constexpr const wchar_t *lang_kubernetes = L"\ue66c";
    constexpr const wchar_t *lang_helm = L"\ue66c";
    constexpr const wchar_t *lang_vagrant = L"\ue6ac";
    constexpr const wchar_t *lang_puppet = L"\ue686";
    constexpr const wchar_t *lang_chef = L"\ue64a";
    constexpr const wchar_t *lang_salt = L"\ue691";
    constexpr const wchar_t *lang_nix = L"\ue675";
    constexpr const wchar_t *lang_guix = L"\ue675";
    constexpr const wchar_t *lang_flatbuffers = L"\ue65a";
    constexpr const wchar_t *lang_avro = L"\ue638";
    constexpr const wchar_t *lang_parquet = L"\ue638";
    constexpr const wchar_t *lang_hdf5 = L"\ue638";
    constexpr const wchar_t *lang_matlab_mat = L"\ue61a";
    constexpr const wchar_t *lang_ipynb = L"\ue66f";
    constexpr const wchar_t *lang_raku = L"\ue769";
    constexpr const wchar_t *lang_ballerina = L"\ue63a";
    constexpr const wchar_t *lang_hack = L"\ue63a";
    constexpr const wchar_t *lang_vhdl = L"\ue63a";
    constexpr const wchar_t *lang_verilog = L"\ue63a";
    constexpr const wchar_t *lang_systemverilog = L"\ue63a";
    constexpr const wchar_t *lang_glsl = L"\ue65b";
    constexpr const wchar_t *lang_hlsl = L"\ue65b";
    constexpr const wchar_t *lang_wgsl = L"\ue65b";
    constexpr const wchar_t *lang_metal = L"\ue65b";
    constexpr const wchar_t *lang_cuda = L"\ue64b";
    constexpr const wchar_t *lang_opencl = L"\ue64b";
    constexpr const wchar_t *lang_shader = L"\ue65b";
    constexpr const wchar_t *lang_godot = L"\ue65f";
    constexpr const wchar_t *lang_unity = L"\ue6a1";
    constexpr const wchar_t *lang_unreal = L"\ue6a1";
    constexpr const wchar_t *lang_blender = L"\uf1b2";
    constexpr const wchar_t *lang_cad = L"\uf1b2";
    constexpr const wchar_t *lang_certificate = L"\uf132";
    constexpr const wchar_t *lang_key = L"\uf084";
    constexpr const wchar_t *lang_database = L"\uf1c0";
    constexpr const wchar_t *lang_sqlite = L"\ue706";
    constexpr const wchar_t *lang_mongodb = L"\ue711";
    constexpr const wchar_t *lang_redis = L"\ue68b";
    constexpr const wchar_t *lang_graphql_sdl = L"\ue662";
    constexpr const wchar_t *lang_robots = L"\uf544";
    constexpr const wchar_t *lang_sitemap = L"\uf0e8";
    constexpr const wchar_t *lang_rss = L"\uf09e";
    constexpr const wchar_t *lang_calendar = L"\uf133";
    constexpr const wchar_t *lang_contact = L"\uf2bb";
    constexpr const wchar_t *lang_ebook = L"\uf02d";
    constexpr const wchar_t *lang_iso = L"\uf19c";
    constexpr const wchar_t *lang_disk = L"\uf0a0";
    constexpr const wchar_t *lang_torrent = L"\uf019";
    constexpr const wchar_t *lang_subtitle = L"\uf1c8";
    constexpr const wchar_t *lang_playlist = L"\uf001";
    constexpr const wchar_t *lang_config = L"\ue615";
    constexpr const wchar_t *lang_log = L"\uf15c";
    constexpr const wchar_t *lang_backup = L"\uf0c7";
    constexpr const wchar_t *lang_temp = L"\uf1c9";
    constexpr const wchar_t *lang_dump = L"\uf1c0";
    constexpr const wchar_t *lang_bytecode = L"\ueae8";
    constexpr const wchar_t *lang_object = L"\ueae8";
    constexpr const wchar_t *lang_library = L"\ueb9c";
    constexpr const wchar_t *lang_package = L"\ueb29";
    constexpr const wchar_t *lang_jar = L"\ue738";
    constexpr const wchar_t *lang_war = L"\ue738";
    constexpr const wchar_t *lang_apk = L"\ue70e";
    constexpr const wchar_t *lang_ipa = L"\ue711";
    constexpr const wchar_t *lang_deb = L"\uf306";
    constexpr const wchar_t *lang_rpm = L"\uf30a";
    constexpr const wchar_t *lang_appimage = L"\uf17c";
    constexpr const wchar_t *lang_snap = L"\uf17c";
    constexpr const wchar_t *lang_flatpak = L"\uf17c";
    constexpr const wchar_t *lang_msi = L"\ue70f";
    constexpr const wchar_t *lang_dmg = L"\uf179";
    constexpr const wchar_t *lang_plist = L"\ue615";
    constexpr const wchar_t *lang_desktop = L"\uf108";
    constexpr const wchar_t *lang_service = L"\uf233";
    constexpr const wchar_t *lang_unit = L"\uf233";
    constexpr const wchar_t *lang_man = L"\uf02d";
    constexpr const wchar_t *lang_info = L"\uf05a";
    constexpr const wchar_t *lang_todo = L"\uf0ae";
    constexpr const wchar_t *lang_changelog = L"\uf15c";
    constexpr const wchar_t *lang_authors = L"\uf007";
    constexpr const wchar_t *lang_contributing = L"\uf09b";
    constexpr const wchar_t *lang_security = L"\uf132";
    constexpr const wchar_t *lang_codeowners = L"\uf007";
    constexpr const wchar_t *lang_editorconfig_file = L"\ue652";
    constexpr const wchar_t *lang_prettierrc = L"\ue6b4";
    constexpr const wchar_t *lang_eslintrc = L"\ue655";
    constexpr const wchar_t *lang_babelrc = L"\ue639";
    constexpr const wchar_t *lang_tsconfig = L"\ue628";
    constexpr const wchar_t *lang_jsconfig = L"\ue74e";
    constexpr const wchar_t *lang_package_json = L"\ue71e";
    constexpr const wchar_t *lang_cargo = L"\ue7a8";
    constexpr const wchar_t *lang_go_mod = L"\ue627";
    constexpr const wchar_t *lang_requirements = L"\ue73c";
    constexpr const wchar_t *lang_pipfile = L"\ue73c";
    constexpr const wchar_t *lang_poetry = L"\ue73c";
    constexpr const wchar_t *lang_gemfile = L"\ue739";
    constexpr const wchar_t *lang_composer = L"\ue73d";
    constexpr const wchar_t *lang_mix = L"\ue62d";
    constexpr const wchar_t *lang_pubspec = L"\ue631";
    constexpr const wchar_t *lang_cartfile = L"\ue711";
    constexpr const wchar_t *lang_podfile = L"\ue711";
    constexpr const wchar_t *lang_build_gradle = L"\ue70e";
    constexpr const wchar_t *lang_pom = L"\ue741";
    constexpr const wchar_t *lang_sln = L"\ue70f";
    constexpr const wchar_t *lang_csproj = L"\ue648";
    constexpr const wchar_t *lang_fsproj = L"\ue65a";
    constexpr const wchar_t *lang_vbproj = L"\ue70f";
    constexpr const wchar_t *lang_xcode = L"\ue711";
    constexpr const wchar_t *lang_workspace = L"\ue711";
    constexpr const wchar_t *lang_pbxproj = L"\ue711";
    constexpr const wchar_t *lang_vcxproj = L"\ue70f";
    constexpr const wchar_t *lang_filters = L"\ue70f";
    constexpr const wchar_t *lang_props = L"\ue70f";
    constexpr const wchar_t *lang_targets = L"\ue70f";
    constexpr const wchar_t *lang_nuspec = L"\ue70f";
    constexpr const wchar_t *lang_nupkg = L"\ueb29";
    constexpr const wchar_t *lang_snapcraft = L"\uf17c";
    constexpr const wchar_t *lang_brewfile = L"\uf179";
    constexpr const wchar_t *lang_justfile = L"\ue779";
    constexpr const wchar_t *lang_taskfile = L"\ue779";
    constexpr const wchar_t *lang_procfile = L"\ue779";
    constexpr const wchar_t *lang_vagrantfile = L"\ue6ac";
    constexpr const wchar_t *lang_rakefile = L"\ue739";
    constexpr const wchar_t *lang_guardfile = L"\ue739";
    constexpr const wchar_t *lang_capfile = L"\ue739";
    constexpr const wchar_t *lang_thorfile = L"\ue739";
    constexpr const wchar_t *lang_berksfile = L"\ue739";
    constexpr const wchar_t *lang_appveyor = L"\ue63a";
    constexpr const wchar_t *lang_netlify = L"\ue66b";
    constexpr const wchar_t *lang_vercel = L"\ue66b";
    constexpr const wchar_t *lang_firebase = L"\ue65a";
    constexpr const wchar_t *lang_supabase = L"\ue65a";
    constexpr const wchar_t *lang_prisma_schema = L"\ue684";
    constexpr const wchar_t *lang_drizzle = L"\ue706";
    constexpr const wchar_t *lang_kustomize = L"\ue66c";
    constexpr const wchar_t *lang_skaffold = L"\ue66c";
    constexpr const wchar_t *lang_tilt = L"\ue66c";
    constexpr const wchar_t *lang_earthly = L"\uf308";
    constexpr const wchar_t *lang_bake = L"\uf308";
    constexpr const wchar_t *lang_compose = L"\uf308";
    constexpr const wchar_t *lang_devcontainer = L"\uf308";
    constexpr const wchar_t *lang_code_workspace = L"\ue70c";
    constexpr const wchar_t *lang_vscode = L"\ue70c";
    constexpr const wchar_t *lang_cursor = L"\ue70c";
    constexpr const wchar_t *lang_clang_format = L"\ue61d";
    constexpr const wchar_t *lang_clang_tidy = L"\ue61d";
    constexpr const wchar_t *lang_compile_commands = L"\ue61d";
    constexpr const wchar_t *lang_gdb = L"\ue637";
    constexpr const wchar_t *lang_lldb = L"\ue637";
    constexpr const wchar_t *lang_valgrind = L"\ue637";
    constexpr const wchar_t *lang_sanitizer = L"\ue637";
    constexpr const wchar_t *lang_coverage = L"\uf46d";
    constexpr const wchar_t *lang_test = L"\uf0ae";
    constexpr const wchar_t *lang_spec = L"\uf0ae";
    constexpr const wchar_t *lang_benchmark = L"\uf0ae";
    constexpr const wchar_t *lang_mock = L"\uf0ae";
    constexpr const wchar_t *lang_fixture = L"\uf0ae";
    constexpr const wchar_t *lang_snapshot = L"\uf0ae";
    constexpr const wchar_t *lang_story = L"\ue696";
    constexpr const wchar_t *lang_graphql_query = L"\ue662";
    constexpr const wchar_t *lang_sql_migration = L"\ue706";
    constexpr const wchar_t *lang_liquibase = L"\ue706";
    constexpr const wchar_t *lang_flyway = L"\ue706";
    constexpr const wchar_t *lang_schema = L"\ue706";
    constexpr const wchar_t *lang_seed = L"\ue706";
    constexpr const wchar_t *lang_migration = L"\ue706";
    constexpr const wchar_t *lang_plantuml = L"\ue678";
    constexpr const wchar_t *lang_mermaid = L"\ue678";
    constexpr const wchar_t *lang_dot = L"\ue678";
    constexpr const wchar_t *lang_drawio = L"\ue678";
    constexpr const wchar_t *lang_figma = L"\ue65b";
    constexpr const wchar_t *lang_sketch = L"\ue65b";
    constexpr const wchar_t *lang_xd = L"\ue65b";
    constexpr const wchar_t *lang_psd = L"\ue65b";
    constexpr const wchar_t *lang_ai = L"\ue65b";
    constexpr const wchar_t *lang_indd = L"\ue65b";
    constexpr const wchar_t *lang_raw = L"\uf1c5";
    constexpr const wchar_t *lang_heic = L"\uf1c5";
    constexpr const wchar_t *lang_tiff = L"\uf1c5";
    constexpr const wchar_t *lang_exr = L"\uf1c5";
    constexpr const wchar_t *lang_hdr = L"\uf1c5";
    constexpr const wchar_t *lang_ico = L"\uf1c5";
    constexpr const wchar_t *lang_icns = L"\uf1c5";
    constexpr const wchar_t *lang_cur = L"\uf1c5";
    constexpr const wchar_t *lang_ani = L"\uf1c5";
    constexpr const wchar_t *lang_midi = L"\uf001";
    constexpr const wchar_t *lang_mod = L"\uf001";
    constexpr const wchar_t *lang_xm = L"\uf001";
    constexpr const wchar_t *lang_it = L"\uf001";
    constexpr const wchar_t *lang_s3m = L"\uf001";
    constexpr const wchar_t *lang_cue = L"\uf001";
    constexpr const wchar_t *lang_m3u = L"\uf001";
    constexpr const wchar_t *lang_pls = L"\uf001";
    constexpr const wchar_t *lang_srt = L"\uf1c8";
    constexpr const wchar_t *lang_vtt = L"\uf1c8";
    constexpr const wchar_t *lang_ass = L"\uf1c8";
    constexpr const wchar_t *lang_ssa = L"\uf1c8";
    constexpr const wchar_t *lang_sub = L"\uf1c8";
    constexpr const wchar_t *lang_idx = L"\uf1c8";
    constexpr const wchar_t *lang_epub = L"\uf02d";
    constexpr const wchar_t *lang_mobi = L"\uf02d";
    constexpr const wchar_t *lang_azw = L"\uf02d";
    constexpr const wchar_t *lang_fb2 = L"\uf02d";
    constexpr const wchar_t *lang_djvu = L"\uf02d";
    constexpr const wchar_t *lang_chm = L"\uf02d";
    constexpr const wchar_t *lang_pdb = L"\uf02d";
    constexpr const wchar_t *lang_cbr = L"\uf02d";
    constexpr const wchar_t *lang_cbz = L"\uf02d";
    constexpr const wchar_t *lang_rtf = L"\uf15c";
    constexpr const wchar_t *lang_odt = L"\uf15c";
    constexpr const wchar_t *lang_ods = L"\uf1c0";
    constexpr const wchar_t *lang_odp = L"\uf1c4";
    constexpr const wchar_t *lang_pages = L"\uf15c";
    constexpr const wchar_t *lang_numbers = L"\uf1c0";
    constexpr const wchar_t *lang_keynote = L"\uf1c4";
    constexpr const wchar_t *lang_tex = L"\ue69b";
    constexpr const wchar_t *lang_sty = L"\ue69b";
    constexpr const wchar_t *lang_cls = L"\ue69b";
    constexpr const wchar_t *lang_dtx = L"\ue69b";
    constexpr const wchar_t *lang_ins = L"\ue69b";
    constexpr const wchar_t *lang_ltx = L"\ue69b";
    constexpr const wchar_t *lang_bib = L"\ue69b";
    constexpr const wchar_t *lang_bst = L"\ue69b";
    constexpr const wchar_t *lang_adoc = L"\ue678";
    constexpr const wchar_t *lang_asciidoc_ext = L"\ue678";
    constexpr const wchar_t *lang_orgmode = L"\ue633";
    constexpr const wchar_t *lang_wiki = L"\uf266";
    constexpr const wchar_t *lang_mediawiki = L"\uf266";
    constexpr const wchar_t *lang_creole = L"\uf266";
    constexpr const wchar_t *lang_pod = L"\ue769";
    constexpr const wchar_t *lang_rdoc = L"\ue739";
    constexpr const wchar_t *lang_yard = L"\ue739";
    constexpr const wchar_t *lang_javadoc = L"\ue738";
    constexpr const wchar_t *lang_doxygen = L"\ue61d";
    constexpr const wchar_t *lang_jsdoc = L"\ue74e";
    constexpr const wchar_t *lang_typedoc = L"\ue628";
    constexpr const wchar_t *lang_sphinx = L"\ue679";
    constexpr const wchar_t *lang_mkdocs = L"\ueb1d";
    constexpr const wchar_t *lang_hugo = L"\ueb1d";
    constexpr const wchar_t *lang_jekyll = L"\ueb1d";
    constexpr const wchar_t *lang_hexo = L"\ueb1d";
    constexpr const wchar_t *lang_eleventy = L"\ueb1d";
    constexpr const wchar_t *lang_docusaurus = L"\ueb1d";
    constexpr const wchar_t *lang_vuepress = L"\ue6a0";
    constexpr const wchar_t *lang_gitbook = L"\ueb1d";
    constexpr const wchar_t *lang_readme = L"\ueb1d";
    constexpr const wchar_t *lang_license = L"\uf02d";
    constexpr const wchar_t *lang_copying = L"\uf02d";
    constexpr const wchar_t *lang_credits = L"\uf007";
    constexpr const wchar_t *lang_notice = L"\uf05a";
    constexpr const wchar_t *lang_patent = L"\uf05a";
    constexpr const wchar_t *lang_trademark = L"\uf05a";
    constexpr const wchar_t *lang_privacy = L"\uf023";
    constexpr const wchar_t *lang_terms = L"\uf15c";
    constexpr const wchar_t *lang_coc = L"\uf15c";
    constexpr const wchar_t *lang_conduct = L"\uf15c";
    constexpr const wchar_t *lang_support = L"\uf05a";
    constexpr const wchar_t *lang_faq = L"\uf059";
    constexpr const wchar_t *lang_roadmap = L"\uf0ae";
    constexpr const wchar_t *lang_architecture = L"\ue678";
    constexpr const wchar_t *lang_adr = L"\ue678";
    constexpr const wchar_t *lang_rfc = L"\uf15c";
    constexpr const wchar_t *lang_spec_doc = L"\uf15c";
    constexpr const wchar_t *lang_proposal = L"\uf15c";
    constexpr const wchar_t *lang_design = L"\ue65b";
    constexpr const wchar_t *lang_wireframe = L"\ue65b";
    constexpr const wchar_t *lang_mockup = L"\ue65b";
    constexpr const wchar_t *lang_prototype = L"\ue65b";

    inline std::string lower_ascii(std::string value)
    {
        for (char &c : value)
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return value;
    }

    inline bool ends_with(const std::string &value, const std::string &suffix)
    {
        return value.size() >= suffix.size() &&
               value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
    }

    inline bool starts_with(const std::string &value, const std::string &prefix)
    {
        return value.size() >= prefix.size() &&
               value.compare(0, prefix.size(), prefix) == 0;
    }

    inline const wchar_t *for_file(const fs::path &path)
    {
        const std::string name = lower_ascii(path.filename().string());
        const std::string ext = lower_ascii(path.extension().string());

        // ---- special filenames ----
        if (name == "makefile" || name == "gnumakefile" || name == "kbuild")
            return lang_makefile;
        if (name == "cmakelists.txt" || name == "cmake")
            return lang_cmake;
        if (name == "dockerfile" || starts_with(name, "dockerfile.") || name == "containerfile")
            return lang_dockerfile;
        if (name == "docker-compose.yml" || name == "docker-compose.yaml" ||
            name == "compose.yml" || name == "compose.yaml")
            return lang_compose;
        if (name == "vagrantfile")
            return lang_vagrantfile;
        if (name == "justfile" || name == ".justfile")
            return lang_justfile;
        if (name == "taskfile.yml" || name == "taskfile.yaml")
            return lang_taskfile;
        if (name == "procfile")
            return lang_procfile;
        if (name == "rakefile" || name == "gemfile" || name == "gemfile.lock")
            return lang_gemfile;
        if (name == "guardfile" || name == "capfile" || name == "thorfile" || name == "berksfile")
            return lang_rakefile;
        if (name == "brewfile")
            return lang_brewfile;
        if (name == "package.json" || name == "package-lock.json")
            return lang_package_json;
        if (name == "yarn.lock" || name == ".yarnrc" || name == ".yarnrc.yml")
            return lang_yarn;
        if (name == "pnpm-lock.yaml" || name == "pnpm-workspace.yaml")
            return lang_pnpm;
        if (name == "bun.lockb" || name == "bunfig.toml")
            return lang_bun;
        if (name == "deno.json" || name == "deno.jsonc")
            return lang_deno;
        if (name == "tsconfig.json" || starts_with(name, "tsconfig."))
            return lang_tsconfig;
        if (name == "jsconfig.json")
            return lang_jsconfig;
        if (name == ".eslintrc" || starts_with(name, ".eslintrc.") || name == "eslint.config.js" ||
            name == "eslint.config.mjs" || name == "eslint.config.cjs" || name == "eslint.config.ts")
            return lang_eslintrc;
        if (name == ".prettierrc" || starts_with(name, ".prettierrc.") || name == "prettier.config.js" ||
            name == "prettier.config.cjs" || name == "prettier.config.mjs")
            return lang_prettierrc;
        if (name == ".babelrc" || starts_with(name, ".babelrc.") || name == "babel.config.js" ||
            name == "babel.config.cjs" || name == "babel.config.json")
            return lang_babelrc;
        if (name == "webpack.config.js" || name == "webpack.config.ts" || name == "webpack.config.cjs")
            return lang_webpack;
        if (name == "vite.config.js" || name == "vite.config.ts" || name == "vite.config.mjs")
            return lang_vite;
        if (name == "next.config.js" || name == "next.config.mjs" || name == "next.config.ts")
            return lang_next;
        if (name == "nuxt.config.js" || name == "nuxt.config.ts")
            return lang_nuxt;
        if (name == "astro.config.mjs" || name == "astro.config.js" || name == "astro.config.ts")
            return lang_astro;
        if (name == "svelte.config.js" || name == "svelte.config.ts")
            return lang_sveltekit;
        if (name == "tailwind.config.js" || name == "tailwind.config.ts" || name == "tailwind.config.cjs")
            return lang_tailwind;
        if (name == "postcss.config.js" || name == "postcss.config.cjs" || name == "postcss.config.mjs")
            return lang_postcss;
        if (name == "jest.config.js" || name == "jest.config.ts" || name == "jest.config.cjs")
            return lang_jest;
        if (name == "cypress.config.js" || name == "cypress.config.ts")
            return lang_cypress;
        if (name == "playwright.config.js" || name == "playwright.config.ts")
            return lang_playwright;
        if (name == "cargo.toml" || name == "cargo.lock")
            return lang_cargo;
        if (name == "go.mod" || name == "go.sum")
            return lang_go_mod;
        if (name == "requirements.txt" || name == "requirements-dev.txt" || name == "constraints.txt")
            return lang_requirements;
        if (name == "pipfile" || name == "pipfile.lock")
            return lang_pipfile;
        if (name == "pyproject.toml" || name == "poetry.lock")
            return lang_poetry;
        if (name == "composer.json" || name == "composer.lock")
            return lang_composer;
        if (name == "mix.exs" || name == "mix.lock")
            return lang_mix;
        if (name == "pubspec.yaml" || name == "pubspec.lock")
            return lang_pubspec;
        if (name == "cartfile" || name == "cartfile.resolved")
            return lang_cartfile;
        if (name == "podfile" || name == "podfile.lock")
            return lang_podfile;
        if (name == "build.gradle" || name == "build.gradle.kts" || name == "settings.gradle" ||
            name == "settings.gradle.kts")
            return lang_build_gradle;
        if (name == "pom.xml")
            return lang_pom;
        if (name == "meson.build" || name == "meson_options.txt")
            return lang_cmake;
        if (name == "build.ninja" || name == "rules.ninja")
            return lang_ninja;
        if (name == "work.bazel" || name == "build.bazel" || name == "workspace" || name == "module.bazel")
            return lang_bazel;
        if (name == ".gitignore" || name == ".gitattributes" || name == ".gitmodules" ||
            name == ".gitkeep" || name == ".keep")
            return lang_git;
        if (name == ".editorconfig")
            return lang_editorconfig_file;
        if (name == ".env" || starts_with(name, ".env."))
            return lang_env;
        if (name == ".clang-format" || name == ".clang-tidy")
            return lang_clang_format;
        if (name == "compile_commands.json")
            return lang_compile_commands;
        if (name == "readme" || name == "readme.md" || name == "readme.txt" || name == "readme.rst" ||
            name == "readme.adoc")
            return lang_readme;
        if (name == "license" || name == "licence" || name == "copying" || name == "license.md" ||
            name == "licence.md" || name == "copying.md")
            return lang_license;
        if (name == "changelog" || name == "changelog.md" || name == "changes" || name == "changes.md" ||
            name == "history.md" || name == "news.md")
            return lang_changelog;
        if (name == "authors" || name == "authors.md" || name == "contributors" || name == "contributors.md")
            return lang_authors;
        if (name == "contributing" || name == "contributing.md")
            return lang_contributing;
        if (name == "security" || name == "security.md")
            return lang_security;
        if (name == "codeowners" || name == "code_of_conduct.md" || name == "code-of-conduct.md")
            return lang_codeowners;
        if (name == "todo" || name == "todo.md" || name == "todos.md")
            return lang_todo;
        if (name == "faq" || name == "faq.md")
            return lang_faq;
        if (name == "robots.txt")
            return lang_robots;
        if (name == "sitemap.xml" || name == "sitemap.txt")
            return lang_sitemap;
        if (name == "manifest.json" || name == "manifest.webmanifest")
            return lang_package;
        if (name == "sw.js" || name == "service-worker.js")
            return lang_js;
        if (name == "firebase.json" || name == ".firebaserc")
            return lang_firebase;
        if (name == "vercel.json" || name == "now.json")
            return lang_vercel;
        if (name == "netlify.toml")
            return lang_netlify;
        if (name == "prisma.schema" || ends_with(name, ".prisma"))
            return lang_prisma_schema;
        if (name == "schema.graphql" || name == "schema.gql")
            return lang_graphql_sdl;
        if (name == "openapi.yaml" || name == "openapi.yml" || name == "openapi.json" ||
            name == "swagger.yaml" || name == "swagger.yml" || name == "swagger.json")
            return lang_openapi;
        if (name == "jenkinsfile" || starts_with(name, "jenkinsfile."))
            return lang_jenkins;
        if (name == ".travis.yml")
            return lang_travis;
        if (name == "appveyor.yml" || name == ".appveyor.yml")
            return lang_appveyor;
        if (name == ".gitlab-ci.yml")
            return lang_gitlab;
        if (name == "azure-pipelines.yml" || name == "azure-pipelines.yaml")
            return lang_azure;
        if (name == "earthfile")
            return lang_earthly;
        if (name == "tiltfile")
            return lang_tilt;
        if (name == "skaffold.yaml" || name == "skaffold.yml")
            return lang_skaffold;
        if (name == "chart.yaml" || name == "chart.yml" || name == "values.yaml" || name == "values.yml")
            return lang_helm;
        if (name == "kustomization.yaml" || name == "kustomization.yml")
            return lang_kustomize;
        if (name == ".devcontainer.json" || name == "devcontainer.json")
            return lang_devcontainer;
        if (ends_with(name, ".code-workspace"))
            return lang_code_workspace;
        if (name == "nix" || ends_with(name, ".nix"))
            return lang_nix;
        if (name == "flake.nix" || name == "flake.lock" || name == "default.nix" || name == "shell.nix")
            return lang_nix;
        if (name == "snapcraft.yaml")
            return lang_snapcraft;
        if (name == "gruntfile.js" || name == "gulpfile.js" || name == "gulpfile.ts")
            return lang_js;
        if (name == "rollup.config.js" || name == "rollup.config.ts" || name == "rollup.config.mjs")
            return lang_webpack;
        if (name == "karma.conf.js" || name == "protractor.conf.js")
            return lang_test;
        if (name == "mocha.opts" || name == ".mocharc.json" || name == ".mocharc.yml")
            return lang_test;
        if (name == "vitest.config.ts" || name == "vitest.config.js" || name == "vitest.config.mjs")
            return lang_test;
        if (name == "storybook" || starts_with(name, ".storybook"))
            return lang_storybook;

        // ---- extensions ----
        if (ext == ".c")
            return lang_c;
        if (ext == ".cpp" || ext == ".cc" || ext == ".cxx" || ext == ".c++" || ext == ".cp" || ext == ".ii")
            return lang_cpp;
        if (ext == ".h" || ext == ".hh")
            return lang_h;
        if (ext == ".hpp" || ext == ".hxx" || ext == ".h++" || ext == ".hp" || ext == ".tcc" ||
            ext == ".inl" || ext == ".inc")
            return lang_hpp;
        if (ext == ".m")
            return lang_objc;
        if (ext == ".mm")
            return lang_objc;
        if (ext == ".cs")
            return lang_csharp;
        if (ext == ".fs" || ext == ".fsi" || ext == ".fsx" || ext == ".fsscript")
            return lang_fsharp;
        if (ext == ".vb" || ext == ".vbs")
            return lang_batch;
        if (ext == ".py" || ext == ".pyw" || ext == ".pyi" || ext == ".pyx" || ext == ".pxd" ||
            ext == ".pxi" || ext == ".pyc" || ext == ".pyo" || ext == ".pyd")
            return python;
        if (ext == ".ipynb")
            return lang_ipynb;
        if (ext == ".rb" || ext == ".rbw" || ext == ".rake" || ext == ".gemspec" || ext == ".ru")
            return ruby;
        if (ext == ".js" || ext == ".mjs" || ext == ".cjs" || ext == ".es" || ext == ".es6")
            return lang_js;
        if (ext == ".ts" || ext == ".mts" || ext == ".cts")
            return lang_ts;
        if (ext == ".jsx")
            return lang_jsx;
        if (ext == ".tsx")
            return lang_tsx;
        if (ext == ".vue")
            return lang_vue;
        if (ext == ".svelte")
            return lang_svelte;
        if (ext == ".astro")
            return lang_astro;
        if (ext == ".coffee" || ext == ".litcoffee")
            return lang_coffeescript;
        if (ext == ".ls")
            return lang_livescript;
        if (ext == ".html" || ext == ".htm" || ext == ".shtml" || ext == ".xhtml" || ext == ".htmlx")
            return lang_html;
        if (ext == ".css")
            return lang_css;
        if (ext == ".scss")
            return lang_scss;
        if (ext == ".sass")
            return lang_sass;
        if (ext == ".less")
            return lang_less;
        if (ext == ".styl" || ext == ".stylus")
            return lang_stylus;
        if (ext == ".rs" || ext == ".rlib")
            return lang_rust;
        if (ext == ".go")
            return lang_go;
        if (ext == ".java" || ext == ".jav")
            return lang_java;
        if (ext == ".kt" || ext == ".kts" || ext == ".ktm")
            return lang_kotlin;
        if (ext == ".scala" || ext == ".sc")
            return lang_scala;
        if (ext == ".groovy" || ext == ".gvy" || ext == ".gy" || ext == ".gsh")
            return lang_groovy;
        if (ext == ".clj" || ext == ".cljs" || ext == ".cljc" || ext == ".edn")
            return lang_clojure;
        if (ext == ".lua" || ext == ".luau" || ext == ".rockspec")
            return lang_lua;
        if (ext == ".vim" || ext == ".vimrc" || ext == ".gvimrc")
            return lang_vim;
        if (ext == ".el" || ext == ".elc")
            return lang_emacs;
        if (ext == ".php" || ext == ".phtml" || ext == ".php3" || ext == ".php4" || ext == ".php5" ||
            ext == ".phps" || ext == ".phpt")
            return lang_php;
        if (ext == ".pl" || ext == ".pm" || ext == ".t" || ext == ".pod")
            return lang_perl;
        if (ext == ".raku" || ext == ".rakumod" || ext == ".rakudoc" || ext == ".rakutest" || ext == ".p6")
            return lang_raku;
        if (ext == ".r" || ext == ".rdata" || ext == ".rds" || ext == ".rda")
            return lang_r;
        if (ext == ".jl")
            return lang_julia;
        if (ext == ".mat")
            return lang_matlab_mat;
        if (ext == ".hs" || ext == ".lhs" || ext == ".hsc")
            return lang_haskell;
        if (ext == ".ml" || ext == ".mli" || ext == ".mll" || ext == ".mly")
            return lang_ocaml;
        if (ext == ".ex" || ext == ".exs" || ext == ".eex" || ext == ".heex" || ext == ".leex")
            return lang_elixir;
        if (ext == ".erl" || ext == ".hrl")
            return lang_erlang;
        if (ext == ".nim" || ext == ".nims" || ext == ".nimble")
            return lang_nim;
        if (ext == ".cr")
            return lang_crystal;
        if (ext == ".zig" || ext == ".zon")
            return lang_zig;
        if (ext == ".v" || ext == ".vv")
            return lang_v;
        if (ext == ".dart")
            return lang_dart;
        if (ext == ".swift")
            return lang_swift;
        if (ext == ".f" || ext == ".for" || ext == ".f90" || ext == ".f95" || ext == ".f03" || ext == ".f08")
            return lang_fortran;
        if (ext == ".cob" || ext == ".cbl" || ext == ".cpy")
            return lang_cobol;
        if (ext == ".pas" || ext == ".pp" || ext == ".dpr" || ext == ".lpr")
            return lang_pascal;
        if (ext == ".asm" || ext == ".s" || ext == ".nasm" || ext == ".yasm")
            return lang_assembly;
        if (ext == ".wasm" || ext == ".wat")
            return lang_wasm;
        if (ext == ".sol")
            return lang_solidity;
        if (ext == ".graphql" || ext == ".gql")
            return lang_graphql;
        if (ext == ".prisma")
            return lang_prisma;
        if (ext == ".proto")
            return lang_proto;
        if (ext == ".thrift")
            return lang_thrift;
        if (ext == ".tf" || ext == ".tfvars" || ext == ".hcl")
            return lang_terraform;
        if (ext == ".pulumi")
            return lang_pulumi;
        if (ext == ".nix")
            return lang_nix;
        if (ext == ".d")
            return lang_d;
        if (ext == ".ada" || ext == ".adb" || ext == ".ads")
            return lang_ada;
        if (ext == ".lisp" || ext == ".lsp" || ext == ".cl" || ext == ".fasl")
            return lang_lisp;
        if (ext == ".scm" || ext == ".ss")
            return lang_scheme;
        if (ext == ".rkt" || ext == ".rktd" || ext == ".rktl")
            return lang_racket;
        if (ext == ".pro" || ext == ".P")
            return lang_prolog;
        if (ext == ".sml" || ext == ".sig" || ext == ".fun")
            return lang_sml;
        if (ext == ".tcl" || ext == ".tk")
            return lang_tcl;
        if (ext == ".awk" || ext == ".gawk" || ext == ".mawk")
            return lang_awk;
        if (ext == ".sed")
            return lang_sed;
        if (ext == ".hx" || ext == ".hxml")
            return lang_haxe;
        if (ext == ".vala" || ext == ".vapi")
            return lang_vala;
        if (ext == ".re" || ext == ".rei")
            return lang_reason;
        if (ext == ".res" || ext == ".resi")
            return lang_rescript;
        if (ext == ".elm")
            return lang_elm;
        if (ext == ".purs")
            return lang_purescript;
        if (ext == ".hack")
            return lang_hack;
        if (ext == ".bal")
            return lang_ballerina;
        if (ext == ".vhd" || ext == ".vhdl")
            return lang_vhdl;
        if (ext == ".sv" || ext == ".svh")
            return lang_systemverilog;
        if (ext == ".glsl" || ext == ".vert" || ext == ".frag" || ext == ".geom" || ext == ".comp" ||
            ext == ".tesc" || ext == ".tese")
            return lang_glsl;
        if (ext == ".hlsl" || ext == ".fx" || ext == ".fxh")
            return lang_hlsl;
        if (ext == ".wgsl")
            return lang_wgsl;
        if (ext == ".metal")
            return lang_metal;
        if (ext == ".cu" || ext == ".cuh")
            return lang_cuda;
        if (ext == ".cl")
            return lang_opencl;
        if (ext == ".shader" || ext == ".cginc" || ext == ".compute")
            return lang_shader;
        if (ext == ".gd" || ext == ".tscn" || ext == ".tres" || ext == ".godot")
            return lang_godot;
        if (ext == ".unity" || ext == ".prefab" || ext == ".asset" || ext == ".meta")
            return lang_unity;
        if (ext == ".uasset" || ext == ".umap")
            return lang_unreal;
        if (ext == ".blend" || ext == ".blend1")
            return lang_blender;
        if (ext == ".fbx" || ext == ".obj" || ext == ".stl" || ext == ".gltf" || ext == ".glb" ||
            ext == ".dae" || ext == ".3ds" || ext == ".max" || ext == ".ma" || ext == ".mb")
            return lang_3d;
        if (ext == ".toml")
            return lang_toml;
        if (ext == ".yml" || ext == ".yaml")
            return lang_yaml;
        if (ext == ".json" || ext == ".jsonc" || ext == ".json5" || ext == ".geojson" || ext == ".har")
            return json;
        if (ext == ".xml" || ext == ".xsl" || ext == ".xslt" || ext == ".xsd" || ext == ".dtd" ||
            ext == ".ent" || ext == ".svg")
            return lang_xml;
        if (ext == ".ini" || ext == ".cfg" || ext == ".conf" || ext == ".config" || ext == ".prefs" ||
            ext == ".properties" || ext == ".prop")
            return lang_ini;
        if (ext == ".env")
            return lang_env;
        if (ext == ".sql" || ext == ".ddl" || ext == ".dml" || ext == ".pgsql" || ext == ".mysql" ||
            ext == ".plsql" || ext == ".psql")
            return lang_sql;
        if (ext == ".sqlite" || ext == ".sqlite3" || ext == ".db" || ext == ".db3")
            return lang_sqlite;
        if (ext == ".bson")
            return lang_mongodb;
        if (ext == ".rdb")
            return lang_redis;
        if (ext == ".md" || ext == ".markdown" || ext == ".mdown" || ext == ".mkd" || ext == ".mkdn" ||
            ext == ".mdwn" || ext == ".mdx")
            return markdown;
        if (ext == ".rst" || ext == ".rest")
            return lang_rst;
        if (ext == ".adoc" || ext == ".asciidoc" || ext == ".asc")
            return lang_adoc;
        if (ext == ".org")
            return lang_orgmode;
        if (ext == ".textile")
            return lang_textile;
        if (ext == ".wiki" || ext == ".mediawiki")
            return lang_wiki;
        if (ext == ".tex" || ext == ".ltx")
            return lang_tex;
        if (ext == ".sty" || ext == ".cls" || ext == ".dtx" || ext == ".ins")
            return lang_sty;
        if (ext == ".bib" || ext == ".bst")
            return lang_bib;
        if (ext == ".sh" || ext == ".bash" || ext == ".zsh" || ext == ".fish" || ext == ".ksh" ||
            ext == ".csh" || ext == ".tcsh" || ext == ".command")
            return lang_sh;
        if (ext == ".ps1" || ext == ".psm1" || ext == ".psd1")
            return lang_powershell;
        if (ext == ".bat" || ext == ".cmd" || ext == ".btm")
            return lang_batch;
        if (ext == ".diff" || ext == ".patch")
            return lang_diff;
        if (ext == ".csv")
            return lang_csv;
        if (ext == ".tsv" || ext == ".tab")
            return lang_tsv;
        if (ext == ".xls" || ext == ".xlsx" || ext == ".xlsm" || ext == ".xlsb" || ext == ".xltx")
            return lang_excel;
        if (ext == ".doc" || ext == ".docx" || ext == ".docm" || ext == ".dotx")
            return lang_word;
        if (ext == ".ppt" || ext == ".pptx" || ext == ".pptm" || ext == ".potx")
            return lang_powerpoint;
        if (ext == ".odt")
            return lang_odt;
        if (ext == ".ods")
            return lang_ods;
        if (ext == ".odp")
            return lang_odp;
        if (ext == ".rtf")
            return lang_rtf;
        if (ext == ".pages")
            return lang_pages;
        if (ext == ".numbers")
            return lang_numbers;
        if (ext == ".key")
            return lang_keynote;
        if (ext == ".pdf")
            return file_pdf;
        if (ext == ".epub")
            return lang_epub;
        if (ext == ".mobi" || ext == ".azw" || ext == ".azw3")
            return lang_mobi;
        if (ext == ".fb2" || ext == ".djvu" || ext == ".chm" || ext == ".cbr" || ext == ".cbz")
            return lang_ebook;
        if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".jpe" || ext == ".jfif" ||
            ext == ".gif" || ext == ".webp" || ext == ".bmp" || ext == ".dib" || ext == ".pbm" ||
            ext == ".pgm" || ext == ".ppm" || ext == ".pnm" || ext == ".xpm" || ext == ".xbm")
            return lang_image;
        if (ext == ".ico" || ext == ".icns" || ext == ".cur" || ext == ".ani")
            return lang_ico;
        if (ext == ".tif" || ext == ".tiff")
            return lang_tiff;
        if (ext == ".heic" || ext == ".heif" || ext == ".avif" || ext == ".jxl")
            return lang_heic;
        if (ext == ".raw" || ext == ".cr2" || ext == ".nef" || ext == ".arw" || ext == ".dng" ||
            ext == ".orf" || ext == ".rw2")
            return lang_raw;
        if (ext == ".exr" || ext == ".hdr" || ext == ".pic")
            return lang_exr;
        if (ext == ".psd" || ext == ".psb" || ext == ".ai" || ext == ".eps" || ext == ".indd" ||
            ext == ".sketch" || ext == ".fig" || ext == ".xd" || ext == ".afdesign" || ext == ".afphoto")
            return lang_design;
        if (ext == ".ttf" || ext == ".otf" || ext == ".woff" || ext == ".woff2" || ext == ".eot" ||
            ext == ".fon" || ext == ".fnt" || ext == ".pfb" || ext == ".pfm")
            return lang_font;
        if (ext == ".mp3" || ext == ".wav" || ext == ".flac" || ext == ".ogg" || ext == ".oga" ||
            ext == ".opus" || ext == ".aac" || ext == ".m4a" || ext == ".wma" || ext == ".aiff" ||
            ext == ".aif" || ext == ".ape" || ext == ".alac")
            return lang_audio;
        if (ext == ".mid" || ext == ".midi")
            return lang_midi;
        if (ext == ".mod" || ext == ".xm" || ext == ".it" || ext == ".s3m")
            return lang_mod;
        if (ext == ".mp4" || ext == ".m4v" || ext == ".mkv" || ext == ".webm" || ext == ".avi" ||
            ext == ".mov" || ext == ".wmv" || ext == ".flv" || ext == ".mpeg" || ext == ".mpg" ||
            ext == ".mpe" || ext == ".3gp" || ext == ".3g2" || ext == ".ogv" || ext == ".mts" ||
            ext == ".m2ts")
            return lang_video;
        if (ext == ".srt" || ext == ".vtt" || ext == ".ass" || ext == ".ssa" || ext == ".sub" ||
            ext == ".idx" || ext == ".smi")
            return lang_subtitle;
        if (ext == ".m3u" || ext == ".m3u8" || ext == ".pls" || ext == ".asx" || ext == ".xspf")
            return lang_playlist;
        if (ext == ".cue")
            return lang_cue;
        if (ext == ".zip" || ext == ".gz" || ext == ".tgz" || ext == ".bz2" || ext == ".tbz2" ||
            ext == ".xz" || ext == ".txz" || ext == ".lz" || ext == ".lzma" || ext == ".zst" ||
            ext == ".zstd" || ext == ".tar" || ext == ".7z" || ext == ".rar" || ext == ".cab" ||
            ext == ".iso" || ext == ".img" || ext == ".dmg" || ext == ".pkg" || ext == ".apk" ||
            ext == ".ipa" || ext == ".deb" || ext == ".rpm" || ext == ".msi" || ext == ".appimage" ||
            ext == ".snap" || ext == ".flatpak")
            return file_zip;
        if (ext == ".jar" || ext == ".war" || ext == ".ear" || ext == ".aar")
            return lang_jar;
        if (ext == ".nupkg" || ext == ".snupkg")
            return lang_nupkg;
        if (ext == ".whl" || ext == ".egg")
            return lang_package;
        if (ext == ".gem")
            return lang_gemfile;
        if (ext == ".crate")
            return lang_cargo;
        if (ext == ".lock")
            return lang_lock;
        if (ext == ".pem" || ext == ".crt" || ext == ".cer" || ext == ".der" || ext == ".p12" ||
            ext == ".pfx" || ext == ".p7b" || ext == ".p7c" || ext == ".csr")
            return lang_certificate;
        if (ext == ".key" || ext == ".pub" || ext == ".gpg" || ext == ".asc" || ext == ".sig" ||
            ext == ".sign")
            return lang_key;
        if (ext == ".kdbx" || ext == ".kdb")
            return lang_lock;
        if (ext == ".log")
            return lang_log;
        if (ext == ".tmp" || ext == ".temp" || ext == ".swp" || ext == ".swo" || ext == ".bak" ||
            ext == ".old" || ext == ".orig" || ext == ".rej")
            return lang_temp;
        if (ext == ".o" || ext == ".obj" || ext == ".a" || ext == ".lib" || ext == ".so" ||
            ext == ".dylib" || ext == ".dll" || ext == ".exe" || ext == ".bin" || ext == ".elf" ||
            ext == ".ko" || ext == ".out" || ext == ".app" || ext == ".com")
            return file_binary;
        if (ext == ".class" || ext == ".beam" || ext == ".hi")
            return lang_bytecode;
        if (ext == ".sln")
            return lang_sln;
        if (ext == ".csproj")
            return lang_csproj;
        if (ext == ".fsproj")
            return lang_fsproj;
        if (ext == ".vbproj")
            return lang_vbproj;
        if (ext == ".vcxproj" || ext == ".vcproj")
            return lang_vcxproj;
        if (ext == ".filters" || ext == ".props" || ext == ".targets" || ext == ".nuspec")
            return lang_props;
        if (ext == ".xcodeproj" || ext == ".xcworkspace" || ext == ".pbxproj" || ext == ".storyboard" ||
            ext == ".xib" || ext == ".plist")
            return lang_xcode;
        if (ext == ".desktop")
            return lang_desktop;
        if (ext == ".service" || ext == ".socket" || ext == ".timer" || ext == ".target" ||
            ext == ".mount" || ext == ".path" || ext == ".slice" || ext == ".scope")
            return lang_service;
        if (ext == ".1" || ext == ".2" || ext == ".3" || ext == ".4" || ext == ".5" || ext == ".6" ||
            ext == ".7" || ext == ".8" || ext == ".9" || ext == ".man" || ext == ".mdoc")
            return lang_man;
        if (ext == ".info")
            return lang_info;
        if (ext == ".puml" || ext == ".plantuml" || ext == ".pu")
            return lang_plantuml;
        if (ext == ".mmd" || ext == ".mermaid")
            return lang_mermaid;
        if (ext == ".dot" || ext == ".gv")
            return lang_dot;
        if (ext == ".drawio" || ext == ".dio")
            return lang_drawio;
        if (ext == ".ics" || ext == ".ical" || ext == ".ifb")
            return lang_calendar;
        if (ext == ".vcf" || ext == ".vcard")
            return lang_contact;
        if (ext == ".rss" || ext == ".atom")
            return lang_rss;
        if (ext == ".torrent")
            return lang_torrent;
        if (ext == ".iso" || ext == ".img" || ext == ".vhd" || ext == ".vmdk" || ext == ".qcow2")
            return lang_iso;
        if (ext == ".parquet" || ext == ".avro" || ext == ".orc" || ext == ".arrow" || ext == ".feather")
            return lang_parquet;
        if (ext == ".h5" || ext == ".hdf5" || ext == ".hdf")
            return lang_hdf5;
        if (ext == ".fbs")
            return lang_flatbuffers;
        if (ext == ".graphqls")
            return lang_graphql_sdl;
        if (ext == ".story.js" || ext == ".story.jsx" || ext == ".story.ts" || ext == ".story.tsx" ||
            ends_with(name, ".stories.js") || ends_with(name, ".stories.jsx") ||
            ends_with(name, ".stories.ts") || ends_with(name, ".stories.tsx"))
            return lang_story;
        if (ends_with(name, ".test.js") || ends_with(name, ".test.jsx") || ends_with(name, ".test.ts") ||
            ends_with(name, ".test.tsx") || ends_with(name, ".spec.js") || ends_with(name, ".spec.jsx") ||
            ends_with(name, ".spec.ts") || ends_with(name, ".spec.tsx") || ends_with(name, "_test.go") ||
            ends_with(name, "_test.py") || ends_with(name, ".test.py") || ends_with(name, "_spec.rb") ||
            ends_with(name, ".spec.rb"))
            return lang_test;
        if (ends_with(name, ".snap"))
            return lang_snapshot;
        if (ext == ".txt" || ext == ".text" || ext == ".nfo" || ext == ".diz")
            return file_text;
        if (ext == ".map" || ext == ".min.js" || ext == ".min.css")
            return file_code;
        if (ext == ".wasm.map")
            return lang_wasm;

        // compound / double extensions via name
        if (ends_with(name, ".d.ts"))
            return lang_ts;
        if (ends_with(name, ".test.js") || ends_with(name, ".spec.js"))
            return lang_test;
        if (ends_with(name, ".module.css") || ends_with(name, ".module.scss"))
            return lang_css;
        if (ends_with(name, ".config.js") || ends_with(name, ".config.ts") ||
            ends_with(name, ".config.mjs") || ends_with(name, ".config.cjs"))
            return lang_config;
        if (ends_with(name, ".service.ts") || ends_with(name, ".controller.ts") ||
            ends_with(name, ".module.ts") || ends_with(name, ".guard.ts") ||
            ends_with(name, ".interceptor.ts") || ends_with(name, ".pipe.ts") ||
            ends_with(name, ".filter.ts") || ends_with(name, ".middleware.ts"))
            return lang_ts;
        if (ends_with(name, ".component.ts") || ends_with(name, ".component.js"))
            return lang_angular;
        if (ends_with(name, ".vue.ts") || ends_with(name, ".vue.js"))
            return lang_vue;

        return file;
    }
}
