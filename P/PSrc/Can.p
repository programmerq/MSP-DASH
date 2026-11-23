// Hub CAN controller (ESP-IDF TWAI driver, esp32-input.ino setup(): twai_driver_install + twai_start, nothing else).
// TWAI rules (ESP-IDF): transmit/receive errors raise the error counters; past 255 the controller goes BUS_OFF and
// stops. It only leaves BUS_OFF when the app calls twai_initiate_recovery(); after 128 x 11 recessive bits it is
// STOPPED, and the app must call twai_start() again to get RUNNING.
// The firmware only reports "bus off" on the status page; it never recovers.

enum TwaiState { RUNNING, BUS_OFF, RECOVERING, STOPPED }
enum CanRecovery { NONE, RECOVER_ONLY, RECOVER_AND_START }

event eBusErrorBurst;          // wiring glitch / unanswered frames: error counter past 255
event eBusIdle;                // 128 x 11 recessive bits seen (bus healthy again)
event eEcuFrame;               // a rusEFI frame on the wire
event eHubLoop;                // one pass of the hub's loop()
event eEcuRx;                  // the hub received ECU data (announced)

machine HubCan {
  var st: TwaiState; var policy: CanRecovery;
  start state Run {
    entry (p: CanRecovery) { policy = p; st = RUNNING; }
    on eBusErrorBurst do { if (st == RUNNING) { st = BUS_OFF; } }
    on eBusIdle do { if (st == RECOVERING) { st = STOPPED; } }
    on eEcuFrame do { if (st == RUNNING) { announce eEcuRx; } }
    on eHubLoop do {
      if (st == BUS_OFF && policy != NONE) { st = RECOVERING; }               // twai_initiate_recovery()
      if (st == STOPPED && policy == RECOVER_AND_START) { st = RUNNING; }     // twai_start()
    }
  }
}
