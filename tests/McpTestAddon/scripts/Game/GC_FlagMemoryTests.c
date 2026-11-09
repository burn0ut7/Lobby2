//! Explicit fixtures: no automatic tests or global gameplay overrides.
class GC_FlagMemoryTests : GC_MissionRuleTests
{
	static GC_MissionFlag Memory(string name, bool repeat = true)
	{
		GC_MissionFlag flag = new GC_MissionFlag();
		flag.m_sName = name;
		flag.m_bRepeatable = repeat;
		return flag;
	}

	static GC_MissionFlagCondition On(string name)
	{
		GC_MissionFlagCondition check = new GC_MissionFlagCondition();
		check.m_sFlagName = name;
		return check;
	}

	static GC_ConditionCount Calls(string name, int comparison, int count)
	{
		GC_ConditionCount check = new GC_ConditionCount();
		check.m_sFlagName = name;
		check.m_iComparison = comparison;
		check.m_iRequiredCount = count;
		check.m_iCountFrom = 1;
		return check;
	}

	static GC_Event Watch(string name, float wait = 0)
	{
		GC_Event missionEvent = new GC_Event();
		missionEvent.m_sName = name;
		missionEvent.m_aConditions = {On(name)};
		GC_LogMessageAction log = new GC_LogMessageAction();
		log.m_sMessage = "Flag memory test";
		missionEvent.m_aActions = {log};
		missionEvent.m_Advanced = new GC_RuleOptions();
		missionEvent.m_Advanced.m_fRandomChance = 1;
		missionEvent.m_Advanced.m_iRun = 1;
		missionEvent.m_Advanced.m_fDelay = wait;
		return missionEvent;
	}

	string RunSerializedMemory()
	{
		Resource resource = Resource.Load("{71620F377D834170}Prefabs/FlagMemoryExample.et");
		if (!resource || !resource.IsValid())
			return "FAIL: flag fixture load";
		BaseContainerList components = resource.GetResource().ToBaseContainer().GetObjectArray("components");
		array<ref GC_MissionFlag> flags = {};
		array<ref GC_CompiledMissionRule> events = {};
		for (int i; i < components.Count(); i++)
		{
			BaseContainer component = components.Get(i);
			if (component.GetClassName() != "GC_LobbyManager")
				continue;
			BaseContainerList savedFlags = component.GetObjectArray("m_aMissionFlags");
			for (int f; f < savedFlags.Count(); f++)
				flags.Insert(GC_MissionFlag.Cast(BaseContainerTools.CreateInstanceFromContainer(savedFlags.Get(f))));
			BaseContainerList savedEvents = component.GetObjectArray("m_aMissionEvents");
			for (int e; e < savedEvents.Count(); e++)
			{
				GC_EventDefinition definition = GC_EventDefinition.Cast(BaseContainerTools.CreateInstanceFromContainer(savedEvents.Get(e)));
				events.Insert(definition.BuildRule());
			}
		}
		if (flags.Count() != 2 || events.Count() != 1 || !flags[0] || !flags[1] || !events[0])
			return "FAIL: flag fixture deserialize";
		Check(flags[0].m_bRepeatable && !flags[1].m_bRepeatable, "saved repeatable and default nonrepeatable");
		GC_MissionRuleEngine engine = new GC_MissionRuleEngine();
		engine.Start(events, flags, SCR_EGameModeState.GAME);
		for (int n; n < 6; n++)
			engine.CallFlag("wave");
		Check(engine.GetRunCount() == 5 && engine.GetFlagCallCount("wave") == 5 && !engine.GetFlag("wave"), "serialized defaults less than five and full event chance");
		engine.CallFlag("remember");
		Check(engine.GetFlag("remember") && !engine.CallFlag("remember"), "serialized default nonrepeatable memory");
		return string.Format("FlagMemoryResource: checks=%1 failures=%2", m_iChecks, m_iFailures);
	}

