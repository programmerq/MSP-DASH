#!/usr/bin/env python3
"""Hub carrier board (rev A): one parts list -> KiCad PCB (placed), schematic, and a DSN for routing.

The ESP32 DevKitC plugs into two 19-pin sockets; every module plugs into a header socket; car wiring goes to 5.08 mm
screw terminals. Pins follow docs/HUB_PINS.md (defaults). Run with KiCad's Python (pcbnew), from the repo root:
    python3 tools/gen_hub_board.py place      -> hardware/hub-board/hub-board.kicad_pcb (placed, unrouted) + .dsn
    (route the .dsn with Freerouting -> .ses)
    python3 tools/gen_hub_board.py finish     -> imports the .ses, adds ground pours, fills, runs DRC, writes Gerbers
Board 120 x 95 mm, 2 layers, THT only (hand soldering).
"""
import os, sys, math
import pcbnew

FPDIR = "/usr/share/kicad/footprints"
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "hardware", "hub-board")
OX, OY = 50.0, 50.0          # board's top-left corner on the KiCad page, mm
W, H = 120.0, 95.0

def mm(v): return pcbnew.FromMM(v)
def P(x, y): return pcbnew.VECTOR2I(mm(OX + x), mm(OY + y))

# ---- the DevKitC pinout (top view, pin 1 at the antenna end). J2 = left column (Freenove L6-L24), J3 = right (R6-R24)
DEVKIT_J2 = ["3V3", "EN", "IO36", "IO39", "IO34", "IO35", "IO32", "IO33", "IO25", "IO26", "IO27", "IO14", "IO12", "GND", "IO13", "SD2", "SD3", "CMD", "5V"]
DEVKIT_J3 = ["GND", "IO23", "IO22", "TX0", "RX0", "IO21", "GND", "IO19", "IO18", "IO5", "IO17", "IO16", "IO4", "IO0", "IO2", "IO15", "SD1", "SD0", "CLK"]
GPIO_NET = {   # docs/HUB_PINS.md defaults
    "3V3": "+3V3", "GND": "GND", "5V": "+5V_ESP",
    "IO36": "FUEL", "IO39": "RF_MISO", "IO34": "KNOB", "IO35": "LIGHTS", "IO32": "DIMMER", "IO33": "RF_GDO0",
    "IO25": "BTN_NEXT", "IO26": "BTN_PREV", "IO27": "RF_CSN", "IO14": "RF_SCK", "IO12": "SPARE_IO12", "IO13": "RF_MOSI",
    "IO23": "EGT_CS", "IO22": "CAN_RX", "IO21": "CAN_TX", "IO19": "EGT_SO", "IO18": "EGT_SCK", "IO5": "I2C_SDA",
    "IO17": "GPS_RXD", "IO16": "GPS_TXD", "IO4": "DHT_DATA", "IO0": "SPARE_IO0", "IO2": "SPARE_IO2", "IO15": "I2C_SCL",
}

# ---- parts: ref, value, footprint "Lib:Name", {pad: (pin name, net)}, x, y (mm on the board), rotation (deg)
PARTS = []
def part(ref, value, fp, pads, x, y, rot=0): PARTS.append(dict(ref=ref, value=value, fp=fp, pads=pads, x=x, y=y, rot=rot))
def seq(names_nets): return {str(i + 1): nn for i, nn in enumerate(names_nets)}

SOCK = "Connector_PinSocket_2.54mm:PinSocket_1x%02d_P2.54mm_Vertical"
TB = "TerminalBlock_Phoenix:TerminalBlock_Phoenix_MKDS-1,5-%d-5.08_1x%02d_P5.08mm_Horizontal"
RES = "Resistor_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P10.16mm_Horizontal"
CAP = "Capacitor_THT:C_Disc_D5.0mm_W2.5mm_P5.00mm"
ZEN = "Diode_THT:D_DO-35_SOD27_P7.62mm_Horizontal"

# DevKitC, turned so the antenna points left (off a copper-free corner) and the USB port right. Rotating the top view
# 90 degrees anticlockwise puts J3 on the top row and J2 on the bottom row; pin 1 of each at the left.
part("J1", "DevKitC J3 (R6-R24)", SOCK % 19, seq([(n, GPIO_NET.get(n)) for n in DEVKIT_J3]), 14, 14, 90)
part("J2", "DevKitC J2 (L6-L24)", SOCK % 19, seq([(n, GPIO_NET.get(n)) for n in DEVKIT_J2]), 14, 14 + 25.4, 90)

