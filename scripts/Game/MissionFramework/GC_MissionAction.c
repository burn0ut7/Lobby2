[BaseContainerProps(visible: false, insertable: false)]
class GC_MissionAction
{
	//------------------------------------------------------------------------------------------------
	GC_RuleAction BuildAction()
	{
		return null;
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Log message")]
class GC_LogMessageAction : GC_MissionAction
{
	[Attribute("Test complete", UIWidgets.EditBox, "The message to write to the Workbench log. This is a test message, not a message shown to players.")]
	string m_sMessage;

	//------------------------------------------------------------------------------------------------
	override GC_RuleAction BuildAction()
	{
		GC_RuleAction action = new GC_RuleAction();
		action.m_eDo = GC_ERuleAction.WRITE_LOG;
		action.m_sTextOrName = m_sMessage;
		return action;
	}
}

[BaseContainerProps(), BaseContainerCustomStringTitleField("Set flag")]
class GC_SetFlagAction : GC_MissionAction
{
	[Attribute("", UIWidgets.EditBox, "Give the flag a name. Other rules can watch this same name. Flags start Off and reset when the world restarts.")]
	string m_sFlagName;
	[Attribute("1", UIWidgets.ComboBox, "Choose the new setting for this flag.", enums: {ParamEnum("On", "1"), ParamEnum("Off", "0")})]
	int m_iSetTo;

	//------------------------------------------------------------------------------------------------
	override GC_RuleAction BuildAction()
	{
		if (m_iSetTo < 0 || m_iSetTo > 1)
			return null;
		GC_RuleAction action = new GC_RuleAction();
		action.m_eDo = GC_ERuleAction.SET_FLAG;
		action.m_sTextOrName = m_sFlagName;
		action.m_bFlagAnswer = m_iSetTo == 1;
		return action;
	}
}

[BaseContainerProps(category: "Advanced"), BaseContainerCustomStringTitleField("Send test event")]
class GC_SendTestEventAction : GC_MissionAction
{
	[Attribute("", UIWidgets.EditBox, "Rules waiting for this event name will run. An event is sent once; it is not remembered like a flag.")]
	string m_sEventName;

	//------------------------------------------------------------------------------------------------
	override GC_RuleAction BuildAction()
	{
		GC_RuleAction action = new GC_RuleAction();
		action.m_eDo = GC_ERuleAction.SEND_TEST_EVENT;
		action.m_sTextOrName = m_sEventName;
		return action;
	}
}
