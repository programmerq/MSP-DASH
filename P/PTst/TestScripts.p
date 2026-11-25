module CmdLink = { CmdHub, CmdScreen, CmdDriver };
module AircoreM = { Aircore, AircoreDriver };
module OutputsM = { HubOutput, OutputDriver };
module CanM = { HubCan, CanDriver };
module AcMailboxM = { AircoreRx, AcMailboxDriver };
module RolesM = { RoleScreen, RolesDriver };
module FbM = { FactoryButton, FbDriver };

// Expected PASS
test tcPassCmdNoReboot [main = TestCmdNoReboot]: assert CmdExactlyOnce in (union CmdLink, { TestCmdNoReboot });
test tcPassCmdHubRebootBootId [main = TestCmdHubRebootFixed]: assert CmdExactlyOnce in (union CmdLink, { TestCmdHubRebootFixed });
test tcPassAircoreRevertWhenLost [main = TestAircoreFixed]: assert GaugeHonestWhenLost in (union AircoreM, { TestAircoreFixed });
test tcPassOutputsNoHang [main = TestOutputs]: assert RelayDropsWhenDataStops in (union OutputsM, { TestOutputs });
test tcPassOutputsHangWatchdog [main = TestOutputsHangWdt]: assert RelayDropsWhenDataStops in (union OutputsM, { TestOutputsHangWdt });
test tcPassCanRecoverAndStart [main = TestCanRecoverStart]: assert EcuDataReturns in (union CanM, { TestCanRecoverStart });
test tcPassAcMailboxQueue [main = TestAcMailboxQueue]: assert AircoreCmdNotLost in (union AcMailboxM, { TestAcMailboxQueue });
test tcPassRolesOneScreen [main = TestRolesOneScreen]: assert SingleDataSource in (union RolesM, { TestRolesOneScreen });
test tcPassRolesHubYield [main = TestRolesHubFixed]: assert SingleDataSource in (union RolesM, { TestRolesHubFixed });
test tcPassRolesTwoCanYield [main = TestRolesTwoCanFixed]: assert SingleDataSource in (union RolesM, { TestRolesTwoCanFixed });
test tcPassFactoryButtonTimeout [main = TestFactoryButtonFixed]: assert NoResetWhileDriving in (union FbM, { TestFactoryButtonFixed });
test tcPassPageFlagsWithQueue [main = TestAcMailboxQueue]: assert PageFlagsLostCmd in (union AcMailboxM, { TestAcMailboxQueue });

// Expected FAIL against the current firmware (each is a finding)
test tcFailCmdHubReboot [main = TestCmdHubReboot]: assert CmdExactlyOnce in (union CmdLink, { TestCmdHubReboot });
test tcFailAircoreHubLost [main = TestAircore]: assert GaugeHonestWhenLost in (union AircoreM, { TestAircore });
test tcFailOutputsHangNoWatchdog [main = TestOutputsHangNoWdt]: assert RelayDropsWhenDataStops in (union OutputsM, { TestOutputsHangNoWdt });
test tcFailCanNoRecovery [main = TestCanNoRecovery]: assert EcuDataReturns in (union CanM, { TestCanNoRecovery });
test tcFailCanRecoverWithoutStart [main = TestCanRecoverOnly]: assert EcuDataReturns in (union CanM, { TestCanRecoverOnly });
// theoretical only: needs 65535 commands in a row fully lost with the real uint16 seq
test tcFailCmdSeqWrapTheoretical [main = TestCmdSeqWrap]: assert CmdExactlyOnce in (union CmdLink, { TestCmdSeqWrap });
test tcFailAcMailboxSlot [main = TestAcMailboxSlot]: assert AircoreCmdNotLost in (union AcMailboxM, { TestAcMailboxSlot });
test tcFailRolesHubAndCanScreen [main = TestRolesHubAndCanScreen]: assert SingleDataSource in (union RolesM, { TestRolesHubAndCanScreen });
test tcFailRolesTwoCanScreens [main = TestRolesTwoCanScreens]: assert SingleDataSource in (union RolesM, { TestRolesTwoCanScreens });
test tcFailFactoryButtonStuck [main = TestFactoryButton]: assert NoResetWhileDriving in (union FbM, { TestFactoryButton });
test tcFailPageMissesLostCmd [main = TestAcMailboxSlot]: assert PageFlagsLostCmd in (union AcMailboxM, { TestAcMailboxSlot });
