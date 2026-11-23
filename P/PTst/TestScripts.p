module CmdLink = { CmdHub, CmdScreen, CmdDriver };
module AircoreM = { Aircore, AircoreDriver };
module OutputsM = { HubOutput, OutputDriver };
module CanM = { HubCan, CanDriver };

// Expected PASS
test tcPassCmdNoReboot [main = TestCmdNoReboot]: assert CmdExactlyOnce in (union CmdLink, { TestCmdNoReboot });
test tcPassCmdHubRebootBootId [main = TestCmdHubRebootFixed]: assert CmdExactlyOnce in (union CmdLink, { TestCmdHubRebootFixed });
test tcPassAircoreRevertWhenLost [main = TestAircoreFixed]: assert GaugeHonestWhenLost in (union AircoreM, { TestAircoreFixed });
test tcPassOutputsNoHang [main = TestOutputs]: assert RelayDropsWhenDataStops in (union OutputsM, { TestOutputs });
test tcPassOutputsHangWatchdog [main = TestOutputsHangWdt]: assert RelayDropsWhenDataStops in (union OutputsM, { TestOutputsHangWdt });
test tcPassCanRecoverAndStart [main = TestCanRecoverStart]: assert EcuDataReturns in (union CanM, { TestCanRecoverStart });

// Expected FAIL against the current firmware (each is a finding)
test tcFailCmdHubReboot [main = TestCmdHubReboot]: assert CmdExactlyOnce in (union CmdLink, { TestCmdHubReboot });
test tcFailAircoreHubLost [main = TestAircore]: assert GaugeHonestWhenLost in (union AircoreM, { TestAircore });
test tcFailOutputsHangNoWatchdog [main = TestOutputsHangNoWdt]: assert RelayDropsWhenDataStops in (union OutputsM, { TestOutputsHangNoWdt });
test tcFailCanNoRecovery [main = TestCanNoRecovery]: assert EcuDataReturns in (union CanM, { TestCanNoRecovery });
test tcFailCanRecoverWithoutStart [main = TestCanRecoverOnly]: assert EcuDataReturns in (union CanM, { TestCanRecoverOnly });
// theoretical only: needs 65535 commands in a row fully lost with the real uint16 seq
test tcFailCmdSeqWrapTheoretical [main = TestCmdSeqWrap]: assert CmdExactlyOnce in (union CmdLink, { TestCmdSeqWrap });
