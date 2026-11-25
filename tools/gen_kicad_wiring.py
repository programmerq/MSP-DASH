#!/usr/bin/env python3
"""Generate hardware/kicad/msp-dash-wiring.kicad_sch: the whole MSP-DASH wiring as a KiCad schematic.

Every part is a simple box symbol (defined in the file, no external libraries), marked "not on board": this is a wiring
drawing, not a PCB, so "Update PCB from Schematic" has nothing to place (no footprint errors). Connections are net labels on short
wire stubs at each pin, so the drawing stays readable and KiCad's netlist / ERC see the real connections. Pin names
carry the Freenove terminal for the hub (L9, R11 ...). Pins follow docs/HUB_PINS.md (defaults) and aircore/README.md.
Run:  python3 tools/gen_kicad_wiring.py   (rewrites the .kicad_sch and .kicad_pro)
"""
import os, uuid, json

G = 1.27   # KiCad grid; every coordinate is a whole number of grid steps
ROOT = str(uuid.uuid5(uuid.NAMESPACE_URL, "msp-dash-wiring"))
_n = [0]
def uid():
    _n[0] += 1
    return str(uuid.uuid5(uuid.NAMESPACE_URL, f"msp-dash-wiring/{_n[0]}"))
def q(s): return '"' + s.replace('\\', '\\\\').replace('"', '\\"') + '"'
def mm(v): return f"{v * G:.2f}".rstrip('0').rstrip('.')
FONT = "(effects (font (size 1.27 1.27)))"

# ---- parts: (ref, value, symbol name, left pins, right pins, x, y)  pins: (name, net or None for not connected)
PARTS = []
def part(ref, value, sym, left, right, x, y): PARTS.append((ref, value, sym, left, right, x, y))

# Hub
part("U1", "Hub ESP32 DevKitC (Freenove terminal board)", "HUB_ESP32", [
    ("5V", "+5V"), ("3V3 L6", "+3V3"), ("GND", "GND"),
    ("IO36 L8", "FUEL_SENDER"), ("IO34 L10", "BRIGHT_POT"), ("IO35 L11", "LIGHTS_DIV"), ("IO32 L12", "DIMMER_DIV"),
    ("IO25 L14", "BTN_NEXT"), ("IO26 L15", "BTN_PREV"),
    ("IO39 L9", "RF_MISO"), ("IO33 L13", "RF_GDO0"), ("IO27 L16", "RF_CSN"), ("IO14 L17", "RF_SCK"), ("IO13 L20", "RF_MOSI"),
], [
    ("IO21 R11", "CAN_TX"), ("IO22 R8", "CAN_RX"),
    ("IO23 R7", "EGT_CS"), ("IO19 R13", "EGT_SO"), ("IO18 R14", "EGT_SCK"),
    ("IO5 R15", "I2C_SDA"), ("IO15 R21", "I2C_SCL"),
    ("IO17 R16", "GPS_RXD"), ("IO16 R17", "GPS_TXD"),
    ("IO4 R18", "DHT_DATA"),
], 60, 70)

# CAN
part("J1", "rusEFI ECU CAN", "ECU_CAN", [], [("CAN H", "CANH"), ("CAN L", "CANL"), ("GND", "GND")], 150, 30)
part("U2", "SN65HVD230 CAN transceiver", "SN65HVD230", [("3V3", "+3V3"), ("GND", "GND"), ("CTX", "CAN_TX"), ("CRX", "CAN_RX")],
     [("CANH", "CANH"), ("CANL", "CANL")], 200, 30)
# Radio, EGT, GPS, DHT
part("U3", "CC1101 433 MHz (TPMS)", "CC1101", [("1 GND", "GND"), ("2 VCC", "+3V3"), ("3 GDO0", "RF_GDO0"), ("4 CSN", "RF_CSN")],
     [("5 SCK", "RF_SCK"), ("6 MOSI", "RF_MOSI"), ("7 MISO", "RF_MISO"), ("8 GDO2", None)], 150, 70)
part("U4", "MAX6675 EGT", "MAX6675", [("VCC", "+3V3"), ("GND", "GND"), ("SCK", "EGT_SCK"), ("CS", "EGT_CS"), ("SO", "EGT_SO")],
     [("T+", "TC_P"), ("T-", "TC_N")], 200, 70)
