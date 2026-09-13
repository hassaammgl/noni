#include <ui/theme.hpp>
#include <utils/logger.hpp>

#include <algorithm>
#include <cstdint>
#include <map>

namespace
{
    struct Rgb
    {
        short r;
        short g;
        short b;
    };

    constexpr Rgb editor_bg{0x1E, 0x1E, 0x1E};
    constexpr Rgb editor_fg{0xD4, 0xD4, 0xD4};
    constexpr Rgb title_bg{0x3C, 0x3C, 0x3C};
    constexpr Rgb title_fg{0xCC, 0xCC, 0xCC};
    constexpr Rgb title_inactive_fg{0x99, 0x99, 0x99};
    constexpr Rgb status_bg{0x00, 0x7A, 0xCC};
    constexpr Rgb status_fg{0xFF, 0xFF, 0xFF};
    constexpr Rgb status_debug_bg{0xCC, 0x66, 0x33};
    constexpr Rgb insert_bg{0x40, 0xA0, 0x40};
    constexpr Rgb visual_bg{0xC5, 0x86, 0xC0};
    constexpr Rgb sidebar_bg{0x25, 0x25, 0x26};
    constexpr Rgb sidebar_fg{0xCC, 0xCC, 0xCC};
    constexpr Rgb sidebar_title_fg{0xBB, 0xBB, 0xBB};
    constexpr Rgb dir_fg{0xDC, 0xDC, 0xAA};
    constexpr Rgb file_fg{0xCC, 0xCC, 0xCC};
    constexpr Rgb list_sel_bg{0x04, 0x39, 0x5E};
    constexpr Rgb list_hover_bg{0x2A, 0x2D, 0x2E};
    constexpr Rgb line_number_fg{0x85, 0x85, 0x85};
    constexpr Rgb line_number_active_fg{0xC6, 0xC6, 0xC6};
    constexpr Rgb current_line_bg{0x2A, 0x2A, 0x2A};
    constexpr Rgb selection_bg{0x26, 0x4F, 0x78};
    constexpr Rgb inactive_sel_bg{0x3A, 0x3D, 0x41};
    constexpr Rgb cursor_fg{0xAE, 0xAF, 0xAD};
    constexpr Rgb whitespace_fg{0x3E, 0x3E, 0x3E};
    constexpr Rgb indent_fg{0x40, 0x40, 0x40};
    constexpr Rgb indent_active_fg{0x70, 0x70, 0x70};
    constexpr Rgb bracket_fg{0x88, 0x88, 0x88};
    constexpr Rgb search_bg{0x51, 0x5C, 0x6A};
    constexpr Rgb search_current_bg{0xEA, 0x5C, 0x00};
    constexpr Rgb word_hl_bg{0x3A, 0x3D, 0x41};
    constexpr Rgb tab_bar_bg{0x25, 0x25, 0x26};
    constexpr Rgb tab_inactive_bg{0x2D, 0x2D, 0x2D};
    constexpr Rgb tab_inactive_fg{0x96, 0x96, 0x96};
    constexpr Rgb tab_active_fg{0xFF, 0xFF, 0xFF};
    constexpr Rgb tab_modified_fg{0xE2, 0xC0, 0x8D};
    constexpr Rgb popup_bg{0x25, 0x25, 0x26};
    constexpr Rgb popup_border_fg{0x45, 0x45, 0x45};
    constexpr Rgb input_bg{0x3C, 0x3C, 0x3C};
    constexpr Rgb button_bg{0x0E, 0x63, 0x9C};
    constexpr Rgb button_secondary_bg{0x3A, 0x3D, 0x41};
    constexpr Rgb badge_bg{0x4D, 0x4D, 0x4D};
    constexpr Rgb dim_fg{0x80, 0x80, 0x80};
    constexpr Rgb border_fg{0x47, 0x47, 0x47};
    constexpr Rgb breadcrumb_fg{0xCC, 0xCC, 0xCC};
    constexpr Rgb minimap_fg{0x7F, 0x7F, 0x7F};
    constexpr Rgb error_fg{0xF4, 0x47, 0x47};
    constexpr Rgb warning_fg{0xCC, 0xA7, 0x00};
    constexpr Rgb info_fg{0x37, 0x94, 0xFF};
    constexpr Rgb hint_fg{0xB0, 0xB0, 0xB0};
    constexpr Rgb git_added_fg{0x81, 0xB8, 0x8B};
    constexpr Rgb git_modified_fg{0xE2, 0xC0, 0x8D};
    constexpr Rgb git_deleted_fg{0xC7, 0x4E, 0x39};
    constexpr Rgb git_untracked_fg{0x73, 0xC9, 0x91};
    constexpr Rgb git_conflict_fg{0xE4, 0x67, 0x6B};
    constexpr Rgb gutter_added_fg{0x48, 0x7E, 0x02};
    constexpr Rgb gutter_modified_fg{0x1B, 0x81, 0xA8};
    constexpr Rgb gutter_deleted_fg{0xF1, 0x4C, 0x4C};
    constexpr Rgb comment_fg{0x6A, 0x99, 0x55};
    constexpr Rgb comment_doc_fg{0x60, 0x8B, 0x4E};
    constexpr Rgb keyword_fg{0x56, 0x9C, 0xD6};
    constexpr Rgb keyword_control_fg{0xC5, 0x86, 0xC0};
    constexpr Rgb string_fg{0xCE, 0x91, 0x78};
    constexpr Rgb string_escape_fg{0xD7, 0xBA, 0x7D};
    constexpr Rgb string_regexp_fg{0xD1, 0x69, 0x69};
    constexpr Rgb number_fg{0xB5, 0xCE, 0xA8};
    constexpr Rgb constant_fg{0x4F, 0xC1, 0xFF};
    constexpr Rgb function_fg{0xDC, 0xDC, 0xAA};
    constexpr Rgb type_fg{0x4E, 0xC9, 0xB0};
    constexpr Rgb interface_fg{0xB8, 0xD7, 0xA3};
    constexpr Rgb variable_fg{0x9C, 0xDC, 0xFE};
    constexpr Rgb invalid_fg{0xF4, 0x47, 0x47};
    constexpr Rgb markup_link_fg{0x37, 0x94, 0xFF};

