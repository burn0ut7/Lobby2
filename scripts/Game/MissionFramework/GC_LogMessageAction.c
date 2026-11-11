//! Serialized compatibility only. New authoring uses Message Action / Server log.
[BaseContainerProps(visible: false, insertable: false), BaseContainerCustomStringTitleField("Log Message Action (legacy)")]
class GC_LogMessageAction : GC_ActionBase
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