# Power: 12 V in -> PTC fuse -> reverse diode -> TVS + bulk cap -> buck module header -> 5 V; Schottky to the DevKitC 5V pin
part("J3", "12V IN", TB % (2, 2), seq([("+12V", "12V_IN"), ("GND", "GND")]), 112, 12, 90)
part("F1", "MF-RG300 PTC", "Fuse:Fuse_Bourns_MF-RG300", {"1": ("1", "12V_IN"), "2": ("2", "12V_F")}, 98, 6, 0)
part("D1", "1N5822", "Diode_THT:D_DO-201AD_P15.24mm_Horizontal", {"1": ("K", "+12V"), "2": ("A", "12V_F")}, 103, 16, 180)
part("D2", "P6KE18A TVS", "Diode_THT:D_DO-15_P10.16mm_Horizontal", {"1": ("K", "+12V"), "2": ("A", "GND")}, 90, 24, 0)
part("C1", "470uF 35V", "Capacitor_THT:CP_Radial_D8.0mm_P3.50mm", {"1": ("+", "+12V"), "2": ("-", "GND")}, 106, 30, 0)
part("J4", "Buck module (IN+ IN- OUT+ OUT-)", "Connector_PinHeader_2.54mm:PinHeader_1x04_P2.54mm_Vertical",
     seq([("IN+", "+12V"), ("IN-", "GND"), ("OUT+", "+5V"), ("OUT-", "GND")]), 92, 33, 0)
part("C2", "100uF 10V", "Capacitor_THT:CP_Radial_D6.3mm_P2.50mm", {"1": ("+", "+5V"), "2": ("-", "GND")}, 98, 44, 0)
part("D3", "1N5819", "Diode_THT:D_DO-41_SOD81_P10.16mm_Horizontal", {"1": ("K", "+5V_ESP"), "2": ("A", "+5V")}, 78, 47, 180)
part("J5", "5V OUT (screens, gauges)", TB % (2, 2), seq([("+5V", "+5V"), ("GND", "GND")]), 112, 52, 90)

# Modules on sockets
part("J6", "CC1101 (2x4)", "Connector_PinSocket_2.54mm:PinSocket_2x04_P2.54mm_Vertical",
     {"1": ("GND", "GND"), "2": ("VCC", "+3V3"), "3": ("GDO0", "RF_GDO0"), "4": ("CSN", "RF_CSN"),
      "5": ("SCK", "RF_SCK"), "6": ("MOSI", "RF_MOSI"), "7": ("MISO", "RF_MISO"), "8": ("GDO2", None)}, 16, 45, 0)
part("J7", "SN65HVD230 (3V3 GND CTX CRX)", SOCK % 4, seq([("3V3", "+3V3"), ("GND", "GND"), ("CTX", "CAN_TX"), ("CRX", "CAN_RX")]), 24, 45, 0)
part("J8", "SN65HVD230 (CANH CANL)", SOCK % 2, seq([("CANH", "CANH"), ("CANL", "CANL")]), 28, 45, 0)
part("J9", "MAX6675 (GND VCC SCK CS SO)", SOCK % 5, seq([("GND", "GND"), ("VCC", "+3V3"), ("SCK", "EGT_SCK"), ("CS", "EGT_CS"), ("SO", "EGT_SO")]), 33, 45, 0)
part("J10", "OLED (GND VCC SCL SDA)", SOCK % 4, seq([("GND", "GND"), ("VCC", "+3V3"), ("SCL", "I2C_SCL"), ("SDA", "I2C_SDA")]), 38, 45, 0)
part("J11", "MPU6050 GY-521", SOCK % 8, seq([("VCC", "+3V3"), ("GND", "GND"), ("SCL", "I2C_SCL"), ("SDA", "I2C_SDA"), ("XDA", None), ("XCL", None), ("AD0", "GND"), ("INT", None)]), 24, 59, 90)
part("J12", "ADS1115", SOCK % 10, seq([("VDD", "+3V3"), ("GND", "GND"), ("SCL", "I2C_SCL"), ("SDA", "I2C_SDA"), ("ADDR", "GND"), ("ALRT", None),
     ("A0", "ADS_A0"), ("A1", "ADS_A1"), ("A2", "ADS_A2"), ("A3", "ADS_A3")]), 46, 59, 90)