    short next_custom_id = 16;
    std::map<std::uint32_t, short> color_ids;

    short to_ncurses_channel(short value)
    {
        return static_cast<short>((value * 1000) / 255);
    }

    std::uint32_t rgb_key(Rgb c)
    {
        return (static_cast<std::uint32_t>(c.r) << 16) |
               (static_cast<std::uint32_t>(c.g) << 8) |
               static_cast<std::uint32_t>(c.b);
    }

    short nearest_256(Rgb c)
    {
        auto cube = [](short v) -> short
        {
            if (v < 48)
                return 0;
            if (v < 115)
                return 1;
            short q = static_cast<short>((v - 35) / 40);
            return std::clamp(q, short{0}, short{5});
        };

        short cube_index = static_cast<short>(
            16 + 36 * cube(c.r) + 6 * cube(c.g) + cube(c.b));

        short gray = static_cast<short>((c.r + c.g + c.b) / 3);
        short gray_index = 232;
        if (gray >= 238)
            gray_index = 255;
        else if (gray >= 8)
            gray_index = static_cast<short>(232 + (gray - 8) / 10);

        auto cube_level = [](short n) -> short
        {
            return n == 0 ? 0 : static_cast<short>(n * 40 + 55);
        };

        short cr = cube_level(cube(c.r));
        short cg = cube_level(cube(c.g));
        short cb = cube_level(cube(c.b));
        int cube_dist = (c.r - cr) * (c.r - cr) +
                        (c.g - cg) * (c.g - cg) +
                        (c.b - cb) * (c.b - cb);

        short gray_value = static_cast<short>(8 + (gray_index - 232) * 10);
        int gray_dist = (c.r - gray_value) * (c.r - gray_value) +
                        (c.g - gray_value) * (c.g - gray_value) +
                        (c.b - gray_value) * (c.b - gray_value);

        return gray_dist < cube_dist ? gray_index : cube_index;
    }

    short nearest_8(Rgb c)
    {
        short index = 0;
        if (c.r >= 128)
            index |= 1;
        if (c.g >= 128)
            index |= 2;
        if (c.b >= 128)
            index |= 4;
        return index;
    }

    short make_color(Rgb c)
    {
        const std::uint32_t key = rgb_key(c);
        if (auto it = color_ids.find(key); it != color_ids.end())
            return it->second;

        short id;
        if (can_change_color() && next_custom_id < COLORS)
        {
            id = next_custom_id++;
            init_color(
                id,
                to_ncurses_channel(c.r),
                to_ncurses_channel(c.g),
                to_ncurses_channel(c.b));
        }
        else if (COLORS >= 256)
        {
            id = nearest_256(c);
        }
        else
        {
            id = nearest_8(c);
        }

        color_ids[key] = id;
        return id;
    }

    void set_pair(short pair, Rgb fg, Rgb bg)
    {
        if (pair <= 0 || pair >= COLOR_PAIRS)
            return;
        init_pair(pair, make_color(fg), make_color(bg));
    }
}

