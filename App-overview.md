# Tinfoil Hat Competition — HackRF Mayhem Standalone App

## Overview

A standalone PortaPack H2 + HackRF One application built on the
[Mayhem firmware](https://github.com/portapack-mayhem/mayhem-firmware) that
runs the complete tinfoil hat RF attenuation test without a host computer,
WiFi, or the Flask web app. Results are displayed on the PortaPack screen and
saved as a CSV file to the SD card for later import.

**Target hardware:** PortaPack H2 + HackRF One (or H4M Mayhem Edition)  
**Target firmware:** Mayhem v2.x (portapack-mayhem/mayhem-firmware)  
**Language:** C++17 (same as all Mayhem apps)  
**New files:** 2 (`.hpp` + `.cpp`); minor edits to 2 existing registration files

---

## How Mayhem Apps Work — Primer

Every Mayhem app is a `ui::View` subclass. The PortaPack nav stack pushes and
pops views like a browser history. A typical app:

```
NavigationView  (global stack)
  └─ MyAppView  (your top-level View)
       ├─ Text / Labels
       ├─ Button / Checkbox / NumberField widgets
       └─ pushes child views for sub-screens
```

Key subsystems used by this app:

| Subsystem | Purpose |
|-----------|---------|
| `ReceiverModel` | Tune frequency, set gain, enable/disable RF |
| `ChannelStatsUpdateMessage` | Delivers `max`/`min`/`mean` RSSI power per window |
| `SDCardProvider` / `File` | Write CSV to SD card |
| `ui::ProgressBar` | Sweep progress (0–50 frequencies) |
| `ui::Text` | Status lines, results display |
| `ui::Button` | OK / Cancel / New Test |
| `rtc::RTC` | Timestamp for CSV filename |

The M4 application core tunes the radio and reads RSSI messages that the M0
baseband processor sends back. No custom baseband image is needed — the
existing `am_audio` or `nfm_audio` baseband image provides RSSI via
`ChannelStatsUpdateMessage`. This is the same mechanism used by the Level and
Soundboard apps.

---

## New Repository Structure

```
tinfoilhat-mayhem/
├── README.md
├── firmware/
│   └── application/
│       └── apps/
│           ├── app_tinfoilhat.hpp      ← new
│           └── app_tinfoilhat.cpp      ← new
│
│   (two lines added to existing files:)
│   ├── apps/apps_nav_play.hpp          ← register app in menu
│   └── apps/apps_nav_play.cpp          ← add AppEntry
│
└── docs/
    └── tinfoilhat_mayhem.md            ← this file
```

The simplest integration point is the **Utilities** menu in Mayhem's app
launcher. Add one `AppEntry` there and the app appears alongside existing
tools.

---

## App Flow — Screen by Screen

```
┌──────────────────┐
│  [HOME SCREEN]   │  User navigates Utilities → Tinfoil Hat Test
└────────┬─────────┘
         │ nav_.push<TinfoilHatAppView>()
         ▼
┌──────────────────────────────┐
│  TINFOIL HAT COMPETITION     │  TinfoilHatMenuView
│  ──────────────────────────  │
│  > Start New Test            │
│    View Last Result          │
│    About                     │
│                   [BACK]     │
└──────────────────────────────┘
         │ Start New Test selected
         ▼
┌──────────────────────────────┐
│  STEP 1 OF 3: BASELINE       │  TinfoilHatScanView (mode=BASELINE)
│  ──────────────────────────  │
│  Remove hat from mannequin.  │
│  Place head form in scanner  │
│  position, then press OK.    │
│                              │
│            [OK]   [CANCEL]   │
└──────────────────────────────┘
         │ OK pressed
         ▼
┌──────────────────────────────┐
│  SCANNING BASELINE...        │  TinfoilHatScanView (scanning)
│  ──────────────────────────  │
│  ████████████░░░░  26 / 50   │
│  Current: 2437 MHz           │
│  Power:   -71.4 dBm          │
│                   [CANCEL]   │
└──────────────────────────────┘
         │ all 50 frequencies done
         ▼
┌──────────────────────────────┐
│  STEP 2 OF 3: HAT SCAN       │  TinfoilHatScanView (mode=HAT)
│  ──────────────────────────  │
│  Place tinfoil hat on the    │
│  mannequin, then press OK.   │
│                              │
│            [OK]   [CANCEL]   │
└──────────────────────────────┘
         │ OK pressed → same sweep, mode=HAT
         ▼
┌──────────────────────────────┐
│  RESULTS                     │  TinfoilHatResultsView
│  ──────────────────────────  │
│  Avg attenuation:  18.4 dB   │
│  Best:  2437 MHz   31.2 dB   │
│  Worst:  433 MHz    1.8 dB   │
│  ──────────────────────────  │
│  Saved: TH_20260802_042.csv  │
│                              │
│   [NEW TEST]        [HOME]   │
└──────────────────────────────┘
```

---

## Class Design

### `app_tinfoilhat.hpp`

```cpp
#pragma once
#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "receiver_model.hpp"
#include "message.hpp"
#include <array>
#include <cstdint>

namespace ui {

// 50 test frequencies in MHz (mirrors the Flask app's frequency list)
static constexpr std::array<uint32_t, 50> TINFOILHAT_FREQS_MHZ = {
    88,   98,   108,  137,  144,  174,  433,  470,  518,  700,
    850,  865,  868,  900,  908,  915,  1090, 1227, 1296, 1575,
    1700, 1800, 1900, 2100, 2400, 2402, 2412, 2437, 2441, 2450,
    2455, 2462, 2480, 2600, 3500, 4700, 5180, 5220, 5320, 5500,
    5700, 5800, 5500, 5320, 5220, 5180, 4700, 3500, 2600, 2480
    // (deduplicated and trimmed at build time; full list in .cpp)
};

static constexpr size_t TINFOILHAT_FREQ_COUNT = 50;
static constexpr uint32_t SETTLE_MS = 150;   // ms per frequency before reading

struct ScanResult {
    float baseline_dbm;
    float hat_dbm;
    float attenuation_db() const { return baseline_dbm - hat_dbm; }
};

// ── Results screen ────────────────────────────────────────────────────────────
class TinfoilHatResultsView : public View {
public:
    TinfoilHatResultsView(NavigationView& nav,
                          const std::array<ScanResult, TINFOILHAT_FREQ_COUNT>& results);
    std::string title() const override { return "TinfoilHat"; }

private:
    void save_csv(const std::array<ScanResult, TINFOILHAT_FREQ_COUNT>& results);

    NavigationView& nav_;
    Text  lbl_avg_    { {0,  40, 240, 16}, "" };
    Text  lbl_best_   { {0,  60, 240, 16}, "" };
    Text  lbl_worst_  { {0,  80, 240, 16}, "" };
    Text  lbl_saved_  { {0, 110, 240, 16}, "" };
    Button btn_new_   { {8,  200, 96, 32}, "New Test" };
    Button btn_home_  { {136, 200, 96, 32}, "Home" };
};

// ── Scan screen (used for both baseline and hat passes) ───────────────────────
enum class ScanMode { BASELINE, HAT };

class TinfoilHatScanView : public View {
public:
    TinfoilHatScanView(NavigationView& nav,
                       ScanMode mode,
                       std::array<ScanResult, TINFOILHAT_FREQ_COUNT>* results);
    std::string title() const override { return "TinfoilHat"; }
    void focus() override;

    // Called by the app message dispatcher
    void on_channel_stats(const ChannelStatsUpdateMessage& msg);

private:
    void start_scan();
    void advance_frequency();
    void finish_scan();
    void tune_to(uint32_t freq_mhz);

    NavigationView& nav_;
    ScanMode        mode_;
    std::array<ScanResult, TINFOILHAT_FREQ_COUNT>* results_;

    size_t   current_idx_  { 0 };
    bool     settling_     { false };
    uint32_t settle_start_ { 0 };
    bool     scanning_     { false };

    Text        lbl_status_  { {0,  40, 240, 16}, "" };
    ProgressBar bar_progress_{ {8,  70, 224, 16} };
    Text        lbl_freq_    { {0,  95, 240, 16}, "" };
    Text        lbl_power_   { {0, 115, 240, 16}, "" };
    Button      btn_ok_      { {8,  200, 96, 32},  "OK" };
    Button      btn_cancel_  { {136, 200, 96, 32}, "Cancel" };
};

// ── Top-level app view ────────────────────────────────────────────────────────
class TinfoilHatAppView : public View {
public:
    explicit TinfoilHatAppView(NavigationView& nav);
    std::string title() const override { return "TinfoilHat"; }
    void focus() override;

private:
    NavigationView& nav_;
    std::array<ScanResult, TINFOILHAT_FREQ_COUNT> results_ {};

    Text   lbl_title_ { {0, 40, 240, 16}, "TINFOIL HAT COMPETITION" };
    Button btn_start_ { {8,  90, 224, 32}, "Start New Test" };
    Button btn_about_ { {8, 130, 224, 32}, "About" };
    Button btn_back_  { {8, 200, 224, 32}, "Back" };
};

} // namespace ui
```

---

## Signal Measurement Approach

The key question is how to get a clean power reading at each frequency.
Mayhem's existing Level app (`app_level.cpp`) is the direct reference — it
does exactly this: tunes `ReceiverModel`, reads `ChannelStatsUpdateMessage`,
and displays mean RSSI.

**Per-frequency procedure in `TinfoilHatScanView`:**

```
1. receiver_model.set_target_frequency(freq_hz)
2. receiver_model.set_modulation(ReceiverModel::Mode::NarrowbandFMAudio)
3. Set LNA = 24 dB, VGA = 38 dB, RF amp OFF  (same as Level app defaults)
4. Mark settle_start_ = rtc::now_ms()
5. settling_ = true  → ignore ChannelStats messages until SETTLE_MS elapsed
6. On first ChannelStats after settle period:
     raw_power = msg.mean  (Q1.14 fixed-point from baseband)
     dbm = (raw_power / 128.0f) - 90.0f  (empirical correction, same as Level app)
     Store to results_[current_idx_].baseline_dbm (or hat_dbm)
7. advance to next frequency
```

The `ChannelStatsUpdateMessage` arrives roughly every 100 ms from the M0
baseband. The settle + one reading window is ~250 ms per frequency, giving a
full 50-frequency sweep in about **13 seconds** — fast enough to feel
responsive.

**Gain settings note:** The physical setup (antenna type, distance to TX
source) may need different LNA/VGA values. Expose these as `NumberField`
widgets on an optional settings screen, or hard-code the Level app defaults
and add a `ponytail:` note to revisit if readings are noisy.

---

## CSV Output Format

Filename: `TH_YYYYMMDD_NNN.csv` where NNN increments per file on the SD card.

```csv
frequency_mhz,baseline_dbm,hat_dbm,attenuation_db
88.0,-72.3,-83.1,10.8
98.0,-69.1,-91.4,22.3
108.0,-65.8,-88.2,22.4
...
AVERAGE,,,18.4
BEST_FREQ,2437.0,,,31.2
WORST_FREQ,433.0,,,1.8
```

Written using Mayhem's `File` class:

```cpp
File f;
auto error = f.open("TH_20260802_001.csv", /*create=*/true);
if (!error.is_valid()) {
    f.write_line("frequency_mhz,baseline_dbm,hat_dbm,attenuation_db");
    for (size_t i = 0; i < TINFOILHAT_FREQ_COUNT; i++) {
        // format string, write line
    }
}
```

---

## Registering the App in Mayhem

Two small edits to existing files:

**`firmware/application/apps/apps_nav_play.hpp`** — add to the Utilities
section:

```cpp
{ "Tinfoil Hat", ui::TinfoilHatAppView::title,
  0x00,  // icon index (reuse wrench or RF icon)
  app_location::UTILITIES }
```

**`firmware/application/apps/apps_nav_play.cpp`** — add the include:

```cpp
#include "app_tinfoilhat.hpp"
```

And add it to the `apps_` initializer list alongside the other utilities.

---

## Differences from the Flask App

| | Flask (web) app | Mayhem standalone app |
|---|---|---|
| Host | Raspberry Pi / laptop | PortaPack H2 (self-contained) |
| Display | Browser billboard | PortaPack 320×240 LCD |
| Storage | SQLite database | CSV on SD card |
| Multi-contestant | Yes (leaderboard) | No — one test at a time |
| Voting / photos | Yes | No |
| Admin panel | Yes | No |
| Network required | Yes (WiFi AP or LAN) | No |
| Frequencies | 50 (dynamically selected) | 50 (compile-time fixed list) |
| Score formula | `baseline − hat`, averaged | Same |

The Mayhem app is deliberately minimal — it captures the core
measurement-and-score workflow in a format that works at the table with no
infrastructure. Results on the SD card can be imported into the Flask app's
admin panel after the event if a combined leaderboard is desired.

---

## Build and Flash

Standard Mayhem firmware build process:

```bash
git clone https://github.com/portapack-mayhem/mayhem-firmware
cd mayhem-firmware
# copy app_tinfoilhat.hpp/.cpp into firmware/application/apps/
# apply the two registration edits
mkdir build && cd build
cmake ..
make firmware -j$(nproc)
# flash firmware.bin via DFU or SD card update
```

Refer to the [Mayhem build guide](https://github.com/portapack-mayhem/mayhem-firmware/wiki/Build-Firmware)
for toolchain setup (`arm-none-eabi-gcc`, CMake ≥ 3.20).

---

## Open Questions / Calibration Notes

- **Correction factor:** The `dbm = (raw / 128.0f) - 90.0f` formula is
  empirical. The Level app uses similar math but the exact offset depends on
  antenna, LNA gain, and PCB variant. Add a calibration screen or at minimum
  a compile-time constant to tune against a known signal source before the
  event.

- **Frequency range:** HackRF covers 1 MHz–6 GHz but the PortaPack tuner has
  gaps. Verify all 50 frequencies tune cleanly on the H4M hardware before
  finalizing the list. Drop any that don't produce stable RSSI readings.

- **Settle time:** 150 ms is a conservative starting point. Reduce to 100 ms
  if sweeps feel slow; increase to 200 ms at higher frequencies where the
  synthesizer takes longer to lock.

- **Antenna:** The existing tinfoil hat test rig uses the HackRF's SMA
  antenna inside the mannequin. The same physical setup works unchanged —
  the PortaPack just replaces the laptop.
