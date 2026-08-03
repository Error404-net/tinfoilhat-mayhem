/*
 * tinfoilhat-sim: host-side emulator for ui_tinfoilhat.cpp.
 *
 * Compiles the full app C++ against stub Mayhem headers. The NavigationView
 * event loop auto-drives the UI: delivers fake ChannelStatistics messages and
 * auto-presses visible (non-Cancel, non-Back) buttons every N ticks.
 *
 * Build:
 *   cmake -B build -S simulator && cmake --build build
 *   ./simulator/build/tinfoilhat-sim
 *
 * Debug:
 *   cmake -B build -S simulator -DCMAKE_BUILD_TYPE=Debug && cmake --build build
 *   lldb ./simulator/build/tinfoilhat-sim   (or gdb on Linux)
 *
 * ASAN:
 *   cmake -B build -S simulator -DASAN=ON && cmake --build build
 *   ./simulator/build/tinfoilhat-sim
 */
#include "ui_tinfoilhat.hpp"
#include "message.hpp"
#include "portapack.hpp"

#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <memory>

// ── Global state required by stubs ──────────────────────────────────────────

namespace portapack {
    ReceiverModel receiver_model;
}

// Focused widget — set by Widget::focus() / Button::focus().
// NavigationView::run() uses this to auto-press buttons.
static Widget* g_focused = nullptr;
void sim_set_focused(Widget* w) { g_focused = w; }

// ── NavigationView event loop ────────────────────────────────────────────────

void NavigationView::run() {
    int tick = 0;
    int ok_cooldown = 0;    // ticks since last auto-press; prevents double-fire
    int max_depth_seen = 0; // detect when we've finished one full scan run
    srand((unsigned)time(nullptr));

    printf("[SIM] starting event loop\n");

    while (!stack_.empty() || pending_.op != NavOp::None) {
        // --- Apply any pending navigation first ---
        if (pending_.op != NavOp::None) {
            auto [op, factory] = std::move(pending_);
            pending_ = {NavOp::None, nullptr};
            g_focused = nullptr;
            ok_cooldown = 0;
            tick = 0;

            if (op == NavOp::Pop) {
                printf("[NAV] pop  (stack depth %zu → %zu)\n",
                       stack_.size(), stack_.size() - 1);
                stack_.pop_back();
                // Exit after the first complete run (scan + results → back to menu).
                if (max_depth_seen >= 2 && stack_.size() == 1) {
                    printf("[SIM] one full run complete — exiting\n");
                    stack_.clear();
                    return;
                }
                if (!stack_.empty()) stack_.back()->focus();
            } else {
                if (op == NavOp::Replace && !stack_.empty()) {
                    printf("[NAV] replace\n");
                    stack_.pop_back();
                } else {
                    printf("[NAV] push  (depth → %zu)\n", stack_.size() + 1);
                }
                stack_.push_back(factory());
                if ((int)stack_.size() > max_depth_seen) max_depth_seen = (int)stack_.size();
                stack_.back()->focus();
            }
            continue;
        }

        ++tick;
        ++ok_cooldown;

        // --- Deliver a fake ChannelStatistics message every tick ---
        {
            ChannelStatisticsMessage msg;
            // Realistic noisy signal: -40 to -100 dBm
            msg.statistics.max_db = -40 - (rand() % 61);
            dispatch_message(&msg);
        }

        // --- Auto-press a button every 60 ticks (once per "frame batch") ---
        // Pressing OK while the scan is in progress is a no-op (on_ok default
        // branch does nothing), so this is safe to fire repeatedly.
        if (ok_cooldown >= 60) {
            ok_cooldown = 0;

            // Prefer the globally focused widget.
            bool pressed = false;
            if (g_focused && !g_focused->is_hidden()) {
                pressed = g_focused->on_key(KeyEvent::Select);
            }

            // Fallback: find the first non-hidden non-cancel button in the
            // current view's children (covers ResultsView where btn_done_ isn't
            // auto-focused by the chart's focus() call).
            if (!pressed && !stack_.empty()) {
                for (auto* w : stack_.back()->children_) {
                    if (w->is_hidden()) continue;
                    auto* b = dynamic_cast<Button*>(w);
                    if (!b) continue;
                    if (b->label_ == "Cancel" || b->label_ == "Back") continue;
                    printf("[SIM] fallback press: \"%s\"\n", b->label_.c_str());
                    b->on_key(KeyEvent::Select);
                    break;
                }
            }
        }

        // Safety exit: if menu is back on top with no scan running and no
        // pending nav after a while, we're done.
        if (tick > 5000) {
            printf("[SIM] tick limit reached — exiting\n");
            break;
        }
    }

    printf("[SIM] event loop complete\n");
}

// ── Entry point ──────────────────────────────────────────────────────────────

int main() {
    // Sandbox all file output (TESTS/, tinfoilhat.ini) into ./sim_out so the
    // sim never litters the repo root.
    std::filesystem::create_directories("sim_out");
    std::filesystem::current_path("sim_out");

    printf("=== tinfoilhat-sim ===\n");
    printf("Simulating: menu → scan (fake RF) → results → done\n");
    printf("CSV files written to ./sim_out/TESTS/\n\n");

    NavigationView nav;
    nav.push<ui::external_app::tinfoilhat::TinfoilHatMenuView>();
    nav.run();

    printf("\n=== simulation complete ===\n");
    return 0;
}
