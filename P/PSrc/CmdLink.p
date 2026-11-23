// Hub -> screen button commands over ESP-NOW broadcast.
// Source: esp32-input/esp32-input.ino sendCmd()/sendShift() and esp32s3-amoled-bsp-base/espnow_link.cpp handleCmd().
//  * Hub: s_seq is a uint16 that starts at 0 on every boot; each command is sent 3 times with the same sq.
//  * Screen: s_lastCmdSeq (init 0xFFFFFFFF) remembers ONE sq; a packet whose sq equals it is dropped as a repeat.
// The same scheme is used for Stock gauges commands (hub /aircore/cmd -> aircore.ino applyCmd, s_lastSq).
// Broadcast has no ack, so each copy may be lost. ESP-NOW frames between one pair arrive in order (P sends are FIFO).

event eCmd : (boot: int, sq: int, id: int);
// Announced for the spec: what the screen did with one copy of command 'id'.
event eScreenRx : (id: int, ignored: bool);
event eCmdIssued : int;

type HubCfg = (screen: machine, sqMod: int, useBootId: bool, cmds: int, reboots: int);

machine CmdHub {
  var screen: machine;
  var sq: int; var sqMod: int; var boot: int; var nextId: int;
  var rebootsLeft: int;
  start state Run {
    entry (cfg: HubCfg) {
      var n: int;
      screen = cfg.screen; sqMod = cfg.sqMod; rebootsLeft = cfg.reboots; n = cfg.cmds;
      sq = 0; boot = 0; nextId = 0;
      while (n > 0) {
        if (rebootsLeft > 0 && $) { rebootsLeft = rebootsLeft - 1; boot = boot + 1; sq = 0; }   // power blip / crash / OTA
        SendCmd();
        n = n - 1;
      }
    }
  }
  fun SendCmd() {
    var i: int;
    sq = (sq + 1) % sqMod;          // p.sq = ++s_seq  (uint16 wraps)
    nextId = nextId + 1;
    announce eCmdIssued, nextId;
    i = 0;
    while (i < 3) {                    // for (i < 3) esp_now_send(...)
      if ($) send screen, eCmd, (boot = boot, sq = sq, id = nextId);   // else: copy lost on air
      i = i + 1;
    }
  }
}

machine CmdScreen {
  var lastSq: int; var lastBoot: int; var useBootId: bool;
  start state Run {
    entry (bootId: bool) { useBootId = bootId; lastSq = -1; lastBoot = -1; }
    on eCmd do (c: (boot: int, sq: int, id: int)) {
      var dup: bool;
      // current firmware: if (c.sq == s_lastCmdSeq) return;   proposed fix: key on (hub boot id, sq)
      dup = c.sq == lastSq && (!useBootId || c.boot == lastBoot);
      announce eScreenRx, (id = c.id, ignored = dup);
      if (!dup) { lastSq = c.sq; lastBoot = c.boot; }
    }
  }
}
