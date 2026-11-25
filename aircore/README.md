# aircore: the stock NB tach and speedo

Drives the original Miata tach and speedometer (air-core gauges) from the hub's data. The gauge board only listens:
it picks up the hub's ESP-NOW data packet (RPM and speed) on whatever Wi-Fi channel the hub is on, so it needs no
network. It is set up from the hub's **Stock gauges** page (`http://192.168.55.204/aircore`) or over USB serial.

Current firmware: **aircore build 11** (hub 68 or later for the Stock gauges page).

## What it uses (as built and tested)

| Part | Notes |
|---|---|
| ESP32 DevKitC (WROOM, 38 pin) | Both gauges on one board, hardware PWM |
| 2 x DRV8833 dual H-bridge | One per gauge. If one gauge slips back partway up the dial, try another DRV8833 first (a weak channel did exactly that on the bench) |
| 5 V supply | One 5 V buck from 12 V for the ESP32 and both DRV8833s |
| Stock tach and speedo | Each has two coils at right angles (sine and cosine), about 155 ohm each |

How it works: driving the two coils with sin(angle) and cos(angle) points the needle at that angle. The coils are
driven in **slow decay** (one DRV8833 input held high, the other switched): the coil current follows the duty cycle,
strong enough at 5 V to hold the needle against the gauge's return magnet over the whole dial.

## Pinout

### ESP32 DevKitC to the DRV8833s

| ESP32 | Tach DRV8833 | | ESP32 | Speedo DRV8833 |
|---|---|---|---|---|
| GPIO 16 | IN1 | | GPIO 21 | IN1 |
| GPIO 17 | IN2 | | GPIO 22 | IN2 |
| GPIO 18 | IN3 | | GPIO 23 | IN3 |
| GPIO 19 | IN4 | | GPIO 25 | IN4 |
| 3V3 | EEP / nSLEEP (if the board has one) | | 3V3 | EEP / nSLEEP |

None of these is a boot, flash or input-only pin.

### DRV8833 to the gauge

| DRV8833 | Gauge coil |
|---|---|
| OUT1 | Coil A + |
| OUT2 | Coil A - |
| OUT3 | Coil B + |
| OUT4 | Coil B - |

- Find the coils with a meter: two pairs of terminals, about 155 ohm across each pair, open between the pairs.
- **+ and - are only labels.** Swapping a coil's two wires, or coil A with coil B, only moves where zero is and which
  way the needle turns; calibration takes care of it. What matters: both ends of one coil on OUT1/OUT2, both ends of the
  other on OUT3/OUT4.

### Power

| From | To |
|---|---|
| 5 V supply + | ESP32 5V pin, tach DRV8833 VCC, speedo DRV8833 VCC |
| 5 V supply - | ESP32 GND, both DRV8833 GND (all grounds tied) |

Never put 12 V on the DRV8833 (its limit is 10.8 V). For the 5 V supply in the car (fuse, diode, TVS, which buck
module) see docs/POWER.md. The whole system's wiring, gauge board included, is in
`hardware/kicad/msp-dash-wiring.kicad_sch` (PDF next to it).

## Build and flash (PowerShell)

USB, first time or after a change (check which COM port is the gauge board, not the hub):

```powershell
cls
$repo = "C:\Users\alejo\OneDrive\GIT\MSP-DASH"
cd $repo
git pull origin main
arduino-cli compile --clean --fqbn esp32:esp32:esp32 --build-path C:\esp32build-aircore32 $repo\aircore
arduino-cli upload -p COM6 --fqbn esp32:esp32:esp32 --input-dir C:\esp32build-aircore32 $repo\aircore
arduino-cli monitor -p COM6 -c baudrate=115200
```

Calibration and settings are kept across updates. A hub page update and a gauge board update that change the packets
(as hub 65 / aircore 8 did) must go together.

## Setting it up: the hub's Stock gauges page

Hub menu, Displays, **Stock gauges**. One card per board (PWM and coil drive, shared by its gauges) and one per gauge.

| Control | What it does |
|---|---|
| Live | Follows the hub's data (the default; every restart comes back to live) |
| Sweep | End to end and back continuously, every N seconds |
| Target | The needle follows the slider (RPM or mph), or a typed value |
| Angle | Holds a coil angle: slider, number, +-0.5 degree buttons (for calibration) |
| Save as mark | The needle's angle now becomes that dial number |
| Marks table | Every mark, each with Remove; Clear marks |
| Strength, needle speed, dial range | Per gauge |
| PWM | 100 Hz to 40 kHz (19 steps, or any number); Coil drive: slow or fast decay |
| Factory settings | That gauge back to defaults, marks removed |

