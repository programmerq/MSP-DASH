# KiCad wiring

`msp-dash-wiring.kicad_sch` is the whole system's wiring as a KiCad schematic (A2, one sheet): the hub and everything on
it, the CAN transceiver and ECU, power, the three screens, and the gauge board with its two DRV8833 and the stock tach
and speedo. `msp-dash-wiring.pdf` is the same sheet, ready to print.

Parts are simple box symbols kept inside the file (no extra libraries needed). It is a wiring drawing, not a board:
every part is marked "not on board", so Update PCB from Schematic places nothing. Wires end in net labels: pins with the
same label are connected. Hub pin names carry the Freenove terminal (IO21_R11 = GPIO 21 on terminal R11).

Generated, not drawn by hand: change `tools/gen_kicad_wiring.py`, then run `python3 tools/gen_kicad_wiring.py`. It
checks that every net reaches at least two pins and that no parts overlap. Pins are the defaults in docs/HUB_PINS.md
and aircore/README.md; a role moved on the Inputs and pins page is not shown here.
