# Power in the car

Every board runs on 5 V. The car gives 12 to 14.5 V, with spikes well above that (starter, alternator, load dump), and
anything can be wired backwards once. Each 5 V supply therefore needs, in this order from the car:

**switched 12 V (ignition) -> fuse -> reverse-polarity diode -> TVS diode to ground -> capacitor -> buck converter -> 5 V**

Switched 12 V, so nothing drains the battery with the key out.

## Current at 5 V (estimates)

| Load | Typical | Peaks |
|---|---|---|
| 3 screens (ESP32-S3 + AMOLED) | about 0.6 A | about 1 A (Wi-Fi bursts) |
| Gauge board (ESP32 + 2 DRV8833 + the coils) | about 0.3 A | about 0.4 A |
| Hub with its add-ons (CC1101, GPS, OLED, I2C chips, CAN) | about 0.3 A | about 0.5 A |
| **Everything** | **about 1.2 A** | **about 1.9 A** |

## Option 1: hub carrier board + one small buck module for the rest

The hub carrier board (hardware/hub-board) has its own protected input (PTC fuse, 1N5822, P6KE18A TVS, 470 uF) and a
buck module header for the hub. The screens and the gauge board get a second, separate supply:

**Module: mini synchronous buck, 4.5-20 V in, 1-16 V or fixed out, 2 A continuous (3 A peak), 2.54 mm pads**
(the DROK-style "4R7" board with pads A and B on the back). About 0.9 A typical, 1.4 A peaks: within its 2 A.

Set it up before wiring:

1. **Fixed 5 V:** cut pad A on the back through its middle, then bridge the 5 V pads. Measure 5.0 V on the output
   with 12 V on the input before connecting any board. (Left on the trimmer, a knock can change the voltage.)
2. **EN:** leave it unconnected (pulled up inside: always on), or wire it to a 2-5 V signal to switch the 5 V.

| From | Through | To |
|---|---|---|
| Switched 12 V | Fuse, 3 A (blade or PTC) | Diode anode |
| Diode 1N5822 (or 1N5819) cathode | | Module Input + |
| Module Input + | TVS SMBJ16A or P6KE16A (cathode to +) | Ground |
| Module Input + | 100 uF 25 V electrolytic (+ to +) | Ground |
| Car ground | | Module Input - |
| Module Output + | | Screens 5 V, gauge board 5V pin and both DRV8833 VCC |
| Module Output - | | Every board's ground |

- **The TVS must clamp below 20 V** (the module's input limit). SMBJ16A / P6KE16A are fine; the P6KE18A on the hub
  board clamps too high for this module.
- The module has **no reverse-polarity protection and no output overvoltage protection**: the diode in its feed
  is not optional.
- Keep the 5 V and ground wires to the screens short and thick enough (0.5 mm2 / 20 AWG): the screens restart if the
  voltage dips during Wi-Fi bursts.

## Option 2: one bigger supply for everything

A single 12 V to 5 V automotive converter rated **3 A or more** (5 A gives room for later), with the same fuse, diode
and TVS in front (a TVS that suits its input rating). Feed the hub's 5 V from it too: on the hub carrier board, wire
the buck header's OUT+ / OUT- (J4) from this supply and leave its IN pins open, or skip the module and the 12 V input.

## Notes

- One ground point: run every board's ground back to the same place, not to separate chassis bolts.
- Before connecting anything, power the supply alone and measure the 5 V output.
