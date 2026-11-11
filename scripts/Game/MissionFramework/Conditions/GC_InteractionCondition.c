[BaseContainerProps(), BaseContainerCustomStringTitleField("Interaction Condition")]
class GC_InteractionCondition : GC_ConditionBase
{
	[Attribute("", UIWidgets.EditComboBox, "Exact unique world name of the object carrying GC_MissionInteractionComponent. Place it before Match starts. True only during the framework check caused by using this object. Resets afterward even if other conditions fail. Use a Mission Flag to remember success.", enums: {ParamEnum("", "", "rst-choices:2:GC_InteractionEntityChoices")})]
	string m_sEntityName;

	//------------------------------------------------------------------------------------------------
	override GC_RuleCondition BuildCheck(int depth = 0)
	{
		if (depth > 16 || m_sEntityName.IsEmpty())
			return null;
		GC_RuleCondition check = new GC_RuleCondition();
		check.m_iCheck = 11;
		check.m_sInteractionEntity = m_sEntityName;
		return check;
	}
}

//! Weak component binding; callbacks mark accepted uses for the existing framework queue.
class GC_InteractionObservation
{
	protected GC_MissionInteractionComponent m_Component;
	protected bool m_bChanged;
	protected bool m_bTriggered;
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
	}

	//------------------------------------------------------------------------------------------------
	protected void OnUsed()
	{
		m_bTriggered = true;
		m_bChanged = true;
	}

	//------------------------------------------------------------------------------------------------
	bool IsAvailable()
	{
		return m_Component != null;
	}

	//------------------------------------------------------------------------------------------------
	bool IsTriggered()
	{
		return IsAvailable() && m_bTriggered;
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
	void ResetPulse()
	{
		m_bTriggered = false;
	}

	//------------------------------------------------------------------------------------------------
	void Stop()
	{
		if (m_Component)
			m_Component.GetOnUsed().Remove(OnUsed);
		ResetPulse();
		m_Component = null;
		m_bAvailable = false;
		m_bChanged = false;
	}
}
