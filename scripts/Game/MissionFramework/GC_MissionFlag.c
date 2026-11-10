[BaseContainerProps(), BaseContainerCustomTitleField("m_sName")]
class GC_MissionFlag
{
	[Attribute("", UIWidgets.EditBox, "A name for a fact your mission remembers, such as radio_destroyed. Use this same name in events, conditions and actions.")]
	string m_sName;
	[Attribute("", UIWidgets.Object, "Requirements for accepting a call. No conditions means every otherwise eligible call is allowed; Start Call Count determines its initial true/false value. Failed conditions change neither its count nor its value. Use Count Condition, check Count Flag Calls, enter this flag's name and select Less than 5 to accept calls while the remembered count is below five. Call checks conditions before increasing the count. Direct count edits from Flag Action do not check these activation conditions. Conditions referencing other flags can activate this flag when those flags change; a check of its own count only limits calls.")]
	ref array<ref GC_ConditionBase> m_aConditions;

	[Attribute("", UIWidgets.Object, "Optional advanced settings. Leave empty to start at zero calls (false) and accept one successful call. Expand to set a starting count or allow repeated calls.")]
	ref GC_FlagOptions m_Advanced;

	//------------------------------------------------------------------------------------------------
	int GetStartCallCount()
	{
		if (!m_Advanced)
			return 0;
		return m_Advanced.m_iStartCallCount;
	}

	//------------------------------------------------------------------------------------------------
	bool IsRepeatable()
	{
		return m_Advanced && m_Advanced.m_bRepeatable;
	}

	//------------------------------------------------------------------------------------------------
	GC_RuleCondition BuildConditions()
	{
		if (!m_aConditions || m_aConditions.IsEmpty())
			return null;
		return GC_ConditionBase.BuildGroup(m_aConditions, GC_ERuleCheck.ALL, 0, 0);
	}
}

[BaseContainerProps(visible: false, insertable: false), BaseContainerCustomStringTitleField("Legacy Random Settings - use Random Chance Condition or Random Event Group")]
class GC_RuleRandom
{
	[Attribute("0", UIWidgets.CheckBox, "Checked: choose one random action. Unchecked: run all actions in list order.")]
	bool m_bPickOneRandomAction;
	[Attribute("100", UIWidgets.SpinBox, "Percent chance each time the event happens and conditions pass. 0 never runs; 100 always runs. A failed chance waits for the next event.", "0 100 1")]
	float m_fChancePercent;
	[Attribute("0", UIWidgets.SpinBox, "Shortest extra wait after the event. Added to the mission event's fixed wait. Conditions are checked again when the wait ends.", "0 86400 0.1")]
	float m_fMinimumSeconds;
	[Attribute("0", UIWidgets.SpinBox, "Longest extra wait. Must be at least Minimum Seconds. The wait is chosen once per accepted event.", "0 86400 0.1")]
	float m_fMaximumSeconds;

	//------------------------------------------------------------------------------------------------
	bool IsValid()
	{
		return m_fChancePercent >= 0 && m_fChancePercent <= 100 && m_fMinimumSeconds >= 0
			&& m_fMaximumSeconds >= m_fMinimumSeconds && m_fMaximumSeconds <= 86400;
	}
}
