# P formal model of MSP-DASH

A [P](https://p-org.github.io/P/) model of the hub/screen/aircore state machines that matter for safety and
correctness. Each machine is an abstraction of real firmware code; the source location is named at the top of each
`.p` file.

## Run

Needs .NET 8 and the P tool (`dotnet tool install --global P`).

    ./run_checks.sh            # SCHED=20000 ./run_checks.sh for a deeper search

`tcPass*` tests must find no bug. `tcFail*` tests find a bug **in the current firmware's behaviour**; each one has a
`tcPass*` twin that models a proposed fix.

## What is modelled

| File | Firmware | Spec |
|---|---|---|
| `PSrc/CmdLink.p` | hub `sendCmd()`/`sendShift()`, screen `handleCmd()` (same scheme: hub `/aircore/cmd` -> `aircore.ino applyCmd`) | `CmdExactlyOnce`: a screen that hears any copy of a command acts on it exactly once |
| `PSrc/Aircore.p` | `aircore.ino` `loop()` / `wantedAngle()` modes | `GaugeHonestWhenLost`: no hub = needle at rest, never a made-up value |
| `PSrc/Outputs.p` | hub `outputsStep()`, `output_want()`, `outputPinSetup()` | `RelayDropsWhenDataStops`: a signal-driven relay drops out within a bounded time after its data stops |
| `PSrc/AircoreMailbox.p` | `aircore.ino` `handlePacket()` single slot `s_cmdRx`/`s_cmdNew`, `loop()`, `applyCmd()` seq check | `AircoreCmdNotLost`: every command the board heard is applied exactly once |
| `PSrc/FactorySetup.p` | screens `autoRoleStep()` / `espnow_link_become_main()`; screens `factoryButtonStep()` and hub `hubFactoryButton()` | `SingleDataSource` (liveness): one data source settles (the hub, else one main); `NoResetWhileDriving`: no factory reset after 60 s |
| `PSrc/Can.p` | hub TWAI setup (`twai_driver_install` + `twai_start` only) | `EcuDataReturns` (liveness): after a bus fault clears, ECU data comes back |

## Findings (tcFail*)

1. **`tcFailCmdHubReboot` — a command is silently dropped after a hub restart.** The hub's `s_seq` restarts at 0
   on every boot; the screens (and aircore, `s_lastSeq`) remember the last seq across the hub's restart. If the
   last command a screen heard before the restart had seq 1 (or N), the hub's first (Nth) command after it is
   dropped as a "repeat". Same for the aircore Stock-gauges command seq. Counterexample: cmd 1 (seq 1) heard,
   hub reboots, cmd 2 (seq 1) dropped. *Fix (`tcPassCmdHubRebootBootId`):* add a random per-boot id
   (`esp_random()`) to the packet and de-duplicate on (boot id, seq); or start `s_seq` at `esp_random()`.
2. **`tcFailAircoreHubLost` — the stock speedo/tach can show a fake value with no hub.** TARGET and SWEEP modes
   (set from the hub's Stock gauges page) are not tied to `heard`. If the hub crashes, reboots, is unplugged, or the
   page is just closed, the needle keeps showing the target / sweeping forever while the car is driven.
   *Fix (`tcPassAircoreRevertWhenLost`):* drop back to LIVE when the hub is not heard (or give TARGET/SWEEP the
   same hold timer ANGLE has).
3. **`tcFailOutputsHangNoWatchdog` — a hung hub holds relays on.** `output_want()` correctly turns a signal output
   off when data stops, but only if `loop()` runs. No task watchdog is configured in any of the firmwares, so a
   stuck I2C/SPI read or deadlock freezes the GPIO (a fan/pump relay stays energised, or stays off).
   *Fix (`tcPassOutputsHangWatchdog`):* subscribe `loop()` to the task watchdog (`esp_task_wdt_add`); at boot
   `outputPinSetup()` already drives outputs off before `pinMode(OUTPUT)`, which the model relies on.
4. **`tcFailCanNoRecovery` — CAN bus-off is permanent.** The hub shows "bus off" but never calls
   `twai_initiate_recovery()`; one wiring glitch loses all ECU data until a power cycle.
5. **`tcFailCanRecoverWithoutStart` — the obvious fix is incomplete.** In ESP-IDF, recovery ends in STOPPED;
   `twai_start()` must be called again. *Fix (`tcPassCanRecoverAndStart`):* in `loop()`, on BUS_OFF call
   `twai_initiate_recovery()`, on STOPPED call `twai_start()`.
6. **`tcFailCmdSeqWrapTheoretical` — not a practical bug.** Single-seq de-dup fails when the seq wraps onto the
   last one heard. With the real uint16 that needs 65535 commands in a row with every copy lost; the test uses
   a modulus of 2 to show the mechanism. It goes away with the fix for finding 1 if the boot id is also bumped on
   wrap, or by keeping a small window of recent seqs.

7. **`tcFailAcMailboxSlot` — aircore loses a command when two arrive before `loop()` runs.** The receive callback
   overwrites the single slot. If `loop()` is held up (a `cfgSave()` flash write after a calibration op, a serial
   command) while the page sends the next command, the earlier one is never applied. Counterexample: commands 1, 2,
   3 in a row; command 2's copies all land while `loop()` is busy and command 3 overwrites them. Losing a "mark this
   number" calibration step is the practical case. *Fix (`tcPassAcMailboxQueue`):* a FreeRTOS queue (`xQueueSend`
   from the callback, drain it in `loop()`); the seq check still drops the repeats.
8. **`tcFailRolesHubAndCanScreen` — a screen with CAN wired can become main next to a hub.** Automatic role picks
   main as soon as a CAN frame arrives before any ESP-NOW data. If the ECU is up before the hub's first packet
   (the hub is still booting / joining Wi-Fi), the screen becomes main. As main its receive callback is `onHello`,
   which ignores data packets, so it never notices the hub: two sources stay on the air until the next power-up,
   the remotes take values from both, and the main screen never gets hub-only signals (EGT, fuel, GPS, IMU,
   custom). Only matters when a screen has its own CAN transceiver in a hub system.
9. **`tcFailRolesTwoCanScreens` — two screens with CAN both become main (screens-only system).** Both see the ECU
   at power-up before either has sent anything, both pick main, neither ever yields.
   *Fix for 8 and 9 (`tcPassRolesHubYield`, `tcPassRolesTwoCanYield`):* a main keeps listening for LinkPackets and
   goes back to remote when it hears the hub (`LINK_COUNT_FROM_HUB`) or a main with a lower MAC. (A short quiet
   period before taking main narrows the window but does not close it.)
10. **`tcFailFactoryButtonStuck` — the "only right after power-up" reset can fire while driving.** State 2 ("let go")
    has no time limit, on the screens and on the hub. A BOOT/page button held or stuck down from power-up (a
    wedged button, something resting on it, a shorted wire) keeps it in state 2; when it frees itself any time later,
    the next page-button press within 10 s wipes the board. Counterexample: pressed at 2 s, held 600 s, pressed again
    8 s after release: reset at 609 s. *Fix (`tcPassFactoryButtonTimeout`):* leave state 2 for "cancelled" after
    ~10 s.

11. **`tcFailPageMissesLostCmd` — the hub 68/69 "applied" confirmation cannot see finding 7.** The Stock gauges
    page now checks the board's reported last seq, but it keeps only one pending command per board (the newest).
    When command 2 is lost and command 3 lands, the page shows "applied command 3" and no warning: the lost
    calibration step goes unnoticed. With a queue on the board (`tcPassPageFlagsWithQueue`) nothing is lost; or the
    page can keep every pending seq per board and warn on each one.

## Upstream re-check (main at bf88c62: hub 69, aircore 11, build 245)

None of findings 1-10 is fixed upstream; the code each one rests on (`sendCmd`/`handleCmd` seq check, aircore
modes, `outputsStep`, TWAI setup, aircore `s_cmdRx` slot, `autoRoleStep`, `factoryButtonStep`/`hubFactoryButton`)
is unchanged. Upstream adds:
- Hub 66-69: status/command confirmation on the Stock gauges page (finding 11 above), short-packet tolerance for
  older aircore boards (length-checked copy: fine).
- Aircore 11: a channel scan (13 channels x 300 ms, keep the loudest) that restarts whenever fewer than 8 hub
  packets/s arrive over 4 s. Not modelled yet. Two things to know: during a scan the board is deaf for ~3.9 s, so
  the needles drop to rest (`LOST_MS` 1.5 s) and any command sent then is missed; and heavy packet loss on the
  right channel (interference, a weak link while driving) looks the same as a wrong channel, so a marginal link
  turns into needles dropping to zero for ~4 s every ~9 s instead of an occasional missed update.
- Build 245: compile-time `lv_conf.h` checks; hardware/KiCad files. No model impact.

## Not modelled (worth a later pass)

- The aircore 11 channel scan (above).

- Settings backup/restore and save crash-recovery; OLED warning priority.
- The screens' factory reset and restore web routes need the X-MSP header, the hub's routes do not (any device on
  the car's network can wipe the hub).
- Unauthenticated ESP-NOW and web/OTA: any device in range can send these packets. The model assumes only the hub
  sends; with spoofing, every spec above can be violated at will.
- Factory setup / role assignment, settings save crash-recovery, OLED warning priority.
