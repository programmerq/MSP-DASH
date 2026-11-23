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
