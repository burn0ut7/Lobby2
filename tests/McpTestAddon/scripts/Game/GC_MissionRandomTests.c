//! Deterministic draws exercise production chance/delay/selection handlers.
class GC_MissionRandomTestEngine : GC_MissionRuleEngine
{
	float m_fRoll;
	int m_iDraws;
	int m_iNamedEvents;

	override protected float RandomUnit()
	{
		m_iDraws++;
		return m_fRoll;
	}

	override bool SendTestEvent(string name)
	{
		m_iNamedEvents++;
		return super.SendTestEvent(name);
	}
}

modded class GC_MissionRuleTests
{
	string RunRandom()
	{
		GC_MissionRandomTestEngine engine = new GC_MissionRandomTestEngine();
		GC_CompiledMissionRule rule = EventRule("go", false);
		rule.m_Random = new GC_RuleRandom();
		rule.m_Random.m_fChancePercent = 0;
		engine.Start({rule});
		engine.SendTestEvent("go");
		Check(engine.GetRunCount() == 0 && engine.m_iDraws == 0, "zero chance never runs even with zero draw");
		rule.m_Random.m_fChancePercent = 100;
		engine.m_fRoll = 1;
		engine.SendTestEvent("go");
		Check(engine.GetRunCount() == 1, "100 percent always runs even at upper endpoint");
		rule.m_Random.m_fChancePercent = 50;
		engine.m_fRoll = 0.75;
		engine.SendTestEvent("go");
		engine.Advance(100);
		Check(engine.GetRunCount() == 1, "failed chance does not retry on ticks");
		engine.m_fRoll = 0.25;
		engine.SendTestEvent("go");
		Check(engine.GetRunCount() == 2, "next event can retry chance");
		rule.m_Random.m_fChancePercent = 100;
		rule.m_Random.m_fMinimumSeconds = 2;
		rule.m_Random.m_fMaximumSeconds = 6;
		rule.m_fWaitSeconds = 1;
		engine.m_fRoll = 0.5;
		engine.Start({rule});
		engine.SendTestEvent("go");
		int draws = engine.m_iDraws;
		engine.SendTestEvent("go");
		engine.Advance(4.9);
		Check(engine.GetRunCount() == 0 && engine.m_iDraws == draws, "pending delay ignores duplicate events and is sampled once");
		engine.Advance(5);
		Check(engine.GetRunCount() == 1, "fixed plus sampled delay runs at due time");
		engine.SendTestEvent("go");
		engine.Stop();
		engine.Advance(20);
		Check(engine.GetRunCount() == 1, "stop cancels random delay");
		rule.m_Random.m_fMaximumSeconds = 1;
		Check(!rule.IsValid(), "reversed random delay rejected");
		rule.m_Random = null;
		rule.m_fWaitSeconds = 0;
		GC_MissionFlag flag = new GC_MissionFlag();
		flag.m_sName = "initial";
		flag.m_iStartingValue = 1;
		GC_CompiledMissionRule watch = EventRule("initial");
		watch.m_eWhen = GC_ERuleEvent.FLAG_ON;
		engine.Start({watch}, {flag});
		Check(engine.GetFlag("initial") && engine.GetRunCount() == 0, "initial flags do not emit changes");
		engine.SetFlag("initial", false);
		engine.SetFlag("initial", true);
		Check(engine.GetRunCount() == 1, "later flag changes emit events");
		engine.Start({rule}, {flag, flag});
		Check(!engine.IsRunning() && !engine.GetFlag("initial"), "duplicate flag definitions fail without partial state");
		GC_RuleAction randomFlag = FlagAction("chance");
		randomFlag.m_eDo = GC_ERuleAction.RANDOM_FLAG;
		randomFlag.m_fChancePercent = 100;
		rule.m_aThen = {randomFlag};
		engine.Start({rule});
		engine.SendTestEvent("go");
		Check(engine.GetFlag("chance"), "random flag can turn On");
		randomFlag.m_fChancePercent = 0;
		engine.SendTestEvent("go");
		Check(!engine.GetFlag("chance"), "failed random flag resets Off");
		GC_RuleAction select = new GC_RuleAction();
		select.m_eDo = GC_ERuleAction.SELECT_FLAGS;
		select.m_aFlagNames = {"north", "east", "west"};
		select.m_iSelectionCount = 2;
		rule.m_aThen = {select};
		engine.m_fRoll = 1;
		engine.Start({rule});
		engine.SendTestEvent("go");
		Check(!engine.GetFlag("north") && engine.GetFlag("east") && engine.GetFlag("west"), "two distinct flags selected at upper draw endpoint");
		Check(select.m_aFlagNames.Count() == 3 && select.m_aFlagNames[0] == "north", "selection does not mutate configured pool");
		engine.m_fRoll = 0;
		engine.SendTestEvent("go");
		Check(engine.GetFlag("north") && engine.GetFlag("east") && !engine.GetFlag("west"), "new selection clears unselected flags");
		select.m_aFlagNames = {"same", "same"};
		Check(!select.IsValid(), "duplicate pool names rejected");
		select.m_aFlagNames = {"only"};
		Check(!select.IsValid(), "oversized selection rejected");
		// No flag definitions, conditions or flag actions are needed for random selection.
		GC_MissionRule noFlags = new GC_MissionRule();
		noFlags.m_sName = "No flags";
		GC_SendTestEventAction first = new GC_SendTestEventAction();
		first.m_sEventName = "first";
		GC_SendTestEventAction second = new GC_SendTestEventAction();
		second.m_sEventName = "second";
		noFlags.m_aActions = {first, second};
		noFlags.m_Random = new GC_RuleRandom();
		noFlags.m_Random.m_fChancePercent = 100;
		noFlags.m_Random.m_iActionSelection = 1;
		engine.m_iNamedEvents = 0;
		engine.Start({noFlags.BuildRule()});
		Check(engine.GetRunCount() == 1 && engine.m_iNamedEvents == 1, "random action selection works without flags");
		noFlags.m_Random = null;
		engine.m_iNamedEvents = 0;
		engine.Start({noFlags.BuildRule()});
		Check(engine.GetRunCount() == 1 && engine.m_iNamedEvents == 2, "default runs all actions without flags");
		engine.Stop();
		return string.Format("RandomRules: checks=%1 failures=%2", m_iChecks, m_iFailures);
	}
}

modded class GC_LobbyManager
{
	string GC_TestMissionRandom()
	{
		GC_MissionRuleTests tests = new GC_MissionRuleTests();
		return tests.RunRandom();
	}
}
