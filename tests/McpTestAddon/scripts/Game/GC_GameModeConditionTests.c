modded class GC_MissionRuleTests
{
	//------------------------------------------------------------------------------------------------
	static GC_MissionRule StateRule(string flag, SCR_EGameModeState phase, bool inverse = false, bool repeat = false, float wait = 0)
	{
		GC_MissionRule rule = new GC_MissionRule();
		rule.m_sName = flag;
		GC_SetFlagAction action = new GC_SetFlagAction();
		action.m_sFlagName = flag;
		action.m_iSetTo = 1;
		rule.m_aActions = {action};
		GC_GameModeStateCondition state = new GC_GameModeStateCondition();
		state.m_eGameModeState = phase;
		if (inverse)
			state.m_iMatch = 1;
		rule.m_aConditions = {state};
		rule.m_Advanced = new GC_RuleOptions();
		if (repeat)
			rule.m_Advanced.m_iRun = 1;
		rule.m_Advanced.m_fWaitSeconds = wait;
		return rule;
	}

	//------------------------------------------------------------------------------------------------
	string RunGameModeConditions()
	{
		array<SCR_EGameModeState> phases = {SCR_EGameModeState.PREGAME, SCR_EGameModeState.SLOTTING, SCR_EGameModeState.BRIEFING, SCR_EGameModeState.GAME, SCR_EGameModeState.POSTGAME};
		foreach (SCR_EGameModeState target : phases)
		{
			for (int inverse; inverse < 2; inverse++)
			{
				GC_GameModeStateCondition condition = new GC_GameModeStateCondition();
				condition.m_eGameModeState = target;
				condition.m_iMatch = inverse;
				GC_RuleCondition built = condition.BuildCheck();
				foreach (SCR_EGameModeState current : phases)
				{
					bool expected = current == target;
					if (inverse == 1)
						expected = !expected;
					Check(built && built.Evaluate(null, current) == expected, "state comparison truth table");
				}
				Check(!built.Evaluate(null), "missing state fails closed including inverse");
				GC_ConditionGroup none = new GC_ConditionGroup();
				none.m_iMatch = 2;
				none.m_aConditions = {condition};
				Check(!none.BuildCheck().Evaluate(null), "missing state fails closed through None");
			}
		}
		GC_GameModeStateCondition invalid = new GC_GameModeStateCondition();
		invalid.m_eGameModeState = -1;
		Check(!invalid.BuildCheck(), "invalid phase rejected");
		invalid.m_eGameModeState = SCR_EGameModeState.GAME;
		invalid.m_iMatch = 2;
		Check(!invalid.BuildCheck(), "invalid comparison rejected");
		GC_MissionRule match = StateRule("match", SCR_EGameModeState.GAME);
		GC_MissionRule outside = StateRule("outside", SCR_EGameModeState.GAME, true, true);
		GC_MissionRule after = StateRule("after", SCR_EGameModeState.POSTGAME);
		GC_MissionRuleEngine engine = new GC_MissionRuleEngine();
		engine.Start({match.BuildRule(), outside.BuildRule(), after.BuildRule()}, null, SCR_EGameModeState.PREGAME);
		Check(engine.GetRunCount() == 1 && engine.GetFlag("outside") && !engine.GetFlag("match"), "initial snapshot checks state without inventing match start");
		engine.SetGameModeState(SCR_EGameModeState.PREGAME);
		engine.Advance(10);
		engine.SendTestEvent("anything");
		engine.SetFlag("unrelated", true);
		Check(engine.GetRunCount() == 1, "duplicate callback ticks and unrelated events do not retrigger state rules");
		engine.SetGameModeState(SCR_EGameModeState.SLOTTING);
		engine.SetGameModeState(SCR_EGameModeState.BRIEFING);
		Check(engine.GetRunCount() == 3, "repeat checks each distinct matching state");
		engine.SetGameModeState(SCR_EGameModeState.GAME);
		Check(engine.GetRunCount() == 4 && engine.GetFlag("match"), "match state fires once without duplicate match-start dispatch");
		engine.SetGameModeState(SCR_EGameModeState.POSTGAME);
		Check(engine.GetRunCount() == 6 && engine.GetFlag("after"), "postgame observed and outside-match condition fires");
		engine.SetGameModeState(-1);
		Check(engine.GetRunCount() == 6, "invalid notification ignored");
		engine.Stop();
		engine.SetGameModeState(SCR_EGameModeState.GAME);
		Check(engine.GetRunCount() == 6, "stopped engine ignores callbacks");

		GC_MissionRule delayed = StateRule("delayed", SCR_EGameModeState.GAME, false, true, 5);
		engine.Start({delayed.BuildRule()}, null, SCR_EGameModeState.GAME);
		engine.SetGameModeState(SCR_EGameModeState.POSTGAME, 2);
		engine.Advance(10);
		Check(!engine.GetFlag("delayed"), "state exit cancels pending actions");
		GC_MissionRule delayedAfter = StateRule("delayed_after", SCR_EGameModeState.POSTGAME, false, false, 2);
		engine.Start({delayedAfter.BuildRule()}, null, SCR_EGameModeState.GAME);
		engine.SetGameModeState(SCR_EGameModeState.POSTGAME, 5);
		engine.Advance(6);
		Check(!engine.GetFlag("delayed_after"), "postgame wait uses callback time");
		engine.Advance(7);
		Check(engine.GetFlag("delayed_after"), "explicit postgame delay completes");

		GC_ConditionCount count = new GC_ConditionCount();
		count.m_iComparison = 2;
		count.m_iRequiredCount = 1;
		count.m_aConditions = {match.m_aConditions[0], after.m_aConditions[0]};
		GC_ConditionGroup group = new GC_ConditionGroup();
		group.m_aConditions = {count};
		match.m_aConditions = {group};
		Check(match.BuildRule().m_eWhen == GC_ERuleEvent.GAME_MODE_STATE, "nested state leaf registers state-change evaluation");
		engine.Start({match.BuildRule()}, null, SCR_EGameModeState.BRIEFING);
		Check(!engine.GetFlag("match"), "nested count blocks unmatched state");
		engine.SetGameModeState(SCR_EGameModeState.GAME);
		Check(engine.GetFlag("match"), "nested count passes matching state");
		engine.Stop();
		return string.Format("GameModeConditions: checks=%1 failures=%2", m_iChecks, m_iFailures);
	}
}

modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestGameModeConditions()
	{
		GC_MissionRuleTests tests = new GC_MissionRuleTests();
		return tests.RunGameModeConditions();
	}

	//------------------------------------------------------------------------------------------------
	//! Explicit runtime-only test configuration; stop Play to restore authored settings.
	string GC_TestStateListenerPrepare()
	{
		if (!HasMissionRuleAuthority() || GetLobbyPhase() != SCR_EGameModeState.PREGAME)
			return "FAIL: requires active authoritative Lobby2 in preview";
		StopMissionRules();
		m_bMissionRulesStarted = false;
		m_bMissionEventsEnabled = true;
		m_aMissionFlags = {};
		m_aMissionEvents = {
			GC_MissionRuleTests.StateRule("phase_preview", SCR_EGameModeState.PREGAME),
			GC_MissionRuleTests.StateRule("phase_slots", SCR_EGameModeState.SLOTTING),
			GC_MissionRuleTests.StateRule("phase_briefing", SCR_EGameModeState.BRIEFING),
			GC_MissionRuleTests.StateRule("phase_match", SCR_EGameModeState.GAME),
			GC_MissionRuleTests.StateRule("phase_after", SCR_EGameModeState.POSTGAME)
		};
		StartMissionRules();
		if (!GetFlag("phase_preview") || GetRunCount() != 1)
			return "FAIL: initial preview not observed";
		UpdateMissionGameState(SCR_EGameModeState.GAME);
		if (GetFlag("phase_match") || GetRunCount() != 1)
			return "FAIL: mismatched callback accepted";
		if (TryAdvanceLobbyPhase(-123, SCR_EGameModeState.SLOTTING) || GetLobbyPhase() != SCR_EGameModeState.PREGAME)
			return "FAIL: unauthenticated phase request accepted";
		return "PASS: startup observation, stale-callback rejection, unauthenticated transition rejection";
	}

	//------------------------------------------------------------------------------------------------
	//! Trusted test driver uses production transition owners, not fake condition notifications.
	string GC_TestStateListenerStep()
	{
		if (!HasMissionRuleAuthority())
			return "FAIL: not active authority";
		GC_LobbyGameMode mode = GC_LobbyGameMode.GetGameMode();
		SCR_EGameModeState next = GC_LobbyPhase.GetNext(mode.GetState());
		bool changed;
		if (next == SCR_EGameModeState.SLOTTING || next == SCR_EGameModeState.BRIEFING)
			changed = mode.GC_SetLobbyState(next);
		else if (next == SCR_EGameModeState.GAME)
			changed = StartLobbyGame();
		else if (next == SCR_EGameModeState.POSTGAME)
			changed = EnterPostGame(null);
		if (!changed || mode.GetState() != next)
			return "FAIL: transition failed";
		string flag;
		int expected;
		switch (next)
		{
			case SCR_EGameModeState.SLOTTING: flag = "phase_slots"; expected = 2; break;
			case SCR_EGameModeState.BRIEFING: flag = "phase_briefing"; expected = 3; break;
			case SCR_EGameModeState.GAME: flag = "phase_match"; expected = 4; break;
			case SCR_EGameModeState.POSTGAME: flag = "phase_after"; expected = 5; break;
		}
		if (!GetFlag(flag) || GetRunCount() != expected)
			return string.Format("FAIL: native callback %1 flag=%2 runs=%3 expected=%4", flag, GetFlag(flag), GetRunCount(), expected);
		UpdateMissionGameState(next);
		if (GetRunCount() != expected)
			return "FAIL: duplicate callback ran another action";
		return string.Format("PASS: real state %1, action flag=%2, runs=%3, duplicate ignored", GC_LobbyPhase.GetName(next), flag, GetRunCount());
	}

	//------------------------------------------------------------------------------------------------
	string GC_TestStateListenerStop()
	{
		StopMissionRules();
		int before = GetRunCount();
		UpdateMissionGameState(GetLobbyPhase());
		TickMissionRules();
		if (m_MissionRuleEngine.IsRunning() || GetRunCount() != before)
			return "FAIL: stopped listener restarted";
		return "PASS: stopped engine stays stopped";
	}
}
