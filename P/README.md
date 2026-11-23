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

## Not modelled (worth a later pass)

- Aircore's single-slot command mailbox (`s_cmdRx` / `s_cmdNew`): a second command arriving before `loop()` takes
  the first (e.g. while `cfgSave()` writes flash) overwrites it.
- Unauthenticated ESP-NOW and web/OTA: any device in range can send these packets. The model assumes only the hub
  sends; with spoofing, every spec above can be violated at will.
- Factory setup / role assignment, settings save crash-recovery, OLED warning priority.