part("TC1", "K thermocouple (exhaust)", "THERMOCOUPLE", [("+", "TC_P"), ("-", "TC_N")], [], 250, 70)
part("U5", "GPS (NEO-6M), 9600 baud", "GPS", [("VCC L24", "+5V"), ("GND", "GND"), ("TX", "GPS_TXD"), ("RX", "GPS_RXD")], [], 150, 105)
part("U6", "DHT22 cabin", "DHT22", [("VCC", "+3V3"), ("DATA", "DHT_DATA"), ("GND", "GND")], [], 200, 105)
# I2C bus
part("U7", "SSD1306 OLED 0x3C", "I2C_4", [("VCC", "+3V3"), ("GND", "GND"), ("SCL", "I2C_SCL"), ("SDA", "I2C_SDA")], [], 150, 135)
part("U8", "MPU6050 0x68", "I2C_4", [("VCC", "+3V3"), ("GND", "GND"), ("SCL", "I2C_SCL"), ("SDA", "I2C_SDA")], [], 200, 135)
part("U9", "MCP23017 0x27 (16 lamp inputs)", "MCP23017", [("VCC", "+3V3"), ("GND", "GND"), ("SCL", "I2C_SCL"), ("SDA", "I2C_SDA")],
     [("PA0-PA7", None), ("PB0-PB7", None)], 250, 135)
part("U10", "ADS1115 0x48", "ADS1115", [("VDD", "+3V3"), ("GND", "GND"), ("SCL", "I2C_SCL"), ("SDA", "I2C_SDA"), ("ADDR", "GND")],
     [("A0", None), ("A1", None), ("A2", None), ("A3", None)], 300, 135)
# Analog inputs and buttons
part("J2", "Fuel sender input (see docs)", "CONN_2", [], [("SIG", "FUEL_SENDER"), ("GND", "GND")], 20, 135)
part("RV1", "Brightness knob 10k", "POT", [], [("+", "+3V3"), ("W", "BRIGHT_POT"), ("-", "GND")], 20, 155)
part("J3", "Lights-on wire (12 V)", "CONN_1", [], [("12V", "LIGHTS_12V")], 20, 175)
part("R1", "47k", "RES", [("1", "LIGHTS_12V")], [("2", "LIGHTS_DIV")], 70, 175)
part("R2", "10k", "RES", [("1", "LIGHTS_DIV")], [("2", "GND")], 120, 175)
part("J4", "Dash dimmer wire (12 V)", "CONN_1", [], [("12V", "DIMMER_12V")], 20, 192)
part("R3", "47k", "RES", [("1", "DIMMER_12V")], [("2", "DIMMER_DIV")], 70, 192)
part("R4", "10k", "RES", [("1", "DIMMER_DIV")], [("2", "GND")], 120, 192)
part("SW1", "Next page", "SW", [("1", "BTN_NEXT")], [("2", "GND")], 40, 210)
part("SW2", "Previous page", "SW", [("1", "BTN_PREV")], [("2", "GND")], 100, 210)
# Power
part("J5", "Car 12 V (switched)", "CONN_2", [], [("+12V", "+12V"), ("GND", "GND")], 150, 175)
part("PS1", "Buck 12 V to 5 V", "BUCK", [("IN+", "+12V"), ("IN-", "GND")], [("OUT+", "+5V"), ("OUT-", "GND")], 200, 175)
# Screens (wireless: power only)
part("U11", "Left screen (ESP-NOW)", "SCREEN", [("5V", "+5V"), ("GND", "GND")], [], 150, 200)
part("U12", "Center screen (ESP-NOW, SD)", "SCREEN", [("5V", "+5V"), ("GND", "GND")], [], 200, 200)
part("U13", "Right screen (ESP-NOW)", "SCREEN", [("5V", "+5V"), ("GND", "GND")], [], 250, 200)
# Gauge board (aircore)
part("U14", "Gauge board ESP32 DevKitC (aircore)", "GB_ESP32", [("5V", "+5V"), ("GND", "GND"), ("3V3", "GB_3V3")], [
    ("IO16", "TACH_IN1"), ("IO17", "TACH_IN2"), ("IO18", "TACH_IN3"), ("IO19", "TACH_IN4"),
    ("IO21", "SPD_IN1"), ("IO22", "SPD_IN2"), ("IO23", "SPD_IN3"), ("IO25", "SPD_IN4")], 150, 235)
