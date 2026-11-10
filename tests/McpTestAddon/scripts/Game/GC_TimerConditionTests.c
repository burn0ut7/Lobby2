//! Explicit engine-logic fixtures. Loading this addon never runs them or enters Play.
class GC_TimerConditionTests : GC_MissionRuleTests
{
	//------------------------------------------------------------------------------------------------
	string RunTimerChecks()
	{
		GC_TimerCondition timer = new GC_TimerCondition();
		timer.m_eStartState = SCR_EGameModeState.GAME;
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
		Check(engine.GetRunCount() == 3, "Finished timer stays true through later states");
		engine.Start({rule}, null, SCR_EGameModeState.GAME);
		engine.Advance(9);
		Check(engine.GetRunCount() == 0, "Framework restart creates a fresh timer");
		engine.Advance(50);
		Check(engine.GetRunCount() == 1, "Late update expires once without catch-up burst");
		engine.Stop();
		CheckPhaseAndRange();
		CheckTimerActions();

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

	//------------------------------------------------------------------------------------------------
	protected void CheckTimerActions()
	{
		GC_TimerCondition timer = new GC_TimerCondition();
		timer.m_sTimerName = "extraction";
		timer.m_eStartState = SCR_EGameModeState.GAME;
		timer.m_fSeconds = 10;
		GC_CompiledMissionRule rule = EventRule("controlled");
		rule.m_If = timer.BuildCheck();
		GC_TimerTestEngine engine = new GC_TimerTestEngine();
		engine.Start({rule}, null, SCR_EGameModeState.BRIEFING);
		engine.ControlTimer("extraction", 3, 20);
		engine.Advance(5);
		engine.ControlTimer("extraction", 1);
		engine.Advance(100);
		Check(!engine.TimerResult(rule.m_If), "Pause freezes timer before configured start state");
		engine.ControlTimer("extraction", 4, 5);
		engine.ControlTimer("extraction", 5, 2.5);
		engine.ControlTimer("extraction", 2);
		engine.SetGameModeState(SCR_EGameModeState.GAME, 100);
		engine.Advance(117.4);
		Check(!engine.TimerResult(rule.m_If), "Resume preserves adjusted decimal remaining time; phase does not restart");
		engine.Advance(117.5);
		Check(engine.TimerResult(rule.m_If), "Adjusted timer expires");
		engine.ControlTimer("extraction", 4, 3);
		Check(!engine.TimerResult(rule.m_If), "Adding time reopens a finished timer");
		engine.ControlTimer("extraction", 5, 100);
		Check(engine.TimerResult(rule.m_If), "Remove clamps to zero and finishes immediately");
		engine.ControlTimer("extraction", 0);
		Check(!engine.TimerResult(rule.m_If) && engine.m_iDraws == 0, "Restart clears result without drawing for fixed timer");
		engine.ControlTimer("extraction", 3, 86400);
		engine.ControlTimer("extraction", 4, 1);
		engine.Advance(86517.5);
		Check(engine.TimerResult(rule.m_If), "Excessive addition rejected without extending deadline");
		Check(!engine.ControlTimer("extraction", 3, -1), "Negative time rejected at action boundary");
		engine.Stop();

		timer.m_fRandomMaxSeconds = 20;
		rule.m_If = timer.BuildCheck();
		engine.Start({rule}, null, SCR_EGameModeState.GAME);
		int draws = engine.m_iDraws;
		engine.ControlTimer("extraction", 0);
		Check(engine.m_iDraws == draws + 1, "Random restart draws exactly once");
		engine.ControlTimer("missing", 0);
		Check(engine.m_iDraws == draws + 1, "Missing name changes nothing");
		engine.Stop();

		GC_CompiledMissionRule other = EventRule("other");
		other.m_If = timer.BuildCheck();
		engine.Start({rule, other}, null, SCR_EGameModeState.GAME);
		engine.ControlTimer("extraction", 3, 0);
		Check(!engine.TimerResult(rule.m_If) && !engine.TimerResult(other.m_If), "Duplicate names change neither timer");
		engine.Stop();
	}

	//------------------------------------------------------------------------------------------------
	protected void CheckPhaseAndRange()
	{
		GC_TimerCondition timer = new GC_TimerCondition();
		timer.m_eStartState = SCR_EGameModeState.BRIEFING;
		timer.m_fSeconds = 10;
		timer.m_fRandomMaxSeconds = 20;
		GC_CompiledMissionRule rule = EventRule("range");
		rule.m_If = timer.BuildCheck();
		GC_TimerTestEngine engine = new GC_TimerTestEngine();
		engine.Start({rule}, null, SCR_EGameModeState.PREGAME);
		engine.Advance(100);
		Check(engine.m_iDraws == 0 && engine.GetRunCount() == 0, "Waiting for selected state does not draw or expire");
		engine.SetGameModeState(SCR_EGameModeState.BRIEFING, 100);
		Check(engine.m_iDraws == 1, "Draw one duration on selected-state entry");
		engine.Advance(110);
		engine.SetGameModeState(SCR_EGameModeState.GAME, 110);
		engine.SendTestEvent("range");
		Check(engine.m_iDraws == 1 && engine.GetRunCount() == 0, "Later state and checks do not reroll or restart");
		engine.Advance(115);
		Check(engine.GetRunCount() == 1 && engine.m_iDraws == 1, "Midpoint draw expires at sampled deadline across states");
		engine.Stop();
		timer.m_fRandomMaxSeconds = 9;
		GC_RuleCondition normalized = timer.BuildCheck();
		Check(normalized && normalized.m_fRandomInterval == 9 && normalized.m_fTimerMaximum == 10, "Reversed bounds are ordered");
		timer.m_fRandomMaxSeconds = 0;
		normalized = timer.BuildCheck();
		Check(normalized && normalized.m_fRandomInterval == 10 && normalized.m_fTimerMaximum == 10, "Blank or zero maximum is fixed");
		timer.m_fRandomMaxSeconds = -1;
		Check(!timer.BuildCheck(), "Negative maximum rejected");
		timer.m_fRandomMaxSeconds = 86401;
		Check(!timer.BuildCheck(), "Excessive maximum rejected");
		timer.m_fRandomMaxSeconds = 0.05;
		Check(!timer.BuildCheck(), "Positive subminimum maximum rejected");
		timer.m_fRandomMaxSeconds = 2.5;
		timer.m_fSeconds = 2.5;
		normalized = timer.BuildCheck();
		Check(normalized && normalized.m_fRandomInterval == 2.5 && normalized.m_fTimerMaximum == 2.5, "Equal decimal bounds are fixed");
		timer.m_fSeconds = 10;
		timer.m_fRandomMaxSeconds = 0;
		timer.m_eStartState = SCR_EGameModeState.POSTGAME;
		rule.m_If = timer.BuildCheck();
		engine.Start({rule}, null, SCR_EGameModeState.POSTGAME);
		engine.Advance(10);
		Check(engine.GetRunCount() == 1, "After-action timer can expire");
		engine.Stop();
	}
}

class GC_TimerTestEngine : GC_MissionRuleEngine
{
	int m_iDraws;

	//------------------------------------------------------------------------------------------------
	bool ControlTimer(string name, int operation, float seconds = 0)
	{
		GC_TimerAction action = new GC_TimerAction();
		action.m_sTimerName = name;
		action.m_iOperation = operation;
		action.m_fSeconds = seconds;
		return QueueFrameworkAction(action.BuildAction());
	}

	//------------------------------------------------------------------------------------------------
	bool TimerResult(GC_RuleCondition condition)
	{
		return m_mRandomResults.Get(condition);
	}

	//------------------------------------------------------------------------------------------------
	override protected float RandomUnit()
	{
		m_iDraws++;
		return 0.5;
	}
}
