[BaseContainerProps(), BaseContainerCustomTitleField("m_sName")]
class GC_MissionFlag
{
	[Attribute("", UIWidgets.EditBox, "A name for a fact your mission remembers, such as radio_destroyed. Use this same name in events, conditions and actions.")]
	string m_sName;
	[Attribute("0", UIWidgets.CheckBox, "Checked: the flag starts true. Unchecked: it starts false. Applied before match-start events, without sending a flag-change notification.")]
	bool m_bStartsTrue;
	[Attribute("0", UIWidgets.CheckBox, "Unchecked: accept one call and stay true. Checked: become true for one check cycle, then reset to false so a later call can activate it again. The remembered call count never resets here.")]
	bool m_bRepeatable;
	[Attribute("", UIWidgets.Object, "Requirements for accepting a call. No conditions means always true. Use Count Condition, check Count Flag Calls, enter this flag's name and select Less than 5 to allow five calls. Conditions are checked before increasing the count. Conditions referencing other flags can activate this flag when those flags change; a check of its own count only limits calls.")]
	ref array<ref GC_ConditionBase> m_aConditions;

	//------------------------------------------------------------------------------------------------
	GC_RuleCondition BuildConditions()
	{
		if (!m_aConditions || m_aConditions.IsEmpty())
			return null;
		return GC_ConditionBase.BuildGroup(m_aConditions, GC_ERuleCheck.ALL, 0, 0);
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Random settings")]
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
