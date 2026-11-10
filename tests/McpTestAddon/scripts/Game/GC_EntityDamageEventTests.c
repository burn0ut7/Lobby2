//! Optional deterministic tests of the production callback-dispatch path, without live damage.
class GC_TestDamageObservation : GC_EntityDamageObservation
{
	protected bool m_bTestDestroyed;
	protected float m_fTestDamage;

	//------------------------------------------------------------------------------------------------
	override bool IsAvailable()
	{
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool Passes(int kind, float damageNeeded)
	{
		return Matches(true, m_bTestDestroyed, m_fTestDamage, kind, damageNeeded);
	}

	//------------------------------------------------------------------------------------------------
	void Emit(bool destroyed, float damage)
	{
		m_bTestDestroyed = destroyed;
		m_fTestDamage = damage;
		GetOnChanged().Invoke(this);
	}
}

class GC_TestDamageEngine : GC_MissionRuleEngine
{
	ref GC_TestDamageObservation m_TestSource;
	bool m_bChangeDuringAction;

	//------------------------------------------------------------------------------------------------
	void Install(GC_RuleCondition check, GC_TestDamageObservation observation)
	{
		GC_EntityDamageObservation previous = m_mDamage.Get(check);
		if (previous)
		{
			previous.GetOnChanged().Remove(OnEntityDamageChanged);
			previous.Stop();
		}
		m_mDamage.Set(check, observation);
		observation.AddCondition(check);
		observation.GetOnChanged().Remove(OnEntityDamageChanged);
		observation.GetOnChanged().Insert(OnEntityDamageChanged);
		RememberDamageAnswer(check, observation);
	}

	//------------------------------------------------------------------------------------------------
	override protected void Execute(GC_MissionRuleState state)
	{
		super.Execute(state);
		if (m_bChangeDuringAction)
		{
			m_bChangeDuringAction = false;
			m_TestSource.Emit(false, 0.1);
			m_TestSource.Emit(false, 0.9);
		}
	}
}

modded class GC_MissionRuleTests
{
	//------------------------------------------------------------------------------------------------
	string RunEntityDamageEvents()
	{
		GC_EntityDamageCondition half = new GC_EntityDamageCondition();
		half.m_sEntityName = "Target";
		half.m_iCheck = 2;
		half.m_fDamageNeeded = 0.5;
		GC_EntityDamageCondition most = new GC_EntityDamageCondition();
		most.m_sEntityName = "Target";
		most.m_iCheck = 2;
		most.m_fDamageNeeded = 0.8;
		GC_Event authored = StateRule("damage", SCR_EGameModeState.GAME);
		authored.m_aConditions = {half, most};
		authored.m_Advanced.m_bRepeatable = true;
		GC_CompiledMissionRule rule = authored.BuildRule();
		GC_TestDamageEngine engine = new GC_TestDamageEngine();
		engine.Start({rule}, null, SCR_EGameModeState.GAME);
		GC_TestDamageObservation source = new GC_TestDamageObservation();
		engine.Install(rule.m_If.m_aChecks[0], source);
		engine.Install(rule.m_If.m_aChecks[1], source);
		source.Emit(false, 0.85);
		Check(engine.GetRunCount() == 1, "both shared-source leaves see damage and run immediately without Advance");
		source.Emit(false, 0.85);
		Check(engine.GetRunCount() == 1, "duplicate callback does not rerun unchanged answers");
		source.Emit(false, 0.9);
		Check(engine.GetRunCount() == 1, "more damage above passing thresholds does not retrigger");
		source.Emit(false, 0.1);
		Check(engine.GetRunCount() == 1, "repair below threshold updates without running false event");
		engine.m_TestSource = source;
		engine.m_bChangeDuringAction = true;
		source.Emit(false, 0.9);
		Check(engine.GetRunCount() == 2, "recrossing runs once; damage during action stays in bounded notification chain");
		engine.Stop();
		source.Emit(false, 0.1);
		source.Emit(false, 0.9);
		Check(engine.GetRunCount() == 2, "Stop removes source listener");
		return string.Format("Entity Damage event checks: %1; failures: %2", m_iChecks, m_iFailures);
	}
}
