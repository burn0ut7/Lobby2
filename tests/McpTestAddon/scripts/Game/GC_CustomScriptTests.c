#ifdef WORKBENCH
[BaseContainerProps()]
class GC_TestMissionScript : GC_MissionScript
{
	[Attribute("0")]
	bool m_bAnswer;
	int m_iCalls;
	bool m_bValid = true;
	GC_MissionRuleEngine m_Stop;

	override bool IsValid()
	{
		return m_bValid;
	}

	override bool Run(GC_LobbyManager manager)
	{
		m_iCalls++;
		if (m_Stop)
			m_Stop.Stop();
		return m_bAnswer;
	}
}

[BaseContainerProps(configRoot: true)]
class GC_ScriptAuthoringFixture
{
	[Attribute()]
	ref GC_CustomScriptAction m_Action;
	[Attribute()]
	ref GC_CustomScriptCondition m_Condition;
}

modded class GC_MissionRuleTests
{
	//------------------------------------------------------------------------------------------------
	string RunCustomScripts(GC_LobbyManager manager, GC_MissionRuleEngine engine)
	{
		GC_CustomScriptAction action = new GC_CustomScriptAction();
		GC_CustomScriptCondition condition = new GC_CustomScriptCondition();
		Check(!action.BuildAction().IsValid() && !condition.BuildCheck(), "empty hooks rejected");
		GC_MissionScript bare = new GC_MissionScript();
		Check(!bare.IsValid(), "base hook is not a usable script");
		Resource resource = Resource.Load("{F71C000000000011}Configs/CustomScripts.conf");
		GC_ScriptAuthoringFixture fixture;
		if (resource && resource.IsValid())
			fixture = GC_ScriptAuthoringFixture.Cast(BaseContainerTools.CreateInstanceFromContainer(resource.GetResource().ToBaseContainer()));
		Check(fixture && fixture.m_Action && fixture.m_Condition, "authored script objects load");
		if (!fixture || !fixture.m_Action || !fixture.m_Condition)
			return "Custom script authoring fixture unavailable";
		GC_TestMissionScript output = GC_TestMissionScript.Cast(fixture.m_Action.m_Script);
		GC_TestMissionScript predicate = GC_TestMissionScript.Cast(fixture.m_Condition.m_Script);
		Check(output && predicate && output.m_bAnswer && !predicate.m_bAnswer, "subclass attributes deserialize");
		action = fixture.m_Action;
		condition = fixture.m_Condition;
		GC_ConditionGroup nested = new GC_ConditionGroup();
		nested.m_aConditions = {condition};
		GC_Event missionEvent = new GC_Event();
		missionEvent.m_sName = "script-missionEvent";
		missionEvent.m_aConditions = {nested};
		missionEvent.m_aActions = {action};
		missionEvent.m_Advanced = new GC_EventOptions();
		missionEvent.m_Advanced.m_fRandomChance = 1;
		missionEvent.m_Advanced.m_bRepeatable = true;
		GC_MissionFlag flag = new GC_MissionFlag();
		flag.m_sName = "script-flag";
		flag.m_aConditions = {condition};
		engine.Start({missionEvent.BuildRule()}, {flag}, SCR_EGameModeState.BRIEFING, null, null, GetGame().GetWorld(), manager);
		engine.Advance(10);
		Check(engine.IsRunning() && predicate.m_iCalls == 0 && output.m_iCalls == 0, "no condition execution in briefing");
		engine.SetGameModeState(SCR_EGameModeState.GAME, 10);
		Check(predicate.m_iCalls == 2 && output.m_iCalls == 0 && !engine.GetFlag("script-flag"), "missionEvent and flag sample false on match entry");
		engine.Advance(10.5);
		Check(predicate.m_iCalls == 2, "cached between one-second samples");
		predicate.m_bAnswer = true;
		engine.Advance(11);
		Check(output.m_iCalls == 1 && engine.GetFlag("script-flag"), "true script triggers action and flag through production scheduler");
		engine.Advance(12);
		Check(output.m_iCalls == 1, "unchanged true does not repeatedly fire missionEvent");
		predicate.m_bAnswer = false;
		engine.Advance(13);
		predicate.m_bAnswer = true;
		engine.Advance(14);
		Check(output.m_iCalls == 2, "false-to-true transition repeats enabled missionEvent");
		bool answer;
		output.m_bAnswer = false;
		Check(manager.GC_RunFrameworkScript(engine, output, answer) && !answer, "valid false differs from unavailable execution");
		Check(!manager.GC_ExecuteFrameworkAction(engine, action.BuildAction()), "false action reports rejection");
		int calls = output.m_iCalls;
		GC_MissionRuleEngine foreign = new GC_MissionRuleEngine();
		foreign.Start({});
		Check(!manager.GC_RunFrameworkScript(foreign, output, answer) && output.m_iCalls == calls, "foreign running engine cannot execute hook");
		foreign.Stop();
		output.m_bValid = false;
		Check(!manager.GC_RunFrameworkScript(engine, output, answer) && output.m_iCalls == calls, "invalid hook cannot execute");
		output.m_bValid = true;
		output.m_bAnswer = true;
		GC_RuleCondition check = condition.BuildCheck();
		GC_RuleCondition none = new GC_RuleCondition();
		none.m_eGroupNeeds = GC_ERuleCheck.NONE;
		none.m_aChecks = {check};
		Check(!none.Evaluate(null), "unsampled script cannot pass through inversion");
		map<GC_RuleCondition, bool> answers = new map<GC_RuleCondition, bool>();
		answers.Set(check, false);
		Check(none.Evaluate(null, -1, null, null, null, null, null, null, null, answers), "sampled false supports None groups");
		int beforePost = predicate.m_iCalls;
		engine.SetGameModeState(SCR_EGameModeState.POSTGAME, 14);
		engine.Advance(30);
		Check(predicate.m_iCalls == beforePost && output.m_iCalls == calls, "postgame stops condition sampling");
		engine.Stop();
		Check(!manager.GC_RunFrameworkScript(engine, output, answer) && output.m_iCalls == calls, "stopped engine cannot execute hook");

		// Delay is owned by the missionEvent; a later false script answer cancels the pending action.
		output.m_iCalls = 0;
		predicate.m_bAnswer = true;
		missionEvent.m_Advanced.m_fDelay = 2;
		engine.Start({missionEvent.BuildRule()}, null, SCR_EGameModeState.GAME, null, null, GetGame().GetWorld(), manager);
		predicate.m_bAnswer = false;
		engine.Advance(1);
		engine.Advance(2);
		Check(output.m_iCalls == 0, "false script cancels delayed action");
		predicate.m_bAnswer = true;
		engine.Advance(3);
		engine.Advance(5);
		Check(output.m_iCalls == 1, "stable true permits delayed action");
		predicate.m_Stop = engine;
		engine.Advance(6);
		Check(!engine.IsRunning(), "hook shutdown stops sampling safely");
		predicate.m_Stop = null;
		return string.Format("Custom script checks: %1; failures: %2", m_iChecks, m_iFailures);
	}
}

modded class GC_LobbyManager
{
	//------------------------------------------------------------------------------------------------
	string GC_TestCustomScripts()
	{
		if (!HasMissionRuleAuthority())
			return "Active Lobby2 server required";
		GC_MissionRuleEngine previous = m_MissionRuleEngine;
		bool enabled = m_bMissionEventsEnabled;
		GC_MissionRuleEngine engine = new GC_MissionRuleEngine();
		m_MissionRuleEngine = engine;
		m_bMissionEventsEnabled = true;
		GC_MissionRuleTests tests = new GC_MissionRuleTests();
		string result = tests.RunCustomScripts(this, engine);
		engine.Stop();
		m_MissionRuleEngine = previous;
		m_bMissionEventsEnabled = enabled;
		return result;
	}
}
#endif
