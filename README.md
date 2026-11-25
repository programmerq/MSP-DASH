# MSP-DASH

A digital gauge cluster for a turbocharged 2002 Mazda Miata (NB2) running rusEFI. Three round AMOLED screens show
gauges; one ESP32 "hub" reads the car (CAN bus and its own sensors) and sends everything to them over ESP-NOW.
A small OLED on the hub shows warning lamps and quick pages.

Power in the car (fuse, diode, TVS, buck options): docs/POWER.md.

The stock tach and speedo: `aircore/` (ESP32 DevKitC + two DRV8833 at 5 V, aircore build 8; set up on the hub's Stock gauges page). Pinout and calibration: aircore/README.md.

Current builds: **screens 245**, **hub 69**.

![OLED pages](tools/icons/oled_pages_preview.png)

## Hardware

| Board | Role | Address |
|---|---|---|
| ESP32 DevKitC (Freenove terminal board) | **Hub**: CAN, sensors, buttons, brightness, OLED, GPS, accelerometer, 433 MHz | 192.168.55.204 |
| Waveshare ESP32-S3-Touch-AMOLED-1.43 (466x466) | **Left** screen | 192.168.55.201 |
| same | **Center** screen (also logs to its SD card) | 192.168.55.202 |
| same | **Right** screen | 192.168.55.203 |

Hub peripherals: SN65HVD230 CAN transceiver, MAX6675 (EGT), DHT22 (cabin temp/humidity), fuel level sender, GT-U7 GPS,
CC1101 433 MHz receiver (TPMS, in progress), 0.96" SSD1306 OLED, MPU6050 accelerometer, optional MCP23017 input
expanders, optional ADS1115 analog expanders, two page buttons, dash dimmer and lights-on inputs. Every GPIO with its terminal:
[docs/HUB_PINS.md](docs/HUB_PINS.md) (kept in step with `esp32-input/hub_pins.h` by a test).

## What it does

**Screens** (`esp32s3-amoled-bsp-base/`, LVGL 8 on Waveshare's BSP)
- Up to 8 named pages, each a 1-cell, 2-up, 3-up or 2x2 layout of needle, number, half-moon or trend-graph gauges, or
  a page of indicator lamps.
- Per gauge: signal, range, ticks, warning (flash or full-screen alert), peak hold, caption and value placement by
  coordinates, needle thickness and glow, 79 car icons (Material Design Icons) beside or instead of the caption.
- 10 contrast-checked colour schemes, switchable live from a button.
- Web editor with a live screen preview of each cell (caption, icon, sample value, colours), web dashboard,
  OTA updates.
- SD logging on the center screen: start conditions (key on, RPM, speed, any sensor), log files named by the GPS
  date and time, a time column, button log markers.
- Clock gauge from the GPS time (12/24 hour).

**Hub** (`esp32-input/`)
- rusEFI over CAN (DBC-matched), ECU detection, 16 configurable custom CAN sensors (`/custom`).
- Own sensors: EGT, cabin temp/humidity, fuel level, GPS speed/time, accelerometer (lateral, accel/brake and vertical
  G, yaw rate), lamp inputs.
- Brightness from the car's lights and dimmer (or a knob), with day/night levels.
- Inputs and pins page: every GPIO and expander pin with what uses it; move any function to another GPIO; up to 16 inputs on free pins as buttons (active LOW or HIGH, press and hold actions), switches or analog sensors (pressure, voltage, temperature presets, or custom) feeding custom sensor slots; up to 16 outputs (relays, LEDs) switched by a signal, an input or a button.
- Two buttons, configurable on `/buttons`: press or hold 1/2/3/5/10 s, left/right/both. Actions: next/previous page,
  choose screen, go to page, dim/brighten, colour scheme, race mode, clear warning, log marker, show IPs, OLED next page.
- OLED warning panel (`/oled`): 8 lamps with icons, WARNING and DANGER levels, signal or wire-input triggers, plus
  pages (big number, values, G-meter, clock); a lit lamp takes the screen over.
- Bench simulator (`/simulator`) that drives every screen without the car.
- One web app at the hub's address, with a side menu: Dashboard (status tiles, warning panel, key readings), Setup (brightness, clock, accelerometer, I2C devices), Screens (each screen's own pages, picked Left / Center / Right), OLED, Live and custom sensors, Buttons, Logging, Firmware, Simulator, Raw CAN, 433 MHz.

## Getting started

New PC setup (arduino-cli, ESP32 core 3.3.11, libraries, `lv_conf.h`): [PREREQUISITES.md](PREREQUISITES.md).
Copy `esp32-input/wifi_config.h.example` to `wifi_config.h` (and the same for the screens) with your WiFi details.

### Flashing (PowerShell)

Hub (it builds with a few shared headers from the screen folder, copied in first):

```powershell
cls
cd C:\Users\alejo\OneDrive\GIT\MSP-DASH
git pull origin main
copy C:\Users\alejo\OneDrive\GIT\MSP-DASH\esp32s3-amoled-bsp-base\can_parsing.h,C:\Users\alejo\OneDrive\GIT\MSP-DASH\esp32s3-amoled-bsp-base\can_flags.h,C:\Users\alejo\OneDrive\GIT\MSP-DASH\esp32s3-amoled-bsp-base\gauge_config.h,C:\Users\alejo\OneDrive\GIT\MSP-DASH\esp32s3-amoled-bsp-base\link_packets.h,C:\Users\alejo\OneDrive\GIT\MSP-DASH\esp32s3-amoled-bsp-base\simulator.h,C:\Users\alejo\OneDrive\GIT\MSP-DASH\esp32s3-amoled-bsp-base\simulator.cpp,C:\Users\alejo\OneDrive\GIT\MSP-DASH\esp32s3-amoled-bsp-base\custom_slots.h,C:\Users\alejo\OneDrive\GIT\MSP-DASH\esp32s3-amoled-bsp-base\color_presets.h,C:\Users\alejo\OneDrive\GIT\MSP-DASH\esp32s3-amoled-bsp-base\button_actions.h,C:\Users\alejo\OneDrive\GIT\MSP-DASH\esp32s3-amoled-bsp-base\gps_time.h C:\Users\alejo\OneDrive\GIT\MSP-DASH\esp32-input\
arduino-cli compile --clean --fqbn esp32:esp32:esp32:FlashFreq=40,FlashMode=dio,PartitionScheme=min_spiffs --build-path C:\esp32build-input C:\Users\alejo\OneDrive\GIT\MSP-DASH\esp32-input
curl.exe -s -H "Expect:" -w " HTTP %{http_code}" -F "firmware=@C:\esp32build-input\esp32-input.ino.bin" http://192.168.55.204/update
```

Screens (all three):

```powershell
cls
cd C:\Users\alejo\OneDrive\GIT\MSP-DASH
arduino-cli compile --clean --fqbn esp32:esp32:esp32s3:PSRAM=opi,FlashSize=16M,CDCOnBoot=cdc,PartitionScheme=app3M_fat9M_16MB --build-path C:\esp32build C:\Users\alejo\OneDrive\GIT\MSP-DASH\esp32s3-amoled-bsp-base
foreach ($ip in "192.168.55.201","192.168.55.202","192.168.55.203") { $r = curl.exe -s -H "Expect:" -w " HTTP %{http_code}" -F "firmware=@C:\esp32build\esp32s3-amoled-bsp-base.ino.bin" http://$ip/update; Write-Host "$ip -> $r" }
```

Or build with the commands above and upload from the hub's Firmware page: the hub, and all screens from one file.
If a board stops answering over WiFi, flash it over USB with `arduino-cli upload -p COMx --fqbn <same fqbn> --input-dir <build path> <sketch folder>`.

## Tests

Host tests run on a PC, no boards needed: [tests/host/README.md](tests/host/README.md). They cover the gauge config,
logging, simulator, button logic, GPS time, OLED panel and pages, accelerometer math, the CAN decoder, every web page's
script (in Node), a compile check of the whole hub sketch (`hub_typecheck.sh`) and of the screen sources
(`typecheck.sh`).

## Repository

| Path | What |
|---|---|
| `esp32s3-amoled-bsp-base/` | Screen firmware, plus the headers shared with the hub |
| `esp32-input/` | Hub firmware |
| `tests/host/` | Host tests and stubs |
| `tools/icons/` | Icon pipeline: Material Design Icons to LVGL fonts and 1-bit OLED bitmaps, previews |
| `tools/gen_fonts.sh` | Number fonts |
| `docs/` | Design notes, see below |

## Docs

| File | About |
|---|---|
| [CODE_GUIDE.md](docs/CODE_GUIDE.md) | How the code is organised, file by file |
| [HUB_PINS.md](docs/HUB_PINS.md) | Hub wiring, every GPIO with its terminal |
| [CAN-map.md](docs/CAN-map.md), [rusefi.dbc](docs/rusefi.dbc) | rusEFI CAN frames and decoding |
| [RUSEFI_LUA.md](docs/RUSEFI_LUA.md) | Lua script for EGT to rusEFI |
| [HUB_SPEED_SENSORS.md](docs/HUB_SPEED_SENSORS.md) | Hall speed sensor and GPS |
| [CONFIGURABLE_SENSORS.md](docs/CONFIGURABLE_SENSORS.md) | Custom CAN sensor slots |
| [HUB_433MHZ.md](docs/HUB_433MHZ.md) | CC1101 receiver and the TPMS decode work |
| [TODO.md](docs/TODO.md) | Tasks to be done, and the voltage divider values for 3.3 V inputs |
| [TIMELINE.md](docs/TIMELINE.md) | Build-by-build history |
| [esp32s3-amoled-board.md](docs/esp32s3-amoled-board.md) | Screen bring-up history and hardware findings |

## Status and next steps

Working on the bench: screens, hub, CAN from rusEFI, brightness, buttons, simulator, logging, OLED panel and pages, GPS,
accelerometer, the web app. Everything still open is in [docs/TODO.md](docs/TODO.md).

The old WROOM + Nextion firmware was removed in build 149 and early bring-up sketches in build 114; both are in git history.
