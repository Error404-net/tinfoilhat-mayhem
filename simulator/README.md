# tinfoilhat simulator — run & debug the app off-device

Two host-side tools, no firmware toolchain or Docker needed:

## 1. `tinfoilhat-sim` — runs the actual app logic

Compiles the real `ui_tinfoilhat.cpp` against stub Mayhem headers
(`includes/`), then auto-drives the UI: fake `ChannelStatistics` messages,
auto-pressed buttons, full menu → baseline sweep → hat sweep → results → CSV
flow. Exits after one complete run. All file output (TESTS/, tinfoilhat.ini)
is sandboxed under `<cwd>/sim_out/`.

```sh
cmake -B simulator/build -S simulator && cmake --build simulator/build
cd simulator/build && ./tinfoilhat-sim
cat sim_out/TESTS/TH_0001.csv
```

Debug build + sanitizers:

```sh
cmake -B simulator/build-asan -S simulator -DASAN=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build simulator/build-asan
cd simulator/build-asan && ./tinfoilhat-sim        # ASAN+UBSan verified clean
lldb ./tinfoilhat-sim                              # or gdb on Linux
```

The stubs log every RF tune, button press, and navigation event, so a hang or
crash pinpoints the exact UI step. Stubs are minimal by design — if the app
starts using a new Mayhem API, add the smallest stub that compiles.

## 2. `loader_sim` — replays the firmware's .ppma loader

Byte-exact host replica of v2.4.0 `ExternalItemsMenuLoader::run_external_app()`
(the function behind the "can't be read" modal): same 512-byte block loops,
same alignment handling, same `simple_checksum`, simulated 40KB `local_sram_1`
with bounds/overlap tracing.

```sh
g++ -std=c++17 simulator/loader_sim.cpp -o simulator/build/loader_sim
simulator/build/loader_sim path/to/tinfoilhat.ppma path/to/level.ppma
```

Prints the header fields, both copy ranges, and the final checksum verdict
(`LOAD OK` vs `FAIL ("can't be read" modal)`). Running it on our CI artifact
AND official v2.4.0 apps side-by-side is the fastest way to tell "bad file"
from "bad device/SD copy": if `loader_sim` says LOAD OK but the device shows
the modal, the file on the card differs from the artifact or the SD reads are
failing — not the build.

## What the sim intentionally does not do

- No pixel-accurate LCD rendering (paint calls are no-ops).
- No real DSP — `max_db` values are random noise in [-100, -40].
- No emulation of the M4 core or baseband images (that would need a full
  LPC43xx QEMU port; the loader_sim + on-device serial shell cover the
  actual failure modes far cheaper).
