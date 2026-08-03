/*
 * Tinfoil Hat Competition — Mayhem external app (UI).
 *
 * Standalone PortaPack H2/H4M app that runs the RF-attenuation test with no
 * host, WiFi, or Flask app. Baseline + hat sweep, saved to SD as CSV, on-device
 * chart, review, and a Classic/Hybrid leaderboard.
 *
 * Scoring/CSV/leaderboard math lives in tinfoilhat_logic.hpp (host-tested).
 */
#ifndef __UI_TINFOILHAT_H__
#define __UI_TINFOILHAT_H__

#include "app_settings.hpp"
#include "file.hpp"
#include "message.hpp"
#include "radio_state.hpp"
#include "receiver_model.hpp"
#include "tinfoilhat_logic.hpp"
#include "ui.hpp"
#include "ui_menu.hpp"
#include "ui_navigation.hpp"
#include "ui_widget.hpp"

#include <memory>
#include <string>
#include <vector>

namespace ui::external_app::tinfoilhat {

// Logic lives in this same namespace now (linker-section requirement — see
// tinfoilhat_logic.hpp); keep the thl:: spelling used throughout.
namespace thl = ::ui::external_app::tinfoilhat;

// Compile-time tuning knobs. ponytail: settle/avg counts are message windows,
// not ms; tune on real hardware if readings are noisy — promote to settings if
// needed. Calibration offset cancels in baseline-hat, so it's cosmetic only.
static constexpr uint8_t SETTLE_MSGS = 3;   // stat windows to skip after retune
static constexpr uint8_t AVG_MSGS = 4;      // stat windows to average per freq
static constexpr float CALIBRATION_OFFSET_DB = 0.f;

// On-device chart is the dual-bar before/after only, to fit the 32KB
// external-app cap; attenuation/line-overlay/table + export are in the web viewer.

// One completed test: the active frequency list + per-freq results + metadata.
struct TestData {
    std::vector<uint16_t> freqs;
    std::vector<thl::ScanResult> results;
    std::string contestant;
    std::string category{"Classic"};
};

// ── Reusable chart: draws one (or two overlaid) TestData in the current mode ──
// Encoder scrolls the visible frequency window. Used by Results, Review, and
// Grading head-to-head.
class ChartWidget : public Widget {
   public:
    ChartWidget(Rect parent_rect);

    void set_data(const TestData* primary);

    void paint(Painter& painter) override;
    bool on_encoder(const EncoderEvent delta) override;

   private:
    const TestData* primary_{nullptr};
    size_t scroll_{0};  // first visible frequency index

    size_t visible_columns() const;
};

// ── Shared helpers (defined in .cpp) ────────────────────────────────────────
// dB reading -> value with calibration applied.
float apply_calibration(int32_t max_db);
// Save a TestData to TESTS/TH_????.csv; returns the saved filename ("" on fail).
std::string save_test_csv(const TestData& data);
// Load a CSV into TestData (freqs+results+meta). Returns false on failure.
bool load_test_csv(const std::filesystem::path& path, TestData& out);

// ── Menu (app entry) ────────────────────────────────────────────────────────
class TinfoilHatMenuView : public View {
   public:
    TinfoilHatMenuView(NavigationView& nav);
    void focus() override;
    std::string title() const override { return "TinfoilHat"; }

   private:
    NavigationView& nav_;

    Text title_{{0, 24, screen_width, 16}, "TINFOIL HAT"};
    Button btn_start_{{16, 60, screen_width - 32, 30}, "Start Test"};
    Button btn_grading_{{16, 100, screen_width - 32, 30}, "Leaderboard"};
    Button btn_settings_{{16, 140, screen_width - 32, 30}, "Settings"};
    Button btn_back_{{16, 220, screen_width - 32, 30}, "Back"};
};

// ── Scan (category select -> baseline -> hat) ───────────────────────────────
class TinfoilHatScanView : public View {
   public:
    TinfoilHatScanView(NavigationView& nav);
    ~TinfoilHatScanView();
    void focus() override;
    std::string title() const override { return "TinfoilHat"; }

   private:
    enum class Step { Setup, ScanBaseline, HatPrompt, ScanHat, Done };

    NavigationView& nav_;
    RxRadioState radio_state_{};

    int32_t lna_gain_{32};
    int32_t vga_gain_{32};
    int32_t rf_amp_{0};
    int32_t freq_set_{0};  // 0 = FULL, 1 = FAST

    app_settings::SettingsManager settings_{
        "tinfoilhat",
        app_settings::Mode::RX,
        {{"lna"sv, &lna_gain_},
         {"vga"sv, &vga_gain_},
         {"amp"sv, &rf_amp_},
         {"freq_set"sv, &freq_set_}}};

    TestData data_{};
    Step step_{Step::Setup};
    size_t idx_{0};
    uint8_t settle_count_{0};
    int32_t avg_accum_{0};
    uint8_t avg_count_{0};
    bool scanning_{false};

    void configure_radio();
    void begin_phase(bool baseline);
    void tune_current();
    void on_stats(const ChannelStatistics& stats);
    void on_ok();
    void update_ui_for_step();

