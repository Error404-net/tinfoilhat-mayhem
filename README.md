```
 ______________________________________________________________________________
/\                                                                             /\
\_|                                                                           |_/
  |    ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄    |
  |                                                                           |
  |   ▀█▀ █ █▄ █ █▀▀ █▀█ █ █    █ █ ▄▀█ ▀█▀   █▀▀ █▀█ █▀▄▀█ █▀█ █▀█   ▄▄  ▀█▀ |
  |    █  █ █ ▀█ █▀  █▄█ █ █▄▄  █▀█ █▀█  █    █▄▄ █▄█ █ ▀ █ █▀▀ █▄█        █  |
  |                                                                           |
  |            . : *  H A C K R F   M A Y H E M   S T A N D A L O N E  * : .  |
  |    ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄   |
  |                                                                           |
  |     [ x ] PortaPack H2/H4M external app ...... proudly self-contained     |
  |     [ x ] versioning ............. YYYY.MM.patch  (year-based)           |
  |     [ x ] protection ............ tinfoil, 3+ layers, no gaps             |
  |     [ x ] supplier .............. error404                                |
  |                                                                           |
 _|                                                                           |_
/ \                                                                           / \
\_/___________________________________________________________________________\_/


  ╔════════════════════════════════════════════════════════════════════════╗
  ║  0x01 .. WHAT iS THiS THiNG                                            ║
  ╚════════════════════════════════════════════════════════════════════════╝

     Source for the Tinfoil Hat Competition Mayhem external app. Runs the
     whole RF-attenuation shielding test directly on the PortaPack H2/H4M.
     No laptop. No WiFi. No Flask. Just tinfoil, a mannequin, and RF.

        Sweeps 50 frequencies 2 MHz → 5900 MHz — baseline then hat-on.
        Scores the shielding. Draws the before/after chart on the LCD.
        Drops a CSV on the SD card. On-device leaderboard. Classic/Hybrid.

        ATTENUATION (dB) = BASELINE - HAT      higher = better tinfoil.
        (negatives are REAL - foil resonance can amplify. it counts against you.)

     One file to install: tinfoilhat.ppma  (drag-n-drop, no reflash)
     One page to review results: viewer.html  (self-contained, runs off SD)


  ╔════════════════════════════════════════════════════════════════════════╗
  ║  0x02 .. iNSTALL  ( two files, two folders )                           ║
  ╚════════════════════════════════════════════════════════════════════════╝

     Grab the latest release from the Releases tab (right side of this page):

        tinfoilhat.ppma  →  /APPS/   on your PortaPack SD card
        viewer.html      →  /TESTS/  on your PortaPack SD card

     Boot → Games menu → "Tinfoil Hat"

     That's it. No toolchain. No reflash. Drop and go.
     .ppma is portable to any PortaPack on a compatible Mayhem nightly.


  ╔════════════════════════════════════════════════════════════════════════╗
  ║  0x03 .. USiNG iT                                                      ║
  ╚════════════════════════════════════════════════════════════════════════╝

     START TEST     pick category (Classic/Hybrid), remove hat, OK for
                    baseline sweep → place hat, OK for hat sweep.
                    auto-saves TESTS/TH_####.csv and shows the chart.

     LEADERBOARD    best run per contestant, Classic & Hybrid ranked
                    separately. select a row to open that run's chart.

     SETTINGS       RF gain (LNA/VGA/AMP), freq set (Full 50 / Fast 12).
                    all settings persist to SD automatically.

     SCORE BANDS:  HF 2-30 . VHF 30-300 . UHF 300-3000 . SHF 3000-5900 (MHz)


  ╔════════════════════════════════════════════════════════════════════════╗
  ║  0x04 .. THE WEB ViEWER  ( big screen, off the SD card )               ║
  ╚════════════════════════════════════════════════════════════════════════╝

     viewer.html sits in /TESTS/ next to the CSVs. open it straight off the
     card (double-click) or browse to it from any browser. no internet needed.

        * "Open TESTS folder" (Chrome/Edge) slurps every TH_####.csv at once
        * sortable/filterable leaderboard, multi-select rows
        * SVG charts: baseline-vs-hat, attenuation, overlay N runs
        * export SVG or PNG, download CSV, Print
        * band breakdown (HF/VHF/UHF/SHF) per run

     per-run review, head-to-head compare, and export all live here — trimmed
     on-device to stay under Mayhem's hard 32KB external-app cap.


  ╔════════════════════════════════════════════════════════════════════════╗
  ║  0x05 .. VERSiONiNG  ( year.month.patch )                              ║
  ╚════════════════════════════════════════════════════════════════════════╝

     releases follow CalVer:  YYYY.MM.patch

        2026.08.1  →  first release of August 2026
        2026.08.2  →  second patch same month
        2026.09.1  →  September drop

     push a tag  2026.08.1  and CI auto-builds + attaches tinfoilhat.ppma
     and viewer.html to a GitHub Release. no manual upload needed.


  ╔════════════════════════════════════════════════════════════════════════╗
  ║  0x06 .. BUiLDiNG FROM SOURCE  ( only needed to hack on it )           ║
  ╚════════════════════════════════════════════════════════════════════════╝

     [ the lazy way -- let a robot do it ]

        push to GitHub (or manually trigger build-ppma.yml).
        the action grafts the app into a fresh mayhem-firmware checkout,
        runs the official Docker build, and spits out tinfoilhat.ppma.
        tag a commit  2026.08.1  and it attaches to a Release automatically.

     [ the manual way ]

        git clone --recurse-submodules \
            https://github.com/portapack-mayhem/mayhem-firmware
        cp -r external/tinfoilhat \
            mayhem-firmware/firmware/application/external/
        # register in TWO firmware files (see external.cmake.patch +
        # .github/workflows/build-ppma.yml for the exact edits to external.ld)
        cd mayhem-firmware
        docker build -t portapack-dev -f dockerfile-nogit .
        docker run -i -v "$PWD:/havoc" portapack-dev
        # → build/firmware/application/tinfoilhat.ppma

     [ host self-check (no toolchain needed) ]

        g++ -std=c++17 test/test_tinfoilhat_logic.cpp -o /tmp/thtest && /tmp/thtest
        # tests scoring, CSV round-trip, leaderboard ranking — must stay green


  ╔════════════════════════════════════════════════════════════════════════╗
  ║  0x07 .. THE LOOT  ( what's in the repo )                              ║
  ╚════════════════════════════════════════════════════════════════════════╝

     external/tinfoilhat/
        main.cpp ................. app registration -> tinfoilhat.ppma
        ui_tinfoilhat.hpp/.cpp ... menu, scan, results, grading, chart
        tinfoilhat_logic.hpp ..... pure scoring/CSV/leaderboard (host-tested)
        viewer.html .............. companion web results viewer (SD /TESTS/)
        external.cmake.patch ..... the 2 registration edits for manual builds
     test/test_tinfoilhat_logic.cpp .. host self-check (no toolchain needed)
     .github/workflows/build-ppma.yml .. CI: builds .ppma + publishes releases


  ╔════════════════════════════════════════════════════════════════════════╗
  ║  0x08 .. GREETZ                                                        ║
  ╚════════════════════════════════════════════════════════════════════════╝

     shoutout to the portapack-mayhem crew for the external-app loader.

              stay shielded. trust no signal. protect ya brainwaves.
 ______________________________________________________________________________
/______________________________________________________________________________\
```
