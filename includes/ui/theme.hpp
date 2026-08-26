#pragma once

#include <ncurses.h>

// VS Code Dark+ inspired ncurses color pairs.
// Use with COLOR_PAIR(Theme::Editor), etc.
namespace Theme
{
    // Workbench
    constexpr short Editor = 1;
    constexpr short Header = 2;
    constexpr short HeaderInactive = 3;
    constexpr short Statusbar = 4;
    constexpr short StatusbarModeNormal = 5;
    constexpr short StatusbarModeInsert = 6;
    constexpr short StatusbarModeVisual = 7;
    constexpr short StatusbarDebugging = 8;
    constexpr short Sidebar = 9;
    constexpr short SidebarTitle = 10;
    constexpr short SidebarDir = 11;
    constexpr short SidebarFile = 12;
    constexpr short SidebarSelected = 13;
    constexpr short SidebarHover = 14;
    constexpr short LineNumber = 15;
    constexpr short LineNumberActive = 16;
    constexpr short CurrentLine = 17;
    constexpr short Selection = 18;
    constexpr short InactiveSelection = 19;
    constexpr short Cursor = 20;
    constexpr short Whitespace = 21;
    constexpr short IndentGuide = 22;
    constexpr short IndentGuideActive = 23;
    constexpr short MatchingBracket = 24;
    constexpr short SearchMatch = 25;
    constexpr short SearchMatchCurrent = 26;
    constexpr short WordHighlight = 27;
    constexpr short TabActive = 28;
    constexpr short TabInactive = 29;
    constexpr short TabModified = 30;
    constexpr short Popup = 31;
    constexpr short PopupSelected = 32;
    constexpr short PopupBorder = 33;
    constexpr short Input = 34;
    constexpr short InputFocus = 35;
    constexpr short Button = 36;
    constexpr short ButtonSecondary = 37;
    constexpr short Badge = 38;
    constexpr short Menu = 39;
    constexpr short MenuSelected = 40;
    constexpr short Notification = 41;
    constexpr short PeekView = 42;
    constexpr short Dim = 43;
    constexpr short Border = 44;
    constexpr short Breadcrumb = 45;
    constexpr short Minimap = 46;

    // Diagnostics
    constexpr short Error = 47;
    constexpr short Warning = 48;
    constexpr short Info = 49;
    constexpr short Hint = 50;
    constexpr short ErrorLine = 51;
    constexpr short WarningLine = 52;

    // Git
    constexpr short GitAdded = 53;
    constexpr short GitModified = 54;
    constexpr short GitDeleted = 55;
    constexpr short GitUntracked = 56;
    constexpr short GitConflict = 57;
    constexpr short GutterAdded = 58;
    constexpr short GutterModified = 59;
    constexpr short GutterDeleted = 60;

    // Syntax
    constexpr short Comment = 61;
    constexpr short CommentDoc = 62;
    constexpr short Keyword = 63;
    constexpr short KeywordControl = 64;
    constexpr short Storage = 65;
    constexpr short String = 66;
    constexpr short StringEscape = 67;
    constexpr short StringRegexp = 68;
    constexpr short Number = 69;
    constexpr short Constant = 70;
    constexpr short Function = 71;
    constexpr short Macro = 72;
    constexpr short Type = 73;
    constexpr short Class = 74;
    constexpr short Interface = 75;
    constexpr short Variable = 76;
    constexpr short Parameter = 77;
    constexpr short Property = 78;
    constexpr short Operator = 79;
    constexpr short Preprocessor = 80;
    constexpr short Namespace = 81;
    constexpr short Punctuation = 82;
    constexpr short Tag = 83;
    constexpr short Attribute = 84;
    constexpr short Invalid = 85;
    constexpr short MarkupHeading = 86;
    constexpr short MarkupBold = 87;
    constexpr short MarkupItalic = 88;
    constexpr short MarkupLink = 89;
    constexpr short MarkupRaw = 90;

    void init();
}