void Theme::init()
{
    start_color();
    // Keep opaque backgrounds so terminal wallpaper does not bleed through.

    color_ids.clear();
    next_custom_id = 16;

    set_pair(Editor, editor_fg, editor_bg);
    set_pair(Header, title_fg, title_bg);
    set_pair(HeaderInactive, title_inactive_fg, title_bg);
    set_pair(Statusbar, status_fg, status_bg);
    set_pair(StatusbarModeNormal, editor_bg, status_fg);
    set_pair(StatusbarModeInsert, editor_bg, insert_bg);
    set_pair(StatusbarModeVisual, editor_bg, visual_bg);
    set_pair(StatusbarDebugging, status_fg, status_debug_bg);
    set_pair(Sidebar, sidebar_fg, sidebar_bg);
    set_pair(SidebarTitle, sidebar_title_fg, sidebar_bg);
    set_pair(SidebarDir, dir_fg, sidebar_bg);
    set_pair(SidebarFile, file_fg, sidebar_bg);
    set_pair(SidebarSelected, status_fg, list_sel_bg);
    set_pair(SidebarHover, sidebar_fg, list_hover_bg);
    set_pair(LineNumber, line_number_fg, editor_bg);
    set_pair(LineNumberActive, line_number_active_fg, editor_bg);
    set_pair(CurrentLine, editor_fg, current_line_bg);
    set_pair(Selection, editor_fg, selection_bg);
    set_pair(InactiveSelection, editor_fg, inactive_sel_bg);
    set_pair(Cursor, cursor_fg, editor_bg);
    set_pair(Whitespace, whitespace_fg, editor_bg);
    set_pair(IndentGuide, indent_fg, editor_bg);
    set_pair(IndentGuideActive, indent_active_fg, editor_bg);
    set_pair(MatchingBracket, bracket_fg, editor_bg);
    set_pair(SearchMatch, editor_fg, search_bg);
    set_pair(SearchMatchCurrent, status_fg, search_current_bg);
    set_pair(WordHighlight, editor_fg, word_hl_bg);
    set_pair(TabActive, tab_active_fg, editor_bg);
    set_pair(TabInactive, tab_inactive_fg, tab_bar_bg);
    set_pair(TabModified, tab_modified_fg, tab_bar_bg);
    set_pair(Popup, sidebar_fg, popup_bg);
    set_pair(PopupSelected, status_fg, list_sel_bg);
    set_pair(PopupBorder, popup_border_fg, popup_bg);
    set_pair(Input, title_fg, input_bg);
    set_pair(InputFocus, status_fg, input_bg);
    set_pair(Button, status_fg, button_bg);
    set_pair(ButtonSecondary, title_fg, button_secondary_bg);
    set_pair(Badge, status_fg, badge_bg);
    set_pair(Menu, sidebar_fg, popup_bg);
    set_pair(MenuSelected, status_fg, list_sel_bg);
    set_pair(Notification, sidebar_fg, popup_bg);
    set_pair(PeekView, editor_fg, current_line_bg);
    set_pair(Dim, dim_fg, editor_bg);
    set_pair(Border, border_fg, editor_bg);
    set_pair(Breadcrumb, breadcrumb_fg, editor_bg);
    set_pair(Minimap, minimap_fg, editor_bg);

    set_pair(Error, error_fg, editor_bg);
    set_pair(Warning, warning_fg, editor_bg);
    set_pair(Info, info_fg, editor_bg);
    set_pair(Hint, hint_fg, editor_bg);
    set_pair(ErrorLine, error_fg, current_line_bg);
    set_pair(WarningLine, warning_fg, current_line_bg);

    set_pair(GitAdded, git_added_fg, editor_bg);
    set_pair(GitModified, git_modified_fg, editor_bg);
    set_pair(GitDeleted, git_deleted_fg, editor_bg);
    set_pair(GitUntracked, git_untracked_fg, editor_bg);
    set_pair(GitConflict, git_conflict_fg, editor_bg);
    set_pair(GutterAdded, gutter_added_fg, editor_bg);
    set_pair(GutterModified, gutter_modified_fg, editor_bg);
    set_pair(GutterDeleted, gutter_deleted_fg, editor_bg);

    set_pair(Comment, comment_fg, editor_bg);
    set_pair(CommentDoc, comment_doc_fg, editor_bg);
    set_pair(Keyword, keyword_fg, editor_bg);
    set_pair(KeywordControl, keyword_control_fg, editor_bg);
    set_pair(Storage, keyword_fg, editor_bg);
    set_pair(String, string_fg, editor_bg);
    set_pair(StringEscape, string_escape_fg, editor_bg);
    set_pair(StringRegexp, string_regexp_fg, editor_bg);
    set_pair(Number, number_fg, editor_bg);
    set_pair(Constant, constant_fg, editor_bg);
    set_pair(Function, function_fg, editor_bg);
    set_pair(Macro, keyword_control_fg, editor_bg);
    set_pair(Type, type_fg, editor_bg);
    set_pair(Class, type_fg, editor_bg);
    set_pair(Interface, interface_fg, editor_bg);
    set_pair(Variable, variable_fg, editor_bg);
    set_pair(Parameter, variable_fg, editor_bg);
    set_pair(Property, variable_fg, editor_bg);
    set_pair(Operator, editor_fg, editor_bg);
    set_pair(Preprocessor, keyword_control_fg, editor_bg);
    set_pair(Namespace, type_fg, editor_bg);
    set_pair(Punctuation, editor_fg, editor_bg);
    set_pair(Tag, keyword_fg, editor_bg);
    set_pair(Attribute, variable_fg, editor_bg);
    set_pair(Invalid, invalid_fg, editor_bg);
    set_pair(MarkupHeading, keyword_fg, editor_bg);
    set_pair(MarkupBold, function_fg, editor_bg);
    set_pair(MarkupItalic, string_fg, editor_bg);
    set_pair(MarkupLink, markup_link_fg, editor_bg);
    set_pair(MarkupRaw, string_fg, editor_bg);
    Logger::debug("Theme color pairs initialized");
}
