//! Explicit in-memory regressions. Loading this addon does not execute them or enter Play.
class GC_FrameworkActionTests : GC_MissionRuleTests
{
	//------------------------------------------------------------------------------------------------
	static GC_RuleAction Command(GC_ERuleAction kind, string name, int operation = 0, int amount = 0)
	{
		GC_RuleAction action = new GC_RuleAction();
		action.m_eDo = kind;
		action.m_sTextOrName = name;
		action.m_iOperation = operation;
		action.m_iAmount = amount;
		return action;
	}

	//------------------------------------------------------------------------------------------------
	string RunActions()
	{
		GC_MissionRuleEngine engine = new GC_MissionRuleEngine();
		GC_MissionFlag flag = GC_FlagMemoryTests.Memory("memory", false);
		flag.m_iStartCallCount = 5;
		engine.Start({}, {flag}, SCR_EGameModeState.GAME);
		engine.QueueFrameworkAction(Command(GC_ERuleAction.FLAG_CONTROL, "memory"));
		Check(engine.GetFlagCallCount("memory") == 5, "Call respects nonrepeatable positive starting count");
		engine.QueueFrameworkAction(Command(GC_ERuleAction.FLAG_CONTROL, "memory", 3, 2));
		Check(engine.GetFlagCallCount("memory") == 3 && engine.GetFlag("memory"), "Subtract keeps positive memory true");
		engine.QueueFrameworkAction(Command(GC_ERuleAction.FLAG_CONTROL, "memory", 3, 20));
		Check(engine.GetFlagCallCount("memory") == 0 && !engine.GetFlag("memory"), "Subtract floors at zero");
		engine.QueueFrameworkAction(Command(GC_ERuleAction.FLAG_CONTROL, "memory"));
		Check(engine.GetFlagCallCount("memory") == 1, "Zero rearms a nonrepeatable flag");
		engine.QueueFrameworkAction(Command(GC_ERuleAction.FLAG_CONTROL, "memory", 5));
		Check(engine.GetFlagCallCount("memory") == 5, "Restore uses configured starting count");
		engine.QueueFrameworkAction(Command(GC_ERuleAction.FLAG_CONTROL, "memory", 4));
		Check(!engine.GetFlag("memory"), "Reset clears rather than restores start count");

		GC_CompiledMissionRule sequence = EventRule("sequence");
		sequence.m_aThen = {Command(GC_ERuleAction.FLAG_CONTROL, "memory", 1, 5), Command(GC_ERuleAction.FLAG_CONTROL, "memory", 2, 2), Command(GC_ERuleAction.FLAG_CONTROL, "memory", 3, 3)};
		engine.Start({sequence}, {flag}, SCR_EGameModeState.GAME);
		engine.SendTestEvent("sequence");
		Check(engine.GetFlagCallCount("memory") == 4, "Queued arithmetic executes in authored order");

		GC_CompiledMissionRule controlled = EventRule("controlled");
		controlled.m_bEnabled = false;
		engine.Start({controlled}, null, SCR_EGameModeState.GAME);
		engine.QueueFrameworkAction(Command(GC_ERuleAction.EVENT_CONTROL, "controlled", 3));
		Check(engine.GetRunCount() == 0, "Check now respects disabled state");
		engine.QueueFrameworkAction(Command(GC_ERuleAction.EVENT_CONTROL, "controlled", 0));
		Check(engine.GetRunCount() == 0, "Enable does not execute actions");
		engine.QueueFrameworkAction(Command(GC_ERuleAction.EVENT_CONTROL, "controlled", 3));
		Check(engine.GetEventResult("controlled") && engine.GetRunCount() == 1, "Check now runs an enabled eligible event");
		engine.QueueFrameworkAction(Command(GC_ERuleAction.EVENT_CONTROL, "controlled", 1));
		Check(engine.GetEventResult("controlled"), "Disable preserves run memory");
		engine.QueueFrameworkAction(Command(GC_ERuleAction.EVENT_CONTROL, "controlled", 2));
		Check(!engine.GetEventResult("controlled") && engine.GetFlag("controlled"), "Reset clears event memory without resetting flags");
		engine.QueueFrameworkAction(Command(GC_ERuleAction.EVENT_CONTROL, "controlled", 3));
		Check(engine.GetRunCount() == 1, "Reset does not enable an event");

		GC_CompiledMissionRule legacy = EventRule("legacy observer");
		legacy.m_bEnabled = false;
		legacy.m_eWhen = GC_ERuleEvent.FLAG_ON;
		legacy.m_sTestEventName = "legacy trigger";
		engine.Start({legacy}, null, SCR_EGameModeState.GAME);
		engine.QueueFrameworkAction(Command(GC_ERuleAction.EVENT_CONTROL, "legacy observer", 0));
		engine.SetFlag("legacy trigger", true);
		Check(engine.GetEventResult("legacy observer"), "Enabled authored-disabled legacy flag observer receives changes");

		GC_CompiledMissionRule loop = EventRule("self");
		loop.m_aThen = {Command(GC_ERuleAction.EVENT_CONTROL, "self", 2), Command(GC_ERuleAction.EVENT_CONTROL, "self", 3)};
		engine.Start({loop}, null, SCR_EGameModeState.GAME);
		engine.SendTestEvent("self");
		Check(engine.GetRunCount() == 1 && !engine.EventLimitReached(), "Reset and Check cannot bypass per-chain run guard");

		GC_RandomChanceCondition random = new GC_RandomChanceCondition();
		random.m_fCheckInterval = 10;
		random.m_fChance = 1;
		random.m_bRollOnce = true;
		random.m_sResetName = "chance";
		GC_CompiledMissionRule scheduled = EventRule("scheduled", false);
		scheduled.m_If = random.BuildCheck();
		engine.Start({scheduled}, null, SCR_EGameModeState.GAME);
		engine.Advance(10);
		engine.Advance(60);
		Check(engine.GetRunCount() == 1, "Roll once holds its result");
		engine.QueueFrameworkAction(Command(GC_ERuleAction.RESET_RANDOM, "chance"));
		engine.Advance(69);
		Check(engine.GetRunCount() == 1, "Reset waits a full new interval");
		engine.Advance(70);
		Check(engine.GetRunCount() == 2, "Reset allows Roll Once to roll again");
		GC_RuleAction invalid = Command(GC_ERuleAction.FLAG_CONTROL, "memory", 2, -1);
		Check(!invalid.IsValid() && !engine.QueueFrameworkAction(invalid), "Negative amounts rejected before queueing");
		Check(!Command(GC_ERuleAction.MISSION_MESSAGE, "<tag>").IsValid(), "Message markup rejected");
		Check(Command(GC_ERuleAction.END_MISSION, "", 0).IsValid(), "Neutral ending needs no faction");
		Check(!Command(GC_ERuleAction.END_MISSION, "", 1).IsValid(), "Faction result requires a faction");
		engine.Stop();
		return string.Format("FrameworkActions: checks=%1 failures=%2", m_iChecks, m_iFailures);
	}
}

#ifdef WORKBENCH
modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestFrameworkActions()
	{
		GC_FrameworkActionTests tests = new GC_FrameworkActionTests();
		return tests.RunActions();
	}
}
#endif