    Text lbl_step_{{0, 20, screen_width, 16}, ""};
    Text lbl_prompt1_{{0, 48, screen_width, 16}, ""};
    Text lbl_prompt2_{{0, 66, screen_width, 16}, ""};
    OptionsField field_category_{
        {8, 100},
        8,
        {{"Classic", 0}, {"Hybrid", 1}}};
    ProgressBar progress_{{8, 140, screen_width - 16, 16}};
    Text lbl_freq_{{0, 166, screen_width, 16}, ""};
    Text lbl_power_{{0, 186, screen_width, 16}, ""};
    Button btn_ok_{{8, 260, 104, 40}, "OK"};
    Button btn_cancel_{{128, 260, 104, 40}, "Cancel"};

    MessageHandlerRegistration message_handler_stats_{
        Message::ID::ChannelStatistics,
        [this](const Message* const p) {
            this->on_stats(static_cast<const ChannelStatisticsMessage*>(p)->statistics);
        }};
};

// ── Results (after a run) / generic chart viewer (Review) ───────────────────
class TinfoilHatResultsView : public View {
   public:
    // save == true: write CSV (fresh run). save == false: just view (Review).
    TinfoilHatResultsView(NavigationView& nav, TestData data, bool save);
    void focus() override;
    std::string title() const override { return "TinfoilHat"; }

   private:
    NavigationView& nav_;
    TestData data_;

    void refresh_summary();

    // On-screen summary kept minimal (name + avg); per-band and best/worst are
    // in the saved CSV and the web viewer, to save code toward the 32KB cap.
    Text lbl_name_{{0, 20, screen_width, 16}, ""};
    Text lbl_avg_{{0, 42, screen_width, 16}, ""};
    ChartWidget chart_{{0, 90, screen_width, 156}};
    Text lbl_saved_{{0, 294, screen_width, 16}, ""};
    Button btn_done_{{16, 258, screen_width - 32, 30}, "Done"};
};

// (On-device head-to-head compare removed to fit the 32KB external-app cap —
//  the companion web viewer does multi-run compare via row multi-select.)

// (Review+rename removed to fit the 32KB cap — open a saved run from the
//  Leaderboard, and use the web viewer for full review/rename.)

// ── Grading / leaderboard (Classic vs Hybrid, best per contestant) ──────────
class TinfoilHatGradingView : public View {
   public:
    TinfoilHatGradingView(NavigationView& nav);
    void focus() override;
    std::string title() const override { return "TinfoilHat"; }

   private:
    NavigationView& nav_;
    std::vector<thl::RunInfo> all_runs_;
    std::vector<thl::RunInfo> ranked_;
    int32_t category_{0};  // 0 Classic, 1 Hybrid

    void reload();
    void rebuild_ranked();
    void on_select(size_t i);

    Text lbl_title_{{0, 12, screen_width, 16}, "LEADERBOARD"};
    OptionsField field_category_{
        {8, 32},
        8,
        {{"Classic", 0}, {"Hybrid", 1}}};
    Text lbl_hint_{{120, 32, screen_width - 120, 16}, "sel=view"};
    MenuView menu_{{0, 56, screen_width, 224}, true};
    Button btn_back_{{16, 286, screen_width - 32, 28}, "Back"};
};

// ── Settings ────────────────────────────────────────────────────────────────
class TinfoilHatSettingsView : public View {
   public:
    TinfoilHatSettingsView(NavigationView& nav);
    void focus() override;
    std::string title() const override { return "TinfoilHat"; }

   private:
    NavigationView& nav_;

    int32_t lna_gain_{32};
    int32_t vga_gain_{32};
    int32_t rf_amp_{0};
    int32_t freq_set_{0};

    app_settings::SettingsManager settings_{
        "tinfoilhat",
        app_settings::Mode::RX,
        {{"lna"sv, &lna_gain_},
         {"vga"sv, &vga_gain_},
         {"amp"sv, &rf_amp_},
         {"freq_set"sv, &freq_set_}}};

    Labels labels_{
        {{2 * 8, 40}, "LNA:", Theme::getInstance()->fg_light->foreground},
        {{2 * 8, 64}, "VGA:", Theme::getInstance()->fg_light->foreground},
        {{2 * 8, 88}, "AMP:", Theme::getInstance()->fg_light->foreground},
        {{2 * 8, 120}, "Set:", Theme::getInstance()->fg_light->foreground}};

    // LNA 0-40 (step 8), VGA 0-62 (step 2) — HackRF gain grids.
    NumberField field_lna_{{8 * 8, 40}, 3, {0, 40}, 8, ' '};
    NumberField field_vga_{{8 * 8, 64}, 3, {0, 62}, 2, ' '};
    OptionsField field_amp_{
        {8 * 8, 88},
        7,
        {{"off", 0}, {"on +14dB", 14}}};
    OptionsField field_freqset_{
        {8 * 8, 120},
        14,
        {{"Full (50)", 0}, {"Fast (12)", 1}}};
    Button btn_back_{{16, 286, screen_width - 32, 28}, "Back"};
};

}  // namespace ui::external_app::tinfoilhat

#endif /*__UI_TINFOILHAT_H__*/
