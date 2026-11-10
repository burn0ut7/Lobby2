[BaseContainerProps(visible: false, insertable: false)]
class GC_MissionAction
{
	//------------------------------------------------------------------------------------------------
	GC_RuleAction BuildAction()
	{
		return null;
	}
}

[BaseContainerProps(category: "Advanced"), BaseContainerCustomStringTitleField("Log Message Action")]
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

[BaseContainerProps(visible: false, insertable: false), BaseContainerCustomStringTitleField("Legacy Set Flag - use Flag Action")]
class GC_SetFlagAction : GC_MissionAction
{
	[Attribute("", UIWidgets.EditBox, "Give the flag a name. Other mission events can watch this same name. Declared flags use Start Call Count; undeclared flags default to false with zero calls. Fresh startup restores configured starting counts.")]
	string m_sFlagName;
	[Attribute("1", UIWidgets.CheckBox, "Checked: set true, calling a declared flag with its conditions and count rules. Unchecked: reset count to zero and make false.")]
	bool m_bSetTrue;

	//------------------------------------------------------------------------------------------------
	override GC_RuleAction BuildAction()
	{
		GC_RuleAction action = new GC_RuleAction();
		action.m_eDo = GC_ERuleAction.SET_FLAG;
		action.m_sTextOrName = m_sFlagName;
		action.m_bFlagAnswer = m_bSetTrue;
		return action;
	}
}

[BaseContainerProps(visible: false, insertable: false), BaseContainerCustomStringTitleField("Legacy Test Event")]
class GC_SendTestEventAction : GC_MissionAction
{
	[Attribute("", UIWidgets.EditBox, "Mission events waiting for this event name will run. An event is sent once; it is not remembered like a flag.")]
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

[BaseContainerProps(visible: false, insertable: false), BaseContainerCustomStringTitleField("Legacy Random Flag")]
class GC_RandomFlagAction : GC_MissionAction
{
	[Attribute("", UIWidgets.EditBox, "The flag to set. Success sets it true; failure resets its count to zero and makes it false.")]
	string m_sFlagName;
	[Attribute("50", UIWidgets.SpinBox, "Percent chance of true. 0 always sets false; 100 always sets true.", "0 100 1")]
	float m_fChancePercent;

	//------------------------------------------------------------------------------------------------
	override GC_RuleAction BuildAction()
	{
		GC_RuleAction action = new GC_RuleAction();
		action.m_eDo = GC_ERuleAction.RANDOM_FLAG;
		action.m_sTextOrName = m_sFlagName;
		action.m_fChancePercent = m_fChancePercent;
		return action;
	}
}

[BaseContainerProps(visible: false, insertable: false), BaseContainerCustomStringTitleField("Legacy Select Random Flags")]
class GC_SelectRandomFlagsAction : GC_MissionAction
{
	[Attribute("", UIWidgets.EditBox, "Possible flag names, such as route_north and route_south. Names must be unique. Selected flags turn true; the others reset their counts to zero and become false.")]
	ref array<string> m_aFlagNames;
	[Attribute("1", UIWidgets.SpinBox, "How many different flags to turn true. Each flag can be selected only once per action.", "1 256 1")]
	int m_iSelectionCount;

	//------------------------------------------------------------------------------------------------
	override GC_RuleAction BuildAction()
	{
		GC_RuleAction action = new GC_RuleAction();
		action.m_eDo = GC_ERuleAction.SELECT_FLAGS;
		action.m_aFlagNames = {};
		if (m_aFlagNames)
			action.m_aFlagNames.Copy(m_aFlagNames);
		action.m_iSelectionCount = m_iSelectionCount;
		return action;
	}
}

[BaseContainerProps(visible: false, insertable: false), BaseContainerCustomStringTitleField("Legacy Call Flag - use Flag Action")]
class GC_CallFlagAction : GC_MissionAction
{
	[Attribute("", UIWidgets.EditBox, "The exact name in Mission Flags. If its conditions pass, increase its count, turn it true and check dependent events and flags. The flag stays true until Reset Flag clears its count.")]
	string m_sFlagName;

	//------------------------------------------------------------------------------------------------
	override GC_RuleAction BuildAction()
	{
		GC_RuleAction action = new GC_RuleAction();
		action.m_eDo = GC_ERuleAction.CALL_FLAG;
		action.m_sTextOrName = m_sFlagName;
		return action;
	}
}

[BaseContainerProps(visible: false, insertable: false), BaseContainerCustomStringTitleField("Legacy Reset Flag - use Flag Action")]
class GC_ResetFlagAction : GC_MissionAction
{
	[Attribute("", UIWidgets.EditBox, "Exact Mission Flag name. Clears successful calls to zero, making the flag false and allowing it to activate again. Notifies dependent events when it changes. Does not reset mission events. The same flag can still activate only once per notification chain.")]
	string m_sFlagName;

	//------------------------------------------------------------------------------------------------
	override GC_RuleAction BuildAction()
	{
		GC_RuleAction action = new GC_RuleAction();
		action.m_eDo = GC_ERuleAction.RESET_FLAG;
		action.m_sTextOrName = m_sFlagName;
		return action;
	}
}
