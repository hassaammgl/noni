#pragma once

#include <lsp/diagnostics.hpp>
#include <scm/scm_diff.hpp>
#include <ui/UIComponent.hpp>

#include <optional>

class LineNumber : public UIComponent
{
private:
    int scroll_y = 0;
    int active_line = 0;
    int total_lines = 0;
    const DiagnosticSnapshot *diagnostics_ = nullptr;
    const ScmFileDiff *scm_diff_ = nullptr;

    static std::optional<DiagnosticSeverity> worst_on_line(const DiagnosticSnapshot *diags, int line);

public:
    void draw() override;

    void sync(
        int scroll_y,
        int active_line,
        int total_lines,
        const DiagnosticSnapshot *diagnostics = nullptr,
        const ScmFileDiff *scm_diff = nullptr);
};
