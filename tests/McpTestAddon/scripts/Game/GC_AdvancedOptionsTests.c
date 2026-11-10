//! Explicit optional-object regressions. Never run automatically.
class GC_AdvancedOptionsTests : GC_MissionRuleTests
{
	//------------------------------------------------------------------------------------------------
	string RunAdvancedOptionsChecks()
	{
		GC_Event missionEvent = SimpleRule("defaults");
		missionEvent.m_Advanced = null;
		GC_CompiledMissionRule rule = missionEvent.BuildRule();
		Check(rule && rule.m_bEnabled && rule.m_bRunOnlyOnce && rule.m_fWaitSeconds == 0 && rule.m_fChance == 1, "Missing missionEvent options preserve normal defaults");
		missionEvent.m_Advanced = new GC_EventOptions();
		missionEvent.m_Advanced.m_fRandomChance = 0;
		missionEvent.m_Advanced.m_bDisabled = true;
		missionEvent.m_Advanced.m_bRepeatable = true;
		missionEvent.m_Advanced.m_fDelay = 5;
		rule = missionEvent.BuildRule();
		Check(rule && !rule.m_bEnabled && !rule.m_bRunOnlyOnce && rule.m_fWaitSeconds == 5 && rule.m_fChance == 0, "Explicit zero chance and overrides are not replaced by defaults");
		missionEvent.m_Advanced.m_fDelay = -1;
		Check(!missionEvent.BuildRule(), "Invalid options remain invalid");
		missionEvent.m_Advanced = null;
		GC_RandomEventGroup group = new GC_RandomEventGroup();
		group.m_sName = "group";
		group.m_iPickCount = 1;
		group.m_aEvents = {missionEvent};
		rule = group.BuildRule();
		Check(rule && rule.m_fChance == 1 && rule.m_bRunOnlyOnce, "Missing group options preserve normal defaults");
		GC_MissionFlag flag = new GC_MissionFlag();
		flag.m_sName = "flag";
		Check(flag.GetStartCallCount() == 0 && !flag.IsRepeatable(), "Missing flag options start false and nonrepeatable");
		flag.m_Advanced = new GC_FlagOptions();
		flag.m_Advanced.m_iStartCallCount = 3;
		flag.m_Advanced.m_bRepeatable = true;
		GC_MissionRuleEngine engine = new GC_MissionRuleEngine();
		engine.Start({}, {flag}, SCR_EGameModeState.GAME);
		Check(engine.GetFlagCallCount("flag") == 3 && engine.GetFlag("flag"), "Flag options seed authoritative count");
		engine.CallFlag("flag");
		Check(engine.GetFlagCallCount("flag") == 4, "Repeatable flag still accepts calls");
		engine.Stop();
		return string.Format("AdvancedOptions: checks=%1 failures=%2", m_iChecks, m_iFailures);
	}
}
