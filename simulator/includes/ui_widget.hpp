#pragma once
#include "ui.hpp"
#include <functional>
#include <initializer_list>
#include <string>
#include <vector>
#include <utility>
#include <cstdio>

// Forward declare so Widget can reference NavigationView for focus tracking.
// Widget::focus() sets a global g_focused; see sim_main.cpp.
extern void sim_set_focused(struct Widget*);

struct Widget {
    Rect rect_;
    bool hidden_{false};
    bool focusable_{false};
    std::vector<Widget*> children_;

    explicit Widget(Rect r) : rect_(r) {}
    virtual ~Widget() = default;

    Rect screen_rect() const { return rect_; }
    void set_dirty() {}
    void set_focusable(bool v) { focusable_ = v; }
    virtual void focus() { sim_set_focused(this); }
    void hidden(bool v) { hidden_ = v; }
    bool is_hidden() const { return hidden_; }
    virtual void paint(Painter&) {}
    virtual bool on_encoder(const EncoderEvent) { return false; }
    virtual bool on_key(const KeyEvent) { return false; }

    void add_children(std::initializer_list<Widget*> list) {
        for (auto* w : list) children_.push_back(w);
    }
};

struct View : public Widget {
    explicit View(Rect r = {}) : Widget(r) {}
    virtual void focus() override { sim_set_focused(this); }
    virtual std::string title() const { return ""; }
    virtual void set_dirty() {}
};

struct Text : public Widget {
    std::string text_;
    Text(Rect r, std::string t = "") : Widget(r), text_(std::move(t)) {}
    void set(const std::string& t) { text_ = t; }
    void set(std::string&& t) { text_ = std::move(t); }
};

struct Button : public Widget {
    std::string label_;
    std::function<void(Button&)> on_select;

    Button(Rect r, std::string label) : Widget(r), label_(std::move(label)) { focusable_ = true; }

    void focus() override {
        printf("[SIM] focus: button \"%s\"\n", label_.c_str());
        sim_set_focused(this);
    }

    bool on_key(const KeyEvent key) override {
        if (key == KeyEvent::Select && on_select && !hidden_) {
            printf("[SIM] press: button \"%s\"\n", label_.c_str());
            on_select(*this);
            return true;
        }
        return false;
    }
};

struct LabelDef {
    Point p;
    std::string text;
    Color color;
};

struct Labels : public Widget {
    Labels(std::initializer_list<LabelDef> items) : Widget({}), items_(items) {}
private:
    std::vector<LabelDef> items_;
};

struct OptionsField : public Widget {
    using value_t = int32_t;
    using option_t = std::pair<std::string, value_t>;
    using options_t = std::vector<option_t>;
    std::function<void(size_t, value_t)> on_change;

    OptionsField(Point p, int chars, options_t opts)
        : Widget({p.x, p.y, chars * 8, 16}), options_(std::move(opts)), idx_(0) {
        focusable_ = true;
    }

    size_t selected_index() const { return idx_; }
    value_t selected_value() const { return options_[idx_].second; }

    void set_by_value(value_t v) {
        for (size_t i = 0; i < options_.size(); ++i)
            if (options_[i].second == v) { idx_ = i; return; }
    }

    bool on_key(const KeyEvent key) override {
        if ((key == KeyEvent::Select || key == KeyEvent::Right) && !hidden_) {
            idx_ = (idx_ + 1) % options_.size();
            if (on_change) on_change(idx_, options_[idx_].second);
            return true;
        }
        return false;
    }

private:
    options_t options_;
    size_t idx_;
};

struct NumberField : public Widget {
    std::function<void(int32_t)> on_change;

    NumberField(Point p, int digits, std::pair<int32_t, int32_t> range, int32_t /*step*/, char /*fill*/)
        : Widget({p.x, p.y, digits * 8, 16}),
          min_(range.first), max_(range.second), value_(range.first) {
        focusable_ = true;
    }

    void set_value(int32_t v) {
        value_ = v < min_ ? min_ : (v > max_ ? max_ : v);
    }
    int32_t value() const { return value_; }

    void focus() override { sim_set_focused(this); }

private:
    int32_t min_, max_, value_;
};

struct ProgressBar : public Widget {
    ProgressBar(Rect r) : Widget(r) {}
    void set_max(size_t) {}
    void set_value(size_t) {}
};
