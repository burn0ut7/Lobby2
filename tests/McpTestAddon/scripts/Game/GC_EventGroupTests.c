modded class GC_MissionRuleTests
{
	//------------------------------------------------------------------------------------------------
	static GC_RandomEventGroup RandomGroup(array<ref GC_Event> children, int picks = 1, bool repeat = false)
	{
		GC_RandomEventGroup group = new GC_RandomEventGroup();
		group.m_sName = "Selection test";
		group.m_iPickCount = picks;
		group.m_aEvents = children;
		GC_GameModeStateCondition match = new GC_GameModeStateCondition();
		match.m_eGameModeState = SCR_EGameModeState.GAME;
		group.m_aConditions = {match};
		group.m_Advanced = new GC_RuleOptions();
		group.m_Advanced.m_fRandomChance = 1;
		if (repeat)
			group.m_Advanced.m_bRepeatable = true;
		return group;
	}

	//------------------------------------------------------------------------------------------------
	string RunEventGroups()
	{
		GC_MissionRandomTestEngine engine = new GC_MissionRandomTestEngine();
		GC_Event basic = SimpleRule("chance");
		Check(basic.BuildRule().m_fChance == 1, "missing Advanced defaults to full chance");
		basic.m_Advanced = new GC_RuleOptions();
		basic.m_Advanced.m_fRandomChance = 0;
		engine.m_fRoll = 0;
		engine.Start({basic.BuildRule()});
		Check(engine.GetRunCount() == 0, "zero float chance never runs");
		basic.m_Advanced.m_fRandomChance = 0.8;
		engine.m_fRoll = 0.79;
		engine.Start({basic.BuildRule()});
		Check(engine.GetFlag("chance"), "0.8 accepts a draw below 80 percent");
		engine.m_fRoll = 0.8;
		engine.Start({basic.BuildRule()});
		Check(!engine.GetFlag("chance"), "0.8 rejects the upper boundary");
		basic.m_Advanced.m_fRandomChance = 1;
		engine.m_fRoll = 1;
		engine.Start({basic.BuildRule()});
		Check(engine.GetFlag("chance"), "one always runs even at upper draw endpoint");
		basic.m_Advanced.m_fRandomChance = -0.1;
		Check(!basic.BuildRule(), "negative chance rejected");
		basic.m_Advanced.m_fRandomChance = 1.1;
		Check(!basic.BuildRule(), "chance above one rejected");

		GC_Event a = StateRule("A", SCR_EGameModeState.GAME);
		GC_Event b = StateRule("B", SCR_EGameModeState.GAME);
		GC_Event c = StateRule("C", SCR_EGameModeState.GAME);
		GC_RandomEventGroup group = RandomGroup({a, b, c}, 2);
		engine.m_fRoll = 0;
		engine.Start({group.BuildRule()}, null, SCR_EGameModeState.GAME);
		Check(engine.GetRunCount() == 2 && engine.GetFlag("A") && engine.GetFlag("B") && !engine.GetFlag("C"), "pick two distinct regular events");
		Check(group.m_aEvents.Count() == 3 && group.m_aEvents[0] == a, "selection preserves authored list");
		engine.SetGameModeState(SCR_EGameModeState.BRIEFING);
		engine.SetGameModeState(SCR_EGameModeState.GAME);
		Check(engine.GetRunCount() == 2 && !engine.GetFlag("C"), "once-only group does not pick again");
		engine.m_fRoll = 1;
		engine.Start({group.BuildRule()}, null, SCR_EGameModeState.GAME);
		Check(!engine.GetFlag("A") && engine.GetFlag("B") && engine.GetFlag("C"), "upper random endpoint selects safely without duplicates");

		GC_GameModeStateCondition briefing = new GC_GameModeStateCondition();
		briefing.m_eGameModeState = SCR_EGameModeState.BRIEFING;
		b.m_aConditions = {briefing};
		engine.m_fRoll = 0;
		engine.Start({group.BuildRule()}, null, SCR_EGameModeState.GAME);
		Check(engine.GetFlag("A") && !engine.GetFlag("B") && engine.GetFlag("C"), "failing child conditions excluded before selection");
		c.m_Advanced.m_bDisabled = true;
		engine.Start({group.BuildRule()}, null, SCR_EGameModeState.GAME);
		Check(engine.GetRunCount() == 1 && engine.GetFlag("A"), "fewer eligible than requested selects available only");
		a.m_Advanced.m_fRandomChance = 0;
		engine.Start({group.BuildRule()}, null, SCR_EGameModeState.GAME);
		Check(engine.GetRunCount() == 0, "selected child's chance still applies");
		a.m_Advanced.m_fRandomChance = 1;
		group.m_Advanced.m_fRandomChance = 0;
		engine.Start({group.BuildRule()}, null, SCR_EGameModeState.GAME);
		Check(engine.GetRunCount() == 0, "group chance gates whole selection");
		group.m_Advanced.m_fRandomChance = 1;
		engine.Start({group.BuildRule()}, null, SCR_EGameModeState.BRIEFING);
		Check(engine.GetRunCount() == 0, "matching child cannot bypass group condition");

		b = StateRule("B", SCR_EGameModeState.GAME);
		group = RandomGroup({a, b}, 1, true);
		engine.Start({group.BuildRule()}, null, SCR_EGameModeState.GAME);
		engine.SetGameModeState(SCR_EGameModeState.BRIEFING);
		engine.SetGameModeState(SCR_EGameModeState.GAME);
		Check(engine.GetRunCount() == 2 && engine.GetFlag("A") && engine.GetFlag("B"), "repeat group excludes completed once-only children");

		GC_Event later = StateRule("later", SCR_EGameModeState.BRIEFING);
		group = RandomGroup({later});
		GC_GameModeStateCondition notPreview = new GC_GameModeStateCondition();
		notPreview.m_eGameModeState = SCR_EGameModeState.PREGAME;
		notPreview.m_bRequireOtherState = true;
		group.m_aConditions = {notPreview};
		engine.Start({group.BuildRule()}, null, SCR_EGameModeState.SLOTTING);
		Check(engine.GetRunCount() == 0, "empty eligible pool runs nothing");
		engine.SetGameModeState(SCR_EGameModeState.BRIEFING);
		Check(engine.GetFlag("later"), "empty pool does not consume once-only selection");

		a = StateRule("delayed", SCR_EGameModeState.GAME, false, false, 5);
		group = RandomGroup({a});
		engine.Start({group.BuildRule()}, null, SCR_EGameModeState.GAME);
		engine.Advance(4);
		Check(!engine.GetFlag("delayed"), "selected child's delay honored");
		engine.Advance(5);
		Check(engine.GetFlag("delayed"), "selected child's delay completes");
		a.m_aConditions = {};
		engine.Start({group.BuildRule()}, null, SCR_EGameModeState.GAME);
		engine.SetGameModeState(SCR_EGameModeState.POSTGAME, 2);
		engine.Advance(10);
		Check(!engine.GetFlag("delayed"), "child delay still obeys parent state condition");
		group.m_Advanced.m_fDelay = 2;
		a.m_Advanced.m_fDelay = 0;
		engine.Start({group.BuildRule()}, null, SCR_EGameModeState.GAME);
		engine.Advance(1);
		Check(engine.GetRunCount() == 0, "group delay postpones selection");
		engine.Advance(2);
		Check(engine.GetFlag("delayed"), "group delay completes before child runs");
		engine.Start({group.BuildRule()}, null, SCR_EGameModeState.GAME);
		engine.Stop();
		engine.Advance(10);
		Check(engine.GetRunCount() == 0, "stop cancels group wait");

		a.m_Advanced.m_fDelay = 5;
		group = RandomGroup({a});
		group.m_aConditions = {};
		engine.Start({group.BuildRule()});
		engine.SetGameModeState(SCR_EGameModeState.POSTGAME, 2);
		engine.Advance(10);
		Check(!engine.GetFlag("delayed"), "default match-start group's child wait ends with match");

		group.m_iPickCount = 0;
		Check(!group.BuildRule(), "zero picks rejected");
		group.m_iPickCount = 2;
		Check(!group.BuildRule(), "more picks than configured children rejected");
		group.m_iPickCount = 1;
		group.m_aEvents = {null};
		Check(!group.BuildRule(), "missing child rejected");
		group.m_aEvents = {};
		Check(!group.BuildRule(), "empty group rejected");
		return string.Format("EventGroups: checks=%1 failures=%2", m_iChecks, m_iFailures);
	}
}

modded class GC_LobbyManager
{
	string GC_TestEventGroups()
	{
		GC_MissionRuleTests tests = new GC_MissionRuleTests();
		return tests.RunEventGroups();
	}
}
