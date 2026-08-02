/*
 * Tinfoil Hat Competition — Mayhem external app (implementation).
 */
#include "ui_tinfoilhat.hpp"

#include "baseband_api.hpp"
#include "portapack.hpp"
#include "rtc_time.hpp"
#include "string_format.hpp"
#include "ui_font_fixed_8x16.hpp"

#include <algorithm>
#include <cstdlib>

using namespace portapack;

namespace ui::external_app::tinfoilhat {

// ── Shared helpers ──────────────────────────────────────────────────────────
// scan_root_files returns bare filenames; results live under TESTS/.
static std::filesystem::path in_tests(const std::filesystem::path& name) {
    return std::filesystem::path{u"TESTS"} / name;
}

float apply_calibration(int32_t max_db) {
    return (float)max_db + CALIBRATION_OFFSET_DB;
}

// Write a full CSV (header + rows + summary + bands + best/worst) to path.
bool write_csv(const std::filesystem::path& path, const TestData& data) {
    File f;
    if (f.create(path)) return false;  // Optional<Error> truthy => failed

    std::string name = data.contestant.empty() ? path.stem().string() : data.contestant;
    f.write_line("# contestant=" + name);
    f.write_line("# category=" + data.category);
    f.write_line("# timestamp=" + to_string_datetime(rtc_time::now()));
    f.write_line("frequency_mhz,baseline_dbm,hat_dbm,attenuation_db");

    for (size_t i = 0; i < data.results.size(); ++i)
        f.write_line(thl::format_row(data.freqs[i], data.results[i]));

    f.write_line(thl::format_summary("AVERAGE", thl::mean_attenuation(data.results)));

    const thl::Band bands[] = {thl::Band::HF, thl::Band::VHF, thl::Band::UHF, thl::Band::SHF};
    const char* blabel[] = {"BAND_HF", "BAND_VHF", "BAND_UHF", "BAND_SHF"};
    for (int b = 0; b < 4; ++b)
        f.write_line(thl::format_summary(
            blabel[b], thl::band_mean(data.results, data.freqs.data(), bands[b])));

    int bi = thl::best_index(data.results);
    int wi = thl::worst_index(data.results);
    if (bi >= 0)
        f.write_line(thl::format_freq_summary("BEST", data.freqs[bi], data.results[bi].attenuation()));
    if (wi >= 0)
        f.write_line(thl::format_freq_summary("WORST", data.freqs[wi], data.results[wi].attenuation()));
    return true;
}

std::string save_test_csv(const TestData& data) {
    ensure_directory(u"TESTS");
    auto path = next_filename_matching_pattern(u"TESTS/TH_????.csv");
    if (path.empty()) return "";
    if (!write_csv(path, data)) return "";
    return path.filename().string();
}

// Iterate lines of a text blob without <sstream>.
template <typename F>
static void for_each_line(const std::string& text, F&& fn) {
    size_t pos = 0;
    while (pos < text.size()) {
        size_t nl = text.find('\n', pos);
        std::string line = text.substr(pos, nl == std::string::npos ? std::string::npos : nl - pos);
        pos = (nl == std::string::npos) ? text.size() : nl + 1;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (!line.empty()) fn(line);
    }
}

bool load_test_csv(const std::filesystem::path& path, TestData& out) {
    auto r = File::read_file(path);
    if (!r) return false;
    out = TestData{};
    out.contestant = path.stem().string();

    for_each_line(r.value(), [&](const std::string& line) {
        std::string v;
        if (thl::parse_header_field(line, "contestant", v)) { out.contestant = v; return; }
        if (thl::parse_header_field(line, "category", v)) { out.category = v; return; }
        char c0 = line[0];
        if (!(c0 == '-' || (c0 >= '0' && c0 <= '9'))) return;  // skip header/summary rows
        char* end = nullptr;
        float mhz = std::strtof(line.c_str(), &end);
        if (*end != ',') return;
        float base = std::strtof(end + 1, &end);
        if (*end != ',') return;
        float hat = std::strtof(end + 1, &end);
        if (*end != ',') return;
        thl::ScanResult sr;
        sr.baseline_db = base;
        sr.hat_db = hat;
        sr.valid = true;
        out.freqs.push_back((uint16_t)(mhz + 0.5f));
        out.results.push_back(sr);
    });
    return !out.results.empty();
}

// ── ChartWidget ─────────────────────────────────────────────────────────────
ChartWidget::ChartWidget(Rect parent_rect)
    : Widget{parent_rect} {
    set_focusable(true);
}

void ChartWidget::set_data(const TestData* primary) {
    primary_ = primary;
    scroll_ = 0;
    set_dirty();
}

size_t ChartWidget::visible_columns() const {
    size_t v = screen_rect().width() / 14;
    return v ? v : 1;
}

bool ChartWidget::on_encoder(const EncoderEvent delta) {
    if (!primary_ || primary_->results.empty()) return false;
    int n = (int)primary_->results.size();
    int s = (int)scroll_ + delta;
    if (s < 0) s = 0;
    if (s > n - 1) s = n - 1;
    scroll_ = (size_t)s;
    set_dirty();
    return true;
}

void ChartWidget::paint(Painter& painter) {
    const auto r = screen_rect();
    painter.fill_rectangle(r, Color::black());
    if (!primary_ || primary_->results.empty()) {
        painter.draw_string({r.left() + 4, r.top() + 4}, font::fixed_8x16,
                            Color::white(), Color::black(), "No data");
        return;
    }
    // Dual bars: baseline (grey) vs hat (green) per frequency.
    const int bottom = r.bottom() - 12;  // room for x labels
    const int h = bottom - (r.top() + 2);
    const size_t n = primary_->results.size();
    const size_t vis = visible_columns();
    const int colw = r.width() / (int)vis;
    const float DB_LO = -110.f, DB_HI = -10.f;

    auto y_db = [&](float db) {
        float t = (db - DB_LO) / (DB_HI - DB_LO);
        t = std::max(0.f, std::min(1.f, t));
        return bottom - (int)(t * h);
    };

    for (size_t c = 0; c < vis; ++c) {
        size_t i = scroll_ + c;
        if (i >= n) break;
        const int x = r.left() + (int)c * colw;
        const auto& s = primary_->results[i];
        const int bw = colw / 2 - 1;
        const int yb = y_db(s.baseline_db);
        const int yh = y_db(s.hat_db);
        painter.fill_rectangle({x + 1, yb, bw, bottom - yb}, Color(120, 120, 120));
        painter.fill_rectangle({x + 1 + bw, yh, bw, bottom - yh}, Color::green());

        if ((c % 2) == 0) {
            painter.draw_string({x, bottom + 1}, font::fixed_8x16, Color(150, 150, 150),
                                Color::black(), to_string_dec_uint(primary_->freqs[i]));
        }
    }
}

// ── Menu (entry) ────────────────────────────────────────────────────────────
TinfoilHatMenuView::TinfoilHatMenuView(NavigationView& nav)
    : nav_{nav} {
    add_children({&title_, &btn_start_, &btn_grading_, &btn_settings_, &btn_back_});

    btn_start_.on_select = [this](Button&) { nav_.push<TinfoilHatScanView>(); };
    btn_grading_.on_select = [this](Button&) { nav_.push<TinfoilHatGradingView>(); };
    btn_settings_.on_select = [this](Button&) { nav_.push<TinfoilHatSettingsView>(); };
    btn_back_.on_select = [this](Button&) { nav_.pop(); };
}

void TinfoilHatMenuView::focus() {
    btn_start_.focus();
}

// ── Scan ────────────────────────────────────────────────────────────────────
TinfoilHatScanView::TinfoilHatScanView(NavigationView& nav)
    : nav_{nav} {
    add_children({&lbl_step_, &lbl_prompt1_, &lbl_prompt2_, &field_category_, &progress_,
                  &lbl_freq_, &lbl_power_, &btn_ok_, &btn_cancel_});

    if (freq_set_ == 1)
        data_.freqs.assign(thl::FREQS_FAST_MHZ.begin(), thl::FREQS_FAST_MHZ.end());
    else
        data_.freqs.assign(thl::FREQS_MHZ.begin(), thl::FREQS_MHZ.end());
    data_.results.assign(data_.freqs.size(), thl::ScanResult{});

    btn_ok_.on_select = [this](Button&) { on_ok(); };
    btn_cancel_.on_select = [this](Button&) { nav_.pop(); };

    update_ui_for_step();
}

TinfoilHatScanView::~TinfoilHatScanView() {
    receiver_model.disable();
    baseband::shutdown();
}

void TinfoilHatScanView::focus() {
    btn_ok_.focus();
}

void TinfoilHatScanView::configure_radio() {
    baseband::run_image(portapack::spi_flash::image_tag_nfm_audio);
    receiver_model.set_modulation(ReceiverModel::Mode::NarrowbandFMAudio);
    receiver_model.set_nbfm_configuration(2);  // 16k, matches Level default
    receiver_model.set_sampling_rate(3072000);
    receiver_model.set_baseband_bandwidth(1750000);
    receiver_model.set_lna(lna_gain_);
    receiver_model.set_vga(vga_gain_);
    receiver_model.set_rf_amp(rf_amp_ != 0);
    receiver_model.enable();
}

void TinfoilHatScanView::tune_current() {
    receiver_model.set_target_frequency((rf::Frequency)data_.freqs[idx_] * 1000000);
    settle_count_ = 0;
    avg_count_ = 0;
    avg_accum_ = 0;
    lbl_freq_.set("F " + to_string_dec_uint(data_.freqs[idx_]) + "M");
}

void TinfoilHatScanView::begin_phase(bool baseline) {
    step_ = baseline ? Step::ScanBaseline : Step::ScanHat;
    idx_ = 0;
    progress_.set_max(data_.freqs.size());
    progress_.set_value(0);
    scanning_ = true;
    update_ui_for_step();
    tune_current();
}

void TinfoilHatScanView::on_ok() {
    switch (step_) {
        case Step::Setup:
            data_.category = (field_category_.selected_index() == 1) ? "Hybrid" : "Classic";
            configure_radio();
            begin_phase(true);
            break;
        case Step::HatPrompt:
            begin_phase(false);
            break;
        case Step::Done: {
            // replace() pops (destroys) this view before constructing the next,
            // so copy data_ to a local (survives on the stack) — passing the
            // member directly would be a use-after-free.
            TestData finished = data_;
            nav_.replace<TinfoilHatResultsView>(std::move(finished), true);
            break;
        }
        default:
            break;  // ignore while scanning
    }
}

void TinfoilHatScanView::on_stats(const ChannelStatistics& stats) {
    if (!scanning_) return;
    if (settle_count_ < SETTLE_MSGS) {
        settle_count_++;
        return;
    }
    avg_accum_ += stats.max_db;
    avg_count_++;
    lbl_power_.set("P " + to_string_dec_int(stats.max_db) + "dB");
    if (avg_count_ < AVG_MSGS) return;

    const float avg = apply_calibration(avg_accum_ / (int)AVG_MSGS);
    auto& r = data_.results[idx_];
    if (step_ == Step::ScanBaseline) {
        r.baseline_db = avg;
    } else {
        r.hat_db = avg;
        r.valid = true;
    }

    idx_++;
    progress_.set_value(idx_);
    if (idx_ >= data_.freqs.size()) {
        scanning_ = false;
        if (step_ == Step::ScanBaseline) {
            step_ = Step::HatPrompt;
        } else {
            // Don't navigate from inside the message handler (would free this
            // view mid-dispatch). Stop the radio and wait for an OK press.
            receiver_model.disable();
            step_ = Step::Done;
        }
        update_ui_for_step();
        return;
    }
    tune_current();
}

void TinfoilHatScanView::update_ui_for_step() {
    const bool setup = (step_ == Step::Setup);
    const bool scanning = (step_ == Step::ScanBaseline || step_ == Step::ScanHat);

    field_category_.hidden(!setup);
    lbl_prompt1_.hidden(scanning);
    lbl_prompt2_.hidden(scanning);
    progress_.hidden(setup);
    lbl_freq_.hidden(!scanning);
    lbl_power_.hidden(!scanning);
    btn_ok_.hidden(scanning);

    switch (step_) {
        case Step::Setup:
            lbl_step_.set("1/2 BASELINE");
            lbl_prompt1_.set("Remove hat.");
            lbl_prompt2_.set("Pick cat, OK.");
            break;
        case Step::ScanBaseline:
            lbl_step_.set("SCAN BASELINE");
            break;
        case Step::HatPrompt:
            lbl_step_.set("2/2 HAT SCAN");
            lbl_prompt1_.set("Place hat on head.");
            lbl_prompt2_.set("Press OK.");
            break;
        case Step::ScanHat:
            lbl_step_.set("SCAN HAT");
            break;
        case Step::Done:
            lbl_step_.set("DONE");
            lbl_prompt1_.set("Test finished.");
            lbl_prompt2_.set("OK = results.");
            break;
    }
    set_dirty();
}

// ── Results / chart viewer ──────────────────────────────────────────────────
TinfoilHatResultsView::TinfoilHatResultsView(NavigationView& nav, TestData data, bool save)
    : nav_{nav}, data_{std::move(data)} {
    add_children({&lbl_name_, &lbl_avg_, &chart_, &lbl_saved_, &btn_done_});

    chart_.set_data(&data_);

    if (save) {
        std::string fname = save_test_csv(data_);
        lbl_saved_.set(fname.empty() ? "SAVE FAIL" : ("Saved: " + fname));
    } else {
        lbl_saved_.set(data_.contestant);
    }

    refresh_summary();
    btn_done_.on_select = [this](Button&) { nav_.pop(); };
}

void TinfoilHatResultsView::focus() {
    chart_.focus();
}

void TinfoilHatResultsView::refresh_summary() {
    lbl_name_.set(data_.contestant + " [" + data_.category + "]");
    lbl_avg_.set("Avg " + to_string_dec_int((int)thl::mean_attenuation(data_.results)) + "dB");
    // (per-band + best/worst live in the CSV and the web viewer)
}

// ── Grading / leaderboard ───────────────────────────────────────────────────
TinfoilHatGradingView::TinfoilHatGradingView(NavigationView& nav)
    : nav_{nav} {
    add_children({&lbl_title_, &field_category_, &lbl_hint_, &menu_, &btn_back_});
    btn_back_.on_select = [this](Button&) { nav_.pop(); };
    field_category_.on_change = [this](size_t, OptionsField::value_t v) {
        category_ = v;
        rebuild_ranked();
    };
    reload();
}

void TinfoilHatGradingView::focus() {
    field_category_.focus();
}

void TinfoilHatGradingView::reload() {
    // Load each CSV as a full run and reduce to leaderboard fields — reuses
    // load_test_csv (no separate header-only parser) to save code.
    all_runs_.clear();
    auto files = scan_root_files(u"TESTS", u"*.csv");
    for (const auto& p : files) {
        TestData d;
        if (!load_test_csv(in_tests(p), d)) continue;
        thl::RunInfo info;
        info.contestant = d.contestant;
        info.category = d.category;
        info.score = thl::mean_attenuation(d.results);
        info.path = in_tests(p).string();
        all_runs_.push_back(info);
    }
    rebuild_ranked();
}

void TinfoilHatGradingView::rebuild_ranked() {
    const std::string cat = (category_ == 1) ? "Hybrid" : "Classic";
    ranked_ = thl::rank_best_per_contestant(all_runs_, cat);

    menu_.clear();
    for (size_t i = 0; i < ranked_.size(); ++i) {
        const auto& run = ranked_[i];
        std::string label = to_string_dec_uint(i + 1, 2) + " " + run.contestant + "  " +
                            to_string_dec_int((int)run.score) + "dB";
        Color col = (i == 0) ? Color::yellow() : Color::white();
        menu_.add_item({label, col, nullptr, [this, i](KeyEvent) { on_select(i); }});
    }
    lbl_hint_.set(ranked_.empty() ? "no runs" : "sel=view");
    set_dirty();
}

void TinfoilHatGradingView::on_select(size_t i) {
    // Open the selected run's chart. (Head-to-head compare lives in the web
    // viewer.)
    if (i >= ranked_.size()) return;
    TestData d;
    if (!load_test_csv(ranked_[i].path, d)) return;
    nav_.push<TinfoilHatResultsView>(std::move(d), false);
}

// ── Settings ────────────────────────────────────────────────────────────────
TinfoilHatSettingsView::TinfoilHatSettingsView(NavigationView& nav)
    : nav_{nav} {
    add_children({&labels_, &field_lna_, &field_vga_, &field_amp_,
                  &field_freqset_, &btn_back_});

    field_lna_.set_value(lna_gain_);
    field_vga_.set_value(vga_gain_);
    field_amp_.set_by_value(rf_amp_);
    field_freqset_.set_by_value(freq_set_);

    field_lna_.on_change = [this](int32_t v) { lna_gain_ = v; };
    field_vga_.on_change = [this](int32_t v) { vga_gain_ = v; };
    field_amp_.on_change = [this](size_t, OptionsField::value_t v) { rf_amp_ = v; };
    field_freqset_.on_change = [this](size_t, OptionsField::value_t v) { freq_set_ = v; };

    btn_back_.on_select = [this](Button&) { nav_.pop(); };
}

void TinfoilHatSettingsView::focus() {
    field_lna_.focus();
}

}  // namespace ui::external_app::tinfoilhat
