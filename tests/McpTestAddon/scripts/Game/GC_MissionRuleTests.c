//! Explicit tests only. Loading this addon never starts or configures rules.
class GC_MissionRuleTests
{
	protected int m_iChecks;
	protected int m_iFailures;

	//------------------------------------------------------------------------------------------------
	protected void Check(bool passed, string label)
	{
		m_iChecks++;
		if (passed)
			return;
		m_iFailures++;
		PrintFormat("GC | Mission rule test failed | %1", label);
	}

	//------------------------------------------------------------------------------------------------
	static GC_RuleCondition Answer(bool answer)
	{
		GC_RuleCondition check = new GC_RuleCondition();
		check.m_iCheck = 2;
		check.m_bTestAnswer = answer;
		return check;
	}

	//------------------------------------------------------------------------------------------------
	static GC_RuleCondition Group()
	{
		GC_RuleCondition group = new GC_RuleCondition();
		group.m_iHowMany = 1;
		group.m_aChecks = {};
		return group;
	}

	//------------------------------------------------------------------------------------------------
	static GC_RuleAction FlagAction(string name, bool answer = true)
	{
		GC_RuleAction action = new GC_RuleAction();
		action.m_eDo = GC_ERuleAction.SET_FLAG;
		action.m_sTextOrName = name;
		action.m_bFlagAnswer = answer;
		return action;
	}

	//------------------------------------------------------------------------------------------------
	static GC_MissionRule EventRule(string name, bool once = true, float delay = 0)
	{
		GC_MissionRule rule = new GC_MissionRule();
		rule.m_sName = name;
		rule.m_bEnabled = true;
		rule.m_fEverySeconds = 5;
		rule.m_aThen = {};
		rule.m_eWhen = GC_ERuleEvent.TEST_EVENT;
		rule.m_sTestEventName = name;
		rule.m_bRunOnlyOnce = once;
		rule.m_fWaitSeconds = delay;
		rule.m_aThen.Insert(FlagAction(name));
		return rule;
	}

	//------------------------------------------------------------------------------------------------
	protected void CheckConditions()
	{
		// Truth-table bitsets for all eight three-answer combinations, threshold 2.
		array<int> expected = {128, 254, 1, 104, 232, 127, 129};
		array<bool> empty = {true, false, true, false, false, true, true};
		ref map<string, bool> flags = new map<string, bool>();
		for (int mode; mode < 7; mode++)
		{
			GC_RuleCondition group = Group();
			group.m_eGroupNeeds = mode;
			group.m_iHowMany = 2;
			Check(group.Evaluate(flags) == empty[mode], "empty group");
			for (int mask; mask < 8; mask++)
			{
				group.m_aChecks.Clear();
				for (int bit; bit < 3; bit++)
					group.m_aChecks.Insert(Answer((mask & (1 << bit)) != 0));
				bool wanted = (expected[mode] & (1 << mask)) != 0;
				group.m_bReverseAnswer = false;
				Check(group.Evaluate(flags) == wanted, "three-answer truth table");
				group.m_bReverseAnswer = true;
				Check(group.Evaluate(flags) == !wanted, "reversed truth table");
			}
		}

		GC_RuleCondition flag = new GC_RuleCondition();
		flag.m_iCheck = 1;
		flag.m_sFlagName = "ready";
		GC_RuleCondition nested = Group();
		nested.m_aChecks.Insert(flag);
		GC_RuleCondition any = Group();
		any.m_eGroupNeeds = GC_ERuleCheck.ANY;
		any.m_aChecks.Insert(Answer(false));
		any.m_aChecks.Insert(Answer(true));
		nested.m_aChecks.Insert(any);
		Check(!nested.Evaluate(flags), "missing flag is no");
		flags.Set("ready", true);
		Check(nested.Evaluate(flags), "nested all and any");
		flags.Set("ready", false);
		flag.m_bReverseAnswer = true;
		Check(nested.Evaluate(flags), "inverted false flag");
		flag.m_sFlagName = "";
		Check(!nested.Evaluate(flags), "invalid inverted flag fails closed");
		nested.m_aChecks.Insert(null);
		nested.m_bReverseAnswer = true;
		Check(!nested.Evaluate(flags), "invalid reversed group fails closed");
		GC_RuleCondition cycle = Group();
		cycle.m_aChecks.Insert(cycle);
		Check(!cycle.Evaluate(flags), "cycle depth guard");
		cycle.m_aChecks.Clear();
		GC_RuleCondition zero = Group();
		zero.m_eGroupNeeds = GC_ERuleCheck.AT_LEAST;
		zero.m_iHowMany = 0;
		Check(zero.Evaluate(flags), "at least zero with no checks");
		zero.m_eGroupNeeds = GC_ERuleCheck.EXACTLY;
		Check(zero.Evaluate(flags), "exactly zero with no checks");
	}