part("J13", "I2C (MCP23017 ...)", SOCK % 4, seq([("3V3", "+3V3"), ("GND", "GND"), ("SCL", "I2C_SCL"), ("SDA", "I2C_SDA")]), 43, 45, 0)
part("J14", "I2C (spare)", SOCK % 4, seq([("3V3", "+3V3"), ("GND", "GND"), ("SCL", "I2C_SCL"), ("SDA", "I2C_SDA")]), 48, 45, 0)
part("J15", "GPS (VCC RX TX GND)", SOCK % 4, seq([("VCC", "+5V"), ("RX", "GPS_RXD"), ("TX", "GPS_TXD"), ("GND", "GND")]), 53, 45, 0)
part("J16", "DHT22 (+ DATA -)", SOCK % 3, seq([("VCC", "+3V3"), ("DATA", "DHT_DATA"), ("GND", "GND")]), 58, 45, 0)
part("J17", "Spare GPIO (0 2 12 GND)", "Connector_PinHeader_2.54mm:PinHeader_1x04_P2.54mm_Vertical",
     seq([("IO0", "SPARE_IO0"), ("IO2", "SPARE_IO2"), ("IO12", "SPARE_IO12"), ("GND", "GND")]), 84, 4, 0)

# CAN termination (most SN65HVD230 boards already have 120 ohm: fit the jumper only if the bus needs it)
part("R1", "120R", RES, {"1": ("1", "CANH"), "2": ("2", "CAN_T")}, 3, 62, 0)
part("JP1", "CAN 120R", "Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical", {"1": ("1", "CAN_T"), "2": ("2", "CANL")}, 17, 62, 90)

# 12 V inputs (lights, dimmer): 47k / 10k divider, 3.3 V zener clamp, 100 nF filter. Fuel: 1k series, clamp, filter.
def divider(n, net, x):
    part(f"R{10 + 2 * n}", "47k", RES, {"1": ("1", f"{net}_12V"), "2": ("2", net)}, x, 66, 270)
    part(f"R{11 + 2 * n}", "10k", RES, {"1": ("1", net), "2": ("2", "GND")}, x + 4, 66, 270)
    part(f"D{10 + n}", "BZX79-C3V3", ZEN, {"1": ("K", net), "2": ("A", "GND")}, x + 8, 66, 270)
    part(f"C{10 + n}", "100nF", CAP, {"1": ("1", net), "2": ("2", "GND")}, x + 12, 68, 270)
divider(0, "LIGHTS", 26)
divider(1, "DIMMER", 44)
part("R14", "1k", RES, {"1": ("1", "FUEL_IN"), "2": ("2", "FUEL")}, 62, 66, 270)
part("R15", "DNP (pull-up)", RES, {"1": ("1", "+3V3"), "2": ("2", "FUEL")}, 66, 66, 270)
part("D12", "BZX79-C3V3", ZEN, {"1": ("K", "FUEL"), "2": ("A", "GND")}, 70, 66, 270)
part("C12", "100nF", CAP, {"1": ("1", "FUEL"), "2": ("2", "GND")}, 74, 68, 270)

# Screw terminals along the bottom edge
part("J20", "CAN (H L GND)", TB % (3, 3), seq([("CANH", "CANH"), ("CANL", "CANL"), ("GND", "GND")]), 4, 89, 0)
part("J21", "LIGHTS DIMMER GND", TB % (3, 3), seq([("LIGHTS", "LIGHTS_12V"), ("DIMMER", "DIMMER_12V"), ("GND", "GND")]), 22, 89, 0)
part("J22", "FUEL GND", TB % (2, 2), seq([("FUEL", "FUEL_IN"), ("GND", "GND")]), 40, 89, 0)
part("J23", "KNOB (3V3 W GND)", TB % (3, 3), seq([("3V3", "+3V3"), ("W", "KNOB"), ("GND", "GND")]), 53, 89, 0)
part("J24", "BUTTONS (NEXT PREV GND)", TB % (3, 3), seq([("NEXT", "BTN_NEXT"), ("PREV", "BTN_PREV"), ("GND", "GND")]), 71, 89, 0)
part("J25", "ADS1115 A0-A3 GND", TB % (5, 5), seq([("A0", "ADS_A0"), ("A1", "ADS_A1"), ("A2", "ADS_A2"), ("A3", "ADS_A3"), ("GND", "GND")]), 89, 89, 0)

