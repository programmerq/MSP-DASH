# MSP-DASH timeline

Every commit in order. The number is the build number (build number = commit count). Generated from `git log`.


## Phase 1: WROOM board, Nextion and web dashboard

| Build | Date | Commit | Change |
|---|---|---|---|
| 1 | 2026-09-05 | 6ef39a2 | Initial commit: minimal README and gitignore |
| 2 | 2026-09-06 | 9223ceb | Add ESP32 CAN sniffer for rusEFI bench testing |
| 3 | 2026-09-06 | 3633b09 | Add CANBed sniffer sketch (confirmed CS pin 17, mcp_can library, per Longan docs) |
| 4 | 2026-09-06 | 87d2fa8 | Fix CANBed sniffer include: mcp_canbus.h not mcp_can.h |
| 5 | 2026-09-06 | c677747 | Add official rusEFI DBC and fully confirmed CAN map (RPM/TPS1 verified live; CLT/OilPress/Lambda pending longer capture) |
| 6 | 2026-09-06 | 32409ff | Bench-confirm RPM/MAP/TPS1/BattVolt/MCUTemp against real values; add esp32-main.ino with confirmed byte layout |
| 7 | 2026-09-06 | 559dad4 | Add Serial debug output to esp32-main for bench verification without Nextion attached |
| 8 | 2026-09-06 | a8a3a4a | Realign esp32-main to real Nextion HMI: 12-field digital dashboard (rpm/mph/tps/eth/ego/fuelp/oilp/oilt/map/clt/mat/vvt) |
| 9 | 2026-09-06 | c499712 | Fix Serial2 baud rate: 115200 to match Nextion project's baud= startup setting (was 9600) |
| 10 | 2026-09-06 | 5f0c11d | Add GT-U7 GPS on UART1: mph now sourced from GPS speed instead of rusEFI VSS |
| 11 | 2026-09-06 | 740306a | Add MegaSquirt support alongside rusEFI (no mode switch, ID ranges don't overlap); reorganize esp32-main.ino into clear sections |
| 12 | 2026-09-06 | 71d3b14 | Add EGT via MAX6675 + K-type thermocouple (console-only, bit-banged) |
| 13 | 2026-09-06 | 67b4111 | Add wifi_config.h.example template and gitignore real wifi_config.h |
| 14 | 2026-09-06 | bf97ce8 | Add live web dashboard (WiFi + WebSocket) showing all gauges including EGT/BattVolt not yet on Nextion |
| 15 | 2026-09-06 | 949e4b9 | Redesign web dashboard: dial gauges for RPM/Boost, progress bar for TPS, cleaner tile grid; add boost field broadcast |
| 16 | 2026-09-06 | 0e83b9b | Add /config page: persistent on/off toggles for each gauge, stored in NVS flash |
| 17 | 2026-09-06 | 20d2212 | Redesign web dashboard as classic OEM instrument cluster (SVG gauges, LCD trip computer); add FuelLevel signal |
| 18 | 2026-09-06 | eb335e8 | Convert all temps to Fahrenheit (fixes CLT gauge unit mismatch); add GPS fix status light |
| 19 | 2026-09-06 | 38afa5c | Major overhaul: per-gauge config (type/refresh/limits/redline), per-ECU config page, debug logging toggle (off by default), LinkECU support (shared parser with MegaSquirt), AEM X-Series scaffold (unconfirmed) |
| 20 | 2026-09-06 | 0f20530 | Expose full rusEFI DBC as configurable gauges (74 total); remove MegaSquirt/LinkECU support (rusEFI-only focus for now) |
| 21 | 2026-09-06 | 58f46bb | Add Light gauge type + MDI icons (unverified names), per-gauge console debug, and dashboard interactivity: close/X, drag-reorder, right-click type change |
| 22 | 2026-09-06 | 8fd5646 | Fix /config blank page: stream response instead of building one giant String (heap fragmentation with 74 gauges) |
| 23 | 2026-09-06 | 53638c0 | Add ESP32 system info panel (RAM/CPU/flash/temp) and SD card CSV logging |
| 24 | 2026-09-06 | 1e6984d | Add OTA firmware update via browser (/update page, ESP32 core Update library); clarify NVS persistence behavior |
| 25 | 2026-09-06 | 1be9e6b | Fix compile error: forward-declare broadcastStatus() before updateSystemInfo() uses it |
| 26 | 2026-09-06 | a11cb8e | Make /config Save button fixed/floating at top of viewport - always visible on the long 74-gauge page |
| 27 | 2026-09-06 | 29eb559 | Fix Save button visibility: replace fragile position:fixed with a simple second button at the top of the form |
| 28 | 2026-09-06 | b897b86 | Add appearance/theme system: configurable face/needle colors, gauge size scale, and fully configurable center LCD panel contents/colors/speed |

## Phase 2: round AMOLED bring-up (Arduino_GFX, LVGL 9 tests)

| Build | Date | Commit | Change |
|---|---|---|---|
| 29 | 2026-09-27 | f235660 | Add Waveshare ESP32-S3-Touch-AMOLED-1.43 display bring-up: CO5300/SH8601 test sketches, confirmed pinout/library docs |
| 30 | 2026-09-27 | 7b4d5da | Fix compile error in AMOLED test sketches: use raw RGB565 hex values instead of undefined named color constants |
| 31 | 2026-09-27 | ffc9e93 | CO5300 test: explicit 20MHz QSPI clock to test signal integrity theory (default is 40MHz) |
| 32 | 2026-09-27 | 21fcf96 | CO5300 test: try 10MHz - 20MHz cleaned fills but text still dithered, confirming signal integrity theory |
| 33 | 2026-09-27 | 11a2974 | CO5300 test: use class default 480x480 buffer size instead of 466x466 - testing persistent green-sliver theory (buffer size mismatch, not signal integrity) |
| 34 | 2026-09-27 | eab35e7 | CO5300 test: try centering 466x466 visible area with offset=7 within 480x480 buffer; add border+crosshair for precise alignment check |
| 35 | 2026-09-27 | 999df53 | Add digital clock test sketch (big text, seconds.milliseconds, boots straight to it) - drops color test pattern, uses partial-region clear for smooth updates at confirmed 10MHz QSPI speed |
| 36 | 2026-09-27 | 31c2773 | Fix clock test tearing: use opaque-background text (single write) instead of separate clear-rect + draw-text (two writes) |
| 37 | 2026-09-27 | 564606f | Add LinkECU CAN + numbers-only gauge display for ESP32-S3 AMOLED board; document confirmed display bring-up (CO5300, 10MHz, offset 7,7,7,7, tearing fix) and CAN pin choices |
| 38 | 2026-09-27 | d19a41e | Fix CAN pin choice: GPIO5/6 were 'free' per the chip table but not actually broken out on this board's headers. Confirmed from real board photo: use GPIO1/GPIO2 instead (physically accessible on the left-side header) |
| 39 | 2026-09-28 | 18948c8 | Re-add MegaSquirt/LinkECU support to WROOM (all 3 ECUs now supported); add UART bridge to forward decoded data to the ESP32-S3 round display instead of it reading CAN independently |
| 40 | 2026-09-28 | 23efb09 | Add SD-card image loading for the round AMOLED display: standalone test sketch + confirmed Arduino_GFX bitmap API + converted test image (RGB565 raw format) |
| 41 | 2026-09-28 | 5beba65 | Add moving needle test for round AMOLED display - automatic simulated sweep, proves rotation math and flicker-free redraw |
| 42 | 2026-09-28 | 2ff3e22 | Rewrite needle test with step-by-step setup logging and continuous heartbeat print - diagnoses exactly where it fails and solves the 'missed the boot-time prints' problem |
| 43 | 2026-09-28 | 53a25ff | Add minimal LVGL v9 bring-up test, adapted from LVGL's own official Arduino example, bridged to our confirmed Arduino_GFX/CO5300 display - first test of whether lv_conf.h needs manual setup |
| 44 | 2026-09-28 | 75cf8dc | Fix LVGL v9 color format: LV_COLOR_DEPTH doesn't exist in v9 (confirmed), use lv_display_set_color_format(RGB565) instead; document lv_conf.h setup and API findings |
| 45 | 2026-09-28 | d36ae15 | Fix white screen: LVGL's default theme uses a light background, making our white text invisible against it. Explicitly set screen background to black to match project's dark aesthetic |
| 46 | 2026-09-28 | 2b5f0cb | Add real LVGL gauge test: lv_scale + line needle, automatic simulated sweep, using confirmed API from installed source |
| 47 | 2026-09-28 | 8e8d7a4 | Test tearing fix: disable rounded line caps (simpler redraw region) and slow update rate to 80ms - isolate whether rendering complexity or update rate is the cause |
| 48 | 2026-09-28 | d9ac195 | Add combined LVGL gauge + direct CAN sketch: real RPM needle gauge on the round AMOLED, reading LinkECU CAN directly on this board |
| 49 | 2026-09-28 | 8200140 | Switch CAN pins from inaccessible 1.27mm GPIO header (GPIO1/GPIO2) to the UART connector (GPIO43/44) - matching JST-SH cable was available; ESP32-S3's flexible GPIO matrix makes this a valid substitution |
| 50 | 2026-09-28 | 6e6c060 | Switch to boost gauge on rusEFI CAN (was RPM/LinkECU); on-screen shows kPa, console shows kPa/bar/inHg/psi for multi-unit visibility and CAN-flow confirmation |
| 51 | 2026-09-29 | 58086e2 | Clean up repo: remove all test/bench sketches (AMOLED bring-up tests, CAN sniffers), keep only the two active firmwares (esp32-main, esp32s3-amoled-lvgl-can-gauge); update README to reflect current structure |
| 52 | 2026-09-29 | ec3748c | Add multi-page gauge cluster: AFR needle+readout, CLT/OilP half-moons, 6-value grid page - cycled via BOOT button, expanded rusEFI CAN parsing |
| 53 | 2026-09-29 | eec923d | Add PREREQUISITES.md: new-machine setup script (winget check, arduino-cli, ESP32 core, libraries, lv_conf.h), OneDrive git quirk fix, board-specific compile flags |
| 54 | 2026-09-29 | 890dc48 | Add standalone touch controller test (bb_captouch library) - validates hardware/library and basic swipe detection before integrating into the gauge cluster |
| 55 | 2026-09-29 | ab39461 | Rewrite touch test with step-by-step setup logging and continuous heartbeat - same fix that resolved the identical 'nothing on Serial' symptom seen with the needle test |
| 56 | 2026-09-30 | f30b49f | Add serial-echo diagnostic: type on terminal, see it on screen - tests Serial RX direction + display, isolates whether the persistent 'nothing on Serial' issue is TX-specific or something more fundamental |

## Phase 3: rebuild on the Waveshare BSP (ESP-IDF + LVGL 8)

| Build | Date | Commit | Change |
|---|---|---|---|
| 57 | 2026-09-30 | 3848b33 | Add confirmed-working Waveshare BSP example as new foundation (ESP-IDF esp_lcd + LVGL v8 + FT3168 touch) - display and touch both confirmed working end-to-end through our own toolchain |
| 58 | 2026-09-30 | 015bdab | Full pivot: rebuild multi-page gauge cluster on Waveshare's confirmed-working ESP-IDF/LVGL v8 BSP foundation, replacing our Arduino_GFX/LVGL v9 stack that had persistent unresolved issues. Uses lv_meter widget, safe cross-task value sharing via lv_timer, rusEFI CAN reading in Arduino loop() |
| 59 | 2026-09-30 | d2b03eb | Add touch swipe left/right page switching using LVGL v8's gesture detection, alongside existing BOOT button |
| 60 | 2026-09-30 | eff443f | Switch CAN pins to GPIO43/44 (UART connector, the only cable currently on hand) - must compile with CDCOnBoot=cdc to avoid conflicting with Serial's default UART0 routing on those same pins |
| 61 | 2026-09-30 | 26c0f96 | Full redesign: 6 full-screen analog gauges (AFR, CLT, Oil Press, Oil Temp, Fuel Press, Ethanol%) matching classic automotive gauge look - black bg, white needle/ticks, auto-cycle every 5s, sits at minimum until real CAN data arrives |
| 62 | 2026-09-30 | ec6ddf5 | Fix compile error: add missing stdbool.h include to gauge_app.h (bool type used in header but only included in the .c file) |
| 63 | 2026-09-30 | 45a34d9 | Thicker needle, wider tick-number gap, new numbers-grid page, new boost/vacuum gauge (rests at 0, kPa scale), removed auto-cycle per request |
| 64 | 2026-09-30 | c53e541 | Enable 28px Montserrat font and apply to LV_PART_TICKS for bigger, brighter tick numbers matching reference image |
| 65 | 2026-09-30 | 28d8930 | Reorder pages (Boost after AFR), change Boost to raw MAP 0-200kPa with 100=ambient, enlarge numbers-grid value text to 28px |
| 66 | 2026-09-30 | 79e2723 | Reorder/rename pages (CLT,MAP,AFR,OIL PRESS,OIL TEMP,FUEL PRESS,ETH%), update CLT/OilTemp ranges to 250F, enlarge caption text to 28px, add flashing-red warning system for threshold values |
| 67 | 2026-09-30 | 3f90ab8 | Replace static rest-value with a back-and-forth sweep animation while waiting for real CAN data on each signal |
| 68 | 2026-09-30 | c50ef77 | Add Stage 1 of wireless config plan: WiFi AP + web server with a live read-only gauge monitoring page (WebSocket broadcast every 500ms) |
| 69 | 2026-09-30 | 0ff4093 | Fix boot crash (buf2 DMA allocation assert) caused by WiFi/LVGL competing for limited DMA-capable memory - shrink LVGL buffer height from /4 to /10 |
| 70 | 2026-09-30 | 673fdf1 | Switch web monitor from AP mode to STA mode (joins home WiFi), same git-ignored wifi_config.h credential pattern as the WROOM board, prints connection status and IP to Serial |
| 71 | 2026-09-30 | 3e23ca6 | Add Stage 2 of wireless config plan: raw CAN broadcast monitor web page (/can), showing every CAN ID seen with raw bytes and frequency |
| 72 | 2026-09-30 | c0ccae4 | Add Stage 3 of wireless config plan: port MegaSquirt/LinkECU parser from WROOM board, add persisted ECU selection (NVS) with web UI dropdown |
| 73 | 2026-09-30 | 863e1f5 | Stage 4: full gauge configuration - data-driven pages, persisted signal/range/color/caption/warning config per slot, web UI editor. Added RPM/TPS/BattV decoding to expand signal pool to 10. Fixed stale hardcoded page-count bug in BOOT button handler |
| 74 | 2026-09-30 | 94c78d1 | Add tick/background color customization per gauge slot, bump tick-label font to 36px, fix warning flash to respect custom background color instead of assuming black |
| 75 | 2026-09-30 | 70d0a70 | Add SD card support: mount (ported from Waveshare's confirmed example), web-based browse/upload/download via ESP-IDF native VFS/FAT + POSIX file I/O |
| 76 | 2026-09-30 | 70fc4d8 | Fix compile error: add missing Arduino.h include to sd_storage.cpp (Serial not auto-included in plain .cpp files, only .ino) |
| 77 | 2026-09-30 | 79d7daa | Document required PartitionScheme=app3M_fat9M_16MB in PREREQUISITES.md - board's default scheme was sized for 4MB flash, not this board's actual 16MB, causing a 103% overflow once the sketch grew large enough |
| 78 | 2026-09-30 | 40232cb | Add SD logging (always-on CSV), gauge page reordering (web UI up/down), and per-slot value divisor + tick density (enables RPM x1000 display with clean 1-9 ticks) |
| 79 | 2026-09-30 | 216321f | Add Waveshare's confirmed QMI8658 IMU driver + I2C bus layer files, for the G-force gauge integration |
| 80 | 2026-09-30 | ce5939a | Merge remote changes |
| 81 | 2026-09-30 | 9971a0e | Merge origin/main into local work |
| 82 | 2026-10-01 | fb3dd11 | Remove i2c_bsp.cpp - duplicates functions already defined in FT3168.cpp (same I2C bus, shared between touch and IMU per the board's pinout) and would cause both a duplicate-driver-install crash and a linker 'multiple definition' error |
| 83 | 2026-10-01 | 5bdd3e0 | Fix CoolantTemp/OilTemp signed-vs-unsigned bug, confirmed against actual rusEFI DBC file - was wrapping negative above ~87C/189F, a realistic normal-operating-temperature range |
| 84 | 2026-10-01 | e470d79 | Complete IMU/G-meter integration: QMI8658 reading (resolved I2C bus conflict with touch), custom cross+trailing-dot widget with accel/brake color coding, wired into the shared page cycle |
| 85 | 2026-10-01 | e44ebdb | Disable touch swipe navigation (BOOT button only now) via a single master toggle - kept Touch_Init() running since the IMU shares its I2C bus for the G-meter page |
| 86 | 2026-10-01 | ed15924 | Remove G-meter (didn't work vertically-mounted), add per-gauge refresh rate (20ms master tick, self-throttling per slot), value-above-caption display toggle with font size, bulk color apply to all gauges |
| 87 | 2026-10-01 | cb58201 | Reposition gauge caption to 3/4 down from top, centered horizontally - better fit for the circular gauge shape |
| 88 | 2026-10-01 | c6fbf1f | Revert caption position (was meant for FPS monitor, not caption) - FPS monitor alignment is a separate lv_conf.h setting, not something set in our own code |
| 89 | 2026-10-01 | 4dd1ef6 | Enable OTA firmware updates: new /update web page + POST endpoint using Arduino's built-in Update library, confirmed our partition scheme genuinely has dual OTA app slots |

## Phase 4: pages and cells, editors, logging, alerts

| Build | Date | Commit | Change |
|---|---|---|---|
| 90 | 2026-10-01 | ef72e93 | Stage A of page/cell redesign: new nested data model (pages/layouts/cells) replacing the old flat one-gauge-per-page model. NOT yet integrated - gauge_app.c and web_monitor.cpp still need updating (Stage B/C) |
| 91 | 2026-10-01 | 31f2f51 | Stage B of page/cell redesign: rewrite gauge_app.c to build multi-cell pages (1/2/3/4 cells per fixed layout, 3 gauge types per cell). Added signal-activity tracking calls. web_monitor.cpp (Stage C) still needs rewriting before this compiles |
| 92 | 2026-10-01 | 8a6e71a | Stage C of page/cell redesign: full web UI rewrite (layout picker, per-cell editor, activity-sorted sensor picker, page management, hamburger menu, remote page nav). Fixed sd_logging.cpp which was missed in earlier stages. Redesign complete |
| 93 | 2026-10-01 | 3d1a2b0 | Fix silent save failure: POST body buffer was too small (8192) for fully-populated multi-cell page configs, causing JSON parse failures and silently-dropped saves. Increased to 24576, added error logging |
| 94 | 2026-10-01 | c1c49c1 | Add fully independent web-dashboard page/cell config (separate from physical display config) with its own /web-config editor. Rewrote main page: phone-frame styling (iPhone 15 Pro Max viewport, auto-detects desktop vs phone), single-page dynamic rendering, generalized WebSocket broadcast |
| 95 | 2026-10-01 | 31d338a | Project review: fix stale comment, remove 9 dead functions (unused page-management + ecu_config_name), fix nav wording inconsistency, reorganize main .ino into can_parsing.h + page_button.h (269 -> 96 lines) |
| 96 | 2026-10-02 | 806fb5c | Stage 1 of ESP-NOW/multi-screen plan: new histogram (strip-chart) gauge type, RPM-gated logging (>800), independent Log Channels config+page, blinking on-screen logging indicator via lv_layer_sys() |
| 97 | 2026-10-02 | bee6d1f | Fix /gauges, /web-config, /log-channels silently hanging on load - concurrent fetches with no error handling occasionally stalled under this board's CAN+LVGL+webserver load. Sequential fetches + try/catch + visible retry |
| 98 | 2026-10-06 | 2a35ed7 | Move CAN receive to its own priority-3 FreeRTOS task, raise TWAI rx queue 5 -> 64, add missed-frame warning + bus-off recovery, lock the /can frame table |
| 99 | 2026-10-07 | 1ef006d | Move POST body buffers from static internal RAM to PSRAM (51KB), add 10s heap readout, add on-page script banner/error display, refuse to save an unloaded (empty) config, check save response |
| 100 | 2026-10-07 | c0c62ca | Lost-signal handling (2s timeout, -- readouts, empty log fields), per-cell full-screen alert overlay, BOOT button no longer desyncs from the shown page, host tests for gauge_app.c and gauge_config.cpp |
| 101 | 2026-10-07 | 8b0e0a1 | Trend graph rework: real history window (own sample rate), scaled values, reference line, fits the round panel, caption under graph; Trend Graph now selectable in the editors; host tests + screenshot tool |
| 102 | 2026-10-07 | e66968f | Show build number (web page, /update, Serial); front page status line + auto-reconnect + silence watchdog; /status.json; live CAN health in the broadcast; host type-check kit |
| 103 | 2026-10-07 | 162784f | Fix settings never saving: config exceeded NVS's 4000-byte string limit. Store as JSON files on the ffat partition, report real save results, share JSON conversion, sanitize input; build 103 |
| 104 | 2026-10-07 | e1410cc | Bench simulator (demo + manual, off at boot, 10 min timeout, stops on real CAN, pauses logging, SIMULATED banner); NO DATA on trend graphs; fix logging dot that was off the round panel; end-to-end tests; build 104 |
| 105 | 2026-10-07 | 75dc9f9 | Whole site in the iPhone frame: shared web_theme.h (bezel on desktop only, plain column on phones, safe-area, 16px fields, shared header+menu on every page); fix SD page overflow and editor checkbox/gap; browser layout tests; build 105 |
| 106 | 2026-10-07 | 26318d6 | Tidy the apply-colours box on both editors (equal columns, labels above swatches, full-width buttons); layout rule for it; build 106 |
| 107 | 2026-10-07 | d5c0e08 | Text sizes up to 120px with shrink-to-fit on the round glass; front page finally honours Value Text Size; separate dial-number size; dial start/end angles with live preview; Save & Reboot Board; tests; build 107 |
| 108 | 2026-10-07 | e0a5092 | Yellow and red tick-mark zones (high or low end of the scale) and needle length trim (gap at the centre, stop short of the tip); editor controls with live dial preview; pixel-level tests; build 108 |
| 109 | 2026-10-07 | f22b626 | ECU on/off flags (fan, check engine, rev limiter ...) decoded per the official DBC; indicators pages (up to 12 lamps, thresholds with hysteresis or ECU flags, flash, colours) on the round display and web dashboard; shared editor with presets; end-to-end tests; build 109 |
| 110 | 2026-10-07 | bf1375b | TunerStudio-style warning zones: four thresholds (low/high critical and warning) drawn as coloured bands behind the ticks, coloured ticks, or both; converts build 108 zone settings; editor rows with band preview; tests; build 110 |
| 111 | 2026-10-08 | e927261 | Caption text size (14/20/28/36/48) and units text; caption on the dial face; gauge position Center/Top/Bottom; default scales AFR 10-20, temperature 60-250, pressure 0-100; new 20 and 48 px text fonts; dashboard honors them; tests; build 111 |
| 112 | 2026-10-08 | 4645c6c | Decimals setting (0/1/2) for the readout: round display, alert value, web dashboard; both editors; old configs load as 1; readout size picked for the widest text with its decimals; dashboard warning compares the real reading; tests; build 112 |
| 113 | 2026-10-11 | 67bb431 | Repo cleanup: remove 5 stray pixel-test dumps (*.rgb) from the root and ignore *.rgb from now on; build 113 (no code change) |
| 114 | 2026-10-11 | b113259 | Repo cleanup: remove the early bring-up sketches (lvgl-can-gauge, serial-echo, touch-test); README and PREREQUISITES point at esp32s3-amoled-bsp-base; build 114 (no code change) |
| 115 | 2026-10-11 | 29bfff8 | PREREQUISITES: the exact compile/upload commands in use (CDCOnBoot, app3M_fat9M_16MB, shared build folder); build 115 (no code change) |
| 116 | 2026-10-11 | 7c368e6 | PREREQUISITES: flash over WiFi (curl to /update at 192.168.55.102) as the normal path, USB as fallback; build 116 (no code change) |

## Phase 5: multi-board ESP-NOW link, board settings, input board

| Build | Date | Commit | Change |
|---|---|---|---|
| 117 | 2026-10-16 | 26102c3 | ESP-NOW link phase 1: one firmware, main/remote role (/role?set=), main broadcasts all signals at 50/s, remote shows them (CAN off, 1 s timeout to --), channel 1 when no WiFi; link status in /status.json and front page; build 117 |
| 118 | 2026-10-16 | 9be624b | Static IP per board: /net?ip=&gw=&mask=(&dns=) or /net?dhcp=1, saved in NVS; serial 'dhcp' recovery; status shows ip; build 118 |
| 119 | 2026-10-17 | a434558 | USB serial commands: info, ip <ip> <gw> <mask> [dns], dhcp, role main/remote; build 119 |
| 120 | 2026-10-17 | 4a8626b | ESP-NOW: main sends from its own task (core 0, 20 ms) so screen redraws can't slow it (was 12/s); build 120 |
| 121 | 2026-10-17 | 5fc0f52 | Main board lists the remotes it hears (ESP-NOW hello every 2 s: IP + build), with links to each remote's gauge editor and a build-mismatch note; build 121 |
| 122 | 2026-10-17 | 2aef4cc | Board names in NVS: /name?set= or serial 'name <text>'; shown on each board's page and in the main board's remote list; build 122 |
| 123 | 2026-10-17 | ce1b336 | PREREQUISITES: 3-board WiFi flash loop (.231/.232/.233, Expect: fix), serial and web setup commands; build 123 (no code change) |
| 124 | 2026-10-17 | 292265c | EGT (placeholder CAN 0x7F0) and IAT (rusEFI BASE3 byte 3) signals, appended after the flags; half moons grow to fill their cell within the glass; 2-Up makes unset cells Half-Moons; tests updated; build 124 |
| 125 | 2026-10-17 | c416027 | Display performance: fps and LVGL busy % (status, front page, /perf overlay, serial 'perf on/off'); ESP-NOW send 20/s; build 125 |
| 126 | 2026-10-17 | 2d9e635 | Text alignment per gauge (Center/Left/Right) for caption, readout and units, kept inside the cell and the glass; build 126 |
| 127 | 2026-10-17 | e65dd72 | Caption alignment only (readout and units always centred); dial readout re-centres as the number's width changes; build 127 |
| 128 | 2026-10-17 | c666029 | Screen diameter per board (/diameter?set= or serial 'diameter N', 300-560, default 466): gauge layout and glass checks scale to it; build 128 |
| 129 | 2026-10-18 | 076f2e7 | Default screen diameter for boards with no saved setting: 520; build 129 |
| 130 | 2026-10-18 | e9c59d0 | Needle thickness per gauge; caption and readout placed by X/Y coordinates (units follow); Apply to ALL also sets needle gap, tip trim and thickness; build 130 |
| 131 | 2026-10-18 | f7d9c42 | Apply to ALL: separate buttons for colors and for the needle; build 131 |
| 132 | 2026-10-18 | 84dcdc0 | Apply to ALL: caption text size with its own button; build 132 |
| 133 | 2026-10-18 | 217af81 | Apply to ALL caption size: options in the page markup (fixes host editor tests); build 133 |
| 134 | 2026-10-18 | b3bf58e | 2-Up half moons come out the same size and position; remove the grey cell outline (the horizontal line); build 134 |
| 135 | 2026-10-18 | d8aa53d | 2-Up half moons: match the partner using its own dial (bowl above dome stays full size); text by coordinates lets half moons use the whole cell; build 135 |
| 136 | 2026-10-18 | b02d11d | Move dial X/Y per needle or half-moon gauge (nudge from its automatic spot); build 136 |
| 137 | 2026-10-18 | 7021ec2 | Boot: quick WiFi scan, skip joining when the home network isn't there (ESP-NOW starts at once in the car); startup needle sweep on all pages; demo sweep on/off; both saved per board; build 137 |
| 138 | 2026-10-18 | 5e6b435 | Board Settings page (/settings): name, startup sweep, demo, FPS overlay, diameter, static IP/DHCP, role; build 138 |
| 139 | 2026-10-18 | 98116f7 | Peak hold per needle/half-moon gauge: shadow needle holds the highest reading for 1-10 s (default 3), on/off; build 139 |
| 140 | 2026-10-18 | 71facd3 | Input board sketch (esp32-input: 2 buttons + brightness knob over ESP-NOW, IP .204, WiFi updates); screens accept page and brightness commands; build 140 |
| 141 | 2026-10-18 | 230000a | esp32-input: Btn type in its own header (the Arduino builder put the pressed() prototype above the struct); no default argument; input build 2, screens build 141 (no screen change) |
| 142 | 2026-10-18 | 996f448 | Input board: compile/upload with FlashFreq=40,FlashMode=dio (80 MHz boot-looped on the ESP32-D); docs only; build 142 |
| 143 | 2026-10-18 | 5a3806b | Input board build 3: brightness from the car's lights-on and dimmer voltages (day %, night min..max % across the dimmer), settings page with live volts and calibration; screens build 143 (no change) |

## Phase 6: the input board becomes the CAN hub

| Build | Date | Commit | Change |
|---|---|---|---|
| 144 | 2026-10-18 | 8691820 | Hub: the input board reads CAN (GPIO 21/22) with the screens' decoder (copied in at compile time) and sends the data packet at 20/s; screens build 144 (no change) |
| 145 | 2026-10-18 | 25ac852 | Hub: rename local F() helper, it clashed with Arduino's F() macro |
| 146 | 2026-10-18 | 1e6035e | Hub build 5: raw CAN page /can (every ID, bytes, count, age, bus state) and /can.json |
| 147 | 2026-10-18 | 68b8a0a | Hub build 5: include the CAN tracker before the web pages use it |
| 148 | 2026-10-18 | (this commit) | Docs: CODE_GUIDE.md and TIMELINE.md |
| 149 | 2026-10-18 | (this commit) | Removed the old WROOM + Nextion firmware (esp32-main); still in git history |
| 150 | 2026-10-18 | (this commit) | Shared link_packets.h for screens and hub, 32-signal compile check, PREREQUISITES cleanup |
| 151 | 2026-10-18 | (this commit) | Hub build 7: MAX6675 EGT on GPIO18/19/23, sent to CAN as 0x7F0 for rusEFI and to the screens |
| 152 | 2026-10-18 | (this commit) | New signals Ambient Temp + Humidity; hub build 8 reads a DHT22 on GPIO4 |
| 153 | 2026-10-18 | (this commit) | Hub build 9: fix compile (CAN decoder included before the EGT code), DHT22 start signal releases the line at once |
| 154 | 2026-10-18 | (this commit) | New signal Fuel Level (%); hub build 10 reads a pot on GPIO36 as a mock fuel sender |
| 155 | 2026-10-19 | (this commit) | Hub build 11: EGT frame sent only while the ECU is heard (avoids bus-off with the ECU off) |
| 156 | 2026-10-19 | (this commit) | Hub build 12: Screens status on the hub page, green online / red offline per screen, from the screens' hello; hello packet moved to link_packets.h |
| 157 | 2026-10-19 | (this commit) | Hub build 13: Sensors page (/sensors) lists every signal with value, age and source: hub sensor or ECU CAN frame |
| 159 | 2026-10-19 | (this commit) | Front page lists every sensor on the screen's own pages, live, one line each (replaces the separate web dashboard cards); up/down arrows to reorder pages in the gauge editor |
| 160 | 2026-10-19 | (this commit) | Gauge templates (scale + warning per signal when picked); board name next to MSP-DASH and in the tab title on every page; removed Web Dashboard Config and Raw CAN Monitor from the screens |
| 161 | 2026-10-19 | (this commit) | Pressures in psi; MAP shown as boost psi (0 = atmosphere, negative = vacuum). Templates, presets, simulator and factory pages updated. Hub build 14 (sends psi) - flash hub and screens together |
| 162 | 2026-10-20 | (this commit) | Bench simulator moved to the hub (hub build 15, /simulator); hub packets flag simulated data (has_mask bit 0), screens show SIMULATED and skip SD logging; simulator page removed from the screens |
| 163 | 2026-10-20 | (this commit) | Logging start conditions (key on / running / moving / sensor level) with stop delay, new file per session, Logging page; Speed signal (rusEFI BASE1, mph); fixed log channel ticks changing the next signal. Hub build 16 |
| 164 | 2026-10-20 | (this commit) | Signal lists alphabetical everywhere (gauge, lamp, logging); hub build 17: Sensors page alphabetical + 'On the bus, not decoded' list of CAN frames no sensor reads |
| 165 | 2026-10-20 | (this commit) | Hub build 18: hall speed sensor (GPIO 27, calibrate from GPS) -> Speed and CAN 0x7F0 bytes 3-4 to rusEFI; GT-U7 GPS (GPIO 16/17) -> GPS Speed, GPS Satellites; rusEFI BASE1 speed no longer read |
| 168 | 2026-10-20 | (this commit) | Hall speed sensor moved to rusEFI: Speed read from BASE1 byte 6 again; hub build 19 drops the hall code and the speed bytes in 0x7F0, keeps the GPS and compares it with rusEFI speed |
| 169 | 2026-10-21 | (this commit) | Hub build 20: web pages moved from the .ino to hub_pages.h (the Arduino builder mangled the raw strings; hub 18 and 19 never compiled) |
| 170 | 2026-10-21 | (this commit) | ECU type setting removed from the screens (it was only a label; both decoders always run). Hub build 21 shows the ECU it hears on its home page |
| 171 | 2026-10-21 | (this commit) | Configurable sensors stage 1: 16 custom slots (SIG_C1..C16), hub /custom page (CAN field decoder with live preview), MSN1 names packet to the screens (kept in NVS), has_mask 64 bit; fixed the hub dropping signals past 32. Hub build 22 |
| hub 22 fix | 2026-10-23 | (this commit) | custom_slot_info written over several lines with extern "C": the Arduino prototype scan read the one-liner as static. Test guards against one-line functions with 'static' inside |
| hub 23 | 2026-10-24 | (this commit) | CC1101 433 MHz listen mode on the hub (/rf): bursts gated by RSSI, pulses, half-bit time, Manchester bits and bytes, for identifying the tire sensors |
| hub 24 | 2026-10-24 | (this commit) | 433 MHz: no SPI traffic while recording a burst, ISR ignores edges during/just after SPI reads (the 1 ms RSSI poll was showing up as pulses) |
| hub 25 | 2026-10-24 | (this commit) | 433 MHz: automatic threshold (background median + margin), 2-minute signal history chart, hide-noise filter |
| hub 26 | 2026-10-24 | (this commit) | 433 MHz: the 5 strongest bursts kept separately (noise cannot push them out), All pulses button (/rf/burst), Clear |
| hub 27 | 2026-10-24 | (this commit) | 433 MHz: pair-based Manchester decoder (stretch correction, glitch merge) - first U912 packet decoded: 44 1D C5 A2 A2 B5 3B 51 0E CD, sent twice |
| 179 | 2026-10-24 | (this commit) | Hub buttons (hub 28): hold Next = choose screen (pop-up on it), hold Prev = clear warning on all screens, both = SD log marker (new marker column); short presses on release, to the chosen screen |
| 180 | 2026-10-25 | (this commit) | 10 colour presets (per cell and Apply to All), contrast-checked; needle glow option (two dimmer, wider lines under the needle) |
| 181 | 2026-10-25 | (this commit) | Configurable buttons (hub 29, /buttons): left/right/both x press,1,2,3,5,10 s; actions next/prev page, next gauge, go to page, dim/brighten all, colour scheme (pick or cycle, recoloured live and saved), race mode, clear warning, log marker; 'Release: ...' hint while held |
| 182 | 2026-10-25 | (this commit) | Page names: name box on every page in the editor (gauge and indicator pages), shown on the front page and as a pop-up when a page is switched to |
| 183 | 2026-10-25 | (this commit) | Caption icons, first icon: Check engine (MDI 'engine') as LVGL fonts 14-48 px; icon picker with preview and position (left / above / instead of caption); icon flashes red/white with a warning; tools/icons generator |
| 184 | 2026-10-25 | (this commit) | 79 vehicle icons (MDI); icons on indicator lamps (presets come with icons); gauge editor screen preview with caption, icon, sample value and units, colours and needle |
| 185 | 2026-10-26 | (this commit) | Hub 30: reorganised home page (cards, screens with links to their pages), nav bar on every hub page; button action Show IP addresses (5 s pop-up on each screen); runtime serial output off on screens and hub (RT_LOG, SERIAL_STREAM) |
| 186 | 2026-10-26 | (this commit) | GPS date/time: hub 31 sends it (MST1, with time zone + US DST set on the hub's Clock card); SD logs named by local date/time and a time column in every row |
| 187 | 2026-10-26 | (this commit) | Clock as a gauge source (signal 44, GPS time): number gauge in 12-hour with AM/PM, 24-hour, or with seconds; digit fonts gain ':'; preview and front page show it. Hub 32 (simulator list skips it) |
| (hub 33) | 2026-11-07 | (this commit) | OLED warning panel on the hub: 0.96" SSD1306 (I2C GPIO 5/15), 8 configurable lamps with the dash icons, seatbelt input GPIO 2, clock/ALL OK when nothing lit, /oled page with live picture; hub compile check script |
| (hub 35) | 2026-11-14 | (this commit) | OLED panel: WARNING and DANGER levels per lamp (danger = inverted flashing box), lamp inputs on GPIO 2/12/0 and two PCF8574 expanders (16 more inputs), lit when LOW or HIGH; /oled page reworked. Saved OLED settings reset to the new defaults once. |
| 193 | 2026-11-14 | (this commit) | Accelerometer: MPU6050 on the hub's I2C bus (hub 36), mounting axes + zero on the hub home page; new signals Lateral G, Accel/Brake G, Vertical G, Yaw Rate (45-48) for gauges, lamps and logging |
| 194 | 2026-11-15 | (this commit) | OLED pages (hub 37): warning lamps, big number, values, G-meter, clock; OLED next page button action, timer, lamp takeover; set up on /oled |
| (hub 38) | 2026-11-15 | (this commit) | All hub GPIOs in esp32-input/hub_pins.h; docs/HUB_PINS.md rewritten as one table; test_hub_pins.js keeps them in step (no behaviour change) |
| (hub 39) | 2026-11-15 | (this commit) | ADS1115 4-input ADC detected on the I2C bus (0x48-0x4B), I2C bus scan at boot, I2C card on the hub home page with every device found and A0-A3 volts |
| (hub 40) | 2026-11-15 | (this commit) | Web UI step 1: side menu grouped by task on every hub page (drawer on phones), shared look, /screen (screen pages inside the hub site with a Left/Center/Right picker), /firmware (all boards' builds), /can moved to hub_pages.h, all pages sent from flash |
| (hub 41) | 2026-11-15 | (this commit) | Web UI step 2: Dashboard (status tiles, warning panel, readings, screens); the old home page becomes /setup |
| (hub 42) | 2026-11-15 | (this commit) | Web UI step 3: Setup in task order with jump links; Firmware page updates the hub (progress bar) and every ticked screen from one .bin |
| 202 | 2026-11-15 | (this commit) | Screens send their uptime in the hello (hub 43 shows it); Firmware page treats a dropped connection as normal and confirms the update by the screen restarting |
| (hub 44) | 2026-11-15 | (this commit) | MCP23017 16-input expanders replace the PCF8574 (board 1 at 0x27, board 2 at 0x26): lamp inputs MCP23017 #1/#2 PA0-PB7, I2C card names them, pin doc updated |
| 204 | 2026-11-15 | (this commit) | Web UI step 4: gauge editor with a clickable layout picture, folding groups, pinned preview, copy cell/page to another screen; screens allow cross-origin requests; hub 45 Dashboard Logging tile shows the center screen's status |
| (hub 46) | 2026-11-15 | (this commit) | Fix: analog reads from two tasks no longer collide ("adc oneshot read fail", a reading of 0 for a moment): one ADC lock |
| (hub 47) | 2026-11-15 | (this commit) | ADS1115 support removed (not used); the I2C bus scan and card stay |
| (hub 48) | 2026-11-15 | (this commit) | Inputs and pins page: every pin and what uses it, move any function to another GPIO (restart), up to 16 inputs on free GPIOs / MCP23017 pins as buttons (NO/NC, press + hold actions), switches or analog sensors (presets or custom) into custom slots; OLED lamps can use any GPIO |
| (hub 49) | 2026-11-15 | (this commit) | Inputs page: NO/NC renamed to Active when LOW (switch to ground, optocoupler) / Active when HIGH (12 V through a divider), with the 12 V wiring note |
| (hub 50) | 2026-11-15 | (this commit) | Inputs page: every free pin (GPIO and MCP23017) has 'Set up an input' (opens the editor on that pin), pins with an input have 'Edit input', lamp pins link to /oled; Cancel fixed |
| 211 | 2026-11-15 | (this commit) | Hub 51: outputs (signal level with hysteresis / input / toggle; GPIO or MCP23017; HIGH or LOW = on; blink), page buttons can move onto expander pins, Toggle output button action (screens: its name in the pop-up) |
| (hub 52) | 2026-11-15 | (this commit) | ADS1115 back on the bus: up to 4 boards, channels as analog pins on the Inputs and pins page (analog sensors, and the fuel / knob / lights / dimmer functions can move onto them) |
| 215 | 2026-11-18 | (this commit) | Hub 53: Buttons page rebuilt. Up to 24 buttons, single (any GPIO / MCP pin, active LOW or HIGH) or on a resistor ladder (one analog pin, each voltage learned); one press action each, page actions pick Left / Center / Right / All / Chosen screen; no holds or combos. ECU map (next) action: named maps on a CAN frame (default 0x7F1), remembered, name pop-up on every screen (screens: LINK_CMD_POPUP). Page buttons are no longer fixed functions (old pin map and bindings carried over once). Every pin picker shows used pins greyed out with their user; the hub refuses a taken pin (OLED lamps too) |
| (hub 54) | 2026-11-18 | (this commit) | Hub static RAM was 536 bytes over (link error): the CC1101 burst lists (last 10, strongest 5, and the page copy) now live on the heap, freeing well over 10 KB of static RAM |
| 217 | 2026-11-18 | (this commit) | Hub 55: one design for the whole site. Screen pages drop the iPhone frame: the hub's look (side menu, menu button on narrow screens, 1000 px column, dark fields), and no menu of their own inside the hub's Screens page. Hub pages share one base style (width, font, iPhone-safe viewport, 16 px fields). The Dashboard is the phone page: Add to Home Screen, Quick controls (previous / next page on a chosen screen, ECU map, race mode, log marker, clear warning, show IPs, OLED page) through the new /action route |
| 218 | 2026-11-19 | (this commit) | Hub 56: staged shift light. A ring round each screen lights yellow left to right from the start RPM, all green at the shift point, red flashing past over-rev (Setup page: on, start, shift, over-rev, 4 s test sweep). Hub computes the level with 100 RPM hysteresis (shift_light.h) and sends LINK_CMD_SHIFT with the screen order; each screen finds itself in the list (link_packets.h) and goes dark after 1 s of silence |
| (hub 57) | 2026-11-19 | (this commit) | Hub's own Wi-Fi network for the car (hub_wifi.h): on when the home network is not found (default), always, or off; name and password set on the Setup page (Save, or Save and restart). It runs on the ESP-NOW channel, so the screens keep their data. Default MSP-DASH / mspdash123, the page warns until changed; the screens pop up the name and the hub's address after start |
| 220 | 2026-11-20 | (this commit) | Hub 58: Board status page (/boards, Overview menu). The hub: main loop rate, CAN frames in, CAN bus state, missed frames and errors, sends to the screens, its Wi-Fi clients. Every board: CPU busy per core (idle-hook estimate, cpu_load.h), free / lowest / largest-block RAM, chip temperature, Wi-Fi signal, clock, uptime. Screens add frames per second, time drawing, data from the hub and its age, PSRAM, SD logging; they send these in their hello (HelloStats), so it works in the car. Green / amber / red per value |
| 221 | 2026-11-20 | a7a26e1 | Hub 59: Speeduino and factory ECUs over OBD-II (removed again in 222) |
| 222 | 2026-11-20 | (this commit) | Hub 60: Speeduino / OBD-II support removed (revert of build 221 / hub 59). The hub asks no OBD-II questions; rusEFI, MegaSquirt and Link work as before |
| 223 | 2026-11-20 | (this commit) | Hub 61: the screens join the hub's own network in the car, so their pages load away from home. While it runs the hub sends its name and password every 10 s (HubNetPacket "MSW1", ESP-NOW, not encrypted); a screen saves them and, with no home network, joins on the car channel only (ESP-NOW unaffected), retrying every 20 s, as 192.168.4.<its home address's last number> (.201 / .202 / .203) or DHCP. Hub's network takes up to 8 devices |
| (aircore 1) | 2026-11-21 | (this commit) | New sketch aircore/: drives the stock NB tach and speedo (air-core, 155 ohm sine and cosine coils) from an ESP32-S2 Mini through two DRV8833 boards. Listens for the hub's ESP-NOW data (RPM, speed) on any channel (hunts until it hears it), no network needed. Calibration over USB serial (zero, span, min, max, strength, slew; hold-at-angle), saved in NVS; start-up sweep; needles to rest 1.5 s after the hub goes quiet |
| 226 | 2026-11-21 | (this commit) | Hub 62: factory setup (docs/FACTORY_SETUP.md). Hub flag on its data; screens with no saved role pick it themselves (remote if a hub or screen sends data, main if only CAN is heard; switch without a restart). First-boot setup on new / reset screens: own network MSP-DASH-Setup-XXXX, /setup for position and home Wi-Fi (NVS; wifi_config.h now optional). Gauge editor lists only signals that ever had data. Factory reset: BOOT held 5 s in the first 15 s then pressed again, Board settings / hub Setup button, serial 'factory reset'. Settings backup: hub Backup page (all boards to one file, restore per part, layout-only onto another screen), screen Board settings export / restore |
| (hub 62, note) | 2026-11-21 | (this commit) | The hub outgrew the default 1.25 MB app area (1,317,260 bytes): it now builds with PartitionScheme=min_spiffs (1.9 MB app, OTA kept; NVS unchanged at 0x9000). Changing the layout needs one USB flash; every hub compile and OTA must use the same FQBN |
| (hub 63) | 2026-11-21 | (this commit) | Crash fix: the CC1101's GDO0 interrupt ran all the time on core 0 (next to Wi-Fi); GDO0 carries the raw demodulated signal, so noise fired it thousands of times a second and starved core 0 (task watchdog in the network task, settings failing to save). Now it is attached only while a burst is recorded; a capture with more than 60 edges / ms is dropped as noise. The ISR is IRAM-safe (esp_timer, GPIO input register instead of micros / digitalRead) |
| 230 | 2026-11-22 | (this commit) | First-boot setup fix: Save returned the setup page itself. AsyncWebServer's "/setup" route also matches "/setup/save", and it was registered first; now registered after /setup/save and /setup.json |
| 231 | 2026-11-22 | (this commit) | A screen's own Restore also takes the hub's whole-system backup (.json): it picks the part with this screen's name (said which parts the file has if none matches). Before, it refused line 1 of the JSON |
| (aircore 2) | 2026-11-22 | (this commit) | aircore also builds for the Wemos D1 Mini (ESP8266): one gauge per board (role tach / speedo over serial, saved in the EEPROM area), DRV8833 on D1 / D2 / D5 / D6 (no boot pins), 20 kHz PWM, ESP-NOW listen with the same channel hunt. The S2 Mini (both gauges) build is unchanged |
| (aircore 3) | 2026-11-22 | (this commit) | aircore: 'pwm <Hz>' command (100-40000, saved) to set the coil PWM frequency; the D1 Mini makes PWM in software, and 20 kHz with a 0-1023 range leaves it uneven (needle stutter and sinking on part of the scale) |
| (aircore 4) | 2026-11-22 | (this commit) | aircore builds for a plain ESP32 DevKitC too: both gauges, hardware PWM (GPIO 16-19 tach, 21-23, 25 speedo). On the bench the stock tach needs 9-10 V on the DRV8833 (at 5 V the return magnet wins above ~5,000 RPM), and the D1 Mini's software PWM made the needle stutter |
| (aircore 5) | 2026-11-22 | (this commit) | aircore: slow-decay drive by default (one DRV8833 input held high, the other PWM'd low; brake between pulses): coil current follows the duty, stronger and in proportion. Fast decay at 20 kHz gave the inductive coils far less than duty x V / R, so the needle stuttered and fell back at the top of the scale even with hardware PWM. 'decay fast' brings the old drive back |
| (aircore 6) | 2026-11-22 | (this commit) | aircore: multi-point calibration, up to 12 marks per gauge (tach mark 2000 takes the held angle; point / unpoint / clearpoints), straight lines between marks, held at the ends; used instead of zero / span once there are 2. On the bench the tach tracks to ~2300 RPM at 10 V on the DRV8833, then the return magnet wins: a TB6612FNG at 12-13 V is next |
| (aircore 6, bench) | 2026-11-22 | (this commit) | Both stock gauges calibrated on DRV8833s at 10 V with slow decay, 0-8000 RPM and 0-145 mph by marks. The tach's earlier limit (~2300 RPM) was a weak DRV8833 channel: a fresh board reaches 8000, no TB6612 needed |
| (aircore 6, bench) | 2026-11-23 | (this commit) | With slow decay and a good DRV8833 both gauges run at 5 V: one 5 V supply for the ESP32 and both drivers. 10 V was only needed to cover the weak board |
| (aircore 7) | 2026-11-23 | (this commit) | Hub 64: Stock gauges page (/aircore). The aircore board sends AcStatus per gauge every half second over ESP-NOW and takes AcCmd from the page: live, continuous sweep (seconds), target value (follows a slider), angle hold; PWM 100 Hz-40 kHz, decay, strength, slew, dial range; marks (save the needle's angle as a number, type one in, remove, clear), factory settings. Serial: tach target 3000, tach sweeping 4 |
| (aircore 8) | 2026-11-23 | (this commit) | Hub 65 / aircore 8: Stock gauges page reliability. Commands carry a number and the hub sends each 3 times (ESP-NOW broadcast has no delivery check); the board applies a number once and reports the last one, so the page flags a command not confirmed. The board hunts channels only after 5 s without the hub's data (was 1.5 s: a short gap sent it hopping, missing commands). Status adds uptime: the page shows it and flags a restart or a quiet board |
| (docs) | 2026-11-23 | (this commit) | KiCad wiring schematic of the whole system (hardware/kicad/msp-dash-wiring.kicad_sch + PDF), generated by tools/gen_kicad_wiring.py: 32 parts, 48 nets; checks every net has two pins and no parts overlap |
| (hardware) | 2026-11-24 | (this commit) | Hub carrier board rev A (hardware/hub-board): 120 x 95 mm, 2 layers, THT. DevKitC on sockets, modules on sockets, screw terminals, protected 12 V input (PTC, reverse diode, TVS) and buck header, divider/clamp inputs. Generated by tools/gen_hub_board.py, routed with Freerouting, DRC: 0 unconnected pads. Gerbers included. Not built yet |
| 245 | 2026-11-24 | (this commit) | Screen colours fix: lv_conf.h rebuilt from LVGL's template had LV_COLOR_16_SWAP 0 (the display needs 1), so every colour came out wrong. The project's own lv_conf.h is now in the repo (lvgl-config/, Waveshare's settings + Montserrat 28/36/48), PREREQUISITES copies it, and lcd_bsp.h refuses to compile with the wrong colour or font settings |
| (docs) | 2026-11-24 | (this commit) | aircore/README.md rewritten for the gauge board as built: ESP32 DevKitC + two DRV8833 at 5 V, pinout (ESP32 to DRV8833, DRV8833 to coils, power), the Stock gauges page, calibration with marks and the bench values, serial commands, troubleshooting, other boards |
| (aircore tools) | 2026-11-24 | (this commit) | aircore/configure.ps1 + calibration.txt: send a saved calibration to the gauge board over USB serial, or save the board's current one (-Save). calibration.txt holds the speedo's new marks (0-150 mph after the rewire); the tach still to calibrate |
| (aircore calibration) | 2026-11-24 | (this commit) | calibration.txt: tach marks 0-8000 RPM (0=0 ... 8000=257, slew 360) and speedo 0-150 mph, both after the rewire, 5 V, slow decay, 40 kHz |
| (aircore 9) | 2026-11-24 | (this commit) | Hub 66 / aircore 9: Stock gauges page diagnostics. The hub counts gauge-board status packets (shown on the page when no board is listed) and takes shorter ones from older boards; the board re-adds its broadcast peer if a send says it is missing, and show prints status sent / failed |
| (hub 67) | 2026-11-24 | (this commit) | Stock gauges page: no more false 'it RESTARTED' (each gauge reports the board's uptime, up to a second apart; a restart is now a drop of more than 5 s) or 'not heard' after a lost packet (flagged after 4 s, was 1.5 s) |
| (aircore 10) | 2026-11-24 | (this commit) | Hub 68 / aircore 10: every command from the Stock gauges page is printed on the gauge board's serial port (number, gauge, what, value), typed commands are echoed (sweep says when done); the page shows 'Board ... applied command N' in green when the board confirms. One status line per board (was per gauge) |
| (docs) | 2026-11-24 | (this commit) | docs/POWER.md: 5 V in the car. Current estimates; option 1: hub carrier board for the hub + a 2 A mini buck (fixed 5 V pad) for the screens and gauge board, with fuse, 1N5822, SMBJ16A TVS (clamps under its 20 V limit) and 100 uF; option 2: one 3-5 A supply for everything |
| (aircore 11) | 2026-11-24 | (this commit) | Gauge board channel choice: it stopped at the first channel with any hub packet, and on the bench sat on channel 1 (a trickle leaking across) while the hub was on 10: the page missed its replies. Now it scans all 13 channels at start-up, keeps the one with the most hub packets, and scans again when fewer than 8 a second arrive. show prints the rate, channel and scans |
| (hub 69) | 2026-11-24 | (this commit) | Stock gauges page: the 'last command not confirmed' warning clears after 8 s (it stayed on after the board restarted, since its command count starts again) |
| (docs) | 2026-11-24 | (this commit) | HUB_PINS.md: GND terminals L19, R6, R12 confirmed. Screens build 245 colours confirmed right; the 5 V buck module for the screens and gauge board set to fixed 5 V and wired with fuse, diode, TVS and capacitor (docs/POWER.md option 1) |
