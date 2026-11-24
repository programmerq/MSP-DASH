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

// ---- aircore mailbox ----
// Three commands from the Stock gauges page in a row (e.g. marking calibration points). Between copies loop() may
// run or be stuck in cfgSave(); afterwards loop() runs freely.
machine AcMailboxDriver {
  start state Init {
    entry (queueLen: int) {
      var rx: machine; var id: int; var k: int;
      rx = new AircoreRx(queueLen);
      id = 1;
      while (id <= 3) {
        k = 0;
        while (k < 3) {
          if ($) { send rx, eAcRadio, (sq = id, id = id); }   // else: copy lost on air
          if ($) { send rx, eAcLoop; }                       // else: loop() busy (flash write, serial, ...)
          k = k + 1;
        }
        id = id + 1;
      }
      k = 0; while (k < 12) { send rx, eAcLoop; k = k + 1; }
      send rx, eAcEnd;
    }
  }
}
machine TestAcMailboxSlot  { start state I { entry { new AcMailboxDriver(1); } } }
machine TestAcMailboxQueue { start state I { entry { new AcMailboxDriver(16); } } }

// ---- automatic roles ----
machine RolesDriver {
  start state Init {
    entry (c: (hub: bool, screens: int, canOn: int, fix: bool)) {
      var s: seq[machine]; var i: int; var j: int; var r: int; var hubAt: int; var ecuAt: int; var others: seq[machine];
      i = 0;
      while (i < c.screens) {
        s += (i, new RoleScreen((id = i + 1, hasCan = i < c.canOn, fix = c.fix)));
        i = i + 1;
      }
      i = 0;
      while (i < c.screens) {
        others = default(seq[machine]); j = 0;
        while (j < c.screens) { if (j != i) { others += (sizeof(others), s[j]); } j = j + 1; }
        send s[i], eSetPeers, others; i = i + 1;
      }
      hubAt = choose(3); ecuAt = choose(3);       // who powers up first: hub, ECU, screens
      if (c.hub && hubAt == 0) { announce eSourceOn, 0; }
      r = 0;
      while (r < 8) {
        if (c.hub && r == hubAt && r > 0) { announce eSourceOn, 0; }
        i = 0;
        while (i < c.screens) {
          if (c.hub && r >= hubAt) { send s[i], eLinkData, 0; }
          if (r >= ecuAt) { send s[i], eCanFrame; }
          send s[i], eRoleStep;
          i = i + 1;
        }
        r = r + 1;
      }
    }
  }
}
machine TestRolesOneScreen        { start state I { entry { new RolesDriver((hub = false, screens = 3, canOn = 1, fix = false)); } } }
machine TestRolesHubAndCanScreen  { start state I { entry { new RolesDriver((hub = true,  screens = 2, canOn = 1, fix = false)); } } }
machine TestRolesTwoCanScreens    { start state I { entry { new RolesDriver((hub = false, screens = 2, canOn = 2, fix = false)); } } }
machine TestRolesHubFixed         { start state I { entry { new RolesDriver((hub = true,  screens = 2, canOn = 2, fix = true)); } } }
machine TestRolesTwoCanFixed      { start state I { entry { new RolesDriver((hub = false, screens = 3, canOn = 3, fix = true)); } } }

// ---- factory reset button ----
// The button is held from second 'pressAt' for 'holdS' seconds, released, then pressed again 'gapS' later.
machine FbDriver {
  start state Init {
    entry (timeoutS: int) {
      var fb: machine; var now: int; var pressAt: int; var holdS: int; var gapS: int; var down: bool;
      var holds: seq[int]; var gaps: seq[int];
      holds += (0, 3); holds += (1, 6); holds += (2, 600);    // short, a real 5 s hold, stuck / wedged for 10 min
      gaps += (0, 2); gaps += (1, 8); gaps += (2, 30);
      fb = new FactoryButton(timeoutS);
      pressAt = 1 + choose(14); holdS = choose(holds); gapS = choose(gaps);
      now = 1;
      while (now <= 700) {
        down = (now >= pressAt && now < pressAt + holdS) || now == pressAt + holdS + gapS;
        send fb, eFbTick, down;
        now = now + 1;
      }
    }
  }
}
machine TestFactoryButton      { start state I { entry { new FbDriver(0); } } }
machine TestFactoryButtonFixed { start state I { entry { new FbDriver(10); } } }
