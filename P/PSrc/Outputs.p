// Hub relay/LED outputs driven by a signal (esp32-input.ino outputsStep() every 20 ms, hub_inputs.h output_want()).
//  * OUT_SIGNAL with "above level": on when value > level (stays on down to level - hyst); OFF when there is no data.
//  * has_data comes from the signal's age (STALE_MS). Abstracted: STALE = 3 steps without a reading.
//  * No task watchdog is configured anywhere in the firmware. If loop() hangs, the GPIO keeps its last level.
//  * Proposed fix modelled: a task watchdog that resets the hub after WDT steps; at boot outputPinSetup() drives
//    every output to its OFF level before pinMode(OUTPUT).

event eOutStep;                       // the 20 ms tick (as seen by the environment: time passes even if the hub hangs)
event eReading : int;                 // a fresh reading of the signal (value)
event eHang;                          // loop() stops (stuck I2C/SPI read, deadlock, flash stall...)
event ePin : bool;                    // the output pin changed (true = relay energised)
event eSensorGone;                    // environment: the sensor stops for good (unplugged, CAN lost)
event eStepSeen;                      // announced: one 20 ms of real time has passed at the hub
event eSensorDied;                    // announced: for the spec

type OutCfg = (level: int, hyst: int, wdtSteps: int);   // wdtSteps 0 = no watchdog (current firmware)

machine HubOutput {
  var level: int; var hyst: int; var wdtSteps: int;
  var value: int; var stepsSinceData: int; var everData: bool;
  var isOn: bool; var hung: bool; var hungSteps: int;
  start state Run {
    entry (c: OutCfg) {
      level = c.level; hyst = c.hyst; wdtSteps = c.wdtSteps;
      Boot();
    }
    on eReading do (v: int) { if (!hung) { value = v; stepsSinceData = 0; everData = true; } }
    on eHang do { hung = true; hungSteps = 0; }
    on eSensorGone do { announce eSensorDied; }
    on eOutStep do {
      var hasData: bool; var want: bool;
      announce eStepSeen;
      if (hung) {
        if (wdtSteps > 0) {
          hungSteps = hungSteps + 1;
          if (hungSteps >= wdtSteps) { Boot(); }        // watchdog reset -> reboot
        }
        return;                                           // pin keeps its level
      }
      stepsSinceData = stepsSinceData + 1;
      hasData = everData && stepsSinceData < 3;
      // output_want(OUT_SIGNAL, OUTC_ABOVE)
      if (!hasData) { want = false; }
      else if (isOn) { want = value > level - hyst; }
      else { want = value > level; }
      SetPin(want);
    }
  }
  fun Boot() {
    hung = false; hungSteps = 0; everData = false; stepsSinceData = 0; value = 0;
    SetPin(false);                                        // outputPinSetup(): off before OUTPUT
  }
  fun SetPin(v: bool) { if (v != isOn) { isOn = v; announce ePin, isOn; } }
}