	string RunMemory()
	{
		GC_MissionRuleEngine engine = new GC_MissionRuleEngine();
		GC_MissionFlag limited = Memory("wave");
		limited.m_aConditions = {Calls("wave", 1, 5)};
		GC_Event watch = Watch("wave");
		engine.Start({watch.BuildRule()}, {limited}, SCR_EGameModeState.GAME);
		Check(engine.GetRunCount() == 0 && engine.GetFlagCallCount("wave") == 0, "self count limits do not self activate");
		for (int i = 1; i <= 5; i++)
		{
			Check(engine.CallFlag("wave"), "call accepted below limit");
			Check(engine.GetFlagCallCount("wave") == i && engine.GetRunCount() == i, "one count and action per call");
			Check(!engine.GetFlag("wave"), "repeatable pulse resets after observers");
		}
		Check(!engine.CallFlag("wave") && engine.GetFlagCallCount("wave") == 5 && engine.GetRunCount() == 5, "sixth call rejected without count or actions");
		Check(!engine.CallFlag("missing"), "unknown call rejected");
		engine.Advance(100);
		Check(engine.GetRunCount() == 5, "no automatic per-tick calls");
		engine.Stop();
		Check(!engine.CallFlag("wave") && engine.GetFlagCallCount("wave") == 5, "stopped rejects and retains diagnostic count");
		engine.Start({watch.BuildRule()}, {limited}, SCR_EGameModeState.GAME);
		Check(engine.GetFlagCallCount("wave") == 0, "new start resets count");

		GC_MissionFlag once = Memory("once", false);
		engine.Start({Watch("once").BuildRule()}, {once}, SCR_EGameModeState.GAME);
		Check(engine.CallFlag("once") && engine.GetFlag("once") && engine.GetFlagCallCount("once") == 1, "nonrepeatable remains on");
		Check(!engine.CallFlag("once") && engine.GetRunCount() == 1, "nonrepeatable rejects next call");
		engine.SetFlag("once", false);
		Check(!engine.CallFlag("once") && engine.GetFlagCallCount("once") == 1, "clearing state does not reset once memory");
		once.m_iStartingValue = 1;
		engine.Start({}, {once}, SCR_EGameModeState.GAME);
		Check(engine.GetFlag("once") && !engine.CallFlag("once") && engine.GetFlagCallCount("once") == 0, "starting on is memory not a counted call");

		GC_MissionFlag first = Memory("first");
		GC_MissionFlag second = Memory("second");
		second.m_aConditions = {On("first")};
		GC_Event both = Watch("first");
		both.m_aConditions = {On("first"), On("second")};
		engine.Start({both.BuildRule()}, {first, second}, SCR_EGameModeState.GAME);
		engine.CallFlag("first");
		Check(engine.GetFlagCallCount("first") == 1 && engine.GetFlagCallCount("second") == 1 && engine.GetRunCount() == 1, "automatic chain sees both pulses");
		Check(!engine.GetFlag("first") && !engine.GetFlag("second"), "whole chain resets pulses");
		engine.CallFlag("first");
		Check(engine.GetRunCount() == 2 && engine.GetFlagCallCount("second") == 2, "later external call starts fresh chain");

		GC_CallFlagAction again = new GC_CallFlagAction();
		again.m_sFlagName = "first";
		GC_Event loop = Watch("first");
		loop.m_aActions = {again, again};
		engine.Start({loop.BuildRule()}, {first}, SCR_EGameModeState.GAME);
		engine.CallFlag("first");
		Check(engine.IsRunning() && !engine.EventLimitReached() && engine.GetFlagCallCount("first") == 1 && engine.GetRunCount() == 1, "self-call duplicates suppressed per chain");
		engine.CallFlag("first");
		Check(engine.GetFlagCallCount("first") == 2, "suppression released after chain");

		GC_ConditionGroup any = new GC_ConditionGroup();
		any.m_iMatch = 1;
		any.m_aConditions = {On("first"), On("second")};
		GC_Event either = Watch("first");
		either.m_aConditions = {any};
		engine.Start({either.BuildRule()}, {first, second}, SCR_EGameModeState.GAME);
		engine.CallFlag("first");
		Check(engine.GetRunCount() == 1, "multiple changed dependencies yield one eligible missionEvent attempt per chain");

		watch = Watch("wave", 2);
		engine.Start({watch.BuildRule()}, {limited}, SCR_EGameModeState.GAME);
		engine.CallFlag("wave");
		engine.Advance(2);
		Check(engine.GetRunCount() == 0, "delayed On check does not keep expired pulse alive");
		watch.m_aConditions = {Calls("wave", 0, 0)};
		engine.Start({watch.BuildRule()}, {limited}, SCR_EGameModeState.GAME);
		engine.CallFlag("wave");
		engine.Advance(2);
		Check(engine.GetRunCount() == 1, "remembered count supports delayed conditions");

		watch = Watch("wave");
		watch.m_Advanced.m_fRandomChance = 0;
		engine.Start({watch.BuildRule()}, {limited}, SCR_EGameModeState.GAME);
		engine.CallFlag("wave");
		Check(engine.GetRunCount() == 0 && engine.GetFlagCallCount("wave") == 1 && !engine.GetFlag("wave"), "missionEvent chance does not undo accepted flag call");
		engine.SetFlag("wave", true);
		Check(engine.GetFlagCallCount("wave") == 2, "Set On routes declared flag through call policy");

		GC_CompiledMissionRule ordered = EventRule("ordered");
		ordered.m_aThen = {FlagAction("ordered_flag", true), FlagAction("ordered_flag", false)};
		GC_MissionFlag orderedFlag = Memory("ordered_flag", false);
		engine.Start({ordered}, {orderedFlag}, SCR_EGameModeState.GAME);
		engine.SendTestEvent("ordered");
		Check(!engine.GetFlag("ordered_flag") && engine.GetFlagCallCount("ordered_flag") == 1, "declared On then Off preserves action order");
		ordered.m_aThen = {FlagAction("ordered_flag", false), FlagAction("ordered_flag", true)};
		engine.Start({ordered}, {orderedFlag}, SCR_EGameModeState.GAME);
		engine.SendTestEvent("ordered");
		Check(engine.GetFlag("ordered_flag") && engine.GetFlagCallCount("ordered_flag") == 1, "declared Off then On preserves action order");
		ordered.m_aThen = {FlagAction("first", true), FlagAction("second", true)};
		engine.Start({ordered}, {first, second}, SCR_EGameModeState.GAME);
		engine.SendTestEvent("ordered");
		Check(engine.GetFlagCallCount("first") == 1 && engine.GetFlagCallCount("second") == 1, "later queued call sees earlier accepted call");

		map<string, int> counts = new map<string, int>();
		counts.Set("wave", 5);
		for (int comparison; comparison < 4; comparison++)
		{
			for (int number = 4; number <= 6; number++)
			{
				bool expected;
				if (comparison == 0)
					expected = 5 > number;
				if (comparison == 1)
					expected = 5 < number;
				if (comparison == 2)
					expected = 5 == number;
				if (comparison == 3)
					expected = 5 != number;
				Check(Calls("wave", comparison, number).BuildCheck().Evaluate(null, SCR_EGameModeState.GAME, counts) == expected, "count comparison boundaries");
			}
		}
		GC_ConditionCount invalid = Calls("wave", 4, 5);
		Check(!invalid.BuildCheck(), "invalid comparison rejected");
		invalid = Calls("wave", 1, -1);
		Check(!invalid.BuildCheck(), "negative count rejected");
		invalid = Calls("", 1, 5);
		Check(!invalid.BuildCheck(), "empty count name rejected");
		GC_ConditionGroup malformed = new GC_ConditionGroup();
		limited.m_aConditions = {malformed};
		engine.Start({}, {limited}, SCR_EGameModeState.GAME);
		Check(!engine.IsRunning() && !engine.CallFlag("wave"), "malformed flag condition fails startup");
		return string.Format("FlagMemory: checks=%1 failures=%2", m_iChecks, m_iFailures);
	}
}

