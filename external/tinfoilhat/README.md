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
  |     [ x ] release date .......... 2026-08-02        release .... FINAL    |
  |     [ x ] protection ............ tinfoil, 3+ layers, no gaps             |
  |     [ x ] supplier .............. error404                                |
  |                                                                           |
 _|                                                                           |_
/ \                                                                           / \
\_/___________________________________________________________________________\_/


  ╔════════════════════════════════════════════════════════════════════════╗
  ║  0x01 .. WHAT iS THiS THiNG                                            ║
  ╚════════════════════════════════════════════════════════════════════════╝

     Runs the whole Tinfoil Hat Contest RF-attenuation test right on the
     PortaPack. Sweeps 50 frequencies 2 MHz -> 5900 MHz twice (baseline, then
     hat-on), scores the shielding, draws the before/after chart on the LCD,
     and drops a CSV on the SD card. Review old runs, scroll the chart, and
     rank contestants on an on-device leaderboard. No laptop at the table.

        ATTENUATION (dB) = BASELINE - HAT      ...higher = better tinfoil.
        (negatives are REAL - foil resonance can amplify. it counts against you.)


  ╔════════════════════════════════════════════════════════════════════════╗
  ║  0x02 .. iNSTALL  ( the drag-n-drop part )                             ║
  ╚════════════════════════════════════════════════════════════════════════╝

     You need ONE file: tinfoilhat.ppma

        1.  Grab tinfoilhat.ppma  (from the GitHub Release, the CI build
            artifact, or build it yourself -> section 0x03)
        2.  Drop it into the  /APPS/  folder on your PortaPack SD card
        3.  Eject, boot, open the  Games  menu -> "Tinfoil Hat"

     That's it. No reflash. No toolchain. One file, one folder.
     .ppma is portable to any device on a compatible Mayhem nightly.


  ╔════════════════════════════════════════════════════════════════════════╗
  ║  0x03 .. BUiLDiNG THE .PPMA  ( only needed once )                      ║
  ╚════════════════════════════════════════════════════════════════════════╝

     [ the lazy way -- let a robot do it ]

        Push to GitHub. The .github/workflows/build-ppma.yml action grafts
        this app into a fresh mayhem-firmware checkout, runs the official
        Docker build, and spits out tinfoilhat.ppma as an artifact. Tag a
        commit  v1.0  and it attaches the .ppma to a Release automatically.

     [ the manual way ]

        git clone --recurse-submodules \
            https://github.com/portapack-mayhem/mayhem-firmware
        cp -r external/tinfoilhat \
            mayhem-firmware/firmware/application/external/
        # register it in TWO firmware files:
        #  external.cmake -> add the 2 sources to EXTCPPSRC + 'tinfoilhat' to
        #                    EXTAPPLIST (see external.cmake.patch)
        #  external.ld    -> add a MEMORY region  ram_external_app_tinfoilhat
        #                    (next free 0x..0000, len 32k) AND a matching
        #                    .external_app_tinfoilhat SECTIONS block, mirroring
        #                    the 'level' entries. (CI does all this for you; see
        #                    .github/workflows/build-ppma.yml for exact edits.)
        cd mayhem-firmware
        docker build -t portapack-dev -f dockerfile-nogit .
        docker run -i -v "$PWD:/havoc" portapack-dev
        # -> build/firmware/application/tinfoilhat.ppma


  ╔════════════════════════════════════════════════════════════════════════╗
  ║  0x04 .. USiNG iT                                                      ║
  ╚════════════════════════════════════════════════════════════════════════╝

     START TEST     pick category (Classic/Hybrid), remove hat, OK for
                    baseline sweep → place hat, OK for hat sweep.
                    auto-saves TESTS/TH_####.csv and shows the chart.
     LEADERBOARD    best run per contestant, Classic & Hybrid ranked
                    separately. select a row to open that run's chart.
     SETTINGS       RF gain (LNA/VGA/AMP), freq set (Full 50 / Fast 12).
                    persists to SD automatically.

     ...line-overlay, numeric table, per-run review/rename, and head-to-head
     compare live in the companion WEB VIEWER (0x05) — trimmed on-device to fit
     Mayhem's hard 32KB external-app limit.

     SCORE BANDS:  HF 2-30 . VHF 30-300 . UHF 300-3000 . SHF 3000-5900 (MHz)


  ╔════════════════════════════════════════════════════════════════════════╗
  ║  0x05 .. THE WEB ViEWER  ( big screen results, on the SD card )        ║
  ╚════════════════════════════════════════════════════════════════════════╝

     viewer.html is a self-contained companion page -- no internet, no server,
     no CDN. Drop it in the SD card's  /TESTS/  folder next to the CSVs.

        * open it straight off the card (double-click), OR browse to it from a
          computer connected to the device
        * "Open TESTS folder" (Chrome/Edge) slurps every TH_####.csv at once;
          "Add CSV files" works in any browser
        * sortable/filterable leaderboard table, multi-select rows
        * SVG charts: baseline-vs-hat, attenuation, or overlay N runs
        * export the chart as SVG or PNG, download any run's CSV, or Print
        * band breakdown (HF/VHF/UHF/SHF) per run

     ( want it written automatically? the app already saves the CSVs; just keep
       one copy of viewer.html in /TESTS/ and it reads whatever's there. )


  ╔════════════════════════════════════════════════════════════════════════╗
  ║  0x06 .. THE LOOT  ( file listing )                                    ║
  ╚════════════════════════════════════════════════════════════════════════╝

     external/tinfoilhat/
        main.cpp ................. app registration -> tinfoilhat.ppma
        ui_tinfoilhat.hpp/.cpp ... menu, scan, results, review, grading, chart
        tinfoilhat_logic.hpp ..... pure scoring/CSV/leaderboard (host-tested)
        viewer.html .............. companion web results viewer (SD /TESTS/)
        external.cmake.patch ..... the 2 registration edits
     test/test_tinfoilhat_logic.cpp .. host self-check (no toolchain needed):
        g++ -std=c++17 test/test_tinfoilhat_logic.cpp -o /tmp/t && /tmp/t


  ╔════════════════════════════════════════════════════════════════════════╗
  ║  0x07 .. CALiBRATiON NFO  ( read before you cry about the numbers )     ║
  ╚════════════════════════════════════════════════════════════════════════╝

     Power is read straight from the baseband (ChannelStatistics.max_db).
     Because the score is baseline MINUS hat, any fixed calibration offset
     cancels -- absolute dBm is cosmetic, the delta is what matters.

     Knobs in ui_tinfoilhat.hpp (ponytail: tune on real hardware):
        SETTLE_MSGS ...... stat windows skipped after each retune
        AVG_MSGS ......... stat windows averaged per frequency
     Higher freqs may need more settle time for synth lock. Not every one of
     the 50 freqs is guaranteed clean on H4M -- verify on-device, prune noisy.


  ╔════════════════════════════════════════════════════════════════════════╗
  ║  0x08 .. GREETZ                                                        ║
  ╚════════════════════════════════════════════════════════════════════════╝

     shoutout to the portapack-mayhem crew for the external-app loader, and
     to the contest sponsors:

              stay shielded. trust no signal. protect ya brainwaves.
  ______________________________________________________________________________
 /______________________________________________________________________________\
```
