// Stock tach/speedo driver (aircore/aircore.ino loop() + wantedAngle()).
//  * LIVE mode: needle shows the hub's value if heard within LOST_MS (1500 ms), else the start of the scale.
//  * TARGET / SWEEP / ANGLE modes are set from the hub's Stock gauges web page. They are NOT tied to 'heard':
//    nothing returns the gauge to LIVE when the hub goes away (ANGLE alone has a hold timer).
// Time is abstracted: one eAcStep = 500 ms, so LOST_MS = 3 steps.

enum AcMode { LIVE, TARGET, SWEEP }
event eAcData;                    // a data packet from the hub
event eAcModeCmd : AcMode;         // AcCmd from the hub's page
event eAcStep;                     // time passes, needle updated
event eNeedle : (heard: bool, fromLiveData: bool, atRest: bool);

machine Aircore {
  var mode: AcMode; var stepsSinceRx: int; var everRx: bool; var revertWhenLost: bool;
  start state Run {
    entry (fix: bool) { mode = LIVE; stepsSinceRx = 0; everRx = false; revertWhenLost = fix; }
    on eAcData do { stepsSinceRx = 0; everRx = true; }
    on eAcModeCmd do (m: AcMode) { mode = m; }
    on eAcStep do {
      var heard: bool;
      stepsSinceRx = stepsSinceRx + 1;
      heard = everRx && stepsSinceRx < 3;            // s_lastRx && now - s_lastRx < LOST_MS
      if (revertWhenLost && !heard) { mode = LIVE; }  // proposed fix
      if (mode == LIVE) {
        announce eNeedle, (heard = heard, fromLiveData = heard, atRest = !heard);   // G1.shown = has ? v : lo
      } else {
        announce eNeedle, (heard = heard, fromLiveData = false, atRest = false);    // target / sweep value
      }
    }
  }
}