	//------------------------------------------------------------------------------------------------
	protected void CheckEvents()
	{
		GC_MissionRuleEngine engine = new GC_MissionRuleEngine();
		GC_MissionRule rule = EventRule("finish");
		rule.m_If = new GC_RuleCondition();
		rule.m_If.m_iCheck = 1;
		rule.m_If.m_sFlagName = "ready";
		engine.Start({rule});
		engine.SendTestEvent("finish");
		Check(engine.GetRunCount() == 0, "event before condition passes");
		engine.SetFlag("ready", true);
		engine.SendTestEvent("wrong");
		Check(engine.GetRunCount() == 0, "event name must match");
		engine.SendTestEvent("finish");
		engine.SendTestEvent("finish");
		Check(engine.GetRunCount() == 1 && engine.GetFlag("finish"), "one-time rule");
		rule.m_bRunOnlyOnce = false;
		engine.SendTestEvent("finish");
		Check(engine.GetRunCount() == 2, "repeatable rule");
		rule.m_bEnabled = false;
		engine.SendTestEvent("finish");
		Check(engine.GetRunCount() == 2, "disabled rule");
		engine.Stop();
		Check(!engine.SendTestEvent("finish") && !engine.SetFlag("late", true), "stopped engine rejects writes");
		engine.Start({});
		Check(!engine.GetFlag("finish") && engine.GetRunCount() == 0, "fresh mission resets state");

		GC_MissionRule delayed = EventRule("delayed", true, 2);
		delayed.m_If = Answer(true);
		engine.Start({delayed});
		engine.SendTestEvent("delayed");
		engine.SendTestEvent("delayed");
		engine.Advance(1.9);
		Check(engine.GetRunCount() == 0, "wait before action");
		delayed.m_If.m_bTestAnswer = false;
		engine.Advance(2);
		Check(engine.GetRunCount() == 0, "condition rechecked after wait");
		delayed.m_If.m_bTestAnswer = true;
		engine.SendTestEvent("delayed");
		engine.Advance(4);
		Check(engine.GetRunCount() == 1, "failed delayed check can retry");
		engine.Advance(100);
		Check(engine.GetRunCount() == 1, "pending events do not stack");
		engine.Start({delayed});
		engine.SendTestEvent("delayed");
		engine.Stop();
		engine.Advance(100);
		Check(engine.GetRunCount() == 0, "stop cancels delayed action");

		GC_MissionRule timer = EventRule("timer", false);
		timer.m_eWhen = GC_ERuleEvent.TIMER;
		timer.m_fEverySeconds = 1;
		engine.Start({timer});
		engine.Advance(0.9);
		Check(engine.GetRunCount() == 0, "timer waits first interval");
		engine.Advance(1);
		engine.Advance(20);
		Check(engine.GetRunCount() == 2, "timer does not burst after a stall");
		engine.Advance(19);
		Check(engine.GetRunCount() == 2, "backward clock rejected");

		GC_MissionRule start = EventRule("start");
		start.m_eWhen = GC_ERuleEvent.MATCH_BEGINS;
		GC_RuleAction send = new GC_RuleAction();
		send.m_eDo = GC_ERuleAction.SEND_TEST_EVENT;
		send.m_sTextOrName = "next";
		start.m_aThen.Insert(send);
		GC_MissionRule next = EventRule("next");
		engine.Start({start, next});
		Check(engine.GetRunCount() == 2 && engine.GetFlag("start") && engine.GetFlag("next"), "start and chained action");
		Check(!engine.SendTestEvent(""), "reserved start event cannot be sent");
		GC_MissionRule invalid = EventRule("invalid");
		invalid.m_aThen.Insert(null);
		engine.Start({invalid, next});
		engine.SendTestEvent("invalid");
		engine.SendTestEvent("next");
		Check(engine.GetInvalidRuleCount() == 1 && !engine.GetFlag("invalid") && engine.GetFlag("next"), "invalid rule has no partial actions");
		GC_MissionRule loop = EventRule("loop");
		send = new GC_RuleAction();
		send.m_eDo = GC_ERuleAction.SEND_TEST_EVENT;
		send.m_sTextOrName = "loop";
		loop.m_aThen.Insert(send);
		engine.Start({loop});
		engine.SendTestEvent("loop");
		Check(engine.GetRunCount() == 1 && engine.IsRunning(), "once committed before recursive event");
		loop.m_bRunOnlyOnce = false;
		engine.Start({loop});
		engine.SendTestEvent("loop");
		Check(engine.EventLimitReached() && !engine.IsRunning() && engine.GetRunCount() == 256, "event loop stops safely");
		array<ref GC_MissionRule> oversized = {};
		for (int index; index < 257; index++)
			oversized.Insert(next);
		engine.Start(oversized);
		Check(!engine.IsRunning() && engine.GetInvalidRuleCount() == 1 && engine.GetRunCount() == 0, "oversized rule list rejected before execution");
	}