for i, (x, y) in enumerate(((4, 4), (116, 91), (5, 74), (116, 72))):
    part(f"H{i + 1}", "M3", "MountingHole:MountingHole_3.2mm_M3", {}, x, y, 0)

# ----------------------------------------------------------------------------------------------------------------
def load_fp(spec):
    lib, name = spec.split(":", 1)
    fp = pcbnew.FootprintLoad(os.path.join(FPDIR, lib + ".pretty"), name)
    if fp is None: raise SystemExit(f"footprint not found: {spec}")
    return fp

def build():
    b = pcbnew.BOARD()
    ds = b.GetDesignSettings()
    ds.SetCopperLayerCount(2)
    nc = ds.m_NetSettings.m_DefaultNetClass
    nc.SetTrackWidth(mm(0.5)); nc.SetClearance(mm(0.25)); nc.SetViaDiameter(mm(0.9)); nc.SetViaDrill(mm(0.45))
    nets = {}
    def net(name):
        if name not in nets:
            n = pcbnew.NETINFO_ITEM(b, name); b.Add(n); nets[name] = n
        return nets[name]
    for p in PARTS:
        fp = load_fp(p["fp"])
        lib, name = p["fp"].split(":", 1); fp.SetFPID(pcbnew.LIB_ID(lib, name))   # library link (KiCad's standard libraries)
        fp.SetReference(p["ref"]); fp.SetValue(p["value"])
        fp.SetPath(pcbnew.KIID_PATH("/" + sym_uuid(p["ref"])))   # linked to its schematic symbol
        fp.SetPosition(P(p["x"], p["y"])); fp.SetOrientationDegrees(p["rot"])
        b.Add(fp)
        for pad in fp.Pads():
            spec = p["pads"].get(pad.GetNumber())
            if spec and spec[1]: pad.SetNet(net(spec[1]))
    # outline
    pts = [(0, 0), (W, 0), (W, H), (0, H)]
    for i in range(4):
        s = pcbnew.PCB_SHAPE(b); s.SetShape(pcbnew.SHAPE_T_SEGMENT); s.SetLayer(pcbnew.Edge_Cuts); s.SetWidth(mm(0.1))
        s.SetStart(P(*pts[i])); s.SetEnd(P(*pts[(i + 1) % 4])); b.Add(s)
    # no copper under the ESP32's antenna (left end, past pin 1)
    ko = pcbnew.ZONE(b); ko.SetIsRuleArea(True); ko.SetDoNotAllowCopperPour(True); ko.SetDoNotAllowTracks(True); ko.SetDoNotAllowVias(True)
    ls = pcbnew.LSET(); ls.AddLayer(pcbnew.F_Cu); ls.AddLayer(pcbnew.B_Cu); ko.SetLayerSet(ls)
    ol = ko.Outline(); ol.NewOutline()
    for x, y in ((0, 8), (11, 8), (11, 45), (0, 45)): ol.Append(mm(OX + x), mm(OY + y))
    b.Add(ko)
    return b

def courtyard_overlaps(b):
    fps = [f for f in b.GetFootprints()]
    bad = []
    for i, a in enumerate(fps):
        ba = a.GetCourtyard(pcbnew.F_CrtYd).BBox() if a.GetCourtyard(pcbnew.F_CrtYd).OutlineCount() else a.GetBoundingBox(False, False)
        for c in fps[i + 1:]:
            bc = c.GetCourtyard(pcbnew.F_CrtYd).BBox() if c.GetCourtyard(pcbnew.F_CrtYd).OutlineCount() else c.GetBoundingBox(False, False)
            if ba.Intersects(bc): bad.append((a.GetReference(), c.GetReference()))
    return bad

def outside(b):
    out = []
    for f in b.GetFootprints():
        if f.GetReference() in ("J1", "J2"): continue   # the DevKitC's own overhang is fine
        cy = f.GetCourtyard(pcbnew.F_CrtYd)
        bb = cy.BBox() if cy.OutlineCount() else f.GetBoundingBox(False, False)
        if bb.GetLeft() < mm(OX) or bb.GetTop() < mm(OY) or bb.GetRight() > mm(OX + W) or bb.GetBottom() > mm(OY + H): out.append(f.GetReference())
    return out


