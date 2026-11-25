// Every command is acted on exactly once by a screen that hears at least one copy of it.
spec CmdExactlyOnce observes eScreenRx {
  var applied: set[int];
  start state Watch {
    on eScreenRx do (r: (id: int, ignored: bool)) {
      if (r.ignored) {
        assert r.id in applied, format("screen dropped command {0} as a repeat, but never acted on it", r.id);
      } else {
        assert !(r.id in applied), format("screen acted on command {0} twice", r.id);
        applied += (r.id);
      }
    }
  }
}

// The stock gauges must never show a made-up value while the hub is not heard: no data = needle at rest
// (aircore README / LOST_MS comment: "no data for this long: needles go to the start of the scale").
spec GaugeHonestWhenLost observes eNeedle {
  start state Watch {
    on eNeedle do (n: (heard: bool, fromLiveData: bool, atRest: bool)) {
      assert n.heard || n.atRest, "hub lost but the stock gauge still shows a target/sweep value";
    }
  }
}

// A signal-driven relay must drop out within a bounded time after its sensor data stops.
// Bound = STALE (3) + watchdog (5) + 2 steps of slack.
spec RelayDropsWhenDataStops observes ePin, eStepSeen, eSensorDied {
  var pinOn: bool; var dead: bool; var stepsDead: int;
  start state Watch {
    on ePin do (v: bool) { pinOn = v; }
    on eSensorDied do { dead = true; stepsDead = 0; }
    on eStepSeen do {
      if (dead && pinOn) {
        stepsDead = stepsDead + 1;
        assert stepsDead <= 10, "relay still energised long after its sensor data stopped (hub hung, no watchdog)";
      }
    }
  }
}

// Liveness: once the bus fault is gone, ECU data must come back (hot = still waiting).
event eFaultCleared;
spec EcuDataReturns observes eFaultCleared, eEcuRx {
  start state Ok {
    on eFaultCleared goto Waiting;
    ignore eEcuRx;
  }
  hot state Waiting {
    on eEcuRx goto Ok;
    ignore eFaultCleared;
  }
}

// Every command the aircore board heard is acted on, exactly once.
spec AircoreCmdNotLost observes eAcHeard, eAcApplied, eAcFinished {
  var heard: set[int]; var applied: set[int];
  start state Watch {
    on eAcHeard do (id: int) { heard += (id); }
    on eAcApplied do (id: int) {
      assert !(id in applied), format("aircore applied command {0} twice", id);
      applied += (id);
    }
    on eAcFinished do (warned: bool) {
      var id: int;
      foreach (id in heard) { assert id in applied, format("aircore heard command {0} but never applied it (mailbox overwritten)", id); }
    }
  }
}

// Liveness: the system settles on ONE source of gauge data: the hub if it is there, else one main screen.
spec SingleDataSource observes eSourceOn, eSourceOff {
  var srcs: set[int];
  start state Settled {
    on eSourceOn do (id: int) { srcs += (id); if (sizeof(srcs) > 1) { goto Conflict; } }
    on eSourceOff do (id: int) { srcs -= (id); }
  }
  hot state Conflict {
    on eSourceOn do (id: int) { srcs += (id); }
    on eSourceOff do (id: int) { srcs -= (id); if (sizeof(srcs) <= 1) { goto Settled; } }
  }
}

// A factory reset can only happen in the setup window after power-up, never while driving (here: after 60 s).
spec NoResetWhileDriving observes eFactoryReset {
  start state Watch {
    on eFactoryReset do (now: int) { assert now <= 60, format("factory reset at {0} s after power-up (driving)", now); }
  }
}

// The hub page's confirmation (hub 68/69) should tell the user about any command the board heard but did not apply.
spec PageFlagsLostCmd observes eAcHeard, eAcApplied, eAcFinished {
  var heard: set[int]; var applied: set[int];
  start state Watch {
    on eAcHeard do (id: int) { heard += (id); }
    on eAcApplied do (id: int) { applied += (id); }
    on eAcFinished do (warned: bool) {
      var id: int;
      foreach (id in heard) {
        assert (id in applied) || warned, format("command {0} was lost and the page still showed it as confirmed", id);
      }
    }
  }
}
