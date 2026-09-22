#include <ui/theme.hpp>
#include <utils/logger.hpp>

#include <algorithm>
#include <cstdint>
#include <map>

#include "theme_detail.hpp"

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
