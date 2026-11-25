# Prerequisites

Setup steps for a new Windows machine to build/flash firmware in this
repo. Both firmwares (esp32s3-amoled-bsp-base screens, esp32-input hub) use
arduino-cli.

## One-time environment setup

Requires **winget** (built into modern Windows 10/11). If missing, the
script below prints instructions to install it via Microsoft Store's
"App Installer" rather than attempting an unverified direct download URL.

```powershell
# --- 0. Check for winget, guide manual install if missing ---
$wingetCheck = Get-Command winget -ErrorAction SilentlyContinue
if (-not $wingetCheck) {
    Write-Host "winget not found on this system."
    Write-Host "Please install it manually: open Microsoft Store, search for 'App Installer', and install/update it."
    Write-Host "Then re-run this script."
    return
}

# --- 1. Install arduino-cli via winget ---
winget install ArduinoSA.CLI

Write-Host "If 'arduino-cli' isn't recognized below, close this PowerShell window, open a fresh one, and re-run from here."

# --- 2. Confirm arduino-cli is on PATH ---
arduino-cli version

# --- 3. Init config and install the ESP32 board package (pinned to match the version this project has been built/tested against) ---
arduino-cli config init
arduino-cli core update-index
arduino-cli core install esp32:esp32@3.3.11

# --- 4. Install required libraries ---
arduino-cli lib install "GFX Library for Arduino"
arduino-cli lib install lvgl@8.4.0

# --- 5. lv_conf.h: the project's own copy (Waveshare's settings for this display + the big Montserrat fonts) ---
# (checks for OneDrive-redirected Documents first - common on machines where OneDrive Known Folder Move is active)
$libPath = if (Test-Path "$env:USERPROFILE\OneDrive\Documents\Arduino\libraries\lvgl") { "$env:USERPROFILE\OneDrive\Documents\Arduino\libraries" } else { "$env:USERPROFILE\Documents\Arduino\libraries" }
Copy-Item "C:\Users\alejo\OneDrive\GIT\MSP-DASH\lvgl-config\lv_conf.h" "$libPath\lv_conf.h" -Force   # never the template: it compiles but the colours come out wrong

Write-Host "Setup complete. Library path used: $libPath"
```

If step 2 (`arduino-cli version`) shows "not recognized," close the
PowerShell window and open a fresh one before continuing - winget's PATH
update sometimes doesn't apply to an already-open session.

## Known Git/OneDrive quirk

If this repo lives inside a OneDrive-synced folder, `git pull`/`git commit`
may intermittently fail with:
```
fatal: update_ref failed for ref 'HEAD': cannot update the ref 'refs/heads/main': unable to append to '.git/logs/refs/heads/main': Invalid argument
```
Fix (per-repo, run once):
```powershell
git config windows.appendAtomically false
```
If a pull is interrupted by this error partway through, just re-run the
same `git pull` command - already-fetched objects aren't re-downloaded,
and it will finish updating the branch pointer.

## Board-specific compile flags

The ESP32-S3 AMOLED board (esp32s3-amoled-bsp-base) needs board
options beyond the default profile - its default has PSRAM disabled and
flash set to 4MB, both wrong for this board's actual ESP32-S3R8 chip
(8MB PSRAM) and 16MB flash:

The full commands used for every build (COM port changes per PC/cable; check Device Manager):

```powershell
cd C:\Users\alejo\OneDrive\GIT\MSP-DASH
git pull origin main
arduino-cli compile --clean --fqbn esp32:esp32:esp32s3:PSRAM=opi,FlashSize=16M,CDCOnBoot=cdc,PartitionScheme=app3M_fat9M_16MB --build-path C:\esp32build esp32s3-amoled-bsp-base
arduino-cli upload -p COM4 --fqbn esp32:esp32:esp32s3:PSRAM=opi,FlashSize=16M,CDCOnBoot=cdc,PartitionScheme=app3M_fat9M_16MB --input-dir C:\esp32build esp32s3-amoled-bsp-base
```

Normal way to flash: over WiFi to each board's `/update` page, no USB cable (replace the upload line above).
Boards: Main 192.168.55.231, Remote 1 192.168.55.232, Remote 2 192.168.55.233.

```powershell
cls
foreach ($ip in "192.168.55.231","192.168.55.232","192.168.55.233") { $r = curl.exe -s -H "Expect:" -w " HTTP %{http_code}" -F "firmware=@C:\esp32build\esp32s3-amoled-bsp-base.ino.bin" http://$ip/update; Write-Host "$ip -> $r" }
```

