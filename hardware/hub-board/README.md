# Hub carrier board, rev A

The hub's ESP32 DevKitC plugs into two 19-pin sockets; every module plugs into a socket; the car's wiring goes to
5.08 mm screw terminals. 120 x 95 mm, 2 layers, through-hole only (hand soldering). Pins are the defaults in
docs/HUB_PINS.md; the board replaces the Freenove terminal board.

**Status: designed and routed, not built.** Design rules pass with no unconnected pads (drc.rpt). What is left in the
report is cosmetic (silkscreen text overlapping, footprint library links that resolve on a PC with KiCad's standard
libraries).

Files: `hub-board.kicad_pro` (open this in KiCad 7 or 8: it brings the schematic and the board), `hub-board.kicad_sch`,
`hub-board.kicad_pcb`, `hub-board-gerbers.zip` (upload to JLCPCB: 2 layers, 1.6 mm, 1 oz, any colour), `gerbers/`.

The schematic and the board are linked part by part, so a change in the schematic goes to the board with Tools >
Update PCB from Schematic. To use a different plug: change that connector's Footprint field in the schematic (for
example a JST-XH or Molex Micro-Fit footprint with the same number of pins), update the PCB, move the new part into
place and re-route its tracks. Parts are boxes with every pin named; pins with the same label are connected.

## Check before ordering

1. **DevKitC row spacing:** the board uses 25.4 mm between the two 19-pin rows. Measure yours.
2. **Module pin order** (each socket's pin 1 has the square pad). Clones differ: check yours against these.

| Socket | Module | Pins from pin 1 |
|---|---|---|
| J6 (2x4) | CC1101 | 1 GND, 2 VCC, 3 GDO0, 4 CSN, 5 SCK, 6 MOSI, 7 MISO, 8 GDO2 |
| J7 + J8 | SN65HVD230 | 3V3, GND, CTX, CRX + CANH, CANL |
| J9 | MAX6675 | GND, VCC, SCK, CS, SO |
| J10 | SSD1306 OLED | GND, VCC, SCL, SDA |
| J11 | MPU6050 GY-521 | VCC, GND, SCL, SDA, XDA, XCL, AD0, INT |
| J12 | ADS1115 | VDD, GND, SCL, SDA, ADDR, ALRT, A0, A1, A2, A3 |
| J13, J14 | I2C (MCP23017, spare) | 3V3, GND, SCL, SDA |
| J15 | GPS (NEO-6M) | VCC (5 V), RX, TX, GND |
| J16 | DHT22 | +, DATA, - |
| J4 | Buck module (wired) | IN+, IN-, OUT+, OUT- |
| J17 | Spare GPIO | IO0, IO2, IO12, GND |

3. **Fuel sender:** J22 goes through 1k with a 3.3 V clamp. R15 (pull-up to 3.3 V) is not fitted; fit it only if your
   sender wiring needs one.
4. **CAN termination:** most SN65HVD230 boards have 120 ohm already. JP1 adds R1 (120 ohm): fit the jumper only if the
   bus needs it.

## Parts (besides the modules)

| Ref | Part |
|---|---|
| F1 | Bourns MF-RG300 PTC fuse |
| D1 | 1N5822 (reverse polarity) |
| D2 | P6KE18A TVS |
| D3 | 1N5819 (5 V to the DevKitC, so USB and the car supply cannot feed each other) |
| C1 | 470 uF 35 V radial, 8 mm |
| C2 | 100 uF 10 V radial, 6.3 mm |
| R10, R12 | 47k; R11, R13 | 10k (lights and dimmer dividers) |
| R14 | 1k; R15 | pull-up, not fitted |
| D10-D12 | BZX79-C3V3 zener |
| C10-C12 | 100 nF disc |
| R1 | 120 ohm; JP1 | 2-pin header + jumper |
| J1, J2 | 1x19 female header 2.54 mm |
| J3, J5, J20-J25 | Phoenix MKDS 1,5 5.08 mm terminals (2, 3, 5 way) |
| Sockets | female headers 2.54 mm: 2x4, 1x2, 1x3, 1x4, 1x5, 1x8, 1x10 |
| H1-H4 | M3 mounting holes |

Traces are 0.5 mm (power included: the hub, screens and gauge board together draw well under 2 A); ground is poured on
both sides. The area under the ESP32's antenna (left end) has no copper.

## Regenerating

`tools/gen_hub_board.py` holds the parts list, placement and nets. With KiCad 7's Python:

```
python3 tools/gen_hub_board.py place                     # board with parts, and hub-board.dsn for the router
java -jar freerouting-2.0.1.jar --gui.enabled=false -de hub-board.dsn -do run.ses -mp 25 -mt 1
python3 tools/gen_hub_board.py finish hardware/hub-board/run.ses   # routes in, ground pours, DRC
```

Freerouting 2.0.1 single-threaded (`-mt 1`): multi-threaded and 2.1.0 runs left connections out of the .ses. If DRC
still lists unconnected pads, export a DSN from that board and route it once more (the second pass keeps the first
pass's tracks and adds the rest); that is how this board's `hub-board.ses` was made.
