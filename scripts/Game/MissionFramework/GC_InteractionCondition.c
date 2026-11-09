[BaseContainerProps(), BaseContainerCustomStringTitleField("Interaction Condition")]
class GC_InteractionCondition : GC_ConditionBase
{
	[Attribute("", UIWidgets.EditBox, "Exact unique world name of the object carrying GC_MissionInteractionComponent. Place it before Match starts. Reuses its remembered result without performing the interaction or rechecking its availability.")]
	string m_sEntityName;
	[Attribute("0", UIWidgets.ComboBox, "Has been used stays true after the first accepted use, including for repeatable interactions. Each later use notifies observing events again. Missing/deleted objects never pass either choice, even in None. Event repetition stays in the event's Advanced settings.", enums: {ParamEnum("Has been used", "0"), ParamEnum("Has not been used", "1")})]
	int m_iMatch;

	//------------------------------------------------------------------------------------------------
	override GC_RuleCondition BuildCheck(int depth = 0)
	{
		if (depth > 16 || m_sEntityName.IsEmpty() || m_iMatch < 0 || m_iMatch > 1)
			return null;
		GC_RuleCondition check = new GC_RuleCondition();
		check.m_iCheck = 11;
		check.m_sInteractionEntity = m_sEntityName;
		check.m_bReverseAnswer = m_iMatch == 1;
		return check;
	}
}

//! Weak component binding; callbacks mark accepted uses for the existing framework queue.
class GC_InteractionObservation
{
	protected GC_MissionInteractionComponent m_Component;
	protected bool m_bChanged;
	protected bool m_bAvailable;
	protected bool m_bReportedMissing;

	//------------------------------------------------------------------------------------------------
	void Bind(BaseWorld world, GC_RuleCondition check)
	{
		if (m_Component || !world)
			return;
		IEntity entity = world.FindEntityByName(check.m_sInteractionEntity);
		if (entity)
			m_Component = GC_MissionInteractionComponent.Cast(entity.FindComponent(GC_MissionInteractionComponent));
		if (!m_Component)
		{
			if (!m_bReportedMissing)
				Print("GC | Mission Framework | Interaction Condition cannot resolve '" + check.m_sInteractionEntity + "'. Retrying at Match start.", LogLevel.WARNING);
			m_bReportedMissing = true;
			return;
		}
		m_Component.GetOnUsed().Insert(OnUsed);
		m_bAvailable = true;
		m_bChanged = true;
	}

	//------------------------------------------------------------------------------------------------
	protected void OnUsed()
	{
		m_bChanged = true;
	}

	//------------------------------------------------------------------------------------------------
	bool IsAvailable()
	{
		return m_Component != null;
	}

	//------------------------------------------------------------------------------------------------
	bool HasBeenUsed()
	{
		return m_Component && m_Component.GetUseCount() > 0;
	}

	//------------------------------------------------------------------------------------------------
	bool ConsumeChange()
	{
		if (m_bAvailable != IsAvailable())
		{
			m_bAvailable = IsAvailable();
			m_bChanged = true;
		}
		bool changed = m_bChanged;
		m_bChanged = false;
		return changed;
	}

	//------------------------------------------------------------------------------------------------
	void Stop()
	{
		if (m_Component)
			m_Component.GetOnUsed().Remove(OnUsed);
		m_Component = null;
		m_bAvailable = false;
		m_bChanged = false;
	}
}
