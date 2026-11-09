//! Explicit deterministic scenarios; no automatic execution or gameplay configuration.
modded class GC_MissionRuleTests
{
	//------------------------------------------------------------------------------------------------
	static GC_RandomChanceCondition RandomCondition(float interval = 60, float chance = 1, int behavior = 0)
	{
		GC_RandomChanceCondition condition = new GC_RandomChanceCondition();
		condition.m_fCheckInterval = interval;
		condition.m_fChance = chance;
		condition.m_bRollOnce = behavior == 1;
		return condition;
	}

	//------------------------------------------------------------------------------------------------
	static GC_Event RandomConditionEvent(GC_ConditionBase condition, bool repeat = false, float delay = 0)
	{
		GC_Event authoredEvent = StateRule("random_condition", SCR_EGameModeState.GAME, false, repeat, delay);
		authoredEvent.m_aConditions = {condition};
		return authoredEvent;
	}

	//------------------------------------------------------------------------------------------------
	string RunRandomConditions()
	{
		GC_MissionRandomTestEngine engine = new GC_MissionRandomTestEngine();
		GC_RandomChanceCondition random = RandomCondition(60, 0.05);
		GC_Event authoredEvent = RandomConditionEvent(random, true);
		GC_CompiledMissionRule rule = authoredEvent.BuildRule();
		Check(rule && rule.m_If.UsesRandom(), "authored random condition compiles and is discoverable");
		engine.m_fRoll = 0.049;
		engine.Start({rule}, null, SCR_EGameModeState.PREGAME);
		engine.Advance(100);
		Check(engine.m_iDraws == 0 && engine.GetRunCount() == 0, "no random draws before Match");
		engine.SetGameModeState(SCR_EGameModeState.GAME, 100);
		engine.Advance(159.9);
		Check(engine.m_iDraws == 0 && engine.GetRunCount() == 0, "first interval measured from Match entry");
		engine.Advance(160);
		Check(engine.m_iDraws == 1 && engine.GetRunCount() == 1, "first scheduled success executes");
		engine.Advance(160);
		engine.Advance(219);
		Check(engine.m_iDraws == 1 && engine.GetRunCount() == 1, "reads and duplicate ticks do not reroll or run");
		engine.m_fRoll = 0.05;
		engine.Advance(220);
		Check(engine.m_iDraws == 2 && engine.GetRunCount() == 1, "draw equal to chance fails");
		engine.m_fRoll = 0;
		engine.Advance(280);
		Check(engine.m_iDraws == 3 && engine.GetRunCount() == 2, "Every time accepts a later success");
		engine.Advance(1000);
		Check(engine.m_iDraws == 4 && engine.GetRunCount() == 3, "late update rolls once without catch-up burst");
		engine.SetGameModeState(SCR_EGameModeState.POSTGAME, 1000);
		engine.Advance(2000);
		Check(engine.m_iDraws == 4 && engine.GetRunCount() == 3, "no draws after Match");
		engine.Stop();
		engine.Advance(3000);
		Check(engine.GetRunCount() == 3, "Stop prevents scheduled execution");

		// A single roll locks either outcome; restarting clears the held result.
		random.m_bRollOnce = true;
		rule = authoredEvent.BuildRule();
		engine.m_iDraws = 0;
		engine.m_fRoll = 0.5;
		engine.Start({rule}, null, SCR_EGameModeState.GAME);
		engine.Advance(60);
		engine.m_fRoll = 0;
		engine.Advance(600);
		Check(engine.m_iDraws == 1 && engine.GetRunCount() == 0, "Roll once preserves failure, not retry until success");
		engine.Start({rule}, null, SCR_EGameModeState.GAME);
		engine.Advance(60);
		engine.Advance(600);
		Check(engine.m_iDraws == 2 && engine.GetRunCount() == 1, "fresh Start resets schedule; Roll once preserves success");

		// Exact chance boundaries do not depend on the random generator's endpoints.
		random.m_bRollOnce = false;
		random.m_fChance = 0;
		engine.m_iDraws = 0;
		engine.Start({authoredEvent.BuildRule()}, null, SCR_EGameModeState.GAME);
		engine.Advance(60);
		Check(engine.GetRunCount() == 0 && engine.m_iDraws == 0, "zero never passes even for zero draw");
		random.m_fChance = 1;
		engine.m_fRoll = 1;
		authoredEvent.m_Advanced.m_bRepeatable = false;
		engine.Start({authoredEvent.BuildRule()}, null, SCR_EGameModeState.GAME);
		engine.Advance(60);
		engine.Advance(120);
		Check(engine.GetRunCount() == 1 && engine.m_iDraws == 0, "one always passes; Once event runs only once");
		random.m_fChance = 0.0005;
		Check(random.BuildCheck() != null, "0.05 percent accepted as 0.0005");
		random.m_fChance = -0.1;
		Check(!random.BuildCheck(), "negative chance rejected");
		random.m_fChance = 1.1;
		Check(!random.BuildCheck(), "chance above one rejected");
		random.m_fChance = 1;
		random.m_fCheckInterval = 0;
		Check(!random.BuildCheck(), "zero interval rejected");
		random.m_fCheckInterval = 86401;
		Check(!random.BuildCheck(), "oversized interval rejected");
		random.m_fCheckInterval = 60;

		random.m_bRollOnce = false;

		// A random answer never overrides enclosing logical operators.
		GC_MissionFlagCondition gate = new GC_MissionFlagCondition();
		gate.m_sFlagName = "gate";
		GC_ConditionGroup group = new GC_ConditionGroup();
		group.m_aConditions = {random, gate};
		authoredEvent = RandomConditionEvent(group, true);
		engine.Start({authoredEvent.BuildRule()}, null, SCR_EGameModeState.GAME);
		engine.Advance(60);
		Check(engine.GetRunCount() == 0, "All rejects success while another child is false");
		engine.SetFlag("gate", true);
		Check(engine.GetRunCount() == 1, "later flag change can use held success without rerolling");
		group.m_iMatch = 1;
		engine.Start({authoredEvent.BuildRule()}, null, SCR_EGameModeState.GAME);
		engine.SetFlag("gate", true);
		Check(engine.GetRunCount() == 1, "Any can pass through another child before first random roll");
		group.m_iMatch = 2;
		group.m_aConditions = {random};
		engine.Start({authoredEvent.BuildRule()}, null, SCR_EGameModeState.GAME);
		Check(engine.GetRunCount() == 1, "None applies ordinary inversion to initial false random result");
		engine.Advance(60);
		Check(engine.GetRunCount() == 1, "None rejects a true random result");

		// All due leaves roll before a nested count is evaluated; different intervals retain results.
		GC_RandomChanceCondition second = RandomCondition(90, 1);
		GC_ConditionCount count = new GC_ConditionCount();
		count.m_iComparison = 2;
		count.m_iRequiredCount = 2;
		count.m_aConditions = {random, second};
		authoredEvent = RandomConditionEvent(count, true);
		engine.Start({authoredEvent.BuildRule()}, null, SCR_EGameModeState.GAME);
		engine.Advance(60);
		Check(engine.GetRunCount() == 0, "count sees only first successful leaf");
		engine.Advance(90);
		Check(engine.GetRunCount() == 1, "count combines held results from different intervals");
		engine.Advance(180);
		Check(engine.GetRunCount() == 2, "multiple due leaves cause one event attempt per update");

		// Latest result governs delayed execution, without another draw at delay expiry.
		random.m_fChance = 0.5;
		authoredEvent = RandomConditionEvent(random, true, 90);
		engine.m_fRoll = 0;
		engine.Start({authoredEvent.BuildRule()}, null, SCR_EGameModeState.GAME);
		engine.Advance(60);
		engine.m_fRoll = 1;
		engine.Advance(120);
		engine.Advance(150);
		Check(engine.GetRunCount() == 0, "later failed roll prevents delayed execution");
		authoredEvent.m_Advanced.m_fDelay = 10;
		engine.m_fRoll = 0;
		engine.Start({authoredEvent.BuildRule()}, null, SCR_EGameModeState.GAME);
		engine.Advance(60);
		int draws = engine.m_iDraws;
		engine.Advance(70);
		Check(engine.GetRunCount() == 1 && engine.m_iDraws == draws, "delay completion uses held result without reroll");

		// Flags own repeat/count policy, including when no mission events are configured.
		GC_MissionFlag flag = new GC_MissionFlag();
		flag.m_sName = "random_flag";
		flag.m_bRepeatable = true;
		random.m_fChance = 1;
		flag.m_aConditions = {random};
		engine.Start({}, {flag}, SCR_EGameModeState.GAME);
		engine.Advance(60);
		engine.Advance(120);
		Check(engine.GetFlagCallCount("random_flag") == 2 && !engine.GetFlag("random_flag"), "random checks call repeatable flag then reset pulse");
		engine.Stop();
		return string.Format("Random condition checks: %1; failures: %2", m_iChecks, m_iFailures);
	}
}
