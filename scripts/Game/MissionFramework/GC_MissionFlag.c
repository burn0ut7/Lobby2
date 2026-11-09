[BaseContainerProps(), BaseContainerCustomTitleField("m_sName")]
class GC_MissionFlag
{
	[Attribute("", UIWidgets.EditBox, "A name for a fact your mission remembers, such as radio_destroyed. Use this same name in events, conditions and actions.")]
	string m_sName;
	[Attribute("0", UIWidgets.ComboBox, "The value before match-start rules run. This starting value does not send a Flag changed event.", enums: {ParamEnum("Off", "0"), ParamEnum("On", "1")})]
	int m_iStartingValue;
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Random settings")]
class GC_RuleRandom
{
	[Attribute("0", UIWidgets.ComboBox, "All actions uses the normal list order. One random action chooses a single action each time the rule runs; no flags are needed.", enums: {ParamEnum("All actions", "0"), ParamEnum("One random action", "1")})]
	int m_iActionSelection;
	[Attribute("100", UIWidgets.SpinBox, "Percent chance each time the event happens and conditions pass. 0 never runs; 100 always runs. A failed chance waits for the next event.", "0 100 1")]
	float m_fChancePercent;
	[Attribute("0", UIWidgets.SpinBox, "Shortest extra wait after the event. Added to the rule's fixed wait. Conditions are checked again when the wait ends.", "0 86400 0.1")]
	float m_fMinimumSeconds;
	[Attribute("0", UIWidgets.SpinBox, "Longest extra wait. Must be at least Minimum Seconds. The wait is chosen once per accepted event.", "0 86400 0.1")]
	float m_fMaximumSeconds;

	//------------------------------------------------------------------------------------------------
	bool IsValid()
	{
		return m_iActionSelection >= 0 && m_iActionSelection <= 1 && m_fChancePercent >= 0 && m_fChancePercent <= 100 && m_fMinimumSeconds >= 0
			&& m_fMaximumSeconds >= m_fMinimumSeconds && m_fMaximumSeconds <= 86400;
	}
}
