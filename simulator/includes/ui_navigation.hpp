#pragma once
#include "ui_widget.hpp"
#include <functional>
#include <memory>
#include <tuple>
#include <vector>

// NavigationView: push/pop/replace set a pending action; run() drives the
// event loop (delivers fake stats, auto-presses visible buttons).
// Implementation lives in sim_main.cpp.
class NavigationView {
public:
    enum class NavOp { None, Pop, Push, Replace };

    struct PendingNav {
        NavOp op{NavOp::None};
        std::function<std::unique_ptr<View>()> factory;
    };

    std::vector<std::unique_ptr<View>> stack_;
    PendingNav pending_;

    void run();   // defined in sim_main.cpp

    void pop() {
        pending_ = {NavOp::Pop, nullptr};
    }

    template <typename T, typename... Args>
    void push(Args&&... args) {
        auto cap = std::tuple<std::decay_t<Args>...>(std::forward<Args>(args)...);
        pending_ = {NavOp::Push, [this, cap = std::move(cap)]() mutable -> std::unique_ptr<View> {
            return std::apply([this](auto&&... a) -> std::unique_ptr<View> {
                return std::make_unique<T>(*this, std::forward<decltype(a)>(a)...);
            }, std::move(cap));
        }};
    }

    template <typename T, typename... Args>
    void replace(Args&&... args) {
        auto cap = std::tuple<std::decay_t<Args>...>(std::forward<Args>(args)...);
        pending_ = {NavOp::Replace, [this, cap = std::move(cap)]() mutable -> std::unique_ptr<View> {
            return std::apply([this](auto&&... a) -> std::unique_ptr<View> {
                return std::make_unique<T>(*this, std::forward<decltype(a)>(a)...);
            }, std::move(cap));
        }};
    }
};
