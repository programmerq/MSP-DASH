// ---- 1. Automatic role (esp32s3-amoled-bsp-base.ino autoRoleStep(), espnow_link.cpp espnow_link_become_main()) ----
//  * A screen with no saved role starts remote, listens (listen-only) to its own CAN transceiver and to ESP-NOW.
//  * Any LinkPacket received first -> remote for good (stops listening to CAN).
//  * A CAN frame received first -> main: starts sending LinkPackets, and switches its receive callback to onHello,
//    which ignores LinkPackets. It never looks at other boards' data again, so it cannot notice a hub or another
//    main that came up a moment later. The decision is not saved: it is taken again on every power-up.
// Proposed fix modelled: a main that hears data from the hub, or from a main with a lower id, goes back to remote.

event eLinkData : int;        // a LinkPacket from board 'from' (0 = hub, n = screen n)
event eCanFrame;              // a CAN frame on this screen's own transceiver
event eRoleStep;              // one pass of the screen's loop()
event eSetPeers : seq[machine];
event eSourceOn : int;        // announced: board 'id' sends gauge data
event eSourceOff : int;

machine RoleScreen {
  var myId: int; var hasCan: bool; var yieldFix: bool; var peers: seq[machine];
  var autoDone: bool; var isMain: bool; var everRx: bool; var canSeen: bool;
  start state Run {
    entry (c: (id: int, hasCan: bool, fix: bool)) { myId = c.id; hasCan = c.hasCan; yieldFix = c.fix; }
    on eSetPeers do (p: seq[machine]) { peers = p; }
    on eLinkData do (from: int) {
      if (!isMain) { everRx = true; return; }                       // remote: onRecv takes it
      if (yieldFix && (from == 0 || from < myId)) {                  // fix: step aside
        isMain = false; everRx = true; announce eSourceOff, myId;
      }                                                              // current: onHello ignores data packets
    }
    on eCanFrame do { if (hasCan && !autoDone) { canSeen = true; } }  // twai_receive in listen-only mode
    on eRoleStep do {
      var i: int;
      if (!autoDone) {
        if (everRx) { autoDone = true; }                             // data from another board: remote
        else if (canSeen) { autoDone = true; isMain = true; announce eSourceOn, myId; }   // become main
      }
      if (isMain) { i = 0; while (i < sizeof(peers)) { send peers[i], eLinkData, myId; i = i + 1; } }   // sendTask
    }
  }
}

// ---- 2. Factory reset from the BOOT button (screens: factoryButtonStep(); hub: hubFactoryButton(), same logic) ----
// Designed: only in the first 15 s after power-up: hold 5 s, let go, press again within 10 s. "It can never happen
// while driving." State 2 ("let go") has no time limit. BOOT (GPIO 0) is also the screens' page button.
// One eFbTick = 1 s; the 300 ms debounce in state 3 is rounded to "the next tick".

event eFbTick : bool;         // one second passes; payload = button is down
event eFactoryReset : int;    // announced: wiped, at second 'now'

machine FactoryButton {
  var st: int; var t: int; var now: int; var state2Timeout: int;
  start state Run {
    entry (timeoutS: int) { state2Timeout = timeoutS; st = 0; now = 0; }
    on eFbTick do (down: bool) {
      now = now + 1;
      if (st == 9) { return; }
      if (st == 0) { if (now > 15) { st = 9; } else if (down) { st = 1; t = now; } }
      else if (st == 1) { if (!down) { st = 0; } else if (now - t >= 5) { st = 2; t = now; } }
      else if (st == 2) {
        if (state2Timeout > 0 && now - t > state2Timeout) { st = 9; }   // proposed fix
        else if (!down) { st = 3; t = now; }
      }
      else if (st == 3) {
        if (now - t > 10) { st = 9; }
        else if (down && now - t >= 1) { announce eFactoryReset, now; st = 9; }   // web_monitor_factory_reset()
      }
    }
  }
}