The board card shows its uptime, flags a restart, a quiet board (not heard for N s), and a command that was not
confirmed (the hub sends each command 3 times; press again if it says so).

## Calibration

The return magnet makes the needle lag more the further it swings, so the dial is calibrated with **marks**: the coil
angle for each dial number (up to 12 per gauge). With two or more marks the needle goes in a straight line from mark to
mark; with fewer it uses zero and span.

1. Set the gauge to **Angle** and move it until the needle sits exactly on 0. **Save as mark** 0.
2. Move it onto each number (1000, 2000 ... 8000 on the tach; 20, 40 ... on the speedo) and save each as a mark.
   On these gauges a higher number is a **lower** angle (about 31 to 33 degrees per 1000 RPM, about 1.8 degrees per mph).
3. **Sweep** or **Target** to check; re-mark any number that is off (a new angle replaces the old one).
4. Calibrate at the supply voltage the car will use: the needle lags a little more on less current.

The current calibration is in `aircore/calibration.txt` (see below): send it back with `configure.ps1` after a
factory reset or a reflash. Angles depend on how the coils are wired: after rewiring a gauge, calibrate it again.

## Saving and restoring the calibration (configure.ps1)

`aircore/calibration.txt` holds the board's settings as serial commands (PWM, decay, and per gauge: strength, needle
speed, dial range, marks). `aircore/configure.ps1` sends that file to the board, or saves the board's current settings
into it. Close the serial monitor first (only one program can have the port).

```powershell
cls
cd C:\Users\alejo\OneDrive\GIT\MSP-DASH\aircore
powershell -ExecutionPolicy Bypass -File configure.ps1 -Port COM6 -Save    # board -> calibration.txt
powershell -ExecutionPolicy Bypass -File configure.ps1 -Port COM6          # calibration.txt -> board
```

Each line's answer is printed (refused ones in red), then the board's `show`. After calibrating on the page, run
`-Save` and commit the file, so a factory reset or a reflash never loses it. `-File other.txt` uses another file.

## Serial commands (115200, type `help`)

| Command | Does |
|---|---|
| `show` | Build, decay, PWM, channel, packets, each gauge's value, needle, settings and marks |
| `tach angle 118` | Hold the needle at a coil angle for a minute |
| `tach mark 2000` | The held angle becomes the 2000 mark |
| `tach point 2000 118` | Type a mark in |
| `tach unpoint 2000` / `tach clearpoints` | Remove one mark / all |
| `tach target 3000` | Show a fixed value |
| `tach sweeping 4` | Sweep end to end and back every 4 s |
| `tach release` | Back to live data |
| `tach zero 175` / `tach span -260` | Zero and span (used with fewer than 2 marks) |
| `tach min 0` / `tach max 8000` | Dial range |
| `tach strength 1` / `tach slew 720` | Drive 0.1-1 / fastest needle speed, degrees a second |
| `pwm 20000` | PWM frequency, 100-40000 Hz (both gauges) |
| `decay slow` / `decay fast` | Coil drive between pulses |
| `sweep` | One sweep end to end and back |
| `defaults` | Factory settings for both gauges |

The same with `speedo` in place of `tach`.

## Troubleshooting

| Symptom | Likely cause |
|---|---|
| Needle stutters and falls back to zero partway up the dial | A weak DRV8833 channel: swap the board. Check slow decay is on and strength is 1 |
| Needle holds at some angles but not others | One coil not driven: check its two wires and its DRV8833 inputs (angles 0/180 use coil B, 90/270 coil A) |
| Needle sits on 0 at some angles | Normal past the bottom of the dial: the gauge's stop pin |
| `show` says "no hub: hunting channels" | Hub off, or the board has not found its channel yet (it listens to every channel and keeps the one where the hub is loudest) |
| Page says "not heard", but page commands still work sometimes | Wrong channel: close to the hub, a few of its packets leak onto other channels. Build 11 scans again when fewer than 8 hub packets a second arrive; `show` prints the rate and the channel (compare with the hub's on /wifi.json) |
| Page commands sometimes do nothing | Update both: hub 65 and aircore 8 confirm each command |

## Other boards (same sketch)

| Board | fqbn | Gauges | Pins (IN1-IN4) |
|---|---|---|---|
| ESP32-S2 Mini | `esp32:esp32:lolin_s2_mini` | both | tach 3, 5, 7, 9; speedo 11, 12, 16, 18 |
| Wemos D1 Mini (ESP8266) | `esp8266:esp8266:d1_mini` | one per board (`role tach` / `role speedo`) | D1, D2, D5, D6 |

The D1 Mini makes PWM in software: on the bench the needle stuttered near the top of the scale whatever the frequency,
which is why the ESP32 is the one to use. Its settings live in the EEPROM area; `pwm` and `decay` work the same.
