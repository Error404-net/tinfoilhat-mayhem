# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Current State

This repo contains **only a design document** (`App-overview.md`) — no source code, no build system, no git history yet. The document specifies a planned PortaPack H2 + HackRF One application ("Tinfoil Hat Competition" standalone RF attenuation test) to be built as a Mayhem firmware app. Treat `App-overview.md` as the spec; the code it describes does not exist here yet.

## What Is Being Built

A standalone Mayhem firmware app (C++17) that runs the tinfoil-hat RF attenuation test entirely on the PortaPack — no host computer, WiFi, or Flask web app. It sweeps 50 fixed frequencies twice (baseline, then hat-on), computes per-frequency attenuation (`baseline_dbm − hat_dbm`), shows results on the 320×240 LCD, and writes a CSV to the SD card.

The app is a **standalone Mayhem** app, not part of this repo's build. It gets copied into a checkout of [portapack-mayhem/mayhem-firmware](https://github.com/portapack-mayhem/mayhem-firmware) and built there:

- **New files:** `app_tinfoilhat.hpp` + `app_tinfoilhat.cpp` in `firmware/application/apps/`
- **Registration edits:** add an `AppEntry` (Utilities menu) + `#include` in `apps_nav_play.hpp`/`.cpp`
- **Build:** inside a mayhem-firmware checkout — `mkdir build && cd build && cmake .. && make firmware -j$(nproc)`; needs the ARM toolchain (`arm-none-eabi-gcc`, CMake ≥ 3.20). See the [Mayhem build guide](https://github.com/portapack-mayhem/mayhem-firmware/wiki/Build-Firmware).

## Architecture Notes (from the spec)

- Every Mayhem app is a `ui::View` subclass pushed/popped on a global `NavigationView` stack. Three views planned: `TinfoilHatAppView` (menu) → `TinfoilHatScanView` (reused for both baseline and hat passes via a `ScanMode` enum) → `TinfoilHatResultsView`.
- **Measurement path:** tune `ReceiverModel` to each frequency, then read `mean` RSSI from `ChannelStatsUpdateMessage` (sent ~every 100 ms by the M0 baseband). No custom baseband image — reuse the existing `nfm_audio`/`am_audio` image. `app_level.cpp` in mayhem-firmware is the direct reference implementation for this tune-and-read loop.
- Per-frequency flow: tune → settle (`SETTLE_MS`, ignore stats messages during settle) → take one reading → advance. `dbm = (raw_mean / 128.0f) - 90.0f`.

## Calibration Is Hardware-Dependent — Don't Hard-Code Blindly

The spec's magic numbers are empirical and must stay tunable against real hardware:

- The `- 90.0f` dbm offset depends on antenna, LNA gain, and PCB variant. Keep it a named constant (or calibration screen), not a buried literal.
- `SETTLE_MS = 150` is a starting point — higher frequencies may need more settle time for synth lock.
- Not all 50 frequencies are guaranteed to tune cleanly on H4M hardware; the list needs on-device verification and pruning. Note the frequency array in the spec has a placeholder/duplicated tail — the real deduplicated list belongs in the `.cpp`.

Leave these as adjustable knobs; a minimal model can't see the physical drift.