modded class GC_LobbyManager
{
	string GC_TestFlagMemory()
	{
		GC_FlagMemoryTests tests = new GC_FlagMemoryTests();
		return tests.RunMemory();
	}

	string GC_TestFlagMemoryResource()
	{
		GC_FlagMemoryTests tests = new GC_FlagMemoryTests();
		return tests.RunSerializedMemory();
	}

	string GC_TestFlagAuthority()
	{
		if (!HasMissionRuleAuthority())
			return "FAIL: test needs active authority";
		StopMissionRules();
		m_bMissionRulesStarted = false;
		m_bMissionEventsEnabled = true;
		m_aMissionFlags = {GC_FlagMemoryTests.Memory("authority")};
		m_aMissionEvents = {GC_FlagMemoryTests.Watch("authority")};
		StartMissionRules();
		if (!CallFlag("authority") || GetFlagCallCount("authority") != 1 || GetRunCount() != 1 || GetFlag("authority"))
			return "FAIL: production call/count/pulse";
		m_bMissionEventsEnabled = false;
		bool disabledAccepted = CallFlag("authority");
		m_bMissionEventsEnabled = true;
		m_bShutdown = true;
		bool shutdownAccepted = CallFlag("authority");
		m_bShutdown = false;
		StopMissionRules();
		if (disabledAccepted || shutdownAccepted || CallFlag("authority") || GetFlagCallCount("authority") != 1)
			return "FAIL: disabled/shutdown/stopped call accepted";
		return "PASS: manager call, count, pulse, disabled/shutdown/stopped rejection";
	}
}