for i, (nm, pre, y) in enumerate((("tach", "TACH", 230), ("speedo", "SPD", 262))):
    part(f"U{15+i}", f"DRV8833 {nm}", "DRV8833", [("VCC", "+5V"), ("GND", "GND"), ("EEP", "GB_3V3"),
         ("IN1", f"{pre}_IN1"), ("IN2", f"{pre}_IN2"), ("IN3", f"{pre}_IN3"), ("IN4", f"{pre}_IN4")],
         [("OUT1", f"{pre}_A_P"), ("OUT2", f"{pre}_A_N"), ("OUT3", f"{pre}_B_P"), ("OUT4", f"{pre}_B_N")], 210, y)
    part(f"M{1+i}", f"Stock {nm} (air-core, 155 ohm coils)", "AIRCORE",
         [("A+", f"{pre}_A_P"), ("A-", f"{pre}_A_N"), ("B+", f"{pre}_B_P"), ("B-", f"{pre}_B_N")], [], 265, y)

TEXTS = [
    ("MSP-DASH wiring (generated by tools/gen_kicad_wiring.py; pins per docs/HUB_PINS.md defaults and aircore/README.md)", 20, 15, 2.5),
    ("Hub", 20, 24, 2), ("CAN, radio, EGT, GPS, cabin", 150, 24, 2), ("I2C bus (3.3 V only)", 150, 127, 2),
    ("Analog inputs and buttons", 20, 127, 2), ("Power", 150, 167, 2), ("Screens (data over ESP-NOW, no wires)", 150, 192, 2),
    ("Gauge board (data over ESP-NOW)", 150, 225, 2),
    ("Nets connect by label. GND terminals on the Freenove board: L19, R6, R12.", 20, 285, 1.5),
]

