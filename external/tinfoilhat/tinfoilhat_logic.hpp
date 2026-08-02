/*
 * Tinfoil Hat Competition — pure scoring/CSV/leaderboard logic.
 *
 * Firmware-INDEPENDENT: STL + <cstdio> only, no PortaPack headers. This is the
 * single source of truth for the numbers, reused by the Mayhem UI and by the
 * host self-check (test/test_tinfoilhat_logic.cpp) so the math can be verified
 * without the firmware toolchain.
 */
#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace tinfoilhat {

// ── Bands (flyer: HF 2-30, VHF 30-300, UHF 300-3000, SHF 3000-5900 MHz) ──────
enum class Band { HF = 0, VHF = 1, UHF = 2, SHF = 3 };
static constexpr size_t BAND_COUNT = 4;

inline const char* band_name(Band b) {
    switch (b) {
        case Band::HF: return "HF";
        case Band::VHF: return "VHF";
        case Band::UHF: return "UHF";
        default: return "SHF";
    }
}

inline Band band_for(uint32_t mhz) {
    if (mhz < 30) return Band::HF;
    if (mhz < 300) return Band::VHF;
    if (mhz < 3000) return Band::UHF;
    return Band::SHF;
}

// ── 50 test frequencies in MHz, 2..5900, sorted, distinct ───────────────────
// Real-world anchors: shortwave/CB, FM/air/marine VHF, cellular, GPS(1575),
// ADS-B(1090), WiFi 2.4 & 5 GHz, with even fill between.
static constexpr size_t FREQ_COUNT = 50;
inline constexpr std::array<uint16_t, FREQ_COUNT> FREQS_MHZ = {{
    2, 7, 14, 21, 27,                                     // HF (5)
    40, 55, 88, 98, 108, 137, 144, 162, 174, 220, 260,    // VHF (11)
    315, 350, 433, 470, 512, 600, 700, 800, 850, 868,     // UHF ...
    900, 915, 1090, 1176, 1227, 1575, 1710, 1800, 1900,   // ...
    2100, 2400, 2412, 2437, 2450,                         // UHF (24)
    3500, 4900, 5180, 5200, 5240, 5320, 5500, 5600, 5745, 5800  // SHF (10)
}};

// FAST subset (~12) for a quick sweep — key real-world frequencies.
inline constexpr std::array<uint16_t, 12> FREQS_FAST_MHZ = {{
    27, 88, 137, 433, 850, 915, 1575, 1800, 2437, 5180, 5320, 5800}};

// ── One measured frequency ──────────────────────────────────────────────────
struct ScanResult {
    float baseline_db{0.f};
    float hat_db{0.f};
    bool valid{false};
    // Signed on purpose: negative attenuation is real (foil resonance can
    // amplify). Never clamp.
    float attenuation() const { return baseline_db - hat_db; }
};

// ── Scoring ─────────────────────────────────────────────────────────────────
// Mean attenuation over valid results. Returns 0 if none valid.
inline float mean_attenuation(const std::vector<ScanResult>& r) {
    float sum = 0.f;
    int n = 0;
    for (const auto& s : r)
        if (s.valid) { sum += s.attenuation(); ++n; }
    return n ? sum / n : 0.f;
}

// Mean attenuation restricted to one band. freqs[i] pairs with r[i].
inline float band_mean(const std::vector<ScanResult>& r,
                       const uint16_t* freqs, Band band) {
    float sum = 0.f;
    int n = 0;
    for (size_t i = 0; i < r.size(); ++i)
        if (r[i].valid && band_for(freqs[i]) == band) { sum += r[i].attenuation(); ++n; }
    return n ? sum / n : 0.f;
}

// Index of highest attenuation (best shielding), -1 if none valid.
inline int best_index(const std::vector<ScanResult>& r) {
    int best = -1;
    for (size_t i = 0; i < r.size(); ++i)
        if (r[i].valid && (best < 0 || r[i].attenuation() > r[best].attenuation()))
            best = (int)i;
    return best;
}

// Index of lowest attenuation (worst / most amplified), -1 if none valid.
inline int worst_index(const std::vector<ScanResult>& r) {
    int worst = -1;
    for (size_t i = 0; i < r.size(); ++i)
        if (r[i].valid && (worst < 0 || r[i].attenuation() < r[worst].attenuation()))
            worst = (int)i;
    return worst;
}

// ── CSV formatting ──────────────────────────────────────────────────────────
inline std::string fmt1(float v) {
    char buf[24];
    std::snprintf(buf, sizeof(buf), "%.1f", v);
    return buf;
}

// "2412.0,-72.3,-83.1,10.8"
inline std::string format_row(uint16_t mhz, const ScanResult& s) {
    return fmt1((float)mhz) + "," + fmt1(s.baseline_db) + "," +
           fmt1(s.hat_db) + "," + fmt1(s.attenuation());
}

// "AVERAGE,,,18.4"  /  "BEST,2437.0,,31.2"
inline std::string format_summary(const std::string& label, float value) {
    return label + ",,," + fmt1(value);
}
inline std::string format_freq_summary(const std::string& label, uint16_t mhz, float value) {
    return label + "," + fmt1((float)mhz) + ",," + fmt1(value);
}

// ── CSV / header parsing (for Review + Grading) ─────────────────────────────
// Parse a "# key=value" comment line. Returns true and fills value on match.
inline bool parse_header_field(const std::string& line, const std::string& key,
                               std::string& value) {
    std::string prefix = "# " + key + "=";
    if (line.rfind(prefix, 0) != 0) return false;
    value = line.substr(prefix.size());
    // strip trailing CR/whitespace
    while (!value.empty() && (value.back() == '\r' || value.back() == '\n' || value.back() == ' '))
        value.pop_back();
    return true;
}

// Extract the trailing float of a "LABEL,,,<num>" line. Returns true on match.
inline bool parse_labeled_value(const std::string& line, const std::string& label,
                                float& out) {
    if (line.rfind(label + ",", 0) != 0) return false;
    auto pos = line.find_last_of(',');
    if (pos == std::string::npos || pos + 1 >= line.size()) return false;
    out = std::strtof(line.c_str() + pos + 1, nullptr);
    return true;
}

// ── Leaderboard ─────────────────────────────────────────────────────────────
struct RunInfo {
    std::string contestant;
    std::string category;  // "Classic" or "Hybrid"
    float score{0.f};      // mean attenuation (AVERAGE row)
    std::string path;      // source CSV
};

// Best run per contestant for one category, sorted by score descending.
// Hand-rolled dedup + insertion sort — avoids pulling std::sort/std::find_if
// template bloat into the 32KB external-app image. ponytail: O(n^2), fine for
// a contest-sized leaderboard.
inline std::vector<RunInfo> rank_best_per_contestant(const std::vector<RunInfo>& runs,
                                                     const std::string& category) {
    std::vector<RunInfo> best;
    for (const auto& run : runs) {
        if (run.category != category) continue;
        size_t j = 0;
        for (; j < best.size(); ++j)
            if (best[j].contestant == run.contestant) break;
        if (j == best.size())
            best.push_back(run);
        else if (run.score > best[j].score)
            best[j] = run;
    }
    // insertion sort, descending by score
    for (size_t i = 1; i < best.size(); ++i) {
        RunInfo key = best[i];
        size_t k = i;
        while (k > 0 && best[k - 1].score < key.score) {
            best[k] = best[k - 1];
            --k;
        }
        best[k] = key;
    }
    return best;
}

}  // namespace tinfoilhat
