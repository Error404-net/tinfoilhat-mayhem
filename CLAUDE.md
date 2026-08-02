# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What This Repo Is

Source for **Tinfoil Hat Competition**, a PortaPack H2/H4M + HackRF standalone
[Mayhem](https://github.com/portapack-mayhem/mayhem-firmware) external app that
runs the contest's RF-attenuation test on-device (no host, WiFi, or Flask app).
It sweeps 50 frequencies (2–5900 MHz) twice — baseline then hat-on — scores the
shielding, charts before/after on the LCD, saves CSVs to the SD card, and has an
on-device review + Classic/Hybrid leaderboard.

`App-overview.md` is the original design doc; the four `*_flyer.html` files are
the contest rules/scoring that drove requirements (bands, categories,
best-score-per-category, averaged readings, signed attenuation). Where the doc
and flyers disagree, the flyers win.

## Layout

- `external/tinfoilhat/` — the app, laid out to drop into a mayhem-firmware
  checkout at `firmware/application/external/tinfoilhat/`.
  - `main.cpp` — `application_information_t` registration → builds to `tinfoilhat.ppma`.
  - `ui_tinfoilhat.hpp/.cpp` — all views (menu, scan, results, review, grading,
    compare, settings) + the reusable `ChartWidget`.
  - `tinfoilhat_logic.hpp` — **pure, firmware-independent** STL-only header:
    frequency list, bands, `ScanResult`, scoring, CSV format/parse, leaderboard
    ranking. Single source of truth for the numbers.
  - `external.cmake.patch` — the two registration edits (EXTCPPSRC + EXTAPPLIST).
  - `viewer.html` — self-contained companion web viewer for the SD `/TESTS/`
    folder (loads the CSVs, SVG charts, sort/filter, SVG/PNG export, print). No
    server/CDN. Its CSV parser must stay in sync with `write_csv` in
    `ui_tinfoilhat.cpp` (same `# contestant=`/`# category=`/`AVERAGE` contract).
- `test/test_tinfoilhat_logic.cpp` — host self-check for the pure logic.
- `.github/workflows/build-ppma.yml` — CI that grafts the app into
  mayhem-firmware, runs the Docker build, and publishes `tinfoilhat.ppma`.

## Build & Test

- **Host self-check (runs anywhere, no firmware toolchain):**
  `g++ -std=c++17 test/test_tinfoilhat_logic.cpp -o /tmp/thtest && /tmp/thtest`
  Any change to scoring/CSV/leaderboard logic must keep this green. Add asserts
  here rather than only testing on-device.
- **Firmware `.ppma`:** cannot be compiled in this repo — it needs the Mayhem
  toolchain. Either push and let `build-ppma.yml` produce it, or manually clone
  mayhem-firmware, copy `external/tinfoilhat/` in, apply `external.cmake.patch`,
  and run the Docker build (`docker build -f dockerfile-nogit .` then
  `docker run -i -v "$PWD:/havoc" portapack-dev`). Output:
  `build/firmware/application/tinfoilhat.ppma`. Install = drop it in SD `/APPS/`.

## Architecture Notes

- **External app plugin model** (not the old `apps_nav_play` edits): a
  self-contained folder that compiles to a `.ppma` loaded from SD `/APPS/` — no
  firmware reflash. Reference/template: `external/level/` in mayhem-firmware.
  Registration takes **two** firmware files, both automated in
  `build-ppma.yml`: `external.cmake` (sources + app list) **and** `external.ld`
  (a unique `ram_external_app_tinfoilhat` MEMORY region + a matching
  `.external_app_tinfoilhat` SECTIONS block). Forgetting the `.ld` entry yields
  an empty app image and an `IndexError` in `export_external_apps.py` at link
  time — not a compile error.
- **`main.cpp` include order matters:** `external_app.hpp` must be included
  LAST (after `ui_navigation.hpp`), or `app_location_t` is undefined.
- **Measurement** mirrors the Level app: `baseband::run_image(nfm_audio)`,
  tune `receiver_model`, read `ChannelStatistics.max_db` (already in dB — the
  design doc's `(raw/128)-90` Q1.14 math is obsolete) via a
  `MessageHandlerRegistration`. The sweep is driven by the stat-message stream
  with a settle-then-average gate per frequency — **not** timers.
- **Never navigate (`nav_.push/replace`) from inside the ChannelStatistics
  handler** — it can free the view mid-dispatch. The scan finishes into a
  `Done` step and navigates from the OK button handler instead.
- **Attenuation is signed** — negative (foil resonance amplifying) is real and
  must drag the average down; never clamp.
- **Persistence** via `app_settings::SettingsManager` (auto-saves to SD). CSV
  filenames auto-increment via `next_filename_matching_pattern`.

## Editing Conventions

- Keep numeric/scoring/parse logic in `tinfoilhat_logic.hpp` (so the host test
  covers it); keep firmware/UI concerns in `ui_tinfoilhat.*`.
- `SETTLE_MSGS` / `AVG_MSGS` / `CALIBRATION_OFFSET_DB` in the header are
  hardware-tuning knobs marked with `ponytail:` — adjust on real hardware, don't
  bury them as literals.
- The README is intentionally in 90s warez-scene `.NFO` style — keep that voice
  if you edit it.
