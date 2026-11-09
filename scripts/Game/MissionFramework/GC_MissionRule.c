[BaseContainerProps(), BaseContainerCustomStringTitleField("Mission event options")]
class GC_RuleOptions
{
	[Attribute("0", UIWidgets.ComboBox, "Once is enough for most mission events. Every time runs again when the chosen event happens again.", enums: {ParamEnum("Once", "0"), ParamEnum("Every time", "1")})]
	int m_iRun;
	[Attribute("0", UIWidgets.SpinBox, "Wait this many seconds before doing the actions. Zero means now. Requirements must still be met when the wait ends.", "0 86400 0.1")]
	float m_fWaitSeconds;
	[Attribute("0", UIWidgets.CheckBox, "Keep this mission event in the list, but do not run it.")]
	bool m_bDisabled;
}

[BaseContainerProps(), BaseContainerCustomTitleField("m_sName")]
class GC_MissionRule
{
	[Attribute("Mission event", UIWidgets.EditBox, "Give this mission event a short name, such as Start the test.")]
	string m_sName;
	[Attribute("", UIWidgets.Object, "The event that starts this mission event. Leave empty to run when the match begins, after briefing.")]
	ref GC_MissionEvent m_Event;
	[Attribute("", UIWidgets.Object, "The actions to perform when this mission event runs. Actions run from top to bottom.")]
	ref array<ref GC_MissionAction> m_aActions;
	[Attribute("", UIWidgets.Object, "Optional conditions. Leave empty for no restrictions. Every condition in this list must be met.")]
	ref array<ref GC_MissionCondition> m_aConditions;
	[Attribute("", UIWidgets.Object, "Optional. Change how often this mission event runs, add a delay, or turn it off. Leave empty to run once, without waiting.")]
	ref GC_RuleOptions m_Advanced;
	[Attribute("", UIWidgets.Object, "Optional chance to run or a random extra wait. Leave empty for no randomness.")]
	ref GC_RuleRandom m_Random;

	//------------------------------------------------------------------------------------------------
	GC_CompiledMissionRule BuildRule()
	{
		GC_CompiledMissionRule rule = new GC_CompiledMissionRule();
		rule.m_sName = m_sName;
		rule.m_bEnabled = true;
		rule.m_bRunOnlyOnce = true;
		rule.m_fEverySeconds = 5;
		rule.m_aThen = {};
		rule.m_Random = m_Random;
		if (m_Event && !m_Event.ApplyTo(rule))
			return null;
		if (m_Advanced)
		{
			if (m_Advanced.m_iRun < 0 || m_Advanced.m_iRun > 1)
				return null;
			rule.m_bEnabled = !m_Advanced.m_bDisabled;
			rule.m_bRunOnlyOnce = m_Advanced.m_iRun == 0;
			rule.m_fWaitSeconds = m_Advanced.m_fWaitSeconds;
		}
		if (!m_aActions || m_aActions.IsEmpty() || m_aActions.Count() > 256)
			return null;
		foreach (GC_MissionAction definition : m_aActions)
		{
			if (!definition)
				return null;
			GC_RuleAction action = definition.BuildAction();
			if (!action || !action.IsValid())
				return null;
			rule.m_aThen.Insert(action);
		}
		if (m_aConditions && !m_aConditions.IsEmpty())
		{
			rule.m_If = GC_MissionCondition.BuildGroup(m_aConditions, GC_ERuleCheck.ALL, 0, 0);
			if (!rule.m_If)
				return null;
		}
		if (!rule.IsValid())
			return null;
		return rule;
	}
}
