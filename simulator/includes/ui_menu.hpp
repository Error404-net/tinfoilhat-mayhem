#pragma once
#include "ui_widget.hpp"
#include "message.hpp"
#include <functional>
#include <string>
#include <vector>

struct MenuEntry {
    std::string text;
    Color color{};
    void* bitmap{nullptr};  // ignored
    std::function<void(KeyEvent)> on_select;
};

struct MenuView : public Widget {
    bool scrollable_;

    MenuView(Rect r, bool scrollable = false)
        : Widget(r), scrollable_(scrollable) { focusable_ = true; }

    void add_item(MenuEntry entry) { items_.push_back(std::move(entry)); }
    void clear() { items_.clear(); selected_ = 0; }

    bool on_key(const KeyEvent key) override {
        if (items_.empty()) return false;
        if (key == KeyEvent::Select && items_[selected_].on_select) {
            items_[selected_].on_select(key);
            return true;
        }
        if (key == KeyEvent::Down) { if (selected_ + 1 < items_.size()) ++selected_; return true; }
        if (key == KeyEvent::Up)   { if (selected_ > 0) --selected_; return true; }
        return false;
    }

    size_t selected() const { return selected_; }
    void print_items() const {
        for (size_t i = 0; i < items_.size(); ++i)
            printf("  %s %zu: %s\n", (i == selected_) ? ">" : " ", i + 1, items_[i].text.c_str());
    }

private:
    std::vector<MenuEntry> items_;
    size_t selected_{0};
};
