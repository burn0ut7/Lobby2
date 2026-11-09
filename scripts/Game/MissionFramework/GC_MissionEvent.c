[BaseContainerProps(visible: false, insertable: false)]
class GC_MissionEvent
{
	//------------------------------------------------------------------------------------------------
	bool ApplyTo(GC_MissionRule rule)
	{
		return false;
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Match started")]
class GC_MatchStartEvent : GC_MissionEvent
{
	//------------------------------------------------------------------------------------------------
	override bool ApplyTo(GC_MissionRule rule)
	{
		rule.m_eWhen = GC_ERuleEvent.MATCH_BEGINS;
		return true;
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Flag changed")]
class GC_FlagChangedEvent : GC_MissionEvent
{
	[Attribute("", UIWidgets.EditBox, "The flag to watch. Use the same name in the action that changes it.")]
	string m_sFlagName;
	[Attribute("1", UIWidgets.ComboBox, "Run when this flag changes to this setting. A flag starts Off. Setting it to its current value does not run the rule again.", enums: {ParamEnum("On", "1"), ParamEnum("Off", "0")})]
	int m_iBecomes;

	//------------------------------------------------------------------------------------------------
	override bool ApplyTo(GC_MissionRule rule)
	{
		if (m_sFlagName.IsEmpty() || m_iBecomes < 0 || m_iBecomes > 1)
			return false;
		rule.m_eWhen = GC_ERuleEvent.FLAG_OFF;
		if (m_iBecomes == 1)
			rule.m_eWhen = GC_ERuleEvent.FLAG_ON;
		rule.m_sTestEventName = m_sFlagName;
		return true;
	}
}

[BaseContainerProps(category: "Advanced"), BaseContainerCustomStringTitleField("Timer elapsed")]
class GC_TimerEvent : GC_MissionEvent
{
	[Attribute("5", UIWidgets.SpinBox, "Seconds after match start. With Advanced > Run set to Every time, this is also the time between runs.", "0.1 86400 0.1")]
	float m_fSeconds;

	//------------------------------------------------------------------------------------------------
	override bool ApplyTo(GC_MissionRule rule)
	{
		rule.m_eWhen = GC_ERuleEvent.TIMER;
		rule.m_fEverySeconds = m_fSeconds;
		return true;
	}
}

[BaseContainerProps(category: "Advanced"), BaseContainerCustomStringTitleField("Test event received")]
class GC_TestEvent : GC_MissionEvent
{
	[Attribute("", UIWidgets.EditBox, "Use the same name as the Send test event action.")]
	string m_sEventName;

	//------------------------------------------------------------------------------------------------
	override bool ApplyTo(GC_MissionRule rule)
	{
		rule.m_eWhen = GC_ERuleEvent.TEST_EVENT;
		rule.m_sTestEventName = m_sEventName;
		return !m_sEventName.IsEmpty();
	}
}
