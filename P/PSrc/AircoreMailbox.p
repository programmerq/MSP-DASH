// Aircore command mailbox (aircore/aircore.ino handlePacket() + loop() + applyCmd()).
//  * The ESP-NOW receive callback copies an AcCmd into ONE slot (s_cmdRx) and sets s_cmdNew; it overwrites whatever
//    is there.
//  * loop() takes the slot when it runs, then applyCmd() drops it if c.seq == s_lastSeq, else acts on it.
//  * applyCmd() for calibration ops calls cfgSave() (NVS write; a flash erase can stall loop() for tens of ms) and
//    loop() is also busy in serial commands / sweeps. Meanwhile the hub's /aircore/cmd sends 3 copies per command,
//    8 ms apart, and the Stock gauges page can fire the next request right away.
// Proposed fix modelled: a small FIFO (FreeRTOS queue) instead of the single slot.

event eAcRadio : (sq: int, id: int);   // one copy of a command, into the receive callback
event eAcLoop;                          // loop() gets to run once
event eAcEnd;                           // test end marker (after enough loop runs)
event eAcHeard : int;                   // announced: the callback got command 'id'
event eAcApplied : int;                 // announced: applyCmd() acted on 'id'
event eAcFinished : bool;               // announced: test end reached the board; payload = the hub page shows a warning
// Hub 68/69 (hub_pages.h): the Stock gauges page keeps ONE pending seq per board (pend[mac] = the newest command)
// and warns "unconfirmed" if the board's reported last seq is not that one after 1.5 s (cleared after 8 s).

machine AircoreRx {
  var cap: int; var q: seq[(sq: int, id: int)]; var lastSq: int; var newestSent: int;
  start state Run {
    entry (queueLen: int) { cap = queueLen; lastSq = 0; }
    on eAcRadio do (c: (sq: int, id: int)) {
      announce eAcHeard, c.id;
      if (c.sq > newestSent) { newestSent = c.sq; }       // stands in for the page's pend[mac] (newest command)
      if (cap == 1) {                                   // LOCK(); memcpy(&s_cmdRx...); s_cmdNew = true;
        if (sizeof(q) == 1) { q -= (0); }
        q += (0, c);
      } else if (sizeof(q) < cap) { q += (sizeof(q), c); }   // xQueueSend (a full queue drops the newest)
    }
    on eAcLoop do {
      var c: (sq: int, id: int);
      if (sizeof(q) == 0) { return; }
      c = q[0]; q -= (0);
      if (c.sq != 0 && c.sq == lastSq) { return; }      // if (c.seq && c.seq == s_lastSeq) return;
      lastSq = c.sq;
      announce eAcApplied, c.id;
    }
    on eAcEnd do { announce eAcFinished, lastSq != newestSent; }   // status reports s_lastSeq
  }
}