# ---- schematic: the same parts and nets as hub-board.kicad_sch, linked to the PCB footprints (same UUID paths), so
# "Update PCB from Schematic" matches every part and a footprint can be swapped (another plug) in the schematic.
import uuid as _uuid
ROOT_UUID = str(_uuid.uuid5(_uuid.NAMESPACE_URL, "msp-dash/hub-board/root"))
def sym_uuid(ref): return str(_uuid.uuid5(_uuid.NAMESPACE_URL, "msp-dash/hub-board/sym/" + ref))
def write_schematic(path):
    G = 1.27
    _n = [0]
    def uid():
        _n[0] += 1; return str(_uuid.uuid5(_uuid.NAMESPACE_URL, f"msp-dash/hub-board/sch/{_n[0]}"))
    def q(t): return '"' + str(t).replace('\\', '\\\\').replace('"', '\\"') + '"'
    def f(v): return f"{v * G:.2f}".rstrip('0').rstrip('.')
    FONT = "(effects (font (size 1.27 1.27)))"
    HIDE = "(effects (font (size 1.27 1.27)) hide)"
    libs, body = [], []
    col_x, y, col_w, top_margin, bottom = 32, 30, 110, 30, 310   # grid steps; A2 = 467 x 330 steps
    for p in PARTS:
        pins = sorted(p["pads"].items(), key=lambda kv: (len(kv[0]), kv[0]))
        half = (len(pins) + 1) // 2
        left, right = pins[:half], pins[half:]
        nl = max(len(left), len(right), 1)
        wn = max([len(v[0]) for _, v in pins] + [4])
        hw = max(6, (wn + 3) if right else (wn // 2 + 3))   # body half width, steps: room for names on both sides
        hgt = 2 * nl + 2
        if y + hgt + 12 > bottom: col_x += col_w; y = top_margin
        x0, y0 = col_x, y + 4
        name = "hub:" + p["ref"]
        pin_txt = []
        for side, lst in ((-1, left), (1, right)):
            for i, (num, (pname, net)) in enumerate(lst):
                pin_txt.append(f'(pin passive line (at {f(side * (hw + 2))} {f(-2 * i)} {0 if side < 0 else 180}) (length 2.54) (name {q(pname)} {FONT}) (number {q(num)} {FONT}))')
        libs.append(f'(symbol {q(name)} (in_bom yes) (on_board yes)\n'
                    f'  (property "Reference" "U" (at 0 {f(2)} 0) {FONT})\n  (property "Value" {q(p["ref"])} (at 0 0 0) {FONT})\n'
                    f'  (property "Footprint" "" (at 0 0 0) {HIDE})\n  (property "Datasheet" "" (at 0 0 0) {HIDE})\n'
                    f'  (symbol {q(p["ref"] + "_0_1")} (rectangle (start {f(-hw)} {f(1)}) (end {f(hw)} {f(-2 * nl + 1)}) (stroke (width 0.254) (type default)) (fill (type background))))\n'
                    f'  (symbol {q(p["ref"] + "_1_1")} ' + " ".join(pin_txt) + '))')
        su = sym_uuid(p["ref"])
        pu = " ".join(f'(pin {q(num)} (uuid {uid()}))' for num, _ in pins)
        body.append(f'(symbol (lib_id {q(name)}) (at {f(x0)} {f(y0)} 0) (unit 1) (in_bom yes) (on_board yes) (dnp no) (uuid {su})\n'
                    f'  (property "Reference" {q(p["ref"])} (at {f(x0)} {f(y0 - 3)} 0) {FONT})\n'
                    f'  (property "Value" {q(p["value"])} (at {f(x0)} {f(y0 + 2 * nl + 1)} 0) {FONT})\n'
                    f'  (property "Footprint" {q(p["fp"])} (at {f(x0)} {f(y0)} 0) {HIDE})\n'
                    f'  (property "Datasheet" "" (at {f(x0)} {f(y0)} 0) {HIDE})\n  {pu}\n'
                    f'  (instances (project "hub-board" (path {q("/" + ROOT_UUID)} (reference {q(p["ref"])}) (unit 1)))))')
        for side, lst in ((-1, left), (1, right)):
            for i, (num, (pname, net)) in enumerate(lst):
                px, py = x0 + side * (hw + 2), y0 + 2 * i
                if not net:
                    body.append(f'(no_connect (at {f(px)} {f(py)}) (uuid {uid()}))'); continue
                ex = px + side * 3
                body.append(f'(wire (pts (xy {f(px)} {f(py)}) (xy {f(ex)} {f(py)})) (stroke (width 0) (type default)) (uuid {uid()}))')
                body.append(f'(label {q(net)} (at {f(ex)} {f(py)} {0 if side > 0 else 180}) (fields_autoplaced) (effects (font (size 1.27 1.27)) (justify {"left" if side > 0 else "right"} bottom)) (uuid {uid()}))')
        y += hgt + 10
    body.append(f'(text "MSP-DASH hub carrier rev A: generated by tools/gen_hub_board.py. Pins with the same label are connected." (at {f(20)} {f(18)} 0) (effects (font (size 2 2)) (justify left bottom)) (uuid {uid()}))')
    sch = (f'(kicad_sch (version 20230121) (generator eeschema)\n  (uuid {ROOT_UUID})\n  (paper "A2")\n'
           f'  (title_block (title "MSP-DASH hub carrier") (rev "A") (comment 1 "Generated by tools/gen_hub_board.py"))\n'
           '  (lib_symbols\n' + "\n".join(libs) + '\n  )\n' + "\n".join(body) + '\n  (sheet_instances (path "/" (page "1")))\n)\n')
    open(path, "w").write(sch)
    if col_x * G > 594: raise SystemExit("schematic wider than the A2 sheet")

# ---- routing in: Freerouting's .ses (coordinates in 0.1 um, y up) -> tracks and vias
def sexp(text):
    tokens, i, n = [], 0, len(text)
    while i < n:
        c = text[i]
        if c in "()": tokens.append(c); i += 1
        elif c.isspace(): i += 1
        elif c == '"':
            j = text.index('"', i + 1); tokens.append(text[i + 1:j]); i = j + 1
        else:
            j = i
            while j < n and not text[j].isspace() and text[j] not in "()": j += 1
            tokens.append(text[i:j]); i = j
    def parse(k):
        out = []
        while k < len(tokens):
            t = tokens[k]
            if t == "(": sub, k = parse(k + 1); out.append(sub)
            elif t == ")": return out, k + 1
            else: out.append(t); k += 1
        return out, k
    return parse(0)[0][0]
def find(node, name):
    return [c for c in node if isinstance(c, list) and c and c[0] == name]
def import_ses(b, path):
    ses = sexp(open(path).read())
    routes = find(ses, "routes")[0]
    res = find(routes, "resolution")[0]; scale = float(res[2]) * (1000 if res[1] == "um" else 1)   # units per mm
    nets = {b.FindNet(n).GetNetname(): b.FindNet(n) for n in []}
    tw = {}
    for ps in find(find(routes, "library_out")[0], "padstack") if find(routes, "library_out") else []:
        pass
    ntr = nvia = 0
    for net in find(find(routes, "network_out")[0], "net"):
        ni = b.FindNet(net[1])
        for w in find(net, "wire"):
            path_ = find(w, "path")[0]
            layer = pcbnew.F_Cu if path_[1] == "F.Cu" else pcbnew.B_Cu
            width = float(path_[2]) / scale
            pts = [float(v) for v in path_[3:]]
            for k in range(0, len(pts) - 2, 2):
                t = pcbnew.PCB_TRACK(b); t.SetLayer(layer); t.SetWidth(mm(width)); t.SetNet(ni)
                t.SetStart(pcbnew.VECTOR2I(mm(pts[k] / scale), mm(-pts[k + 1] / scale)))
                t.SetEnd(pcbnew.VECTOR2I(mm(pts[k + 2] / scale), mm(-pts[k + 3] / scale)))
                b.Add(t); ntr += 1
        for v in find(net, "via"):
            via = pcbnew.PCB_VIA(b); via.SetPosition(pcbnew.VECTOR2I(mm(float(v[2]) / scale), mm(-float(v[3]) / scale)))
            via.SetWidth(mm(0.9)); via.SetDrill(mm(0.45)); via.SetNet(ni); b.Add(via); nvia += 1
    return ntr, nvia

def finish(pcb, ses):
    b = pcbnew.LoadBoard(pcb)
    ntr, nvia = import_ses(b, ses)
    print(f"routing in: {ntr} track segments, {nvia} vias")
    # ground on both layers
    for layer in (pcbnew.F_Cu, pcbnew.B_Cu):
        z = pcbnew.ZONE(b); z.SetLayer(layer); z.SetNet(b.FindNet("GND"))
        z.SetLocalClearance(mm(0.4)); z.SetMinThickness(mm(0.3))
        z.SetPadConnection(pcbnew.ZONE_CONNECTION_THERMAL); z.SetThermalReliefGap(mm(0.5)); z.SetThermalReliefSpokeWidth(mm(0.5))
        ol = z.Outline(); ol.NewOutline()
        for x, y in ((0.5, 0.5), (W - 0.5, 0.5), (W - 0.5, H - 0.5), (0.5, H - 0.5)): ol.Append(mm(OX + x), mm(OY + y))
        b.Add(z)
    pcbnew.ZONE_FILLER(b).Fill(b.Zones())
    # labels: what plugs in where, on the silkscreen
    SHORT = {"J6": "CC1101", "J7": "CAN 3V3 GND TX RX", "J8": "H L", "J9": "EGT", "J10": "OLED", "J11": "MPU6050", "J12": "ADS1115",
             "J13": "I2C", "J14": "I2C", "J15": "GPS", "J16": "DHT", "J17": "IO0 IO2 IO12 GND", "J4": "BUCK IN+ IN- OUT+ OUT-",
             "J3": "12V IN", "J5": "5V OUT", "J20": "CAN H L GND", "J21": "LIGHTS DIM GND", "J22": "FUEL GND", "J23": "KNOB 3V3 W GND",
             "J24": "NEXT PREV GND", "J25": "A0 A1 A2 A3 GND", "J1": "DevKitC (R6-R24)", "J2": "DevKitC (L6-L24)", "JP1": "CAN 120R"}
    for f in b.GetFootprints():
        if f.GetReference() in SHORT:   # a short label on the silkscreen: what plugs in where
            v = f.Value(); v.SetText(SHORT[f.GetReference()]); v.SetLayer(pcbnew.F_SilkS); v.SetVisible(True)
            v.SetTextSize(pcbnew.VECTOR2I(mm(0.9), mm(0.9))); v.SetTextThickness(mm(0.15))
    t = pcbnew.PCB_TEXT(b); t.SetText("MSP-DASH hub carrier rev A"); t.SetLayer(pcbnew.F_SilkS)
    t.SetTextSize(pcbnew.VECTOR2I(mm(1.5), mm(1.5))); t.SetTextThickness(mm(0.25)); t.SetPosition(P(40, 4)); b.Add(t)
    b.SetFileName(pcb); pcbnew.SaveBoard(pcb, b)
    rpt = os.path.join(OUT, "drc.rpt")
    pcbnew.WriteDRCReport(b, rpt, pcbnew.EDA_UNITS_MILLIMETRES, True)
    print("saved", pcb, "| DRC report", rpt)

if __name__ == "__main__":
    if sys.argv[1:2] == ["finish"]:
        finish(os.path.join(OUT, "hub-board.kicad_pcb"), sys.argv[2] if len(sys.argv) > 2 else os.path.join(OUT, "hub-board.ses")); sys.exit(0)
    os.makedirs(OUT, exist_ok=True)
    pcb = os.path.join(OUT, "hub-board.kicad_pcb")
    if sys.argv[1:] == ["place"]:
        b = build()
        for f in b.GetFootprints(): f.BuildCourtyardCaches()
        ov, off = courtyard_overlaps(b), outside(b)
        print(f"{len(PARTS)} parts, {b.GetNetCount() - 1} nets")
        if ov: print("COURTYARD OVERLAPS:", ov)
        if off: print("OUTSIDE THE BOARD:", off)
        b.SetFileName(pcb); pcbnew.SaveBoard(pcb, b)
        pcbnew.ExportSpecctraDSN(b, os.path.join(OUT, "hub-board.dsn"))
        write_schematic(os.path.join(OUT, "hub-board.kicad_sch"))
        print("saved", pcb)
        if ov or off: sys.exit(1)
