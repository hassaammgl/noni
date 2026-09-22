#pragma once

#include <editor/selection.hpp>
#include <editor/window.hpp>
#include <utils/text_metrics.hpp>

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

struct MotionResult
{
    Cursor dest{.line = 0, .column = 0};
    TextRange range{};
    bool linewise = false;
    bool ok = true;
};
