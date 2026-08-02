// Host self-check for tinfoilhat_logic.hpp — no firmware toolchain needed.
//   g++ -std=c++17 test/test_tinfoilhat_logic.cpp -o /tmp/thtest && /tmp/thtest
#include "../external/tinfoilhat/tinfoilhat_logic.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>

using namespace tinfoilhat;

static bool close(float a, float b) { return std::fabs(a - b) < 0.05f; }

static ScanResult mk(float base, float hat) { return ScanResult{base, hat, true, }; }

int main() {
    // Frequency table: 50 entries, sorted, in-range, distinct.
    assert(FREQS_MHZ.size() == FREQ_COUNT);
    for (size_t i = 0; i < FREQS_MHZ.size(); ++i) {
        assert(FREQS_MHZ[i] >= 2 && FREQS_MHZ[i] <= 5900);
        if (i) assert(FREQS_MHZ[i] > FREQS_MHZ[i - 1]);  // strictly increasing => distinct
    }

    // Band classification boundaries.
    assert(band_for(2) == Band::HF);
    assert(band_for(29) == Band::HF);
    assert(band_for(30) == Band::VHF);
    assert(band_for(299) == Band::VHF);
    assert(band_for(300) == Band::UHF);
    assert(band_for(2999) == Band::UHF);
    assert(band_for(3000) == Band::SHF);
    assert(band_for(5900) == Band::SHF);

    // Attenuation is signed; negatives survive.
    assert(close(mk(-70, -85).attenuation(), 15));
    assert(close(mk(-70, -66).attenuation(), -4));  // amplified => negative

    // Mean, best, worst over a mixed set (one negative drags the mean down).
    std::vector<ScanResult> r = {mk(-70, -85), mk(-70, -66), mk(-80, -100)};
    //   attenuations: +15, -4, +20  => mean 10.333
    assert(close(mean_attenuation(r), 10.333f));
    assert(best_index(r) == 2);   // +20
    assert(worst_index(r) == 1);  // -4

    // Invalid entries are ignored by scoring.
    r.push_back(ScanResult{-10, -99, /*valid=*/false});  // would be +89 if counted
    assert(close(mean_attenuation(r), 10.333f));
    assert(best_index(r) == 2);

    // Band means: two UHF freqs, one HF.
    uint16_t freqs[] = {10, 700, 2400};  // HF, UHF, UHF
    std::vector<ScanResult> br = {mk(-50, -60), mk(-70, -85), mk(-70, -95)};
    //   HF: +10 ; UHF: +15, +25 => 20
    assert(close(band_mean(br, freqs, Band::HF), 10));
    assert(close(band_mean(br, freqs, Band::UHF), 20));
    assert(close(band_mean(br, freqs, Band::VHF), 0));  // none => 0

    // CSV formatting.
    assert(format_row(2412, mk(-72.3f, -83.1f)) == "2412.0,-72.3,-83.1,10.8");
    assert(format_summary("AVERAGE", 18.4f) == "AVERAGE,,,18.4");
    assert(format_freq_summary("BEST", 2437, 31.2f) == "BEST,2437.0,,31.2");

    // Header + summary parsing round-trip.
    std::string name, cat;
    assert(parse_header_field("# contestant=Alice", "contestant", name) && name == "Alice");
    assert(parse_header_field("# category=Hybrid\r", "category", cat) && cat == "Hybrid");
    assert(!parse_header_field("# contestant=Alice", "category", cat));
    float avg = 0;
    assert(parse_labeled_value("AVERAGE,,,18.4", "AVERAGE", avg) && close(avg, 18.4f));
    assert(parse_labeled_value("WORST,433.0,,-4.2", "WORST", avg) && close(avg, -4.2f));
    assert(!parse_labeled_value("AVERAGE,,,18.4", "BEST", avg));

    // Leaderboard: best run per contestant per category, ranked desc, categories separate.
    std::vector<RunInfo> runs = {
        {"Alice", "Classic", 20.0f, "a1.csv"},
        {"Alice", "Classic", 24.1f, "a2.csv"},  // Alice's better Classic run
        {"Bob", "Classic", 12.3f, "b1.csv"},
        {"Carol", "Classic", 19.8f, "c1.csv"},
        {"Alice", "Hybrid", 30.0f, "a3.csv"},   // different category, must not mix in
    };
    auto classic = rank_best_per_contestant(runs, "Classic");
    assert(classic.size() == 3);                 // Alice, Bob, Carol (deduped)
    assert(classic[0].contestant == "Alice" && close(classic[0].score, 24.1f));
    assert(classic[1].contestant == "Carol");
    assert(classic[2].contestant == "Bob");
    auto hybrid = rank_best_per_contestant(runs, "Hybrid");
    assert(hybrid.size() == 1 && close(hybrid[0].score, 30.0f));

    std::printf("ALL TESTS PASSED\n");
    return 0;
}
