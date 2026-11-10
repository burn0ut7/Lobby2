[BaseContainerProps(), BaseContainerCustomTitleField("m_sName")]
class GC_MissionFlag
{
	[Attribute("", UIWidgets.EditBox, "A name for a fact your mission remembers, such as radio_destroyed. Use this same name in events, conditions and actions.")]
	string m_sName;
	[Attribute("0", UIWidgets.SpinBox, "Remember this many successful calls when the framework starts. Default 0 means false; 1 or more means true. This does not perform a call. A positive count uses a non-repeatable flag's activation; enable Repeatable to accept more calls. Conditions control future calls. Flag Action > Reset to zero clears to zero, not this starting count.", "0 1000000 1")]
	int m_iStartCallCount;
	[Attribute("0", UIWidgets.CheckBox, "Unchecked: accept one successful call until Flag Action > Reset to zero is used. Checked: accept more calls, increasing the count and notifying dependent events each time. A remembered count of zero means false; one or more means true.")]
	bool m_bRepeatable;
	[Attribute("", UIWidgets.Object, "Requirements for accepting a call. No conditions means every otherwise eligible call is allowed; Start Call Count determines its initial true/false value. Failed conditions change neither its count nor its value. Use Count Condition, check Count Flag Calls, enter this flag's name and select Less than 5 to allow five calls. Conditions are checked before increasing the count. Conditions referencing other flags can activate this flag when those flags change; a check of its own count only limits calls.")]
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