Each line should end in `HTTP 200`; the board then reboots (the build number shows on its web page). A reset or
`HTTP 100` usually still means the update worked: check the build line. Use `curl.exe`, not `curl`: in Windows
PowerShell `curl` is a different command. The USB upload line is the fallback for a board that is off WiFi or won't boot.

### USB serial commands

`arduino-cli monitor -p COMx -c baudrate=115200`, then type a command and press Enter (typing is not echoed):

| Command | What it does |
|---|---|
| `info` | name, build, role, IP (static or DHCP), MAC |
| `ip <ip> <gateway> <mask> [dns]` | static IP, saved, reboots |
| `dhcp` | back to DHCP, reboots |
| `role main` / `role remote` | ESP-NOW role, saved, reboots |
| `name <text>` | board name (up to 20 characters), no reboot |

The same settings over WiFi: `/net?ip=&gw=&mask=`, `/net?dhcp=1`, `/role?set=remote`, `/name?set=Remote%201`.

`--build-path` on compile and `--input-dir` on upload must name the same folder, or upload sends an old binary
(this once looked like an OTA firmware bug). `CDCOnBoot=cdc` puts Serial on the USB port. The partition scheme is
explained below.

## IMPORTANT: partition scheme required

This board's default partition scheme is sized for 4MB flash chips,
NOT this board's actual 16MB - even with FlashSize=16M set, the app
partition stayed tiny (~1.25MB), which we discovered the hard way once
the sketch grew past that ceiling (103% overflow). Compile and upload
commands for esp32s3-amoled-bsp-base MUST include
PartitionScheme=app3M_fat9M_16MB (a real 16MB-sized scheme, 3MB app
partition) or the build will fail once the sketch grows large enough:

```powershell
arduino-cli compile --clean --fqbn esp32:esp32:esp32s3:PSRAM=opi,FlashSize=16M,CDCOnBoot=cdc,PartitionScheme=app3M_fat9M_16MB esp32s3-amoled-bsp-base
arduino-cli upload -p COMx --fqbn esp32:esp32:esp32s3:PSRAM=opi,FlashSize=16M,CDCOnBoot=cdc,PartitionScheme=app3M_fat9M_16MB esp32s3-amoled-bsp-base
```

## Input board (esp32-input)

Bench board ESP32-D at 192.168.55.204. Copy `esp32-input/wifi_config.h.example` to `wifi_config.h` and fill in the same
network as the screens. First flash over USB, then over WiFi:

```powershell
cls
copy C:\Users\alejo\OneDrive\GIT\MSP-DASH\esp32s3-amoled-bsp-base\can_parsing.h,C:\Users\alejo\OneDrive\GIT\MSP-DASH\esp32s3-amoled-bsp-base\can_flags.h,C:\Users\alejo\OneDrive\GIT\MSP-DASH\esp32s3-amoled-bsp-base\gauge_config.h,C:\Users\alejo\OneDrive\GIT\MSP-DASH\esp32s3-amoled-bsp-base\link_packets.h,C:\Users\alejo\OneDrive\GIT\MSP-DASH\esp32s3-amoled-bsp-base\simulator.h,C:\Users\alejo\OneDrive\GIT\MSP-DASH\esp32s3-amoled-bsp-base\simulator.cpp,C:\Users\alejo\OneDrive\GIT\MSP-DASH\esp32s3-amoled-bsp-base\custom_slots.h,C:\Users\alejo\OneDrive\GIT\MSP-DASH\esp32s3-amoled-bsp-base\color_presets.h,C:\Users\alejo\OneDrive\GIT\MSP-DASH\esp32s3-amoled-bsp-base\button_actions.h,C:\Users\alejo\OneDrive\GIT\MSP-DASH\esp32s3-amoled-bsp-base\gps_time.h C:\Users\alejo\OneDrive\GIT\MSP-DASH\esp32-input\
arduino-cli compile --clean --fqbn esp32:esp32:esp32:FlashFreq=40,FlashMode=dio,PartitionScheme=min_spiffs --build-path C:\esp32build-input C:\Users\alejo\OneDrive\GIT\MSP-DASH\esp32-input
arduino-cli upload -p COM6 --fqbn esp32:esp32:esp32:FlashFreq=40,FlashMode=dio,PartitionScheme=min_spiffs --input-dir C:\esp32build-input C:\Users\alejo\OneDrive\GIT\MSP-DASH\esp32-input
curl.exe -s -H "Expect:" -F "firmware=@C:\esp32build-input\esp32-input.ino.bin" http://192.168.55.204/update
```

The input board is also the hub: it reads CAN (SN65HVD230 on GPIO 21 TX, 22 RX) and sends the data to every screen, so all three screens are set to role remote. The copy line brings in the screens' CAN decoder, so there is one copy in the repo.
