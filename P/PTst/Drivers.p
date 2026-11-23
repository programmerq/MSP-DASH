// ---- command link ----
machine CmdDriver {
  start state Init {
    entry (c: (sqMod: int, bootId: bool, cmds: int, reboots: int)) {
      var s: machine;
      s = new CmdScreen(c.bootId);
      new CmdHub((screen = s, sqMod = c.sqMod, useBootId = c.bootId, cmds = c.cmds, reboots = c.reboots));
    }
  }
}
machine TestCmdNoReboot        { start state I { entry { new CmdDriver((sqMod = 65536, bootId = false, cmds = 4, reboots = 0)); } } }
machine TestCmdSeqWrap         { start state I { entry { new CmdDriver((sqMod = 2, bootId = false, cmds = 5, reboots = 0)); } } }
machine TestCmdHubReboot       { start state I { entry { new CmdDriver((sqMod = 65536, bootId = false, cmds = 3, reboots = 1)); } } }
machine TestCmdHubRebootFixed  { start state I { entry { new CmdDriver((sqMod = 65536, bootId = true, cmds = 3, reboots = 2)); } } }

// ---- stock gauges ----
machine AircoreDriver {
  start state Init {
    entry (fix: bool) {
      var a: machine; var i: int;
      a = new Aircore(fix);
      send a, eAcData; send a, eAcStep;
      if ($) { send a, eAcModeCmd, TARGET; } else { if ($) { send a, eAcModeCmd, SWEEP; } }
      i = 0;
      while (i < 6) {                       // the hub may keep sending, or crash / be unplugged / reboot
        if ($) { send a, eAcData; }
        send a, eAcStep; i = i + 1;
      }
    }
  }
}
machine TestAircore      { start state I { entry { new AircoreDriver(false); } } }
machine TestAircoreFixed { start state I { entry { new AircoreDriver(true); } } }

// ---- relay outputs ----
machine OutputDriver {
  start state Init {
    entry (c: (wdt: int, canHang: bool)) {
      var h: machine; var i: int; var died: bool; var hungYet: bool;
      h = new HubOutput((level = 200, hyst = 10, wdtSteps = c.wdt));
      i = 0;
      while (i < 25) {
        if (!died) {
          if (i > 2 && $) { died = true; send h, eSensorGone; }         // sensor unplugged / CAN gone
          else { if ($) { send h, eReading, 220; } else { send h, eReading, 195; } }
        }
        if (c.canHang && !hungYet && $) { hungYet = true; send h, eHang; }
        send h, eOutStep;
        i = i + 1;
      }
    }
  }
}
machine TestOutputs            { start state I { entry { new OutputDriver((wdt = 0, canHang = false)); } } }
machine TestOutputsHangNoWdt   { start state I { entry { new OutputDriver((wdt = 0, canHang = true)); } } }
machine TestOutputsHangWdt     { start state I { entry { new OutputDriver((wdt = 5, canHang = true)); } } }

// ---- CAN bus-off ----
machine CanDriver {
  start state Init {
    entry (p: CanRecovery) {
      var h: machine; var i: int;
      h = new HubCan(p);
      send h, eEcuFrame; send h, eHubLoop;
      send h, eBusErrorBurst;              // glitch: bus-off
      send h, eHubLoop;
      send h, eBusIdle;                    // the bus is fine again
      announce eFaultCleared;
      i = 0;
      while (i < 4) { send h, eHubLoop; send h, eBusIdle; send h, eEcuFrame; i = i + 1; }
    }
  }
}
machine TestCanNoRecovery   { start state I { entry { new CanDriver(NONE); } } }
machine TestCanRecoverOnly  { start state I { entry { new CanDriver(RECOVER_ONLY); } } }
machine TestCanRecoverStart { start state I { entry { new CanDriver(RECOVER_AND_START); } } }
