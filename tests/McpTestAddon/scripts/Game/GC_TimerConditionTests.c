//! Explicit engine-logic fixtures. Loading this addon never runs them or enters Play.
class GC_TimerConditionTests : GC_MissionRuleTests
{
	//------------------------------------------------------------------------------------------------
	string RunTimerChecks()
	{
		GC_TimerCondition timer = new GC_TimerCondition();
		timer.m_fSeconds = 10;
		GC_CompiledMissionRule rule = EventRule("timer", false);
		rule.m_If = timer.BuildCheck();
		GC_MissionRuleEngine engine = new GC_MissionRuleEngine();
		engine.Start({rule}, null, SCR_EGameModeState.BRIEFING);
		engine.Advance(100);
		Check(engine.GetRunCount() == 0, "Briefing time does not expire timer");
		engine.SetGameModeState(SCR_EGameModeState.GAME, 100);
		engine.Advance(109.9);
		Check(engine.GetRunCount() == 0, "Timer false before deadline");
		engine.Advance(110);
		Check(engine.GetRunCount() == 1, "Deadline notifies its event once");
		engine.Advance(200);
		Check(engine.GetRunCount() == 1, "Repeatable event does not turn timer into interval");
		engine.SendTestEvent("timer");
		Check(engine.GetRunCount() == 2, "Timer remains true on later relevant checks");
		engine.SetGameModeState(SCR_EGameModeState.POSTGAME, 201);
		engine.SendTestEvent("timer");
		Check(engine.GetRunCount() == 2, "Timer is false after Match ends");
		engine.Start({rule}, null, SCR_EGameModeState.GAME);
		engine.Advance(9);
		Check(engine.GetRunCount() == 0, "Framework restart creates a fresh timer");
		engine.Advance(50);
		Check(engine.GetRunCount() == 1, "Late update expires once without catch-up burst");
		engine.Stop();

		GC_RuleCondition gate = new GC_RuleCondition();
		gate.m_iCheck = 1;
		gate.m_sFlagName = "ready";
		GC_RuleCondition all = new GC_RuleCondition();
		all.m_aChecks = {timer.BuildCheck(), gate};
		rule.m_If = all;
		rule.m_bObserveFlags = true;
		engine.Start({rule}, null, SCR_EGameModeState.GAME);
		engine.Advance(10);
		Check(engine.GetRunCount() == 0, "Expired timer cannot bypass another false condition");
		engine.SetFlag("ready", true);
		Check(engine.GetRunCount() == 1, "Later flag change can use elapsed timer in nested group");
		engine.Stop();
		timer.m_fSeconds = 0;
		Check(!timer.BuildCheck(), "Zero duration rejected; use Game Mode State for match start");
		timer.m_fSeconds = 86401;
		Check(!timer.BuildCheck(), "Duration over one day rejected");
		return string.Format("TimerConditions: checks=%1 failures=%2", m_iChecks, m_iFailures);
	}
}
