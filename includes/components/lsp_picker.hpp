#pragma once

#include <lsp/lsp_models.hpp>
#include <ui/UIComponent.hpp>
#include <utils/fuzzy.hpp>

#include <string>
#include <variant>
#include <vector>

// Fuzzy/list picker for LSP locations, symbols, and code actions.
class LspPicker : public UIComponent
{
public:
    using Payload = std::variant<LspLocation, LspSymbol, LspCodeAction>;

private:
    bool active_ = false;
    std::string title_;
    std::string query_;
    int selected_ = 0;
    int scroll_y_ = 0;
    std::vector<Payload> items_;
    std::vector<int> filtered_; // indices into items_

    void refilter();
    void ensure_selection_visible();
    std::string label_for(const Payload &p) const;

public:
    void draw() override;

    void open_locations(std::string title, std::vector<LspLocation> locs);
    void open_symbols(std::string title, std::vector<LspSymbol> syms);
    void open_actions(std::string title, std::vector<LspCodeAction> actions);

    void close();
    bool is_active() const { return active_; }

    void handle_input(int key);

    bool take_location(LspLocation &out);
    bool take_symbol(LspSymbol &out);
    bool take_action(LspCodeAction &out);

    const std::string &get_query() const { return query_; }
};
