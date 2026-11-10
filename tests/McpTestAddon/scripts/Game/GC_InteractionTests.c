//! Deterministic observation fixture. No world entities or player RPCs are simulated by production.
class GC_TestInteractionObservation : GC_InteractionObservation
{
	bool m_bTestAvailable = true;
	bool m_bTestChanged;

	//------------------------------------------------------------------------------------------------
	override bool IsAvailable()
	{
		return m_bTestAvailable;
	}

	//------------------------------------------------------------------------------------------------
	void Use()
	{
		m_bTriggered = true;
		m_bTestChanged = true;
	}

	//------------------------------------------------------------------------------------------------
	override bool ConsumeChange()
	{
		bool changed = m_bTestChanged;
		m_bTestChanged = false;
		return changed;
	}
}

class GC_TestInteractionEngine : GC_MissionRuleEngine
{
	//------------------------------------------------------------------------------------------------
	void Install(GC_RuleCondition check, GC_TestInteractionObservation observation)
	{
		m_mInteractions.Set(check, observation);
	}
}

modded class GC_MissionRuleTests
{
	//------------------------------------------------------------------------------------------------
	string RunInteractions()
	{
		GC_InteractionCondition condition = new GC_InteractionCondition();
		Check(!condition.BuildCheck(), "blank interaction name rejected");
		condition.m_sEntityName = "Radio";
		GC_RuleCondition check = condition.BuildCheck();
		Check(check && check.UsesInteraction(), "interaction leaf is discoverable");
		Check(!check.Evaluate(null), "unresolved interaction fails closed");
		GC_RuleCondition none = GC_ConditionBase.BuildGroup({condition}, GC_ERuleCheck.NONE, 0, 0);
		Check(!none.Evaluate(null), "unresolved interaction under None fails closed");
		GC_Event authored = StateRule("interaction", SCR_EGameModeState.GAME);
		authored.m_aConditions = {condition};
		authored.m_Advanced = new GC_RuleOptions();
		authored.m_Advanced.m_bRepeatable = true;
		GC_CompiledMissionRule rule = authored.BuildRule();
		GC_RuleCondition leaf = rule.m_If.m_aChecks[0];
		GC_TestInteractionEngine engine = new GC_TestInteractionEngine();
		engine.Start({rule}, null, SCR_EGameModeState.GAME);
		GC_TestInteractionObservation observation = new GC_TestInteractionObservation();
		engine.Install(leaf, observation);
		observation.Use();
		engine.NotifyInteraction("");
		Check(engine.GetRunCount() == 1, "accepted use runs observing event");
		Check(!observation.IsTriggered(), "successful interaction resets after dispatch");
		engine.NotifyInteraction("");
		Check(engine.GetRunCount() == 1, "unchanged interaction does not notify twice");
		observation.Use();
		engine.NotifyInteraction("");
		Check(engine.GetRunCount() == 2, "another accepted use reaches repeatable event");
		engine.Stop();
		engine.NotifyInteraction("");
		Check(engine.GetRunCount() == 2, "stopped engine ignores interaction");
		GC_MissionFlag flag = new GC_MissionFlag();
		flag.m_sName = "used";
		flag.m_bRepeatable = true;
		GC_MissionFlagCondition flagCondition = new GC_MissionFlagCondition();
		flagCondition.m_sFlagName = "used";
		authored.m_aConditions = {condition, flagCondition};
		rule = authored.BuildRule();
		leaf = rule.m_If.m_aChecks[0];
		engine.Start({rule}, {flag}, SCR_EGameModeState.GAME);
		observation = new GC_TestInteractionObservation();
		engine.Install(leaf, observation);
		observation.Use();
		engine.NotifyInteraction("");
		Check(engine.GetRunCount() == 0 && !observation.IsTriggered(), "failed conditions reset the interaction");
		engine.CallFlag("used");
		Check(engine.GetRunCount() == 0, "later flag change cannot reuse a previous interaction");
		observation.Use();
		engine.NotifyInteraction("used");
		Check(engine.GetRunCount() == 1, "interaction and flag call share one notification chain");
		Check(engine.GetFlagCallCount("used") == 2 && engine.GetFlag("used"), "output flag remembers accepted calls");
		return string.Format("Interaction checks: %1; failures: %2", m_iChecks, m_iFailures);
	}
}