# ---- output
def sym_geom(left, right):
    lw = max([len(n) for n, _ in left] + [0]); rw = max([len(n) for n, _ in right] + [0])
    half = max(6, (lw + rw) // 2 + 4)          # body half-width in grid steps (~1 step per character)
    rows = max(len(left), len(right), 1)
    top = rows                                  # pins at y = top-2, top-4, ... (2 steps apart)
    return half, rows, top
def lib_symbol(sym, left, right):
    half, rows, top = sym_geom(left, right)
    body = [f'(rectangle (start {mm(-half)} {mm(top)}) (end {mm(half)} {mm(top - 2 * rows - 2)}) (stroke (width 0.254) (type default)) (fill (type background)))']
    pins = []
    for i, (n, _) in enumerate(left):
        pins.append(f'(pin passive line (at {mm(-half - 2)} {mm(top - 2 - 2 * i)} 0) (length 2.54) (name {q(n)} {FONT}) (number {q(str(i + 1))} {FONT}))')
    for i, (n, _) in enumerate(right):
        pins.append(f'(pin passive line (at {mm(half + 2)} {mm(top - 2 - 2 * i)} 180) (length 2.54) (name {q(n)} {FONT}) (number {q(str(len(left) + i + 1))} {FONT}))')
    return (f'(symbol {q("msp:" + sym)} (in_bom yes) (on_board no)\n'
            f'  (property "Reference" "U" (at 0 {mm(top + 2)} 0) {FONT})\n  (property "Value" {q(sym)} (at 0 {mm(top - 2 * rows - 4)} 0) {FONT})\n'
            f'  (property "Footprint" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))\n  (property "Datasheet" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))\n'
            f'  (symbol {q(sym + "_0_1")} {" ".join(body)})\n  (symbol {q(sym + "_1_1")}\n    ' + "\n    ".join(pins) + '))')

out, libs, seen, labels, nc = [], [], {}, [], []
boxes = []
for idx, (ref, value, sym, left, right, x, y) in enumerate(PARTS):
    layout = tuple(n for n, _ in left) + ("|",) + tuple(n for n, _ in right)
    if sym in seen and seen[sym] != layout:   # same name, other pins: its own symbol
        k = 2
        while f"{sym}_{k}" in seen and seen[f"{sym}_{k}"] != layout: k += 1
        sym = f"{sym}_{k}"
    if sym not in seen:
        seen[sym] = layout; libs.append(lib_symbol(sym, left, right))
    half, rows, top = sym_geom(left, right)
    pin_uuids = " ".join(f'(pin {q(str(i + 1))} (uuid {uid()}))' for i in range(len(left) + len(right)))
    out.append(f'(symbol (lib_id {q("msp:" + sym)}) (at {mm(x)} {mm(y)} 0) (unit 1) (in_bom yes) (on_board no) (dnp no) (uuid {uid()})\n'
               f'  (property "Reference" {q(ref)} (at {mm(x)} {mm(y - top - 2)} 0) {FONT})\n'
               f'  (property "Value" {q(value)} (at {mm(x)} {mm(y - top + 2 * rows + 4)} 0) {FONT})\n'
               f'  (property "Footprint" "" (at {mm(x)} {mm(y)} 0) (effects (font (size 1.27 1.27)) hide))\n'
               f'  (property "Datasheet" "" (at {mm(x)} {mm(y)} 0) (effects (font (size 1.27 1.27)) hide))\n'
               f'  {pin_uuids}\n  (instances (project "msp-dash-wiring" (path {q("/" + ROOT)} (reference {q(ref)}) (unit 1)))))')
    lw = max([len(net or "") for _, net in left] + [0]); rw = max([len(net or "") for _, net in right] + [0])
    boxes.append((ref, x - half - 6 - lw, y - top - 3, x + half + 6 + rw, y - top + 2 * rows + 6))   # body, stubs, labels, ref, value
    for side, pins in ((-1, left), (1, right)):
        for i, (n, net) in enumerate(pins):
            px, py = x + side * (half + 2), y - (top - 2 - 2 * i)   # schematic Y runs down
            if net is None: nc.append((px, py)); continue
            ex = px + side * 4
            out.append(f'(wire (pts (xy {mm(px)} {mm(py)}) (xy {mm(ex)} {mm(py)})) (stroke (width 0) (type default)) (uuid {uid()}))')
            just = "left" if side > 0 else "right"
            out.append(f'(label {q(net)} (at {mm(ex)} {mm(py)} {0 if side > 0 else 180}) (fields_autoplaced) (effects (font (size 1.27 1.27)) (justify {just} bottom)) (uuid {uid()}))')
            labels.append((net, ref, n))
for px, py in nc:
    out.append(f'(no_connect (at {mm(px)} {mm(py)}) (uuid {uid()}))')
for t, x, y, s in TEXTS:
    out.append(f'(text {q(t)} (at {mm(x)} {mm(y)} 0) (effects (font (size {s} {s})) (justify left bottom)) (uuid {uid()}))')

sch = (f'(kicad_sch (version 20230121) (generator eeschema)\n  (uuid {ROOT})\n  (paper "A2")\n'
       f'  (title_block (title "MSP-DASH wiring") (comment 1 "Generated by tools/gen_kicad_wiring.py"))\n'
       '  (lib_symbols\n' + "\n".join(libs) + '\n  )\n' + "\n".join(out) +
       f'\n  (sheet_instances (path "/" (page "1")))\n)\n')
here = os.path.dirname(os.path.abspath(__file__))
dst = os.path.join(here, "..", "hardware", "kicad")
os.makedirs(dst, exist_ok=True)
open(os.path.join(dst, "msp-dash-wiring.kicad_sch"), "w").write(sch)
open(os.path.join(dst, "msp-dash-wiring.kicad_pro"), "w").write(json.dumps({"meta": {"filename": "msp-dash-wiring.kicad_pro", "version": 1}}, indent=2) + "\n")

# ---- check: every net reaches at least two pins (a single pin on a net is a wiring mistake in this file)
nets = {}
for net, ref, n in labels: nets.setdefault(net, []).append(f"{ref}.{n}")
lonely = {k: v for k, v in nets.items() if len(v) < 2}
print(f"{len(PARTS)} parts, {len(nets)} nets, {len(labels)} connections, {len(nc)} not connected")
if lonely: print("NETS WITH ONE PIN:", lonely); raise SystemExit(1)
# ---- check: no two parts (with their labels) overlap on the sheet, everything on the A2 page (594 x 420 mm)
bad = [(a[0], b[0]) for i, a in enumerate(boxes) for b in boxes[i + 1:] if a[1] < b[3] and b[1] < a[3] and a[2] < b[4] and b[2] < a[4]]
off = [b[0] for b in boxes if b[1] < 0 or b[2] < 0 or b[3] * G > 594 or b[4] * G > 420]
if bad or off: print("OVERLAP:", bad, "OFF PAGE:", off); raise SystemExit(1)
print("no overlaps, all on the page")
