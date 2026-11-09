[BaseContainerProps(), BaseContainerCustomStringTitleField("Mission event")]
class GC_MissionEvent
{
	[Attribute("0", UIWidgets.ComboBox, "Choose what starts this rule. Match started happens after briefing. Flag events happen only when a flag changes.", enums: {ParamEnum("Match started", "0"), ParamEnum("Timer elapsed", "1"), ParamEnum("Named event received", "2"), ParamEnum("Flag turned On", "3"), ParamEnum("Flag turned Off", "4")})]
	GC_ERuleEvent m_eType;
	[Attribute("", UIWidgets.EditBox, "For Flag or Named event: the exact name to watch. Not used by Match started or Timer elapsed.")]
	string m_sName;
	[Attribute("5", UIWidgets.SpinBox, "For Timer elapsed: seconds after match start. With Advanced > Run = Every time, also the time between checks. Other events ignore this setting.", "0.1 86400 0.1")]
	float m_fSeconds;

	//------------------------------------------------------------------------------------------------
	bool ApplyTo(GC_CompiledMissionRule rule)
	{
		rule.m_eWhen = m_eType;
		rule.m_sTestEventName = m_sName;
		rule.m_fEverySeconds = m_fSeconds;
		return m_eType >= 0 && m_eType <= GC_ERuleEvent.FLAG_OFF;
	}
}