	//------------------------------------------------------------------------------------------------
	string Run()
	{
		CheckConditions();
		CheckEvents();
		return string.Format("MissionRules: checks=%1 failures=%2", m_iChecks, m_iFailures);
	}

	//------------------------------------------------------------------------------------------------
	//! Read the actual authored example without spawning a second game mode.
	string RunExample()
	{
		Resource resource = Resource.Load("{7B115D5DFF7ED995}Prefabs/MP/Modes/GameMode_Lobby_RulesExample.et");
		if (!resource || !resource.IsValid())
			return "FAIL: example resource could not load";
		BaseContainer root = resource.GetResource().ToBaseContainer();
		if (!root)
			return "FAIL: example root missing";
		BaseContainerList components = root.GetObjectArray("components");
		if (!components)
			return "FAIL: example components missing";
		BaseContainerList authoredRules;
		for (int index; index < components.Count(); index++)
		{
			BaseContainer component = components.Get(index);
			if (component.GetClassName() == "GC_LobbyManager")
				authoredRules = component.GetObjectArray("m_aMissionRules");
		}
		if (!authoredRules || authoredRules.Count() != 3)
			return "FAIL: expected three authored rules";
		array<ref GC_MissionRule> rules = {};
		for (int index; index < authoredRules.Count(); index++)
		{
			GC_MissionRule rule = GC_MissionRule.Cast(BaseContainerTools.CreateInstanceFromContainer(authoredRules.Get(index)));
			if (!rule)
				return "FAIL: authored rule could not deserialize";
			rules.Insert(rule);
		}
		GC_MissionRuleEngine engine = new GC_MissionRuleEngine();
		engine.Start(rules);
		Check(engine.GetInvalidRuleCount() == 0, "example settings deserialize correctly");
		Check(engine.GetFlag("test_ready") && !engine.GetFlag("test_complete"), "example starts ready");
		engine.Advance(2);
		Check(!engine.GetFlag("test_complete"), "example waits after event");
		engine.Advance(3);
		Check(engine.GetFlag("test_complete") && engine.GetRunCount() == 3, "example nested rule completes");
		engine.Advance(4);
		engine.Advance(10);
		Check(engine.GetRunCount() == 5, "example completion stays once-only");
		engine.Stop();
		return string.Format("MissionRulesExample: checks=%1 failures=%2", m_iChecks, m_iFailures);
	}
}

modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	//! Runtime-only fixture; does not save or alter the authored rule list.
	string GC_TestMissionRulesPrepare()
	{
		if (!Replication.IsServer() || GC_LobbyGameMode.GetGameMode() != GetOwner() || m_bMissionRulesStarted || (m_aMissionRules && !m_aMissionRules.IsEmpty()))
			return "UNAVAILABLE: requires authority and empty rules before match start";
		m_aMissionRules = {};
		GC_MissionRule start = GC_MissionRuleTests.EventRule("live_start");
		start.m_eWhen = GC_ERuleEvent.MATCH_BEGINS;
		m_aMissionRules.Insert(start);
		GC_MissionRule pulse = GC_MissionRuleTests.EventRule("live_timer", false);
		pulse.m_eWhen = GC_ERuleEvent.TIMER;
		pulse.m_fEverySeconds = 0.5;
		m_aMissionRules.Insert(pulse);
		m_aMissionRules.Insert(GC_MissionRuleTests.EventRule("live_delay", true, 1));
		m_aMissionRules.Insert(GC_MissionRuleTests.EventRule("live_cancel", true, 10));
		return "prepared=4";
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestMissionRulesSnapshot()
	{
		bool running = m_MissionRuleEngine && m_MissionRuleEngine.IsRunning();
		return string.Format("server=%1 started=%2 running=%3 runs=%4 start=%5 timer=%6 delay=%7 cancel=%8", Replication.IsServer(), m_bMissionRulesStarted, running, GetRunCount(), GetFlag("live_start"), GetFlag("live_timer"), GetFlag("live_delay"), GetFlag("live_cancel"));
	}
	//------------------------------------------------------------------------------------------------
	string GC_TestMissionRules()
	{
		GC_MissionRuleTests tests = new GC_MissionRuleTests();
		return tests.Run();
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestMissionRulesExample()
	{
		GC_MissionRuleTests tests = new GC_MissionRuleTests();
		return tests.RunExample();
	}

}
